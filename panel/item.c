/*  $Id$
 *  
 *  Copyright 2002-2004 Jasper Huijsmans (jasper@xfce.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/*  Item
 *  ----
 *  An Item is a traditional launcher item for the panel and is also used for
 *  (subpanel) menu items.
 *
 *  For the panel, the ControlClass interface is implemented, menu items have
 *  there own interface.
 *
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gmodule.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/xfce_togglebutton.h>
#include <libxfcegui4/xfce_menubutton.h>

#include "xfce.h"
#include "item.h"
#include "item-control.h"
#include "item_dialog.h"
#include "settings.h"

#define INSENSITIVE_TIMEOUT     800

static gboolean popup_from_timeout = FALSE;
static int popup_timeout_id = 0;

/* prevent users from clicking twice because of slow app start */
static gboolean set_sensitive_cb (GtkWidget *button)
{
    if (GTK_IS_WIDGET (button))
        gtk_widget_set_sensitive (button, TRUE);
    
    return FALSE;
}

static void
after_exec_insensitive (GtkWidget *button)
{
    gtk_widget_set_sensitive (button, FALSE);
    
    g_timeout_add (INSENSITIVE_TIMEOUT, 
                   (GSourceFunc) set_sensitive_cb, button);
}

/*  Common item callbacks
 *  ---------------------
*/
static void
item_drop_cb (GtkWidget * widget, GdkDragContext * context, gint x,
	      gint y, GtkSelectionData * data, guint info,
	      guint time, Item * item)
{
    GList *fnames, *fnp;
    guint count;
    GString *execute;

    if (!item || !(item->command))
	return;

    fnames = gnome_uri_list_extract_filenames ((char *) data->data);
    count = g_list_length (fnames);

    if (count > 0)
    {
	execute = g_string_sized_new (3 * strlen (item->command));

	if (item->in_terminal)
	    g_string_append_c (execute, '"');

	g_string_append (execute, item->command);

	for (fnp = fnames; fnp; fnp = fnp->next, count--)
	{
	    g_string_append_c (execute, ' ');
	    g_string_append_c (execute, '\'');
	    g_string_append (execute, (char *) (fnp->data));
	    g_string_append_c (execute, '\'');
	}

	if (item->in_terminal)
	    g_string_append_c (execute, '"');

	exec_cmd (execute->str, item->in_terminal, item->use_sn);
        after_exec_insensitive (item->button);
        
	g_string_free (execute, TRUE);

	hide_current_popup_menu ();
    }

    gnome_uri_list_free_strings (fnames);
    gtk_drag_finish (context, (count > 0),
		     (context->action == GDK_ACTION_MOVE), time);
}

static void
item_click_cb (GtkButton * b, Item * item)
{
    if (item->type == PANELITEM && popup_from_timeout)
	return;

    if (popup_timeout_id > 0)
    {
	g_source_remove (popup_timeout_id);
	popup_timeout_id = 0;
    }

    hide_current_popup_menu ();

    exec_cmd (item->command, item->in_terminal, item->use_sn);
    after_exec_insensitive (item->button);
}

static gboolean
item_middle_click (GtkWidget * w, GdkEventButton * ev, Item * item)
{
    if (ev->button == 2)
    {
	/* TODO: run command with data from clipboard */
    }

    return FALSE;
}

/*  Menu item callbacks
 *  -------------------
*/
static const char *keys[] = {
    "Name",
    "GenericName",
    "Comment",
    "Exec",
    "Icon",
    "Terminal"
};

static void
addtomenu_item_drop_cb (GtkWidget * widget,
			GdkDragContext * context,
			gint x, gint y,
			GtkSelectionData * data,
			guint info, guint time, PanelPopup * pp)
{
    GList *fnames, *fnp;
    guint count;

    fnames = gnome_uri_list_extract_filenames ((char *) data->data);
    count = g_list_length (fnames);

    if (count > 0)
    {
	for (fnp = fnames; fnp; fnp = fnp->next, count--)
	{
	    Item *mi;
            char *buf, *s;
            XfceDesktopEntry *dentry;

            s = (char *) fnp->data;

            if (!strncmp (s, "file:", 5))
            {
                s += 5;

                if (!strncmp (s, "//", 2))
                    s += 2;
            }

            buf = g_strdup (s);

            if ((s = strchr (buf, '\n')))
                *s = '\0';
            
	    mi = menu_item_new (pp);

            if (g_file_test (buf, G_FILE_TEST_EXISTS) &&
                XFCE_IS_DESKTOP_ENTRY (dentry =
                                       xfce_desktop_entry_new (buf, keys, 
                                           G_N_ELEMENTS (keys))))
            {
                char *term;
                
                xfce_desktop_entry_get_string (dentry, "GenericName", FALSE,
                                               &(mi->caption));
                
                if (!mi->caption)
                {
                    xfce_desktop_entry_get_string (dentry, "Name", FALSE,
                                                   &(mi->caption));
                }
                
                xfce_desktop_entry_get_string (dentry, "Comment", FALSE,
                                               &(mi->tooltip));
                
                xfce_desktop_entry_get_string (dentry, "Exec", FALSE,
                                               &(mi->command));

                xfce_desktop_entry_get_string (dentry, "Icon", FALSE,
                                               &(mi->icon_path));

                if (mi->icon_path)
                    mi->icon_id = EXTERN_ICON;
                
                xfce_desktop_entry_get_string (dentry, "Terminal", FALSE,
                                               &term);

                if (term && 
                    (!strncmp ("1", term, 1) || !strncmp ("true", term, 4)))
                {
                    mi->in_terminal = TRUE;
                    g_free (term);
                }

                g_object_unref (dentry);
                g_free (buf);
            }
            else
            {
                mi->command = buf;

                mi->caption = g_path_get_basename (mi->command);
                mi->caption[0] = g_ascii_toupper (mi->caption[0]);
            }

	    create_menu_item (mi);
	    panel_popup_add_item (pp, mi);

	    if (!panel_popup_is_detached (pp))
	    {
                panel_popup_update_menu_position (pp);
	    }
	}
    }

    gnome_uri_list_free_strings (fnames);
    gtk_drag_finish (context, (count > 0),
		     (context->action == GDK_ACTION_MOVE), time);
}

static void
addtomenu_item_click_cb (GtkButton * b, PanelPopup * pp)
{
    add_menu_item_dialog (pp);
}

static gboolean
menu_item_press (GtkButton * b, GdkEventButton * ev, Item * mi)
{
    if (disable_user_config)
	return FALSE;

    if (ev->button == 3 || (ev->button == 1 && (ev->state & GDK_SHIFT_MASK)))
    {
	edit_menu_item_dialog (mi);

	return TRUE;
    }

    return FALSE;
}

/*  Panel item callbacks
 *  --------------------
*/
static gboolean
popup_menu_timeout (Item * item)
{
    GtkWidget *box;
    GList *li;

    popup_timeout_id = 0;

    /* FIXME: items should know about their parent */

    /* Explanantion of code below:
     * 
     * For a panel item with menu we have a GtkBox containing an item 
     * and a popup button, so 'item->button->parent' is the box we are 
     * looking for.
     *
     * A GtkBox contains children of the type GtkBoxChild. We are 
     * interested in the widget member of this struct. If it is not
     * the same as the parent container of our button, it must be the
     * toggle button.
     *
     * There, that's it.
     */
    box = item->button->parent;

    for (li = GTK_BOX (box)->children; li; li = li->next)
    {
	GtkWidget *w = ((GtkBoxChild *) li->data)->widget;

	if (!GTK_IS_WIDGET (w))
	    continue;

	if (w != item->button)
	{
	    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
	    {
		popup_from_timeout = TRUE;

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
	    }

	    break;
	}
    }

    return FALSE;
}

static gboolean
panel_item_press (GtkButton * b, GdkEventButton * ev, Item * item)
{
    popup_from_timeout = FALSE;

    if (!item->with_popup)
	return FALSE;

    if (ev->button == 1 && !(ev->state & GDK_SHIFT_MASK))
    {
	popup_timeout_id =
	    g_timeout_add (400, (GSourceFunc) popup_menu_timeout, item);
    }

    return FALSE;
}

/*  Common item interface
 *  ---------------------
*/
G_MODULE_EXPORT /* EXPORT:item_read_config */
void
item_read_config (Item * item, xmlNodePtr node)
{
    const gchar *locale;
    xmlNodePtr child;
    xmlChar   *value;
    xmlChar   *lang;
    gboolean   caption_found = FALSE;
    guint      caption_match = XFCE_LOCALE_NO_MATCH;
    gboolean   tooltip_found = FALSE;
    guint      tooltip_match = XFCE_LOCALE_NO_MATCH;
    guint      match;

    locale = setlocale (LC_MESSAGES, NULL);

    value = xmlGetProp (node, "popup");

    if (value)
    {
	item->with_popup = strtol (value, NULL, 0) == 1 ? TRUE : FALSE;
	g_free (value);
    }

    for (child = node->children; child; child = child->next)
    {
	if (xmlStrEqual (child->name, (const xmlChar *) "Caption"))
	{
	    value = DATA (child);

	    if (value != NULL)
            {
                lang = xmlNodeGetLang (child);
            
                if (lang != NULL)
                {
                    match = xfce_locale_match (locale, (const gchar *) lang);
                    xmlFree (lang);
                }
                else
                {
                    match = XFCE_LOCALE_NO_MATCH;
                }

                if (match > caption_match || !caption_found)
                {
		    item->caption = g_strdup ((const char *) value);
                    caption_match = match;
                    caption_found = TRUE;
                }

                xmlFree (value);
            }
	}
	else if (xmlStrEqual (child->name, (const xmlChar *) "Command"))
	{
	    int n = -1;

	    value = DATA (child);

	    if (value)
		item->command = (char *) value;

	    value = xmlGetProp (child, "term");

	    if (value)
	    {
		n = (int) strtol (value, NULL, 0);
		g_free (value);
	    }

	    if (n == 1)
	    {
		item->in_terminal = TRUE;
	    }
	    else
	    {
		item->in_terminal = FALSE;
	    }

	    n = -1;
	    value = xmlGetProp (child, "sn");

	    if (value)
	    {
		n = (int) strtol (value, NULL, 0);
		g_free (value);
	    }

	    if (n == 1)
	    {
		item->use_sn = TRUE;
	    }
	    else
	    {
		item->use_sn = FALSE;
	    }

	}
	else if (xmlStrEqual (child->name, (const xmlChar *) "Tooltip"))
	{
	    value = DATA (child);

	    if (value != NULL)
            {
                lang = xmlNodeGetLang (child);
                if (lang != NULL)
                {
                    match = xfce_locale_match (locale, (const gchar *) lang);
                    xmlFree (lang);
                }
                else
                {
                    match = XFCE_LOCALE_NO_MATCH;
                }

                if (match > tooltip_match || !tooltip_found)
                {
                    item->tooltip = g_strdup ((const char *) value);
                    tooltip_match = match;
                    tooltip_found = TRUE;
                }

                xmlFree (value);
            }
	}
	else if (xmlStrEqual (child->name, (const xmlChar *) "Icon"))
	{
	    value = xmlGetProp (child, (const xmlChar *) "id");

	    if (value)
		item->icon_id = (int) strtol (value, NULL, 0);

	    if (!value || item->icon_id < EXTERN_ICON ||
		item->icon_id >= NUM_ICONS)
		item->icon_id = UNKNOWN_ICON;

	    g_free (value);

	    if (item->icon_id == EXTERN_ICON)
	    {
		value = DATA (child);

		if (value)
		    item->icon_path = (char *) value;
		else
		    item->icon_id = UNKNOWN_ICON;
	    }
	}
    }
}

G_MODULE_EXPORT /* EXPORT:item_write_config */
void
item_write_config (Item * item, xmlNodePtr node)
{
    xmlNodePtr child;
    char value[3];

    if (item->type == MENUITEM)
	xmlNewTextChild (node, NULL, "Caption", item->caption);

    child = xmlNewTextChild (node, NULL, "Command", item->command);

    snprintf (value, 2, "%d", item->in_terminal);
    xmlSetProp (child, "term", value);

    snprintf (value, 2, "%d", item->use_sn);
    xmlSetProp (child, "sn", value);

    if (item->tooltip)
	xmlNewTextChild (node, NULL, "Tooltip", item->tooltip);

    child = xmlNewTextChild (node, NULL, "Icon", item->icon_path);

    snprintf (value, 3, "%d", item->icon_id);
    xmlSetProp (child, "id", value);

    if (item->type == PANELITEM)
    {
	snprintf (value, 3, "%d", item->with_popup);
	xmlSetProp (node, "popup", value);
    }
}

G_MODULE_EXPORT /* EXPORT:item_free */
void
item_free (Item * item)
{
    if (item->type == PANELITEM)
    {
        g_object_unref (item->button);
        gtk_widget_destroy (item->button);
    }

    g_free (item->command);
    g_free (item->caption);
    g_free (item->tooltip);
    g_free (item->icon_path);

    g_free (item);
}

G_MODULE_EXPORT /* EXPORT:item_set_theme */
void
item_set_theme (Item * item, const char *theme)
{
    GdkPixbuf *pb;

    if (item->icon_id <= EXTERN_ICON)
	return;

    pb = get_pixbuf_by_id (item->icon_id);

    if (item->type == MENUITEM)
	xfce_menubutton_set_pixbuf (XFCE_MENUBUTTON (item->button), pb);
    else
	xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (item->button), pb);

    g_object_unref (pb);
}

G_MODULE_EXPORT /* EXPORT:item_apply_config */
void
item_apply_config (Item * item)
{
    GdkPixbuf *pb = NULL;

    if (item->icon_id == EXTERN_ICON && item->icon_path)
	pb = get_pixbuf_from_file (item->icon_path);
    else if (item->icon_id != STOCK_ICON)
	pb = get_pixbuf_by_id (item->icon_id);

    if (pb)
    {
	if (item->type == MENUITEM)
	    xfce_menubutton_set_pixbuf (XFCE_MENUBUTTON (item->button), pb);
	else
	    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (item->button), pb);

	g_object_unref (pb);
    }

    if (item->type == MENUITEM)
    {
	xfce_menubutton_set_text (XFCE_MENUBUTTON (item->button),
				  item->caption);
	menu_item_set_popup_size (item, settings.size);
    }

    if (item->tooltip && strlen (item->tooltip))
    {
	add_tooltip (item->button, item->tooltip);
    }
    else if (item->command && strlen (item->command))
    {
	if (item->type != MENUITEM)
	    add_tooltip (item->button, item->command);
	else
	    add_tooltip (item->button, NULL);
    }
    else
    {
	add_tooltip (item->button, _("Click mouse button 3 to change item"));
    }
}

/*  Menu items  
 *  ----------
*/
G_MODULE_EXPORT /* EXPORT:menu_item_new */
Item *
menu_item_new (PanelPopup * pp)
{
    Item *mi = g_new0 (Item, 1);

    mi->type = MENUITEM;

    mi->parent = pp;

    return mi;
}

G_MODULE_EXPORT /* EXPORT:create_addtomenu_item */
void
create_addtomenu_item (Item * mi)
{
    mi->button =
	xfce_menubutton_new_with_stock_icon (_("Add launcher"),
					     GTK_STOCK_ADD);
    gtk_widget_show (mi->button);
    gtk_button_set_relief (GTK_BUTTON (mi->button), GTK_RELIEF_NONE);

    add_tooltip (mi->button, _("Add new item"));

    /* signals */
    dnd_set_drag_dest (mi->button);

    g_signal_connect (mi->button, "drag_data_received",
		      G_CALLBACK (addtomenu_item_drop_cb), mi->parent);

    g_signal_connect (mi->button, "clicked",
		      G_CALLBACK (addtomenu_item_click_cb), mi->parent);

    menu_item_set_popup_size (mi, settings.size);
}

G_MODULE_EXPORT /* EXPORT:create_menu_item */
void
create_menu_item (Item * mi)
{
    mi->button = xfce_menubutton_new (NULL);
    gtk_widget_show (mi->button);
    gtk_button_set_relief (GTK_BUTTON (mi->button), GTK_RELIEF_NONE);

    item_apply_config (mi);

    /* signals */
    g_signal_connect (mi->button, "button-press-event",
		      G_CALLBACK (menu_item_press), mi);

    g_signal_connect (mi->button, "button-press-event",
		      G_CALLBACK (item_middle_click), mi);

    g_signal_connect (mi->button, "clicked", G_CALLBACK (item_click_cb), mi);

    dnd_set_drag_dest (mi->button);

    g_signal_connect (mi->button, "drag_data_received",
		      G_CALLBACK (item_drop_cb), mi);

    menu_item_set_popup_size (mi, settings.size);
}

G_MODULE_EXPORT /* EXPORT:menu_item_set_popup_size */
void
menu_item_set_popup_size (Item * mi, int size)
{
    gtk_widget_set_size_request (mi->button, -1,
				 popup_icon_size[size] + border_width);
}

/*  Panel item
 *  ----------
*/
Item *
panel_item_new (void)
{
    Item *pi = g_new0 (Item, 1);

    pi->type = PANELITEM;
    pi->button = xfce_iconbutton_new ();
    gtk_widget_show (pi->button);
    gtk_button_set_relief (GTK_BUTTON (pi->button), GTK_RELIEF_NONE);
    pi->with_popup = FALSE;

    item_apply_config (pi);

    g_signal_connect (pi->button, "clicked", G_CALLBACK (item_click_cb), pi);

    g_signal_connect (pi->button, "button-press-event",
		      G_CALLBACK (panel_item_press), pi);

    g_signal_connect (pi->button, "button-press-event",
		      G_CALLBACK (item_middle_click), pi);

    dnd_set_drag_dest (pi->button);
    g_signal_connect (pi->button, "drag-data-received",
		      G_CALLBACK (item_drop_cb), pi);

    g_object_ref (pi->button);
    gtk_object_sink (GTK_OBJECT (pi->button));
    
    return pi;
}


