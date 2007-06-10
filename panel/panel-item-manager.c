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
#include <gmodule.h>
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

    /* close the module if needed */
    if (klass->gmodule != NULL)
        g_module_close (klass->gmodule);

    /* cleanup */
    g_free (klass->plugin_name);
    g_free (klass->name);
    g_free (klass->comment);
    g_free (klass->icon);
    g_free (klass->file);

    /* free the structure */
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
new_plugin_klass_from_desktop_file (const gchar *directory,
                                    const gchar *filename)
{
    XfcePanelItemClass *klass = NULL;
    XfceRc             *rc;
    gchar              *name, *file;
    const gchar        *value, *p;
    const gchar        *dir;
    gchar              *path;

    /* create the full desktop filename */
    file = g_build_filename (directory, filename, NULL);
    
    /* get the plugin (fallback) name */
    /* NOTE: this cannot fail since the filename passed the suffix test */
    p = strrchr (filename, '.');
    name = g_strndup (filename, p - filename);
   
    /* check if the plugin already exisits in the table */
    if (G_UNLIKELY (g_hash_table_lookup (plugin_klasses, name) != NULL))
        goto already_exists;

    /* open the desktop file */
    rc = xfce_rc_simple_open (file, TRUE);

    if (G_LIKELY (rc != NULL))
    {
        if (G_LIKELY (xfce_rc_has_group (rc, "Xfce Panel")))
        {
            /* set the xfce panel group */
            xfce_rc_set_group (rc, "Xfce Panel");

            /* check if this is an external plugin and if it exists */
            if ((value = xfce_rc_read_entry (rc, "X-XFCE-Exec", NULL)) &&
                g_path_is_absolute (value) &&
                g_file_test (value, G_FILE_TEST_EXISTS))

            {
                klass = create_item_klass (value, TRUE);

                DBG ("External plugin: %s", value);
            }
            /* an internal plugin then ... ? */
            else if (G_LIKELY ((value = xfce_rc_read_entry (rc, "X-XFCE-Module", NULL))))
            {
                /* read the module path */
                dir = xfce_rc_read_entry (rc, "X-XFCE-Module-Path", NULL);
                
                if (G_LIKELY (dir != NULL))
                {
                    /* build the module filename */
                    path = g_module_build_path (dir, value);
                    
                    /* test if the module exists */
                    if (G_LIKELY (g_file_test (path, G_FILE_TEST_EXISTS)))
                    {
                        klass = create_item_klass (path, FALSE);
                        
                        DBG ("Internal plugin: %s", path);
                    }
                    
                    /* cleanup */
                    g_free (path);
                }
                else
                {
                    g_warning ("Internal plugins need the \"X-XFCE-Module-Path\" entry to work properly.");
                }
            }
            else
            {
                g_warning ("No X-XFCE-{Module,Exec} entry found in \"%s\".", file);
            }
        }
        
        if (G_LIKELY (klass != NULL))
        {
            /* set the plugin name */
            klass->plugin_name = name;
            
            /* set the plugin name with our generated name as fallback */
            value = xfce_rc_read_entry (rc, "Name", name);
            klass->name = g_strdup (value);
            
            /* reset the name since it is now pointed to klass->plugin_name */
            name = NULL;
                
            /* set a comment from the desktop file */
            value = xfce_rc_read_entry (rc, "Comment", NULL);
            klass->comment = g_strdup (value);
                
            /* set the plugin icon */
            value = xfce_rc_read_entry (rc, "Icon", NULL);
            klass->icon = g_strdup (value);
            
            /* whether this plugin can only be placed once on the panel(s) */
            klass->unique = xfce_rc_read_bool_entry (rc, "X-XFCE-Unique", FALSE);
        }
        else
        {
            g_warning ("Failed to create plugin \"%s\"", name);
        }
        
        /* close the rc file */
        xfce_rc_close (rc);
    }

    already_exists:
        
    /* cleanup */
    g_free (name);
    g_free (file);

    return klass;
}

static void
update_plugin_list (void)
{
    gchar              **directories;
    gint                 n;
    GDir                *gdir;
    gchar               *dirname;
    const gchar         *file;
    XfcePanelItemClass  *klass;

    if (G_LIKELY (xfce_allow_panel_customization() == TRUE))
    {
        /* if panel customization is allowed, we search all resource dirs */
        directories = xfce_resource_dirs (XFCE_RESOURCE_DATA);

        /* check if the DATADIR is in the list */
        for (n = 0; directories[n] != NULL; ++n)
            if (strcmp (directories[n], DATADIR) == 0)
                break;

        if (G_UNLIKELY (directories[n] == NULL))
        {
            /* append the datadir path */
            directories      = g_realloc (directories, (n + 2) * sizeof (gchar*));
            directories[n]   = g_strdup (DATADIR);
            directories[n+1] = NULL;
        }
    }
    else 
    {
        /* only append the data directory */
        directories    = g_new0 ( char*, 2 );
        directories[0] = g_strdup (DATADIR);
        directories[1] = NULL;
    }

    /* walk through the directories */
    for (n = 0; directories[n] != NULL; ++n)
    {
        /* build the plugins directory name */
        dirname = g_build_filename (directories[n], "xfce4", "panel-plugins", NULL);
        
        /* open the directory */
        gdir = g_dir_open (dirname, 0, NULL);

        DBG (" + directory: %s", dirname);

        if (G_LIKELY (gdir != NULL))
        {
            /* walk though all the files in the directory */
            while ((file = g_dir_read_name (gdir)) != NULL)
            {
                /* continue if it's not a .desktop file */
                if (!g_str_has_suffix (file, ".desktop"))
                    continue;

                /* try to create a klass */
                klass = new_plugin_klass_from_desktop_file (dirname, file);

                if (G_LIKELY (klass))
                {
                    DBG (" + klass \"%s\": "
                         "name=%s, comment=%s, icon=%s, external=%d, path=%s",
                         klass->plugin_name ? klass->plugin_name : "(null)",
                         klass->name        ? klass->name        : "(null)", 
                         klass->comment     ? klass->comment     : "(null)", 
                         klass->icon        ? klass->icon        : "(null)",
                         klass->is_external, 
                         klass->file        ? klass->file        : "(null)");

                    /* insert the class in the hash table */
                    g_hash_table_insert (plugin_klasses, klass->plugin_name, klass);
                }
            }

            /* close the directory */
            g_dir_close (gdir);
        }

        /* cleanup */
        g_free (dirname);
    }

    /* cleanup */
    g_strfreev (directories);
}

static gboolean
load_module (XfcePanelItemClass *klass)
{
    gpointer               symbol;
    XfcePanelPluginFunc  (*get_construct) (void);
    XfcePanelPluginCheck (*get_check)     (void);

    /* try to open the module */
    klass->gmodule = g_module_open (klass->file, G_MODULE_BIND_LOCAL);

    /* check */
    if (G_UNLIKELY (klass->gmodule == NULL))
    {
        /* show a warning */
        g_critical ("Could not open module \"%s\" (%s): %s",
                    klass->name, klass->file, g_module_error ());

        return FALSE;
    }

    /* check */
    if (G_UNLIKELY (!g_module_symbol (klass->gmodule,
                                      "xfce_panel_plugin_get_construct",
                                      &symbol)))
    {
        /* show a warning */
        g_critical ("Could not open symbol in module \"%s\" (%s): %s",
                    klass->name, klass->file, g_module_error ());

        /* close the module */
        g_module_close (klass->gmodule);
        klass->gmodule = NULL;

        return FALSE;
    }

    get_construct    = symbol;
    klass->construct = get_construct ();

    /* check of the check function ^_^ */
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
