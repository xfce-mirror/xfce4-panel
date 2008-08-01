/*  $Id$
 *
 *  Copyright (c) 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <time.h>
#include <gtk/gtk.h>

#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-window.h>
#include <libxfce4panel/xfce-itembar.h>
#include <libxfce4panel/xfce-panel-item-iface.h>

#include <libxfce4panel/libxfce4panel-enum-types.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#include <libxfce4panel/xfce-panel-macros.h>

#include "panel.h"
#include "panel-app.h"
#include "panel-properties.h"
#include "panel-item-manager.h"
#include "panel-dnd.h"

/* PanelPrivate struct */
#include "panel-private.h"

#ifndef _
#define _(x) x
#endif

enum
{
    PROP_0,
    PROP_SIZE,
    PROP_MONITOR,
    PROP_SCREEN_POSITION,
    PROP_XOFFSET,
    PROP_YOFFSET,
    PROP_AUTOHIDE,
    PROP_FULL_WIDTH,
    PROP_TRANSPARENCY,
    PROP_ACTIVE_TRANS
};


/* GObject */
static void panel_finalize     (GObject * object);

static void panel_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec);

static void panel_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec);

/* GtkWidget */
static void panel_size_request  (GtkWidget      *widget,
                                 GtkRequisition *requisition);

static gboolean panel_button_pressed (GtkWidget      *widget,
                                      GdkEventButton *ev);

/* plugin menu */
static void panel_menu_deactivated (GtkWidget *item);

static void panel_menu_opened (GtkWidget *item);
static void _item_start_move (GtkWidget *item, Panel     *panel);

/* DND dest */
static void _panel_drag_data_received (GtkWidget        *widget,
                                       GdkDragContext   *context,
                                       gint              x,
                                       gint              y,
                                       GtkSelectionData *data,
                                       guint             info,
                                       guint             time,
                                       Panel            *panel);

static gboolean _panel_drag_drop (GtkWidget      *widget,
                                  GdkDragContext *context,
                                  gint            x,
                                  gint            y,
                                  guint           time,
                                  Panel          *panel);

/* DND source */
static void _panel_drag_begin (GtkWidget      *widget,
                               GdkDragContext *drag_context,
                               Panel          *panel);

/* pass through button press events */
static gboolean _panel_itembar_button_pressed (GtkWidget      *widget,
                                               GdkEventButton *ev,
                                               Panel          *panel);

/* menu */
static GtkWidget *_panel_create_menu (Panel *panel);


/* this sets up a lot of stuff, see GObject API reference */
G_DEFINE_TYPE (Panel, panel, XFCE_TYPE_PANEL_WINDOW);

static void
panel_class_init (PanelClass * klass)
{
    GObjectClass   *object_class;
    GtkWidgetClass *widget_class;
    GParamSpec     *pspec;

    g_type_class_add_private (klass, sizeof (PanelPrivate));

    object_class = (GObjectClass *) klass;
    widget_class = (GtkWidgetClass *) klass;

    object_class->finalize           = panel_finalize;
    object_class->get_property       = panel_get_property;
    object_class->set_property       = panel_set_property;

    widget_class->button_press_event = panel_button_pressed;
    widget_class->size_request       = panel_size_request;

    /* properties */

    pspec = g_param_spec_int ("size",
                              "Size",
                              "The size of the panel",
                              MIN_SIZE, MAX_SIZE,
                              DEFAULT_SIZE, G_PARAM_READWRITE);

    g_object_class_install_property (object_class, PROP_SIZE, pspec);

    pspec = g_param_spec_int ("monitor",
                              "Monitor",
                              "The monitor number of the panel",
                              0, G_MAXINT,
                              DEFAULT_MONITOR, G_PARAM_READWRITE);

    g_object_class_install_property (object_class, PROP_MONITOR, pspec);

    pspec = g_param_spec_enum ("screen-position",
                               "Screen position",
                               "The screen position of the panel",
                               XFCE_TYPE_SCREEN_POSITION,
                               DEFAULT_SCREEN_POSITION, G_PARAM_READWRITE);

    g_object_class_install_property (object_class, PROP_SCREEN_POSITION,
                                     pspec);

    pspec = g_param_spec_int ("xoffset",
                              "Offset in x direction",
                              "Offset in x direction",
                              0, G_MAXINT,
                              DEFAULT_XOFFSET, G_PARAM_READWRITE);

    g_object_class_install_property (object_class, PROP_XOFFSET, pspec);

    pspec = g_param_spec_int ("yoffset",
                              "Offset in y direction",
                              "Offset in y direction",
                              0, G_MAXINT,
                              DEFAULT_YOFFSET, G_PARAM_READWRITE);

    g_object_class_install_property (object_class, PROP_YOFFSET, pspec);

    pspec = g_param_spec_boolean ("autohide",
                                  "panel_autohide",
                                  "Automatically hide the panel",
                                  DEFAULT_AUTOHIDE, G_PARAM_READWRITE);

    g_object_class_install_property (object_class, PROP_AUTOHIDE, pspec);

    pspec = g_param_spec_int ("fullwidth",
                              "panel_fullwidth",
                              "Use the full screen width",
                              XFCE_PANEL_NORMAL_WIDTH,
                              XFCE_PANEL_SPAN_MONITORS,
                              DEFAULT_FULL_WIDTH, G_PARAM_READWRITE);

    g_object_class_install_property (object_class, PROP_FULL_WIDTH, pspec);

    pspec = g_param_spec_int ("transparency",
                              "panel_transparency",
                              "Transparency of the panel",
                              0, 100,
                              DEFAULT_TRANSPARENCY, G_PARAM_READWRITE);

    g_object_class_install_property (object_class, PROP_TRANSPARENCY, pspec);

    pspec = g_param_spec_boolean ("activetrans",
                                  "panel_activetrans",
                                  "Keep the active panel transparent",
                                  DEFAULT_ACTIVE_TRANS, G_PARAM_READWRITE);

    g_object_class_install_property (object_class, PROP_ACTIVE_TRANS, pspec);
}

static void
panel_init (Panel * panel)
{
    PanelPrivate *priv;

    priv = panel->priv    = G_TYPE_INSTANCE_GET_PRIVATE (panel,
                                                         PANEL_TYPE_PANEL,
                                                         PanelPrivate);

    priv->size            = DEFAULT_SIZE;
    priv->monitor         = DEFAULT_MONITOR;
    priv->screen_position = DEFAULT_SCREEN_POSITION;
    priv->xoffset         = DEFAULT_XOFFSET;
    priv->yoffset         = DEFAULT_YOFFSET;
    priv->autohide        = DEFAULT_AUTOHIDE;
    priv->full_width      = DEFAULT_FULL_WIDTH;
    priv->transparency    = DEFAULT_TRANSPARENCY;
    priv->activetrans     = DEFAULT_ACTIVE_TRANS;

    priv->itembar = xfce_itembar_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_show (priv->itembar);
    gtk_container_add (GTK_CONTAINER (panel), priv->itembar);

    panel_dnd_set_dest_name_and_widget (priv->itembar);
    panel_dnd_set_source_widget (priv->itembar);

    /* don't allow the wm to close the panel window */
    g_signal_connect (panel, "delete-event", G_CALLBACK (gtk_true), NULL);

    /* DND */
    g_signal_connect (priv->itembar, "drag-data-received",
                      G_CALLBACK (_panel_drag_data_received), panel);

    g_signal_connect (priv->itembar, "drag-drop",
                      G_CALLBACK (_panel_drag_drop), panel);

    g_signal_connect (priv->itembar, "drag-begin",
                      G_CALLBACK (_panel_drag_begin), panel);

    /* mouse click */
    g_signal_connect (priv->itembar, "button-press-event",
                      G_CALLBACK (_panel_itembar_button_pressed), panel);

    /* menu */
    priv->menu = _panel_create_menu (PANEL (panel));

    priv->opacity = priv->saved_opacity = 0;
}

static void
panel_finalize (GObject * object)
{
    /* TODO: properly ref and unref private widgets  */

    G_OBJECT_CLASS (panel_parent_class)->finalize (object);
}

static void
panel_get_property (GObject    *object,
                    guint       prop_id,
                    GValue     *value,
                    GParamSpec *pspec)
{
    PanelPrivate *priv = PANEL(object)->priv;

    switch (prop_id)
    {
        case PROP_SIZE:
            g_value_set_int (value, priv->size);
            break;
        case PROP_MONITOR:
            g_value_set_int (value, priv->monitor);
            break;
        case PROP_SCREEN_POSITION:
            g_value_set_enum (value, priv->screen_position);
            break;
        case PROP_XOFFSET:
            g_value_set_int (value, priv->xoffset);
            break;
        case PROP_YOFFSET:
            g_value_set_int (value, priv->yoffset);
            break;
        case PROP_AUTOHIDE:
            g_value_set_boolean (value, priv->autohide);
            break;
        case PROP_FULL_WIDTH:
            g_value_set_int (value, priv->full_width);
            break;
        case PROP_TRANSPARENCY:
            g_value_set_int (value, priv->transparency);
            break;
        case PROP_ACTIVE_TRANS:
            g_value_set_boolean (value, priv->activetrans);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
panel_set_property (GObject      *object,
                    guint         prop_id,
                    const GValue *value,
                    GParamSpec   *pspec)
{
    Panel *panel = PANEL (object);

    switch (prop_id)
    {
        case PROP_SIZE:
            panel_set_size (panel, g_value_get_int (value));
            break;
        case PROP_MONITOR:
            panel_set_monitor (panel, g_value_get_int (value));
            break;
        case PROP_SCREEN_POSITION:
            panel_set_screen_position (panel, g_value_get_enum (value));
            break;
        case PROP_XOFFSET:
            panel_set_xoffset (panel, g_value_get_int (value));
            break;
        case PROP_YOFFSET:
            panel_set_yoffset (panel, g_value_get_int (value));
            break;
        case PROP_AUTOHIDE:
            panel_set_autohide (panel, g_value_get_boolean (value));
            break;
        case PROP_FULL_WIDTH:
            panel_set_full_width (panel, g_value_get_int (value));
            break;
        case PROP_TRANSPARENCY:
            panel_set_transparency (panel, g_value_get_int (value));
            break;
        case PROP_ACTIVE_TRANS:
            panel_set_activetrans (panel, g_value_get_boolean (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
panel_size_request  (GtkWidget      *widget,
                     GtkRequisition *requisition)
{
    PanelPrivate *priv = PANEL(widget)->priv;

    GTK_WIDGET_CLASS (panel_parent_class)->size_request (widget, requisition);

    if (panel_is_horizontal (PANEL (widget)))
    {
        requisition->width  = MAX (MIN_SIZE,   requisition->width);
        requisition->height = MAX (priv->size, requisition->height);
    }
    else
    {
        requisition->width  = MAX (priv->size, requisition->width);
        requisition->height = MAX (MIN_SIZE,   requisition->height);
    }
}

static gboolean
panel_button_pressed (GtkWidget      *widget,
                      GdkEventButton *ev)
{
    guint modifiers;

    modifiers = gtk_accelerator_get_default_mod_mask ();

    if (ev->button == 3 || (ev->button == 1 &&
        (ev->state & modifiers) == GDK_CONTROL_MASK))
    {
        PanelPrivate *priv;

        priv = PANEL (widget)->priv;

        gtk_menu_set_screen (GTK_MENU (priv->menu),
                             gtk_widget_get_screen (widget));

        gtk_menu_popup (GTK_MENU (priv->menu), NULL, NULL, NULL, NULL,
                        ev->button, ev->time);

        return TRUE;
    }

    return
        GTK_WIDGET_CLASS (panel_parent_class)->button_press_event (widget, ev);
}

/* DND dest */
static void
_panel_drag_data_received (GtkWidget        *widget,
                           GdkDragContext   *context,
                           gint              x,
                           gint              y,
                           GtkSelectionData *data,
                           guint             info,
                           guint             time,
                           Panel            *panel)
{
    XfceItembar   *itembar = XFCE_ITEMBAR (widget);
    PanelPrivate  *priv = panel->priv;
    XfcePanelItem *item;
    GtkWidget     *plugin;
    gint           index;
    gint           oldindex;
    gboolean       expand;
    gboolean       succeed = FALSE;

    DBG (" + drag data received: %d", info);

    /* get the drop index */
    index = xfce_itembar_get_drop_index (itembar, x, y);

    switch (info)
    {
        case TARGET_PLUGIN_NAME:
            if (data->length > 0)
            {
                /* insert the new plugin */
                panel_insert_item (panel, (const gchar *) data->data, index);

                /* succeeded */
                succeed = TRUE;
            }
            break;

        case TARGET_PLUGIN_WIDGET:
            /* get the plugin from the drag context */
            plugin = gtk_drag_get_source_widget (context);

            /* try the drag_widget or leave */
            if (!plugin || !XFCE_IS_PANEL_ITEM (plugin))
                break;

            if (gtk_widget_get_parent (plugin) != widget)
            {
                /* get the plugin and information */
                item   = XFCE_PANEL_ITEM (plugin);
                expand = xfce_panel_item_get_expand (item);

                /* freeze plugin notifications */
                g_object_freeze_notify (G_OBJECT (widget));

                /* move the plugin from the old panel to the new one */
                gtk_widget_reparent (GTK_WIDGET (plugin), widget);

                /* update the plugin */
                xfce_panel_item_set_size (item, priv->size);
                xfce_panel_item_set_screen_position (item, priv->screen_position);

                /* update the itembar */
                xfce_itembar_reorder_child (itembar, plugin, index);
                xfce_itembar_set_child_expand (itembar, plugin, expand);

                /* thaw update notifications */
                g_object_thaw_notify (G_OBJECT (widget));
            }
            else /* move on same panel */
            {
                /* get the old index */
                oldindex = xfce_itembar_get_item_index (itembar, plugin);

                if (index > oldindex)
                    index--;

                if (index != oldindex)
                    xfce_itembar_reorder_child (itembar, plugin, index);
            }

            /* properly handled */
            succeed = TRUE;

            break;

        default:
            break;
    }

    /* finish the drag */
    gtk_drag_finish (context, succeed, FALSE, time);
}

static gboolean
_panel_drag_drop (GtkWidget      *widget,
                  GdkDragContext *context,
                  gint            x,
                  gint            y,
                  guint           time,
                  Panel          *panel)
{
    GdkAtom target = gtk_drag_dest_find_target (widget, context, NULL);

    /* we cannot handle the drag data */
    if (G_UNLIKELY (target == GDK_NONE))
        return FALSE;

    /* request the drag data */
    gtk_drag_get_data (widget, context, target, time);

    /* we call gtk_drag_finish later */
    return TRUE;
}

/* DND source */
static void
_panel_drag_begin (GtkWidget      *widget,
                   GdkDragContext *drag_context,
                   Panel          *panel)
{
    gint         x, y, rootx, rooty;
    GtkWidget   *plugin;

    DBG (" + drag begin");

    /* get the pointer position */
    gdk_display_get_pointer (gtk_widget_get_display (widget), NULL, &x, &y, NULL);

    /* get the window root coordinates */
    gdk_window_get_root_origin (widget->window, &rootx, &rooty);

    /* calc the position inside the panel */
    x -= rootx;
    y -= rooty;

    /* get the plugin on the itembar at this position */
    plugin = xfce_itembar_get_item_at_point (XFCE_ITEMBAR (widget), x, y);

    /* start an item move */
    if (G_LIKELY (plugin))
        _item_start_move (plugin, panel);
}

/* pass through right-click events when the event window of itembar is raised
 */
static gboolean
_panel_itembar_button_pressed (GtkWidget      *widget,
                               GdkEventButton *ev,
                               Panel          *panel)
{
    GtkWidget *plugin;
    guint      modifiers;

    if (xfce_itembar_event_window_is_raised (XFCE_ITEMBAR (widget)))
    {
        modifiers = gtk_accelerator_get_default_mod_mask ();

        if (ev->button == 3 || (ev->button == 1 &&
            (ev->state & modifiers) == GDK_CONTROL_MASK))
        {
            plugin = xfce_itembar_get_item_at_point (XFCE_ITEMBAR (widget),
                                                     ev->x, ev->y);
            if (plugin)
            {
                gtk_widget_event (plugin, (GdkEvent *)ev);
                return TRUE;
            }
        }
    }

    return FALSE;
}

/* menu */

static GtkWidget *
_panel_create_menu (Panel *panel)
{
    GtkWidget *menu, *mi, *img;

    menu = gtk_menu_new ();

    if (xfce_allow_panel_customization ())
    {
        mi = gtk_image_menu_item_new_with_label (_("Customize Panel"));
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        img = gtk_image_new_from_stock (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU);
        gtk_widget_show (img);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);

        g_signal_connect (mi, "activate", G_CALLBACK (panel_app_customize),
                          NULL);

        mi = gtk_image_menu_item_new_with_label (_("Add Items"));
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        img = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
        gtk_widget_show (img);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);

        g_signal_connect_swapped (mi, "activate",
                                  G_CALLBACK (panel_app_customize_items), NULL);

        mi = gtk_separator_menu_item_new ();
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        mi = gtk_image_menu_item_new_with_label (_("Quit"));
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        img = gtk_image_new_from_stock (GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU);
        gtk_widget_show (img);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);

        g_signal_connect (mi, "activate", G_CALLBACK (panel_app_quit), NULL);

        mi = gtk_image_menu_item_new_with_label (_("Restart"));
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        img = gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU);
        gtk_widget_show (img);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);

        g_signal_connect (mi, "activate", G_CALLBACK (panel_app_restart), NULL);

        mi = gtk_separator_menu_item_new ();
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }

    mi = gtk_image_menu_item_new_with_label (_("About the Xfce Panel"));
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    img = gtk_image_new_from_stock (GTK_STOCK_ABOUT, GTK_ICON_SIZE_MENU);
    gtk_widget_show (img);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);

    g_signal_connect_swapped (mi, "activate", G_CALLBACK (panel_app_about),
                              panel);

    return menu;
}

/* public API */

Panel *
panel_new (void)
{
    return PANEL (g_object_new (PANEL_TYPE_PANEL, NULL));
}

void
panel_free_data (Panel *panel)
{
    PanelPrivate *priv;
    GList        *l;
    int           i;

    _panel_return_if_fail (PANEL_IS_PANEL (panel));

    priv = panel->priv;

    /* try and prevent some race conditions */
    priv->block_autohide++;
    xfce_panel_window_set_move_function (XFCE_PANEL_WINDOW (panel),
                                         NULL, NULL);
    xfce_panel_window_set_resize_function (XFCE_PANEL_WINDOW (panel),
                                           NULL, NULL);

    for (l = gtk_container_get_children (GTK_CONTAINER (priv->itembar));
         l != NULL;
         l = l->next)
    {
        XfcePanelItem *item = l->data;

        xfce_panel_item_free_data (item);
    }

    g_list_free (l);

    /* give plugins the chance to quit */
    for (i = 0;  i < 10 &&
         xfce_itembar_get_n_items (XFCE_ITEMBAR (priv->itembar)) != 0; ++i)
    {
        DBG (" + %d item(s) on the panel",
             xfce_itembar_get_n_items (XFCE_ITEMBAR (priv->itembar)));
        g_usleep (200000); /* 0.2 sec */

        while (gtk_events_pending ())
            gtk_main_iteration ();
    }

}

/* items */

static void
panel_menu_deactivated (GtkWidget *item)
{
    int    x, y, w, h, px, py;
    Panel *panel = PANEL (item->parent->parent);

    _panel_return_if_fail (PANEL_IS_PANEL (panel));

    panel_unblock_autohide (panel);

    gdk_display_get_pointer (gdk_display_get_default (), NULL, &px, &py, NULL);

    gtk_window_get_position (GTK_WINDOW (panel), &x, &y);
    gtk_window_get_size (GTK_WINDOW (panel), &w, &h);

    if (px < x || px > x + w || py < y || py > y + h)
    {
        GdkEvent *ev = gdk_event_new (GDK_LEAVE_NOTIFY);

        ((GdkEventCrossing *) ev)->time = GDK_CURRENT_TIME;
        ((GdkEventCrossing *) ev)->detail = GDK_NOTIFY_NONLINEAR;

        gtk_widget_event (GTK_WIDGET (panel), ev);

        gdk_event_free (ev);
    }
}

static void
panel_menu_opened (GtkWidget *item)
{
    _panel_return_if_fail (PANEL_IS_PANEL (item->parent->parent));

    panel_block_autohide (PANEL (item->parent->parent));
}

static void
_item_expand_changed (GtkWidget *item,
                      gboolean   expand,
                      Panel     *panel)
{
    PanelPrivate *priv;

    _panel_return_if_fail (PANEL_IS_PANEL (panel));

    priv = panel->priv;

    xfce_itembar_set_child_expand (XFCE_ITEMBAR (priv->itembar), item, expand);
}



static void
_item_start_move_end (GtkWidget      *item,
                      GdkDragContext *context,
                      Panel          *panel)
{
    PanelPrivate *priv = panel->priv;
    Panel        *p;

    DBG ("+ finish item drag");

    /* disconnect drag end signal */
    g_signal_handlers_disconnect_by_func (G_OBJECT (item), G_CALLBACK (_item_start_move_end), panel);

    if (!priv->edit_mode)
    {
        const GPtrArray *panels = panel_app_get_panel_list ();
        guint i;

        for (i = 0; i < panels->len; ++i)
        {
            p = g_ptr_array_index (panels, i);
            priv = p->priv;

            xfce_itembar_lower_event_window (XFCE_ITEMBAR (priv->itembar));
            panel_set_items_sensitive (p, TRUE);

            panel_unblock_autohide (p);
        }
    }
}



static void
_item_start_move (GtkWidget *item,
                  Panel     *panel)
{
    const GPtrArray *panels = panel_app_get_panel_list ();
    PanelPrivate    *priv;
    Panel           *p;
    guint             i;

    for (i = 0; i < panels->len; ++i)
    {
        p    = g_ptr_array_index (panels, i);
        priv = p->priv;

        if (!priv->edit_mode)
        {
            panel_set_items_sensitive (p, FALSE);
            xfce_itembar_raise_event_window (XFCE_ITEMBAR (priv->itembar));
            panel_block_autohide (p);
        }
    }

    /* start the drag */
    panel_dnd_begin_drag (item);

    /* signal to make panels sensitive after a drop */
    g_signal_connect (G_OBJECT (item), "drag-end", G_CALLBACK (_item_start_move_end), panel);
}

extern void panel_set_hidden (Panel    *panel,
                              gboolean  hide);

static void
_item_set_panel_hidden (GtkWidget *item,
                        gboolean   hidden,
                        Panel     *panel)
{
    PanelPrivate *priv;

    _panel_return_if_fail (PANEL_IS_PANEL (panel));

    priv = panel->priv;

    if (priv->autohide)
        panel_set_hidden (panel, hidden);
}

static void
panel_insert_widget (Panel     *panel,
                     GtkWidget *item,
                     int        position)
{
    PanelPrivate *priv = panel->priv;

    gtk_widget_show (item);

    if (position == -1)
        xfce_itembar_append (XFCE_ITEMBAR (priv->itembar), item);
    else
        xfce_itembar_insert (XFCE_ITEMBAR (priv->itembar), item, position);

    xfce_itembar_set_child_expand (XFCE_ITEMBAR (priv->itembar), item,
            xfce_panel_item_get_expand (XFCE_PANEL_ITEM (item)));

    if (xfce_itembar_event_window_is_raised (XFCE_ITEMBAR (priv->itembar)))
        xfce_panel_item_set_sensitive (XFCE_PANEL_ITEM (item), FALSE);

    g_signal_connect (item, "destroy", G_CALLBACK (panel_app_queue_save),
                      NULL);
}

static GtkWidget *
panel_create_item (Panel      *panel,
                   const char *name,
                   const char *id)
{
    PanelPrivate *priv;
    GtkWidget    *item = NULL;
    XfceMonitor  *xmon;

    priv = panel->priv;
    xmon = panel_app_get_monitor (priv->monitor);

    item = xfce_panel_item_manager_create_item (xmon->screen,
                                                name,
                                                id,
                                                priv->size,
                                                priv->screen_position);

    if (item != NULL)
    {
        g_signal_connect (item, "menu-deactivated",
                          G_CALLBACK (panel_menu_deactivated), panel);

        g_signal_connect (item, "menu-opened",
                          G_CALLBACK (panel_menu_opened), panel);

        g_signal_connect (item, "expand-changed",
                          G_CALLBACK (_item_expand_changed), panel);

        g_signal_connect (item, "customize-panel",
                          G_CALLBACK (panel_app_customize), NULL);

        g_signal_connect (item, "customize-items",
                          G_CALLBACK (panel_app_customize_items), NULL);

        g_signal_connect (item, "move",
                          G_CALLBACK (_item_start_move), panel);

        g_signal_connect (item, "set-hidden",
                          G_CALLBACK (_item_set_panel_hidden), panel);
    }

    return item;
}

static char *
_panel_get_new_id (void)
{
    static int  counter = 0;
    static char id[30];

    /* unique number: pseudo-random time() + counter */
    g_snprintf (id, sizeof(id), "%ld%d", (glong) time (NULL), counter++);

    return id;
}

GtkWidget *
panel_add_item_with_id (Panel      *panel,
                        const char *name,
                        const char *id)
{
    GtkWidget *item;

    item = panel_create_item (panel, name, id);

    if (item != NULL)
    {
        panel_insert_widget (panel, item, -1);
    }

    return item;
}

GtkWidget *
panel_add_item (Panel      *panel,
                const char *name)
{
    return panel_add_item_with_id (panel, name, _panel_get_new_id ());
}

GtkWidget *
panel_insert_item (Panel      *panel,
                   const char *name,
                   int         position)
{
    GtkWidget *item;

    item = panel_create_item (panel, name, _panel_get_new_id ());

    if (item != NULL)
    {
        panel_insert_widget (panel, item, position);
    }

    return item;
}

/* configuration */

GList *
panel_get_item_list (Panel * panel)
{
    PanelPrivate *priv;

    priv = panel->priv;

    return gtk_container_get_children (GTK_CONTAINER (priv->itembar));
}

void
panel_save_items (Panel *panel)
{
    PanelPrivate *priv;

    _panel_return_if_fail (PANEL_IS_PANEL (panel));

    priv = panel->priv;

    gtk_container_foreach (GTK_CONTAINER (priv->itembar),
                           (GtkCallback)xfce_panel_item_save, NULL);
}

/* convenience */

gboolean
panel_is_horizontal (Panel *panel)
{
    return (GTK_ORIENTATION_HORIZONTAL ==
            xfce_panel_window_get_orientation (XFCE_PANEL_WINDOW (panel)));
}

void
panel_set_items_sensitive (Panel    *panel,
                           gboolean  sensitive)
{
    PanelPrivate *priv = panel->priv;
    GList        *l, *children;

    children = gtk_container_get_children (GTK_CONTAINER (priv->itembar));

    for (l = children; l != NULL; l = l->next)
    {
        xfce_panel_item_set_sensitive (XFCE_PANEL_ITEM (l->data), sensitive);
    }

    g_list_free (children);
}
