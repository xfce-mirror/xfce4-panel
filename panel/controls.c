/*  controls.c
 *  
 *  Copyright (C) 2002-2003 Jasper Huijsmans (jasper@xfce.org)
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

#include <libxfce4util/i18n.h>
#include <libxfce4util/debug.h>

#include "xfce.h"
#include "item.h"
#include "popup.h"
#include "groups.h"
#include "plugins.h"
#include "controls_dialog.h"
#include "settings.h"

#define SOEXT 		("." G_MODULE_SUFFIX)
#define SOEXT_LEN 	(strlen (SOEXT))

#define API_VERSION 5

#define UNLOAD_TIMEOUT 30000 /* 30 secs */

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
};


static GSList *control_class_info_list = NULL;
static ControlClassInfo *info_to_add = NULL;

static int unload_id = 0;
static int unloading = 0;

/* not static, must be available in panel.c for handle menu */
Control *popup_control = NULL;


/*  ControlClass and ControlClassInfo 
 *  ---------------------------------
*/

static gint
compare_class_info_by_name (gconstpointer info, gconstpointer name)
{
    g_assert (info != NULL);
    g_assert (name != NULL);

    return g_ascii_strcasecmp (((ControlClassInfo *)info)->name,
	    		       (char *)name);
}

static gint
lookup_info_by_filename (gconstpointer info, gconstpointer filename)
{
    char *fn;
    int result = -1;

    g_assert (info != NULL);
    g_assert (filename != NULL);

    if (((ControlClassInfo *)info)->class->gmodule)
    {
	fn = g_path_get_basename (
		g_module_name(((ControlClassInfo *)info)->class->gmodule));

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
get_control_class_info (ControlClass *cc)
{
    ControlClassInfo *info;
    
    g_return_val_if_fail (cc->name != NULL, NULL);
    
    if (info_to_add && 
	g_ascii_strcasecmp (info_to_add->class->name, cc->name) == 0)
    {
	DBG ("class is from info_to_add");
	info = info_to_add;
    }
    else
    {
	GSList *li = 
	    g_slist_find_custom (control_class_info_list,
	    			 cc->name, compare_class_info_by_name);

	info = li->data; 
    }
    
    return info;
}

static gboolean
control_class_info_create_control (ControlClassInfo *info, Control *control)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (control != NULL, FALSE);

    while (unloading)
    {
	DBG ("unloading in progress");
	g_usleep (200*1000);
    }
    
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
	    DBG ("...already loaded");
	    
	    control_class_free (cc);
	    control_class_info_free (info);
	}
	else
	{
	    DBG ("...ok");
	    
	    control_class_info_list = 
		g_slist_prepend (control_class_info_list, info);
	
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
    char **dirs, **d;

    dirs = get_plugin_dirs ();

    for (d = dirs; *d; d++)
	load_plugin_dir (*d);

    g_strfreev (dirs);
}

/*  builtin launcher class */

static void
add_launcher_class (void)
{
    ControlClassInfo *info;
    ControlClass *cc;
    
    info = g_new0(ControlClassInfo, 1);
    cc = info->class = g_new0 (ControlClass, 1);

    panel_item_class_init (cc);

    info->name = g_strdup (cc->name);
    info->caption = g_strdup (cc->caption);

    control_class_info_list = g_slist_append (control_class_info_list, info);
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

	DBG ("info: %s (%d items)", info->caption, info->refcount);
	
	if (info->class->id == PLUGIN && info->refcount == 0 && 
	    info->class->gmodule != NULL)
	{
	    DBG ("info: unload %s", info->caption);

	    g_module_close (info->class->gmodule);
	    info->class->gmodule = NULL;
	}
    }
    
    unloading--;
    
    return TRUE;
}

/* exported interface */

void
control_class_list_init (void)
{
    /* prepend class info to the list */
    add_launcher_class ();
    add_plugin_classes ();

    /* reverse to get correct order */
    control_class_info_list = g_slist_reverse (control_class_info_list);

    unload_id = 
	g_timeout_add (UNLOAD_TIMEOUT, (GSourceFunc) unload_modules, NULL);
}

void
control_class_list_cleanup (void)
{
    GSList *li;

    unloading++;
    
    if (unload_id)
    {
	g_source_remove (unload_id);
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

void 
control_class_set_unique (ControlClass *cclass, gboolean unique)
{
    ControlClassInfo *info;

    info = get_control_class_info (cclass);

    if (info)
	info->unique = unique;
}

void 
control_class_set_icon (ControlClass *cclass, GdkPixbuf *icon)
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

void 
control_class_unref (ControlClass *cclass)
{
    ControlClassInfo *info;

    info = get_control_class_info (cclass);

    DBG ("decrease refcount: %s (%d)", info->caption, info->refcount);
    
    if (info && info->refcount > 0)
	info->refcount--;
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
	    confirm (_("Removing the item will also remove its popup menu."),
		     GTK_STOCK_REMOVE, NULL))
	{
	    groups_remove (popup_control->index);
	}
    }

    popup_control = NULL;
}

static void
add_control (GtkWidget * w, ControlClassInfo *info)
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

static GtkItemFactoryEntry control_items[] = {
    {"/Item", NULL, NULL, 0, "<Title>"},
    {"/sep", NULL, NULL, 0, "<Separator>"},
    {N_("/Add _new item"), NULL, NULL, 0, "<Branch>"},
    {"/sep", NULL, NULL, 0, "<Separator>"},
    {N_("/_Properties..."), NULL, edit_control, 0, "<Item>"},
    {N_("/_Remove"), NULL, remove_control, 0, "<Item>"},
};

static const char *
translate_menu (const char *msg)
{
#if ENABLE_NLS
    /* ensure we use the panel domain and not that of a plugin */
    return dgettext (GETTEXT_PACKAGE, msg);
#else
    return msg;
#endif
}

GtkWidget *
get_controls_submenu (void)
{
    static GtkWidget *menu = NULL;
    GSList *li;
    
    if (menu)
	gtk_widget_destroy (menu);

    menu = gtk_menu_new ();
    
    for (li = control_class_info_list; li != NULL; li = li->next)
    {
	ControlClassInfo *info = li->data;
	GtkWidget *item;

	DBG ("info: %s (%s)", info->caption, info->name);
	
	item = gtk_menu_item_new_with_label (info->caption);
	gtk_widget_show (item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	g_signal_connect (item, "activate", G_CALLBACK (add_control), info);

	if (info->unique && info->refcount > 0)
	    gtk_widget_set_sensitive (item, FALSE);
    }

    return menu;
}

static GtkWidget *
get_control_menu (void)
{
    static GtkItemFactory *factory;
    static GtkWidget *menu = NULL;
    GtkWidget *submenu, *item;

    if (!menu)
    {
	factory = gtk_item_factory_new (GTK_TYPE_MENU, "<popup>", NULL);

	gtk_item_factory_set_translate_func (factory,
					     (GtkTranslateFunc)
					     translate_menu, NULL, NULL);

	gtk_item_factory_create_items (factory, G_N_ELEMENTS (control_items),
				       control_items, NULL);

	menu = gtk_item_factory_get_widget (factory, "<popup>");
    }

    g_assert (menu != NULL);
    
    /* the third item, keep in sync with factory */
    item = GTK_MENU_SHELL (menu)->children->next->next->data;
    
    submenu = get_controls_submenu();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);

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
	item = GTK_MENU_SHELL(menu)->children->data;
	label = gtk_bin_get_child (GTK_BIN (item));
	gtk_label_set_text (GTK_LABEL (label), 
			    popup_control->cclass->caption);

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

void
control_pack (Control * control, GtkBox * box)
{
    gtk_box_pack_start (box, control->base, TRUE, TRUE, 0);
}

void
control_unpack (Control * control)
{
    gtk_container_remove (GTK_CONTAINER (control->base->parent),
			  control->base);
}

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
	id = atoi (value);
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

void
control_set_settings (Control * control)
{
    control_set_orientation (control, settings.orientation);
    control_set_size (control, settings.size);
    control_set_theme (control, settings.theme);
}

void
control_set_orientation (Control * control, int orientation)
{
    ControlClass *cc = control->cclass;

    if (cc && cc->set_orientation)
	cc->set_orientation (control, orientation);
}

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

void
control_set_theme (Control * control, const char *theme)
{
    ControlClass *cc = control->cclass;

    if (cc && cc->set_theme)
	cc->set_theme (control, theme);
}
