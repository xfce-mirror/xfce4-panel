/*  callbacks.c
 *
 *  Copyright (C) 2002 Jasper Huijsmans <j.b.huijsmans@hetnet.nl>
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

#include "xfce.h"

#include "callbacks.h"
#include "dialogs.h"
#include "panel.h"
#include "central.h"
#include "side.h"
#include "item.h"
#include "popup.h"
#include "icons.h"

static PanelPopup *open_popup = NULL;

void hide_current_popup_menu(void);

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Utility functions

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void exec_cmd(const char *cmd, gboolean in_terminal)
{
    GError *error = NULL;       /* this must be NULL to prevent crash :( */
    char execute[MAXSTRLEN + 1];

    if(!cmd)
        return;

    if (in_terminal)
	snprintf(execute, MAXSTRLEN, "xterm -e %s", cmd);
    else
	snprintf(execute, MAXSTRLEN, "%s", cmd);
    
    if(!g_spawn_command_line_async(execute, &error))
    {
        char *msg;

        msg = g_strcompress(error->message);

	report_error(msg);
	
        g_free(msg);
    }
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/*** the next three routines are taken straight from gnome-libs so that the
     gtk-only version can receive drag and drops as well ***/
/**
 * gnome_uri_list_free_strings:
 * @list: A GList returned by gnome_uri_list_extract_uris() or gnome_uri_list_extract_filenames()
 *
 * Releases all of the resources allocated by @list.
 */
void gnome_uri_list_free_strings(GList * list)
{
    g_list_foreach(list, (GFunc) g_free, NULL);
    g_list_free(list);
}


/**
 * gnome_uri_list_extract_uris:
 * @uri_list: an uri-list in the standard format.
 *
 * Returns a GList containing strings allocated with g_malloc
 * that have been splitted from @uri-list.
 */
GList *gnome_uri_list_extract_uris(const gchar * uri_list)
{
    const gchar *p, *q;
    gchar *retval;
    GList *result = NULL;

    g_return_val_if_fail(uri_list != NULL, NULL);

    p = uri_list;

    /* We don't actually try to validate the URI according to RFC
     * 2396, or even check for allowed characters - we just ignore
     * comments and trim whitespace off the ends.  We also
     * allow LF delimination as well as the specified CRLF.
     */
    while(p)
    {
        if(*p != '#')
        {
            while(isspace((int)(*p)))
                p++;

            q = p;
            while(*q && (*q != '\n') && (*q != '\r'))
                q++;

            if(q > p)
            {
                q--;
                while(q > p && isspace((int)(*q)))
                    q--;

                retval = (char *)g_malloc(q - p + 2);
                strncpy(retval, p, q - p + 1);
                retval[q - p + 1] = '\0';

                result = g_list_prepend(result, retval);
            }
        }
        p = strchr(p, '\n');
        if(p)
            p++;
    }

    return g_list_reverse(result);
}


/**
 * gnome_uri_list_extract_filenames:
 * @uri_list: an uri-list in the standard format
 *
 * Returns a GList containing strings allocated with g_malloc
 * that contain the filenames in the uri-list.
 *
 * Note that unlike gnome_uri_list_extract_uris() function, this
 * will discard any non-file uri from the result value.
 */
GList *gnome_uri_list_extract_filenames(const gchar * uri_list)
{
    GList *tmp_list, *node, *result;

    g_return_val_if_fail(uri_list != NULL, NULL);

    result = gnome_uri_list_extract_uris(uri_list);

    tmp_list = result;
    while(tmp_list)
    {
        gchar *s = (char *)tmp_list->data;

        node = tmp_list;
        tmp_list = tmp_list->next;

        if(!strncmp(s, "file:", 5))
        {
            /* added by Jasper Huijsmans
               remove leading multiple slashes */
            if(!strncmp(s + 5, "///", 3))
                node->data = g_strdup(s + 7);
            else
                node->data = g_strdup(s + 5);
        }
        else
        {
            node->data = g_strdup(s);
        }
        g_free(s);
    }
    return result;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Panel callbacks

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

gboolean panel_delete_cb(GtkWidget * window, GdkEvent * ev, gpointer data)
{
    GtkWidget *dialog;
    int response;
    char *text = _("The panel recieved a request from the window "
                   "manager to quit. Usually this is by accident.\n"
                   "If you want to continue using the panel choose "
                   "\'No\'\n\n" "Do you want to quit the panel?");

    hide_current_popup_menu();

    dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, text);

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if(response == GTK_RESPONSE_YES)
        quit();

    return TRUE;
}

gboolean panel_destroy_cb(GtkWidget * window, GdkEvent * ev, gpointer data)
{
    quit();

    return TRUE;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Central panel callbacks

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

gboolean
screen_button_pressed_cb(GtkButton * b, GdkEventButton * ev, ScreenButton * sb)
{
    hide_current_popup_menu();

    if(ev->button != 3)
        return FALSE;

    return TRUE;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   To be removed !

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/* temporary functions for testing purposes */
void switch_size_cb(void)
{
    static int size = MEDIUM;

    size = size == MEDIUM ? SMALL : size == SMALL ? LARGE : MEDIUM;

    panel_set_size(size);
    panel_set_popup_size(size);
}

void switch_style_cb(void)
{
    static int style = NEW_STYLE;

    style = style == NEW_STYLE ? OLD_STYLE : NEW_STYLE;

    panel_set_style(style);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   End temporary functions

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void mini_lock_cb(char *cmd)
{
    if(!cmd)
        return;

    hide_current_popup_menu();

    exec_cmd(cmd, FALSE);
}

void mini_info_cb(void)
{
    hide_current_popup_menu();

    switch_size_cb();
}

void mini_palet_cb(void)
{
    hide_current_popup_menu();

    switch_style_cb();
}

void mini_power_cb(GtkButton * b, GdkEventButton * ev, char *cmd)
{
    int w, h;

    hide_current_popup_menu();

    gdk_window_get_size(ev->window, &w, &h);

    /* we have to check if the pointer is still inside the button */
    if(ev->x < 0 || ev->x > w || ev->y < 0 || ev->y > h)
    {
        gtk_button_released(b);
        return;
    }

    if(ev->button == 1)
    {
        exec_cmd(cmd, FALSE);
        quit();
    }
    else
        restart();
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Panel group callbacks

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void iconify_cb(void)
{
    hide_current_popup_menu();

    gtk_window_iconify(GTK_WINDOW(toplevel));
}

gboolean panel_group_press_cb(GtkButton * b, GdkEventButton * ev, PanelGroup * pg)
{
    hide_current_popup_menu();

    if(ev->button != 3)
        return FALSE;

    edit_panel_control_dialog(pg);

    return TRUE;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Panel popup callbacks

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

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

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Panel item callbacks

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void
panel_item_drop_cb(GtkWidget * widget, GdkDragContext * context,
             gint x, gint y, GtkSelectionData * data,
             guint info, guint time, PanelItem *pi)
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

void panel_item_click_cb(GtkButton * b, PanelItem *pi)
{
    hide_current_popup_menu();
    exec_cmd(pi->command, pi->in_terminal);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Menu item callbacks

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

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

    edit_menu_item_dialog(mi);

    return TRUE;
}

void
menu_item_drop_cb(GtkWidget * widget, GdkDragContext * context,
             gint x, gint y, GtkSelectionData * data,
             guint info, guint time, MenuItem *mi)
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

void menu_item_click_cb(GtkButton * b, MenuItem *mi)
{
    hide_current_popup_menu();
    exec_cmd(mi->command, mi->in_terminal);
}

