/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005-2006 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published 
 *  by the Free Software Foundation; either version 2 of the License, or
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
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
    char                 *plugin_name;
    char                 *name;
    char                 *comment;
    char                 *icon;
    guint                 unique:1;
    guint                 is_external:1;

    /* either executable or loadable module */
    char                 *file;

    /* for loadable modules only */
    int                   use_count;
    GModule              *gmodule;
    XfcePanelPluginFunc   construct;
    XfcePanelPluginCheck  check;
};

static GHashTable *plugin_classes = NULL;

/* hash table */

static void
free_item_class (XfcePanelItemClass *class)
{
    DBG ("Free item class: %s", class->plugin_name);

    if (class->gmodule != NULL)
        g_module_close (class->gmodule);
    
    g_free (class->plugin_name);
    g_free (class->name);
    g_free (class->comment);
    g_free (class->icon);

    g_free (class->file);

    panel_slice_free (XfcePanelItemClass, class);
}

static void
add_item_info_to_array (char *plugin_name, 
                        XfcePanelItemClass *class, 
                        gpointer data)
{
    GPtrArray         *array = data;
    XfcePanelItemInfo *info;

    info = panel_slice_new0 (XfcePanelItemInfo);
    
    info->name         = plugin_name;
    info->display_name = class->name;
    info->comment      = class->comment;
    
    /* for the item list in the 'Add Items' dialog */
    if (class->icon)
        info->icon = xfce_themed_icon_load (class->icon, 48);

    g_ptr_array_add (array, info);
}

static gboolean
check_class_removal (gpointer key, XfcePanelItemClass *class)
{
    if (!g_file_test (class->file, G_FILE_TEST_EXISTS))
    {
        if (class->is_external || !class->use_count)
            return TRUE;
    }

    return FALSE;
}

static int
compare_classes (gpointer *a, gpointer *b)
{
    XfcePanelItemClass *class_a, *class_b;
    
    if (!a || !(*a))
        return 1;

    if (!b || !(*b))
        return -1;

    if (*a == *b)
        return 0;

    class_a = *a;
    class_b = *b;

    if (strcmp (class_a->plugin_name, "launcher") == 0)
        return -1;

    if (strcmp (class_b->plugin_name, "launcher") == 0)
        return 1;

    return strcmp (class_a->name ? class_a->name : "z",
                   class_b->name ? class_b->name : "z");
}

/* plugin desktop files */

static char *
plugin_name_from_filename (const char *file)
{
    const char *s, *p;
    char       *name;

    if ((s = strrchr (file, G_DIR_SEPARATOR)) != NULL) 
    {
        s++;
    }
    else
    {
        s = file;
    }

    if ((p = strrchr (s, '.')) != NULL)
    {
        name = g_strndup (s, p - s);
    }
    else
    {
        name = g_strdup (s);
    }

    return name;
}

static XfcePanelItemClass *
create_item_class (const char *file, 
                   gboolean is_external)
{
    XfcePanelItemClass *class;

    class              = panel_slice_new0 (XfcePanelItemClass);
    class->file        = g_strdup (file);
    class->is_external = is_external;

    return class;
}

static XfcePanelItemClass *
new_plugin_class_from_desktop_file (const char *file)
{
    XfcePanelItemClass *class = NULL;
    XfceRc             *rc;
    char               *name;
    const char         *value;
    const char         *dir;
    char               *path;

    DBG ("Plugin .desktop file: %s", file);
    
    name = plugin_name_from_filename (file);
    
    if (g_hash_table_lookup (plugin_classes, name) != NULL)
    {
        DBG ("Already loaded");
        g_free (name);
        return NULL;
    }
    
    rc = xfce_rc_simple_open (file, TRUE);

    if (rc && xfce_rc_has_group (rc, "Xfce Panel"))
    {
        xfce_rc_set_group (rc, "Xfce Panel");

        if ((value = xfce_rc_read_entry (rc, "X-XFCE-Exec", NULL)) 
            && g_path_is_absolute (value)
            && g_file_test (value, G_FILE_TEST_EXISTS))

        {
            class = create_item_class (value, TRUE);
            DBG ("External plugin: %s", value);
        }
        else if ((value = xfce_rc_read_entry (rc, "X-XFCE-Module", NULL)))
        {
            if ((dir = xfce_rc_read_entry (rc, "X-XFCE-Module-Path", NULL)))
            {
                path = g_module_build_path (dir, value);

                if (g_path_is_absolute (path)
                    && g_file_test (path, G_FILE_TEST_EXISTS))
                {
                    class = create_item_class (path, FALSE);
                    
                    DBG ("Internal plugin: %s", path);
                }

                g_free (path);
            }
            else if (g_path_is_absolute (value)
                     && g_file_test (value, G_FILE_TEST_EXISTS))
            {
                class = create_item_class (value, FALSE);
                
                DBG ("Internal plugin: %s", value);
            }
        }

        if (class)
        {
            class->plugin_name = name;
            
            if ((value = xfce_rc_read_entry (rc, "Name", NULL)))
                class->name = g_strdup (value);
            else
                class->name = g_strdup (class->plugin_name);

            if ((value = xfce_rc_read_entry (rc, "Comment", NULL)))
                class->comment = g_strdup (value);

            if ((value = xfce_rc_read_entry (rc, "Icon", NULL)))
                class->icon = g_strdup (value);

            class->unique = 
                xfce_rc_read_bool_entry (rc, "X-XFCE-Unique", FALSE);
        }
        else
        {
            DBG ("No plugin class found");
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
    
    return class;
}

static void
update_plugin_list (void)
{
    char     **dirs;
    char     **d;
    gboolean   datadir_used = FALSE;

    if (G_UNLIKELY (!xfce_allow_panel_customization()))
    {
        dirs    = g_new (char*, 2);
        dirs[0] = g_strdup (DATADIR);
        dirs[1] = NULL;
    }
    else
    {
        dirs = xfce_resource_dirs (XFCE_RESOURCE_DATA);

        if (G_UNLIKELY(!dirs))
        {
            dirs    = g_new (char*, 2);
            dirs[0] = g_strdup (DATADIR);
            dirs[1] = NULL;
        }
    }
    
    for (d = dirs; *d != NULL || !datadir_used; ++d)
    {
        GDir               *gdir;
        char               *dirname;
        const char         *file;
        XfcePanelItemClass *class;
        char               *path;
    
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
        
        if (gdir)
        {
            while ((file = g_dir_read_name (gdir)) != NULL)
            {
                if (!g_str_has_suffix (file, ".desktop"))
                    continue;

                path = g_build_filename (dirname, file, NULL);

                class = new_plugin_class_from_desktop_file (path);
            
                g_free (path);

                if (class)
                {
                    DBG (" + class \"%s\": "
                         "name=%s, comment=%s, icon=%s, external=%d, path=%s", 
                         class->plugin_name ? class->plugin_name : "(null)",
                         class->name        ? class->name        : "(null)", 
                         class->comment     ? class->comment     : "(null)", 
                         class->icon        ? class->icon        : "(null)",
                         class->is_external, 
                         class->file        ? class->file        : "(null)");
                
                    g_hash_table_insert (plugin_classes, 
                                         class->plugin_name, 
                                         class);
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
load_module (XfcePanelItemClass *class)
{
    gpointer               symbol;
    XfcePanelPluginFunc  (*get_construct) (void);
    XfcePanelPluginCheck (*get_check)     (void);
            
    class->gmodule = g_module_open (class->file, G_MODULE_BIND_LOCAL);

    if (G_UNLIKELY (class->gmodule == NULL))
    {
        g_critical ("Could not open \"%s\": %s", 
                    class->name, g_module_error ());
        return FALSE;
    }

    if (!g_module_symbol (class->gmodule, 
                          "xfce_panel_plugin_get_construct", &symbol))
    {
        g_critical ("Could not open \"%s\": %s", 
                    class->name, g_module_error ());
        g_module_close (class->gmodule);
        class->gmodule = NULL;
        return FALSE;
    }
            
    get_construct    = symbol;
    class->construct = get_construct ();
            
    if (g_module_symbol (class->gmodule, 
                         "xfce_panel_plugin_get_check", &symbol))
    {
        get_check    = symbol;
        class->check = get_check ();
    }
    else
    {
        class->check = NULL;
    }

    return TRUE;
}

static void
decrease_use_count (GtkWidget *item, 
                    XfcePanelItemClass *class)
{
    if (class->use_count > 0)
        class->use_count--;
}

/* public API */

void
xfce_panel_item_manager_init (void)
{
    plugin_classes = g_hash_table_new_full ((GHashFunc) g_str_hash, 
                                            (GEqualFunc)  g_str_equal,
                                            NULL,
                                            (GDestroyNotify) free_item_class);

    update_plugin_list ();
}

void 
xfce_panel_item_manager_cleanup (void)
{
    g_hash_table_destroy (plugin_classes);

    plugin_classes = NULL;
}

GtkWidget *
xfce_panel_item_manager_create_item (GdkScreen          *screen, 
                                     const char         *name, 
                                     const char         *id, 
                                     int                 size, 
                                     XfceScreenPosition  position)
{
    XfcePanelItemClass *class;
    GtkWidget          *item = NULL;

    if ((class = g_hash_table_lookup (plugin_classes, name)) == NULL)
        return NULL;

    if (class->is_external)
    {
        item = xfce_external_panel_item_new (class->plugin_name, 
                                             id, 
                                             class->name, 
                                             class->file, 
                                             size, 
                                             position);
    }
    else
    {
        if (!class->gmodule && !load_module (class))
            return NULL;

        if (class->check == NULL || class->check(screen) == TRUE )
        {
            item = xfce_internal_panel_plugin_new (class->plugin_name, 
                                                   id, 
                                                   class->name, 
                                                   size, 
                                                   position, 
                                                   class->construct);
        }
    }

    if (item)
    {
        class->use_count++;
        g_signal_connect (item, "destroy", 
                          G_CALLBACK (decrease_use_count), class);
    }

    return item;
}

GPtrArray *
xfce_panel_item_manager_get_item_info_list (void)
{
    GPtrArray *array;
    
    update_plugin_list ();
    
    g_hash_table_foreach_remove (plugin_classes, 
                                 (GHRFunc)check_class_removal, NULL);
    
    array = g_ptr_array_sized_new (g_hash_table_size (plugin_classes));
    
    g_hash_table_foreach (plugin_classes, 
                          (GHFunc)add_item_info_to_array, array);

    g_ptr_array_sort (array, (GCompareFunc)compare_classes);
    
    return array;
}

void 
xfce_panel_item_manager_free_item_info_list (GPtrArray *info_list)
{
    int i;

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
xfce_panel_item_manager_is_available (const char *name)
{
    XfcePanelItemClass *class;
    
    if ((class = g_hash_table_lookup (plugin_classes, name)) == NULL)
        return FALSE;

    return (!(class->unique && class->use_count));
}
