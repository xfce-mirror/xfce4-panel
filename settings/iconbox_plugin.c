/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
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

#include <config.h>

#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#include <libxfce4mcs/mcs-manager.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <xfce-mcs-manager/manager-plugin.h>

/* configuration */
#define PLUGIN_NAME  "iconbox"
#define CHANNEL      "iconbox"
#define RCFILE       "xfce4" G_DIR_SEPARATOR_S \
                         "mcs_settings" G_DIR_SEPARATOR_S \
                             "iconbox.xml"

/* default settings */
#define DEFAULT_SIDE           GTK_SIDE_BOTTOM
#define DEFAULT_JUSTIFICATION  GTK_JUSTIFY_LEFT
#define DEFAULT_SIZE	       32
#define DEFAULT_SHOW_HIDDEN    FALSE
#define DEFAULT_TRANSPARENCY   45
#define MAX_TRANSPARENCY       80

/* gui */
#define BORDER             6
#define DEFAULT_ICON_SIZE  48


/* prototypes */
static gboolean write_options (McsPlugin * mcs_plugin);


/* global vars */
static gboolean is_running            = FALSE;
static GtkSideType side               = DEFAULT_SIDE;
static GtkJustification justification = DEFAULT_JUSTIFICATION;
static int icon_size                  = DEFAULT_SIZE;
static gboolean only_hidden           = DEFAULT_SHOW_HIDDEN;
static int transparency               = DEFAULT_TRANSPARENCY;


/* dialog */
typedef struct _IconboxDialog IconboxDialog;

struct _IconboxDialog
{
    McsPlugin *mcs_plugin;

    GtkWidget *dlg;

    /* side / justification */
    GtkWidget *top_left;
    GtkWidget *top_center;
    GtkWidget *top_right;
    GtkWidget *left_left;
    GtkWidget *left_center;
    GtkWidget *left_right;
    GtkWidget *right_left;
    GtkWidget *right_center;
    GtkWidget *right_right;
    GtkWidget *bottom_left;
    GtkWidget *bottom_center;
    GtkWidget *bottom_right;

    GtkWidget *size16;
    GtkWidget *size24;
    GtkWidget *size32;
    GtkWidget *size48;
    GtkWidget *size64;
    
    GtkWidget *only_hidden;

    GtkWidget *transparency;
};

/* functions */
static void
add_space (GtkBox *box, int size)
{
    GtkWidget *align = gtk_alignment_new (0,0,0,0);

    gtk_widget_show (align);
    gtk_widget_set_size_request (align, size, size);
    gtk_box_pack_start (box, align, FALSE, FALSE, 0);
}

/* position */
static gboolean
position_button_pressed (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    IconboxDialog *id = data;

    if (ev->button != 1 ||
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
        return TRUE;
    }

    if (button == id->top_left)
    {
        side = GTK_SIDE_TOP;
        justification = GTK_JUSTIFY_LEFT;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->top_left), 
                                      TRUE);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->top_left), 
                                      FALSE);
    }

    if (button == id->top_center)
    {
        side = GTK_SIDE_TOP;
        justification = GTK_JUSTIFY_CENTER;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->top_center), 
                                      TRUE);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->top_center), 
                                      FALSE);
    }

    if (button == id->top_right)
    {
        side = GTK_SIDE_TOP;
        justification = GTK_JUSTIFY_RIGHT;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->top_right), 
                                      TRUE);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->top_right), 
                                      FALSE);
    }

    if (button == id->bottom_left)
    {
        side = GTK_SIDE_BOTTOM;
        justification = GTK_JUSTIFY_LEFT;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->bottom_left), 
                                      TRUE);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->bottom_left), 
                                      FALSE);
    }
    
    if (button == id->bottom_center)
    {
        side = GTK_SIDE_BOTTOM;
        justification = GTK_JUSTIFY_CENTER;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->bottom_center), 
                                      TRUE);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->bottom_center), 
                                      FALSE);
    }
    
    if (button == id->bottom_right)
    {
        side = GTK_SIDE_BOTTOM;
        justification = GTK_JUSTIFY_RIGHT;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->bottom_right), 
                                      TRUE);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->bottom_right), 
                                      FALSE);
    }

    if (button == id->left_left)
    {
        side = GTK_SIDE_LEFT;
        justification = GTK_JUSTIFY_LEFT;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->left_left), 
                                      TRUE);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->left_left), 
                                      FALSE);
    }

    if (button == id->left_center)
    {
        side = GTK_SIDE_LEFT;
        justification = GTK_JUSTIFY_CENTER;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->left_center), 
                                      TRUE);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->left_center), 
                                      FALSE);
    }
    
    if (button == id->left_right)
    {
        side = GTK_SIDE_LEFT;
        justification = GTK_JUSTIFY_RIGHT;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->left_right), 
                                      TRUE);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->left_right), 
                                      FALSE);
    }
    
    if (button == id->right_left)
    {
        side = GTK_SIDE_RIGHT;
        justification = GTK_JUSTIFY_LEFT;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->right_left), 
                                      TRUE);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->right_left), 
                                      FALSE);
    }
    
    if (button == id->right_center)
    {
        side = GTK_SIDE_RIGHT;
        justification = GTK_JUSTIFY_CENTER;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->right_center), 
                                      TRUE);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->right_center), 
                                      FALSE);
    }
    
    if (button == id->right_right)
    {
        side = GTK_SIDE_RIGHT;
        justification = GTK_JUSTIFY_RIGHT;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->right_right), 
                                      TRUE);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->right_right), 
                                      FALSE);
    }
    
    mcs_manager_set_int (id->mcs_plugin->manager, "Iconbox/Side", CHANNEL, 
                         (int)side);

    mcs_manager_set_int (id->mcs_plugin->manager, "Iconbox/Justification", 
                         CHANNEL, (int)justification);

    mcs_manager_notify (id->mcs_plugin->manager, CHANNEL);
    
    return TRUE;
}

static void
add_position_options (GtkBox *box, IconboxDialog *id)
{
    GtkWidget *frame, *table, *b;
    GtkToggleButton *tb;
    GtkSizeGroup *vsg, *hsg;

    frame = xfce_framebox_new (_("Position"), TRUE);
    gtk_widget_show (frame);
    gtk_box_pack_start (box, frame, FALSE, FALSE, 0);

    table = gtk_table_new (5,5,FALSE);
    gtk_widget_show (table);
    xfce_framebox_add (XFCE_FRAMEBOX (frame), table);

    vsg = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
    hsg = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

    /* top */
    b = id->top_left = gtk_toggle_button_new ();
    gtk_widget_show (b);
    gtk_table_attach (GTK_TABLE (table), b, 1, 2, 0, 1,
                      GTK_FILL, 0, 0, 0);
    gtk_size_group_add_widget (hsg, b);

    gtk_widget_set_size_request (b, 30, 15);

    b = id->top_center = gtk_toggle_button_new ();
    gtk_widget_show (b);
    gtk_table_attach (GTK_TABLE (table), b, 2, 3, 0, 1,
                      GTK_FILL, 0, 0, 0);
    gtk_size_group_add_widget (hsg, b);

    b = id->top_right = gtk_toggle_button_new ();
    gtk_widget_show (b);
    gtk_table_attach (GTK_TABLE (table), b, 3, 4, 0, 1,
                      GTK_FILL, 0, 0, 0);
    gtk_size_group_add_widget (hsg, b);

    /* bottom */
    b = id->bottom_left = gtk_toggle_button_new ();
    gtk_widget_show (b);
    gtk_table_attach (GTK_TABLE (table), b, 1, 2, 4, 5,
                      GTK_FILL, 0, 0, 0);
    gtk_size_group_add_widget (hsg, b);

    b = id->bottom_center = gtk_toggle_button_new ();
    gtk_widget_show (b);
    gtk_table_attach (GTK_TABLE (table), b, 2, 3, 4, 5,
                      GTK_FILL, 0, 0, 0);
    gtk_size_group_add_widget (hsg, b);

    b = id->bottom_right = gtk_toggle_button_new ();
    gtk_widget_show (b);
    gtk_table_attach (GTK_TABLE (table), b, 3, 4, 4, 5,
                      GTK_FILL, 0, 0, 0);
    gtk_size_group_add_widget (hsg, b);

    /* left */
    b = id->left_left = gtk_toggle_button_new ();
    gtk_widget_show (b);
    gtk_table_attach (GTK_TABLE (table), b, 0, 1, 1, 2,
                      0, GTK_FILL, 0, 0);
    gtk_size_group_add_widget (vsg, b);

    gtk_widget_set_size_request (b, 15, 30);

    b = id->left_center = gtk_toggle_button_new ();
    gtk_widget_show (b);
    gtk_table_attach (GTK_TABLE (table), b, 0, 1, 2, 3,
                      0, GTK_FILL, 0, 0);
    gtk_size_group_add_widget (vsg, b);

    b = id->left_right = gtk_toggle_button_new ();
    gtk_widget_show (b);
    gtk_table_attach (GTK_TABLE (table), b, 0, 1, 3, 4,
                      0, GTK_FILL, 0, 0);
    gtk_size_group_add_widget (vsg, b);

    /* right */
    b = id->right_left = gtk_toggle_button_new ();
    gtk_widget_show (b);
    gtk_table_attach (GTK_TABLE (table), b, 4, 5, 1, 2,
                      0, GTK_FILL, 0, 0);
    gtk_size_group_add_widget (vsg, b);

    b = id->right_center = gtk_toggle_button_new ();
    gtk_widget_show (b);
    gtk_table_attach (GTK_TABLE (table), b, 4, 5, 2, 3,
                      0, GTK_FILL, 0, 0);
    gtk_size_group_add_widget (vsg, b);

    b = id->right_right = gtk_toggle_button_new ();
    gtk_widget_show (b);
    gtk_table_attach (GTK_TABLE (table), b, 4, 5, 3, 4,
                      0, GTK_FILL, 0, 0);
    gtk_size_group_add_widget (vsg, b);

    /* activate */
    switch (side)
    {
        case GTK_SIDE_TOP:
            if (justification == GTK_JUSTIFY_LEFT)
            {
                tb = GTK_TOGGLE_BUTTON (id->top_left);
                gtk_toggle_button_set_active (tb, TRUE);
            }
            else if (justification == GTK_JUSTIFY_CENTER)
            {
                tb = GTK_TOGGLE_BUTTON (id->top_center);
                gtk_toggle_button_set_active (tb, TRUE);
            }
            else
            {
                tb = GTK_TOGGLE_BUTTON (id->top_right);
                gtk_toggle_button_set_active (tb, TRUE);
            }
            break;
        case GTK_SIDE_BOTTOM:
            if (justification == GTK_JUSTIFY_LEFT)
            {
                tb = GTK_TOGGLE_BUTTON (id->bottom_left);
                gtk_toggle_button_set_active (tb, TRUE);
            }
            else if (justification == GTK_JUSTIFY_CENTER)
            {
                tb = GTK_TOGGLE_BUTTON (id->bottom_center);
                gtk_toggle_button_set_active (tb, TRUE);
            }
            else
            {
                tb = GTK_TOGGLE_BUTTON (id->bottom_right);
                gtk_toggle_button_set_active (tb, TRUE);
            }
            break;
        case GTK_SIDE_LEFT:
            if (justification == GTK_JUSTIFY_LEFT)
            {
                tb = GTK_TOGGLE_BUTTON (id->left_left);
                gtk_toggle_button_set_active (tb, TRUE);
            }
            else if (justification == GTK_JUSTIFY_CENTER)
            {
                tb = GTK_TOGGLE_BUTTON (id->left_center);
                gtk_toggle_button_set_active (tb, TRUE);
            }
            else
            {
                tb = GTK_TOGGLE_BUTTON (id->left_right);
                gtk_toggle_button_set_active (tb, TRUE);
            }
            break;
        case GTK_SIDE_RIGHT:
            if (justification == GTK_JUSTIFY_LEFT)
            {
                tb = GTK_TOGGLE_BUTTON (id->right_left);
                gtk_toggle_button_set_active (tb, TRUE);
            }
            else if (justification == GTK_JUSTIFY_CENTER)
            {
                tb = GTK_TOGGLE_BUTTON (id->right_center);
                gtk_toggle_button_set_active (tb, TRUE);
            }
            else
            {
                tb = GTK_TOGGLE_BUTTON (id->right_right);
                gtk_toggle_button_set_active (tb, TRUE);
            }
            break;
    }

    /* signals */
    g_signal_connect (id->top_left, "button-press-event",
                      G_CALLBACK (position_button_pressed), id);
    g_signal_connect (id->top_center, "button-press-event",
                      G_CALLBACK (position_button_pressed), id);
    g_signal_connect (id->top_right, "button-press-event",
                      G_CALLBACK (position_button_pressed), id);

    g_signal_connect (id->bottom_left, "button-press-event",
                      G_CALLBACK (position_button_pressed), id);
    g_signal_connect (id->bottom_center, "button-press-event",
                      G_CALLBACK (position_button_pressed), id);
    g_signal_connect (id->bottom_right, "button-press-event",
                      G_CALLBACK (position_button_pressed), id);

    g_signal_connect (id->left_left, "button-press-event",
                      G_CALLBACK (position_button_pressed), id);
    g_signal_connect (id->left_center, "button-press-event",
                      G_CALLBACK (position_button_pressed), id);
    g_signal_connect (id->left_right, "button-press-event",
                      G_CALLBACK (position_button_pressed), id);

    g_signal_connect (id->right_left, "button-press-event",
                      G_CALLBACK (position_button_pressed), id);
    g_signal_connect (id->right_center, "button-press-event",
                      G_CALLBACK (position_button_pressed), id);
    g_signal_connect (id->right_right, "button-press-event",
                      G_CALLBACK (position_button_pressed), id);
}

/* icons */
static gboolean
size_button_pressed (GtkWidget *b, GdkEventButton *ev, gpointer data)
{
    IconboxDialog *id = data;

    if (ev->button != 1 ||
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (b)))
    {
        return TRUE;
    }

    if (b == id->size16)
    {
        icon_size = 16;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->size16), TRUE);
    }
    else 
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->size16), FALSE);
    }
        
    if (b == id->size24)
    {
        icon_size = 24;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->size24), TRUE);
    }
    else  
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->size24), FALSE);
    }
        
    if (b == id->size32)
    {
        icon_size = 32;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->size32), TRUE);
    }
    else  
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->size32), FALSE);
    }
        
    if (b == id->size48)
    {
        icon_size = 48;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->size48), TRUE);
    }
    else  
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->size48), FALSE);
    }
        
    if (b == id->size64)
    {
        icon_size = 64;
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->size64), TRUE);
    } 
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->size64), FALSE);
    }

    mcs_manager_set_int (id->mcs_plugin->manager, "Iconbox/Size", CHANNEL, 
                         icon_size);

    mcs_manager_notify (id->mcs_plugin->manager, CHANNEL);

    write_options (id->mcs_plugin);

    return TRUE;
}

static void
only_hidden_toggled (GtkWidget *b, gpointer data)
{
    IconboxDialog *id = data;

    only_hidden = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (b));

    mcs_manager_set_int (id->mcs_plugin->manager, "Iconbox/OnlyHidden", 
                         CHANNEL, only_hidden ? 1 : 0);

    mcs_manager_notify (id->mcs_plugin->manager, CHANNEL);

    write_options (id->mcs_plugin);
}

static GtkWidget *
create_icon_button (int size)
{
    GtkWidget *button, *image;
    GdkPixbuf *pb;
    int w, h;

    button = gtk_toggle_button_new ();
    
    image = gtk_image_new ();
    gtk_widget_show (image);
    gtk_container_add (GTK_CONTAINER (button), image);

    pb = xfce_themed_icon_load ("xfce4-iconbox", size);
    w = gdk_pixbuf_get_width (pb);
    h = gdk_pixbuf_get_height (pb);

    if (w > size || h > size)
    {
        GdkPixbuf *scaled;

        if (w > h)
        {
            h = h * rint ((double)size/(double)w);
            w = size;
        }
        else
        {
            w = w * rint ((double)size/(double)h);
            h = size;
        }

        scaled = gdk_pixbuf_scale_simple (pb, w, h, GDK_INTERP_BILINEAR);
        g_object_unref (pb);
        pb = scaled;
    }
    
    gtk_image_set_from_pixbuf (GTK_IMAGE (image), pb);
    g_object_unref (pb);

    gtk_widget_set_size_request (image, size, size);

    return button;
}

static void
add_icon_options (GtkBox *box, IconboxDialog *id)
{
    GtkWidget *frame, *table, *align, *button;
    
    frame = xfce_framebox_new (_("Icon size"), TRUE);
    gtk_widget_show (frame);
    gtk_box_pack_start (box, frame, FALSE, FALSE, 0);
    
    /* size */
    table = gtk_table_new (1, 5, FALSE);
    gtk_widget_show (table);
    xfce_framebox_add (XFCE_FRAMEBOX (frame), table);
    gtk_table_set_col_spacings (GTK_TABLE (table), BORDER);

    /* 16 */
    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_table_attach (GTK_TABLE (table), align, 0, 1, 0, 1, 0, GTK_FILL, 0, 0);

    button = id->size16 = create_icon_button (16);
    gtk_widget_show (button);
    gtk_container_add (GTK_CONTAINER (align), button);
    
    /* 24 */
    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_table_attach (GTK_TABLE (table), align, 1, 2, 0, 1, 0, GTK_FILL, 0, 0);
    
    button = id->size24 = create_icon_button (24);
    gtk_widget_show (button);
    gtk_container_add (GTK_CONTAINER (align), button);

    /* 32 */
    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_table_attach (GTK_TABLE (table), align, 2, 3, 0, 1, 0, GTK_FILL, 0, 0);
    
    button = id->size32 = create_icon_button (32);
    gtk_widget_show (button);
    gtk_container_add (GTK_CONTAINER (align), button);

    /* 48 */
    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_table_attach (GTK_TABLE (table), align, 3, 4, 0, 1, 0, GTK_FILL, 0, 0);
    
    button = id->size48 = create_icon_button (48);
    gtk_widget_show (button);
    gtk_container_add (GTK_CONTAINER (align), button);

    /* 64 */
    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_table_attach (GTK_TABLE (table), align, 4, 5, 0, 1, 0, GTK_FILL, 0, 0);
    
    button = id->size64 = create_icon_button (64);
    gtk_widget_show (button);
    gtk_container_add (GTK_CONTAINER (align), button);

    /* show only hidden */
    frame = xfce_framebox_new (_("Behaviour"), TRUE);
    gtk_widget_show (frame);
    gtk_box_pack_start (box, frame, FALSE, FALSE, 0);
    
    id->only_hidden = 
        gtk_check_button_new_with_mnemonic (
                _("_Show only minimized applications"));
    gtk_widget_show (id->only_hidden);
    xfce_framebox_add (XFCE_FRAMEBOX (frame), id->only_hidden);

    /* activate */
    if (icon_size == 16)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->size16), TRUE);
    else if (icon_size == 24)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->size24), TRUE);
    else if (icon_size == 32)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->size32), TRUE);
    else if (icon_size == 48)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->size48), TRUE);
    else if (icon_size == 64)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->size64), TRUE);

    if (only_hidden)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (id->only_hidden), 
                                      TRUE);

    /* signals */
    g_signal_connect (id->size16, "button-press-event", 
                      G_CALLBACK (size_button_pressed), id);

    g_signal_connect (id->size24, "button-press-event", 
                      G_CALLBACK (size_button_pressed), id);

    g_signal_connect (id->size32, "button-press-event", 
                      G_CALLBACK (size_button_pressed), id);

    g_signal_connect (id->size48, "button-press-event", 
                      G_CALLBACK (size_button_pressed), id);

    g_signal_connect (id->size64, "button-press-event", 
                      G_CALLBACK (size_button_pressed), id);

    g_signal_connect (id->only_hidden, "toggled", 
                      G_CALLBACK (only_hidden_toggled), id);
}

/* transparency */
static void
transparency_changed (GtkWidget *w, gpointer data)
{
    IconboxDialog *id = data;
    int transparency = 
        (int) gtk_range_get_value (GTK_RANGE (id->transparency));

    g_print (" ++ transparency: %d\n", transparency);
    
    mcs_manager_set_int (id->mcs_plugin->manager, "Iconbox/Transparency",
                         CHANNEL, transparency);

    mcs_manager_notify (id->mcs_plugin->manager, CHANNEL);

    write_options (id->mcs_plugin);
}

static void
add_transparency_option (GtkBox *box, IconboxDialog *id)
{
    GtkWidget *frame, *hbox, *label;
    
    frame = xfce_framebox_new (_("Transparency"), TRUE);
    gtk_widget_show (frame);
    gtk_box_pack_start (box, frame, FALSE, FALSE, 0);

    hbox =  gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    xfce_framebox_add (XFCE_FRAMEBOX (frame), hbox);

    label = xfce_create_small_label (_("None"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    id->transparency =
        gtk_hscale_new_with_range (0, MAX_TRANSPARENCY, 5);
    gtk_widget_show (id->transparency);
    gtk_range_set_value (GTK_RANGE (id->transparency), transparency);
    gtk_scale_set_draw_value (GTK_SCALE (id->transparency), FALSE);
    gtk_scale_set_digits (GTK_SCALE (id->transparency), 0);
    gtk_range_set_update_policy (GTK_RANGE (id->transparency),
                                 GTK_UPDATE_DISCONTINUOUS);
    gtk_box_pack_start (GTK_BOX (hbox), id->transparency, TRUE, TRUE, 0);

    label = xfce_create_small_label (_("Full"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    g_signal_connect (id->transparency, "value-changed", 
                      G_CALLBACK (transparency_changed), id);
}

/* main dialog */
static void
dialog_response (GtkWidget * dialog, gint response_id, IconboxDialog *id)
{
    if (response_id == GTK_RESPONSE_HELP)
    {
        g_message ("HELP: TBD");
    }
    else
    {
        DBG ("\n"
             "Side         : %s\n"
             "Justification: %s\n"
             "Size         : %d\n"
             "Only hidden  : %s\n",
             "Transparency : %s",
             (side == GTK_SIDE_TOP) ? "top" :
               (side == GTK_SIDE_BOTTOM) ? "bottom" :
                 (side == GTK_SIDE_LEFT) ? "left" : 
                   "right",
             (justification == GTK_JUSTIFY_LEFT) ? "left" :
               (justification == GTK_JUSTIFY_CENTER) ? "center" : 
                 "right",
             icon_size,
             only_hidden ? "true" : "false",
             transparency);

        write_options (id->mcs_plugin);
        g_free (id);

        is_running = FALSE;
        gtk_widget_destroy (dialog);
    }
}

static void
create_dialog (McsPlugin * mcs_plugin)
{
    IconboxDialog *dialog;
    GtkWidget *vbox, *header, *vbox2;

    dialog = g_new0 (IconboxDialog, 1);

    dialog->mcs_plugin = mcs_plugin;

    dialog->dlg = gtk_dialog_new_with_buttons (_("Iconbox"), NULL,
                                               GTK_DIALOG_NO_SEPARATOR,
                                               GTK_STOCK_CLOSE,
                                               GTK_RESPONSE_OK,
                                               NULL);

    gtk_window_set_icon (GTK_WINDOW (dialog->dlg), mcs_plugin->icon);

    vbox = GTK_DIALOG (dialog->dlg)->vbox;
    
    header = xfce_create_header (mcs_plugin->icon, _("Iconbox"));
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (vbox), header, FALSE, TRUE, 0);

    add_space (GTK_BOX (vbox), BORDER);

    vbox2 = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (vbox2), BORDER);
    gtk_widget_show (vbox2);
    gtk_box_pack_start (GTK_BOX (vbox), vbox2, TRUE, TRUE, 0);
    
    /* position */
    add_position_options (GTK_BOX (vbox2), dialog);

    /* icons */
    add_icon_options (GTK_BOX (vbox2), dialog);

    /* transparency */
    add_transparency_option (GTK_BOX (vbox2), dialog);
    
    add_space (GTK_BOX (vbox), BORDER);

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

    rcfile = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, RCFILE, TRUE);
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

    rcfile = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, RCFILE);

    if (rcfile)
        mcs_manager_add_channel_from_file (mcs_plugin->manager, CHANNEL,
                                           rcfile);
    else
        mcs_manager_add_channel (mcs_plugin->manager, CHANNEL);

    g_free (rcfile);

    /* position */
    setting = mcs_manager_setting_lookup (mcs_plugin->manager, 
                                          "Iconbox/Justification",
                                          CHANNEL);
    if (setting)
    {
        justification = (GtkJustification)setting->data.v_int;
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Iconbox/Justification", 
                             CHANNEL, (int)justification);
    }

    setting = mcs_manager_setting_lookup (mcs_plugin->manager, 
                                          "Iconbox/Side",
                                          CHANNEL);
    if (setting)
    {
        side = (GtkSideType)setting->data.v_int;
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Iconbox/Side", 
                             CHANNEL, (int)side);
    }

    setting = mcs_manager_setting_lookup (mcs_plugin->manager, 
                                          "Iconbox/Size",
                                          CHANNEL);
    if (setting)
    {
        icon_size = CLAMP (setting->data.v_int, 16, 64);
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Iconbox/Size", 
                             CHANNEL, icon_size);
    }

    setting = mcs_manager_setting_lookup (mcs_plugin->manager, 
                                          "Iconbox/OnlyHidden",
                                          CHANNEL);
    if (setting)
    {
        only_hidden = (setting->data.v_int == 1);
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Iconbox/OnlyHidden", 
                             CHANNEL, only_hidden ? 1 : 0);
    }

    setting = mcs_manager_setting_lookup (mcs_plugin->manager, 
                                          "Iconbox/Transparency",
                                          CHANNEL);
    if (setting)
    {
        transparency = CLAMP (setting->data.v_int, 0, MAX_TRANSPARENCY);
    }
    else
    {
        mcs_manager_set_int (mcs_plugin->manager, "Iconbox/Transparency", 
                             CHANNEL, transparency);
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
    mcs_plugin->caption = g_strdup (_("Iconbox"));
    mcs_plugin->run_dialog = run_dialog;
    mcs_plugin->icon = xfce_themed_icon_load ("xfce4-iconbox", 48);

    create_channel (mcs_plugin);
    mcs_manager_notify (mcs_plugin->manager, CHANNEL);

    return (MCS_PLUGIN_INIT_OK);
}

/* macro defined in manager-plugin.h */
MCS_PLUGIN_CHECK_INIT
