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
    GtkWidget *widget;
    
    xfce_item_set_has_handle (XFCE_ITEM (control->base), TRUE);
    g_signal_connect (control->base, "realize",
                      G_CALLBACK (tasklist_parent_set), control);

    widget = netk_tasklist_new (netk_screen_get_default ());
    gtk_widget_show (widget);
    gtk_container_add (GTK_CONTAINER (control->base), widget);
    
    gtk_widget_set_size_request (control->base, -1, -1);

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
}

XFCE_PLUGIN_CHECK_INIT
