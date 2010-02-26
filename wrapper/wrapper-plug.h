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

#ifndef __WRAPPER_PLUG_H__
#define __WRAPPER_PLUG_H__

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

G_BEGIN_DECLS

typedef struct _WrapperPlugClass WrapperPlugClass;
typedef struct _WrapperPlug      WrapperPlug;

#define WRAPPER_TYPE_PLUG            (wrapper_plug_get_type ())
#define WRAPPER_PLUG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WRAPPER_TYPE_PLUG, WrapperPlug))
#define WRAPPER_PLUG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WRAPPER_TYPE_PLUG, WrapperPlugClass))
#define WRAPPER_IS_PLUG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WRAPPER_TYPE_PLUG))
#define WRAPPER_IS_PLUG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WRAPPER_TYPE_PLUG))
#define WRAPPER_PLUG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), WRAPPER_TYPE_PLUG, WrapperPlugClass))

GType      wrapper_plug_get_type (void) G_GNUC_CONST;

GtkWidget *wrapper_plug_new      (GdkNativeWindow          socket_id,
                                  XfcePanelPluginProvider *provider);

G_END_DECLS

#endif /* !__WRAPPER_PLUG_H__ */
