/*  $Id$
 *
 *  Copyright 2003-2004 Jasper Huijsmans (jasper@xfce.org)
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

#ifndef __XFCE_ITEM_DIALOG_H__
#define __XFCE_ITEM_DIALOG_H__

#include <gmodule.h>
#include <panel/item.h>

/* CommandOptions
 * --------------
 * For use in property dialogs. Consists of:
 * - Entry to hold command. 
 * - Checkbutton to run in terminal.
 * - Checkbutton to use startup notification (if available at compile time).
 *
 * You can set a callback that will be run when something changes.
*/
typedef struct _CommandOptions CommandOptions;

struct _CommandOptions
{
    /* add this to your dialog */
    GtkWidget *base;

    /* use the functions declared below to change the contents of 
     * the members of this struct */

    GtkWidget *command_entry;
    GtkWidget *term_checkbutton;
    GtkWidget *sn_checkbutton;

    void (*on_change) (const char *command, gboolean in_term, gboolean use_sn,
		       gpointer data);
    gpointer data;
};

G_MODULE_IMPORT CommandOptions *create_command_options (GtkSizeGroup * sg);

G_MODULE_IMPORT void destroy_command_options (CommandOptions * opts);

G_MODULE_IMPORT void command_options_set_command (CommandOptions * opts, const char *command,
				  gboolean in_term, gboolean use_sn);

G_MODULE_IMPORT void command_options_set_callback (CommandOptions * opts,
				   void (*callback) (const char *, gboolean,
						     gboolean, gpointer),
				   gpointer data);

G_MODULE_IMPORT void command_options_get_command (CommandOptions * opts, char **command,
				  gboolean * in_term, gboolean * use_sn);


/* IconOptions
 * --------------
 * For use in property dialogs. Consists of:
 * - Preview frame for the icon (allows dropping of image files).
 * - Optional option menu for builtin, themed icons.
 * - Entry to hold file name. Gives access to xfmime-edit when that is
 *   installed.
 *
 * You can set a callback that will be run when the icon is changed.
*/
typedef struct _IconOptions IconOptions;

struct _IconOptions
{
    /* add this to your dialog */
    GtkWidget *base;

    /* use the functions declared below to change the contents of 
     * the members of this struct */

    GtkWidget *icon_menu;
    GtkWidget *icon_entry;
    GtkWidget *image;

    /* complex signal handling needed */
    int id_sig;
    int icon_id;
    char *saved_path;

    /* callback */
    void (*on_change) (int icon_id, const char *icon_path, gpointer data);
    gpointer data;
};

G_MODULE_IMPORT IconOptions *create_icon_options (GtkSizeGroup * sg, gboolean use_builtins);

G_MODULE_IMPORT void destroy_icon_options (IconOptions * opts);

G_MODULE_IMPORT void icon_options_set_icon (IconOptions * opts, int id, const char *path);

G_MODULE_IMPORT void icon_options_set_callback (IconOptions * opts,
				void (*callback) (int, const char *,
						  gpointer), gpointer data);

G_MODULE_IMPORT void icon_options_get_icon (IconOptions * opts, int *id, char **path);


/* PanelItem and MenuItem dialogs
 * ------------------------------
 * These functions should only be used by the panel 
*/
G_MODULE_IMPORT void panel_item_create_options (Control * control, GtkContainer * container,
				GtkWidget * done);

G_MODULE_IMPORT void edit_menu_item_dialog (Item * mi);

G_MODULE_IMPORT void add_menu_item_dialog (PanelPopup * pp);

G_MODULE_IMPORT void destroy_menu_dialog (void);

#endif /* __XFCE_ITEM_DIALOG_H__ */
