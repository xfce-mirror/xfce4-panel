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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4util/i18n.h>
#include <libxfcegui4/xgtkicontheme.h>

#include "xfce.h"
#include "icons/xfce4-panel-icon.h"

#ifdef HAVE_GDK_PIXBUF_NEW_FROM_STREAM
#define gdk_pixbuf_new_from_inline gdk_pixbuf_new_from_stream
#endif

static GtkIconTheme *icon_theme = NULL;
const char *icon_names[NUM_ICONS];

/* icon themes */
static char *icon_suffix[] = {
    "svg",			/* librsvg provides a gdk_pixbuf loader for svg files */
    "svg.gz",			/* librsvg provides a gdk_pixbuf loader for svg files */
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

static void
set_icon_names ()
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

void
icons_init (void)
{
    set_icon_names ();

    icon_theme = gtk_icon_theme_get_default ();
    gtk_icon_theme_prepend_search_path (icon_theme, DATADIR "/themes");
}

GdkPixbuf *
get_pixbuf_by_id (int id)
{
    if (id < UNKNOWN_ICON || id >= NUM_ICONS)
	id = UNKNOWN_ICON;

    return gtk_icon_theme_load_icon (icon_theme, xfce_icon_names[id], 48, 0, NULL);
}

GdkPixbuf *
get_pixbuf_from_file (const char *path)
{
    GdkPixbuf *pb = NULL;

    if (!g_file_test (path, G_FILE_TEST_EXISTS))
	return get_pixbuf_by_id (UNKNOWN_ICON);

    pb = gdk_pixbuf_new_from_file (path, NULL);

    if (pb && GDK_IS_PIXBUF (pb))
	return pb;
    else
	return get_pixbuf_by_id (UNKNOWN_ICON);
}

GdkPixbuf *
get_panel_pixbuf (void)
{
    GdkPixbuf *pb = NULL;

    pb = gdk_pixbuf_new_from_inline (-1, panel_icon_data, FALSE, NULL);

    if (!pb)
	pb = get_pixbuf_by_id (UNKNOWN_ICON);

    return pb;
}

GdkPixbuf *
get_minibutton_pixbuf (int id)
{
    GdkPixbuf *pb;

    if (id < 0 || id >= MINIBUTTONS)
	return get_pixbuf_by_id (UNKNOWN_ICON);

    pb = gtk_icon_theme_load_icon (icon_theme, minibutton_names[id], 24, 0, NULL);

    if (!pb)
	pb = get_pixbuf_by_id (UNKNOWN_ICON);

    return pb;
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

static GdkPixbuf *
_get_themed_pixbuf (const char *name, const char *theme)
{
    GdkPixbuf *pb = NULL;
    gchar *p, *rootname = g_strdup(name);

	if ((p=g_strrstr(rootname, "."))) {
		if (strlen(rootname) - (p-rootname) <= 5)
			*p = 0;
	}

    pb = gtk_icon_theme_load_icon (icon_theme, rootname, 48, 0, NULL);
    g_free(rootname);

    /* prevent race condition when we can't find our fallback icon:
     * default theme, unknown icon */
    if (!pb && strequal (name, xfce_icon_names[UNKNOWN_ICON]))
    {
	g_printerr ("\n** ERROR **: xfce: unable to find any icons! "
		    "Please check your installation.\n\n");

	quit (TRUE);
    }

    return pb;
}

GdkPixbuf *
get_themed_pixbuf (const char *name)
{
    GdkPixbuf *pb = NULL;

    pb = _get_themed_pixbuf (name, settings.theme);

    if (!pb && settings.theme && !strequal (DEFAULT_THEME, settings.theme))
	pb = _get_themed_pixbuf (name, DEFAULT_THEME);

    if (!pb)
    {
	pb = get_pixbuf_by_id (UNKNOWN_ICON);
	g_printerr ("xfce: couldn't find icon: %s\n", name);
    }

    return pb;
}
