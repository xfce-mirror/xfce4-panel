/*  callbacks.c
 *
 *  Copyright (C) 2002 Jasper Huijsmans <huysmans@users.sourceforge.net>
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "callbacks.h"
#include "xfce_support.h"
#include "xfce.h"
#include "central.h"
#include "item.h"
#include "popup.h"
#include "dialogs.h"
#include "controls_dialog.h"
#include "settings.h"


static PanelPopup *open_popup = NULL;

void hide_current_popup_menu(void);

/*  Panel callbacks
 *  ---------------
*/
gboolean panel_delete_cb(GtkWidget * window, GdkEvent * ev, gpointer data)
{
    quit();

    return TRUE;
}

gboolean panel_destroy_cb(GtkWidget * window, GdkEvent * ev, gpointer data)
{
    write_panel_config();
    
    panel_cleanup();

    gtk_main_quit();

    return TRUE;
}

gboolean main_frame_destroy_cb(GtkWidget * frame, GdkEvent * ev, gpointer data)
{
    side_panel_cleanup(LEFT);
    central_panel_cleanup();
    side_panel_cleanup(RIGHT);

    return TRUE;
}

void iconify_cb(void)
{
    hide_current_popup_menu();

    gtk_window_iconify(GTK_WINDOW(toplevel));
}

/*  Central panel callbacks
 *  -----------------------
*/
void screen_button_click(GtkWidget * b, ScreenButton * sb)
{
    int n =screen_button_get_index(sb);
    
    if(n == current_screen)
    {
	/* keep the button depressed */
        central_panel_set_current(n);
        return;
    }

    request_net_current_desktop(n);
}

gboolean
screen_button_pressed_cb(GtkButton * b, GdkEventButton * ev, ScreenButton * sb)
{
    hide_current_popup_menu();

    if(disable_user_config)
        return FALSE;

    if(ev->button != 3)
        return FALSE;

    screen_button_dialog(sb);

    return TRUE;
}

void mini_lock_cb(void)
{
    char *cmd = settings.lock_command;

    if(!cmd)
        return;

    hide_current_popup_menu();

    exec_cmd(cmd, FALSE);
}

void mini_info_cb(void)
{
    hide_current_popup_menu();

    info_panel_dialog();
}

void mini_palet_cb(void)
{
    hide_current_popup_menu();

    if(disable_user_config)
    {
        show_info(_("Access to the configuration system has been disabled.\n\n"
                    "Ask your system administrator for more information"));
        return;
    }

    global_settings_dialog();
}

void mini_power_cb(GtkButton * b, GdkEventButton * ev, gpointer data)
{
    hide_current_popup_menu();

    quit();
}

/*  Panel control callbacks
 *  -----------------------
*/
gboolean panel_control_press_cb(GtkButton * b, GdkEventButton * ev,
                                PanelControl * pc)
{
    if(ev->button != 3)
        return FALSE;

    if(disable_user_config)
        return FALSE;

    hide_current_popup_menu();

    change_panel_control_dialog(pc);

    return TRUE;
}

/*  Panel popup callbacks
 *  ---------------------
*/
static void hide_popup(PanelPopup * pp)
{
    if(open_popup == pp)
        open_popup = NULL;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pp->button), FALSE);

    gtk_image_set_from_pixbuf(GTK_IMAGE(pp->image), pp->up);

    gtk_widget_hide(pp->window);

    if(pp->detached)
    {
        pp->detached = FALSE;
        gtk_widget_show(pp->tearoff_button);
        gtk_window_set_decorated(GTK_WINDOW(pp->window), FALSE);
    }
}

static void show_popup(PanelPopup * pp)
{
    GtkRequisition req1, req2;
    GdkWindow *p;
    int xb, yb, xp, yp, x, y;

    if(open_popup)
        hide_popup(open_popup);

    if(!pp->detached)
        open_popup = pp;

    if(pp->items && !pp->detached)
        gtk_widget_show(pp->tearoff_button);
    else
        gtk_widget_hide(pp->tearoff_button);

    if(disable_user_config || g_list_length(pp->items) >= NBITEMS)
    {
        gtk_widget_hide(pp->addtomenu_item->button);
        gtk_widget_hide(pp->separator);
    }
    else
    {
        gtk_widget_show(pp->addtomenu_item->button);
        gtk_widget_show(pp->separator);
    }

    gtk_image_set_from_pixbuf(GTK_IMAGE(pp->image), pp->down);

    if(!GTK_WIDGET_REALIZED(pp->button))
        gtk_widget_realize(pp->button);

    gtk_widget_size_request(pp->button, &req1);
    xb = pp->button->allocation.x;
    yb = pp->button->allocation.y;

    p = gtk_widget_get_parent_window(pp->button);
    gdk_window_get_root_origin(p, &xp, &yp);

    x = xb + xp + req1.width / 2;
    y = yb + yp;

    if(!GTK_WIDGET_REALIZED(pp->window))
        gtk_widget_realize(pp->window);

    gtk_widget_size_request(pp->window, &req2);

    /* show upwards or downwards ?
     * and make detached menu appear loose from the button */
    if(y < req2.height && gdk_screen_height() - y > req2.height)
    {
        y = y + req1.height;

        if(pp->detached)
            y = y + 30;
    }
    else
    {
        y = y - req2.height;

        if(pp->detached)
            y = y - 15;
    }

    x = x - req2.width / 2;

    if(x < 0)
        x = 0;
    if(x + req2.width > gdk_screen_width())
        x = gdk_screen_width() - req2.width;

    gtk_window_move(GTK_WINDOW(pp->window), x, y);
    gtk_widget_show(pp->window);
}

void hide_current_popup_menu(void)
{
    if(open_popup)
        hide_popup(open_popup);
}

void toggle_popup(GtkWidget * button, PanelPopup * pp)
{
    hide_current_popup_menu();

    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
        show_popup(pp);
    else
        hide_popup(pp);
}

void tearoff_popup(GtkWidget * button, PanelPopup * pp)
{
    open_popup = NULL;
    pp->detached = TRUE;
    gtk_widget_hide(pp->window);
    gtk_widget_hide(pp->tearoff_button);
    gtk_window_set_decorated(GTK_WINDOW(pp->window), TRUE);
    show_popup(pp);
}

gboolean delete_popup(GtkWidget * window, GdkEvent * ev, PanelPopup * pp)
{
    hide_popup(pp);

    return TRUE;
}

/*  Panel item callbacks
 *  --------------------
*/
void
panel_item_drop_cb(GtkWidget * widget, GdkDragContext * context,
                   gint x, gint y, GtkSelectionData * data,
                   guint info, guint time, PanelItem * pi)
{
    GList *fnames, *fnp;
    guint count;
    char *execute;

    fnames = gnome_uri_list_extract_filenames((char *)data->data);
    count = g_list_length(fnames);

    if(count > 0)
    {
        execute = (char *)g_new0(char, MAXSTRLEN);

        strcpy(execute, pi->command);

        for(fnp = fnames; fnp; fnp = fnp->next, count--)
        {
            strcat(execute, " ");
            strncat(execute, (char *)(fnp->data), MAXSTRLEN - strlen(execute));
        }

        exec_cmd(execute, pi->in_terminal);
        g_free(execute);

        hide_current_popup_menu();
    }
    gnome_uri_list_free_strings(fnames);
    gtk_drag_finish(context, (count > 0), (context->action == GDK_ACTION_MOVE),
                    time);
}

void panel_item_click_cb(GtkButton * b, PanelItem * pi)
{
    hide_current_popup_menu();
    exec_cmd(pi->command, pi->in_terminal);
}

/*  Menu item callbacks
 *  -------------------
*/
void
addtomenu_item_drop_cb(GtkWidget * widget, GdkDragContext * context,
                       gint x, gint y, GtkSelectionData * data,
                       guint info, guint time, PanelPopup * pp)
{
    GList *fnames, *fnp;
    guint count;

    fnames = gnome_uri_list_extract_filenames((char *)data->data);
    count = g_list_length(fnames);

    if(count > 0)
    {
        for(fnp = fnames; fnp; fnp = fnp->next, count--)
        {
            char *cmd;
            MenuItem *mi = menu_item_new(pp);

            if(!(cmd = g_find_program_in_path((char *)fnp->data)))
                continue;

            mi->command = cmd;

            mi->caption = g_path_get_basename(cmd);
            mi->caption[0] = g_ascii_toupper(mi->caption[0]);

            create_menu_item(mi);
            pp->items = g_list_prepend(pp->items, mi);

            gtk_size_group_add_widget(pp->hgroup, mi->image);
            gtk_box_pack_start(GTK_BOX(pp->vbox), mi->button, TRUE, TRUE, 0);

            gtk_box_reorder_child(GTK_BOX(pp->vbox), mi->button, 2);

            gtk_button_clicked(GTK_BUTTON(pp->button));
            gtk_button_clicked(GTK_BUTTON(pp->button));
        }
    }

    gnome_uri_list_free_strings(fnames);
    gtk_drag_finish(context, (count > 0), (context->action == GDK_ACTION_MOVE),
                    time);
}

void addtomenu_item_click_cb(GtkButton * b, PanelPopup * pp)
{
    add_menu_item_dialog(pp);
}

gboolean menu_item_press(GtkButton * b, GdkEventButton * ev, MenuItem * mi)
{
    if(ev->button != 3)
        return FALSE;

    if(disable_user_config)
        return FALSE;

    edit_menu_item_dialog(mi);

    return TRUE;
}

void
menu_item_drop_cb(GtkWidget * widget, GdkDragContext * context,
                  gint x, gint y, GtkSelectionData * data,
                  guint info, guint time, MenuItem * mi)
{
    GList *fnames, *fnp;
    guint count;
    char *execute;

    fnames = gnome_uri_list_extract_filenames((char *)data->data);
    count = g_list_length(fnames);

    if(count > 0)
    {
        execute = (char *)g_new0(char, MAXSTRLEN);

        strcpy(execute, mi->command);

        for(fnp = fnames; fnp; fnp = fnp->next, count--)
        {
            strcat(execute, " ");
            strncat(execute, (char *)(fnp->data), MAXSTRLEN - strlen(execute));
        }

        exec_cmd(execute, mi->in_terminal);
        g_free(execute);

        hide_current_popup_menu();
    }
    gnome_uri_list_free_strings(fnames);
    gtk_drag_finish(context, (count > 0), (context->action == GDK_ACTION_MOVE),
                    time);
}

void menu_item_click_cb(GtkButton * b, MenuItem * mi)
{
    hide_current_popup_menu();
    exec_cmd(mi->command, mi->in_terminal);
}

