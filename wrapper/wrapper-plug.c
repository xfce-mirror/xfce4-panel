/* $Id$ */
/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gdk/gdk.h>
#include <panel/panel-private.c>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <wrapper/wrapper-plug.h>



static void wrapper_plug_class_init (WrapperPlugClass *klass);
static void wrapper_plug_init (WrapperPlug *plug);
static void wrapper_plug_finalize (GObject *object);
static gboolean wrapper_plug_client_event (GtkWidget *widget, GdkEventClient *event);



struct _WrapperPlugClass
{
  GtkPlugClass __parent__;
};

struct _WrapperPlug
{
  GtkPlug __parent__;

  /* the panel plugin */
  XfcePanelPluginProvider *provider;

  /* socket id of panel window */
  GdkNativeWindow          socket_id;

  /* the message atom */
  GdkAtom                  atom;

  /* background alpha */
  gdouble                  background_alpha;

  /* if this plugin is on an active panel */
  guint                    is_active_panel : 1;
};



G_DEFINE_TYPE (WrapperPlug, wrapper_plug, GTK_TYPE_PLUG);



static gboolean
wrapper_plug_expose_event (GtkWidget      *widget,
                           GdkEventExpose *event)
{
  WrapperPlug    *plug = WRAPPER_PLUG (widget);
  cairo_t        *cr;
  GdkColor       *color;
  GtkStateType    state = GTK_STATE_NORMAL;
  GtkOrientation  orientation;
  GtkAllocation  *alloc = &(widget->allocation);
  gdouble         alpha = plug->background_alpha;

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      /* create the cairo context */
      cr = gdk_cairo_create (widget->window);

      if (alpha < 1.00 || plug->is_active_panel)
        {
          /* change the state is this plugin is on an active panel */
          if (G_UNLIKELY (plug->is_active_panel))
            state = GTK_STATE_SELECTED;

          /* get the background gdk color */
          color = &(widget->style->bg[state]);

          /* set the cairo source color */
          _set_source_rgba (cr, color, alpha);

          /* create retangle */
          cairo_rectangle (cr, event->area.x, event->area.y,
                           event->area.width, event->area.height);

          /* draw on source */
          cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

          /* paint rectangle */
          cairo_fill (cr);
        }

      /* 1px line */
      cairo_set_line_width (cr, 2.0);

      /* get the plugin orientation */
      orientation = xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plug->provider));

      /* dark color */
      color = &(widget->style->light[state]);
      _set_source_rgba (cr, color, alpha);

      /* move the cursor to the top left corner */
      cairo_move_to (cr, alloc->x, alloc->y);

      /* draw the light line */
      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        cairo_rel_line_to (cr, alloc->width, 0);
      else
        cairo_rel_line_to (cr, 0, alloc->height);

      /* stroke the lines */
      cairo_stroke (cr);

      /* light color */
      color = &(widget->style->dark[state]);
      _set_source_rgba (cr, color, alpha);

      /* set start position to bottom right */
      cairo_move_to (cr, alloc->x + alloc->width, alloc->y + alloc->height);

      /* draw the dark line */
      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        cairo_rel_line_to (cr, -alloc->width, 0);
      else
        cairo_rel_line_to (cr, 0, -alloc->height);

      /* stroke the lines */
      cairo_stroke (cr);

      /* destroy cairo context */
      cairo_destroy (cr);
    }

    return GTK_WIDGET_CLASS(wrapper_plug_parent_class)->expose_event(widget, event);
}



static void
wrapper_plug_class_init (WrapperPlugClass *klass)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = wrapper_plug_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->client_event = wrapper_plug_client_event;
  gtkwidget_class->expose_event = wrapper_plug_expose_event;
}



static void
wrapper_plug_init (WrapperPlug *plug)
{
  plug->socket_id = 0;
  plug->atom = panel_atom_intern ("XFCE_PANEL_PLUGIN");
  plug->background_alpha = 1.00;
  plug->is_active_panel = FALSE;

  GdkScreen   *screen;
  GdkColormap *colormap;
  GtkWidget   *widget = GTK_WIDGET (plug);

  /* try to set the rgba colormap */
  screen = gtk_window_get_screen (GTK_WINDOW (plug));
  if (gdk_screen_is_composited (screen))
    {
      /* try to get the rgba colormap */
      colormap = gdk_screen_get_rgba_colormap (screen);

      if (G_LIKELY (colormap))
        {
          /* set the colormap */
          gtk_widget_set_colormap (widget, colormap);
        }
    }

  gtk_widget_set_app_paintable (GTK_WIDGET (plug), TRUE);
}



static void
wrapper_plug_finalize (GObject *object)
{
  //WrapperPlug *plug = WRAPPER_PLUG (object);

  (*G_OBJECT_CLASS (wrapper_plug_parent_class)->finalize) (object);
}



static gboolean
wrapper_plug_client_event (GtkWidget      *widget,
                           GdkEventClient *event)
{
  WrapperPlug            *plug = WRAPPER_PLUG (widget);
  XfcePanelPluginMessage  message;
  glong                   value;

  /* check if this is a panel client event */
  if (G_LIKELY (event->message_type == plug->atom))
    {
      /* get message and value */
      message = event->data.l[0];
      value = event->data.l[1];

      switch (message)
        {
          case MESSAGE_SET_SENSITIVE:
            /* set the sensitivity of the plug */
            gtk_widget_set_sensitive (widget, !!(value == 1));
            break;

          case MESSAGE_SET_SIZE:
            /* set the new plugin size */
            xfce_panel_plugin_provider_set_size (plug->provider, value);
            break;

          case MESSAGE_SET_ORIENTATION:
            /* set the plugin orientation */
            xfce_panel_plugin_provider_set_orientation (plug->provider, value);
            break;

          case MESSAGE_SET_SCREEN_POSITION:
            /* set the plugin screen position */
            xfce_panel_plugin_provider_set_screen_position (plug->provider, value);
            break;

          case MESSAGE_SET_BACKGROUND_ALPHA:
            /* set the background alpha */
            plug->background_alpha = CLAMP (value, 0, 100) / 100.00;

            /* redraw the window */
            gtk_widget_queue_draw (widget);
            break;

          case MESSAGE_SET_ACTIVE_PANEL:
            /* set if this plugin is on an active panel */
            plug->is_active_panel = !!(value == 1);

            /* redraw the window */
            gtk_widget_queue_draw (widget);
            break;

          case MESSAGE_SAVE:
            /* save the plugin */
            xfce_panel_plugin_provider_save (plug->provider);
            break;

          case MESSAGE_QUIT:
            /* don't send messages */
            plug->socket_id = 0;

            /* quit the main loop (destroy plugin) */
            gtk_main_quit ();
            break;

          default:
            g_message ("The wrapper received an unknown message: %d", message);
            break;
        }

      /* we handled the event */
      return TRUE;
    }

  /* propagate event */
  if (GTK_WIDGET_CLASS (wrapper_plug_parent_class)->client_event)
    return (*GTK_WIDGET_CLASS (wrapper_plug_parent_class)->client_event) (widget, event);
  else
    return FALSE;
}



static void
wrapper_plug_send_message (WrapperPlug            *plug,
                           XfcePanelPluginMessage  message,
                           glong                   value)
{
  GdkEventClient event;

  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  if (G_LIKELY (plug->socket_id > 0))
    {
      /* setup the event */
      event.type = GDK_CLIENT_EVENT;
      event.window = GTK_WIDGET (plug)->window;
      event.send_event = TRUE;
      event.message_type = plug->atom;
      event.data_format = 32;
      event.data.l[0] = message;
      event.data.l[1] = value;
      event.data.l[2] = 0;

      /* don't crash on x errors */
      gdk_error_trap_push ();

      /* send the event to the wrapper */
      gdk_event_send_client_message_for_display (gdk_display_get_default (), (GdkEvent *) &event, plug->socket_id);

      /* flush the x output buffer */
      gdk_flush ();

      /* pop the push */
      gdk_error_trap_pop ();
    }
}



static void
wrapper_plug_message_expand_changed (XfcePanelPluginProvider *provider,
                                     gboolean                 expand,
                                     WrapperPlug             *plug)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));
  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  /* send signal */
  wrapper_plug_send_message (plug, MESSAGE_EXPAND_CHANGED, expand ? 1 : 0);
}



static void
wrapper_plug_message_move_item (XfcePanelPluginProvider *provider,
                                WrapperPlug             *plug)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));
  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  /* send signal */
  wrapper_plug_send_message (plug, MESSAGE_MOVE_ITEM, 0);
}



static void
wrapper_plug_message_add_new_items (XfcePanelPluginProvider *provider,
                                    WrapperPlug             *plug)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));
  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  /* send signal */
  wrapper_plug_send_message (plug, MESSAGE_ADD_NEW_ITEMS, 0);
}



static void
wrapper_plug_message_panel_preferences (XfcePanelPluginProvider *provider,
                                        WrapperPlug             *plug)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));
  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  /* send signal */
  wrapper_plug_send_message (plug, MESSAGE_PANEL_PREFERENCES, 0);
}



static void
wrapper_plug_message_remove (XfcePanelPluginProvider *provider,
                             WrapperPlug             *plug)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));
  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  if (plug->socket_id > 0)
    {
      /* send signal */
      wrapper_plug_send_message (plug, MESSAGE_REMOVE, 0);

      /* quit the wrapper */
      gtk_main_quit ();
    }
}



GtkWidget *
wrapper_plug_new (GdkNativeWindow          socket_id,
                  XfcePanelPluginProvider *provider)
{
  WrapperPlug *plug;

  /* create new object */
  plug = g_object_new (WRAPPER_TYPE_PLUG, NULL);

  /* store info */
  plug->socket_id = socket_id;
  plug->provider = provider;

  /* monitor changes in the provider */
  g_signal_connect (G_OBJECT (provider), "expand-changed", G_CALLBACK (wrapper_plug_message_expand_changed), plug);
  g_signal_connect (G_OBJECT (provider), "move-item", G_CALLBACK (wrapper_plug_message_move_item), plug);
  g_signal_connect (G_OBJECT (provider), "add-new-items", G_CALLBACK (wrapper_plug_message_add_new_items), plug);
  g_signal_connect (G_OBJECT (provider), "panel-preferences", G_CALLBACK (wrapper_plug_message_panel_preferences), plug);
  g_signal_connect (G_OBJECT (provider), "destroy", G_CALLBACK (wrapper_plug_message_remove), plug);

  /* contruct the plug */
  gtk_plug_construct (GTK_PLUG (plug), socket_id);

  /* send the plug id back to the panel */
  wrapper_plug_send_message (plug, MESSAGE_SET_PLUG_ID, gtk_plug_get_id (GTK_PLUG (plug)));

  return GTK_WIDGET (plug);
}
