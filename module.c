/*  module.c
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (j.b.huijsmans@hetnet.nl)
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

/* Plugins
 * ----------------
 * External plugins are accessed by the gmodule interface. A plugin
 * must define several symbols:
 * - 'is_xfce_module' : this is only checked for existence.
 * - 'module_init' : this function takes a PanelModule as argument
 *    and fills it with the apropriate pointers.
 * 
 * Look at 'create_extern_module' function.
*/

#include "module.h"

#include "builtins.h"
#include "callbacks.h"
#include "icons.h"

PanelModule *panel_module_new(PanelGroup * pg)
{
    PanelModule *pm = g_new(PanelModule, 1);

    pm->parent = pg;
    pm->id = -1;
    pm->name = NULL;
    pm->dir = NULL;
    pm->gmodule = NULL;
    
    pm->caption = NULL;
    pm->data = NULL;
    pm->main = NULL;
    
    pm->interval = 0;
    pm->timeout_id = 0;
    pm->update = NULL;
    
    pm->free = NULL;
    
    pm->set_size = NULL;
    pm->set_style = NULL;
    pm->set_icon_theme = NULL;
    
    pm->configure = NULL;

    return pm;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Extern modules

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

static char *find_module(const char *name)
{
    const char *home = g_getenv("HOME");
    char dir[MAXSTRLEN + 1];
    char *path;

    snprintf(dir, MAXSTRLEN, "%s/.xfce4/panel/plugins", home);
    path = g_build_filename(dir, name, NULL);

    if(g_file_test(path, G_FILE_TEST_EXISTS))
        return path;
    else
        g_free(path);

    strcpy(dir, "/usr/share/xfce4/panel/plugins");
    path = g_build_filename(dir, name, NULL);

    if(g_file_test(path, G_FILE_TEST_EXISTS))
        return path;
    else
        g_free(path);

    strcpy(dir, "/usr/local/share/xfce4/panel/plugins");
    path = g_build_filename(dir, name, NULL);

    if(g_file_test(path, G_FILE_TEST_EXISTS))
        return path;
    else
        g_free(path);

    return NULL;
}

gboolean create_extern_module(PanelModule * pm)
{
    gpointer tmp;
    char *path;
    void (*init) (PanelModule * pm);

    g_return_val_if_fail(pm->name, FALSE);
    g_return_val_if_fail(g_module_supported(), FALSE);

    if(pm->dir)
        path = g_build_filename(pm->dir, pm->name, NULL);
    else
        path = find_module(pm->name);

    g_return_val_if_fail(path, FALSE);

    pm->gmodule = g_module_open(path, 0);
    g_free(path);

    if(!pm->gmodule || !g_module_symbol(pm->gmodule, "is_xfce_panel_module", &tmp))
    {
        g_printerr("Not a panel module: %s\n", pm->name);
        return FALSE;
    }

    if(g_module_symbol(pm->gmodule, "module_init", &tmp))
    {
        init = tmp;
        init(pm);
    }

    if(!pm->update || !pm->main)
    {
        g_module_close(pm->gmodule);
        pm->gmodule = NULL;

        return FALSE;
    }

    return TRUE;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Panel module interface

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

gboolean create_panel_module(PanelModule * pm)
{
    gboolean success;

    if(pm->id == EXTERN_MODULE)
        success = create_extern_module(pm);
    else if(pm->id > EXTERN_MODULE && pm->id < BUILTIN_MODULES)
        success = create_builtin_module(pm);
    else
        success = FALSE;

    if(success)
    {
        g_signal_connect(pm->main, "button-press-event",
                         G_CALLBACK(panel_group_press_cb), pm->parent);
	
	if (pm->parent)
	    panel_module_start(pm);
    }

    return success;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void panel_module_pack(PanelModule * pm, GtkBox * box)
{
    if(pm->pack)
        pm->pack(pm, box);
    else
        gtk_box_pack_start(box, pm->main, TRUE, TRUE, 0);
    
    if (pm->parent)
	panel_module_start(pm);
}

void panel_module_unpack(PanelModule * pm, GtkContainer * container)
{
    if(pm->unpack)
        pm->unpack(pm, container);
    else
        gtk_container_remove(container, pm->main);
    
    panel_module_stop(pm);
}

void panel_module_free(PanelModule * pm)
{
    if(pm->free)
        pm->free(pm);

    if(pm->gmodule)
    {
        g_module_close(pm->gmodule);
        g_free(pm->name);
        g_free(pm->dir);
    }

    g_free(pm);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void panel_module_start(PanelModule * pm)
{
    if(pm->interval > 0 && pm->timeout_id == 0)
        pm->timeout_id = g_timeout_add(pm->interval, (GSourceFunc) pm->update, pm);
}

void panel_module_stop(PanelModule * pm)
{
    if(pm->timeout_id > 0)
        g_source_remove(pm->timeout_id);
    
    pm->timeout_id = 0;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void panel_module_set_size(PanelModule * pm, int size)
{
    if(pm->set_size)
        pm->set_size(pm, size);
}

void panel_module_set_style(PanelModule * pm, int style)
{
    if(pm->set_style)
        pm->set_style(pm, style);
}

void panel_module_set_icon_theme(PanelModule * pm, const char *theme)
{
    if(pm->set_icon_theme)
        pm->set_icon_theme(pm, theme);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Configuration

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void panel_module_parse_xml(xmlNodePtr node, PanelModule * pm)
{
    xmlChar *value;

    value = xmlGetProp(node, (const xmlChar *)"id");

    if(!value)
        return;

    pm->id = atoi(value);

    g_free(value);

    if(pm->id == EXTERN_MODULE)
    {
        value = DATA(node);

        if(value)
            pm->name = (char *)value;
        else
            pm->id = EXTERN_MODULE - 1;
    }
}

void panel_module_write_xml(xmlNodePtr root, PanelModule *pm)
{
    xmlNodePtr node;
    char value[3];
    
    node = xmlNewTextChild(root, NULL, "Module", pm->name);
    
    snprintf(value, 3, "%d", pm->id);
    xmlSetProp(node, "id", value);
}