/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2006 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published 
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "xfce-panel-convenience.h"

/**
 * xfce_create_panel_button:
 *
 * Create regular #GtkButton with a few properties set to be useful in the
 * Xfce panel: Flat (%GTK_RELIEF_NONE), no focus on click and minimal padding.
 *
 * Returns: newly created #GtkButton.
 **/
GtkWidget *xfce_create_panel_button (void)
{
    GtkWidget *button = gtk_button_new ();
    
    GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_DEFAULT|GTK_CAN_FOCUS);
    gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
    gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);

    return button;
}

/**
 * xfce_create_panel_button:
 *
 * Create regular #GtkToggleButton with a few properties set to be useful in 
 * Xfce panel: Flat (%GTK_RELIEF_NONE), no focus on click and minimal padding.
 *
 * Returns: newly created #GtkToggleButton.
 **/
GtkWidget *xfce_create_panel_toggle_button (void)
{
    GtkWidget *button = gtk_toggle_button_new ();
    
    GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_DEFAULT|GTK_CAN_FOCUS);
    gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
    gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);

    return button;
}
