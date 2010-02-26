/* $Id$ */
/*
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

#ifndef __TASKLIST_PLUGIN_H__
#define __TASKLIST_PLUGIN_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _TasklistPluginClass TasklistPluginClass;
typedef struct _TasklistPlugin      TasklistPlugin;

#define TASKLIST_TYPE_PLUGIN            (tasklist_plugin_get_type ())
#define TASKLIST_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TASKLIST_TYPE_PLUGIN, TasklistPlugin))
#define TASKLIST_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TASKLIST_TYPE_PLUGIN, TasklistPluginClass))
#define TASKLIST_IS_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TASKLIST_TYPE_PLUGIN))
#define TASKLIST_IS_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TASKLIST_TYPE_PLUGIN))
#define TASKLIST_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TASKLIST_TYPE_PLUGIN, TasklistPluginClass))

GType tasklist_plugin_get_type (void) G_GNUC_CONST G_GNUC_INTERNAL;

G_END_DECLS

#endif /* !__TASKLIST_PLUGIN_H__ */
