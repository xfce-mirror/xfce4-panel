/*  $Id: launcher.c 26118 2007-09-30 18:14:31Z jasper $
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
#include <libxfce4panel/libxfce4panel.h>

#include "launcher.h"
#include "launcher-exec.h"
#include "launcher-dialog.h"

/* prototypes */
static gboolean        launcher_utility_icon_theme_changed          (GSignalInvocationHint *ihint,
                                                                     guint                  n_param_values,
                                                                     const GValue          *param_values,
                                                                     LauncherPlugin        *launcher);
static gboolean        launcher_icon_button_expose_event            (GtkWidget             *widget,
                                                                     GdkEventExpose        *event,
                                                                     LauncherPlugin        *launcher);
static void            launcher_icon_button_set_icon                (LauncherPlugin        *launcher);
#if LAUNCHER_NEW_TOOLTIP_API
static gboolean        launcher_icon_button_query_tooltip           (GtkWidget             *widget,
                                                                     gint                   x,
                                                                     gint                   y,
                                                                     gboolean               keyboard_mode,
                                                                     GtkTooltip            *tooltip,
                                                                     LauncherPlugin        *launcher);
#else
static void            launcher_icon_button_set_tooltip             (LauncherPlugin        *launcher);
#endif
static gboolean        launcher_icon_button_pressed                 (GtkWidget             *button,
                                                                     GdkEventButton        *event,
                                                                     LauncherPlugin        *launcher);
static gboolean        launcher_icon_button_released                (GtkWidget             *button,
                                                                     GdkEventButton        *event,
                                                                     LauncherPlugin        *launcher);
static void            launcher_icon_button_drag_data_received      (GtkWidget             *widget,
                                                                     GdkDragContext        *context,
                                                                     gint                   x,
                                                                     gint                   y,
                                                                     GtkSelectionData      *selection_data,
                                                                     guint                  info,
                                                                     guint                  time,
                                                                     LauncherPlugin        *launcher);
static gboolean        launcher_arrow_button_pressed                (GtkWidget             *button,
                                                                     GdkEventButton        *event,
                                                                     LauncherPlugin        *launcher);
static void            launcher_button_state_changed                (GtkWidget             *button_a,
                                                                     GtkStateType           state,
                                                                     GtkWidget             *button_b);
static gboolean        launcher_menu_item_released                  (GtkWidget             *mi,
                                                                     GdkEventButton        *event,
                                                                     LauncherPlugin        *launcher);
static void            launcher_menu_popup_destroyed                (gpointer               user_data);
static gboolean        launcher_menu_popup                          (gpointer               user_data);
static void            launcher_menu_deactivated                    (LauncherPlugin        *launcher);
static void            launcher_menu_destroy                        (LauncherPlugin        *launcher);
static void            launcher_menu_rebuild                        (LauncherPlugin        *launcher);
static LauncherPlugin *launcher_plugin_new                          (XfcePanelPlugin       *plugin) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
static void            launcher_plugin_pack_buttons                 (LauncherPlugin        *launcher);
static gchar          *launcher_plugin_read_entry                   (XfceRc                *rc,
                                                                     const gchar           *name) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
static void            launcher_plugin_screen_position_changed      (LauncherPlugin        *launcher);
static void            launcher_plugin_orientation_changed          (LauncherPlugin        *launcher);
static gboolean        launcher_plugin_set_size                     (LauncherPlugin        *launcher,
                                                                     guint                  size);
static void            launcher_plugin_free                         (LauncherPlugin        *launcher);
static void            launcher_plugin_construct                    (XfcePanelPlugin       *plugin);



/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER (launcher_plugin_construct);



/**
 * Utility Functions
 **/
static gboolean
launcher_utility_icon_theme_changed (GSignalInvocationHint *ihint,
                                     guint                  n_param_values,
                                     const GValue          *param_values,
                                     LauncherPlugin        *launcher)
{
    /* only update if we already have an image, this fails when the signal is connected */
    if (G_LIKELY (gtk_image_get_storage_type (GTK_IMAGE (launcher->image)) == GTK_IMAGE_EMPTY))
        return TRUE;

    /* update the button icon */
    launcher_icon_button_set_icon (launcher);

    /* destroy the menu */
    launcher_menu_destroy (launcher);

    /* keep hook alive */
    return TRUE;
}



GSList *
launcher_utility_filenames_from_selection_data (GtkSelectionData *selection_data)
{
    gchar  **uri_list;
    GSList  *filenames = NULL;
    gchar   *filename;
    guint    i;

    /* check whether the retrieval worked */
    if (G_LIKELY (selection_data->length > 0))
    {
        /* split the received uri list */
        uri_list = g_uri_list_extract_uris ((gchar *) selection_data->data);

        if (G_LIKELY (uri_list))
        {
            /* walk though the list */
            for (i = 0; uri_list[i] != NULL; i++)
            {
                /* convert the uri to a filename */
                filename = g_filename_from_uri (uri_list[i], NULL, NULL);

                /* prepend the filename */
                if (G_LIKELY (filename))
                    filenames = g_slist_prepend (filenames, filename);
            }

            /* cleanup */
            g_strfreev (uri_list);

            /* reverse the list */
            filenames = g_slist_reverse (filenames);
        }
    }

    return filenames;
}



GdkPixbuf *
launcher_utility_load_pixbuf (GdkScreen   *screen,
                              const gchar *name,
                              guint        size)
{
    GdkPixbuf    *pixbuf = NULL;
    GdkPixbuf    *scaled;
    GtkIconTheme *theme;

    if (G_LIKELY (name))
    {
        if (g_path_is_absolute (name))
        {
            /* load the icon from the file */
            pixbuf = exo_gdk_pixbuf_new_from_file_at_max_size (name, size, size, TRUE, NULL);
        }
        else
        {
            /* determine the appropriate icon theme */
            if (G_LIKELY (screen))
                theme = gtk_icon_theme_get_for_screen (screen);
            else
                theme = gtk_icon_theme_get_default ();

            /* try to load the named icon */
            pixbuf = gtk_icon_theme_load_icon (theme, name, size, 0, NULL);

            if (G_LIKELY (pixbuf))
            {
                /* scale down the icon if required */
                scaled = exo_gdk_pixbuf_scale_down (pixbuf, TRUE, size, size);
                g_object_unref (G_OBJECT (pixbuf));
                pixbuf = scaled;
            }
        }
    }

    return pixbuf;
}



#if LAUNCHER_NEW_TOOLTIP_API
static gboolean
launcher_utility_query_tooltip (GtkWidget     *widget,
                                gint           x,
                                gint           y,
                                gboolean       keyboard_mode,
                                GtkTooltip    *tooltip,
                                LauncherEntry *entry)
{
    gchar     *string;
    GdkPixbuf *pixbuf;

    /* create tooltip text */
    if (G_LIKELY (entry && entry->name))
    {
        if (entry->comment)
            string = g_strdup_printf ("<b>%s</b>\n%s", entry->name, entry->comment);
        else
            string = g_strdup_printf ("%s", entry->name);

        /* set the markup tooltip */
        gtk_tooltip_set_markup (tooltip, string);

        /* cleanup */
        g_free (string);

        if (G_LIKELY (entry->icon))
        {
            /* try to load an pixbuf */
            pixbuf = launcher_utility_load_pixbuf (gtk_widget_get_screen (widget), entry->icon,
                                                   LAUNCHER_TOOLTIP_SIZE);

            if (G_LIKELY (pixbuf))
            {
                /* set the tooltip icon */
                gtk_tooltip_set_icon (tooltip, pixbuf);

                /* releases */
                g_object_unref (G_OBJECT (pixbuf));
            }
        }

        /* show the tooltip */
        return TRUE;
    }

    /* nothing to show */
    return FALSE;
}
#endif



/**
 * Icon Button Functions
 **/
static gboolean
launcher_icon_button_expose_event (GtkWidget      *widget,
                                   GdkEventExpose *event,
                                   LauncherPlugin *launcher)
{
    gint         x, y, w;
    GtkArrowType arrow_type;

    /* only paint the arrow when the arrow button is hidden */
    if (launcher->arrow_position == LAUNCHER_ARROW_INSIDE_BUTTON)
    {
        /* calculate the width of the arrow */
        w = widget->allocation.width / 3;

        /* get the arrow type */
        arrow_type = xfce_arrow_button_get_arrow_type (XFCE_ARROW_BUTTON (launcher->arrow_button));

        /* start coordinates */
        x = widget->allocation.x;
        y = widget->allocation.y;

        /* calculate the position based on the arrow type */
        switch (arrow_type)
        {
            case GTK_ARROW_UP:
                /* north east */
                x += (widget->allocation.width - w);
                break;

            case GTK_ARROW_DOWN:
                /* south west */
                y += (widget->allocation.height - w);
                break;

            case GTK_ARROW_RIGHT:
                /* south east */
                x += (widget->allocation.width - w);
                y += (widget->allocation.height - w);
                break;

            default:
                /* north west */
                break;
        }

        /* paint the arrow */
        gtk_paint_arrow (widget->style, widget->window,
                         GTK_WIDGET_STATE (widget), GTK_SHADOW_IN,
                         &(event->area), widget, "launcher_button",
                         arrow_type, TRUE, x, y, w, w);
    }

    return FALSE;
}



static void
launcher_icon_button_set_icon (LauncherPlugin *launcher)
{
    GdkPixbuf     *pixbuf;
    LauncherEntry *entry;
    GdkScreen     *screen;

    /* get the first entry in the list */
    entry = g_list_first (launcher->entries)->data;

    /* get widget screen */
    screen = gtk_widget_get_screen (launcher->image);

    /* try to load the file */
    pixbuf = launcher_utility_load_pixbuf (screen, entry->icon, launcher->image_size);

    if (G_LIKELY (pixbuf))
    {
        /* set the image and release the pixbuf */
        gtk_image_set_from_pixbuf (GTK_IMAGE (launcher->image), pixbuf);
        g_object_unref (G_OBJECT (pixbuf));
    }
    else
    {
        /* clear the image */
        gtk_image_clear (GTK_IMAGE (launcher->image));
    }
}



#if LAUNCHER_NEW_TOOLTIP_API
static gboolean
launcher_icon_button_query_tooltip (GtkWidget      *widget,
                                    gint            x,
                                    gint            y,
                                    gboolean        keyboard_mode,
                                    GtkTooltip     *tooltip,
                                    LauncherPlugin *launcher)
{
    /* don't show tooltips on a menu button */
    if (launcher->arrow_position == LAUNCHER_ARROW_INSIDE_BUTTON)
       return FALSE;

    return launcher_utility_query_tooltip (widget, x, y, keyboard_mode, tooltip,
                                           g_list_first (launcher->entries)->data);
}



#else
static void
launcher_icon_button_set_tooltip (LauncherPlugin *launcher)
{
    LauncherEntry *entry;
    gchar         *string = NULL;

    /* get first entry */
    entry = g_list_first (launcher->entries)->data;

    /* create tooltip text */
    if (G_LIKELY (entry->name && launcher->arrow_position != LAUNCHER_ARROW_INSIDE_BUTTON))
    {
        if (entry->comment)
            string = g_strdup_printf ("%s\n%s", entry->name, entry->comment);
        else
            string = g_strdup_printf ("%s", entry->name);
    }

    /* set the tooltip */
    gtk_tooltips_set_tip (launcher->tips, launcher->icon_button, string, NULL);

    /* cleanup */
    g_free (string);
}
#endif



static gboolean
launcher_icon_button_pressed (GtkWidget      *button,
                              GdkEventButton *event,
                              LauncherPlugin *launcher)
{
    guint modifiers;

    /* get the default accelerator modifier mask */
    modifiers = event->state & gtk_accelerator_get_default_mod_mask ();

    /* exit if another button then 1 is pressed or control is hold */
    if (event->button != 1 || modifiers == GDK_CONTROL_MASK)
        return FALSE;

    /* popup the menu or start the popup timeout */
    if (launcher->arrow_position == LAUNCHER_ARROW_INSIDE_BUTTON)
    {
        launcher_menu_popup (launcher);
    }
    else if (launcher->popup_timeout_id == 0 && g_list_length (launcher->entries) > 1)
    {
        launcher->popup_timeout_id =
            g_timeout_add_full (G_PRIORITY_DEFAULT, LAUNCHER_POPUP_DELAY, launcher_menu_popup,
                                launcher, launcher_menu_popup_destroyed);
    }

    return FALSE;
}



static gboolean
launcher_icon_button_released (GtkWidget      *button,
                               GdkEventButton *event,
                               LauncherPlugin *launcher)
{
    LauncherEntry *entry;
    GdkScreen     *screen;

    /* remove the timeout */
    if (G_LIKELY (launcher->popup_timeout_id > 0))
        g_source_remove (launcher->popup_timeout_id);

    /* only accept click in the button and don't respond on multiple clicks */
    if (GTK_BUTTON (button)->in_button && launcher->arrow_position != LAUNCHER_ARROW_INSIDE_BUTTON)
    {
        /* get the first launcher entry */
        entry = g_list_first (launcher->entries)->data;

        /* get the widget screen */
        screen = gtk_widget_get_screen (button);

        /* execute the command on button 1 and 2 */
        if (event->button == 1)
            launcher_execute (screen, entry, NULL);
        else if (event->button == 2)
            launcher_execute_from_clipboard (screen, entry);
    }

    return FALSE;
}



static void
launcher_icon_button_drag_data_received (GtkWidget        *widget,
                                         GdkDragContext   *context,
                                         gint              x,
                                         gint              y,
                                         GtkSelectionData *selection_data,
                                         guint             info,
                                         guint             time,
                                         LauncherPlugin   *launcher)
{
    GSList        *filenames;
    LauncherEntry *entry;

    /* execute */
    if (launcher->arrow_position != LAUNCHER_ARROW_INSIDE_BUTTON)
    {
        /* create filenames list from all the uris */
        filenames = launcher_utility_filenames_from_selection_data (selection_data);

        if (G_LIKELY (filenames))
        {
            /* get entry */
            entry = g_list_first (launcher->entries)->data;

            /* execute the entry with the filenames */
            launcher_execute (gtk_widget_get_screen (widget), entry, filenames);

            /* cleanup */
            launcher_free_filenames (filenames);
        }
    }

    /* finish drag */
    gtk_drag_finish (context, TRUE, FALSE, time);
}



/**
 * Arrow Button Functions
 **/
static gboolean
launcher_arrow_button_pressed (GtkWidget      *button,
                               GdkEventButton *event,
                               LauncherPlugin *launcher)
{
    /* only popup on 1st button */
    if (event->button == 1)
        launcher_menu_popup (launcher);

    return FALSE;
}



/**
 * Global Button Functions
 **/
static void
launcher_button_state_changed (GtkWidget    *button_a,
                               GtkStateType  state,
                               GtkWidget    *button_b)
{
    if (GTK_WIDGET_STATE (button_b) != GTK_WIDGET_STATE (button_a)
        && GTK_WIDGET_STATE (button_a) != GTK_STATE_INSENSITIVE)
    {
        /* sync the button states */
        gtk_widget_set_state (button_b, GTK_WIDGET_STATE (button_a));
    }
}



/**
 * Menu Functions
 **/
static gboolean
launcher_menu_item_released (GtkWidget      *mi,
                             GdkEventButton *event,
                             LauncherPlugin *launcher)
{
    GdkScreen     *screen;
    gint           i;
    LauncherEntry *entry;

    /* get the widget screen */
    screen = gtk_widget_get_screen (mi);

    /* get the item number */
    i = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (mi), I_("entry-number")));

    /* get the entry from the list */
    entry = g_list_nth_data (launcher->entries, i);

    if (G_LIKELY (entry))
    {
        if (event->button == 1)
            launcher_execute (screen, entry, NULL);
        else if (event->button == 2)
            launcher_execute_from_clipboard (screen, entry);

        /* move the item to the first position in the list */
        if (G_UNLIKELY (launcher->move_first && i > 0))
        {
            /* remove from the list */
            launcher->entries = g_list_remove (launcher->entries, entry);

            /* insert in first position */
            launcher->entries = g_list_prepend (launcher->entries, entry);

            /* destroy the menu */
            launcher_menu_destroy (launcher);

            /* rebuild the icon button */
            launcher_icon_button_set_icon (launcher);
#if !LAUNCHER_NEW_TOOLTIP_API
            launcher_icon_button_set_tooltip (launcher);
#endif
        }
    }

    return FALSE;
}



static void
launcher_menu_popup_destroyed (gpointer user_data)
{
    LauncherPlugin *launcher = user_data;

    launcher->popup_timeout_id = 0;
}



static gboolean
launcher_menu_popup (gpointer user_data)
{
    LauncherPlugin *launcher = user_data;

    GDK_THREADS_ENTER ();

    /* check if the menu exists, if not, rebuild it */
    if (launcher->menu == NULL)
        launcher_menu_rebuild (launcher);

    /* toggle the arrow button */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (launcher->arrow_button), TRUE);

    /* popup menu */
    gtk_menu_popup (GTK_MENU (launcher->menu), NULL, NULL,
                    xfce_panel_plugin_position_menu,
                    launcher->panel_plugin,
                    1, gtk_get_current_event_time ());

    GDK_THREADS_LEAVE ();

    return FALSE;
}



static void
launcher_menu_deactivated (LauncherPlugin *launcher)
{
    /* deactivate arrow button */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (launcher->arrow_button), FALSE);
}



static void
launcher_menu_destroy (LauncherPlugin *launcher)
{
    if (launcher->menu != NULL)
    {
        /* destroy the menu and null the variable */
        gtk_widget_destroy (launcher->menu);
        launcher->menu = NULL;

        /* deactivate arrow button */
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (launcher->arrow_button), FALSE);
    }

    /* set the visibility of the arrow button */
    if (launcher->arrow_position == LAUNCHER_ARROW_INSIDE_BUTTON ||
        g_list_length (launcher->entries) < 2)
    {
        gtk_widget_hide (launcher->arrow_button);
    }
    else
    {
        gtk_widget_show (launcher->arrow_button);
    }
}



static void
launcher_menu_rebuild (LauncherPlugin *launcher)
{
    GdkScreen     *screen;
    GList         *li;
    guint          n = 0;
    LauncherEntry *entry;
    GtkWidget     *mi, *image;
    GdkPixbuf     *pixbuf;

    /* destroy the old menu */
    if (G_UNLIKELY (launcher->menu))
        launcher_menu_destroy (launcher);

    /* create new menu */
    launcher->menu = gtk_menu_new ();

    /* get the plugin screen */
    screen = gtk_widget_get_screen (GTK_WIDGET (launcher->panel_plugin));

    /* set the menu screen */
    gtk_menu_set_screen (GTK_MENU (launcher->menu), screen);

    /* walk through the entries */
    for (li = launcher->entries; li != NULL; li = li->next, n++)
    {
        /* skip the first entry when the arrow is visible */
        if (n == 0 && launcher->arrow_position != LAUNCHER_ARROW_INSIDE_BUTTON)
            continue;

        entry = li->data;

        /* create menu item */
        mi = gtk_image_menu_item_new_with_label (entry->name ? entry->name : _("New Item"));
        gtk_menu_shell_prepend (GTK_MENU_SHELL (launcher->menu), mi);
        gtk_widget_show (mi);

        /* try to set an image */
        if (G_LIKELY (entry->icon))
        {
            /* load pixbuf */
            pixbuf = launcher_utility_load_pixbuf (screen, entry->icon, LAUNCHER_MENU_SIZE);

            if (G_LIKELY (pixbuf))
            {
                /* set image */
                image = gtk_image_new_from_pixbuf (pixbuf);
                gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
                gtk_widget_show (image);

                /* release reference */
                g_object_unref (G_OBJECT (pixbuf));
            }
        }

        /* set entries list number */
        g_object_set_data (G_OBJECT (mi), I_("entry-number"), GINT_TO_POINTER (n));

        /* connect signals */
        g_signal_connect (G_OBJECT (mi), "button-release-event", G_CALLBACK (launcher_menu_item_released), launcher);
#if LAUNCHER_NEW_TOOLTIP_API
        g_object_set (G_OBJECT (mi), "has-tooltip", TRUE, NULL);
        g_signal_connect (G_OBJECT (mi), "query-tooltip", G_CALLBACK (launcher_utility_query_tooltip), entry);
#endif

        /* dnd support */
        gtk_drag_dest_set (mi, GTK_DEST_DEFAULT_ALL, drop_targets, G_N_ELEMENTS (drop_targets), GDK_ACTION_COPY);

#if !LAUNCHER_NEW_TOOLTIP_API
        /* set tooltip */
        if (entry->comment)
            gtk_tooltips_set_tip (launcher->tips, mi, entry->comment, NULL);
#endif
    }

    /* connect deactivate signal */
    g_signal_connect_swapped (G_OBJECT (launcher->menu), "deactivate", G_CALLBACK (launcher_menu_deactivated), launcher);
}



/**
 * Entry Functions
 **/
LauncherEntry *
launcher_entry_new (void)
{
    LauncherEntry *entry;

    /* allocate structure */
    entry = g_slice_new0 (LauncherEntry);

    /* set some default values */
    entry->name    = g_strdup (_("New Item"));
    entry->comment = NULL;
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



void
launcher_entry_free (LauncherEntry  *entry,
                     LauncherPlugin *launcher)
{
    /* remove from the list */
    if (G_LIKELY (launcher))
        launcher->entries = g_list_remove (launcher->entries, entry);

    /* free variables */
    g_free (entry->name);
    g_free (entry->comment);
    g_free (entry->path);
    g_free (entry->icon);
    g_free (entry->exec);

    /* free structure */
    g_slice_free (LauncherEntry, entry);
}



/**
 * Panel Plugin Functions
 **/
static LauncherPlugin*
launcher_plugin_new (XfcePanelPlugin *plugin)
{
    LauncherPlugin *launcher;
    gpointer        klass;

    /* create launcher structure */
    launcher = g_slice_new0 (LauncherPlugin);

    /* init */
    launcher->panel_plugin = plugin;
    launcher->menu = NULL;
    launcher->plugin_can_save = TRUE;

#if !LAUNCHER_NEW_TOOLTIP_API
    /* create tooltips */
    launcher->tips = gtk_tooltips_new ();
    exo_gtk_object_ref_sink (GTK_OBJECT (launcher->tips));
#endif

    /* create widgets */
    launcher->box = xfce_hvbox_new (GTK_ORIENTATION_HORIZONTAL, FALSE, 0);
    gtk_container_add (GTK_CONTAINER (plugin), launcher->box);
    gtk_widget_show (launcher->box);

    launcher->icon_button = xfce_create_panel_button ();
    gtk_box_pack_start (GTK_BOX (launcher->box), launcher->icon_button, TRUE, TRUE, 0);
    gtk_widget_show (launcher->icon_button);

    launcher->image = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (launcher->icon_button), launcher->image);
    gtk_widget_show (launcher->image);

    launcher->arrow_button = xfce_arrow_button_new (GTK_ARROW_UP);
    GTK_WIDGET_UNSET_FLAGS (launcher->arrow_button, GTK_CAN_DEFAULT | GTK_CAN_FOCUS);
    gtk_box_pack_start (GTK_BOX (launcher->box), launcher->arrow_button, FALSE, FALSE, 0);
    gtk_button_set_relief (GTK_BUTTON (launcher->arrow_button), GTK_RELIEF_NONE);
    gtk_button_set_focus_on_click (GTK_BUTTON (launcher->arrow_button), FALSE);

    /* hook for icon themes changes */
    klass = g_type_class_ref (GTK_TYPE_ICON_THEME);
    launcher->theme_timeout_id =
        g_signal_add_emission_hook (g_signal_lookup ("changed", GTK_TYPE_ICON_THEME),
                                    0, (GSignalEmissionHook) launcher_utility_icon_theme_changed,
                                    launcher, NULL);
    g_type_class_unref (klass);

    /* icon button signals */
    g_signal_connect (G_OBJECT (launcher->icon_button), "state-changed",
                      G_CALLBACK (launcher_button_state_changed), launcher->arrow_button);
    g_signal_connect (G_OBJECT (launcher->icon_button), "button-press-event",
                      G_CALLBACK (launcher_icon_button_pressed), launcher);
    g_signal_connect (G_OBJECT (launcher->icon_button), "button-release-event",
                      G_CALLBACK (launcher_icon_button_released), launcher);
    g_signal_connect (G_OBJECT (launcher->icon_button), "drag-data-received",
                      G_CALLBACK (launcher_icon_button_drag_data_received), launcher);
    g_signal_connect_after (G_OBJECT (launcher->image), "expose-event",
                            G_CALLBACK (launcher_icon_button_expose_event), launcher);

#if LAUNCHER_NEW_TOOLTIP_API
    g_object_set (G_OBJECT (launcher->icon_button), "has-tooltip", TRUE, NULL);
    g_signal_connect (G_OBJECT (launcher->icon_button), "query-tooltip",
                      G_CALLBACK (launcher_icon_button_query_tooltip), launcher);
#endif

    /* arrow button signals */
    g_signal_connect (G_OBJECT (launcher->arrow_button), "state-changed",
                      G_CALLBACK (launcher_button_state_changed), launcher->icon_button);
    g_signal_connect (G_OBJECT (launcher->arrow_button), "button-press-event",
                      G_CALLBACK (launcher_arrow_button_pressed), launcher);

    /* set drag destinations */
    gtk_drag_dest_set (launcher->icon_button, GTK_DEST_DEFAULT_ALL,
                       drop_targets, G_N_ELEMENTS (drop_targets),
                       GDK_ACTION_COPY);
    gtk_drag_dest_set (launcher->arrow_button, GTK_DEST_DEFAULT_ALL,
                       drop_targets, G_N_ELEMENTS (drop_targets),
                       GDK_ACTION_COPY);

    /* read the user settings */
    launcher_plugin_read (launcher);

    /* create new launcher */
    if (G_UNLIKELY (g_list_length (launcher->entries) == 0))
      {
        gchar **filenames;
        guint i;
        LauncherEntry *entry;

        /* check for startup arguments */
        if (xfce_panel_plugin_get_arguments (plugin, &filenames))
          {
            /* try to add the entries to the new launcher */
            for (i = 0; filenames[i] != NULL; i++)
              {
                /* create new entry */
                entry = launcher_entry_new ();

                if (launcher_dialog_read_desktop_file (filenames[i], entry))
                  launcher->entries = g_list_append (launcher->entries, entry);
                else
                  launcher_entry_free (entry, NULL);
              }

            /* cleanup */
            g_strfreev (filenames);
          }

        /* add new entry if the list is still empty */
        if (G_UNLIKELY (g_list_length (launcher->entries) == 0))
          launcher->entries = g_list_prepend (launcher->entries, launcher_entry_new ());
      }

    /* set the arrow direction */
    launcher_plugin_screen_position_changed (launcher);

    /* set the buttons in the correct position */
    launcher_plugin_pack_buttons (launcher);

    /* change the visiblity of the arrow button */
    launcher_menu_destroy (launcher);

#if !LAUNCHER_NEW_TOOLTIP_API
    /* set the button tooltip */
    launcher_icon_button_set_tooltip (launcher);
#endif

    return launcher;
}



void
launcher_plugin_rebuild (LauncherPlugin *launcher,
                         gboolean        update_icon)
{
    /* pack buttons again */
    launcher_plugin_pack_buttons (launcher);

    /* size */
    launcher_plugin_set_size (launcher, xfce_panel_plugin_get_size (launcher->panel_plugin));

#if !LAUNCHER_NEW_TOOLTIP_API
    /* update tooltip */
    launcher_icon_button_set_tooltip (launcher);
#endif

    if (update_icon)
        launcher_icon_button_set_icon (launcher);

    /* destroy menu */
    launcher_menu_destroy (launcher);
}



static void
launcher_plugin_pack_buttons (LauncherPlugin *launcher)
{
    GtkOrientation orientation;
    guint          position = launcher->arrow_position;
    gint           box_position, width = -1, height = -1;

    if (position == LAUNCHER_ARROW_DEFAULT)
    {
        /* get the current panel orientation */
        orientation = xfce_panel_plugin_get_orientation (launcher->panel_plugin);

        /* get the arrow position in the default layout */
        if (orientation == GTK_ORIENTATION_HORIZONTAL)
            position = LAUNCHER_ARROW_RIGHT;
        else
            position = LAUNCHER_ARROW_BOTTOM;
    }
    else if (position == LAUNCHER_ARROW_INSIDE_BUTTON)
    {
        /* nothing to arrange */
        return;
    }

    /* set the arrow button position in the box */
    box_position = (position == LAUNCHER_ARROW_LEFT || position == LAUNCHER_ARROW_TOP) ? 0 : -1;
    gtk_box_reorder_child (GTK_BOX (launcher->box), launcher->arrow_button, box_position);

    /* set the arrow button size and box orientation */
    if (position == LAUNCHER_ARROW_LEFT || position == LAUNCHER_ARROW_RIGHT)
    {
        orientation = GTK_ORIENTATION_HORIZONTAL;
        width = LAUNCHER_ARROW_SIZE;
    }
    else
    {
        orientation = GTK_ORIENTATION_VERTICAL;
        height = LAUNCHER_ARROW_SIZE;
    }

    gtk_widget_set_size_request (launcher->arrow_button, width, height);
    xfce_hvbox_set_orientation (XFCE_HVBOX (launcher->box), orientation);

    /* set the visibility of the arrow button */
    if (g_list_length (launcher->entries) > 1)
        gtk_widget_show (launcher->arrow_button);
    else
        gtk_widget_hide (launcher->arrow_button);
}



static gchar *
launcher_plugin_read_entry (XfceRc      *rc,
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
launcher_plugin_read (LauncherPlugin *launcher)
{
    gchar         *file;
    gchar          group[10];
    XfceRc        *rc;
    guint          i;
    LauncherEntry *entry;

    /* get rc file name, create it if needed */
    file = xfce_panel_plugin_lookup_rc_file ( launcher->panel_plugin);
    if (G_LIKELY (file))
    {
        /* open config file, read-only */
        rc = xfce_rc_simple_open (file, TRUE);

        /* cleanup */
        g_free (file);

        if (G_LIKELY (rc))
        {
            /* read global settings */
            xfce_rc_set_group (rc, "Global");

            launcher->move_first = xfce_rc_read_bool_entry (rc, "MoveFirst", FALSE);
            launcher->arrow_position = xfce_rc_read_int_entry (rc, "ArrowPosition", 0);

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
                entry = g_slice_new0 (LauncherEntry);

                /* read all the entry settings */
                entry->name = launcher_plugin_read_entry (rc, "Name");
                entry->comment = launcher_plugin_read_entry (rc, "Comment");
                entry->icon = launcher_plugin_read_entry (rc, "Icon");
                entry->exec = launcher_plugin_read_entry (rc, "Exec");
                entry->path = launcher_plugin_read_entry (rc, "Path");

                entry->terminal = xfce_rc_read_bool_entry (rc, "Terminal", FALSE);
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
                entry->startup = xfce_rc_read_bool_entry (rc, "StartupNotify", FALSE);
#endif
                /* prepend the entry */
                launcher->entries = g_list_prepend (launcher->entries, entry);
            }

            /* reverse the list */
            launcher->entries = g_list_reverse (launcher->entries);

            /* close the rc file */
            xfce_rc_close (rc);
        }
    }
}



void
launcher_plugin_save (LauncherPlugin *launcher)
{
    gchar          *file;
    gchar         **groups;
    gchar           group[10];
    XfceRc         *rc;
    GList          *li;
    guint           i;
    LauncherEntry  *entry;

    /* check if it's allowed to save */
    if (G_UNLIKELY (launcher->plugin_can_save == FALSE))
        return;

    /* get rc file name, create it if needed */
    file = xfce_panel_plugin_save_location ( launcher->panel_plugin, TRUE);
    if (G_LIKELY (file))
    {
        /* open the config file, writable */
        rc = xfce_rc_simple_open (file, FALSE);

        /* cleanup */
        g_free (file);

        if (G_LIKELY (rc))
        {
            /* retreive all the groups in the config file */
            groups = xfce_rc_get_groups (rc);
            if (G_LIKELY (groups))
            {
                /* remove all the groups */
                for (i = 0; groups[i] != NULL; i++)
                    xfce_rc_delete_group (rc, groups[i], TRUE);

                /* cleanup */
                g_strfreev (groups);
            }

            /* save global launcher settings */
            xfce_rc_set_group (rc, "Global");
            xfce_rc_write_bool_entry (rc, "MoveFirst", launcher->move_first);
            xfce_rc_write_int_entry (rc, "ArrowPosition", launcher->arrow_position);

            /* save all the entries */
            for (li = launcher->entries, i = 0; li != NULL; li = li->next, i++)
            {
                entry = li->data;

                /* create group name */
                g_snprintf (group, sizeof (group), "Entry %d", i);

                /* set entry group */
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
    }
}



static void
launcher_plugin_screen_position_changed (LauncherPlugin *launcher)
{
    GtkArrowType arrow_type;

    /* get the arrow type */
    arrow_type = xfce_panel_plugin_arrow_type (launcher->panel_plugin);

    /* set the arrow direction */
    xfce_arrow_button_set_arrow_type (XFCE_ARROW_BUTTON (launcher->arrow_button), arrow_type);
}



static void
launcher_plugin_orientation_changed (LauncherPlugin *launcher)
{
    /* set the arrow direction */
    launcher_plugin_screen_position_changed (launcher);

    /* reorder the boxes again */
    launcher_plugin_pack_buttons (launcher);

    /* update size */
    launcher_plugin_set_size (launcher, xfce_panel_plugin_get_size (launcher->panel_plugin));
}



static gboolean
launcher_plugin_set_size (LauncherPlugin  *launcher,
                          guint            size)
{
    gint            width = size, height = size;
    GtkOrientation  orientation;
    GtkWidget      *widget = launcher->icon_button;

    if (g_list_length (launcher->entries) > 1)
    {
        /* get the orientation of the panel */
        orientation = xfce_panel_plugin_get_orientation (launcher->panel_plugin);

        switch (launcher->arrow_position)
        {
            case LAUNCHER_ARROW_DEFAULT:
                if (orientation == GTK_ORIENTATION_HORIZONTAL)
                    width += LAUNCHER_ARROW_SIZE;
                else
                    height += LAUNCHER_ARROW_SIZE;

                break;

            case LAUNCHER_ARROW_LEFT:
            case LAUNCHER_ARROW_RIGHT:
                if (orientation == GTK_ORIENTATION_HORIZONTAL)
                    width += LAUNCHER_ARROW_SIZE;
                else
                    height -= LAUNCHER_ARROW_SIZE;

                break;

            case LAUNCHER_ARROW_TOP:
            case LAUNCHER_ARROW_BOTTOM:
                if (orientation == GTK_ORIENTATION_HORIZONTAL)
                    width -= LAUNCHER_ARROW_SIZE;
                else
                    height += LAUNCHER_ARROW_SIZE;

                break;

            default:
                /* nothing to do for a hidden arrow button */
                break;
        }
    }

    /* calculate the image size inside the button */
    launcher->image_size = MIN (width, height);
    launcher->image_size -= 2 + 2 * MAX (widget->style->xthickness, widget->style->ythickness);

    /* set the plugin size */
    gtk_widget_set_size_request (GTK_WIDGET (launcher->panel_plugin), width, height);

    /* update the icon button */
    launcher_icon_button_set_icon (launcher);

    /* we handled the size */
    return TRUE;
}



static void
launcher_plugin_free (LauncherPlugin  *launcher)
{
    GtkWidget *dialog;

    /* check if we still need to destroy the properties dialog */
    dialog = g_object_get_data (G_OBJECT (launcher->panel_plugin), I_("launcher-dialog"));
    if (G_UNLIKELY (dialog != NULL))
        gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);

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
    g_list_foreach (launcher->entries, (GFunc) launcher_entry_free, launcher);
    g_list_free (launcher->entries);

#if !LAUNCHER_NEW_TOOLTIP_API
    /* release the tooltips */
    g_object_unref (G_OBJECT (launcher->tips));
#endif

    /* free launcher structure */
    g_slice_free (LauncherPlugin, launcher);
}



static void
launcher_plugin_construct (XfcePanelPlugin *plugin)
{
    LauncherPlugin *launcher;

    /* create the plugin */
    launcher = launcher_plugin_new (plugin);

    /* set the action widgets and show configure */
    xfce_panel_plugin_add_action_widget (plugin, launcher->icon_button);
    xfce_panel_plugin_add_action_widget (plugin, launcher->arrow_button);
    xfce_panel_plugin_menu_show_configure (plugin);

    /* connect signals */
    g_signal_connect_swapped (G_OBJECT (plugin), "screen-position-changed",
                              G_CALLBACK (launcher_plugin_screen_position_changed), launcher);
    g_signal_connect_swapped (G_OBJECT (plugin), "orientation-changed",
                              G_CALLBACK (launcher_plugin_orientation_changed), launcher);
    g_signal_connect_swapped (G_OBJECT (plugin), "size-changed",
                              G_CALLBACK (launcher_plugin_set_size), launcher);
    g_signal_connect_swapped (G_OBJECT (plugin), "save",
                              G_CALLBACK (launcher_plugin_save), launcher);
    g_signal_connect_swapped (G_OBJECT (plugin), "free-data",
                              G_CALLBACK (launcher_plugin_free), launcher);
    g_signal_connect_swapped (G_OBJECT (plugin), "configure-plugin",
                              G_CALLBACK (launcher_dialog_show), launcher);
}
