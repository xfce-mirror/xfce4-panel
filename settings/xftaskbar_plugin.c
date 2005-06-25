/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *  
 *  Copyright (C) 2002-2004 Olivier Fourdan (fourdan@xfce.org)
 *  Copyright (c) 2004      Benedikt Meurer <benny@xfce.org>
 *  Copyright  ©  2005      Jasper Huijsmans <jasper@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#include <libxfce4mcs/mcs-common.h>
#include <libxfce4mcs/mcs-manager.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <xfce-mcs-manager/manager-plugin.h>

/* file locations */
#define RCDIR        "xfce4" G_DIR_SEPARATOR_S "mcs_settings"
#define OLDRCDIR     "settings"
#define CHANNEL      "taskbar"
#define RCFILE       "taskbar.xml"
#define PLUGIN_NAME  "taskbar"

/* default settings */
#define TOP                    TRUE
#define BOTTOM                 FALSE
#define DEFAULT_HEIGHT	       30
#define DEFAULT_WIDTH_PERCENT  100
#define DEFAULT_HORIZ_ALIGN    0

/* gui */
#define BORDER             6
#define DEFAULT_ICON_SIZE  48

/* prototypes */
static void create_channel(McsPlugin * mcs_plugin);
static gboolean write_options(McsPlugin * mcs_plugin);
static void run_dialog(McsPlugin * mcs_plugin);

/* global vars */
static gboolean is_running    = FALSE;
static int last_page          = 0;

static gboolean position      = TOP;
static gboolean shrink        = FALSE;
static int height             = DEFAULT_HEIGHT;
static int width_percent      = DEFAULT_WIDTH_PERCENT;
static int horiz_align        = DEFAULT_HORIZ_ALIGN;

static gboolean autohide      = FALSE;

static gboolean show_tasklist = TRUE;
static gboolean all_tasks     = FALSE;
static gboolean group_tasks   = FALSE;
static gboolean show_text     = TRUE;

static gboolean show_pager    = TRUE;

static gboolean show_tray     = TRUE;

static gboolean show_time     = TRUE;


/* dialog */
typedef struct _TaskbarDialog TaskbarDialog;

struct _TaskbarDialog
{
    McsPlugin *mcs_plugin;

    GtkWidget *dlg;
    GtkWidget *notebook;

    GtkWidget *pos_bottom_rb;
    GtkWidget *pos_top_rb;
    
    GtkWidget *height_scale;
    GtkWidget *width_scale;
    GtkWidget *width_labels[2];
    GtkWidget *shrink_check;

    GtkWidget *align_left_button;
    GtkWidget *align_center_button;
    GtkWidget *align_right_button;
    
    GtkWidget *tasklist_check;
    GtkWidget *tasklist_box;
    GtkWidget *alltasks_check;
    GtkWidget *grouptasks_check;
    GtkWidget *showtext_check;
    
    GtkWidget *autohide_check;
    
    GtkWidget *pager_check;

    GtkWidget *tray_check;

    GtkWidget *time_check;
};

/* utility functions */
static void
add_space (GtkBox *box, int size)
{
    GtkWidget *align = gtk_alignment_new (0,0,0,0);

    gtk_widget_show (align);
    gtk_widget_set_size_request (align, size, size);
    gtk_box_pack_start (box, align, FALSE, FALSE, 0);
}

/* main dialog */
static void
dialog_response (GtkWidget * dialog, gint response_id, TaskbarDialog *td)
{
    if (response_id == GTK_RESPONSE_HELP)
    {
        g_message ("HELP: TBD");
    }
    else
    {
        is_running = FALSE;
        last_page = 
            gtk_notebook_get_current_page (GTK_NOTEBOOK (td->notebook));
        gtk_widget_destroy (dialog);
        g_free (td);
    }
}

/* position */
static void
position_changed (GtkWidget * rb, gpointer user_data)
{
    TaskbarDialog *td = (TaskbarDialog *) user_data;
    McsPlugin *mcs_plugin = td->mcs_plugin;

    position =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (td->pos_top_rb));

    mcs_manager_set_int (mcs_plugin->manager, "Taskbar/Position", CHANNEL,
                         position ? 1 : 0);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

/* alignment */
static void
align_changed (GtkWidget * button, gpointer user_data)
{
    TaskbarDialog *td = (TaskbarDialog *) user_data;
    McsPlugin *mcs_plugin = td->mcs_plugin;

    if (gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON (td->align_left_button)))
    {
        horiz_align = -1;
    }
    else if (gtk_toggle_button_get_active
             (GTK_TOGGLE_BUTTON (td->align_center_button)))
    {
        horiz_align = 0;
    }
    else
    {
        horiz_align = 1;
    }

    mcs_manager_set_int (mcs_plugin->manager, "Taskbar/HorizAlign", CHANNEL,
                         horiz_align);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

/* autohide */
static void
autohide_changed (GtkWidget * dialog, gpointer user_data)
{
    TaskbarDialog *td = (TaskbarDialog *) user_data;
    McsPlugin *mcs_plugin = td->mcs_plugin;

    autohide =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (td->autohide_check));

    mcs_manager_set_int (mcs_plugin->manager, "Taskbar/AutoHide", CHANNEL,
                         autohide ? 1 : 0);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

static void
height_changed (GtkWidget * dialog, gpointer user_data)
{
    TaskbarDialog *td = (TaskbarDialog *) user_data;
    McsPlugin *mcs_plugin = td->mcs_plugin;

    height = (int) gtk_range_get_value (GTK_RANGE (td->height_scale));

    mcs_manager_set_int (mcs_plugin->manager, "Taskbar/Height", CHANNEL,
                         height);

    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

static void
width_percent_changed (GtkWidget * dialog, gpointer user_data)
{
    TaskbarDialog *td = (TaskbarDialog *) user_data;
    McsPlugin *mcs_plugin = td->mcs_plugin;

    width_percent = (int) gtk_range_get_value (GTK_RANGE (td->width_scale));

    mcs_manager_set_int (mcs_plugin->manager, "Taskbar/WidthPercent",
                         CHANNEL, width_percent);

    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

static void
shrink_changed (GtkWidget *cb, gpointer user_data)
{
    TaskbarDialog *td = (TaskbarDialog *) user_data;
    McsPlugin *mcs_plugin = td->mcs_plugin;

    shrink = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cb));
    gtk_widget_set_sensitive (td->width_scale, !shrink);
    gtk_widget_set_sensitive (td->width_labels[0], !shrink);
    gtk_widget_set_sensitive (td->width_labels[1], !shrink);

    mcs_manager_set_int (mcs_plugin->manager, "Taskbar/Shrink",
                         CHANNEL, shrink ? 1 : 0);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

/*tasklist */
static void
showtasklist_changed (GtkWidget * dialog, gpointer user_data)
{
    TaskbarDialog *td = (TaskbarDialog *) user_data;
    McsPlugin *mcs_plugin = td->mcs_plugin;

    show_tasklist =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (td->tasklist_check));

    gtk_widget_set_sensitive (td->tasklist_box, show_tasklist);
    
    mcs_manager_set_int (mcs_plugin->manager, "Taskbar/ShowTasklist", CHANNEL,
                         show_tasklist ? 1 : 0);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

static void
alltasks_changed (GtkWidget * dialog, gpointer user_data)
{
    TaskbarDialog *td = (TaskbarDialog *) user_data;
    McsPlugin *mcs_plugin = td->mcs_plugin;

    all_tasks =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (td->alltasks_check));

    mcs_manager_set_int (mcs_plugin->manager, "Taskbar/ShowAllTasks", CHANNEL,
                         all_tasks ? 1 : 0);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

static void
grouptasks_changed (GtkWidget * dialog, gpointer user_data)
{
    TaskbarDialog *td = (TaskbarDialog *) user_data;
    McsPlugin *mcs_plugin = td->mcs_plugin;

    group_tasks =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                      (td->grouptasks_check));

    mcs_manager_set_int (mcs_plugin->manager, "Taskbar/GroupTasks", CHANNEL,
                         group_tasks ? 1 : 0);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

static void
showtext_changed (GtkWidget * dialog, gpointer user_data)
{
    TaskbarDialog *td = (TaskbarDialog *) user_data;
    McsPlugin *mcs_plugin = td->mcs_plugin;

    show_text =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (td->showtext_check));

    mcs_manager_set_int (mcs_plugin->manager, "Taskbar/ShowText", CHANNEL,
                         show_text ? 1 : 0);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

/* pager */
static void
showpager_changed (GtkWidget * dialog, gpointer user_data)
{
    TaskbarDialog *td = (TaskbarDialog *) user_data;
    McsPlugin *mcs_plugin = td->mcs_plugin;

    show_pager =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (td->pager_check));

    mcs_manager_set_int (mcs_plugin->manager, "Taskbar/ShowPager", CHANNEL,
                         show_pager ? 1 : 0);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

/* status area */
static void
showtray_changed (GtkWidget * dialog, gpointer user_data)
{
    TaskbarDialog *td = (TaskbarDialog *) user_data;
    McsPlugin *mcs_plugin = td->mcs_plugin;

    show_tray =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (td->tray_check));

    mcs_manager_set_int (mcs_plugin->manager, "Taskbar/ShowTray", CHANNEL,
                         show_tray ? 1 : 0);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

static void
showtime_changed (GtkWidget * dialog, gpointer user_data)
{
    TaskbarDialog *td = (TaskbarDialog *) user_data;
    McsPlugin *mcs_plugin = td->mcs_plugin;

    show_time =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (td->time_check));

    mcs_manager_set_int (mcs_plugin->manager, "Taskbar/ShowTime", CHANNEL,
                         show_time ? 1 : 0);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);
    write_options (mcs_plugin);
}

/* general */
static void
add_general_options (GtkBox *box, TaskbarDialog *td)
{
    GtkWidget *label, *frame, *table;
    GtkRadioButton *rb;

    /* position */
    frame = xfce_framebox_new (_("Position"), TRUE);
    gtk_widget_show (frame);
    gtk_box_pack_start (box, frame, FALSE, FALSE, 0);

    table = gtk_table_new (2, 4, FALSE);
    gtk_table_set_col_spacing (GTK_TABLE (table), 0, BORDER);
    gtk_table_set_row_spacings (GTK_TABLE (table), BORDER);
    gtk_widget_show (table);
    xfce_framebox_add (XFCE_FRAMEBOX (frame), table);
    
    label = gtk_label_new (_("Position:"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                      GTK_FILL, 0, 0, 0);
    
    td->pos_top_rb = gtk_radio_button_new_with_mnemonic (NULL, _("_Top"));
    gtk_widget_show (td->pos_top_rb);
    gtk_table_attach (GTK_TABLE (table), td->pos_top_rb, 1, 2, 0, 1,
                      GTK_FILL, 0, 0, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (td->pos_top_rb),
                                  position);

    rb = GTK_RADIO_BUTTON (td->pos_top_rb);
    
    td->pos_bottom_rb =
        gtk_radio_button_new_with_mnemonic_from_widget (rb, _("_Bottom"));
    gtk_widget_show (td->pos_bottom_rb);
    gtk_table_attach (GTK_TABLE (table), td->pos_bottom_rb, 2, 3, 0, 1,
                      GTK_FILL, 0, 0, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (td->pos_bottom_rb),
                                  !position);

    g_signal_connect (td->pos_top_rb, "toggled", 
                      G_CALLBACK (position_changed), td);
    
    g_signal_connect (td->pos_bottom_rb, "toggled", 
                      G_CALLBACK (position_changed), td);

    label = gtk_label_new (_("Alignment:"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                      GTK_FILL, 0, 0, 0);
    
    td->align_left_button =
        gtk_radio_button_new_with_mnemonic (NULL, _("_Left"));
    gtk_widget_show (td->align_left_button);
    gtk_table_attach (GTK_TABLE (table), td->align_left_button, 1, 2, 1, 2,
                      GTK_FILL, 0, 0, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (td->align_left_button),
                                  horiz_align < 0);

    rb = GTK_RADIO_BUTTON (td->align_left_button);
    
    td->align_center_button =
        gtk_radio_button_new_with_mnemonic_from_widget (rb, _("_Center"));
    gtk_widget_show (td->align_center_button);
    gtk_table_attach (GTK_TABLE (table), td->align_center_button, 2, 3, 1, 2,
                      GTK_FILL, 0, 0, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (td->align_center_button),
                                  horiz_align == 0);

    td->align_right_button =
        gtk_radio_button_new_with_mnemonic_from_widget (rb, _("_Right"));
    gtk_widget_show (td->align_right_button);
    gtk_table_attach (GTK_TABLE (table), td->align_right_button, 3, 4, 1, 2,
                      GTK_FILL, 0, 0, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (td->align_right_button),
                                  horiz_align > 0);

    g_signal_connect (td->align_left_button, "toggled",
                      G_CALLBACK (align_changed), td);
    
    g_signal_connect (td->align_center_button, "toggled",
                      G_CALLBACK (align_changed), td);
    
    g_signal_connect (td->align_right_button, "toggled",
                      G_CALLBACK (align_changed), td);
    
    /* size */
    frame = xfce_framebox_new (_("Size"), TRUE);
    gtk_widget_show (frame);
    gtk_box_pack_start (box, frame, FALSE, FALSE, 0);

    table = gtk_table_new (3, 4, FALSE);
    gtk_widget_show (table);
    gtk_table_set_col_spacing (GTK_TABLE (table), 0, BORDER);
    gtk_table_set_row_spacings (GTK_TABLE (table), BORDER);
    xfce_framebox_add (XFCE_FRAMEBOX (frame), table);

    label = gtk_label_new (_("Height:"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                      GTK_FILL, 0, 0, 0);

    label = xfce_create_small_label (_("Small"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
                      GTK_FILL, 0, 2, 0);

    td->height_scale =
        gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (height, 
                                                            28, 60, 
                                                            10, 10, 10)));
    gtk_widget_show (td->height_scale);
    gtk_widget_set_size_request (td->height_scale, 120, -1);
    gtk_scale_set_draw_value (GTK_SCALE (td->height_scale), FALSE);
    gtk_scale_set_digits (GTK_SCALE (td->height_scale), 0);
    gtk_range_set_update_policy (GTK_RANGE (td->height_scale),
                                 GTK_UPDATE_DISCONTINUOUS);
    gtk_table_attach (GTK_TABLE (table), td->height_scale, 2, 3, 0, 1,
                      GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = xfce_create_small_label (_("Large"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 3, 4, 0, 1,
                      GTK_FILL, 0, 2, 0);

    label = gtk_label_new (_("Width:"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                      GTK_FILL, 0, 0, 0);

    label = xfce_create_small_label (_("Small"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2,
                      GTK_FILL, 0, 2, 0);
    
    td->width_labels[0] = label;

    td->width_scale =
        gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (width_percent, 
                                                            20, 110, 
                                                            10, 10, 10)));
    gtk_widget_show (td->width_scale);
    gtk_widget_set_size_request (td->width_scale, 120, -1);
    gtk_scale_set_draw_value (GTK_SCALE (td->width_scale), FALSE);
    gtk_scale_set_digits (GTK_SCALE (td->width_scale), 0);
    gtk_range_set_update_policy (GTK_RANGE (td->width_scale),
                                 GTK_UPDATE_DISCONTINUOUS);
    gtk_table_attach (GTK_TABLE (table), td->width_scale, 2, 3, 1, 2,
                      GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = xfce_create_small_label (_("Large"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 3, 4, 1, 2,
                      GTK_FILL, 0, 2, 0);

    td->width_labels[1] = label;

    td->shrink_check = 
        gtk_check_button_new_with_mnemonic (_("_Shrink to fit"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (td->shrink_check), 
                                  shrink);
    gtk_widget_show (td->shrink_check);
    gtk_table_attach (GTK_TABLE (table), td->shrink_check, 1, 4, 2, 3,
                      GTK_FILL, 0, 0, 0);
    
    if (shrink)
        gtk_widget_set_sensitive (td->width_scale, FALSE);

    g_signal_connect (td->height_scale, "value_changed", 
                      G_CALLBACK (height_changed), td);

    g_signal_connect (td->width_scale, "value_changed", 
                      G_CALLBACK (width_percent_changed), td);

    g_signal_connect (td->shrink_check, "toggled", 
                      G_CALLBACK (shrink_changed), td);

    /* Autohide */
    frame = xfce_framebox_new (_("Autohide"), TRUE);
    gtk_widget_show (frame);
    gtk_box_pack_start (box, frame, FALSE, FALSE, 0);

    td->autohide_check =
        gtk_check_button_new_with_mnemonic (_("Auto _hide taskbar"));
    gtk_widget_show (td->autohide_check);
    xfce_framebox_add (XFCE_FRAMEBOX (frame), td->autohide_check);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (td->autohide_check),
                                  autohide);

    g_signal_connect (td->autohide_check, "toggled",
                      G_CALLBACK (autohide_changed), td);
}

/* components */
static void
add_component_options (GtkBox *box, TaskbarDialog *td)
{
    GtkWidget *frame, *vbox;

    /* tasklist */
    frame = xfce_framebox_new (_("Tasklist"), TRUE);
    gtk_widget_show (frame);
    gtk_box_pack_start (box, frame, FALSE, FALSE, 0);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    xfce_framebox_add (XFCE_FRAMEBOX (frame), vbox);

    td->tasklist_check =
        gtk_check_button_new_with_mnemonic (_("Show tasklist"));
    gtk_widget_show (td->tasklist_check);
    gtk_box_pack_start (GTK_BOX (vbox), td->tasklist_check, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (td->tasklist_check), 
                                  show_tasklist);

    td->tasklist_box = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (td->tasklist_box);
    gtk_box_pack_start (GTK_BOX (vbox), td->tasklist_box, FALSE, FALSE, 0);
    gtk_widget_set_sensitive (td->tasklist_box, show_tasklist);
    
    td->alltasks_check =
        gtk_check_button_new_with_mnemonic (_("Show tasks "
                                              "from _all workspaces"));
    gtk_widget_show (td->alltasks_check);
    gtk_box_pack_start (GTK_BOX (td->tasklist_box), td->alltasks_check,
                        FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (td->alltasks_check),
                                  all_tasks);

    td->grouptasks_check =
        gtk_check_button_new_with_mnemonic (_("Always _group tasks"));
    gtk_widget_show (td->grouptasks_check);
    gtk_box_pack_start (GTK_BOX (td->tasklist_box), td->grouptasks_check, 
                        FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (td->grouptasks_check), 
                                  group_tasks);

    td->showtext_check =
        gtk_check_button_new_with_mnemonic (_("Show application _names"));
    gtk_widget_show (td->showtext_check);
    gtk_box_pack_start (GTK_BOX (td->tasklist_box), td->showtext_check,
                        FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (td->showtext_check),
                                  show_text);

    /* pager */
    frame = xfce_framebox_new (_("Pager"), TRUE);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (box), frame, FALSE, FALSE, 0);

    td->pager_check =
        gtk_check_button_new_with_mnemonic (_("Show _pager in taskbar"));
    gtk_widget_show (td->pager_check);
    xfce_framebox_add (XFCE_FRAMEBOX (frame), td->pager_check);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (td->pager_check),
                                  show_pager);

    /* status area */
    frame = xfce_framebox_new (_("Status area"), TRUE);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (box), frame, FALSE, FALSE, 0);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    xfce_framebox_add (XFCE_FRAMEBOX (frame), vbox);

    td->tray_check =
        gtk_check_button_new_with_mnemonic (_ ("Show _notification icons"));
    gtk_widget_show (td->tray_check);
    gtk_box_pack_start (GTK_BOX (vbox), td->tray_check, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (td->tray_check),
                                  show_tray);

    td->time_check =
        gtk_check_button_new_with_mnemonic (_ ("Show _time"));
    gtk_widget_show (td->time_check);
    gtk_box_pack_start (GTK_BOX (vbox), td->time_check, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (td->time_check),
                                  show_time);

    g_signal_connect (td->tasklist_check, "toggled",
                      G_CALLBACK (showtasklist_changed), td);

    g_signal_connect (td->alltasks_check, "toggled",
                      G_CALLBACK (alltasks_changed), td);

    g_signal_connect (td->grouptasks_check, "toggled",
                      G_CALLBACK (grouptasks_changed), td);

    g_signal_connect (td->showtext_check, "toggled",
                      G_CALLBACK (showtext_changed), td);

    g_signal_connect (td->pager_check, "toggled",
                      G_CALLBACK (showpager_changed), td);

    g_signal_connect (td->tray_check, "toggled",
                      G_CALLBACK (showtray_changed), td);

    g_signal_connect (td->time_check, "toggled",
                      G_CALLBACK (showtime_changed), td);
}

/* main dialog */
static void
create_dialog (McsPlugin * mcs_plugin)
{
    TaskbarDialog *dialog;
    GtkWidget *vbox, *header, *label, *vbox2;

    dialog = g_new0 (TaskbarDialog, 1);

    dialog->mcs_plugin = mcs_plugin;

    dialog->dlg = gtk_dialog_new_with_buttons (_("Taskbar"), NULL,
                                               GTK_DIALOG_NO_SEPARATOR,
                                               GTK_STOCK_CLOSE,
                                               GTK_RESPONSE_OK,
                                               NULL);

    gtk_window_set_icon (GTK_WINDOW (dialog->dlg), mcs_plugin->icon);

    vbox = GTK_DIALOG (dialog->dlg)->vbox;
    
    header = xfce_create_header (mcs_plugin->icon, _("Taskbar"));
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (vbox), header, FALSE, TRUE, 0);

    add_space (GTK_BOX (vbox), BORDER);

    dialog->notebook = gtk_notebook_new ();
    gtk_container_set_border_width (GTK_CONTAINER (dialog->notebook), 
                                    BORDER - 1);
    gtk_widget_show (dialog->notebook);
    gtk_box_pack_start (GTK_BOX (vbox), dialog->notebook, TRUE, TRUE, 0);
    
    label = gtk_label_new (_("General"));
    gtk_widget_show (label);
    
    vbox2 = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (vbox2), BORDER);
    gtk_widget_show (vbox2);
    
    gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook), vbox2, label);
    
    add_general_options (GTK_BOX (vbox2), dialog);
    
    label = gtk_label_new (_("Components"));
    gtk_widget_show (label);
    
    vbox2 = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (vbox2), BORDER);
    gtk_widget_show (vbox2);
    
    gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook), vbox2, label);
    
    add_component_options (GTK_BOX (vbox2), dialog);
    
    gtk_notebook_set_current_page (GTK_NOTEBOOK (dialog->notebook), last_page);
    
    g_signal_connect (dialog->dlg, "response", 
                      G_CALLBACK (dialog_response), dialog);
    
    xfce_gtk_window_center_on_monitor_with_pointer (GTK_WINDOW (dialog->dlg));
    gtk_widget_show (dialog->dlg);
}

/* run dialog */
static void
run_dialog (McsPlugin * mcs_plugin)
{
    if (is_running)
        return;

    is_running = TRUE;

    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    create_dialog (mcs_plugin);
}

/* mcs channel */
static gboolean
write_options (McsPlugin * mcs_plugin)
{
    gchar *rcfile;
    gboolean result = FALSE;

    rcfile = xfce_resource_save_location (XFCE_RESOURCE_CONFIG,
                                          RCDIR G_DIR_SEPARATOR_S RCFILE,
                                          TRUE);
    if (G_LIKELY (rcfile != NULL))
    {
        result = mcs_manager_save_channel_to_file (mcs_plugin->manager, CHANNEL, rcfile);
        g_free (rcfile);
    }

    return result;
}

static void
create_channel (McsPlugin * mcs_plugin)
{
    McsSetting *setting;
    gchar *rcfile;

    rcfile = xfce_resource_lookup (XFCE_RESOURCE_CONFIG,
                                   RCDIR G_DIR_SEPARATOR_S RCFILE);

    if (!rcfile)
        rcfile = xfce_get_userfile (OLDRCDIR, RCFILE, NULL);

    if (g_file_test (rcfile, G_FILE_TEST_EXISTS))
        mcs_manager_add_channel_from_file (mcs_plugin->manager, CHANNEL,
                                           rcfile);
    else
        mcs_manager_add_channel (mcs_plugin->manager, CHANNEL);

    g_free (rcfile);

    /* position */
    setting =
        mcs_manager_setting_lookup (mcs_plugin->manager, "Taskbar/Position",
                                    CHANNEL);
    if (setting)
    {
        position = setting->data.v_int ? TOP : BOTTOM;
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Taskbar/Position", CHANNEL,
                             position ? 1 : 0);
    }

    setting =
        mcs_manager_setting_lookup (mcs_plugin->manager, "Taskbar/Height",
                                    CHANNEL);
    if (setting)
    {
        height = setting->data.v_int;
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Taskbar/Height", CHANNEL,
                             height);
    }

    setting =
        mcs_manager_setting_lookup (mcs_plugin->manager,
                                    "Taskbar/WidthPercent", CHANNEL);
    if (setting)
    {
        width_percent = setting->data.v_int;
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Taskbar/WidthPercent",
                             CHANNEL, width_percent);
    }

    setting =
        mcs_manager_setting_lookup (mcs_plugin->manager,
                                    "Taskbar/Shrink", CHANNEL);
    if (setting)
    {
        shrink = setting->data.v_int ? TRUE : FALSE;
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Taskbar/Shrink",
                             CHANNEL, shrink);
    }

    setting =
        mcs_manager_setting_lookup (mcs_plugin->manager, "Taskbar/HorizAlign",
                                    CHANNEL);
    if (setting)
    {
        horiz_align = setting->data.v_int;
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Taskbar/HorizAlign",
                             CHANNEL, horiz_align);
    }

    /* autohide */
    setting =
        mcs_manager_setting_lookup (mcs_plugin->manager, "Taskbar/AutoHide",
                                    CHANNEL);
    if (setting)
    {
        autohide = (setting->data.v_int ? TRUE : FALSE);
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Taskbar/AutoHide", CHANNEL,
                             autohide ? 1 : 0);
    }

    /* tasklist */
    setting =
        mcs_manager_setting_lookup (mcs_plugin->manager, "Taskbar/ShowTasklist",
                                    CHANNEL);
    if (setting)
    {
        show_tasklist = (setting->data.v_int ? TRUE : FALSE);
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Taskbar/ShowTasklist",
                             CHANNEL, show_pager ? 1 : 0);
    }

    setting =
        mcs_manager_setting_lookup (mcs_plugin->manager,
                                    "Taskbar/ShowAllTasks", CHANNEL);
    if (setting)
    {
        all_tasks = (setting->data.v_int ? TRUE : FALSE);
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Taskbar/ShowAllTasks",
                             CHANNEL, all_tasks ? 1 : 0);
    }

    setting =
        mcs_manager_setting_lookup (mcs_plugin->manager, "Taskbar/GroupTasks",
                                    CHANNEL);
    if (setting)
    {
        group_tasks = (setting->data.v_int ? TRUE : FALSE);
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Taskbar/GroupTasks",
                             CHANNEL, group_tasks ? 1 : 0);
    }

    setting =
        mcs_manager_setting_lookup (mcs_plugin->manager, "Taskbar/ShowText",
                                    CHANNEL);
    if (setting)
    {
        show_text = setting->data.v_int == 0 ? FALSE : TRUE;
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Taskbar/ShowText", CHANNEL,
                             show_text ? 1 : 0);
    }

    /* pager */
    setting =
        mcs_manager_setting_lookup (mcs_plugin->manager, "Taskbar/ShowPager",
                                    CHANNEL);
    if (setting)
    {
        show_pager = (setting->data.v_int ? TRUE : FALSE);
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Taskbar/ShowPager",
                             CHANNEL, show_pager ? 1 : 0);
    }

    /* status: systray */
    setting =
        mcs_manager_setting_lookup (mcs_plugin->manager, "Taskbar/ShowTray",
                                    CHANNEL);
    if (setting)
    {
        show_tray = (setting->data.v_int ? TRUE : FALSE);
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Taskbar/ShowTray", CHANNEL,
                             show_tray ? 1 : 0);
    }

    /* Time */
    setting =
        mcs_manager_setting_lookup (mcs_plugin->manager, "Taskbar/ShowTime",
                                    CHANNEL);
    if (setting)
    {
        show_time = (setting->data.v_int ? TRUE : FALSE);
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Taskbar/ShowTime",
                             CHANNEL, show_time ? 1 : 0);
    }

    write_options (mcs_plugin);
}

/* public interface */
McsPluginInitResult
mcs_plugin_init (McsPlugin * mcs_plugin)
{
    /* This is required for UTF-8 at least - Please don't remove it */
    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    mcs_plugin->plugin_name = g_strdup (PLUGIN_NAME);
    mcs_plugin->caption = g_strdup (_("Taskbar"));
    mcs_plugin->run_dialog = run_dialog;
    mcs_plugin->icon = xfce_themed_icon_load ("xfce4-taskbar", 48);

    create_channel (mcs_plugin);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);

    return (MCS_PLUGIN_INIT_OK);
}

/* macro defined in manager-plugin.h */
MCS_PLUGIN_CHECK_INIT
