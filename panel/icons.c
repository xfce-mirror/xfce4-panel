/*  icons.c
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

#include "global.h"
#include "icons.h"
#include "xfce_support.h"

/* launcher and menu icons */

#include "icons/unknown.xpm"

#include "icons/edit.xpm"
#include "icons/file1.xpm"
#include "icons/file2.xpm"
#include "icons/games.xpm"
#include "icons/man.xpm"
#include "icons/multimedia.xpm"
#include "icons/network.xpm"
#include "icons/paint.xpm"
#include "icons/print.xpm"
#include "icons/schedule.xpm"
#include "icons/sound.xpm"
#include "icons/terminal.xpm"

/* system icons */

/* icons for the panel */
#include "icons/minilock.xpm"
#include "icons/miniinfo.xpm"
#include "icons/minipalet.xpm"
#include "icons/minipower.xpm"

#include "icons/handle.xpm"
#include "icons/addicon.xpm"
#include "icons/close.xpm"
#include "icons/iconify.xpm"
#include "icons/up.xpm"
#include "icons/down.xpm"

/* program icons */
#include "icons/diag_icon.xpm"
#include "icons/menu_icon.xpm"
#include "icons/xfce_icon.xpm"

/* module icons */

/* trashcan icons */
#include "icons/trash_full.xpm"
#include "icons/trash_empty.xpm"

static GdkPixbuf *xfce_icons[NUM_ICONS];
static GdkPixbuf *system_icons[SYS_ICONS];
static GdkPixbuf *module_icons[MODULE_ICONS];

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

static char *module_icon_names[] = {
    "trash_empty",
    "trash_full",
    "ppp_off",
    "ppp_on",
    "ppp_connecting"
};

static char *system_icon_names[] = {
    /* icons for the panel */
    "minilock",
    "miniinfo",
    "minipalet",
    "minipower",
    "handle",
    "addicon",
    "close",
    "iconify",
    "up",
    "down",
    /* program icons */
    "diag",
    "menu",
    "xfce"
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
	
    /* general icons */
    xfce_icons[UNKNOWN_ICON] =
        gdk_pixbuf_new_from_xpm_data((const char **)unknown_xpm);
    xfce_icons[EDIT_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)edit_xpm);
    xfce_icons[FILE1_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)file1_xpm);
    xfce_icons[FILE2_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)file2_xpm);
    xfce_icons[GAMES_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)games_xpm);
    xfce_icons[MAN_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)man_xpm);
    xfce_icons[MULTIMEDIA_ICON] =
        gdk_pixbuf_new_from_xpm_data((const char **)multimedia_xpm);
    xfce_icons[NETWORK_ICON] =
        gdk_pixbuf_new_from_xpm_data((const char **)network_xpm);
    xfce_icons[PAINT_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)paint_xpm);
    xfce_icons[PRINT_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)print_xpm);
    xfce_icons[SCHEDULE_ICON] =
        gdk_pixbuf_new_from_xpm_data((const char **)schedule_xpm);
    xfce_icons[SOUND_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)sound_xpm);
    xfce_icons[TERMINAL_ICON] =
        gdk_pixbuf_new_from_xpm_data((const char **)terminal_xpm);

    /* small buttons on central panel */
    system_icons[MINILOCK_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)minilock_xpm);
    system_icons[MINIINFO_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)miniinfo_xpm);
    system_icons[MINIPALET_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)minipalet_xpm);
    system_icons[MINIPOWER_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)minipower_xpm);

    /* move handles */
    system_icons[HANDLE_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)handle_xpm);
    system_icons[ADDICON_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)addicon_xpm);
    system_icons[CLOSE_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)close_xpm);
    system_icons[ICONIFY_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)iconify_xpm);

    /* POPUP BUTTOns */
    system_icons[UP_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)up_xpm);
    system_icons[DOWN_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)down_xpm);

    /* APP ICONS */
    system_icons[DIAG_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)diag_icon_xpm);
    system_icons[MENU_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)menu_icon_xpm);
    system_icons[XFCE_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)xfce_icon_xpm);

    /* TRASH ICONS */
    module_icons[TRASH_FULL_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)trash_full_xpm);
    module_icons[TRASH_EMPTY_ICON] = gdk_pixbuf_new_from_xpm_data((const char **)trash_empty_xpm);
}

GdkPixbuf *get_pixbuf_by_id(int id)
{
    GdkPixbuf *pb;

    if(id < UNKNOWN_ICON || id >= NUM_ICONS)
	id = UNKNOWN_ICON;

    pb = get_themed_pixbuf(xfce_icon_names[id]);

    if (!pb)
    {
	pb = xfce_icons[id];
	g_object_ref(pb);
    }
    
    return pb;
}

GdkPixbuf *get_pixbuf_from_file(const char* path)
{
	GdkPixbuf *pb = NULL;
	
	if (!g_file_test(path, G_FILE_TEST_EXISTS))
		return get_pixbuf_by_id(UNKNOWN_ICON);
	
	pb = gdk_pixbuf_new_from_file(path, NULL);
	
	if (pb && GDK_IS_PIXBUF(pb))
		return pb;
	else
		return get_pixbuf_by_id(UNKNOWN_ICON);
}

GdkPixbuf *get_system_pixbuf(int id)
{
    GdkPixbuf *pb;

    if(id < 0 || id >= SYS_ICONS)
	return get_pixbuf_by_id(UNKNOWN_ICON);

    pb = get_themed_pixbuf(system_icon_names[id]);

    if (!pb)
    {
	pb = system_icons[id];
	g_object_ref(pb);
    }
    
    return pb;
}

GdkPixbuf *get_scaled_pixbuf(GdkPixbuf *pb, int size)
{
	int w, h, neww, newh;
	GdkPixbuf *newpb;
	
	if (!pb || !GDK_IS_PIXBUF(pb))
	{
		GdkPixbuf *tmp = get_pixbuf_by_id(UNKNOWN_ICON);
		
		newpb = get_scaled_pixbuf(tmp, size);
		g_object_unref(tmp);
		
		return newpb;
	}
	
	w = gdk_pixbuf_get_width(pb);
	h = gdk_pixbuf_get_height(pb);
	
	if (size > w && size > h)
	{
		newpb = pb;
		g_object_ref(newpb);
		return newpb;
	}
	else if (h > w)
	{
		newh = size;
		neww = w * size / h;
	}
	else
	{
		neww = size;
		newh = h * size / w;
	}
	
	return gdk_pixbuf_scale_simple(pb, neww, newh, GDK_INTERP_BILINEAR);
}

GdkPixbuf *get_module_pixbuf(int id)
{
    GdkPixbuf *pb;

    if(id < 0 || id >= MODULE_ICONS)
	return get_pixbuf_by_id(UNKNOWN_ICON);

    pb = get_themed_pixbuf(module_icon_names[id]);

    if (!pb)
    {
	pb = module_icons[id];
	g_object_ref(pb);
    }
    
    return pb;
}

GdkPixbuf *get_themed_pixbuf(const char *name)
{
    GdkPixbuf *pb = NULL;
    char *theme = settings.icon_theme;
    char **icon_paths, **p;

    if(!theme)
        return NULL;

    icon_paths = get_theme_dirs();
    
    for(p = icon_paths; *p; p++)
    {
	char **suffix;

	for(suffix = icon_suffix; *suffix; suffix++)
	{
	    char *path =
		g_strconcat(*p, "/", theme, "/", name, ".", *suffix, NULL);

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


