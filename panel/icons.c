/*  icons.c
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (huysmans@users.sourceforge.net)
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

#include "global.h"
#include "icons.h"
#include "xfce_support.h"

/* program icons */
#include "icons/diag_icon.xpm"
#include "icons/menu_icon.xpm"
#include "icons/xfce_icon.xpm"

/* icon themes */
static char *icon_suffix[] = {
    "png",
    "xpm",
    NULL
};

static char *xfce_icon_names[] = {
    "unknown",
    "edit",
    "file1",
    "file2",
    "games",
    "man",
    "multimedia",
    "network",
    "paint",
    "print",
    "schedule",
    "sound",
    "terminal"
};

static char *minibutton_names[] = {
    /* icons for the panel */
    "minilock",
    "miniinfo",
    "minipalet",
    "minipower",
};

static void set_icon_names()
{
    int i = 0;

    icon_names[0] = _("Default");
    icon_names[++i] = _("Editor");
    icon_names[++i] = _("File management");
    icon_names[++i] = _("Utilities");
    icon_names[++i] = _("Games");
    icon_names[++i] = _("Help browser");
    icon_names[++i] = _("Multimedia");
    icon_names[++i] = _("Network");
    icon_names[++i] = _("Graphics");
    icon_names[++i] = _("Printer");
    icon_names[++i] = _("Productvity");
    icon_names[++i] = _("Sound");
    icon_names[++i] = _("Terminal");
}

void create_builtin_pixbufs(void)
{
    set_icon_names();
}

GdkPixbuf *get_pixbuf_by_id(int id)
{
    GdkPixbuf *pb;

    if(id < UNKNOWN_ICON || id >= NUM_ICONS)
        id = UNKNOWN_ICON;

    return get_themed_pixbuf(xfce_icon_names[id]);
}

GdkPixbuf *get_pixbuf_from_file(const char *path)
{
    GdkPixbuf *pb = NULL;

    if(!g_file_test(path, G_FILE_TEST_EXISTS))
        return get_pixbuf_by_id(UNKNOWN_ICON);

    pb = gdk_pixbuf_new_from_file(path, NULL);

    if(pb && GDK_IS_PIXBUF(pb))
        return pb;
    else
        return get_pixbuf_by_id(UNKNOWN_ICON);
}

GdkPixbuf *get_system_pixbuf(int id)
{
    GdkPixbuf *pb = NULL;

    /* APP ICONS */
    if (id == DIAG_ICON)
	pb = gdk_pixbuf_new_from_xpm_data((const char **)diag_icon_xpm);
    else if (id == MENU_ICON)
        pb = gdk_pixbuf_new_from_xpm_data((const char **)menu_icon_xpm);
    else if (id == XFCE_ICON)
        pb = gdk_pixbuf_new_from_xpm_data((const char **)xfce_icon_xpm);

    if (!pb)
	pb = get_pixbuf_by_id(UNKNOWN_ICON);
}

GdkPixbuf *get_minibutton_pixbuf(int id)
{
    GdkPixbuf *pb, *tmp;

    if(id < 0 || id >= MINIBUTTONS)
        return get_pixbuf_by_id(UNKNOWN_ICON);

    pb = get_themed_pixbuf(minibutton_names[id]);

    if(!pb)
        pb = get_pixbuf_by_id(UNKNOWN_ICON);

    tmp = pb;
    pb = get_scaled_pixbuf(tmp, minibutton_size[settings.size]);
    g_object_unref(tmp);

    return pb;
}

GdkPixbuf *get_scaled_pixbuf(GdkPixbuf * pb, int size)
{
    int w, h, neww, newh;
    GdkPixbuf *newpb;

    if(!pb || !GDK_IS_PIXBUF(pb))
    {
        GdkPixbuf *tmp = get_pixbuf_by_id(UNKNOWN_ICON);

        newpb = get_scaled_pixbuf(tmp, size);
        g_object_unref(tmp);

        return newpb;
    }

    w = gdk_pixbuf_get_width(pb);
    h = gdk_pixbuf_get_height(pb);

    if(size > w && size > h)
    {
        newpb = pb;
        g_object_ref(newpb);
        return newpb;
    }
    else if(h > w)
    {
        newh = size;
        neww = ((double)w * (double)size / (double)h);
    }
    else
    {
        neww = size;
        newh = ((double)h * (double)size / (double)w);
    }

    return gdk_pixbuf_scale_simple(pb, neww, newh, GDK_INTERP_BILINEAR);
}

static GdkPixbuf *_get_themed_pixbuf(const char *name, const char *theme)
{
    GdkPixbuf *pb = NULL;
    char **icon_paths, **p;
    const char *real_theme;
    char *path = NULL;

    if(theme)
	real_theme = theme;
    else
        real_theme = DEFAULT_THEME;
    
    icon_paths = get_theme_dirs();

    for(p = icon_paths; *p; p++)
    {
        char **suffix;

        for(suffix = icon_suffix; *suffix; suffix++)
        {
            path = g_strconcat(*p, "/", real_theme, "/", name, ".", *suffix, NULL);

            if(g_file_test(path, G_FILE_TEST_EXISTS))
                pb = gdk_pixbuf_new_from_file(path, NULL);

            g_free(path);

            if(pb)
                break;
        }

        if(pb)
            break;
    }

    g_strfreev(icon_paths);

    return pb;
}

GdkPixbuf *get_themed_pixbuf(const char *name)
{
    GdkPixbuf *pb = NULL;

    pb = _get_themed_pixbuf(name, settings.theme);

    if (!pb && settings.theme && !strequal(DEFAULT_THEME, settings.theme))
	pb = _get_themed_pixbuf(name, DEFAULT_THEME);

    if (!pb)
    {
	pb = get_pixbuf_by_id(UNKNOWN_ICON);
	g_printerr("xfce: couldn't find icon: %s\n", name);
    }
    
    return pb;
}
