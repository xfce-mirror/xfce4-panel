/*  $Id$
 *  
 *  Copyright 2002-2004 Jasper Huijsmans (jasper@xfce.org)
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

#ifndef _XFCE_PLUGINS_H
#define _XFCE_PLUGINS_H

#include <glib.h>
#include <gmodule.h>

#include <panel/xfce.h>

#define XFCE_PLUGIN_API_VERSION 7

G_BEGIN_DECLS

/* prototype for plugin init function 
 * (must be implemented by plugin) */
G_MODULE_IMPORT void xfce_control_class_init (ControlClass * control);

/* plugin version check function (implemented by xfce4 in controls.c) */
G_MODULE_IMPORT gchar *xfce_plugin_check_version (gint version);

/* nifty idea, I think from dia:
 * every module just has to include this header and put 
 * XFCE_PLUGIN_CHECK_INIT
 * somewhere in the file (e.g. at the end :), and the API version will
 * be checked on opening the GModule.
*/
#define XFCE_PLUGIN_CHECK_INIT \
	G_MODULE_EXPORT const gchar *g_module_check_init(GModule *gmodule); \
	const gchar * \
	g_module_check_init(GModule *gmodule) \
	{ \
  	    return xfce_plugin_check_version(XFCE_PLUGIN_API_VERSION); \
	}

G_END_DECLS

#endif				/* _XFCE_PLUGIN_H */
