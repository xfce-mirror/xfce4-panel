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

/*  Panel Control Configuration Dialog
 *  ----------------------------------
 *  All panel controls may provide an add_options() functions to allow
 *  options to be changed. The arguments to this function are a PanelControl,
 *  a GtkContainer to pack options into, and two GtkButton's. 
 *
 *  The first button is used to revert changes. The button is initially 
 *  insensitive. The panel control should make it sensitive when an option
 *  is changed. The panel control should also connect a callback that 
 *  reverts the options to the initial state. This implies the initial state
 *  should be saved when creating the options.
 *
 *  The second button is used to close the dialog. Any changes that weren't 
 *  applied immediately should be applied now.
 *  
 *  You can connect to the destroy signal on the option widgets (or the 
 *  continainer) to know when to clean up the backup data. 
 *  
 *  
 *  Plugins
 *  -------
 *  Plugins are accessed through the GModule interface. They must provide these 
 *  two symbols:
 *  - is_xfce_panel_control : only checked for existance
 *  - module_init : this function takes a PanelControl structure as argument
 *    and must fill it with the appropriate values and pointers.
*/

#include "controls.h"
#include "item.h"
#include "builtins.h"
#include "xfce_support.h"
#include "callbacks.h"
#include "side.h"

/*  Plugins
 *  -------
*/
static char *find_plugin(const char *name)
{
    char **dirs, **d;
    char *path;

    dirs = get_plugin_dirs();

    for(d = dirs; *d; d++)
    {
        path = g_build_filename(*d, name, NULL);

        if(g_file_test(path, G_FILE_TEST_EXISTS))
        {
            g_strfreev(dirs);
            return path;
        }
        else
            g_free(path);
    }

    return NULL;
}

gboolean create_plugin(PanelControl * pc)
{
    gpointer tmp;
    char *path;
    void (*init) (PanelControl * pc);

    g_return_val_if_fail(pc->filename, FALSE);
    g_return_val_if_fail(g_module_supported(), FALSE);

    if(pc->dir)
        path = g_build_filename(pc->dir, pc->filename, NULL);
    else
        path = find_plugin(pc->filename);

    g_return_val_if_fail(path, FALSE);

    pc->gmodule = g_module_open(path, 0);

    if(!pc->gmodule)
    {
        const char *err = g_module_error();

        g_printerr("gmodule could not be opened: %s\n", path);
        g_printerr("	%s\n", err);
        g_free(path);
        return FALSE;
    }
    else if(!g_module_symbol(pc->gmodule, "is_xfce_panel_control", &tmp))
    {
        g_printerr("Not a panel module: %s\n", path);
        g_free(path);
        return FALSE;
    }

    g_free(path);

    if(g_module_symbol(pc->gmodule, "module_init", &tmp))
    {
        init = tmp;
        init(pc);
    }

    if(!pc->main || !pc->base)
    {
        g_module_close(pc->gmodule);
        pc->gmodule = NULL;

        return FALSE;
    }

    return TRUE;
}

/*  The PanelControl interface
 *
*/

/* static functions */
static void panel_control_stop(PanelControl * pc)
{
    if(!pc || !pc->update)
        return;

    if(pc->timeout_id)
    {
        g_source_remove(pc->timeout_id);
        pc->timeout_id = 0;
    }
}

static void panel_control_start(PanelControl * pc)
{
    if(!pc->update)
        return;

    if(pc->interval > 0)
    {
        if(pc->timeout_id)
            panel_control_stop(pc);


        pc->timeout_id =
            g_timeout_add(pc->interval, (GSourceFunc) pc->update, pc);
    }
}

static void panel_control_read_config(PanelControl * pc, xmlNodePtr node)
{
    if(pc->read_config)
        pc->read_config(pc, node);
}

/*  creation
*/
PanelControl *panel_control_new(int side, int index)
{
    int size = icon_size[settings.size] + border_width;
    PanelControl *pc = g_new(PanelControl, 1);

    pc->container = NULL;

    pc->side = side;
    pc->index = index;

    /* this is the only widget created here
     * We use the eventbox as the simplest container 
     * (a frame adds a 2 pixel border)
     */
    pc->base = gtk_event_box_new();
    gtk_container_set_border_width(GTK_CONTAINER(pc->base), 0);
    gtk_widget_show(pc->base);

    /* protect against destruction when unpacking */
    g_object_ref(pc->base);

    pc->id = ICON;
    pc->filename = NULL;
    pc->dir = NULL;
    pc->gmodule = NULL;

    pc->caption = NULL;
    pc->main = NULL;
    pc->data = NULL;

    pc->read_config = NULL;
    pc->write_config = NULL;

    pc->free = NULL;

    pc->interval = 0;
    pc->timeout_id = 0;
    pc->update = NULL;

    pc->set_orientation = NULL;
    pc->set_size = NULL;
    pc->set_style = NULL;
    pc->set_theme = NULL;

    pc->callback_id = 0;
    pc->add_options = NULL;

    return pc;
}

void create_panel_control(PanelControl * pc)
{
    switch (pc->id)
    {
        case PLUGIN:
            create_plugin(pc);
            break;
        case CLOCK:
            create_clock(pc);
            break;
        case TRASH:
            create_trash(pc);
            break;
        case EXIT:
            create_exit(pc);
            break;
        case CONFIG:
            create_config(pc);
            break;
        default:
            create_panel_item(pc);
            break;
    }

    /* these are required for proper operation */
    if(!pc->main || !gtk_bin_get_child(GTK_BIN(pc->base)) || !pc->caption)
    {
        if(pc->free)
            pc->free(pc);

        create_panel_item(pc);
    }

    pc->callback_id =
        g_signal_connect(pc->main, "button-press-event",
                         G_CALLBACK(panel_control_press_cb), pc);

    panel_control_start(pc);
    panel_control_set_settings(pc);
}

/* here the actual panel control is created */
void panel_control_set_from_xml(PanelControl * pc, xmlNodePtr node)
{
    xmlChar *value;

    if(!node)
    {
        create_panel_control(pc);

        return;
    }

    /* get id and filename */
    value = xmlGetProp(node, (const xmlChar *)"id");

    if(value)
    {
        pc->id = atoi(value);
        g_free(value);
    }

    if(pc->id == PLUGIN)
    {
        value = xmlGetProp(node, (const xmlChar *)"filename");

        if(value)
            pc->filename = (char *)value;
    }

    /* create a default control */
    create_panel_control(pc);

    /* allow the control to read its configuration */
    panel_control_read_config(pc, node);

    panel_control_start(pc);
}

/*  packing and unpacking
*/
void panel_control_pack(PanelControl * pc, GtkContainer * container)
{
    pc->container = container;

    gtk_container_add(container, pc->base);

    side_panel_register_control(pc);

    if(pc->main)
    {
        pc->callback_id =
            g_signal_connect(pc->main, "button-press-event",
                             G_CALLBACK(panel_control_press_cb), pc);

	panel_control_set_settings(pc);
    }
}

void panel_control_unpack(PanelControl * pc)
{
    panel_control_stop(pc);

    if(pc->callback_id)
        g_signal_handler_disconnect(pc->main, pc->callback_id);

    if(GTK_IS_WIDGET(pc->container))
    {
        gtk_container_remove(pc->container, pc->base);
        pc->container = NULL;
    }
}

/*  destruction
*/
void panel_control_free(PanelControl * pc)
{
    panel_control_stop(pc);

    if(pc->caption)
        g_free(pc->caption);

    if(pc->free)
        pc->free(pc);

    if(GTK_IS_WIDGET(pc->base))
        gtk_widget_destroy(pc->base);

    g_free(pc);
}

/*  exit
*/
void panel_control_write_xml(PanelControl * pc, xmlNodePtr parent)
{
    xmlNodePtr node;
    char value[4];

    node = xmlNewTextChild(parent, NULL, "Control", NULL);

    snprintf(value, 3, "%d", pc->id);
    xmlSetProp(node, "id", value);

    if(pc->filename)
        xmlSetProp(node, "filename", pc->filename);

    if(pc->write_config)
        pc->write_config(pc, node);
}

/*  global settings
 *  ---------------
 *  These are mostly wrappers around the functions provided by a 
 *  panel control
*/
void panel_control_set_settings(PanelControl *pc)
{
    panel_control_set_orientation(pc, settings.size);
    panel_control_set_size(pc, settings.size);
    panel_control_set_style(pc, settings.style);

    if (settings.theme)
	panel_control_set_theme(pc, settings.theme);
}

void panel_control_set_orientation(PanelControl *pc, int orientation)
{
    if (pc->set_orientation)
	pc->set_orientation(pc, orientation);
}

void panel_control_set_size(PanelControl * pc, int size)
{
    int s = icon_size[size] + border_width;

    if(pc->set_size)
        pc->set_size(pc, size);
}

void panel_control_set_style(PanelControl * pc, int style)
{
    if(pc->set_style)
        pc->set_style(pc, style);
}

void panel_control_set_theme(PanelControl * pc, const char *theme)
{
    if(pc->set_theme)
        pc->set_theme(pc, theme);
}

/*  Change panel controls
*/
void panel_control_add_options(PanelControl * pc, GtkContainer * container,
                               GtkWidget * revert, GtkWidget * done)
{
    if(pc->add_options)
        pc->add_options(pc, container, revert, done);
    else
    {
        GtkWidget *hbox, *image, *label;

        hbox = gtk_hbox_new(FALSE, 4);
        gtk_widget_show(hbox);
        gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);

        image =
            gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,
                                     GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_widget_show(image);
        gtk_box_pack_start(GTK_BOX(hbox), image, TRUE, FALSE, 0);

        label = gtk_label_new(_("This module has no configuration options"));
        gtk_widget_show(label);
        gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);

        gtk_container_add(container, hbox);
    }

    g_signal_connect_swapped(done, "clicked",
                             G_CALLBACK(panel_control_start), pc);
}
