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
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4ui/libxfce4ui.h>

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
void xfce_clock_dialog_options (ClockPlugin *clock);



static void
xfce_clock_dialog_reload_settings (ClockPlugin *clock)
{
    if (G_LIKELY (clock->widget))
    {
        /* save the settings */
        xfce_clock_widget_update_settings (clock);

        /* make sure the plugin sets it's size again */
        gtk_widget_queue_resize (clock->widget);

        /* run a direct update */
        (clock->update) (clock->widget);

        /* reschedule the timeout */
        xfce_clock_widget_sync (clock);
    }
}



static void
xfce_clock_dialog_mode_changed (GtkComboBox *combo,
                                ClockPlugin *clock)
{
    /* set the new mode */
    clock->mode = gtk_combo_box_get_active (combo);

    /* update the plugin widget */
    if (G_LIKELY (clock->widget))
    {
        /* set the new clock mode */
        xfce_clock_widget_set_mode (clock);

        /* update the settings */
        xfce_clock_dialog_reload_settings (clock);

        /* update the plugin size */
        xfce_clock_plugin_set_size (clock, xfce_panel_plugin_get_size (clock->plugin));
    }

    /* update the dialog */
    xfce_clock_dialog_options (clock);
}



static void
xfce_clock_dialog_show_frame_toggled (GtkToggleButton *button,
                                      ClockPlugin     *clock)
{
    /* set frame mode */
    clock->show_frame = gtk_toggle_button_get_active (button);

    /* change frame shadow */
    gtk_frame_set_shadow_type (GTK_FRAME (clock->frame), clock->show_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
}



static void
xfce_clock_dialog_tooltip_format_changed (GtkComboBox *combo,
                                          ClockPlugin *clock)
{
    gint       index;
    GtkWidget *entry;

    /* stop running timeout */
    if (clock->tooltip_timeout_id)
    {
        g_source_remove (clock->tooltip_timeout_id);
        clock->tooltip_timeout_id = 0;
    }

    /* get index of selected item */
    index = gtk_combo_box_get_active (combo);

    /* get the custom entry */
    entry = g_object_get_data (G_OBJECT (combo), I_("entry"));

    /* set one of the default formats */
    if (index < (gint) G_N_ELEMENTS (tooltip_formats))
    {
        /* hide the entry */
        gtk_widget_hide (entry);

        /* cleanup old format */
        g_free (clock->tooltip_format);

        /* set new format */
        clock->tooltip_format = g_strdup (tooltip_formats[index]);

        /* restart the tooltip timeout */
        xfce_clock_tooltip_sync (clock);
    }
    else
    {
        /* set string */
        gtk_entry_set_text (GTK_ENTRY (entry), clock->tooltip_format);

        /* show */
        gtk_widget_show (entry);
    }
}



static void
xfce_clock_dialog_tooltip_entry_changed (GtkEntry    *entry,
                                         ClockPlugin *clock)
{
    /* cleanup old format */
    g_free (clock->tooltip_format);

    /* set new format */
    clock->tooltip_format = g_strdup (gtk_entry_get_text (entry));

    /* restart the tooltip timeout */
    xfce_clock_tooltip_sync (clock);
}



static void
xfce_clock_dialog_show_seconds_toggled (GtkToggleButton *button,
                                        ClockPlugin     *clock)
{
    /* whether we show seconds */
    clock->show_seconds = gtk_toggle_button_get_active (button);

    /* update the clock */
    xfce_clock_dialog_reload_settings (clock);
}



static void
xfce_clock_dialog_show_military_toggled (GtkToggleButton *button,
                                      ClockPlugin     *clock)
{
    /* whether we show a 24h clock */
    clock->show_military = gtk_toggle_button_get_active (button);

    /* update the clock */
    xfce_clock_dialog_reload_settings (clock);
}



static void
xfce_clock_dialog_show_meridiem_toggled (GtkToggleButton *button,
                                         ClockPlugin     *clock)
{
    /* whether we show am/pm */
    clock->show_meridiem = gtk_toggle_button_get_active (button);

    /* update the clock */
    xfce_clock_dialog_reload_settings (clock);
}



static void
xfce_clock_dialog_flash_separators_toggled (GtkToggleButton *button,
                                            ClockPlugin     *clock)
{
    /* whether flash the separators */
    clock->flash_separators = gtk_toggle_button_get_active (button);

    /* update the clock */
    xfce_clock_dialog_reload_settings (clock);
}



static void
xfce_clock_dialog_true_binary_toggled (GtkToggleButton *button,
                                       ClockPlugin     *clock)
{
    /* whether we this is a true binary clock */
    clock->true_binary = gtk_toggle_button_get_active (button);

    /* update the clock */
    xfce_clock_dialog_reload_settings (clock);
}



static void
xfce_clock_dialog_digital_format_changed (GtkComboBox *combo,
                                          ClockPlugin *clock)
{
    gint       index;
    GtkWidget *entry;

    /* get index of selected item */
    index = gtk_combo_box_get_active (combo);

    /* get the custom entry */
    entry = g_object_get_data (G_OBJECT (combo), I_("entry"));

    /* set one of the default formats */
    if (index < (gint) G_N_ELEMENTS (digital_formats))
    {
        /* hide the entry */
        gtk_widget_hide (entry);

        /* cleanup old format */
        g_free (clock->digital_format);

        /* set new format */
        clock->digital_format = g_strdup (digital_formats[index]);

        /* reload settings */
        xfce_clock_dialog_reload_settings (clock);
    }
    else
    {
        /* set string */
        gtk_entry_set_text (GTK_ENTRY (entry), clock->digital_format);

        /* show */
        gtk_widget_show (entry);
    }
}



static void
xfce_clock_dialog_digital_entry_changed (GtkEntry    *entry,
                                         ClockPlugin *clock)
{
    /* cleanup old format */
    g_free (clock->digital_format);

    /* set new format */
    clock->digital_format = g_strdup (gtk_entry_get_text (entry));

    /* reload settings */
    xfce_clock_dialog_reload_settings (clock);
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
                            ClockPlugin *clock)
{
    /* destroy the dialog */
    gtk_widget_destroy (dialog);

    /* remove links */
    g_object_set_data (G_OBJECT (clock->plugin), I_("configure-dialog"), NULL);
    g_object_set_data (G_OBJECT (clock->plugin), I_("configure-dialog-bin"), NULL);

    /* unblock the plugin menu */
    xfce_panel_plugin_unblock_menu (clock->plugin);
}



void
xfce_clock_dialog_options (ClockPlugin *clock)
{
    GtkWidget *button, *bin, *vbox, *combo, *entry;
    gboolean   has_active;

    /* get the frame bin */
    bin = g_object_get_data (G_OBJECT (clock->plugin), I_("configure-dialog-bin"));
    gtk_container_foreach (GTK_CONTAINER (bin), (GtkCallback) gtk_widget_destroy, NULL);

    /* main vbox */
    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_add (GTK_CONTAINER (bin), vbox);
    gtk_widget_show (vbox);

    if (clock->mode == XFCE_CLOCK_ANALOG || clock->mode == XFCE_CLOCK_BINARY || clock->mode == XFCE_CLOCK_LCD)
    {
        button = gtk_check_button_new_with_mnemonic (_("Display _seconds"));
        gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clock->show_seconds);
        g_signal_connect (button, "toggled", G_CALLBACK (xfce_clock_dialog_show_seconds_toggled), clock);
        gtk_widget_show (button);
    }

    if (clock->mode == XFCE_CLOCK_LCD)
    {
        button = gtk_check_button_new_with_mnemonic (_("Use 24-_hour clock"));
        gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clock->show_military);
        g_signal_connect (button, "toggled", G_CALLBACK (xfce_clock_dialog_show_military_toggled), clock);
        gtk_widget_show (button);

        button = gtk_check_button_new_with_mnemonic (_("Fl_ash time separators"));
        gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clock->flash_separators);
        g_signal_connect (button, "toggled", G_CALLBACK (xfce_clock_dialog_flash_separators_toggled), clock);
        gtk_widget_show (button);

        button = gtk_check_button_new_with_mnemonic (_("Sho_w AM/PM"));
        gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clock->show_meridiem);
        g_signal_connect (button, "toggled", G_CALLBACK (xfce_clock_dialog_show_meridiem_toggled), clock);
        gtk_widget_show (button);
    }

    if (clock->mode == XFCE_CLOCK_BINARY)
    {
        button = gtk_check_button_new_with_mnemonic (_("True _binary clock"));
        gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clock->true_binary);
        g_signal_connect (button, "toggled", G_CALLBACK (xfce_clock_dialog_true_binary_toggled), clock);
        gtk_widget_show (button);
    }

    if (clock->mode == XFCE_CLOCK_DIGITAL)
    {
        combo = gtk_combo_box_new_text ();
        gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);
        gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo), xfce_clock_dialog_row_separator_func, NULL, NULL);
        has_active = xfce_clock_dialog_append_combobox_formats (GTK_COMBO_BOX (combo), digital_formats, clock->digital_format);
        g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (xfce_clock_dialog_digital_format_changed), clock);
        gtk_widget_show (combo);

        entry = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (entry), BUFFER_SIZE - 1);
        gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);
        g_object_set_data (G_OBJECT (combo), I_("entry"), entry);
        if (!has_active)
        {
            gtk_widget_show (entry);
            gtk_entry_set_text (GTK_ENTRY (entry), clock->digital_format);
        }
        g_signal_connect (G_OBJECT (entry), "changed", G_CALLBACK (xfce_clock_dialog_digital_entry_changed), clock);
    }
}



void
xfce_clock_dialog_show (ClockPlugin *clock)
{
    GtkWidget *dialog, *dialog_vbox;
    GtkWidget *frame, *bin, *vbox, *combo;
    GtkWidget *button, *entry;
    gboolean   has_active;

    /* block the right-click menu */
    xfce_panel_plugin_block_menu (clock->plugin);

    /* create dialog */
    dialog = xfce_titled_dialog_new_with_buttons (_("Clock"), NULL,
                                                  GTK_DIALOG_NO_SEPARATOR,
                                                  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                                  NULL);
    gtk_window_set_screen (GTK_WINDOW (dialog), gtk_widget_get_screen (GTK_WIDGET (clock->plugin)));
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-settings");
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
    g_object_set_data (G_OBJECT (clock->plugin), I_("configure-dialog"), dialog);
    g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (xfce_clock_dialog_response), clock);

    /* main vbox */
    dialog_vbox = gtk_vbox_new (FALSE, 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), dialog_vbox, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (dialog_vbox), 6);
    gtk_widget_show (dialog_vbox);

    /* appearance settings */
    frame = xfce_gtk_frame_box_new (_("Appearance"), &bin);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, TRUE, 0);
    gtk_widget_show (frame);

    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_add (GTK_CONTAINER (bin), vbox);
    gtk_widget_show (vbox);

    combo = gtk_combo_box_new_text ();
    gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Analog"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Binary"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Digital"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("LCD"));
    gtk_combo_box_set_active (GTK_COMBO_BOX (combo), clock->mode);
    g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (xfce_clock_dialog_mode_changed), clock);
    gtk_widget_show (combo);

    button = gtk_check_button_new_with_mnemonic (_("Show _frame"));
    gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clock->show_frame);
    g_signal_connect (button, "toggled", G_CALLBACK (xfce_clock_dialog_show_frame_toggled), clock);
    gtk_widget_show (button);

    /* tooltip settings */
    frame = xfce_gtk_frame_box_new (_("Tooltip Format"), &bin);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, TRUE, 0);
    gtk_widget_show (frame);

    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_add (GTK_CONTAINER (bin), vbox);
    gtk_widget_show (vbox);

    combo = gtk_combo_box_new_text ();
    gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);
    gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo), xfce_clock_dialog_row_separator_func, NULL, NULL);
    has_active = xfce_clock_dialog_append_combobox_formats (GTK_COMBO_BOX (combo), tooltip_formats, clock->tooltip_format);
    g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (xfce_clock_dialog_tooltip_format_changed), clock);
    gtk_widget_show (combo);

    entry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (entry), BUFFER_SIZE - 1);
    gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);
    g_object_set_data (G_OBJECT (combo), I_("entry"), entry);
    if (!has_active)
    {
        gtk_widget_show (entry);
        gtk_entry_set_text (GTK_ENTRY (entry), clock->tooltip_format);
    }
    g_signal_connect (G_OBJECT (entry), "changed", G_CALLBACK (xfce_clock_dialog_tooltip_entry_changed), clock);

    /* clock settings */
    frame = xfce_gtk_frame_box_new (_("Clock Options"), &bin);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, TRUE, 0);
    g_object_set_data (G_OBJECT (clock->plugin), I_("configure-dialog-bin"), bin);
    gtk_widget_show (frame);

    /* add the potions */
    xfce_clock_dialog_options (clock);

    /* show the dialog */
    gtk_widget_show (dialog);
}

