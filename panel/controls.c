/*  $Id$
 *  
 *  Copyright 2002-2004 Jasper Huijsmans (jasper@xfce.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/**
 *  Controls
 *  --------
 *  Controls provide a unified interface for panel items. Each control has an
 *  associated control class that defines interface functions for the control.
 *  Each class in turn has an ControlClassInfo parent structure, that contains
 *  private info like refcount and uniqueness setting.
 *
 *  A control class can be provided by a plugin. The plugin is accessed 
 *  using the g_module interface.
 *  
 *  A control is created using the create_control function of its associated
 *  class. The module is loaded if necessary. This function takes an allocated 
 *  control structure as argument, containing only a base container widget.
 * 
 *  On startup a list of ControlClassInfo structures is made from which new 
 *  controls can be created.
 *
 *  Periodically the panel will check the list and unload modules that are no
 *  longer in use.
 *  
 **/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4util/libxfce4util.h>

#include "xfce.h"
#include "item.h"
#include "popup.h"
#include "groups.h"
#include "plugins.h"
#include "controls_dialog.h"
#include "add-control-dialog.h"
#include "settings.h"

#define SOEXT 		("." G_MODULE_SUFFIX)
#define SOEXT_LEN 	(strlen (SOEXT))

#define API_VERSION     5

#define UNLOAD_TIMEOUT  30000	/* 30 secs */

typedef struct _ControlClassInfo ControlClassInfo;

struct _ControlClassInfo
{
    ControlClass *class;

    /* Copied from ControlClass. This allows use of 
     * these values when the module is unloaded */
    char *name;
    char *caption;

    /* private info */
    char *path;
    int refcount;
    gboolean unique;
    GdkPixbuf *icon;
    gboolean unloadable;
};


static GSList *control_class_info_list = NULL;
static ControlClassInfo *info_to_add = NULL;

static int unload_id = 0;
static int unloading = 0;

/* not static, must be available in panel.c for handle menu */
G_MODULE_EXPORT /* EXPORT:popup_control */
Control *popup_control = NULL;

/* convenience function */
static void
wait_for_unloading (void)
{
    while (unloading)
    {
	DBG ("unloading in progress");
	g_usleep (200 * 1000);  /* 200 msec */
    }

}

/*  ControlClass and ControlClassInfo 
 *  ---------------------------------
*/

static gint
compare_class_info_by_name (gconstpointer info, gconstpointer name)
{
    g_assert (info != NULL);
    g_assert (name != NULL);

    return g_ascii_strcasecmp (((ControlClassInfo *) info)->name,
			       (char *) name);
}

static gint
lookup_info_by_filename (gconstpointer info, gconstpointer filename)
{
    char *fn;
    int result = -1;

    g_assert (info != NULL);
    g_assert (filename != NULL);

    if (((ControlClassInfo *) info)->class->gmodule)
    {
	fn = g_path_get_basename (g_module_name
				  (((ControlClassInfo *) info)->class->
				   gmodule));

	result = g_ascii_strcasecmp (fn, filename);
	g_free (fn);
    }

    return result;
}

static void
control_class_info_free (ControlClassInfo * cci)
{
    g_free (cci->path);
    g_free (cci->name);
    g_free (cci->caption);
    
    if (cci->icon)
	g_object_unref (cci->icon);

    g_free (cci);
}

static void
control_class_free (ControlClass * cc)
{
    if (cc->id == PLUGIN)
    {
	if (cc->gmodule)
	    g_module_close (cc->gmodule);

	g_free (cc->filename);
    }

    g_free (cc);
}

static ControlClassInfo *
get_control_class_info (ControlClass * cc)
{
    ControlClassInfo *info;

    g_return_val_if_fail (cc->name != NULL, NULL);

    if (info_to_add &&
	g_ascii_strcasecmp (info_to_add->class->name, cc->name) == 0)
    {
	info = info_to_add;
    }
    else
    {
	GSList *li = g_slist_find_custom (control_class_info_list,
					  cc->name,
					  compare_class_info_by_name);

	info = li->data;
    }

    return info;
}

static gboolean
control_class_info_create_control (ControlClassInfo * info, Control * control)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (control != NULL, FALSE);

    if (info->class->id == ICON)
    {
	info->class->create_control (control);
	return TRUE;
    }

    wait_for_unloading ();

    if (info->class->id == PLUGIN && info->refcount == 0 &&
	info->class->gmodule == NULL)
    {
	GModule *gm;
	gpointer symbol;
	void (*init) (ControlClass * cc);

	gm = info->class->gmodule = g_module_open (info->path, 0);

	if (!g_module_symbol (gm, "xfce_control_class_init", &symbol))
	    goto failed;

	init = symbol;
	init (info->class);
    }

    if (info->unique && info->refcount > 0)
	goto failed;

    info->refcount++;

    return info->class->create_control (control);

  failed:
    return FALSE;
}

/* plugins */

G_MODULE_EXPORT /* EXPORT:xfce_plugin_check_version */
gchar *
xfce_plugin_check_version (gint version)
{
    if (version != API_VERSION)
	return "Incompatible plugin version";
    else
	return NULL;
}

static void
load_plugin (gchar * path)
{
    GModule *gm;
    gpointer symbol;

    DBG ("Load module: %s", path);

    gm = g_module_open (path, 0);

    if (gm && g_module_symbol (gm, "xfce_control_class_init", &symbol))
    {
	ControlClassInfo *info;
	ControlClass *cc;
	void (*init) (ControlClass * cc);

	info = g_new0 (ControlClassInfo, 1);
	cc = info->class = g_new0 (ControlClass, 1);

	info->unloadable = TRUE;

	cc->id = PLUGIN;
	cc->gmodule = gm;

	/* keep track of info structure we are about to add */
	info_to_add = info;

	/* fill in the class structure */
	init = symbol;
	init (cc);

	info_to_add = NULL;

	if (g_slist_find_custom (control_class_info_list,
				 cc->name, compare_class_info_by_name))
	{
	    control_class_free (cc);
	    control_class_info_free (info);
	}
	else
	{
	    control_class_info_list =
		g_slist_append (control_class_info_list, info);

	    cc->filename = g_path_get_basename (path);
	    info->path = g_strdup (path);
	    info->name = g_strdup (cc->name);
	    info->caption = g_strdup (cc->caption);
	}
    }
    else if (gm)
    {
	g_warning ("%s: incompatible module %s", PACKAGE, path);

	g_module_close (gm);
    }
    else
    {
	g_warning ("%s: module %s cannot be opened (%s)",
		   PACKAGE, path, g_module_error ());
    }
}

static void
load_plugin_dir (const char *dir)
{
    GDir *gdir = g_dir_open (dir, 0, NULL);
    const char *file;

    if (!gdir)
	return;

    while ((file = g_dir_read_name (gdir)))
    {
	const char *s = file;

	s += strlen (file) - SOEXT_LEN;

	if (strequal (s, SOEXT))
	{
	    char *path = g_build_filename (dir, file, NULL);

	    load_plugin (path);
	    g_free (path);
	}
    }

    g_dir_close (gdir);
}

static void
add_plugin_classes (void)
{
    char *dir;

    wait_for_unloading ();
    unloading++;

    dir = g_build_filename (LIBDIR, PLUGINDIR, NULL);

    load_plugin_dir (dir);

    unloading--;
    g_free (dir);
}

static void
clean_plugin_classes (void)
{
    GSList *li, *prev;

    prev = control_class_info_list;

    for (li = control_class_info_list->next; li; li = li->next)
    {
	ControlClassInfo *info = li->data;

	if (g_file_test (info->path, G_FILE_TEST_EXISTS)
	    || info->refcount > 0)
	{
	    DBG ("plugin %s exists, %d in use", info->caption,
		 info->refcount);
	    prev = li;
	}
	else
	{
	    GSList *tmp;

	    DBG ("plugin %s (%s) was removed", info->name, info->path);

	    tmp = li;
	    li = prev;
	    li->next = tmp->next;
	    g_slist_free_1 (tmp);

	    control_class_free (info->class);
	    control_class_info_free (info);
	}
    }
}

/*  builtin launcher class */

static void
add_launcher_class (void)
{
    ControlClassInfo *info;
    ControlClass *cc;

    info = g_new0 (ControlClassInfo, 1);
    cc = info->class = g_new0 (ControlClass, 1);

    panel_item_class_init (cc);

    info->name = g_strdup (cc->name);
    info->caption = g_strdup (cc->caption);

    control_class_info_list = g_slist_append (NULL, info);
}

/* module unloading timeout */

static gboolean
unload_modules (void)
{
    GSList *li;

    if (unloading)
	return TRUE;

    unloading++;

    /* first item is launcher */
    for (li = control_class_info_list->next; li != NULL; li = li->next)
    {
	ControlClassInfo *info = li->data;

	if (info->unloadable &&
	    info->class->id == PLUGIN &&
	    info->refcount == 0 && info->class->gmodule != NULL)
	{
	    DBG ("unload %s", info->caption);

	    g_module_close (info->class->gmodule);
	    info->class->gmodule = NULL;
	}
    }

    unloading--;

    return TRUE;
}

/* exported interface */

G_MODULE_EXPORT /* EXPORT:control_class_list_init */
void
control_class_list_init (void)
{
    /* prepend class info to the list */
    add_launcher_class ();
    add_plugin_classes ();

    unload_id =
	g_timeout_add (UNLOAD_TIMEOUT, (GSourceFunc) unload_modules, NULL);
}

G_MODULE_EXPORT /* EXPORT:control_class_list_cleanup */
void
control_class_list_cleanup (void)
{
    GSList *li;

    unloading++;

    if (unload_id)
    {
	g_source_remove (unload_id);
	unload_id = 0;
    }

    for (li = control_class_info_list; li; li = li->next)
    {
	ControlClassInfo *info = li->data;

	control_class_free (info->class);
	control_class_info_free (info);
    }

    g_slist_free (control_class_info_list);
    control_class_info_list = NULL;
    unloading--;
}

G_MODULE_EXPORT /* EXPORT:control_class_set_unique */
void
control_class_set_unique (ControlClass * cclass, gboolean unique)
{
    ControlClassInfo *info;

    info = get_control_class_info (cclass);

    if (info)
	info->unique = unique;
}

G_MODULE_EXPORT /* EXPORT:control_class_set_icon */
void
control_class_set_icon (ControlClass * cclass, GdkPixbuf * icon)
{
    ControlClassInfo *info;

    info = get_control_class_info (cclass);

    if (info)
    {
	if (info->icon)
	    g_object_unref (info->icon);

	if (icon)
	    info->icon = g_object_ref (icon);
	else
	    info->icon = NULL;
    }
}

G_MODULE_EXPORT /* EXPORT:control_class_set_unloadable */
void
control_class_set_unloadable (ControlClass * cclass, gboolean unloadable)
{
    ControlClassInfo *info;

    info = get_control_class_info (cclass);

    if (info)
	info->unloadable = unloadable;
}


/* not in header, but exported for groups.c */

G_MODULE_EXPORT /* EXPORT:control_class_unref */
void
control_class_unref (ControlClass * cclass)
{
    ControlClassInfo *info;

    info = get_control_class_info (cclass);

    if (info && info->refcount > 0)
	info->refcount--;
}

static gint
sort_control_func (gpointer a, gpointer b)
{
    ControlInfo *ca = (ControlInfo *) a;
    ControlInfo *cb = (ControlInfo *) b;

    if ((!ca) || (!ca->caption))
	return -1;

    if ((!cb) || (!cb->caption))
	return 1;

    return g_utf8_collate (ca->caption, cb->caption);
}

static ControlInfo *
create_control_info (ControlClassInfo * info)
{
    ControlInfo *ci;

    ci = g_new0 (ControlInfo, 1);

    ci->name = g_strdup (info->name);
    ci->caption = g_strdup (info->caption);
    ci->icon = info->icon != NULL ? g_object_ref (info->icon) : NULL;

    ci->can_be_added = !(info->unique && info->refcount > 0);

    return ci;
}

/* for add-controls-dialog.c */
G_MODULE_EXPORT /* EXPORT:get_control_info_list */
GSList *
get_control_info_list (void)
{
    GSList *li, *infolist = NULL;
    ControlInfo *launcherinfo;

    /* good place to remove plugins that are not in use and were
     * uninstalled after the panel was started */
    clean_plugin_classes ();

    /* update module list */
    add_plugin_classes ();

    launcherinfo =
	create_control_info ((ControlClassInfo *) control_class_info_list->
			     data);

    for (li = control_class_info_list->next; li; li = li->next)
    {
	ControlClassInfo *info = li->data;

	infolist = g_slist_prepend (infolist, create_control_info (info));
    }

    /* sort alphabetically, but keep launcher at the top */
    infolist = g_slist_sort (infolist, (GCompareFunc) sort_control_func);

    infolist = g_slist_prepend (infolist, launcherinfo);

    return infolist;
}

G_MODULE_EXPORT /* EXPORT:insert_control */
void
insert_control (Panel * panel, const char *name, int position)
{
    gboolean hidden = settings.autohide;
    Control *control;
    ControlClassInfo *info;
    GSList *list;

    if (hidden)
    {
	DBG ("unhide before adding new item");
	panel_set_autohide (FALSE);

	while (gtk_events_pending ())
	    gtk_main_iteration ();
    }

    list = g_slist_find_custom (control_class_info_list, name,
				compare_class_info_by_name);

    if (G_UNLIKELY (list == NULL))
    {
	g_warning ("Could not load item \"%s\"", name);

	goto out;
    }

    info = list->data;

    control = control_new (position);
    control->cclass = info->class;

    if (control_class_info_create_control (info, control))
    {
	hide_current_popup_menu ();
	groups_add_control (control, position);
	control_attach_callbacks (control);
	control_set_settings (control);

	controls_dialog (control);

	write_panel_config ();
    }
    else
    {
	control_free (control);

	xfce_err (_("Could not create panel item \"%s\"."), info->caption);
    }

  out:
    popup_control = NULL;

    if (hidden)
	panel_set_autohide (TRUE);
}

static void
run_add_control_dialog (void)
{
    int position;

    position = popup_control ? popup_control->index : -1;

    add_control_dialog (&panel, position);
}

/*  Controls
 *  --------
*/

/* right-click menu */

static void
edit_control (void)
{
    if (popup_control)
	controls_dialog (popup_control);

    popup_control = NULL;
}

static void
remove_control (void)
{
    if (popup_control)
    {
	PanelPopup *pp;

	pp = groups_get_popup (popup_control->index);

	if (!(popup_control->with_popup) || !pp || pp->items == NULL ||
	    xfce_confirm (_("Removing the item will also remove "
			    "its popup menu."), GTK_STOCK_REMOVE, NULL))
	{
	    groups_remove (popup_control->index);
	}
    }

    popup_control = NULL;
}

static void
add_control (GtkWidget * w, ControlClassInfo * info)
{
    gboolean hidden = settings.autohide;
    Control *control;
    int index;

    if (hidden)
    {
	DBG ("unhide before adding new item");
	panel_set_autohide (FALSE);

	while (gtk_events_pending ())
	    gtk_main_iteration ();
    }

    index = popup_control ? popup_control->index : -1;

    control = control_new (index);
    control->cclass = info->class;

    if (control_class_info_create_control (info, control))
    {
	groups_add_control (control, index);
	control_attach_callbacks (control);
	control_set_settings (control);

	controls_dialog (control);

	write_panel_config ();
    }
    else
    {
	control_free (control);

	xfce_err (_("Could not create panel item \"%s\"."), info->caption);
    }

    popup_control = NULL;

    if (hidden)
	panel_set_autohide (TRUE);
}

static GtkWidget *
get_control_menu (void)
{
    static GtkWidget *menu = NULL;
    GtkWidget *mi;

    if (!menu)
    {
        xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

	menu = gtk_menu_new ();

	/* replaced with actual name */
	mi = gtk_menu_item_new_with_label ("Item");
	gtk_widget_show (mi);
	gtk_widget_set_sensitive (mi, FALSE);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);


	mi = gtk_separator_menu_item_new ();
	gtk_widget_show (mi);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	mi = gtk_menu_item_new_with_mnemonic (_("_Properties..."));
	gtk_widget_show (mi);
	g_signal_connect (mi, "activate", G_CALLBACK (edit_control), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	mi = gtk_menu_item_new_with_mnemonic (_("_Remove"));
	gtk_widget_show (mi);
	g_signal_connect (mi, "activate", G_CALLBACK (remove_control), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	mi = gtk_separator_menu_item_new ();
	gtk_widget_show (mi);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	mi = gtk_menu_item_new_with_mnemonic (_("Add _new item"));
	gtk_widget_show (mi);
	g_signal_connect (mi, "activate", G_CALLBACK (run_add_control_dialog),
			  NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }

    g_assert (menu != NULL);

    return menu;
}

static gboolean
control_press_cb (GtkWidget * b, GdkEventButton * ev, Control * control)
{
    if (disable_user_config)
	return FALSE;

    if (ev->button == 3 || (ev->button == 1 && (ev->state & GDK_SHIFT_MASK)))
    {
	GtkWidget *menu, *item, *label;

	hide_current_popup_menu ();

	gtk_widget_grab_focus (b);

	popup_control = control;

	menu = get_control_menu ();

	/* update first item */
	item = GTK_MENU_SHELL (menu)->children->data;
	label = gtk_bin_get_child (GTK_BIN (item));
	gtk_label_set_text (GTK_LABEL (label),
			    popup_control->cclass->caption);

	panel_register_open_menu (menu);

	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
			ev->button, ev->time);

	return TRUE;
    }
    else
    {
	return FALSE;
    }
}

/* creation and destruction */

static gboolean
create_plugin (Control * control, const char *filename)
{
    GSList *li = NULL;
    ControlClassInfo *info;

    li = g_slist_find_custom (control_class_info_list, filename,
			      lookup_info_by_filename);

    if (!li)
	return FALSE;

    info = li->data;
    control->cclass = info->class;

    return control_class_info_create_control (info, control);
}

static void
create_launcher (Control * control)
{
    ControlClassInfo *info;

    /* we know it is the first item in the list */
    info = control_class_info_list->data;
    control->cclass = info->class;

    /* we also assume it never fails ;-) */
    info->class->create_control (control);
    info->refcount++;
}

G_MODULE_EXPORT /* EXPORT:create_control */
gboolean
create_control (Control * control, int id, const char *filename)
{
    ControlClass *cc;

    if (id == PLUGIN)
    {
	if (!create_plugin (control, filename))
	    goto failed;
    }
    else
    {
	create_launcher (control);
    }

    /* the control class is set in the create_* functions above */
    cc = control->cclass;

    control_attach_callbacks (control);
    control_set_settings (control);

    return TRUE;

  failed:
    return FALSE;
}

G_MODULE_EXPORT /* EXPORT:control_new */
Control *
control_new (int index)
{
    Control *control = g_new0 (Control, 1);

    control->index = index;
    control->with_popup = FALSE;

    control->base = gtk_event_box_new ();
    gtk_widget_show (control->base);

    /* protect against destruction when unpacking */
    g_object_ref (control->base);
    gtk_object_sink (GTK_OBJECT (control->base));

    return control;
}

G_MODULE_EXPORT /* EXPORT:control_free */
void
control_free (Control * control)
{
    ControlClass *cc = control->cclass;

    if (cc && cc->free)
	cc->free (control);

    gtk_widget_destroy (control->base);
    g_object_unref (control->base);

    g_free (control);
}

G_MODULE_EXPORT /* EXPORT:control_pack */
void
control_pack (Control * control, GtkBox * box)
{
    gtk_box_pack_start (box, control->base, TRUE, TRUE, 0);
}

G_MODULE_EXPORT /* EXPORT:control_unpack */
void
control_unpack (Control * control)
{
    gtk_container_remove (GTK_CONTAINER (control->base->parent),
			  control->base);
}

G_MODULE_EXPORT /* EXPORT:control_attach_callbacks */
void
control_attach_callbacks (Control * control)
{
    ControlClass *cc = control->cclass;

    cc->attach_callback (control, "button-press-event",
			 G_CALLBACK (control_press_cb), control);

    g_signal_connect (control->base, "button-press-event",
		      G_CALLBACK (control_press_cb), control);
}

/* controls configuration */

static void
control_read_config (Control * control, xmlNodePtr node)
{
    ControlClass *cc = control->cclass;

    if (cc && cc->read_config)
	cc->read_config (control, node);
}

static void
control_write_config (Control * control, xmlNodePtr node)
{
    ControlClass *cc = control->cclass;

    if (cc && cc->write_config)
	cc->write_config (control, node);
}

G_MODULE_EXPORT /* EXPORT:control_set_from_xml */
gboolean
control_set_from_xml (Control * control, xmlNodePtr node)
{
    xmlChar *value;
    int id = ICON;
    char *filename = NULL;

    if (!node)
    {
	create_control (control, ICON, NULL);
	return TRUE;
    }

    /* get id and filename */
    value = xmlGetProp (node, (const xmlChar *) "id");

    if (value)
    {
	id = (int) strtol (value, NULL, 0);
	g_free (value);
    }

    if (id == PLUGIN)
	filename = (char *) xmlGetProp (node, (const xmlChar *) "filename");

    /* create a default control of specified type */
    if (!create_control (control, id, filename))
    {
	g_free (filename);
	return FALSE;
    }

    g_free (filename);

    /* hide popup? also check if added to the panel */
    if (control->with_popup && control->base->parent)
	groups_show_popup (control->index, TRUE);

    /* allow the control to read its configuration */
    control_read_config (control, node);

    return TRUE;
}

G_MODULE_EXPORT /* EXPORT:control_write_xml */
void
control_write_xml (Control * control, xmlNodePtr parent)
{
    xmlNodePtr node;
    char value[4];
    ControlClass *cc = control->cclass;

    node = xmlNewTextChild (parent, NULL, "Control", NULL);

    snprintf (value, 3, "%d", cc->id);
    xmlSetProp (node, "id", value);

    if (cc->filename)
	xmlSetProp (node, "filename", cc->filename);

    /* allow the control to write its configuration */
    control_write_config (control, node);
}

G_MODULE_EXPORT /* EXPORT:control_create_options */
void
control_create_options (Control * control, GtkContainer * container,
			GtkWidget * done)
{
    ControlClass *cc = control->cclass;

    if (cc && cc->create_options)
    {
	cc->create_options (control, container, done);
    }
    else
    {
	GtkWidget *hbox, *image, *label;

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_widget_show (hbox);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);

	image =
	    gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO,
				      GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

	label = gtk_label_new (_("This item has no configuration options"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	gtk_container_add (container, hbox);
    }
}

/* controls global preferences */

G_MODULE_EXPORT /* EXPORT:control_set_settings */
void
control_set_settings (Control * control)
{
    control_set_orientation (control, settings.orientation);
    control_set_size (control, settings.size);
    control_set_theme (control, settings.theme);
}

G_MODULE_EXPORT /* EXPORT:control_set_orientation */
void
control_set_orientation (Control * control, int orientation)
{
    ControlClass *cc = control->cclass;

    if (cc && cc->set_orientation)
	cc->set_orientation (control, orientation);
}

G_MODULE_EXPORT /* EXPORT:control_set_size */
void
control_set_size (Control * control, int size)
{
    ControlClass *cc = control->cclass;

    if (cc && cc->set_size)
	cc->set_size (control, size);
    else
    {
	int s = icon_size[size] + border_width;

	gtk_widget_set_size_request (control->base, s, s);
    }
}

G_MODULE_EXPORT /* EXPORT:control_set_theme */
void
control_set_theme (Control * control, const char *theme)
{
    ControlClass *cc = control->cclass;

    if (cc && cc->set_theme)
	cc->set_theme (control, theme);
}
