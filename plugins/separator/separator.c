/*  $Id$
 *
 *  Copyright 2003 Jasper Huijsmans (jasper)
 *
 *  Adapted from initial implementation by Ejvend Nielsen, copyright 2003 
 *  licensed under GNU GPL.   
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

#include <panel/plugins.h>
#include <panel/xfce.h>

typedef struct
{
    GtkWidget *box;
    GtkWidget *align;
    GtkWidget *hsep;
    GtkWidget *vsep;
}
t_separator;

static void
separator_free (Control * control)
{
    t_separator *sep = control->data;

    g_object_unref (sep->hsep);
    g_object_unref (sep->vsep);

    g_free (sep);
}

static void
separator_attach_callback (Control * control, const char *signal,
			   GCallback callback, gpointer data)
{
    t_separator *sep = (t_separator *) control->data;

    g_signal_connect (sep->box, signal, callback, data);
    g_signal_connect (sep->hsep, signal, callback, data);
    g_signal_connect (sep->vsep, signal, callback, data);
}

static void
separator_set_size (Control * control, int size)
{
    /* define explicitly to do nothing */

#if 0
    int s;
    t_separator *sep = control->data;

    s = icon_size[size] + border_width;

    if (settings.orientation == HORIZONTAL)
	gtk_widget_set_size_request (control->base, -1, s);
    else
	gtk_widget_set_size_request (control->base, s, -1);
#endif
}

static void
separator_set_orientation (Control * control, int orientation)
{
    t_separator *sep = control->data;
    GtkWidget *child;

    child = gtk_bin_get_child (GTK_BIN (sep->align));

    if (child)
	gtk_container_remove (GTK_CONTAINER (sep->align), child);

    if (orientation == HORIZONTAL)
    {
	gtk_container_add (GTK_CONTAINER (sep->align), sep->vsep);
	gtk_widget_show (sep->vsep);
    }
    else
    {
	gtk_container_add (GTK_CONTAINER (sep->align), sep->hsep);
	gtk_widget_show (sep->hsep);
    }
}

static t_separator *
separator_new (void)
{
    t_separator *sep = g_new0 (t_separator, 1);

    sep->box = gtk_event_box_new ();
    gtk_widget_show (sep->box);
    gtk_container_set_border_width (GTK_CONTAINER (sep->box), 3);

    sep->align = gtk_alignment_new (0.5, 0.5, 0.75, 0.75);
    gtk_widget_show (sep->align);
    gtk_container_add (GTK_CONTAINER (sep->box), sep->align);

    sep->hsep = gtk_hseparator_new ();
    g_object_ref (sep->hsep);
    gtk_object_sink (GTK_OBJECT (sep->hsep));

    sep->vsep = gtk_vseparator_new ();
    g_object_ref (sep->vsep);
    gtk_object_sink (GTK_OBJECT (sep->vsep));

    return sep;
}

/* panel control */
gboolean
create_separator_control (Control * control)
{
    t_separator *sep;

    sep = separator_new ();
    gtk_container_add (GTK_CONTAINER (control->base), sep->box);
    gtk_widget_set_size_request (control->base, -1, -1);

    control->data = (gpointer) sep;
    control->with_popup = FALSE;

    separator_set_orientation (control, settings.orientation);

    return TRUE;
}

G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
    cc->name = "separator";
    cc->caption = _("Separator");

    cc->create_control = (CreateControlFunc) create_separator_control;

    cc->free = separator_free;
    cc->attach_callback = separator_attach_callback;

    cc->set_size = separator_set_size;
    cc->set_orientation = separator_set_orientation;
}

XFCE_PLUGIN_CHECK_INIT
