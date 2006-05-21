/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
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
#include <libxfce4panel/xfce-panel-internal-plugin.h>
#include <libxfce4panel/xfce-panel-external-item.h>
#include <libxfce4panel/xfce-panel-enums.h>

#include "panel-item-manager.h"

#ifndef _
#define _(x) x
#endif

typedef struct _XfcePanelItemClass XfcePanelItemClass;

struct _XfcePanelItemClass
{
    /* info from .desktop file */
    char *plugin_name;

    char *name;
    char *comment;
    char *icon;

    guint unique:1;
    guint is_external:1;

    char *file; /* either executable or loadable module */
    
    /* using the plugin */
    int use_count;

    /* for loadable modules only */
    GModule *gmodule;
    XfcePanelPluginFunc construct;
    XfcePanelPluginCheck check;
};

static GHashTable *plugin_classes = NULL;

/* hash table */

static void
_free_item_class (XfcePanelItemClass *class)
{
    DBG ("Free item class: %s", class->plugin_name);

    if (class->gmodule != NULL)
        g_module_close (class->gmodule);
    
    g_free (class->plugin_name);
    g_free (class->name);
    g_free (class->comment);
    g_free (class->icon);

    g_free (class->file);

    g_free (class);
}

static void
_add_item_info_to_array (char *plugin_name, XfcePanelItemClass *class, 
                         gpointer data)
{
    GPtrArray *array = data;
    XfcePanelItemInfo *info;

    info = g_new0 (XfcePanelItemInfo, 1);
    
    info->name         = plugin_name;
    info->display_name = class->name;
    info->comment      = class->comment;
    
    /* for the item list in the 'Add Items' dialog */
    if (class->icon)
        info->icon = xfce_themed_icon_load (class->icon, 48);

    g_ptr_array_add (array, info);
}

/* plugin desktop files */

static char *
_plugin_name_from_filename (const char *file)
{
    const char *s, *p;
    char *name;

    if ((s = strrchr (file, G_DIR_SEPARATOR)))
        s++;
    else
        s = file;

    p = strrchr (file, '.');

    name = g_strndup (s, p - s);

    return name;
}

static XfcePanelItemClass *
_new_plugin_class_from_desktop_file (const char *file)
{
    XfcePanelItemClass *class = NULL;
    XfceRc *rc;
    char *name;

    DBG ("Plugin .desktop file: %s", file);
    
    name = _plugin_name_from_filename (file);
    
    if (g_hash_table_lookup (plugin_classes, name) != NULL)
    {
        DBG ("Already loaded");
        g_free (name);
        return NULL;
    }
    
    rc = xfce_rc_simple_open (file, TRUE);

    if (rc && xfce_rc_has_group (rc, "Xfce Panel"))
    {
        const char *value;
 
        xfce_rc_set_group (rc, "Xfce Panel");

        if ((value = xfce_rc_read_entry (rc, "X-XFCE-Exec", NULL)) &&
            g_file_test (value, G_FILE_TEST_EXISTS))

        {
            class = g_new0 (XfcePanelItemClass, 1);
            
            class->file = g_strdup (value);
            
            class->is_external = TRUE;

            DBG ("External plugin: %s", value);
        }
        else if ((value = xfce_rc_read_entry (rc, "X-XFCE-Module", NULL)) &&
                 g_file_test (value, G_FILE_TEST_EXISTS))
        {
            class = g_new0 (XfcePanelItemClass, 1);
            
            class->file = g_strdup (value);
            
            class->is_external = FALSE;

            DBG ("Internal plugin: %s", value);
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
_update_plugin_list (void)
{
    char **dirs, **d;
    gboolean datadir_used = FALSE;
    XfceKiosk *kiosk = NULL;
    gboolean use_user_config = TRUE;

    kiosk = xfce_kiosk_new ("xfce4-panel");
    use_user_config = xfce_kiosk_query (kiosk, "CustomizePanel");
    xfce_kiosk_free (kiosk);

    if (G_UNLIKELY (!use_user_config))
    {
        dirs = g_new (char*, 2);
        dirs[0] = g_strdup (DATADIR);
        dirs[1] = NULL;
    }
    else
    {
        dirs = xfce_resource_dirs (XFCE_RESOURCE_DATA);
    }
    
    for (d = dirs; *d != NULL || !datadir_used; ++d)
    {
        GDir *gdir;
        char *dirname;
        const char *file;
    
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

        while ((file = g_dir_read_name (gdir)) != NULL)
        {
            XfcePanelItemClass *class;
            char *path;

            if (!g_str_has_suffix (file, ".desktop"))
                continue;

            path = g_build_filename (dirname, file, NULL);
            class = _new_plugin_class_from_desktop_file (path);
            
            g_free (path);

            if (class)
            {
                DBG (" + class \"%s\": "
                     "name=%s, comment=%s, icon=%s, external=%d, path=%s", 
                     class->plugin_name,
                     class->name, class->comment, class->icon,
                     class->is_external, class->file);
                
                g_hash_table_insert (plugin_classes, 
                                     class->plugin_name, class);
            }
        }

        g_free (dirname);
        g_dir_close (gdir);

        if (*d == NULL)
            break;
    }

    g_strfreev (dirs);
}

/* public API */

void
xfce_panel_item_manager_init (void)
{
    plugin_classes = 
        g_hash_table_new_full ((GHashFunc) g_str_hash, 
                               (GEqualFunc) g_str_equal,
                               NULL,
                               (GDestroyNotify)_free_item_class);

    _update_plugin_list ();
}

void 
xfce_panel_item_manager_cleanup (void)
{
    g_hash_table_destroy (plugin_classes);

    plugin_classes = NULL;
}

static void
_decrease_use_count (GtkWidget *item, XfcePanelItemClass *class)
{
    if (class->use_count > 0)
        class->use_count--;
}

GtkWidget *
xfce_panel_item_manager_create_item (GdkScreen *screen, const char *name, 
                                     const char *id, int size, 
                                     XfceScreenPosition position)
{
    XfcePanelItemClass *class;
    GtkWidget *item = NULL;

    if ((class = g_hash_table_lookup (plugin_classes, name)) == NULL)
        return NULL;

    if (class->is_external)
    {
        item = xfce_external_panel_item_new (class->plugin_name, id, 
                                             class->name, class->file, 
                                             size, position);
    }
    else
    {
        if (!class->gmodule)
        {
            gpointer symbol;
            XfcePanelPluginFunc (*get_construct) (void);
            XfcePanelPluginCheck (*get_check) (void);
            
            if (!(class->gmodule = g_module_open (class->file, 0)))
            {
                g_critical ("Could not open \"%s\": %s", 
                            class->name, g_module_error ());
                return NULL;
            }

            if (!g_module_symbol (class->gmodule, 
                                  "xfce_panel_plugin_get_construct", &symbol))
            {
                g_critical ("Could not open \"%s\": %s", 
                            class->name, g_module_error ());
                g_module_close (class->gmodule);
                class->gmodule = NULL;
                return NULL;
            }
            
            get_construct = symbol;
            
            class->construct = get_construct ();
            
            if (g_module_symbol (class->gmodule, 
                                  "xfce_panel_plugin_get_check", &symbol))
            {
                get_check = symbol;
                
                class->check = get_check ();
            }
            else
            {
                class->check = NULL;
            }
        }

        if (!class->check || class->check(screen) == TRUE )
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
                          G_CALLBACK (_decrease_use_count), class);
    }

    return item;
}

static gboolean
_check_class_removal (gpointer key, XfcePanelItemClass *class)
{
    if (!g_file_test (class->file, G_FILE_TEST_EXISTS))
    {
        if (class->is_external)
            return TRUE;

        if (!class->use_count)
            return TRUE;
    }

    return FALSE;
}

static int
_compare_classes (gpointer *a, gpointer *b)
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

    if (!strcmp (class_a->plugin_name, "launcher"))
        return -1;

    if (!strcmp (class_b->plugin_name, "launcher"))
        return 1;

    return strcmp (class_a->name ? class_a->name : "x",
                   class_b->name ? class_b->name : "x");
}

GPtrArray *
xfce_panel_item_manager_get_item_info_list (void)
{
    GPtrArray *array;
    
    _update_plugin_list ();
    
    g_hash_table_foreach_remove (plugin_classes, (GHRFunc)_check_class_removal,
                                 NULL);
    
    array = g_ptr_array_sized_new (g_hash_table_size (plugin_classes));
    
    g_hash_table_foreach (plugin_classes, (GHFunc)_add_item_info_to_array,
                          array);

    g_ptr_array_sort (array, (GCompareFunc)_compare_classes);
    
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
            g_object_unref (info->icon);

        g_free (info);
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
