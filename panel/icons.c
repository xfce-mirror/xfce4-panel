/*  $Id$
 *  
 *  Copyright (C) 2002-2004 Jasper Huijsmans (jasper@xfce.org)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4util/libxfce4util.h>

#include "xfce.h"
#include "icons/xfce4-panel-icon.h"

#ifdef HAVE_GDK_PIXBUF_NEW_FROM_STREAM
#define gdk_pixbuf_new_from_inline gdk_pixbuf_new_from_stream
#endif

const char *icon_names[NUM_ICONS];

/* icon themes */
/* TODO: Someone please get me a list of names that give a fully themed panel 
 * for all major icon themes.
 */
static char *xfce_icon_names[][4] = {
    {"xfce-unknown", "gnome-fs-executable", "exec", NULL},
    {"xfce-edit", "gedit-icon", "edit", NULL},
    {"xfce-filemanager", "file-manager", "folder", NULL},
    {"xfce-utils", "gnome-util", "utilities", NULL},
    {"xfce-games", "gnome-joystick", "games", NULL},
    {"xfce-man", "gnome-help", "help", NULL},
    {"xfce-multimedia", "gnome-multimedia", "multimedia", NULL},
    {"xfce-internet", "gnome-globe", "web-browser", NULL},
    {"xfce-graphics", "gnome-graphics", "graphics", NULL},
    {"xfce-printer", "gnome-dev-printer", "printer", NULL},
    {"xfce-schedule", "gnome-month", "productivity", NULL},
    {"xfce-sound", "gnome-audio", "sound", NULL},
    {"xfce-terminal", "gnome-terminal", "terminal", NULL},
};

/* icons for the panel */
/* TODO: add kde names */
static char *minibutton_names[][3] = {
    {"xfce-system-lock", "gnome-lockscreen", NULL},
    {"xfce-system-info", "gnome-info", NULL},
    {"xfce-system-settings", "gnome-settings", NULL},
    {"xfce-system-exit", "gnome-logout", NULL},
};

void
icons_init (void)
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
    icon_names[++i] = _("Productivity");
    icon_names[++i] = _("Sound");
    icon_names[++i] = _("Terminal");
}

GdkPixbuf *
themed_pixbuf_from_name_list (char **namelist, int size)
{
    GdkPixbuf *pb = NULL, *fallback = NULL;
    
    for (; namelist[0] != NULL; ++namelist)
    {
	char *iconname = xfce_themed_icon_lookup (namelist[0], size);

	if (iconname)
	{
	    gboolean is_fallback = FALSE;
	    
	    is_fallback = strstr (iconname, "/hicolor/") != NULL 
			  || strstr (iconname, "/pixmaps/") != NULL;

	    DBG ("Icon: %s %s\n", iconname, is_fallback ? "(fallback)" : "");

	    if (is_fallback)
	    {
		if (!fallback)
		{
		    fallback = 
			xfce_pixbuf_new_from_file_at_size (iconname,
							   size, size, NULL);
		}
	    }
	    else
	    {
		pb = xfce_pixbuf_new_from_file_at_size (iconname, 
							size, size, NULL);
	    
		if (pb)
		    break;
	    }

	    g_free (iconname);
	}
    }

    if (pb)
    {
	if (fallback)
	    g_object_unref (fallback);

	return pb;
    }

    return fallback;
}

GdkPixbuf *
get_pixbuf_by_id (int id)
{
    GdkPixbuf *pb;
    
    if (id < UNKNOWN_ICON || id >= NUM_ICONS)
	id = UNKNOWN_ICON;

    pb = themed_pixbuf_from_name_list (xfce_icon_names [id], 
	    			icon_size[settings.size]);
    
    if (!pb && id != UNKNOWN_ICON)
	return get_pixbuf_by_id (UNKNOWN_ICON);

    return pb;
}

GdkPixbuf *
get_minibutton_pixbuf (int id)
{
    GdkPixbuf *pb;

    if (id < 0 || id >= MINIBUTTONS)
	return get_pixbuf_by_id (UNKNOWN_ICON);

    pb = themed_pixbuf_from_name_list (minibutton_names [id], 16);
    
    if (!pb)
	return get_pixbuf_by_id (UNKNOWN_ICON);

    return pb;
}

GdkPixbuf *
get_pixbuf_from_file (const char *path)
{
    GdkPixbuf *pb;

    if (!g_file_test (path, G_FILE_TEST_EXISTS))
	return get_pixbuf_by_id (UNKNOWN_ICON);

    pb = xfce_themed_icon_load (path, icon_size[settings.size]);

    if (!pb)
	return get_pixbuf_by_id (UNKNOWN_ICON);

    return pb;
}

GdkPixbuf *
get_panel_pixbuf (void)
{
    return gdk_pixbuf_new_from_inline (-1, panel_icon_data, FALSE, NULL);
}

GdkPixbuf *
get_scaled_pixbuf (GdkPixbuf * pb, int size)
{
    int w, h, neww, newh;
    GdkPixbuf *newpb;

    if (!pb || !GDK_IS_PIXBUF (pb))
    {
	GdkPixbuf *tmp = get_pixbuf_by_id (UNKNOWN_ICON);

	newpb = get_scaled_pixbuf (tmp, size);
	g_object_unref (tmp);

	return newpb;
    }

    w = gdk_pixbuf_get_width (pb);
    h = gdk_pixbuf_get_height (pb);

    if (size > w && size > h)
    {
	newpb = pb;
	g_object_ref (newpb);
	return newpb;
    }
    else if (h > w)
    {
	newh = size;
	neww = ((double) w * (double) size / (double) h);
    }
    else
    {
	neww = size;
	newh = ((double) h * (double) size / (double) w);
    }

    return gdk_pixbuf_scale_simple (pb, neww, newh, GDK_INTERP_BILINEAR);
}

GdkPixbuf *
get_themed_pixbuf (const char *name)
{
    GdkPixbuf *pb = NULL;
    gchar *p, *rootname = g_strdup(name);

    if ((p=g_strrstr(rootname, "."))) 
    {
	if (strlen(rootname) - (p-rootname) <= 5)
	    *p = 0;
    }

    pb = xfce_themed_icon_load (rootname, 48);
    g_free(rootname);

    if (!pb)
    {
	int size = settings.size;

	/* usually the pixbuf will be scaled afterwards, so we want to get a
	 * decent size, even for a 'missing' icon ;-) */
	settings.size = LARGE;
	pb = get_pixbuf_by_id (UNKNOWN_ICON);
	settings.size = size;
	
	g_warning ("Couldn't find icon: %s\n", name);
    }

    return pb;
}
