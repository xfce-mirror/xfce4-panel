/*  xfce4
 *
 *  Copyright (C) 2004 Jasper Huijsmans (jasper@xfce.org)
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

#ifndef _XFCOMBO_H
#define _XFCOMBO_H

#include <gtk/gtk.h>

#ifdef HAVE_LIBDBH

/* FIXME: should be included from combo.h install by the module */
typedef struct _xfc_combo_info_t xfc_combo_info_t;

struct _xfc_combo_info_t
{
    GtkCombo *combo;
    GtkEntry *entry;
    gchar *active_dbh_file;
    gpointer cancel_user_data;
    gpointer activate_user_data;
    void (*cancel_func) (GtkEntry * entry, gpointer cancel_user_data);
    void (*activate_func) (GtkEntry * entry, gpointer activate_user_data);
    GList *list;
    GList *limited_list;
    GList *old_list;
};

typedef struct _xfc_combo_functions xfc_combo_functions;
struct _xfc_combo_functions
{
    /* exported: */
    gboolean (*xfc_is_in_history) (char *path2dbh_file, char *path2find);
    gboolean (*xfc_set_combo) (xfc_combo_info_t * combo_info, char *token);
    void (*xfc_set_blank) (xfc_combo_info_t * combo_info);
    void (*xfc_set_entry) (xfc_combo_info_t * combo_info, char *entry_string);
    void (*xfc_save_to_history) (char *path2dbh_file, char *path2save);
    void (*xfc_remove_from_history) (char *path2dbh_file, char *path2remove);
    void (*xfc_read_history) (xfc_combo_info_t * combo_info,
			      gchar * path2dbh_file);
    void (*xfc_clear_history) (xfc_combo_info_t * combo_info);
    xfc_combo_info_t *(*xfc_init_combo) (GtkCombo * combo);
    xfc_combo_info_t *(*xfc_destroy_combo) (xfc_combo_info_t * combo_info);
    /* imported (or null): */
    int (*extra_key_completion) (gpointer extra_key_data);
    gpointer extra_key_data;
};

#else /* HAVE_LIBDBH */
/* stub to keep compiler happy */
typedef struct _xfc_combo_info_t xfc_combo_info_t;

struct _xfc_combo_info_t
{
    GtkCombo *combo;
    GtkEntry *entry;
};

#endif /* HAVE_LIBDBH */

typedef void (*ComboCallback) (xfc_combo_info_t *info, gpointer data);

xfc_combo_info_t *create_completion_combo (ComboCallback completion_cb, 
    					   gpointer data);

void destroy_completion_combo (xfc_combo_info_t *info);

void completion_combo_set_text (xfc_combo_info_t *info, const char *text);

gboolean history_check_in_terminal (const char *command);

#endif /* _XFCOMBO_H */

