/* $Id$ */
/*
 * Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 * Copyright (c) 2007-2009 Nick Schermer <nick@xfce.org>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __SEPARATOR_H__
#define __SEPARATOR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _SeparatorPluginClass SeparatorPluginClass;
typedef struct _SeparatorPlugin      SeparatorPlugin;
typedef enum   _SeparatorPluginStyle  SeparatorPluginStyle;

#define XFCE_TYPE_SEPARATOR_PLUGIN            (separator_plugin_get_type ())
#define XFCE_SEPARATOR_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_SEPARATOR_PLUGIN, SeparatorPlugin))
#define XFCE_SEPARATOR_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_SEPARATOR_PLUGIN, SeparatorPluginClass))
#define XFCE_IS_SEPARATOR_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_SEPARATOR_PLUGIN))
#define XFCE_IS_SEPARATOR_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_SEPARATOR_PLUGIN))
#define XFCE_SEPARATOR_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_SEPARATOR_PLUGIN, SeparatorPluginClass))

GType separator_plugin_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* !__SEPARATOR_H__ */
