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

/*  flexspace.c : xfce4 panel plugin for flexible (expanding) spacing. 
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
flexspace_attach_callback (Control * control, const char *signal,
			   GCallback callback, gpointer data)
{
    /* define explicitly to do nothing */
}

static void
flexspace_set_size (Control *control, int size)
{
    /* define explicitly to do nothing */
}

/* panel control */
static void
flexspace_parent_set (GtkWidget *base, GtkObject *old, Control *control)
{
    if (base->parent)
    {
        xfce_itembar_set_child_expand (XFCE_ITEMBAR (base->parent), 
                                       base, TRUE);
    }
}

static gboolean
create_flexspace_control (Control * control)
{
    xfce_item_set_use_drag_window (XFCE_ITEM (control->base), TRUE);
    g_signal_connect (control->base, "realize",
                      G_CALLBACK (flexspace_parent_set), control);

    gtk_widget_set_size_request (control->base, 8, 8);

    return TRUE;
}

G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    cc->name = "flexspace";
    cc->caption = _("Flexible space");

    cc->create_control = (CreateControlFunc) create_flexspace_control;

    cc->attach_callback = flexspace_attach_callback;
    cc->set_size = flexspace_set_size;
}

XFCE_PLUGIN_CHECK_INIT
