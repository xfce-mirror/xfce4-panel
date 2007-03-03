/* $Id$
 *
 * Copyright (c) 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-item-iface.h>
#include <libxfce4panel/xfce-panel-macros.h>
#include <libxfce4panel/xfce-panel-internal-plugin.h>
#include <libxfce4panel/xfce-panel-external-item.h>
#include <libxfce4panel/xfce-panel-enums.h>
#include <libxfce4panel/xfce-panel-convenience.h>

#include "panel-item-manager.h"

#ifndef _
#define _(x) x
#endif

typedef struct _XfcePanelItemClass XfcePanelItemClass;

struct _XfcePanelItemClass
{
    /* info from .desktop file */
    gchar                *plugin_name;

    gchar                *name;
    gchar                *comment;
    gchar                *icon;

    guint                 unique : 1;
    guint                 is_external : 1;

    /* either executable or loadable module */
    gchar                *file;

    /* using the plugin */
    gint                  use_count;

    /* for loadable modules only */
    GModule              *gmodule;
    XfcePanelPluginFunc   construct;
    XfcePanelPluginCheck  check;
};

static GHashTable *plugin_klasses = NULL;

/* hash table */

static void
free_item_klass (XfcePanelItemClass *klass)
{
    DBG ("Free item klass: %s", klass->plugin_name);

    if (klass->gmodule != NULL)
        g_module_close (klass->gmodule);

    g_free (klass->plugin_name);
    g_free (klass->name);
    g_free (klass->comment);
    g_free (klass->icon);

    g_free (klass->file);

    panel_slice_free (XfcePanelItemClass, klass);
}

static void
add_item_info_to_array (char               *plugin_name,
                        XfcePanelItemClass *klass,
                        gpointer            data)
{
    GPtrArray         *array = data;
    XfcePanelItemInfo *info;

    info = panel_slice_new0 (XfcePanelItemInfo);

    info->name         = plugin_name;
    info->display_name = klass->name;
    info->comment      = klass->comment;

    /* for the item list in the 'Add Items' dialog */
    if (klass->icon)
        info->icon = xfce_themed_icon_load (klass->icon, 48);

    g_ptr_array_add (array, info);
}

static gboolean
check_klass_removal (gpointer            key,
                     XfcePanelItemClass *klass)
{
    if (!g_file_test (klass->file, G_FILE_TEST_EXISTS))
    {
        if (klass->is_external || !klass->use_count)
            return TRUE;
    }

    return FALSE;
}

static int
compare_klasses (gpointer *a,
                 gpointer *b)
{
    XfcePanelItemClass *klass_a, *klass_b;

    if (!a || !(*a))
        return 1;

    if (!b || !(*b))
        return -1;

    if (*a == *b)
        return 0;

    klass_a = *a;
    klass_b = *b;

    if (!strcmp (klass_a->plugin_name, "launcher"))
        return -1;

    if (!strcmp (klass_b->plugin_name, "launcher"))
        return 1;

    return strcmp (klass_a->name ? klass_a->name : "x",
                   klass_b->name ? klass_b->name : "x");
}

/* plugin desktop files */

static gchar *
plugin_name_from_filename (const gchar *file)
{
    const gchar *s, *p;
    gchar       *name;

    if ((s = strrchr (file, G_DIR_SEPARATOR)) != NULL)
        s++;
    else
        s = file;

    if ((p = strrchr (s, '.')) != NULL)
        name = g_strndup (s, p - s);
    else
        name = g_strdup (s);

    return name;
}

static XfcePanelItemClass *
create_item_klass (const gchar *file,
                   gboolean     is_external)
 {
    XfcePanelItemClass *klass;

    klass              = panel_slice_new0 (XfcePanelItemClass);
    klass->file        = g_strdup (file);
    klass->is_external = is_external;

    return klass;
}

static XfcePanelItemClass *
new_plugin_klass_from_desktop_file (const gchar *file)
{
    XfcePanelItemClass *klass = NULL;
    XfceRc             *rc;
    gchar              *name;
    const gchar        *value;
    const gchar        *dir;
    gchar              *path;

    DBG ("Plugin .desktop file: %s", file);

    name = plugin_name_from_filename (file);

    if (g_hash_table_lookup (plugin_klasses, name) != NULL)
    {
        DBG ("Already loaded");
        g_free (name);
        return NULL;
    }

    rc = xfce_rc_simple_open (file, TRUE);

    if (rc && xfce_rc_has_group (rc, "Xfce Panel"))
    {
        xfce_rc_set_group (rc, "Xfce Panel");

        if ((value = xfce_rc_read_entry (rc, "X-XFCE-Exec", NULL)) &&
            g_file_test (value, G_FILE_TEST_EXISTS))

        {
            klass = create_item_klass (value, TRUE);

            DBG ("External plugin: %s", value);
        }
        else if ((value = xfce_rc_read_entry (rc, "X-XFCE-Module", NULL)))
        {
            if (g_file_test (value, G_FILE_TEST_EXISTS))
            {
                klass = create_item_klass (value, FALSE);

                DBG ("Internal plugin: %s", value);
            }
            else if ((dir = xfce_rc_read_entry (rc, "X-XFCE-Module-Path",
                                                NULL)))
            {
                path = g_module_build_path (dir, value);

                if (g_file_test (path, G_FILE_TEST_EXISTS))
                {
                    klass = create_item_klass (path, FALSE);

                    DBG ("Internal plugin: %s", path);
                }

                g_free (path);
            }
        }

        if (klass)
        {
            klass->plugin_name = name;

            if ((value = xfce_rc_read_entry (rc, "Name", NULL)))
                klass->name = g_strdup (value);
            else
                klass->name = g_strdup (klass->plugin_name);

            if ((value = xfce_rc_read_entry (rc, "Comment", NULL)))
                klass->comment = g_strdup (value);

            if ((value = xfce_rc_read_entry (rc, "Icon", NULL)))
                klass->icon = g_strdup (value);

            klass->unique =
                xfce_rc_read_bool_entry (rc, "X-XFCE-Unique", FALSE);
        }
        else
        {
            DBG ("No plugin klass found");
            g_free (name);
        }
    }
    else
    {
        DBG ("No Xfce Panel group");
        g_free (name);
    }

    if (rc)
        xfce_rc_close (rc);

    return klass;
}

static void
update_plugin_list (void)
{
    gchar             **dirs, **d;
    gboolean            datadir_used = FALSE;
    GDir               *gdir;
    gchar              *dirname, *path;
    const gchar        *file;
    XfcePanelItemClass *klass;

    if (G_UNLIKELY (!xfce_allow_panel_customization()))
    {
        dirs    = g_new (gchar*, 2);
        dirs[0] = g_strdup (DATADIR);
        dirs[1] = NULL;
    }
    else
    {
        dirs = xfce_resource_dirs (XFCE_RESOURCE_DATA);

        if (G_UNLIKELY(!dirs))
        {
            dirs    = g_new (gchar*, 2);
            dirs[0] = g_strdup (DATADIR);
            dirs[1] = NULL;
        }
    }

    for (d = dirs; *d != NULL || !datadir_used; ++d)
    {
        /* check if resource dirs include our prefix */
        if (*d == NULL)
        {
            dirname = g_build_filename (DATADIR, "xfce4", "panel-plugins",
                                        NULL);
            datadir_used = TRUE;
        }
        else
        {
            if (strcmp (DATADIR, *d) == 0)
                datadir_used = TRUE;

            dirname = g_build_filename (*d, "xfce4", "panel-plugins", NULL);
        }

        gdir = g_dir_open (dirname, 0, NULL);

        DBG (" + directory: %s", dirname);

        if (!gdir)
        {
            g_free (dirname);
            continue;
        }

        if (gdir)
        {
            while ((file = g_dir_read_name (gdir)) != NULL)
            {
                if (!g_str_has_suffix (file, ".desktop"))
                    continue;

                path  = g_build_filename (dirname, file, NULL);
                klass = new_plugin_klass_from_desktop_file (path);

                g_free (path);

                if (klass)
                {
                    DBG (" + klass \"%s\": "
                         "name=%s, comment=%s, icon=%s, external=%d, path=%s",
                         klass->plugin_name ? klass->plugin_name : "(null)",
                         klass->name        ? klass->name        : "(null)", 
                         klass->comment     ? klass->comment     : "(null)", 
                         klass->icon        ? klass->icon        : "(null)",
                         klass->is_external, 
                         klass->file        ? klass->file        : "(null)");

                    g_hash_table_insert (plugin_klasses,
                                         klass->plugin_name, klass);
                }
            }

            g_dir_close (gdir);
        }

        g_free (dirname);

        if (*d == NULL)
            break;
    }

    g_strfreev (dirs);
}

static gboolean
load_module (XfcePanelItemClass *klass)
{
    gpointer               symbol;
    XfcePanelPluginFunc  (*get_construct) (void);
    XfcePanelPluginCheck (*get_check)     (void);

    klass->gmodule = g_module_open (klass->file, G_MODULE_BIND_LOCAL);

    if (G_UNLIKELY (klass->gmodule == NULL))
    {
        g_critical ("Could not open \"%s\": %s",
                    klass->name, g_module_error ());
        return FALSE;
    }

    if (G_UNLIKELY (!g_module_symbol (klass->gmodule,
                                      "xfce_panel_plugin_get_construct",
                                      &symbol)))
    {
        g_critical ("Could not open \"%s\": %s",
                    klass->name, g_module_error ());

        g_module_close (klass->gmodule);
        klass->gmodule = NULL;

        return FALSE;
    }

    get_construct    = symbol;
    klass->construct = get_construct ();

    if (g_module_symbol (klass->gmodule,
                         "xfce_panel_plugin_get_check", &symbol))
    {
        get_check    = symbol;
        klass->check = get_check ();
    }
    else
    {
        klass->check = NULL;
    }

    return TRUE;
}

static void
decrease_use_count (GtkWidget          *item,
                    XfcePanelItemClass *klass)
{
    if (klass->use_count > 0)
        klass->use_count--;
}

/* public API */

void
xfce_panel_item_manager_init (void)
{
    plugin_klasses = g_hash_table_new_full ((GHashFunc) g_str_hash,
                                            (GEqualFunc) g_str_equal,
                                            NULL,
                                            (GDestroyNotify) free_item_klass);

    update_plugin_list ();
}

void
xfce_panel_item_manager_cleanup (void)
{
    g_hash_table_destroy (plugin_klasses);

    plugin_klasses = NULL;
}

GtkWidget *
xfce_panel_item_manager_create_item (GdkScreen          *screen,
                                     const gchar        *name,
                                     const gchar        *id,
                                     gint                size,
                                     XfceScreenPosition  position)
{
    XfcePanelItemClass *klass;
    GtkWidget          *item = NULL;

    if ((klass = g_hash_table_lookup (plugin_klasses, name)) == NULL)
        return NULL;

    if (klass->is_external)
    {
        item = xfce_external_panel_item_new (klass->plugin_name, id,
                                             klass->name, klass->file,
                                             size, position);
    }
    else
    {
        if (!klass->gmodule && !load_module (klass))
            return NULL;

        if (klass->check == NULL || klass->check(screen) == TRUE )
        {
            item = xfce_internal_panel_plugin_new (klass->plugin_name,
                                                   id,
                                                   klass->name,
                                                   size,
                                                   position,
                                                   klass->construct);
        }
    }

    if (item)
    {
        klass->use_count++;
        g_signal_connect (G_OBJECT (item), "destroy",
                          G_CALLBACK (decrease_use_count), klass);
    }

    return item;
}

GPtrArray *
xfce_panel_item_manager_get_item_info_list (void)
{
    GPtrArray *array;

    update_plugin_list ();

    g_hash_table_foreach_remove (plugin_klasses, (GHRFunc)check_klass_removal,
                                 NULL);

    array = g_ptr_array_sized_new (g_hash_table_size (plugin_klasses));

    g_hash_table_foreach (plugin_klasses, (GHFunc)add_item_info_to_array,
                          array);

    g_ptr_array_sort (array, (GCompareFunc)compare_klasses);

    return array;
}

void
xfce_panel_item_manager_free_item_info_list (GPtrArray *info_list)
{
    gint i;

    for (i = 0; i < info_list->len; ++i)
    {
        XfcePanelItemInfo *info = g_ptr_array_index (info_list, i);

        if (info->icon)
            g_object_unref (G_OBJECT (info->icon));

        panel_slice_free (XfcePanelItemInfo, info);
    }

    g_ptr_array_free (info_list, TRUE);
}

gboolean
xfce_panel_item_manager_is_available (const gchar *name)
{
    XfcePanelItemClass *klass;

    if ((klass = g_hash_table_lookup (plugin_klasses, name)) == NULL)
        return FALSE;

    return (!(klass->unique && klass->use_count));
}
