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

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-arrow-button.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "launcher.h"
#include "launcher-dialog.h"


static LauncherPlugin * launcher_new (XfcePanelPlugin *plugin);

static void launcher_free (LauncherPlugin *launcher);

static void launcher_set_orientation (XfcePanelPlugin *plugin, 
                                      LauncherPlugin *launcher, 
                                      GtkOrientation orientation);

static void launcher_set_screen_position (LauncherPlugin *launcher, 
                                          XfceScreenPosition position);

static void launcher_write_rc_file (XfcePanelPlugin *plugin, 
                                    LauncherPlugin *launcher);

static void launcher_construct (XfcePanelPlugin *plugin);

/* -------------------------------------------------------------------- *
 *                     Panel Plugin Interface                           *
 * -------------------------------------------------------------------- */

/* Register with the panel */

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (launcher_construct);


/* Interface Implementation */

static void
launcher_screen_position_changed (XfcePanelPlugin *plugin, 
                                  XfceScreenPosition position, 
                                  LauncherPlugin *launcher)
{
    launcher_set_screen_position (launcher, position);
}

static void
launcher_orientation_changed (XfcePanelPlugin *plugin, 
                              GtkOrientation orientation, 
                              LauncherPlugin *launcher)
{
    launcher_set_orientation (plugin, launcher, orientation);
}

static gboolean 
launcher_set_size (XfcePanelPlugin *plugin, int size, LauncherPlugin *launcher)
{
    gtk_widget_set_size_request (launcher->iconbutton, size, size);

    return TRUE;
}

static void 
launcher_free_data (XfcePanelPlugin *plugin, LauncherPlugin *launcher)
{
    GtkWidget *dlg = g_object_get_data (G_OBJECT (plugin), "dialog");

    if (dlg)
        gtk_widget_destroy (dlg);
    
    launcher_free (launcher);
}

void 
launcher_save (XfcePanelPlugin *plugin, LauncherPlugin *launcher)
{
    launcher_write_rc_file (plugin, launcher);
}

static void 
launcher_configure (XfcePanelPlugin *plugin, LauncherPlugin *launcher)
{
    launcher_properties_dialog (plugin, launcher);
}

/* create widgets and connect to signals */
static void 
launcher_construct (XfcePanelPlugin *plugin)
{
    LauncherPlugin *launcher = launcher_new (plugin);

    gtk_container_add (GTK_CONTAINER (plugin), launcher->box);

    xfce_panel_plugin_add_action_widget (plugin, launcher->iconbutton);
    xfce_panel_plugin_add_action_widget (plugin, launcher->arrowbutton);

    g_signal_connect (plugin, "screen-position-changed", 
                      G_CALLBACK (launcher_screen_position_changed), launcher);

    g_signal_connect (plugin, "orientation-changed", 
                      G_CALLBACK (launcher_orientation_changed), launcher);
    
    g_signal_connect (plugin, "size-changed", 
                      G_CALLBACK (launcher_set_size), launcher);
    
    g_signal_connect (plugin, "free-data", 
                      G_CALLBACK (launcher_free_data), launcher);
    
    g_signal_connect (plugin, "save", 
                      G_CALLBACK (launcher_save), launcher);
    
    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
                      G_CALLBACK (launcher_configure), launcher);
}

/* -------------------------------------------------------------------- *
 *                   Launcher Plugin Interface                          *
 * -------------------------------------------------------------------- */

static XfceIconTheme *icontheme = NULL;
static LauncherPlugin *open_launcher = NULL;


/* DND *
 * --- */

static GtkTargetEntry target_list [] =
{
    { "text/uri-list", 0, 0 },
    { "STRING", 0, 0 }
};

static guint n_targets = G_N_ELEMENTS (target_list);

GPtrArray *
launcher_get_file_list_from_selection_data (GtkSelectionData *data)
{
    GPtrArray *files;
    const char *s1, *s2;

    if (data->length < 1)
        return NULL;

    files = g_ptr_array_new ();

    /* Assume text/uri-list (RFC2483):
     * - Commented lines are allowed; they start with #.
     * - Lines are separated by CRLF (\r\n)
     * - We also allow LF (\n) as separator
     * - We strip "file:" and multiple slashes ("/") at the start
     */
    for (s1 = (const char *)data->data; s1 != NULL && strlen(s1); ++s1)
    {
        if (*s1 != '#')
        {
            while (isspace ((int)*s1))
                ++s1;
            
            if (!strncmp (s1, "file:", 5))
            {
                s1 += 5;
                while (*(s1 + 1) == '/')
                    ++s1;
            }
            
            for (s2 = s1; *s2 != '\0' && *s2 != '\r' && *s2 != '\n'; ++s2)
                /* */;
            
            if (s2 > s1)
            {
                while (isspace ((int)*(s2-1)))
                    --s2;
                
                if (s2 > s1)
                {
                    int len, i;
                    char *file;
                    
                    len = s2 - s1;
                    file = g_new (char, len + 1);
                    
                    /* decode % escaped characters */
                    for (i = 0, s2 = s1; s2 - s1 <= len; ++i, ++s2)
                    {
                        if (*s2 != '%' || s2 + 3 -s1 > len)
                        {
                            file[i] = *s2;
                        }
                        else
                        {
                            guint c;
                            
                            if (sscanf (s2+1, "%2x", &c) == 1)
                                file[i] = (char)c;

                            s2 += 2;
                        }
                    }

                    file[i-1] = '\0';
                    g_ptr_array_add (files, file);
                }
            }
        }
        
        if (!(s1 = strchr (s1, '\n')))
            break;
    }
    
    if (files->len > 0)
    {
        return files;
    }

    g_ptr_array_free (files, TRUE);

    return NULL;    
}

void launcher_set_drag_dest (GtkWidget *widget)
{
    gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_ALL, 
                       target_list, n_targets, GDK_ACTION_COPY);
}


/* utility functions *
 * ----------------- */

GdkPixbuf *
launcher_icon_load_pixbuf (LauncherIcon *icon, int size)
{
    GdkPixbuf *pb = NULL;
    
    if (icon->type == LAUNCHER_ICON_TYPE_NAME)
    {
        if (g_path_is_absolute (icon->icon.name))
            pb = xfce_pixbuf_new_from_file_at_size (icon->icon.name, 
                                                    size, size, NULL);
        else
            pb = xfce_themed_icon_load (icon->icon.name, size);
    }
    else if (icon->type == LAUNCHER_ICON_TYPE_CATEGORY)
    {
        pb = xfce_icon_theme_load_category (icontheme, icon->icon.category, 
                                            size);
    }

    if (!pb)
        pb = xfce_icon_theme_load_category (icontheme, 0, size);

    return pb;
}

LauncherEntry *
launcher_entry_new (void)
{
    return g_new0 (LauncherEntry, 1);
}

void
launcher_entry_free (LauncherEntry *e)
{
    g_free (e->name);
    g_free (e->comment);
    if (e->icon.type == LAUNCHER_ICON_TYPE_NAME)
        g_free (e->icon.icon.name);
    g_free (e->exec);
    g_free (e->real_exec);

    g_free (e);
}

static void
launcher_entry_exec (GdkScreen *screen, LauncherEntry *entry)
{
    GError *error = NULL;
    
    if (!entry->exec || !strlen (entry->exec))
        return;
    
    xfce_exec_on_screen (screen, entry->real_exec, entry->terminal, 
                         entry->startup, &error);
        
    if (error)
    {
        char first[256];

        g_snprintf (first, 256, _("Could not run \"%s\""), entry->name);
    
        xfce_message_dialog (NULL, _("Xfce Panel"), 
                             GTK_STOCK_DIALOG_ERROR, first, error->message,
                             GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);

        g_error_free (error);
    }
}

static void
launcher_entry_drop_cb (GdkScreen *screen, LauncherEntry *entry, 
                        GPtrArray *files)
{
    char **argv;
    int i, n;
    GError *error = NULL;

    n = files->len;
    
    if (G_UNLIKELY (!entry->exec))
        return;

    /* FIXME: realexec may contain arguments -> split up */
    if (entry->terminal)
    {
        argv = g_new (char *, n + 4);
        argv[0] = "xfterm4";
        argv[1] = "-e";
        argv[2] = entry->real_exec;
        n = 3;
    }
    else
    {
        argv = g_new (char *, n + 2);
        argv[0] = entry->real_exec;
        n = 1;
    }

    for (i = 0; i < files->len; ++i)
        argv[n+i] = g_ptr_array_index (files, i);

    argv[n+i] = NULL;
    
    if (!xfce_exec_argv_on_screen (screen, argv, entry->terminal, 
                                   entry->startup, &error))
    {
        char first[256];
        
        g_snprintf (first, 256, _("Could not run \"%s\""), entry->name);
    
        xfce_message_dialog (NULL, _("Xfce Panel"), 
                             GTK_STOCK_DIALOG_ERROR, first, error->message,
                             GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);

        g_free (first);
                                    
        g_error_free (error);
    }

    g_free (argv);
}

static void
launcher_entry_data_received (GtkWidget *widget, GdkDragContext *context, 
                              gint x, gint y, GtkSelectionData *data, 
                              guint info, guint time, LauncherEntry *entry)
{
    GPtrArray *files;
    
    if (!data || data->length < 1)
        return;
    
    files = launcher_get_file_list_from_selection_data (data);

    if (files)
    {
        launcher_entry_drop_cb (gtk_widget_get_screen (widget), entry, files);
        g_ptr_array_free (files, TRUE);
    }

    if (open_launcher)
    {
        gtk_widget_hide (GTK_MENU (open_launcher->menu)->toplevel);
        gtk_toggle_button_set_active (
                GTK_TOGGLE_BUTTON (open_launcher->arrowbutton), FALSE);
        open_launcher = NULL;
    }
}

static void
launcher_drag_data_received (GtkWidget *widget, GdkDragContext *context, 
                             gint x, gint y, GtkSelectionData *data, 
                             guint info, guint time, LauncherPlugin *launcher)
{
    GPtrArray *files;
    
    if (!data || data->length < 1)
        return;
    
    files = launcher_get_file_list_from_selection_data (data);

    if (files)
    {
        launcher_entry_drop_cb (gtk_widget_get_screen (widget), 
                                g_ptr_array_index (launcher->entries, 0), 
                                files);

        g_ptr_array_free (files, TRUE);
    }

    if (open_launcher)
    {
        gtk_widget_hide (GTK_MENU (open_launcher->menu)->toplevel);
        gtk_toggle_button_set_active (
                GTK_TOGGLE_BUTTON (open_launcher->arrowbutton), FALSE);
        open_launcher = NULL;
    }
}

static void
launcher_menu_deactivated (GtkWidget *menu, LauncherPlugin *launcher)
{
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (launcher->arrowbutton), 
                                  FALSE);
    launcher->from_timeout = FALSE;

    /* 'fix' button depressed state for gtk 2.4 */
    if (gtk_major_version == 2 && gtk_minor_version == 4)
    {
        gtk_button_released (GTK_BUTTON (launcher->iconbutton));
    }
}

static gboolean
launcher_button_released (GtkWidget *mi, GdkEventButton *ev, 
                          LauncherPlugin *launcher)
{
    if (launcher->from_timeout)
    {
        launcher->from_timeout = FALSE;
        /* don't activate on button release */
        return TRUE;
    }
    
    /* don't activate on right-click */
    if (ev->button == 3)
        return TRUE;

    return FALSE;
}

static void
launcher_menu_item_activate (GtkWidget *mi, LauncherEntry *entry)
{
    launcher_entry_exec (gtk_widget_get_screen (mi), entry);
}

static void
menu_detached (void)
{
    /* do nothing */
}

static gboolean
launcher_menu_drag_leave_timeout (LauncherPlugin *launcher)
{
    GdkScreen *screen = gtk_widget_get_screen (launcher->arrowbutton);
    GdkDisplay *dpy = gdk_screen_get_display (screen);
    int x, y, wx, wy, ww, wh;
    
    gdk_display_get_pointer (dpy, NULL, &x, &y, NULL);
    
    gdk_window_get_root_origin (launcher->menu->window, &wx, &wy);
    gdk_drawable_get_size (GDK_DRAWABLE (launcher->menu->window), &ww, &wh);
    
    if (x < wx || x > wx + ww || y < wy || y > wy +wh)
    {
        gtk_widget_hide (GTK_MENU (launcher->menu)->toplevel);
        gtk_toggle_button_set_active (
                GTK_TOGGLE_BUTTON (launcher->arrowbutton), FALSE);
    }
    
    return FALSE;
}

static void
launcher_menu_drag_leave (GtkWidget *w, GdkDragContext *drag_context,
                          guint time, LauncherPlugin *launcher)
{
    g_timeout_add (100, (GSourceFunc)launcher_menu_drag_leave_timeout, 
                   launcher);
}

static void
launcher_create_menu (LauncherPlugin *launcher)
{
    launcher->menu = gtk_menu_new ();

    g_signal_connect (launcher->menu, "button-release-event", 
                      G_CALLBACK (launcher_button_released),
                      launcher);
    
    g_signal_connect (launcher->menu, "deactivate", 
                      G_CALLBACK (launcher_menu_deactivated), launcher);

    gtk_menu_attach_to_widget (GTK_MENU (launcher->menu), 
                               launcher->arrowbutton,
                               (GtkMenuDetachFunc)menu_detached);

    launcher_set_drag_dest (launcher->menu);
    
    g_signal_connect (launcher->menu, "drag-leave", 
                      G_CALLBACK (launcher_menu_drag_leave), launcher);
}

static void
launcher_destroy_menu (LauncherPlugin *launcher)
{
    gtk_widget_destroy (launcher->menu);

    launcher->menu = NULL;
}

static gboolean
load_menu_icon (GtkImageMenuItem *mi)
{
    GtkWidget *img;
    GdkPixbuf *pb;
    LauncherEntry *entry;
    
    if ((entry = g_object_get_data (G_OBJECT (mi), "launcher_entry")) != NULL)
    {
        pb = launcher_icon_load_pixbuf (&entry->icon, MENU_ICON_SIZE);

        img = gtk_image_new_from_pixbuf (pb);
        gtk_widget_show (img);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);
        g_object_unref (pb);
    }

    return FALSE;
}

void
launcher_recreate_menu (LauncherPlugin *launcher)
{
    int i;
    
    if (launcher->menu)
        launcher_destroy_menu (launcher);

    if (launcher->entries->len <= 1)
    {
        gtk_widget_hide (launcher->arrowbutton);
        return;
    }
    
    launcher_create_menu (launcher);
    
    for (i = launcher->entries->len - 1; i > 0; --i)
    {
        GtkWidget *mi;
        LauncherEntry *entry = g_ptr_array_index (launcher->entries, i);
        
        mi = gtk_image_menu_item_new_with_label (entry->name ? 
                entry->name : _("New Item"));
        gtk_widget_show (mi);
        gtk_menu_shell_prepend (GTK_MENU_SHELL (launcher->menu), mi);

        /* delayed loading of icons */
        g_object_set_data (G_OBJECT (mi), "launcher_entry", entry);
        g_idle_add ((GSourceFunc) load_menu_icon, mi);
        
        g_signal_connect (mi, "button-release-event", 
                          G_CALLBACK (launcher_button_released),
                          launcher);
        
        g_signal_connect (mi, "activate", 
                          G_CALLBACK (launcher_menu_item_activate), entry);

        gtk_tooltips_set_tip (launcher->tips, mi, entry->comment, NULL);

        launcher_set_drag_dest (mi);
        
        g_signal_connect (mi, "drag-data-received", 
                          G_CALLBACK (launcher_entry_data_received), entry);

        g_signal_connect (mi, "drag-leave", 
                          G_CALLBACK (launcher_menu_drag_leave), launcher);
    }
}

void
launcher_update_panel_entry (LauncherPlugin *launcher)
{    
    GdkPixbuf *pb;
    char *tip = NULL;
    LauncherEntry *entry;
    
    if (launcher->entries->len == 0)
        return;
    
    entry = g_ptr_array_index (launcher->entries, 0);

    pb = launcher_icon_load_pixbuf (&(entry->icon), PANEL_ICON_SIZE);
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (launcher->iconbutton), pb);
    g_object_unref (pb);

    if (entry->name)
    {
        if (entry->comment)
            tip = g_strdup_printf ("%s\n%s", entry->name, entry->comment);
        else
            tip = g_strdup (entry->name);

    }
    else
    {
        tip = g_strdup (_("This item has not yet been configured"));
    }

    gtk_tooltips_set_tip (launcher->tips, launcher->iconbutton, tip, NULL);
    g_free (tip);
}

static gboolean
update_panel_entry_idle (LauncherPlugin *launcher)
{
    launcher_update_panel_entry (launcher);
    return FALSE;
}

static void
launcher_position_menu (GtkMenu * menu, int *x, int *y, gboolean * push_in,
                        GtkWidget *b)
{
    GtkWidget *widget;
    GtkRequisition req;
    GdkScreen *screen;
    GdkRectangle geom;
    int num;

    widget = b->parent->parent;
    
    if (!GTK_WIDGET_REALIZED (GTK_WIDGET (menu)))
        gtk_widget_realize (GTK_WIDGET (menu));

    gtk_widget_size_request (GTK_WIDGET (menu), &req);

    gdk_window_get_origin (widget->window, x, y);

    widget = b->parent;
 
    switch (xfce_arrow_button_get_arrow_type (XFCE_ARROW_BUTTON (b)))
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
        case GTK_ARROW_RIGHT:
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
real_toggle_menu (LauncherPlugin *launcher)
{
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (launcher->arrowbutton), 
                                                     TRUE);
    
    if (launcher->entries->len > 1)
    {
        xfce_panel_plugin_register_menu (XFCE_PANEL_PLUGIN (launcher->plugin), 
                                         GTK_MENU (launcher->menu));

        gtk_menu_popup (GTK_MENU (launcher->menu), NULL, NULL, 
                        (GtkMenuPositionFunc) launcher_position_menu, 
                        launcher->arrowbutton, 0, 
                        gtk_get_current_event_time ());
    }

    launcher->popup_timeout = 0;
    return FALSE;
}

static gboolean
launcher_toggle_menu_timeout (GtkToggleButton *b, GdkEventButton *ev, 
                              LauncherPlugin *launcher)
{
    if (ev->button != 1)
        return FALSE;

    if (launcher->popup_timeout < 1)
    {
        launcher->from_timeout = TRUE;
        
        launcher->popup_timeout = 
            g_timeout_add (MENU_TIMEOUT, (GSourceFunc)real_toggle_menu, 
                           launcher);
    }

    return FALSE;
}

static gboolean
launcher_toggle_menu (GtkToggleButton *b, GdkEventButton *ev, 
                      LauncherPlugin *launcher)
{
    if (ev->button != 1)
        return FALSE;
    
    real_toggle_menu (launcher);

    return TRUE;
}

static gboolean
launcher_arrow_drag (GtkToggleButton * tb, GdkDragContext *drag_context,
                     gint x, gint y, guint time, LauncherPlugin * launcher)
{
    int push_in;

    if (open_launcher && open_launcher != launcher)
    {
        gtk_widget_hide (open_launcher->menu);
        gtk_toggle_button_set_active (
                GTK_TOGGLE_BUTTON (open_launcher->arrowbutton), FALSE);
        open_launcher = NULL;
    }
    
    gtk_toggle_button_set_active (tb, TRUE);

    gtk_widget_show (launcher->menu);

    {
        GtkRequisition tmp_request;
        GtkAllocation tmp_allocation = { 0, };

        gtk_widget_size_request (GTK_MENU (launcher->menu)->toplevel, 
                                 &tmp_request);

        tmp_allocation.width = tmp_request.width;
        tmp_allocation.height = tmp_request.height;

        gtk_widget_size_allocate (GTK_MENU (launcher->menu)->toplevel, 
                                  &tmp_allocation);

        gtk_widget_realize (launcher->menu);
    }

    launcher_position_menu (GTK_MENU (launcher->menu), &x, &y, &push_in,
                            GTK_WIDGET (tb));

    gtk_window_move (GTK_WINDOW (GTK_MENU (launcher->menu)->toplevel),
                     x, y);

    gtk_widget_show (GTK_MENU (launcher->menu)->toplevel);

    open_launcher = launcher;

    return TRUE;
}

static void
launcher_state_changed (GtkWidget *b1, GtkStateType state, GtkWidget *b2)
{
    if (GTK_WIDGET_STATE (b2) != GTK_WIDGET_STATE (b1)
        && GTK_WIDGET_STATE (b1) != GTK_STATE_INSENSITIVE)
    {
        gtk_widget_set_state (b2, GTK_WIDGET_STATE (b1));
    }
}

static void
launcher_clicked (GtkWidget *w, LauncherPlugin *launcher)
{
    if (launcher->popup_timeout > 0)
    {
        g_source_remove (launcher->popup_timeout);
        launcher->popup_timeout = 0;
        launcher->from_timeout = FALSE;
    }

    launcher_entry_exec (gtk_widget_get_screen (w),
                         g_ptr_array_index (launcher->entries, 0));
}


/* Read Configuration Data */

static LauncherEntry*
launcher_entry_from_rc_file (XfceRc *rc)
{
    LauncherEntry *entry = launcher_entry_new ();
    const char *s;

    if ((s = xfce_rc_read_entry (rc, "Name", NULL)) != NULL)
        entry->name = g_strdup (s);

    if ((s = xfce_rc_read_entry (rc, "Exec", NULL)) != NULL)
    {
        entry->exec = g_strdup (s);
        
        if (!(entry->real_exec = xfce_expand_variables (entry->exec, NULL)))
                entry->real_exec = g_strdup (entry->exec);
    }

    entry->terminal = xfce_rc_read_bool_entry (rc, "Terminal", FALSE);
    
    entry->startup = xfce_rc_read_bool_entry (rc, "StartupNotify", FALSE);

    if ((s = xfce_rc_read_entry (rc, "Comment", NULL)) != NULL)
        entry->comment = g_strdup (s);

    if ((s = xfce_rc_read_entry (rc, "Icon", NULL)) != NULL)
    {
        entry->icon.type = LAUNCHER_ICON_TYPE_NAME;
        entry->icon.icon.name = g_strdup (s);
    }
    else
    {
        entry->icon.type = LAUNCHER_ICON_TYPE_CATEGORY;
        entry->icon.icon.category = 
            xfce_rc_read_int_entry (rc, "X-XFCE-IconCategory", 0);
    }

    return entry;
}

static void
launcher_read_rc_file (XfcePanelPlugin *plugin, LauncherPlugin *launcher)
{
    XfceRc *rc;
    char *file;
    int i;
    
    if (!(file = xfce_panel_plugin_lookup_rc_file (plugin)))
        return;

    rc = xfce_rc_simple_open (file, TRUE);
    g_free (file);

    if (!rc)
        return;
    
    for (i = 0; i < 100 /* arbitrary */; ++i)
    {
        LauncherEntry *entry;
        char group[10];
        
        g_snprintf (group, 10, "Entry %d", i);
        
        if (!xfce_rc_has_group (rc, group))
            break;
        
        xfce_rc_set_group (rc, group);

        if ((entry = launcher_entry_from_rc_file (rc)) != NULL)
            g_ptr_array_add (launcher->entries, entry);
    }
    
    xfce_rc_close (rc);
    
    g_idle_add ((GSourceFunc)update_panel_entry_idle, launcher);
    launcher_recreate_menu (launcher);
}

/* Write Configuration Data */

static void
launcher_entry_write_rc_file (LauncherEntry *entry, XfceRc *rc)
{
    if (entry->name)
        xfce_rc_write_entry (rc, "Name", entry->name);

    if (entry->exec)
        xfce_rc_write_entry (rc, "Exec", entry->exec);

    xfce_rc_write_bool_entry (rc, "Terminal", entry->terminal);
    
    xfce_rc_write_bool_entry (rc, "StartupNotify", entry->startup);
    
    if (entry->comment)
        xfce_rc_write_entry (rc, "Comment", entry->comment);

    if (entry->icon.type == LAUNCHER_ICON_TYPE_CATEGORY)
    {
        xfce_rc_write_int_entry (rc, "X-XFCE-IconCategory", 
                                 entry->icon.icon.category);
    }
    else if (entry->icon.type == LAUNCHER_ICON_TYPE_NAME)
    {
        xfce_rc_write_entry (rc, "Icon", entry->icon.icon.name);
    }
}

static void
launcher_write_rc_file (XfcePanelPlugin *plugin, LauncherPlugin *launcher)
{
    char *file;
    XfceRc *rc;
    char group[10];
    int i;

    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;

    unlink (file);
    
    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;
    
    for (i = 0; i < launcher->entries->len; ++i)
    {
        LauncherEntry *entry = g_ptr_array_index (launcher->entries, i);

        g_snprintf (group, 10, "Entry %d", i);

        xfce_rc_set_group (rc, group);

        launcher_entry_write_rc_file (entry, rc);
    }

    xfce_rc_close (rc);
}

/* position and orientation */
static GtkArrowType
calculate_floating_arrow_type (LauncherPlugin *launcher, 
                               XfceScreenPosition position)
{
    GtkArrowType type = GTK_ARROW_UP;
    int mon, x, y;
    GdkScreen *screen;
    GdkRectangle geom;
    
    if (!GTK_WIDGET_REALIZED (launcher->iconbutton))
    {
        if (xfce_screen_position_is_horizontal (position))
            return GTK_ARROW_UP;
        else
            return GTK_ARROW_LEFT;
    }
    
    screen = gtk_widget_get_screen (launcher->iconbutton);
    mon = gdk_screen_get_monitor_at_window (screen, 
                                            launcher->iconbutton->window);
    gdk_screen_get_monitor_geometry (screen, mon, &geom);
    
    gdk_window_get_root_origin (launcher->iconbutton->window, &x, &y);
    
    if (xfce_screen_position_is_horizontal (position))
    {
        if (y > geom.y + geom.height / 2)
            type = GTK_ARROW_UP;
        else
            type = GTK_ARROW_DOWN;
    }
    else
    {
        if (x > geom.x + geom.width / 2)
            type = GTK_ARROW_LEFT;
        else
            type = GTK_ARROW_RIGHT;
    }

    return type;
}

static void
launcher_set_screen_position (LauncherPlugin *launcher, 
                              XfceScreenPosition position)
{
    GtkArrowType type = GTK_ARROW_UP;
    
    if (xfce_screen_position_is_floating (position))
    {
        type = calculate_floating_arrow_type (launcher, position);
    }
    else if (xfce_screen_position_is_top (position))
    {
        type = GTK_ARROW_DOWN;
    }
    else if (xfce_screen_position_is_left (position))
    {
        type = GTK_ARROW_RIGHT;
    }
    else if (xfce_screen_position_is_right (position))
    {
        type = GTK_ARROW_LEFT;
    }
    else if (xfce_screen_position_is_bottom (position))
    {
        type = GTK_ARROW_UP;
    }
    
    xfce_arrow_button_set_arrow_type (
            XFCE_ARROW_BUTTON (launcher->arrowbutton), type);
}

static void
launcher_set_orientation (XfcePanelPlugin *plugin,
                          LauncherPlugin *launcher, 
                          GtkOrientation orientation)
{
    GtkWidget *box;

    box = launcher->box;

    if (GTK_ORIENTATION_HORIZONTAL == orientation)
        launcher->box = gtk_hbox_new (FALSE, 0);
    else
        launcher->box = gtk_vbox_new (FALSE, 0);
    
    gtk_widget_show (launcher->box);

    gtk_widget_reparent (launcher->iconbutton, launcher->box);
    gtk_widget_reparent (launcher->arrowbutton, launcher->box);

    gtk_widget_destroy (box);

    gtk_container_add (GTK_CONTAINER (plugin), launcher->box);
}

/* Create Launcher Plugin Contents */

static void
plugin_realized (XfcePanelPlugin *plugin, LauncherPlugin *launcher)
{
    if (!GTK_WIDGET_REALIZED (launcher->iconbutton))
    {
        gtk_widget_realize (launcher->box);
        gtk_widget_realize (launcher->iconbutton);
    }

    launcher_set_screen_position (launcher, 
            xfce_panel_plugin_get_screen_position (plugin));
}

static LauncherPlugin *
launcher_new (XfcePanelPlugin *plugin)
{
    LauncherPlugin *launcher;
    int size;
    XfceScreenPosition screen_position;

    if (G_UNLIKELY (!icontheme))
        icontheme = xfce_icon_theme_get_for_screen (NULL);

    size = xfce_panel_plugin_get_size (plugin);
    screen_position = xfce_panel_plugin_get_screen_position (plugin);
    
    launcher = g_new0 (LauncherPlugin, 1);
    
    launcher->plugin = GTK_WIDGET (plugin);
    
    launcher->tips = gtk_tooltips_new ();
    g_object_ref (launcher->tips);
    gtk_object_sink (GTK_OBJECT (launcher->tips));
    
    launcher->entries = g_ptr_array_new ();
    
    if (xfce_screen_position_is_horizontal (screen_position))
        launcher->box = gtk_hbox_new (FALSE, 0);
    else
        launcher->box = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (launcher->box);

    launcher->iconbutton = xfce_iconbutton_new ();
    gtk_widget_show (launcher->iconbutton);
    gtk_box_pack_start (GTK_BOX (launcher->box), launcher->iconbutton,
                        TRUE, TRUE, 0);
    gtk_widget_set_size_request (launcher->iconbutton, size, size);
    gtk_button_set_relief (GTK_BUTTON (launcher->iconbutton), 
                           GTK_RELIEF_NONE);
    gtk_button_set_focus_on_click (GTK_BUTTON (launcher->iconbutton), FALSE);

    launcher->arrowbutton = xfce_arrow_button_new (GTK_ARROW_UP);
    gtk_box_pack_start (GTK_BOX (launcher->box), launcher->arrowbutton,
                        FALSE, FALSE, 0);
    gtk_widget_set_size_request (launcher->arrowbutton, W_ARROW, W_ARROW);
    gtk_button_set_relief (GTK_BUTTON (launcher->arrowbutton), 
                           GTK_RELIEF_NONE);
    gtk_button_set_focus_on_click (GTK_BUTTON (launcher->arrowbutton), FALSE);
    launcher_set_screen_position (launcher, screen_position);
    
    g_signal_connect (launcher->iconbutton, "button-press-event",
                      G_CALLBACK (launcher_toggle_menu_timeout), launcher);
    
    g_signal_connect (launcher->iconbutton, "clicked",
                      G_CALLBACK (launcher_clicked), launcher);
    
    g_signal_connect (launcher->arrowbutton, "button-press-event",
                      G_CALLBACK (launcher_toggle_menu), launcher);
    
    g_signal_connect (launcher->iconbutton, "state-changed",
                      G_CALLBACK (launcher_state_changed), 
                      launcher->arrowbutton);
    
    g_signal_connect (launcher->arrowbutton, "state-changed",
                      G_CALLBACK (launcher_state_changed), 
                      launcher->iconbutton);

    g_signal_connect (launcher->iconbutton, "destroy", 
                      G_CALLBACK (gtk_widget_destroyed), 
                      &(launcher->iconbutton));

    launcher_read_rc_file (plugin, launcher);

    if (launcher->entries->len == 0)
    {
        LauncherEntry *entry = entry = g_new0 (LauncherEntry, 1);

        entry->name = g_strdup (_("New item"));
        entry->comment = g_strdup (_("This item has not yet been configured"));

        g_ptr_array_add (launcher->entries, entry);

        g_idle_add ((GSourceFunc)update_panel_entry_idle, launcher);
    }
    else if (launcher->entries->len > 1)
    {
        gtk_widget_show (launcher->arrowbutton);
    }

    launcher_set_drag_dest (launcher->iconbutton);
    
    g_signal_connect (launcher->iconbutton, "drag-data-received",
                      G_CALLBACK (launcher_drag_data_received), launcher);

    launcher_set_drag_dest (launcher->arrowbutton);
    
    g_signal_connect (launcher->arrowbutton, "drag-motion",
                      G_CALLBACK (launcher_arrow_drag), launcher);
    
    g_signal_connect (launcher->arrowbutton, "drag-leave", 
                      G_CALLBACK (launcher_menu_drag_leave), launcher);

    g_signal_connect (plugin, "realize", G_CALLBACK (plugin_realized), 
                      launcher);
    
    return launcher;
}

/* Free Launcher Data */

static void
launcher_free (LauncherPlugin *launcher)
{
    int i;

    g_object_unref (launcher->tips);
    
    for (i = 0; i < launcher->entries->len; ++i)
    {
        LauncherEntry *e = g_ptr_array_index (launcher->entries, i);

        launcher_entry_free (e);
    }

    g_ptr_array_free (launcher->entries, TRUE);

    if (launcher->menu)
        gtk_widget_destroy (launcher->menu);
    
    g_free (launcher);
}

