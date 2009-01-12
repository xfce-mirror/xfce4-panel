/* $Id$ */
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

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfcegui4/xfce-titled-dialog.h>
#include <libxfcegui4/xfce-widget-helpers.h>

#include "clock.h"
#include "clock-dialog.h"



/** default formats **/
static const gchar *tooltip_formats[] = {
    DEFAULT_TOOLTIP_FORMAT,
    "%x",
    NULL
};

static const gchar *digital_formats[] = {
    DEFAULT_DIGITAL_FORMAT,
    "%T",
    "%r",
    "%R %p",
    NULL
};



/** prototypes **/
void xfce_clock_dialog_options (ClockPlugin *plugin);



static void
xfce_clock_dialog_reload_settings (ClockPlugin *plugin)
{
    if (G_LIKELY (plugin->widget))
    {
        /* save the settings */
        xfce_clock_widget_update_settings (plugin);

        /* make sure the plugin sets it's size again */
        gtk_widget_queue_resize (plugin->widget);

        /* run a direct update */
        (plugin->update) (plugin->widget);

        /* reschedule the timeout */
        xfce_clock_widget_sync (plugin);
    }
}



static void
xfce_clock_dialog_mode_changed (GtkComboBox *combo,
                                ClockPlugin *plugin)
{
    /* set the new mode */
    plugin->mode = gtk_combo_box_get_active (combo);

    /* update the plugin widget */
    if (G_LIKELY (plugin->widget))
    {
        /* set the new clock mode */
        xfce_clock_widget_set_mode (plugin);

        /* update the settings */
        xfce_clock_dialog_reload_settings (plugin);

        /* update the plugin size */
        xfce_clock_plugin_set_size (plugin, xfce_panel_plugin_get_size (plugin->plugin));
    }

    /* update the dialog */
    xfce_clock_dialog_options (plugin);
}



static void
xfce_clock_dialog_show_frame_toggled (GtkToggleButton *button,
                                      ClockPlugin     *plugin)
{
    /* set frame mode */
    plugin->show_frame = gtk_toggle_button_get_active (button);

    /* change frame shadow */
    gtk_frame_set_shadow_type (GTK_FRAME (plugin->frame), plugin->show_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
}



static void
xfce_clock_dialog_tooltip_format_changed (GtkComboBox *combo,
                                          ClockPlugin *plugin)
{
    gint       idx;
    GtkWidget *entry;

    /* stop running timeout */
    if (plugin->tooltip_timeout_id)
    {
        g_source_remove (plugin->tooltip_timeout_id);
        plugin->tooltip_timeout_id = 0;
    }

    /* get index of selected item */
    idx = gtk_combo_box_get_active (combo);

    /* get the custom entry */
    entry = g_object_get_data (G_OBJECT (combo), I_("entry"));

    /* set one of the default formats */
    if (idx < (gint) G_N_ELEMENTS (tooltip_formats))
    {
        /* hide the entry */
        gtk_widget_hide (entry);

        /* cleanup old format */
        g_free (plugin->tooltip_format);

        /* set new format */
        plugin->tooltip_format = g_strdup (tooltip_formats[idx]);

        /* restart the tooltip timeout */
        xfce_clock_tooltip_sync (plugin);
    }
    else
    {
        /* set string */
        gtk_entry_set_text (GTK_ENTRY (entry), plugin->tooltip_format);

        /* show */
        gtk_widget_show (entry);
    }
}



static void
xfce_clock_dialog_tooltip_entry_changed (GtkEntry    *entry,
                                         ClockPlugin *plugin)
{
    /* cleanup old format */
    g_free (plugin->tooltip_format);

    /* set new format */
    plugin->tooltip_format = g_strdup (gtk_entry_get_text (entry));

    /* restart the tooltip timeout */
    xfce_clock_tooltip_sync (plugin);
}



static void
xfce_clock_dialog_show_seconds_toggled (GtkToggleButton *button,
                                        ClockPlugin     *plugin)
{
    /* whether we show seconds */
    plugin->show_seconds = gtk_toggle_button_get_active (button);

    /* update the clock */
    xfce_clock_dialog_reload_settings (plugin);
}



static void
xfce_clock_dialog_show_military_toggled (GtkToggleButton *button,
                                         ClockPlugin     *plugin)
{
    /* whether we show a 24h clock */
    plugin->show_military = gtk_toggle_button_get_active (button);

    /* update the clock */
    xfce_clock_dialog_reload_settings (plugin);
}



static void
xfce_clock_dialog_show_meridiem_toggled (GtkToggleButton *button,
                                         ClockPlugin     *plugin)
{
    /* whether we show am/pm */
    plugin->show_meridiem = gtk_toggle_button_get_active (button);

    /* update the clock */
    xfce_clock_dialog_reload_settings (plugin);
}



static void
xfce_clock_dialog_flash_separators_toggled (GtkToggleButton *button,
                                            ClockPlugin     *plugin)
{
    /* whether flash the separators */
    plugin->flash_separators = gtk_toggle_button_get_active (button);

    /* update the clock */
    xfce_clock_dialog_reload_settings (plugin);
}



static void
xfce_clock_dialog_true_binary_toggled (GtkToggleButton *button,
                                       ClockPlugin     *plugin)
{
    /* whether we this is a true binary clock */
    plugin->true_binary = gtk_toggle_button_get_active (button);

    /* update the clock */
    xfce_clock_dialog_reload_settings (plugin);
}



static void
xfce_clock_dialog_digital_format_changed (GtkComboBox *combo,
                                          ClockPlugin *plugin)
{
    gint       idx;
    GtkWidget *entry;

    /* get index of selected item */
    idx = gtk_combo_box_get_active (combo);

    /* get the custom entry */
    entry = g_object_get_data (G_OBJECT (combo), I_("entry"));

    /* set one of the default formats */
    if (idx < (gint) G_N_ELEMENTS (digital_formats))
    {
        /* hide the entry */
        gtk_widget_hide (entry);

        /* cleanup old format */
        g_free (plugin->digital_format);

        /* set new format */
        plugin->digital_format = g_strdup (digital_formats[idx]);

        /* reload settings */
        xfce_clock_dialog_reload_settings (plugin);
    }
    else
    {
        /* set string */
        gtk_entry_set_text (GTK_ENTRY (entry), plugin->digital_format);

        /* show */
        gtk_widget_show (entry);
    }
}



static void
xfce_clock_dialog_digital_entry_changed (GtkEntry    *entry,
                                         ClockPlugin *plugin)
{
    /* cleanup old format */
    g_free (plugin->digital_format);

    /* set new format */
    plugin->digital_format = g_strdup (gtk_entry_get_text (entry));

    /* reload settings */
    xfce_clock_dialog_reload_settings (plugin);
}



static gboolean
xfce_clock_dialog_row_separator_func (GtkTreeModel *model,
                                      GtkTreeIter  *iter,
                                      gpointer      user_data)
{
    gchar    *title;
    gboolean  result = FALSE;

    gtk_tree_model_get (model, iter, 0, &title, -1);
    if (G_LIKELY (title != NULL))
    {
        result = (strcmp (title, "-") == 0);

        /* cleanup */
        g_free (title);
    }

    return result;
}



static gboolean
xfce_clock_dialog_append_combobox_formats (GtkComboBox *combo,
                                           const gchar *formats[],
                                           const gchar *current_format)
{
    gint       i;
    struct tm  tm;
    gchar     *string;
    gboolean   has_active = FALSE;

    /* get the local time */
    xfce_clock_util_get_localtime (&tm);

    for (i = 0; formats[i] != NULL; i++)
    {
        /* insert user-redable string */
        string = xfce_clock_util_strdup_strftime (formats[i], &tm);
        gtk_combo_box_append_text (GTK_COMBO_BOX (combo), string);
        g_free (string);

        /* check if this is the active string */
        if (!has_active && current_format && strcmp (formats[i], current_format) == 0)
        {
            gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
            has_active = TRUE;
        }
    }

    /* insert the separator and the custom line */
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "-");
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Custom"));

    if (!has_active)
    {
        if (!current_format)
        {
            has_active = TRUE;
            i = -1;
        }

        /* select item */
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i + 1);
    }

    return has_active;
}



static void
xfce_clock_dialog_response (GtkWidget   *dialog,
                            gint         response,
                            ClockPlugin *plugin)
{
    /* destroy the dialog */
    gtk_widget_destroy (dialog);

    /* remove links */
    g_object_set_data (G_OBJECT (plugin->plugin), I_("configure-dialog"), NULL);
    g_object_set_data (G_OBJECT (plugin->plugin), I_("configure-dialog-bin"), NULL);

    /* unblock the plugin menu */
    xfce_panel_plugin_unblock_menu (plugin->plugin);
}



void
xfce_clock_dialog_options (ClockPlugin *plugin)
{
    GtkWidget *button, *bin, *vbox, *combo, *entry;
    gboolean   has_active;

    /* get the frame bin */
    bin = g_object_get_data (G_OBJECT (plugin->plugin), I_("configure-dialog-bin"));
    gtk_container_foreach (GTK_CONTAINER (bin), (GtkCallback) gtk_widget_destroy, NULL);

    /* main vbox */
    vbox = gtk_vbox_new (FALSE, 6);
    gtk_container_add (GTK_CONTAINER (bin), vbox);
    gtk_widget_show (vbox);

    if (plugin->mode == XFCE_CLOCK_ANALOG || plugin->mode == XFCE_CLOCK_BINARY || plugin->mode == XFCE_CLOCK_LCD)
    {
        button = gtk_check_button_new_with_mnemonic (_("Display _seconds"));
        gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), plugin->show_seconds);
        g_signal_connect (button, "toggled", G_CALLBACK (xfce_clock_dialog_show_seconds_toggled), plugin);
        gtk_widget_show (button);
    }

    if (plugin->mode == XFCE_CLOCK_LCD)
    {
        button = gtk_check_button_new_with_mnemonic (_("Use 24-_hour clock"));
        gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), plugin->show_military);
        g_signal_connect (button, "toggled", G_CALLBACK (xfce_clock_dialog_show_military_toggled), plugin);
        gtk_widget_show (button);

        button = gtk_check_button_new_with_mnemonic (_("Fl_ash time separators"));
        gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), plugin->flash_separators);
        g_signal_connect (button, "toggled", G_CALLBACK (xfce_clock_dialog_flash_separators_toggled), plugin);
        gtk_widget_show (button);

        button = gtk_check_button_new_with_mnemonic (_("Sho_w AM/PM"));
        gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), plugin->show_meridiem);
        g_signal_connect (button, "toggled", G_CALLBACK (xfce_clock_dialog_show_meridiem_toggled), plugin);
        gtk_widget_show (button);
    }

    if (plugin->mode == XFCE_CLOCK_BINARY)
    {
        button = gtk_check_button_new_with_mnemonic (_("True _binary clock"));
        gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), plugin->true_binary);
        g_signal_connect (button, "toggled", G_CALLBACK (xfce_clock_dialog_true_binary_toggled), plugin);
        gtk_widget_show (button);
    }

    if (plugin->mode == XFCE_CLOCK_DIGITAL)
    {
        combo = gtk_combo_box_new_text ();
        gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);
        gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo), xfce_clock_dialog_row_separator_func, NULL, NULL);
        has_active = xfce_clock_dialog_append_combobox_formats (GTK_COMBO_BOX (combo), digital_formats, plugin->digital_format);
        g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (xfce_clock_dialog_digital_format_changed), plugin);
        gtk_widget_show (combo);

        entry = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (entry), BUFFER_SIZE - 1);
        gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);
        g_object_set_data (G_OBJECT (combo), I_("entry"), entry);
        if (!has_active)
        {
            gtk_widget_show (entry);
            gtk_entry_set_text (GTK_ENTRY (entry), plugin->digital_format);
        }
        g_signal_connect (G_OBJECT (entry), "changed", G_CALLBACK (xfce_clock_dialog_digital_entry_changed), plugin);
    }
}



void
xfce_clock_dialog_show (ClockPlugin *plugin)
{
    GtkWidget *dialog, *dialog_vbox;
    GtkWidget *frame, *bin, *vbox, *combo;
    GtkWidget *button, *entry;
    gboolean   has_active;

    /* block the right-click menu */
    xfce_panel_plugin_block_menu (plugin->plugin);

    dialog = xfce_titled_dialog_new_with_buttons (_("Clock"), NULL,
                                                  GTK_DIALOG_NO_SEPARATOR,
                                                  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                                  NULL);
    gtk_window_set_screen (GTK_WINDOW (dialog), gtk_widget_get_screen (GTK_WIDGET (plugin->plugin)));
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_PROPERTIES);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
    g_object_set_data (G_OBJECT (plugin->plugin), I_("configure-dialog"), dialog);
    g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (xfce_clock_dialog_response), plugin);

    /* main vbox */
    dialog_vbox = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), dialog_vbox, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (dialog_vbox), 6);
    gtk_widget_show (dialog_vbox);

    frame = xfce_create_framebox (_("Appearance"), &bin);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, TRUE, 0);
    gtk_widget_show (frame);

    vbox = gtk_vbox_new (FALSE, 6);
    gtk_container_add (GTK_CONTAINER (bin), vbox);
    gtk_widget_show (vbox);

    combo = gtk_combo_box_new_text ();
    gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Analog"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Binary"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Digital"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("LCD"));
    gtk_combo_box_set_active (GTK_COMBO_BOX (combo), plugin->mode);
    g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (xfce_clock_dialog_mode_changed), plugin);
    gtk_widget_show (combo);

    button = gtk_check_button_new_with_mnemonic (_("Show _frame"));
    gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), plugin->show_frame);
    g_signal_connect (button, "toggled", G_CALLBACK (xfce_clock_dialog_show_frame_toggled), plugin);
    gtk_widget_show (button);

    /* tooltip settings */
    frame = xfce_create_framebox (_("Tooltip Format"), &bin);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, TRUE, 0);
    gtk_widget_show (frame);

    vbox = gtk_vbox_new (FALSE, 6);
    gtk_container_add (GTK_CONTAINER (bin), vbox);
    gtk_widget_show (vbox);

    combo = gtk_combo_box_new_text ();
    gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);
    gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo), xfce_clock_dialog_row_separator_func, NULL, NULL);
    has_active = xfce_clock_dialog_append_combobox_formats (GTK_COMBO_BOX (combo), tooltip_formats, plugin->tooltip_format);
    g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (xfce_clock_dialog_tooltip_format_changed), plugin);
    gtk_widget_show (combo);

    entry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (entry), BUFFER_SIZE - 1);
    gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);
    g_object_set_data (G_OBJECT (combo), I_("entry"), entry);
    if (!has_active)
    {
        gtk_widget_show (entry);
        gtk_entry_set_text (GTK_ENTRY (entry), plugin->tooltip_format);
    }
    g_signal_connect (G_OBJECT (entry), "changed", G_CALLBACK (xfce_clock_dialog_tooltip_entry_changed), plugin);

    /* clock settings */
    frame = xfce_create_framebox (_("Clock Options"), &bin);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, TRUE, 0);
    g_object_set_data (G_OBJECT (plugin->plugin), I_("configure-dialog-bin"), bin);
    gtk_widget_show (frame);

    /* add the potions */
    xfce_clock_dialog_options (plugin);

    /* show the dialog */
    gtk_widget_show (dialog);
}

