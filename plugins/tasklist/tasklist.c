/*  $Id$
 *
 *  Copyright 2005 Jasper Huijsmans (jasper@xfce.org)
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

/*  tasklist.c : xfce4 panel plugin that shows a tasklist. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfcegui4/libxfcegui4.h>

#include "libs/xfce-item.h"
#include "libs/xfce-itembar.h"

#include <panel/plugins.h>
#include <panel/xfce.h>

typedef struct
{
    GtkWidget *widget;

    gboolean always_group;
    gboolean all_workspaces;
    gboolean show_label;
}
Tasklist;

static void
tasklist_attach_callback (Control * control, const char *signal,
			   GCallback callback, gpointer data)
{
    /* define explicitly to do nothing */
}

static void
tasklist_set_size (Control * control, int size)
{
    if (xfce_itembar_get_orientation (XFCE_ITEMBAR (control->base->parent))
            == GTK_ORIENTATION_HORIZONTAL)
    {
        gtk_widget_set_size_request (control->base, 300, icon_size[size]);
    }
    else
    {
        gtk_widget_set_size_request (control->base, icon_size[size], 300);
    }
}

/* config */
static void
tasklist_read_config (Control *control, xmlNodePtr node)
{
    xmlChar *value;
    int n;
    Tasklist *tasklist = control->data;

    if (node)
    {
        value = xmlGetProp (node, "all_workspaces");

        if (value)
        {
            n = strtol (value, NULL, 0);

            tasklist->all_workspaces = (n == 1);

            xmlFree (value);
        }
        
        value = xmlGetProp (node, "always_group");

        if (value)
        {
            n = strtol (value, NULL, 0);

            tasklist->always_group = (n == 1);

            xmlFree (value);
        }

        value = xmlGetProp (node, "show_label");

        if (value)
        {
            n = strtol (value, NULL, 0);

            tasklist->show_label = (n != 0);

            xmlFree (value);
        }
    }
    
    netk_tasklist_set_include_all_workspaces (NETK_TASKLIST (tasklist->widget), 
                                              tasklist->all_workspaces);

    netk_tasklist_set_grouping (NETK_TASKLIST (tasklist->widget),
                                tasklist->always_group ? 
                                        NETK_TASKLIST_ALWAYS_GROUP :
                                        NETK_TASKLIST_AUTO_GROUP);

    netk_tasklist_set_show_label (NETK_TASKLIST (tasklist->widget),
                                  tasklist->show_label);
}

static void
tasklist_write_config (Control *control, xmlNodePtr node)
{
    char value[2];
    Tasklist *tasklist = control->data;

    g_snprintf (value, 2, "%d", tasklist->all_workspaces);
    xmlSetProp (node, "all_workspaces", (const xmlChar *)value);
    
    g_snprintf (value, 2, "%d", tasklist->always_group);
    xmlSetProp (node, "always_group", (const xmlChar *)value);

    g_snprintf (value, 2, "%d", tasklist->show_label);
    xmlSetProp (node, "show_label", (const xmlChar *)value);
}

/* properties dialog */

static void
all_workspaces_toggled (GtkToggleButton *tb, Tasklist *tasklist)
{
    tasklist->all_workspaces = gtk_toggle_button_get_active (tb);

    netk_tasklist_set_include_all_workspaces (NETK_TASKLIST (tasklist->widget),
                                              tasklist->all_workspaces);
}

static void
always_group_toggled (GtkToggleButton *tb, Tasklist *tasklist)
{
    tasklist->always_group = gtk_toggle_button_get_active (tb);

    netk_tasklist_set_grouping (NETK_TASKLIST (tasklist->widget),
                                tasklist->always_group ? 
                                        NETK_TASKLIST_ALWAYS_GROUP :
                                        NETK_TASKLIST_AUTO_GROUP);
}

static void
show_label_toggled (GtkToggleButton *tb, Tasklist *tasklist)
{
    tasklist->show_label = gtk_toggle_button_get_active (tb);

    netk_tasklist_set_show_label (NETK_TASKLIST (tasklist->widget),
                                  tasklist->show_label);
}

static void
tasklist_create_options (Control *control, GtkContainer *container,
                         GtkWidget *close)
{
    GtkWidget *vbox, *cb;
    Tasklist *tasklist;

    tasklist = control->data;

    vbox = gtk_vbox_new (FALSE, 8);
    gtk_widget_show (vbox);
    gtk_container_add (container, vbox);

    cb = gtk_check_button_new_with_mnemonic (_("Show tasks "
                                               "from _all workspaces"));
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
                                  tasklist->all_workspaces);

    g_signal_connect (cb, "toggled", G_CALLBACK (all_workspaces_toggled),
                      tasklist);

    cb = gtk_check_button_new_with_mnemonic (_("Always _group tasks"));
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
                                  tasklist->always_group);

    g_signal_connect (cb, "toggled", G_CALLBACK (always_group_toggled),
                      tasklist);

    cb = gtk_check_button_new_with_mnemonic (_("Show application _names"));
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
                                  tasklist->show_label);
    g_signal_connect (cb, "toggled", G_CALLBACK (show_label_toggled),
                      tasklist);
}

/* panel control */
static void
tasklist_parent_set (GtkWidget *base, GtkObject *old, Control *control)
{
    if (base->parent)
    {
        xfce_itembar_set_child_expand (XFCE_ITEMBAR (base->parent), 
                                       base, TRUE);
    }
}

static gboolean
create_tasklist_control (Control * control)
{
    Tasklist *tasklist = g_new0 (Tasklist, 1);
    
    tasklist->show_label = TRUE;

    xfce_item_set_has_handle (XFCE_ITEM (control->base), TRUE);
    g_signal_connect (control->base, "realize",
                      G_CALLBACK (tasklist_parent_set), control);

    tasklist->widget = netk_tasklist_new (netk_screen_get_default ());
    gtk_widget_show (tasklist->widget);
    gtk_container_add (GTK_CONTAINER (control->base), tasklist->widget);
    
    gtk_widget_set_size_request (control->base, -1, -1);

    control->data = tasklist;
    
    return TRUE;
}

G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    cc->name = "tasklist";
    cc->caption = _("Tasklist");

    cc->create_control = (CreateControlFunc) create_tasklist_control;

    cc->attach_callback = tasklist_attach_callback;
    cc->set_size = tasklist_set_size;

    cc->read_config = tasklist_read_config;
    cc->write_config = tasklist_write_config;
    cc->create_options = tasklist_create_options;
}

XFCE_PLUGIN_CHECK_INIT
