/*  $Id$
 *
 *  Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 *  Copyright (c) 2006-2007 Nick Schermer <nick@xfce.org>
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
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-arrow-button.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#include <libxfce4panel/xfce-hvbox.h>

#include "launcher.h"
#include "launcher-exec.h"
#include "launcher-dialog.h"

/**
 * Prototypes
 **/
static void             launcher_g_list_move_first           (GList                 *li);
static gboolean         launcher_theme_changed               (GSignalInvocationHint *ihint,
                                                              guint                  n_param_values,
                                                              const GValue          *param_values,
                                                              LauncherPlugin        *launcher);
static void             launcher_button_drag_data_received   (GtkWidget             *widget,
                                                              GdkDragContext        *context,
                                                              gint                   x,
                                                              gint                   y,
                                                              GtkSelectionData      *selection_data,
                                                              guint                  info,
                                                              guint                  time,
                                                              LauncherPlugin        *launcher);
static void             launcher_menu_drag_data_received     (GtkWidget             *widget,
                                                              GdkDragContext        *context,
                                                              gint                   x,
                                                              gint                   y,
                                                              GtkSelectionData      *selection_data,
                                                              guint                  info,
                                                              guint                  time,
                                                              LauncherEntry         *entry);
static void             launcher_button_clicked              (GtkWidget             *button,
                                                              LauncherPlugin        *launcher);
static void             launcher_button_state_changed        (GtkWidget             *button_a,
                                                              GtkStateType           state,
                                                              GtkWidget             *button_b);
static gboolean         launcher_button_pressed              (LauncherPlugin        *launcher,
                                                              GdkEventButton        *ev);
static gboolean         launcher_button_released             (GtkWidget             *button,
                                                              GdkEventButton        *ev,
                                                              LauncherPlugin        *launcher);
static gboolean         launcher_arrow_pressed               (LauncherPlugin        *launcher,
                                                              GdkEventButton        *ev);
static void             launcher_menu_item_activate          (GtkWidget             *mi,
                                                              GList                 *li);
static gboolean         launcher_menu_item_released          (GtkWidget             *mi,
                                                              GdkEventButton        *ev,
                                                              GList                 *li);
static void             launcher_menu_position               (GtkMenu               *menu,
                                                              gint                  *x,
                                                              gint                  *y,
                                                              gboolean              *push_in,
                                                              GtkWidget             *button);
static gboolean         launcher_menu_popup                  (LauncherPlugin        *launcher);
static void             launcher_menu_deactivated            (LauncherPlugin        *launcher);
static gboolean         launcher_menu_update                 (LauncherPlugin        *launcher);
static LauncherPlugin  *launcher_new                         (XfcePanelPlugin       *plugin) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
static GtkArrowType     launcher_calculate_floating_arrow    (LauncherPlugin        *launcher,
                                                              XfceScreenPosition     position);
static void             launcher_screen_position_changed     (LauncherPlugin        *launcher,
                                                              XfceScreenPosition     position);
static void             launcher_orientation_changed         (LauncherPlugin        *launcher,
                                                              GtkOrientation         orientation);
static gboolean         launcher_set_size                    (LauncherPlugin        *launcher,
                                                              guint                  size);
static void             launcher_free                        (LauncherPlugin        *launcher);
static void             launcher_construct                   (XfcePanelPlugin       *plugin);



/* global setting */
static gboolean move_first = FALSE;



/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (launcher_construct);



/**
 * Utility functions
 **/
void
launcher_g_list_swap (GList *li_a,
                          GList *li_b)
{
    gpointer data;

    /* swap data between the items */
    data = li_a->data;
    li_a->data = li_b->data;
    li_b->data = data;
}



static void
launcher_g_list_move_first (GList *li)
{
    /* quit if we're not going to sort */
    if (move_first == FALSE)
        return;

    /* swap items till we reached the beginning of the list */
    while (li->prev != NULL)
{
        launcher_g_list_swap (li, li->prev);
        li = li->prev;
    }
}



/**
 * Miscelanious function
 **/
static gboolean
launcher_theme_changed (GSignalInvocationHint *ihint,
                        guint                  n_param_values,
                        const GValue          *param_values,
                        LauncherPlugin        *launcher)
{
    /* only update if we already have an image, this fails when the signal is connected */
    if (G_LIKELY (gtk_image_get_storage_type (GTK_IMAGE (launcher->image)) == GTK_IMAGE_EMPTY))
        return TRUE;

    /* update the icon button */
    launcher->icon_update_required = TRUE;
    launcher_button_update (launcher);

    /* destroy the menu */
    launcher_menu_prepare (launcher);

    /* keep hook alive */
    return TRUE;
}



GdkPixbuf *
launcher_load_pixbuf (GtkWidget   *widget,
                      const gchar *icon_name,
                      guint        size,
                      gboolean     fallback)
{
    GdkPixbuf    *icon = NULL;
    GdkPixbuf    *icon_scaled;
    GtkIconTheme *icon_theme;

    if (G_LIKELY (icon_name != NULL) &&
          G_UNLIKELY (g_path_is_absolute (icon_name)))
    {
        /* load the icon from the file */
        icon = exo_gdk_pixbuf_new_from_file_at_max_size (icon_name, size, size, TRUE, NULL);
    }
    else if (G_LIKELY (icon_name != NULL))
    {
        /* determine the appropriate icon theme */
        if (G_LIKELY (gtk_widget_has_screen (widget)))
            icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (widget));
        else
            icon_theme = gtk_icon_theme_get_default ();

        /* try to load the named icon */
        icon = gtk_icon_theme_load_icon (icon_theme, icon_name, size, 0, NULL);
    }

    /* scale down the icon (function above is a convenience function) */
    if (G_LIKELY (icon != NULL))
    {
        /* scale down the icon if required */
        icon_scaled = exo_gdk_pixbuf_scale_down (icon, TRUE, size, size);
        g_object_unref (G_OBJECT (icon));
        icon = icon_scaled;
    }
    else if (fallback) /* create empty pixbuf */
    {
        /* this is a transparent pixbuf, used for tree convenience */
        icon = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, size, size);
        gdk_pixbuf_fill (icon, 0x00000000);
    }

    return icon;
}



void
launcher_set_move_first (gboolean activate)
{
    /* set the value of the move first boolean */
    move_first = activate;
}



/**
 * Drag and Drop
 **/
static void
launcher_button_drag_data_received (GtkWidget        *widget,
                                    GdkDragContext   *context,
                                    gint              x,
                                    gint              y,
                                    GtkSelectionData *selection_data,
                                    guint             info,
                                    guint             time,
                                    LauncherPlugin   *launcher)
{
    /* run the 'entry' dnd with first item from the list */
    launcher_menu_drag_data_received (widget, context, x, y,
                                      selection_data, info, time,
                                      g_list_first (launcher->entries)->data);
}



static void
launcher_menu_drag_data_received (GtkWidget        *widget,
                                  GdkDragContext   *context,
                                  gint              x,
                                  gint              y,
                                  GtkSelectionData *selection_data,
                                  guint             info,
                                  guint             time,
                                  LauncherEntry    *entry)
{
    GSList *file_list = NULL;

    /* create list from all the uri list */
    file_list = launcher_file_list_from_selection (selection_data);

    if (G_LIKELY (file_list != NULL))
    {
        launcher_execute (gtk_widget_get_screen (widget), entry, file_list);

        /* cleanup */
        g_slist_free_all (file_list);
    }

    /* finish drag */
    gtk_drag_finish (context, TRUE, FALSE, time);
}



GSList *
launcher_file_list_from_selection (GtkSelectionData *selection_data)
{
    gchar  **uri_list;
    GSList  *file_list = NULL;
    gchar   *filename;
    guint    i;

    /* check whether the retrieval worked */
    if (G_LIKELY (selection_data->length > 0))
    {
        /* split the received uri list */
        uri_list = g_uri_list_extract_uris ((gchar *) selection_data->data);

        /* walk though the list */
        for (i = 0; uri_list[i] != NULL; i++)
        {
            /* escape the ascii-encoded uri */
            filename = g_filename_from_uri (uri_list[i], NULL, NULL);

            /* append the uri, if uri -> filename conversion failed */
            if (G_UNLIKELY (filename == NULL))
                filename = g_strdup (uri_list[i]);

            /* append the filename */
            file_list = g_slist_append (file_list, filename);
        }

        /* cleanup */
        g_strfreev (uri_list);
    }

    return file_list;
}



/**
 * (Icon) Button Functions
 **/
gboolean
launcher_button_update (LauncherPlugin *launcher)
{
    GdkPixbuf     *icon;
    LauncherEntry *entry;
    guint          size;
    gchar         *tooltip = NULL;

    /* safety check */
    if (G_UNLIKELY (g_list_length (launcher->entries) == 0))
        return FALSE;

    /* get first entry */
    entry = g_list_first (launcher->entries)->data;

    /* check if we really need to update the button icon */
    if (launcher->icon_update_required == FALSE)
        goto updatetooltip;

    /* reset button update */
    launcher->icon_update_required = FALSE;

    /* calculate icon size */
    size = xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (launcher->plugin))
           - 2 - 2 * MAX (launcher->iconbutton->style->xthickness,
                          launcher->iconbutton->style->ythickness);

    /* load the pixbuf, we use the fallback image here because else
     * the theme signal fails when xfce-mcs-manager restarts */
    icon = launcher_load_pixbuf (launcher->iconbutton, entry->icon, size, TRUE);

    /* set the new button icon */
    if (G_LIKELY (icon != NULL))
    {
        gtk_image_set_from_pixbuf (GTK_IMAGE (launcher->image), icon);

        /* release the icon */
        g_object_unref (G_OBJECT (icon));
    }
    else
    {
        /* clear image if no (valid) icon is set */
        gtk_image_clear (GTK_IMAGE (launcher->image));
    }

updatetooltip:

    /* create tooltip text */
    if (G_LIKELY (entry->name))
    {
        if (entry->comment)
            tooltip = g_strdup_printf ("%s\n%s", entry->name, entry->comment);
        else
            tooltip = g_strdup_printf ("%s", entry->name);
    }

    /* set the tooltip */
    gtk_tooltips_set_tip (launcher->tips, launcher->iconbutton,
                          tooltip, NULL);

    /* free string */
    if (G_LIKELY (tooltip))
        g_free (tooltip);

    return FALSE;
}



static void
launcher_button_state_changed (GtkWidget    *button_a,
                               GtkStateType  state,
                               GtkWidget    *button_b)
{
    if (GTK_WIDGET_STATE (button_b) != GTK_WIDGET_STATE (button_a) &&
        GTK_WIDGET_STATE (button_a) != GTK_STATE_INSENSITIVE)
    {
        /* sync the button states */
        gtk_widget_set_state (button_b, GTK_WIDGET_STATE (button_a));
    }
}



static void
launcher_button_clicked (GtkWidget      *button,
                         LauncherPlugin *launcher)
{
    LauncherEntry *entry;

    /* remove popup timeout */
    if (launcher->popup_timeout_id)
    {
        g_source_remove (launcher->popup_timeout_id);
        launcher->popup_timeout_id = 0;
    }

    /* get first entry */
    entry = g_list_first (launcher->entries)->data;

    /* execute command */
    launcher_execute (gtk_widget_get_screen (button), entry, NULL);
}



static gboolean
launcher_button_pressed (LauncherPlugin  *launcher,
                         GdkEventButton  *ev)
{
    guint modifiers;

    modifiers = gtk_accelerator_get_default_mod_mask ();

    /* exit if control is pressed, or pressed != button 1 */
    if (G_LIKELY (ev->button != 1) ||
        (ev->button == 1 && (ev->state & modifiers) == GDK_CONTROL_MASK))
    {
        return FALSE;
    }

    /* start new popup timeout */
    if (launcher->popup_timeout_id == 0)
    {
        launcher->popup_timeout_id =
            g_timeout_add (MENU_POPUP_DELAY,
                           (GSourceFunc) launcher_menu_popup,
                           launcher);
    }

    return FALSE;
}



static gboolean
launcher_button_released (GtkWidget       *button,
                          GdkEventButton  *ev,
                          LauncherPlugin  *launcher)
{
    LauncherEntry *entry;

    /* stop the timeout (from button press) if it's still running */
    if (launcher->popup_timeout_id > 0)
    {
        g_source_remove (launcher->popup_timeout_id);
        launcher->popup_timeout_id = 0;
    }

    /* if button is 2, start command with arg from clipboard */
    if (ev->button == 2)
    {
        entry = g_list_first (launcher->entries)->data;

        launcher_execute_from_clipboard (gtk_widget_get_screen (button), entry);
    }

    return FALSE;
}



/**
 * Arrow button functions
 **/
static gboolean
launcher_arrow_pressed (LauncherPlugin  *launcher,
                        GdkEventButton  *ev)
{
    /* only popup on 1st button */
    if (G_UNLIKELY (ev->button != 1))
        return FALSE;

    /* popup menu */
    launcher_menu_popup (launcher);

    return FALSE;
}



static gboolean
launcher_arrow_drag_motion (GtkToggleButton *button,
                            GdkDragContext  *drag_context,
                            gint             x,
                            gint             y,
                            guint            time,
                            LauncherPlugin  *launcher)
{
    /* we need to popup the menu to allow a drop in a menu item, but
     * since there is no good way to do this (because the pointer is grabbed)
     * we do nothing... */
    return TRUE;
}



/**
 * Menu functions
 **/
static void
launcher_menu_item_activate (GtkWidget     *mi,
                             GList         *li)
{
    /* execute command */
    launcher_execute (gtk_widget_get_screen (mi), li->data, NULL);

    /* reorder list, if needed */
    launcher_g_list_move_first (li);
}



static gboolean
launcher_menu_item_released (GtkWidget      *mi,
                             GdkEventButton *ev,
                             GList          *li)
{
    if (ev->button == 2)
    {
        /* execute command with arg from clipboard */
        launcher_execute_from_clipboard (gtk_widget_get_screen (mi), li->data);

        /* reorder list, if needed */
        launcher_g_list_move_first (li);

        return TRUE;
    }

    return FALSE;
}



static void
launcher_menu_position (GtkMenu   *menu,
                        gint      *x,
                        gint      *y,
                        gboolean  *push_in,
                        GtkWidget *button)
{
    GtkWidget      *widget;
    GtkRequisition  req;
    GdkScreen      *screen;
    GdkRectangle    geom;
    gint            num;

    if (!GTK_WIDGET_REALIZED (GTK_WIDGET (menu)))
        gtk_widget_realize (GTK_WIDGET (menu));

    gtk_widget_size_request (GTK_WIDGET (menu), &req);

    widget = button->parent->parent;
    gdk_window_get_origin (widget->window, x, y);

    widget = button->parent;

    switch (xfce_arrow_button_get_arrow_type (XFCE_ARROW_BUTTON (button)))
    {
        case GTK_ARROW_UP:
            *x += widget->allocation.x;
            *y += widget->allocation.y - req.height;
            break;

        case GTK_ARROW_DOWN:
            *x += widget->allocation.x;
            *y += widget->allocation.y + widget->allocation.height;
            break;

        case GTK_ARROW_LEFT:
            *x += widget->allocation.x - req.width;
            *y += widget->allocation.y - req.height
                  + widget->allocation.height;
            break;

        default: /* GTK_ARROW_RIGHT and GTK_ARROW_NONE */
            *x += widget->allocation.x + widget->allocation.width;
            *y += widget->allocation.y - req.height
                  + widget->allocation.height;
            break;
    }

    screen = gtk_widget_get_screen (widget);

    num = gdk_screen_get_monitor_at_window (screen, widget->window);

    gdk_screen_get_monitor_geometry (screen, num, &geom);

    if (*x > geom.x + geom.width - req.width)
        *x = geom.x + geom.width - req.width;
    if (*x < geom.x)
        *x = geom.x;

    if (*y > geom.y + geom.height - req.height)
        *y = geom.y + geom.height - req.height;
    if (*y < geom.y)
        *y = geom.y;
}



static gboolean
launcher_menu_popup (LauncherPlugin *launcher)
{
    /* stop timeout if needed */
    if (launcher->popup_timeout_id > 0)
    {
        g_source_remove (launcher->popup_timeout_id);
        launcher->popup_timeout_id = 0;
    }

    /* check if menu is needed, or it needs an update */
    if (g_list_length (launcher->entries) <= 1)
        return FALSE;
    else if (launcher->menu == NULL)
        launcher_menu_update (launcher);

    /* toggle the arrow button */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (launcher->arrowbutton), TRUE);

    /* popup menu */
    gtk_menu_popup (GTK_MENU (launcher->menu), NULL, NULL,
                    (GtkMenuPositionFunc) launcher_menu_position,
                    launcher->arrowbutton, 0,
                    gtk_get_current_event_time ());

    return TRUE;
}



static void
launcher_menu_deactivated (LauncherPlugin *launcher)
{
    /* deactivate arrow button */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (launcher->arrowbutton), FALSE);

    /* rebuild menu and button */
    if (move_first)
    {
        launcher->icon_update_required = TRUE;
        g_idle_add ((GSourceFunc) launcher_button_update, launcher);

        g_idle_add ((GSourceFunc) launcher_menu_prepare, launcher);
    }
}



gboolean
launcher_menu_prepare (LauncherPlugin *launcher)
{
    if (launcher->menu != NULL)
    {
        g_signal_emit_by_name (G_OBJECT (launcher->menu), "deactivate");
        gtk_widget_destroy (launcher->menu);
        launcher->menu = NULL;
    }

    /* show or hide the arrow button */
    if (g_list_length (launcher->entries) <= 1)
        gtk_widget_hide (launcher->arrowbutton);
    else
        gtk_widget_show (launcher->arrowbutton);

    return FALSE;
}



static gboolean
launcher_menu_update (LauncherPlugin *launcher)
{
    GList         *li;
    GtkWidget     *mi, *image;
    GdkPixbuf     *icon;
    LauncherEntry *entry;

    /* destroy the old menu, if needed */
    if (G_UNLIKELY (launcher->menu != NULL))
        launcher_menu_prepare (launcher);

    /* create new menu */
    launcher->menu = gtk_menu_new ();

    /* make sure the menu popups up in right screen */
    gtk_menu_set_screen (GTK_MENU (launcher->menu),
                         gtk_widget_get_screen (GTK_WIDGET (launcher->plugin)));

    /* append all entries, except first one */
    for (li = g_list_nth (launcher->entries, 1); li != NULL; li = li->next)
    {
        entry = li->data;

        /* create new menu item */
        mi = gtk_image_menu_item_new_with_label (entry->name ? entry->name : _("New Item"));
        gtk_widget_show (mi);
        gtk_menu_shell_prepend (GTK_MENU_SHELL (launcher->menu), mi);

        /* load and set menu icon */
        icon = launcher_load_pixbuf (launcher->iconbutton, entry->icon, MENU_ICON_SIZE, FALSE);

        if (G_LIKELY (icon != NULL))
        {
            image = gtk_image_new_from_pixbuf (icon);
            gtk_widget_show (image);
            gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);

            /* release pixbuf */
            g_object_unref (G_OBJECT (icon));
        }

        /* connect signals */
        g_signal_connect (G_OBJECT (mi), "activate",
                          G_CALLBACK (launcher_menu_item_activate), li);

        g_signal_connect (G_OBJECT (mi), "button-release-event",
                          G_CALLBACK (launcher_menu_item_released), li);

        /* dnd support */
        gtk_drag_dest_set (mi, GTK_DEST_DEFAULT_ALL,
                           drop_targets, G_N_ELEMENTS (drop_targets),
                           GDK_ACTION_COPY);

        g_signal_connect (G_OBJECT (mi), "drag-data-received",
                          G_CALLBACK (launcher_menu_drag_data_received), entry);

        /* set tooltip */
        if (G_LIKELY (entry->comment))
            gtk_tooltips_set_tip (launcher->tips, mi, entry->comment, NULL);
    }

    /* connect deactivate signal */
    g_signal_connect_swapped (G_OBJECT (launcher->menu), "deactivate",
                                G_CALLBACK (launcher_menu_deactivated), launcher);

    return FALSE;
}



/**
 * Save and load settings
 **/
static gchar *
launcher_read_entry (XfceRc      *rc,
                         const gchar *name)
{
    const gchar *temp;
    gchar       *value = NULL;

    temp = xfce_rc_read_entry (rc, name, NULL);
    if (G_LIKELY (temp != NULL && *temp != '\0'))
        value = g_strdup (temp);

    return value;
}



void
launcher_read (LauncherPlugin *launcher)
{
    gchar         *file;
    gchar          group[10];
    XfceRc        *rc;
    guint          i;
    LauncherEntry *entry;

    /* get rc file name, create it if needed */
    file = xfce_panel_plugin_save_location (launcher->plugin, TRUE);

    if (G_UNLIKELY (file == NULL))
        return;

    /* open rc, read-only */
    rc = xfce_rc_simple_open (file, TRUE);
    g_free (file);

    if (G_UNLIKELY (rc == NULL))
        return;

    /* read global settings */
    xfce_rc_set_group (rc, "Global");

    launcher->move_first = xfce_rc_read_bool_entry (rc, "MoveFirst", FALSE);

    for (i = 0; i < 100 /* arbitrary */; ++i)
    {
        /* create group name */
        g_snprintf (group, sizeof (group), "Entry %d", i);

        /* break if no more entries found */
        if (xfce_rc_has_group (rc, group) == FALSE)
            break;

        /* set the group */
        xfce_rc_set_group (rc, group);

        /* create new entry structure */
        entry = panel_slice_new0 (LauncherEntry);

        /* read all the entry settings */
        entry->name = launcher_read_entry (rc, "Name");
        entry->comment = launcher_read_entry (rc, "Comment");
        entry->icon = launcher_read_entry (rc, "Icon");
        entry->exec = launcher_read_entry (rc, "Exec");
        entry->path = launcher_read_entry (rc, "Path");

        entry->terminal = xfce_rc_read_bool_entry (rc, "Terminal", FALSE);
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
        entry->startup = xfce_rc_read_bool_entry (rc, "StartupNotify", FALSE);
#endif
        /* append the entry */
        launcher->entries = g_list_append (launcher->entries, entry);
    }

    /* close the rc file */
    xfce_rc_close (rc);
}



void
launcher_write (LauncherPlugin *launcher)
{
    gchar          *file;
    gchar         **groups;
    gchar           group[10];
    XfceRc         *rc;
    GList          *li;
    guint           i = 0, n;
    guint           block;
    LauncherEntry  *entry;

    /* HACK:
     * sometimes the panel emits a save signal, this is cool, unless we're working
     * in the properties dialog. This because the cancel button won't work then. */
    block = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (launcher->plugin),
                                                "xfce-panel-plugin-block"));
    if (G_UNLIKELY (block > 0))
        return;

    /* get rc file name, create it if needed */
    file = xfce_panel_plugin_save_location (launcher->plugin, TRUE);

    if (G_UNLIKELY (file == NULL))
        return;

    /* open rc, read/write */
    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (G_UNLIKELY (rc == NULL))
        return;

    /* retreive all the groups in the config file */
    groups = xfce_rc_get_groups (rc);

    /* remove all the groups
     * note: you can use g_unlink to remove the file, but first you need some
     * stdio library and some benchmarking showed this is almost 2x faster for
     * 'normal' situations */
    for (n = 0; groups[n] != NULL; ++n)
        xfce_rc_delete_group (rc, groups[n], TRUE);

    /* cleanup */
    g_strfreev (groups);

    /* save global launcher settings */
    xfce_rc_set_group (rc, "Global");

    xfce_rc_write_bool_entry (rc, "MoveFirst", launcher->move_first);

    /* save all the entries */
    for (li = launcher->entries; li != NULL; li = li->next, ++i)
    {
        entry = li->data;

        /* set entry group */
        g_snprintf (group, sizeof (group), "Entry %d", i);
        xfce_rc_set_group (rc, group);

        /* write entry settings */
        if (G_LIKELY (entry->name))
            xfce_rc_write_entry (rc, "Name", entry->name);

        if (G_LIKELY (entry->comment))
            xfce_rc_write_entry (rc, "Comment", entry->comment);

        if (G_LIKELY (entry->icon))
            xfce_rc_write_entry (rc, "Icon", entry->icon);

        if (G_LIKELY (entry->exec))
            xfce_rc_write_entry (rc, "Exec", entry->exec);

        if (G_LIKELY (entry->path))
            xfce_rc_write_entry (rc, "Path", entry->path);

        xfce_rc_write_bool_entry (rc, "Terminal", entry->terminal);
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
        xfce_rc_write_bool_entry (rc, "StartupNotify", entry->startup);
#endif
    }

    /* close the rc file */
    xfce_rc_close (rc);
}



LauncherEntry *
launcher_new_entry (void)
{
    LauncherEntry *entry;

    /* create new entry slice */
    entry = panel_slice_new0 (LauncherEntry);

    /* set some default values */
    entry->name    = g_strdup (_("New Item"));
    entry->comment = NULL; /* _("This item has not yet been configured")); */
    entry->icon    = g_strdup ("applications-other");

    /* fill others */
    entry->exec     = NULL;
    entry->path     = NULL;
    entry->terminal = FALSE;
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    entry->startup  = FALSE;
#endif

    return entry;
}



/**
 * Plugin create function and signal handling
 **/
static LauncherPlugin*
launcher_new (XfcePanelPlugin *plugin)
{
    LauncherPlugin     *launcher;
    LauncherEntry      *entry;
    GtkWidget          *box;
    XfceScreenPosition  position;
    gpointer            klass;

    /* get panel information */
    position = xfce_panel_plugin_get_screen_position (plugin);

    launcher = panel_slice_new0 (LauncherPlugin);

    /* link plugin */
    launcher->plugin = plugin;

    /* defaults */
    launcher->menu = NULL;

    /* create tooltips */
    launcher->tips = gtk_tooltips_new ();
    exo_gtk_object_ref_sink (GTK_OBJECT (launcher->tips));

    /* create widgets */
    box = xfce_hvbox_new (GTK_ORIENTATION_HORIZONTAL, FALSE, 0);
    gtk_container_add (GTK_CONTAINER (plugin), box);
    gtk_widget_show (box);

    launcher->iconbutton = xfce_create_panel_button ();
    gtk_widget_show (launcher->iconbutton);
    gtk_box_pack_start (GTK_BOX (box), launcher->iconbutton, TRUE, TRUE, 0);

    gtk_drag_dest_set (launcher->iconbutton, GTK_DEST_DEFAULT_ALL,
                       drop_targets, G_N_ELEMENTS (drop_targets),
                       GDK_ACTION_COPY);

    launcher->image = gtk_image_new ();
    gtk_widget_show (launcher->image);
    gtk_container_add (GTK_CONTAINER (launcher->iconbutton), launcher->image);

    launcher->arrowbutton = xfce_arrow_button_new (GTK_ARROW_UP);
    GTK_WIDGET_UNSET_FLAGS (launcher->arrowbutton, GTK_CAN_DEFAULT|GTK_CAN_FOCUS);
    gtk_box_pack_start (GTK_BOX (box), launcher->arrowbutton, FALSE, FALSE, 0);
    gtk_widget_set_size_request (launcher->arrowbutton, ARROW_WIDTH, ARROW_WIDTH);
    gtk_button_set_relief (GTK_BUTTON (launcher->arrowbutton), GTK_RELIEF_NONE);
    gtk_button_set_focus_on_click (GTK_BUTTON (launcher->arrowbutton), FALSE);

    gtk_drag_dest_set (launcher->arrowbutton, GTK_DEST_DEFAULT_ALL,
                       drop_targets, G_N_ELEMENTS (drop_targets),
                       GDK_ACTION_COPY);

    /* signals for button state sync */
    g_signal_connect (G_OBJECT (launcher->iconbutton), "state-changed",
                      G_CALLBACK (launcher_button_state_changed), launcher->arrowbutton);
    g_signal_connect (G_OBJECT (launcher->arrowbutton), "state-changed",
                      G_CALLBACK (launcher_button_state_changed), launcher->iconbutton);

    /* hook for icon themes changes */
    klass = g_type_class_ref (GTK_TYPE_ICON_THEME);
    launcher->theme_timeout_id = g_signal_add_emission_hook (g_signal_lookup ("changed", GTK_TYPE_ICON_THEME),
                                                             0, (GSignalEmissionHook) launcher_theme_changed,
                                                             launcher, NULL);
    g_type_class_unref (klass);

    /* icon button signals */
    g_signal_connect (G_OBJECT (launcher->iconbutton), "clicked",
                      G_CALLBACK (launcher_button_clicked), launcher);

    g_signal_connect_swapped (G_OBJECT (launcher->iconbutton), "button-press-event",
                              G_CALLBACK (launcher_button_pressed), launcher);

    g_signal_connect (G_OBJECT (launcher->iconbutton), "button-release-event",
                      G_CALLBACK (launcher_button_released), launcher);

    g_signal_connect (G_OBJECT (launcher->iconbutton), "drag-data-received",
                      G_CALLBACK (launcher_button_drag_data_received), launcher);

    /* this signal with update the button (show icon) when the button is realizde */
    launcher->icon_update_required = TRUE;
    g_signal_connect_swapped (G_OBJECT (launcher->iconbutton), "realize",
                              G_CALLBACK (launcher_button_update), launcher);

    /* arrow button signals */
    g_signal_connect_swapped (G_OBJECT (launcher->arrowbutton), "button-press-event",
                              G_CALLBACK (launcher_arrow_pressed), launcher);

    g_signal_connect (G_OBJECT (launcher->arrowbutton), "drag-motion",
                      G_CALLBACK (launcher_arrow_drag_motion), launcher);

    /* read the user settings */
    launcher_read (launcher);

    /* set move_first boolean */
    launcher_set_move_first (launcher->move_first);

    /* append new entry if no entries loaded */
    if (G_UNLIKELY (g_list_length (launcher->entries) == 0))
    {
          entry = launcher_new_entry ();

          launcher->entries = g_list_append (launcher->entries, entry);
    }

    /* set arrow position */
    launcher_screen_position_changed (launcher, position);

    /* prepare the menu */
    launcher_menu_prepare (launcher);

    return launcher;
}



static GtkArrowType
launcher_calculate_floating_arrow (LauncherPlugin     *launcher,
                                   XfceScreenPosition  position)
{
    gint          mon, x, y;
    GdkScreen    *screen;
    GdkRectangle  geom;

    if (!GTK_WIDGET_REALIZED (launcher->iconbutton))
    {
        if (xfce_screen_position_is_horizontal (position))
            return GTK_ARROW_UP;
        else
            return GTK_ARROW_LEFT;
    }

    screen = gtk_widget_get_screen (launcher->iconbutton);
    mon = gdk_screen_get_monitor_at_window (screen, launcher->iconbutton->window);
    gdk_screen_get_monitor_geometry (screen, mon, &geom);
    gdk_window_get_root_origin (launcher->iconbutton->window, &x, &y);

    if (xfce_screen_position_is_horizontal (position))
    {
        if (y > geom.y + geom.height / 2)
            return GTK_ARROW_UP;
        else
            return GTK_ARROW_DOWN;
    }
    else
    {
        if (x > geom.x + geom.width / 2)
            return GTK_ARROW_LEFT;
        else
            return GTK_ARROW_RIGHT;
    }
}



static void
launcher_screen_position_changed (LauncherPlugin     *launcher,
                                  XfceScreenPosition  position)
{
    GtkArrowType type;

    if (xfce_screen_position_is_bottom (position))
        type = GTK_ARROW_UP;
    else if (xfce_screen_position_is_top (position))
        type = GTK_ARROW_DOWN;
    else if (xfce_screen_position_is_left (position))
        type = GTK_ARROW_RIGHT;
    else if (xfce_screen_position_is_right (position))
        type = GTK_ARROW_LEFT;
    else /* floating */
        type = launcher_calculate_floating_arrow (launcher, position);

    /* set the arrow direction */
    xfce_arrow_button_set_arrow_type (XFCE_ARROW_BUTTON (launcher->arrowbutton), type);
}



static void
launcher_orientation_changed (LauncherPlugin  *launcher,
                              GtkOrientation   orientation)
{
    /* noting yet */
}



static gboolean
launcher_set_size (LauncherPlugin  *launcher,
                       guint            size)
{
    /* set fixed button size */
    gtk_widget_set_size_request (launcher->iconbutton, size, size);

    /* reload the icon button */
    if (GTK_WIDGET_REALIZED (launcher->iconbutton))
    {
        launcher->icon_update_required = TRUE;
        launcher_button_update (launcher);
    }

    return TRUE;
}



void
launcher_free_entry (LauncherEntry  *entry,
                     LauncherPlugin *launcher)
{
    /* remove from the list */
    if (G_LIKELY (launcher != NULL))
        launcher->entries = g_list_remove (launcher->entries, entry);

    /* free variables */
    g_free (entry->name);
    g_free (entry->comment);
    g_free (entry->path);
    g_free (entry->icon);
    g_free (entry->exec);

    /* free structure */
    panel_slice_free (LauncherEntry, entry);
}



static void
launcher_free (LauncherPlugin *launcher)
{
    GtkWidget *dialog;

    /* check if we still need to destroy the dialog */
    dialog = g_object_get_data (G_OBJECT (launcher->plugin), "dialog");
    if (G_UNLIKELY (dialog != NULL))
        gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

    /* stop timeout */
    if (G_UNLIKELY (launcher->popup_timeout_id))
        g_source_remove (launcher->popup_timeout_id);

    /* remove icon theme change */
    g_signal_remove_emission_hook (g_signal_lookup ("changed", GTK_TYPE_ICON_THEME),
                                   launcher->theme_timeout_id);

    /* destroy the popup menu */
    if (launcher->menu)
        gtk_widget_destroy (launcher->menu);

    /* remove the entries */
    g_list_foreach (launcher->entries,
                    (GFunc) launcher_free_entry, launcher);
    g_list_free (launcher->entries);

    /* release the tooltips */
    g_object_unref (G_OBJECT (launcher->tips));

    /* free launcher structure */
    panel_slice_free (LauncherPlugin, launcher);
}



static void
launcher_construct (XfcePanelPlugin *plugin)
{
    LauncherPlugin *launcher = launcher_new (plugin);

    xfce_panel_plugin_add_action_widget (plugin, launcher->iconbutton);
    xfce_panel_plugin_add_action_widget (plugin, launcher->arrowbutton);
    xfce_panel_plugin_menu_show_configure (plugin);

    /* connect signals */
    g_signal_connect_swapped (G_OBJECT (plugin), "screen-position-changed",
                              G_CALLBACK (launcher_screen_position_changed), launcher);

    g_signal_connect_swapped (G_OBJECT (plugin), "orientation-changed",
                              G_CALLBACK (launcher_orientation_changed), launcher);

    g_signal_connect_swapped (G_OBJECT (plugin), "size-changed",
                              G_CALLBACK (launcher_set_size), launcher);

    g_signal_connect_swapped (G_OBJECT (plugin), "free-data",
                      G_CALLBACK (launcher_free), launcher);

    g_signal_connect_swapped (G_OBJECT (plugin), "save",
                              G_CALLBACK (launcher_write), launcher);

    g_signal_connect_swapped (G_OBJECT (plugin), "configure-plugin",
                              G_CALLBACK (launcher_dialog_show), launcher);
}
