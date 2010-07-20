/*
 * Copyright (c) 2009 Nick Schermer <nick@xfce.org>
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

#ifndef __PAGER_H__
#define __PAGER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _PagerPluginClass PagerPluginClass;
typedef struct _PagerPlugin      PagerPlugin;

#define XFCE_TYPE_PAGER_PLUGIN            (pager_plugin_get_type ())
#define XFCE_PAGER_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_PAGER_PLUGIN, PagerPlugin))
#define XFCE_PAGER_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_PAGER_PLUGIN, PagerPluginClass))
#define XFCE_IS_PAGER_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_PAGER_PLUGIN))
#define XFCE_IS_PAGER_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_PAGER_PLUGIN))
#define XFCE_PAGER_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_PAGER_PLUGIN, PagerPluginClass))

GType pager_plugin_get_type      (void) G_GNUC_CONST;

void  pager_plugin_register_type (XfcePanelTypeModule *type_module);

G_END_DECLS

#endif /* !__PAGER_H__ */
