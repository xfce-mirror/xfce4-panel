/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
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

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-arrow-button.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#define SEPARATOR_WIDTH  8
#define SEP_START        0.15
#define SEP_END          0.85

typedef enum 
{
    SEP_SPACE,
    SEP_EXPAND,
    SEP_LINE,
    SEP_HANDLE,
    SEP_DOTS
}
SeparatorType;

typedef struct _Separator
{
    XfcePanelPlugin *plugin;
    SeparatorType    type;
}
Separator;

static void separator_properties_dialog (XfcePanelPlugin *plugin, Separator *sep);

static void separator_construct (XfcePanelPlugin *plugin);

static void separator_paint_dots (GtkWidget *widget, GdkRectangle * area, Separator *sep);

static const unsigned char dark_bits[]  = { 0x00, 0x0e, 0x02, 0x02, 0x00, 0x00, };
static const unsigned char light_bits[] = { 0x00, 0x00, 0x10, 0x10, 0x1c, 0x00, };
static const unsigned char mid_bits[]   = { 0x00, 0x00, 0x0c, 0x0c, 0x00, 0x00, };


/* -------------------------------------------------------------------- *
 *                     Panel Plugin Interface                           *
 * -------------------------------------------------------------------- */

/* Register with the panel */

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (separator_construct);


/* Interface Implementation */

static gboolean
separator_expose (GtkWidget      *widget, 
                  GdkEventExpose *event,
                  Separator      *sep)
{
    if (GTK_WIDGET_DRAWABLE (widget))
    {
        GtkAllocation  *allocation;
        gboolean        horizontal;
        int             start, end, position;
        int             x, y, w, h;

        allocation = &(widget->allocation);
        horizontal = (xfce_panel_plugin_get_orientation (sep->plugin)
                       == GTK_ORIENTATION_HORIZONTAL);

        switch (sep->type)
        {
            case SEP_SPACE:
            case SEP_EXPAND:
                /* do nothing */
                break;

            case SEP_LINE:
                if (horizontal)
                {
                    start = allocation->y + SEP_START * allocation->height;
                    end   = allocation->y + SEP_END   * allocation->height;
                    position = allocation->x + allocation->width / 2;

                    gtk_paint_vline (widget->style, 
                                     widget->window,
                                     GTK_WIDGET_STATE (widget), 
                                     &(event->area), 
                                     widget, 
                                     "separator",
                                     start, end, position);
                }
                else
                {
                    start = allocation->x + SEP_START * allocation->width;
                    end   = allocation->x + SEP_END   * allocation->width;
                    position = allocation->y + allocation->height / 2;

                    gtk_paint_hline (widget->style, 
                                     widget->window,
                                     GTK_WIDGET_STATE (widget), 
                                     &(event->area), 
                                     widget, 
                                     "separator",
                                     start, end, position);
                }
                break;

            case SEP_HANDLE:
                x = allocation->x;
                y = allocation->y;
                w = allocation->width;
                h = allocation->height;

                gtk_paint_handle (widget->style, 
                                  widget->window,
                                  GTK_WIDGET_STATE (widget), 
                                  GTK_SHADOW_NONE,
                                  &(event->area), 
                                  widget, 
                                  "handlebox",
                                  x, y, w, h,
                                  horizontal ? GTK_ORIENTATION_VERTICAL :
                                               GTK_ORIENTATION_HORIZONTAL);
                break;

            case SEP_DOTS:
                separator_paint_dots (widget, &(event->area), sep);
                break;

            default:
                /* do nothing */
                break;
        }

        return TRUE;
    }

    return FALSE;
}

static void
separator_add_widget (Separator *sep)
{
    GtkWidget *widget;

    widget = gtk_drawing_area_new ();
    gtk_widget_show (widget);
    gtk_container_add (GTK_CONTAINER (sep->plugin), widget);

    xfce_panel_plugin_add_action_widget (sep->plugin, widget);

    g_signal_connect_after (widget, "expose-event",
                            G_CALLBACK (separator_expose), sep);
}

static void
separator_orientation_changed (XfcePanelPlugin *plugin,
                               GtkOrientation   orientation,
                               Separator       *sep)
{
    gtk_widget_queue_draw(GTK_WIDGET(plugin));
}

static gboolean
separator_set_size (XfcePanelPlugin *plugin, 
                    int              size,
                    Separator       *sep)
{
    if (xfce_panel_plugin_get_orientation (plugin) ==
            GTK_ORIENTATION_HORIZONTAL)
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin),
                                     SEPARATOR_WIDTH, size);
    }
    else
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin),
                                     size, SEPARATOR_WIDTH);
    }

    return TRUE;
}

static void
separator_read_rc_file (XfcePanelPlugin *plugin, 
                        Separator       *sep)
{
    char   *file;
    XfceRc *rc;
    int     type = SEP_LINE;

    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (rc != NULL)
        {
            type = xfce_rc_read_int_entry (rc, "separator-type", SEP_LINE);

            /* backward compatibility with xfce 4.4 */
            if (type == SEP_LINE && xfce_rc_has_entry (rc, "expand"))
            {
                if (xfce_rc_read_int_entry (rc, "expand", 0) == 1)
                {
                    type = SEP_EXPAND;
                }
                else if (xfce_rc_read_int_entry (rc, "draw-separator", 1) == 0)
                {
                    type = SEP_SPACE;
                }
            }

            xfce_rc_close (rc);
        }
    }

    sep->type = type;

    if (sep->type > SEP_EXPAND)
        separator_add_widget (sep);
    else if (sep->type == SEP_EXPAND)
        xfce_panel_plugin_set_expand (plugin, TRUE);
}

static void
separator_write_rc_file (XfcePanelPlugin *plugin,
                         Separator       *sep)
{
    char   *file;
    XfceRc *rc;

    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;

    xfce_rc_write_int_entry (rc, "separator-type", sep->type);

    xfce_rc_close (rc);
}

static void
separator_free_data (XfcePanelPlugin *plugin, 
                     Separator       *sep)
{
    GtkWidget *dlg = g_object_get_data (G_OBJECT (plugin), "dialog");

    if (dlg)
        gtk_widget_destroy (dlg);

    panel_slice_free (Separator, sep);
}

/* create widgets and connect to signals */
static void
separator_construct (XfcePanelPlugin *plugin)
{
    Separator *sep = panel_slice_new0 (Separator);

    sep->plugin = plugin;
    sep->type   = SEP_LINE;

    g_signal_connect (plugin, "orientation-changed",
                      G_CALLBACK (separator_orientation_changed), sep);

    g_signal_connect (plugin, "size-changed",
                      G_CALLBACK (separator_set_size), sep);

    g_signal_connect (plugin, "save",
                      G_CALLBACK (separator_write_rc_file), sep);

    g_signal_connect (plugin, "free-data",
                      G_CALLBACK (separator_free_data), sep);

    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin",
                      G_CALLBACK (separator_properties_dialog), sep);

    separator_read_rc_file (plugin, sep);
}

/* -------------------------------------------------------------------- *
 *                        Configuration Dialog                          *
 * -------------------------------------------------------------------- */

static void
change_style (GtkToggleButton *tb, 
              Separator       *sep,
              SeparatorType    type)
{
    if (gtk_toggle_button_get_active (tb))
    {
        gboolean add_widget = FALSE;
        gboolean expand     = FALSE;

        sep->type = type;

        switch( type )
        {
            case SEP_SPACE:
                /* do nothing */
                break;
            case SEP_EXPAND:
                expand = TRUE;
                break;
            case SEP_LINE:
            case SEP_HANDLE:
            case SEP_DOTS:
                add_widget = TRUE;
                break;
        }

        if (add_widget && !GTK_BIN(sep->plugin)->child)
        {
            separator_add_widget (sep);
        }
        else if (!add_widget && GTK_BIN(sep->plugin)->child)
        {
            gtk_widget_destroy (GTK_BIN(sep->plugin)->child);
        }

        xfce_panel_plugin_set_expand (sep->plugin, expand);
        gtk_widget_queue_draw (GTK_WIDGET(sep->plugin));
    }
}

static void
space_toggled (GtkToggleButton *tb, 
               Separator       *sep)
{
    change_style (tb, sep, SEP_SPACE);
}

static void
expand_toggled (GtkToggleButton *tb, 
                Separator       *sep)
{
    change_style (tb, sep, SEP_EXPAND);
}

static void
line_toggled (GtkToggleButton *tb, 
              Separator       *sep)
{
    change_style (tb, sep, SEP_LINE);
}

static void
handle_toggled (GtkToggleButton *tb, 
                Separator       *sep)
{
    change_style (tb, sep, SEP_HANDLE);
}

static void
dots_toggled (GtkToggleButton *tb, 
              Separator       *sep)
{
    change_style (tb, sep, SEP_DOTS);
}

static void
separator_dialog_response (GtkWidget *dlg, 
                           int        reponse,
                           Separator *sep)
{
    g_object_set_data (G_OBJECT (sep->plugin), "dialog", NULL);

    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (sep->plugin);
    separator_write_rc_file (sep->plugin, sep);
}

static void
separator_properties_dialog (XfcePanelPlugin *plugin,
                             Separator       *sep)
{
    GtkWidget *dlg, *vbox, *frame, *tb;

    xfce_panel_plugin_block_menu (plugin);

    dlg = xfce_titled_dialog_new_with_buttons (_("Separator or Spacing"), NULL,
                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);

    gtk_window_set_screen (GTK_WINDOW (dlg),
                           gtk_widget_get_screen (GTK_WIDGET (plugin)));

    g_object_set_data (G_OBJECT (plugin), "dialog", dlg);

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name (GTK_WINDOW (dlg), GTK_STOCK_PROPERTIES);

    g_signal_connect (dlg, "response", G_CALLBACK (separator_dialog_response),
                      sep);

    vbox = gtk_vbox_new (FALSE, 6);
    gtk_widget_show (vbox);

    frame = xfce_create_framebox_with_content(_("Separator Style"), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame,
                        TRUE, TRUE, 0);

    /* space */
    tb = gtk_radio_button_new_with_mnemonic (NULL, 
                                             _("_Empty space"));
    gtk_widget_show (tb);
    gtk_box_pack_start (GTK_BOX (vbox), tb, FALSE, FALSE, 0);
    if (sep->type == SEP_SPACE)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), TRUE);
    g_signal_connect (tb, "toggled", G_CALLBACK (space_toggled), sep);

    /* expand */
    tb = gtk_radio_button_new_with_mnemonic_from_widget (
            GTK_RADIO_BUTTON(tb), _("E_xpanding empty space"));
    gtk_widget_show (tb);
    gtk_box_pack_start (GTK_BOX (vbox), tb, FALSE, FALSE, 0);
    if (sep->type == SEP_EXPAND)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), TRUE);
    g_signal_connect (tb, "toggled", G_CALLBACK (expand_toggled), sep);

    /* line */
    tb = gtk_radio_button_new_with_mnemonic_from_widget (
            GTK_RADIO_BUTTON(tb), _("_Line"));
    gtk_widget_show (tb);
    gtk_box_pack_start (GTK_BOX (vbox), tb, FALSE, FALSE, 0);
    if (sep->type == SEP_LINE)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), TRUE);
    g_signal_connect (tb, "toggled", G_CALLBACK (line_toggled), sep);

    /* handle */
    tb = gtk_radio_button_new_with_mnemonic_from_widget (
            GTK_RADIO_BUTTON(tb), _("_Handle"));
    gtk_widget_show (tb);
    gtk_box_pack_start (GTK_BOX (vbox), tb, FALSE, FALSE, 0);
    if (sep->type == SEP_HANDLE)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), TRUE);
    g_signal_connect (tb, "toggled", G_CALLBACK (handle_toggled), sep);

    /* dots */
    tb = gtk_radio_button_new_with_mnemonic_from_widget (
            GTK_RADIO_BUTTON(tb), _("_Dots"));
    gtk_widget_show (tb);
    gtk_box_pack_start (GTK_BOX (vbox), tb, FALSE, FALSE, 0);
    if (sep->type == SEP_DOTS)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), TRUE);
    g_signal_connect (tb, "toggled", G_CALLBACK (dots_toggled), sep);

    gtk_widget_show (dlg);
}

static void
separator_paint_dots (GtkWidget *widget, GdkRectangle * area, Separator *sep)
{
    GdkBitmap *dark_bmap, *mid_bmap, *light_bmap;
    gint       x, y, w, h, rows, cols;
    gint       width, height;
    
    width  = widget->allocation.width;
    height = widget->allocation.height;

    dark_bmap = gdk_bitmap_create_from_data (widget->window, 
                                             (const gchar*) dark_bits,
                                             6, 6);
    mid_bmap = gdk_bitmap_create_from_data (widget->window, 
                                             (const gchar*) mid_bits,
                                             6, 6);
    light_bmap = gdk_bitmap_create_from_data (widget->window, 
                                             (const gchar*) light_bits,
                                             6, 6);
    if (area)
    {
        gdk_gc_set_clip_rectangle (widget->style->light_gc[widget->state],
                                   area);
        gdk_gc_set_clip_rectangle (widget->style->dark_gc[widget->state],
                                   area);
        gdk_gc_set_clip_rectangle (widget->style->mid_gc[widget->state],
                                   area);
    }

    if (xfce_panel_plugin_get_orientation (sep->plugin) == GTK_ORIENTATION_HORIZONTAL)
      {
        rows = MAX (height / 6, 1);
        w = 6;
        h = rows * 6;
      }
    else
      {
        cols = MAX (width / 6, 1);
        h = 6;
        w = cols * 6;
      }

    x = (width - w) / 2;
    y = (height - h) / 2;


    gdk_gc_set_stipple (widget->style->light_gc[widget->state],
                        light_bmap);
    gdk_gc_set_stipple (widget->style->mid_gc[widget->state],
                        mid_bmap);
    gdk_gc_set_stipple (widget->style->dark_gc[widget->state],
                        dark_bmap);

    gdk_gc_set_fill (widget->style->light_gc[widget->state], GDK_STIPPLED);
    gdk_gc_set_fill (widget->style->mid_gc[widget->state], GDK_STIPPLED);
    gdk_gc_set_fill (widget->style->dark_gc[widget->state], GDK_STIPPLED);

    gdk_gc_set_ts_origin (widget->style->light_gc[widget->state], x, y);
    gdk_gc_set_ts_origin (widget->style->mid_gc[widget->state], x, y);
    gdk_gc_set_ts_origin (widget->style->dark_gc[widget->state], x, y);

    gdk_draw_rectangle (widget->window,
                        widget->style->light_gc[widget->state], 
                        TRUE, 
                        x, y, w, h);
    gdk_draw_rectangle (widget->window,
                        widget->style->mid_gc[widget->state], 
                        TRUE, 
                        x, y, w, h);
    gdk_draw_rectangle (widget->window,
                        widget->style->dark_gc[widget->state], 
                        TRUE, 
                        x, y, w, h);

    gdk_gc_set_fill (widget->style->light_gc[widget->state], GDK_SOLID);
    gdk_gc_set_fill (widget->style->mid_gc[widget->state], GDK_SOLID);
    gdk_gc_set_fill (widget->style->dark_gc[widget->state], GDK_SOLID);

    if (area)
    {
        gdk_gc_set_clip_rectangle (widget->style->light_gc[widget->state],
                                   NULL);
        gdk_gc_set_clip_rectangle (widget->style->dark_gc[widget->state],
                                   NULL);
        gdk_gc_set_clip_rectangle (widget->style->mid_gc[widget->state],
                                   NULL);
    }

    g_object_unref (dark_bmap);
    g_object_unref (mid_bmap);
    g_object_unref (light_bmap);
}


