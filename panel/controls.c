/*  controls.c
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (huysmans@users.sourceforge.net)
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

/*  Controls
 *  --------
 *  Controls provide a unified interface for panel items. Each control has an
 *  associated control class that defines interface functions for the control.
 *
 *  A control is created using the create_control function of the associated
 *  class. This takes an allocated control structure as argument, containing
 *  only a base container widget.
 * 
 *  On startup a list of control classes is made from which new controls can
 *  be created.
 *
 *  Plugins
 *  -------
 *  A control class can also be provided by a plugin. The plugin is accessed 
 *  using the g_module interface.
*/

/*  control class list 
 *  ------------------
*/

#include <config.h>
#include <my_gettext.h>

#include "xfce.h"
#include "item.h"
#include "popup.h"
#include "groups.h"
#include "plugins.h"
#include "controls_dialog.h"

static GSList *control_class_list = NULL;

/* lookup functions */
static gint compare_classes (gconstpointer class_a, gconstpointer class_b)
{
    g_assert (class_a != NULL);
    g_assert (class_b != NULL);
    
    return (g_ascii_strcasecmp(CONTROL_CLASS(class_a)->name, 
			       CONTROL_CLASS(class_b)->name));
}

#if 0
static gint lookup_classes (gconstpointer class, gconstpointer name)
{
    g_assert (class != NULL);
    g_assert (name != NULL);
    
    return (g_ascii_strcasecmp(CONTROL_CLASS(class)->name, name));
}

static gint lookup_classes_by_id (gconstpointer class, 
				  gconstpointer id)
{
    int real_id;
    
    g_assert (class != NULL);
    g_assert (id != NULL);
    
    real_id = GPOINTER_TO_INT(id);
    
    return (CONTROL_CLASS(class)->id != real_id);
}
#endif

static gint lookup_classes_by_filename (gconstpointer class, 
					gconstpointer filename)
{
    char *fn;
    int result;
    
    g_assert (class != NULL);
    g_assert (filename != NULL);
    
    if (CONTROL_CLASS(class)->gmodule)
	fn = g_path_get_basename(g_module_name(CONTROL_CLASS(class)->gmodule));
    else
	return -1;
    
    result = g_ascii_strcasecmp(fn, filename);
    g_free(fn);

    return result;
}

/* free a class */
static void control_class_free(ControlClass *cc)
{
    if (cc->id == PLUGIN)
    {
	g_module_close(cc->gmodule);
	g_free(cc->filename);
    }

    g_free(cc);
}

/* plugins */
#define SOEXT 		("." G_MODULE_SUFFIX)
#define SOEXT_LEN 	(strlen (SOEXT))

#define API_VERSION 3

gchar *xfce_plugin_check_version(gint version)
{
    if (version != API_VERSION)
	return "Incompatible plugin version";
    else
	return NULL;
}

static void load_plugin(gchar *path)
{
    gpointer tmp;
    ControlClass *cc;
    GModule *gm;
    
    void (*init)(ControlClass *cc);
        
    cc = g_new0(ControlClass, 1);
    cc->id = PLUGIN;
    
    cc->gmodule = gm = g_module_open(path,0);

    if (gm && g_module_symbol(gm, "xfce_control_class_init", &tmp))
    {
        init = tmp;
        init(cc);
	
	if (g_slist_find_custom(control_class_list, cc, compare_classes))
	{
            g_message ("xfce4: module %s has already been loaded", cc->name);
	    control_class_free(cc);
	}
	else
	{
            g_message ("xfce4: module %s successfully loaded", cc->name);
	    control_class_list = g_slist_append(control_class_list, cc);
	    cc->filename = g_path_get_basename(path);
	}
    }
    else if (gm)
    {
        g_warning ("xfce4: incompatible module %s",  path);
        g_module_close (gm);
	g_free(cc);
    }
    else
    {
        g_warning ("xfce4: module %s cannot be opened (%s)",  
		   path, g_module_error());
	g_free(cc);
    }
}

static void
load_plugin_dir (const char *dir)
{
    GDir *gdir = g_dir_open(dir, 0, NULL);
    const char *file;

    if(!gdir)
	return;

    while((file = g_dir_read_name(gdir)))
    {
	const char *s = file;

	s += strlen(file) - SOEXT_LEN;
	
	if(strequal(s, SOEXT))
	{
	    char *path = g_build_filename(dir, file, NULL);
	    
	    load_plugin (path);
	    g_free (path);
	}
    }
    
    g_dir_close (gdir);
}

/* control class list */
static void add_plugin_classes(void)
{
    char **dirs, **d;

    dirs = get_plugin_dirs();

    for(d = dirs; *d; d++)
	load_plugin_dir(*d);

    g_strfreev(dirs);
}

static void add_builtin_classes(void)
{
    /* There are no builtin classes at the moment */
}

static void add_launcher_class(void)
{
    ControlClass *cc = g_new0(ControlClass, 1);

    panel_item_class_init(cc);

    control_class_list = g_slist_append(control_class_list, cc);
}

void control_class_list_init(void)
{
    add_launcher_class();
    add_builtin_classes();
    add_plugin_classes();
}

void control_class_list_cleanup(void)
{
    GSList *li;

    for (li = control_class_list; li; li = li->next)
    {
	ControlClass *cc = li->data;

	control_class_free(cc);
    }

    g_slist_free(control_class_list);
    control_class_list = NULL;
}

GSList *get_control_class_list(void)
{
    return control_class_list;
}

/* right click menu 
 * ----------------
*/
static Control *popup_control = NULL;

static void edit_control(void)
{
    if (popup_control)
	controls_dialog(popup_control);

    popup_control = NULL;
}

static void remove_control(void)
{
    if (popup_control)
	groups_remove(popup_control->index);

    popup_control = NULL;
}

static void add_control(void)
{
    Control *newcontrol;
    
    panel_add_control();
    panel_set_position();
    newcontrol = groups_get_control(settings.num_groups-1);
    
    if (popup_control)
	groups_move(settings.num_groups-1, popup_control->index);

    popup_control = NULL;

    controls_dialog(newcontrol);
}

static GtkItemFactoryEntry control_items[] = {
  { N_("/_Edit ..."),     NULL, edit_control,   0, "<Item>" },
  { N_("/_Remove"),       NULL, remove_control, 0, "<Item>" },
  { "/sep",              NULL, NULL,           0, "<Separator>" },
  { N_("/Add _new item"), NULL, add_control,    0, "<Item>" }
};

static GtkWidget *get_control_menu(void)
{
    static GtkItemFactory *factory;
    GtkWidget *menu = NULL;

    if (!menu)
    {
        factory = gtk_item_factory_new(GTK_TYPE_MENU, "<popup>", NULL);
       
	gtk_item_factory_set_translate_func(factory, 
					    (GtkTranslateFunc) gettext,
					    NULL, NULL);
        gtk_item_factory_create_items(factory, G_N_ELEMENTS(control_items),
				      control_items, NULL);

        menu = gtk_item_factory_get_widget(factory, "<popup>");
    }

    return menu;
}
static gboolean control_press_cb(GtkWidget *b, GdkEventButton * ev, Control *control)
{
    if (disable_user_config)
	return FALSE;
    
    if(ev->button == 3 || 
	    (ev->button == 1 && (ev->state & GDK_SHIFT_MASK)))
    {
	GtkWidget *menu;
	
	hide_current_popup_menu();

	gtk_widget_grab_focus(b);

	popup_control = control;

	menu = get_control_menu();
	
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 
		       ev->button, ev->time); 
	
/*	gtk_item_factory_popup_with_data(factory, control, NULL, 
					 ev->x_root, ev->y_root,
					 ev->button, ev->time);

	controls_dialog(control); */

	return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*  Controls 
 *  --------
*/
static gboolean create_plugin(Control *control, const char *filename)
{
    GSList *li = NULL;
    ControlClass *cc;

    li = g_slist_find_custom(control_class_list, filename, 
	    		     lookup_classes_by_filename);

    if (!li)
	return FALSE;

    cc = control->cclass = li->data;
    
    return cc->create_control(control);
}

static gboolean create_builtin(Control *control, int id)
{
    g_warning("xfce4: unknown control id: %d\n", id);
    return FALSE;
}

static void create_launcher(Control *control)
{
    ControlClass *cc;
    
    /* we know it is the first item in the list */
    cc = control->cclass = control_class_list->data;
    
    cc->create_control(control);
}

void create_control(Control * control, int id, const char *filename)
{
    ControlClass *cc;
    
    /* the control class is set in the create_* functions */
    switch (id)
    {
	case ICON:
	    create_launcher(control);
	    break;
	case PLUGIN:
	    if (!create_plugin(control, filename))
		create_launcher(control);
	    break;
	default:
	    if (!create_builtin(control, id))
		create_launcher(control);
    }

    cc = control->cclass;
    
    /* these are required for proper operation */
    if(!gtk_bin_get_child(GTK_BIN(control->base)))
    {
        if(cc && cc->free)
            cc->free(control);

        create_launcher(control);
    }

    control_attach_callbacks(control);
    control_set_settings(control);
}

Control *control_new(int index)
{
    Control *control = g_new0(Control, 1);

    control->index = index;
    control->with_popup = TRUE;

    control->base = gtk_event_box_new();
    gtk_widget_show(control->base);
    
    /* protect against destruction when unpacking */
    g_object_ref(control->base);
    gtk_object_sink(GTK_OBJECT(control->base));

    return control;
}

void control_free(Control * control)
{
    ControlClass *cc = control->cclass;

    if (cc && cc->free)
	cc->free(control);

    gtk_widget_destroy(control->base);
    g_object_unref(control->base);

    g_free(control);
}

void control_pack(Control * control, GtkBox * box)
{
    gtk_box_pack_start(box, control->base, TRUE, TRUE, 0);
}

void control_unpack(Control * control)
{
    gtk_container_remove(GTK_CONTAINER(control->base->parent), control->base);
}

void control_attach_callbacks(Control *control)
{
    ControlClass *cc = control->cclass;
    
    cc->attach_callback(control, "button-press-event", 
	    		G_CALLBACK(control_press_cb), control);

    g_signal_connect(control->base, "button-press-event", 
	    	     G_CALLBACK(control_press_cb), control);
}

/* configuration */
static void control_read_config(Control *control, xmlNodePtr node)
{
    ControlClass *cc = control->cclass;

    if (cc && cc->read_config)
	cc->read_config(control, node);
}

static void control_write_config(Control *control, xmlNodePtr node)
{
    ControlClass *cc = control->cclass;

    if (cc && cc->write_config)
	cc->write_config(control, node);
}

void control_set_from_xml(Control * control, xmlNodePtr node)
{
    xmlChar *value;
    int id = ICON;
    char *filename = NULL;

    if(!node)
    {
        create_control(control, ICON, NULL);
        return;
    }

    /* get id and filename */
    value = xmlGetProp(node, (const xmlChar *)"id");

    if(value)
    {
        id = atoi(value);
        g_free(value);
    }

    if(id == PLUGIN)
        filename = (char *) xmlGetProp(node, (const xmlChar *)"filename");

    /* create a default control of specified type */
    create_control(control, id, filename);
    g_free(filename);

    /* allow the control to read its configuration */
    control_read_config(control, node);

    /* also check if added to the panel */
    if (!control->with_popup && control->base->parent) 
	groups_show_popup(control->index, FALSE);
}

void control_write_xml(Control * control, xmlNodePtr parent)
{
    xmlNodePtr node;
    char value[4];
    ControlClass *cc = control->cclass;

    node = xmlNewTextChild(parent, NULL, "Control", NULL);

    snprintf(value, 3, "%d", cc->id);
    xmlSetProp(node, "id", value);

    if(cc->filename)
        xmlSetProp(node, "filename", cc->filename);

    /* allow the control to write its configuration */
    control_write_config(control, node);
}

/* options dialog */
void control_add_options(Control * control, GtkContainer * container,
                         GtkWidget * revert, GtkWidget * done)
{
    ControlClass *cc = control->cclass;

    if (cc && cc->add_options)
    {
        cc->add_options(control, container, revert, done);
    }
    else
    {
        GtkWidget *hbox, *image, *label;

        hbox = gtk_hbox_new(FALSE, 4);
        gtk_widget_show(hbox);
        gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);

        image =
            gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,
                                     GTK_ICON_SIZE_LARGE_TOOLBAR);
        gtk_widget_show(image);
        gtk_box_pack_start(GTK_BOX(hbox), image, TRUE, FALSE, 0);

        label = gtk_label_new(_("This item has no configuration options"));
        gtk_widget_show(label);
        gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);

        gtk_container_add(container, hbox);
    }
}

/* global settings */

void control_set_settings(Control * control)
{
    control_set_orientation(control, settings.orientation);
    control_set_size(control, settings.size);
    control_set_theme(control, settings.theme);
}

void control_set_orientation(Control * control, int orientation)
{
    ControlClass *cc = control->cclass;

    if (cc && cc->set_orientation)
	cc->set_orientation(control, orientation);
}

void control_set_size(Control * control, int size)
{
    ControlClass *cc = control->cclass;

    if (cc && cc->set_size)
	cc->set_size(control, size);
    else
    {
	int s = icon_size[size] + border_width;
	gtk_widget_set_size_request(control->base, s, s);
    }
}

void control_set_theme(Control * control, const char *theme)
{
    ControlClass *cc = control->cclass;

    if (cc && cc->set_theme)
	cc->set_theme(control, theme);
}

