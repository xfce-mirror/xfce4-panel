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

#include <gdk/gdkkeysyms.h>
#include <libxfcegui4/xfce_togglebutton.h>

#include "xfce.h"
#include "callbacks.h"
#include "item.h"
#include "item_dialog.h"
#include "popup.h"
#include "controls_dialog.h"
#include "settings.h"

static PanelPopup *open_popup = NULL;

/*  Panel callbacks
 *  ---------------
*/
void iconify_cb(void)
{
    hide_current_popup_menu();

    gtk_window_iconify(GTK_WINDOW(toplevel));
}

void close_cb(void)
{
    hide_current_popup_menu();

    quit(FALSE);
}

/*  Panel popup callbacks
 *  ---------------------
*/
static void hide_popup(PanelPopup * pp)
{
    if(open_popup == pp)
        open_popup = NULL;

    xfce_togglebutton_set_active(XFCE_TOGGLEBUTTON(pp->button), FALSE);

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
    GtkRequisition req;
    GdkWindow *p;
    int xbutton, ybutton, xparent, yparent, x, y;
    int w, h;
    gboolean vertical = settings.orientation == VERTICAL;
    int pos = settings.popup_position;
    GtkAllocation alloc1 = {0, }, alloc2 = {0, };

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

    alloc1 = pp->button->allocation;
    xbutton = alloc1.x;
    ybutton = alloc1.y;
    
    p = gtk_widget_get_parent_window(pp->button);
    gdk_window_get_root_origin(p, &xparent, &yparent);

    w = gdk_screen_width();
    h = gdk_screen_height();

    if(!GTK_WIDGET_REALIZED(pp->window))
        gtk_widget_realize(pp->window);

    gtk_widget_size_request(pp->window, &req);

    alloc2.width = req.width;
    alloc2.height = req.height;
    gtk_widget_size_allocate(pp->window, &alloc2);

    gtk_widget_realize(pp->window);
    
    /* this is necessary, because the icons are resized on allocation */
    gtk_widget_size_request(pp->window, &req);

    /*  positioning logic (well ...)
     *  ----------------------------
     *  1) vertical panel
     *  - to the left or the right
     *    - if buttons left the left  else right
     *  - fit the screen
     *  2) horizontal
     *  - up or down
     *    - if buttons down -> down else up
     *  - fit the screen
    */

    /* set x and y to topleft corner of the button */
    x = xbutton + xparent;
    y = ybutton + yparent;
	
    if (vertical)
    {
	/* left if buttons left ... */
	if (pos == LEFT && x - req.width >= 0)
	{
	    x = x - req.width;
	    y = y + alloc1.height - req.height;
	}
	/* .. or if menu doesn't fit right, but does fit left */
	else if (x + req.width + alloc1.width > w && x - req.width >= 0)
	{
	    x = x - req.width;
	    y = y + alloc1.height - req.height;
	}
	else /* right */
	{
	    x = x + alloc1.width;
	    y = y + alloc1.height - req.height;
	}

	/* check if it fits upwards */
	if (y < 0)
	    y = 0;
    }
    else
    {
	/* down if buttons on bottom ... */
	if (pos == BOTTOM && y + alloc1.height + req.height <= h)
	{
	    x = x + alloc1.width / 2 - req.width / 2;
	    y = y + alloc1.height;
	}
	/* ... or up doesn't fit and down does */
	else if (y - req.height < 0 && y + alloc1.height + req.height <= h)
	{
	    x = x + alloc1.width / 2 - req.width / 2;
	    y = y + alloc1.height;
	}
	else
	{
	    x = x + alloc1.width / 2 - req.width / 2;
	    y = y - req.height;
	}

	/* check if it fits to the left and the right */
	if (x < 0)
	    x = 0;
	else if (x + req.width > w)
	    x = w - req.width;
    }

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

gboolean popup_key_pressed(GtkWidget * window, GdkEventKey * ev, 
			   PanelPopup * pp)
{
    if (ev->keyval == GDK_Escape)
    {
	hide_popup(pp);
	return TRUE;
    }
    else
	return FALSE;
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
	    panel_popup_add_item(pp, mi);

	    if (!pp->detached)
	    {
		xfce_togglebutton_toggled(XFCE_TOGGLEBUTTON(pp->button));
		xfce_togglebutton_toggled(XFCE_TOGGLEBUTTON(pp->button));
	    }
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
    if(disable_user_config)
        return FALSE;

    if(ev->button == 3 || 
	    (ev->button == 1 && (ev->state & GDK_SHIFT_MASK)))
    {
	edit_menu_item_dialog(mi);

	return TRUE;
    }
    else
    {
        return FALSE;
    }
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
