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

#ifndef __PANEL_DBUS_H__
#define __PANEL_DBUS_H__

#include <gtk/gtk.h>

typedef struct _PanelDBusServiceClass PanelDBusServiceClass;
typedef struct _PanelDBusService      PanelDBusService;

enum
{
  SIGNAL_CUSTOMIZE,
  SIGNAL_ADD,
  SIGNAL_SAVE,
  SIGNAL_RESTART,
  SIGNAL_QUIT
};

#define PANEL_TYPE_DBUS_SERVICE            (panel_dbus_service_get_type ())
#define PANEL_DBUS_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PANEL_TYPE_DBUS_SERVICE, PanelDBusService))
#define PANEL_DBUS_CLASS_SERVICE(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PANEL_TYPE_DBUS_SERVICE, PanelDBusServiceClass))
#define PANEL_IS_DBUS_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PANEL_TYPE_DBUS_SERVICE))
#define PANEL_IS_DBUS_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANEL_TYPE_DBUS_SERVICE))
#define PANEL_DBUS_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PANEL_TYPE_DBUS_SERVICE, PanelDBusServiceClass))

GType     panel_dbus_service_get_type     (void) G_GNUC_CONST;

GObject  *panel_dbus_service_new          (void);

gboolean  panel_dbus_client_send_signal   (guint32   signal_id,
                                           GError  **error);

G_END_DECLS

#endif /* !__PANEL_DBUS_H__ */

