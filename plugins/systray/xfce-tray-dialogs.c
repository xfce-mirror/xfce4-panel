/* $Id: xfce-tray-dialogs.c 26462 2007-12-12 12:00:59Z nick $ */
/*
 * Copyright (c) 2007 Nick Schermer <nick@xfce.org>
 *
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
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>

#include "xfce-tray-manager.h"
#include "xfce-tray-widget.h"
#include "xfce-tray-plugin.h"
#include "xfce-tray-dialogs.h"



enum
{
    APPLICATION_ICON,
    APPLICATION_NAME,
    APPLICATION_HIDDEN,
    N_COLUMNS
};



/* prototypes */
static gchar     *xfce_tray_dialogs_camel_case         (const gchar           *text);
static GdkPixbuf *xfce_tray_dialogs_icon               (GtkIconTheme          *icon_theme,
                                                        const gchar           *name);
static void       xfce_tray_dialogs_show_frame_toggled (GtkToggleButton       *button,
                                                        XfceTrayPlugin        *plugin);
static void       xfce_tray_dialogs_n_rows_changed     (GtkSpinButton         *button,
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
    const gchar *p;
    const gchar *fallback[][2] =
    {
        /* application name , fallback icon name or path */
        { "xfce-mcs-manager",      "input-mouse" },
        { "bluetooth-applet",      "stock_bluetooth" },
        { "gdl_box",               "/opt/google/desktop/resource/gdl_small.png" },
        { "networkmanager applet", "network-workgroup" },
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
        {
            if (g_path_is_absolute (fallback[i][1]))
            {
               return gdk_pixbuf_new_from_file_at_size (fallback[i][1], XFCE_TRAY_DIALOG_ICON_SIZE,
                                                        XFCE_TRAY_DIALOG_ICON_SIZE, NULL);
            }
            else
            {
               return gtk_icon_theme_load_icon (icon_theme, fallback[i][1], XFCE_TRAY_DIALOG_ICON_SIZE,
                                                0, NULL);
            }
        }

    return NULL;
}



static void
xfce_tray_dialogs_show_frame_toggled (GtkToggleButton *button,
                                      XfceTrayPlugin  *plugin)
{
    gboolean active;
    gint     panel_size;

    /* get state */
    active = gtk_toggle_button_get_active (button);

    /* set frame shadow */
    gtk_frame_set_shadow_type (GTK_FRAME (plugin->frame), active ? GTK_SHADOW_IN : GTK_SHADOW_NONE);

    /* save */
    plugin->show_frame = active;

    /* get the panel size */
    panel_size = xfce_panel_plugin_get_size (plugin->panel_plugin);

    /* emit size-changed signal */
    g_signal_emit_by_name (G_OBJECT (plugin->panel_plugin), "size-changed", panel_size, &active);
}



static void
xfce_tray_dialogs_n_rows_changed (GtkSpinButton  *button,
                                  XfceTrayPlugin *plugin)
{
   gint value;

   /* get value */
   value = gtk_spin_button_get_value_as_int (button);

   /* set rows */
   xfce_tray_widget_set_rows (XFCE_TRAY_WIDGET (plugin->tray), value);
}



static void
xfce_tray_dialogs_treeview_toggled (GtkCellRendererToggle *widget,
                                    gchar                 *path,
                                    GtkWidget             *treeview)
{
    GtkTreeModel   *model;
    GtkTreeIter     iter;
    gchar          *tmp, *name;
    gboolean        hidden;
    XfceTrayPlugin *plugin;

    /* get the tree model */
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));

    /* get the tree iter */
    if (G_LIKELY (gtk_tree_model_get_iter_from_string (model, &iter, path)))
    {
        /* get the lower case application name and hidden state */
        gtk_tree_model_get (model, &iter, APPLICATION_NAME, &tmp, APPLICATION_HIDDEN, &hidden, -1);
        name = g_utf8_strdown (tmp, -1);
        g_free (tmp);

        /* get tray plugin */
        plugin = g_object_get_data (G_OBJECT (treeview), I_("xfce-tray-plugin"));

        if (G_LIKELY (name && plugin))
        {
           /* update the manager */
           xfce_tray_widget_name_update (XFCE_TRAY_WIDGET (plugin->tray), name, !hidden);

           /* set the new list value */
           gtk_list_store_set (GTK_LIST_STORE (model), &iter, APPLICATION_HIDDEN, !hidden, -1);
        }

        /* cleanup */
        g_free (name);
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
    GtkWidget    *question;
    GtkListStore *store;

    if (response == GTK_RESPONSE_YES)
    {
        /* create question dialog, with hig buttons */
        question = gtk_message_dialog_new (GTK_WINDOW (dialog), GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
                                           GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
                                           _("Are you sure you want to clear the list of known applications?"));
        gtk_dialog_add_buttons (GTK_DIALOG (question), GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
                                GTK_STOCK_CLEAR, GTK_RESPONSE_YES, NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (question), GTK_RESPONSE_NO);

        /* run dialog */
        if (gtk_dialog_run (GTK_DIALOG (question)) == GTK_RESPONSE_YES)
        {
           /* clear hash table and update tray */
           xfce_tray_widget_clear_name_list (XFCE_TRAY_WIDGET (plugin->tray));

           /* clear store */
           store = g_object_get_data (G_OBJECT (dialog), I_("xfce-tray-store"));
           gtk_list_store_clear (store);
        }

       /* destroy */
       gtk_widget_destroy (question);
    }
    else
    {
        /* destroy dialog */
        gtk_widget_destroy (dialog);

        /* unblock plugin menu */
        xfce_panel_plugin_unblock_menu (plugin->panel_plugin);
    }
}



void
xfce_tray_dialogs_configure (XfceTrayPlugin *plugin)
{
    GtkWidget           *dialog, *dialog_vbox;
    GtkWidget           *frame, *bin, *button;
    GtkWidget           *scroll, *treeview;
    GtkWidget           *vbox, *label, *hbox, *spin;
    GtkListStore        *store;
    GtkTreeViewColumn   *column;
    GtkCellRenderer     *renderer;
    GtkTreeIter          iter;
    GList               *names, *li;
    const gchar         *name;
    gchar               *camelcase;
    gboolean             hidden;
    GtkIconTheme        *icon_theme;
    GdkPixbuf           *pixbuf;

    /* lock plugin menu */
    xfce_panel_plugin_block_menu (plugin->panel_plugin);

    /* create dialog */
    dialog = xfce_titled_dialog_new_with_buttons (_("System Tray"),
                                                  NULL,
                                                  GTK_DIALOG_NO_SEPARATOR,
                                                  GTK_STOCK_CLEAR, GTK_RESPONSE_YES,
                                                  GTK_STOCK_OK, GTK_RESPONSE_OK,
                                                  NULL);
    gtk_window_set_screen (GTK_WINDOW (dialog), gtk_widget_get_screen (GTK_WIDGET (plugin->panel_plugin)));
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-settings");
    gtk_window_set_default_size (GTK_WINDOW (dialog), 280, 400);
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
    g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (xfce_tray_dialogs_configure_response), plugin);

    /* main vbox */
    dialog_vbox = GTK_DIALOG (dialog)->vbox;

    /* appearance */
    frame = xfce_gtk_frame_box_new (_("Appearance"), &bin);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
    gtk_widget_show (frame);

    /* vbox */
    vbox = gtk_vbox_new (FALSE, 6);
    gtk_container_add (GTK_CONTAINER (bin), vbox);
    gtk_widget_show (vbox);

    /* show frame */
    button = gtk_check_button_new_with_mnemonic (_("Show _frame"));
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), plugin->show_frame);
    g_signal_connect (button, "toggled", G_CALLBACK (xfce_tray_dialogs_show_frame_toggled), plugin);
    gtk_widget_show (button);

    /* box */
    hbox = gtk_hbox_new (FALSE, 12);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    /* number of rows */
    label = gtk_label_new_with_mnemonic (_("_Number of rows:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    /* spin */
    spin = gtk_spin_button_new_with_range (1, 6, 1);
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spin), 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), xfce_tray_widget_get_rows (XFCE_TRAY_WIDGET (plugin->tray)));
    g_signal_connect (G_OBJECT (spin), "value-changed", G_CALLBACK (xfce_tray_dialogs_n_rows_changed), plugin);
    gtk_box_pack_start (GTK_BOX (hbox), spin, FALSE, FALSE, 0);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), spin);
    gtk_widget_show (spin);

    /* applications */
    frame = xfce_gtk_frame_box_new (_("Hidden Applications"), &bin);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
    gtk_widget_show (frame);

    /* scrolled window */
    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);
    gtk_container_add (GTK_CONTAINER (bin), scroll);
    gtk_widget_show (scroll);

    /* create list store */
    store = gtk_list_store_new (N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_BOOLEAN);
    g_object_set_data (G_OBJECT (dialog), I_("xfce-tray-store"), store);

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
    names = xfce_tray_widget_name_list (XFCE_TRAY_WIDGET (plugin->tray));

    /* add all the application to the list */
    for (li = names; li != NULL; li = li->next)
    {
        name = li->data;

        /* create a camel case name of the application */
        camelcase = xfce_tray_dialogs_camel_case (name);

        /* whether this name is hidden */
        hidden = xfce_tray_widget_name_hidden (XFCE_TRAY_WIDGET (plugin->tray), name);

        /* append the application */
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            APPLICATION_NAME, camelcase,
                            APPLICATION_HIDDEN, hidden, -1);

        /* cleanup */
        g_free (camelcase);

        /* get the application icon */
        pixbuf = xfce_tray_dialogs_icon (icon_theme, name);

        if (G_LIKELY (pixbuf))
        {
            /* set the icon */
            gtk_list_store_set (store, &iter, APPLICATION_ICON, pixbuf, -1);

            /* release */
            g_object_unref (G_OBJECT (pixbuf));
        }
    }

    /* cleanup */
    g_list_free (names);

    /* show the dialog */
    gtk_widget_show (dialog);
}
