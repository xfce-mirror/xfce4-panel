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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#define XFCE_TRAY_DIALOG_ICON_SIZE 22

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/xfce-titled-dialog.h>
#include <libxfcegui4/xfce-widget-helpers.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-macros.h>

#include "xfce-tray-manager.h"
#include "xfce-tray-widget.h"
#include "xfce-tray-plugin.h"
#include "xfce-tray-dialogs.h"



enum
{
    APPLICATION_ICON,
    APPLICATION_NAME,
    APPLICATION_HIDDEN,
    APPLICATION_DATA,
    N_COLUMNS
};



/* prototypes */
static gchar     *xfce_tray_dialogs_camel_case         (const gchar           *text);
static GdkPixbuf *xfce_tray_dialogs_icon               (GtkIconTheme          *icon_theme, 
                                                        const gchar           *name);
static void       xfce_tray_dialogs_show_frame_toggled (GtkToggleButton       *button,
                                                        XfceTrayPlugin        *plugin);
static void       xfce_tray_dialogs_treeview_toggled   (GtkCellRendererToggle *widget,
                                                        gchar                 *path,
                                                        GtkWidget             *treeview);
static void       xfce_tray_dialogs_free_store         (GtkListStore          *store);
static void       xfce_tray_dialogs_configure_response (GtkWidget             *dialog, 
                                                        gint                   response, 
                                                        XfceTrayPlugin        *plugin);



static gchar *
xfce_tray_dialogs_camel_case (const gchar *text)
{
    const gchar *t;
    gboolean     upper = TRUE;
    gunichar     c;
    GString     *result;

    /* allocate a new string for the result */
    result = g_string_sized_new (32);

    /* convert the input text */
    for (t = text; *t != '\0'; t = g_utf8_next_char (t))
    {
        /* check the next char */
        c = g_utf8_get_char (t);
        if (g_unichar_isspace (c))
        {
            upper = TRUE;
        }
        else if (upper)
        {
            c = g_unichar_toupper (c);
            upper = FALSE;
        }
        else
        {
           c = g_unichar_tolower (c);
        }

        /* append the char to the result */
        g_string_append_unichar (result, c);
    }

  return g_string_free (result, FALSE);
}



static GdkPixbuf *
xfce_tray_dialogs_icon (GtkIconTheme *icon_theme,
                        const gchar  *name)
{
    GdkPixbuf   *icon;
    guint        i;
    gchar       *first_occ;
    const gchar *p, *fallback[][2] = 
    {
        { "xfce-mcs-manager", "input-mouse" },
        { "bluetooth-applet", "stock_bluetooth" }
    };
    
    /* return null on no name */
    if (G_UNLIKELY (name == NULL))
        return NULL;
    
    /* try to load the icon from the theme */
    icon = gtk_icon_theme_load_icon (icon_theme, name, XFCE_TRAY_DIALOG_ICON_SIZE, 0, NULL);
    if (G_LIKELY (icon))
        return icon;
    
    /* try the first part when the name contains a space */
    p = g_utf8_strchr (name, -1, ' ');
    if (p)
    {
        /* get the string before the first occurrence */
        first_occ = g_strndup (name, p - name);
        
        /* try to load the icon from the theme */
        icon = gtk_icon_theme_load_icon (icon_theme, first_occ, XFCE_TRAY_DIALOG_ICON_SIZE, 0, NULL);
            
        /* cleanup */
        g_free (first_occ);
        
        if (icon)
            return icon;
    }
        
    /* find and return a fall-back icon */
    for (i = 0; i < G_N_ELEMENTS (fallback); i++)
        if (strcmp (name, fallback[i][0]) == 0)
            return gtk_icon_theme_load_icon (icon_theme, fallback[i][1], XFCE_TRAY_DIALOG_ICON_SIZE, 0, NULL);
    
    return NULL;
}



static void       
xfce_tray_dialogs_show_frame_toggled (GtkToggleButton *button,
                                      XfceTrayPlugin  *plugin)
{
    gboolean active;
    
    /* get state */
    active = gtk_toggle_button_get_active (button);
    
    /* set frame shadow */
    gtk_frame_set_shadow_type (GTK_FRAME (plugin->frame), active ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
    
    /* save */
    plugin->show_frame = active;
}



static void
xfce_tray_dialogs_treeview_toggled (GtkCellRendererToggle *widget,
                                    gchar                 *path,
                                    GtkWidget             *treeview)
{
    GtkTreeModel        *model;
    GtkTreeIter          iter;
    XfceTrayApplication *application;
    XfceTrayPlugin      *plugin;
    
    /* get the tree model */
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
    
    /* get the tree iter */
    if (G_LIKELY (gtk_tree_model_get_iter_from_string (model, &iter, path)))
    {
        /* get the application data */
        gtk_tree_model_get (model, &iter, APPLICATION_DATA, &application, -1);
        
        /* get tray plugin */
        plugin = g_object_get_data (G_OBJECT (treeview), I_("xfce-tray-plugin"));
        
        if (G_LIKELY (application && plugin))
        {
           /* update the manager */
           xfce_tray_manager_application_update (plugin->manager, application->name, !application->hidden);
    
           /* sort and update the tray widget */
           xfce_tray_widget_sort (XFCE_TRAY_WIDGET (plugin->tray));
    
           /* set the new list value */
           gtk_list_store_set (GTK_LIST_STORE (model), &iter, APPLICATION_HIDDEN, application->hidden, -1);
        }
    }
}



static void
xfce_tray_dialogs_free_store (GtkListStore *store)
{
    /* clear store */
    gtk_list_store_clear (store);
    
    /* release the store */
    g_object_unref (G_OBJECT (store));
}



static void
xfce_tray_dialogs_configure_response (GtkWidget      *dialog, 
                                      gint            response, 
                                      XfceTrayPlugin *plugin)
{
    /* destroy dialog */
    gtk_widget_destroy (dialog);
    
    /* unblock plugin menu */
    xfce_panel_plugin_unblock_menu (plugin->panel_plugin);
}



void 
xfce_tray_dialogs_configure (XfceTrayPlugin *plugin)
{
    GtkWidget           *dialog, *dialog_vbox;
    GtkWidget           *frame, *bin, *button;
    GtkWidget           *scroll, *treeview;
    GtkListStore        *store;
    GtkTreeViewColumn   *column;
    GtkCellRenderer     *renderer;
    GtkTreeIter          iter;
    XfceTrayApplication *application;
    GSList              *applications, *li;
    gchar               *name;
    GtkIconTheme        *icon_theme;
    GdkPixbuf           *pixbuf;
    
    /* lock plugin menu */
    xfce_panel_plugin_block_menu (plugin->panel_plugin);
    
    /* create dialog */
    dialog = xfce_titled_dialog_new_with_buttons (_("System Tray"),
                                                  NULL,
                                                  GTK_DIALOG_NO_SEPARATOR,
                                                  GTK_STOCK_OK, GTK_RESPONSE_OK,
                                                  NULL);
    gtk_window_set_screen (GTK_WINDOW (dialog), gtk_widget_get_screen (GTK_WIDGET (plugin->panel_plugin)));
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-settings");
    gtk_window_set_default_size (GTK_WINDOW (dialog), 280, 350);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
    g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (xfce_tray_dialogs_configure_response), plugin);
    
    dialog_vbox = GTK_DIALOG (dialog)->vbox;
    
    /* appearance */
    frame = xfce_create_framebox (_("Appearance"), &bin);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
    gtk_widget_show (frame);
    
    /* show frame */
    button = gtk_check_button_new_with_mnemonic (_("Show _frame"));
    gtk_container_add (GTK_CONTAINER (bin), button);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), plugin->show_frame);
    g_signal_connect (button, "toggled", G_CALLBACK (xfce_tray_dialogs_show_frame_toggled), plugin);
    gtk_widget_show (button);
    
    /* applications */
    frame = xfce_create_framebox (_("Hidden Applications"), &bin);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
    gtk_widget_show (frame);
    
    /* scrolled window */
    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);
    gtk_container_add (GTK_CONTAINER (bin), scroll);
    gtk_widget_show (scroll);
    
    /* create list store */
    store = gtk_list_store_new (N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_POINTER);
    
    /* create treeview */
    treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview), APPLICATION_NAME);
    gtk_tree_view_set_enable_search (GTK_TREE_VIEW (treeview), TRUE);
    gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (treeview), TRUE);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
    g_signal_connect_swapped (G_OBJECT (treeview), "destroy", G_CALLBACK (xfce_tray_dialogs_free_store), store);
    gtk_container_add (GTK_CONTAINER (scroll), treeview);
    gtk_widget_show (treeview);
    
    /* connect the plugin to the treeview */
    g_object_set_data (G_OBJECT (treeview), I_("xfce-tray-plugin"), plugin);
    
    /* create column */
    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_spacing (column, 2);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_column_set_resizable (column, FALSE);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    /* renderer for the icon */
    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_cell_renderer_set_fixed_size (renderer, XFCE_TRAY_DIALOG_ICON_SIZE, XFCE_TRAY_DIALOG_ICON_SIZE);
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "pixbuf", APPLICATION_ICON, NULL);
    
    /* renderer for the name */
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_set_attributes (column, renderer, "text", APPLICATION_NAME, NULL);
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
    
    /* renderer for the toggle button */
    renderer = gtk_cell_renderer_toggle_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "active", APPLICATION_HIDDEN, NULL);
    g_signal_connect (G_OBJECT (renderer), "toggled", G_CALLBACK(xfce_tray_dialogs_treeview_toggled), treeview);
    
    /* get the icon theme */
    if (G_LIKELY (gtk_widget_has_screen (dialog)))
        icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (dialog));
    else
        icon_theme = gtk_icon_theme_get_default ();
    
    /* get the sorted list of applications */
    applications = xfce_tray_manager_application_list (plugin->manager, TRUE);
    
    /* add all the application to the list */
    for (li = applications; li != NULL; li = li->next)
    {
        application = li->data;
        
        /* create a camel case name of the application */
        name = xfce_tray_dialogs_camel_case (application->name);
        
        /* append the application */
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            APPLICATION_NAME, name,
                            APPLICATION_HIDDEN, application->hidden,
                            APPLICATION_DATA, application, -1);
                            
        /* cleanup */
        g_free (name);
        
        /* get the application icon */
        pixbuf = xfce_tray_dialogs_icon (icon_theme, application->name);
                            
        if (pixbuf)
        {
            /* set the icon */
            gtk_list_store_set (store, &iter, APPLICATION_ICON, pixbuf, -1);
            
            /* release */
            g_object_unref (G_OBJECT (pixbuf));
        }
    }
    
    /* show the dialog */
    gtk_widget_show (dialog);
}
