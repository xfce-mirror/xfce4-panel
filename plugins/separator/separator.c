/*  $Id$
 *
 *  Copyright 2003,2005 Jasper Huijsmans (jasper)
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

/*  separator.c : xfce4 panel plugin that shows a thin separator line. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4util/libxfce4util.h>

#include "libs/xfce-separator-item.h"
#include "libs/xfce-itembar.h"

#include <panel/plugins.h>
#include <panel/xfce.h>


static void
separator_attach_callback (Control * control, const char *signal,
			   GCallback callback, gpointer data)
{
    /* define explicitly to do nothing */
}

static void
separator_set_size (Control * control, int size)
{
    /* define explicitly to do nothing */
}

/* panel control */
gboolean
create_separator_control (Control * control)
{
    control_swap_base (control, xfce_separator_item_new ());
    gtk_widget_set_size_request (control->base, -1, -1);

    return TRUE;
}

G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    cc->name = "separator";
    cc->caption = _("Separator");

    cc->create_control = (CreateControlFunc) create_separator_control;

    cc->attach_callback = separator_attach_callback;
    cc->set_size = separator_set_size;
}

XFCE_PLUGIN_CHECK_INIT
