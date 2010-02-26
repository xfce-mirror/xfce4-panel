/* $Id$ */
/*
 * Copyright (c) 2002      Anders Carlsson <andersca@gnu.org>
 * Copyright (c) 2003-2004 Benedikt Meurer <benny@xfce.org>
 * Copyright (c) 2003-2004 Olivier Fourdan <fourdan@xfce.org>
 * Copyright (c) 2003-2006 Vincent Untz
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

#ifndef __XFCE_TRAY_MANAGER_H__
#define __XFCE_TRAY_MANAGER_H__

#define XFCE_TRAY_MANAGER_ENABLE_MESSAGES 0



typedef struct _XfceTrayManagerClass XfceTrayManagerClass;
typedef struct _XfceTrayManager      XfceTrayManager;
#if XFCE_TRAY_MANAGER_ENABLE_MESSAGES
typedef struct _XfceTrayMessage      XfceTrayMessage;
#endif

enum
{
    XFCE_TRAY_MANAGER_ERROR_SELECTION_FAILED
};



#define XFCE_TYPE_TRAY_MANAGER            (xfce_tray_manager_get_type ())
#define XFCE_TRAY_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_TRAY_MANAGER, XfceTrayManager))
#define XFCE_TRAY_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_TRAY_MANAGER, XfceTrayManagerClass))
#define XFCE_IS_TRAY_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_TRAY_MANAGER))
#define XFCE_IS_TRAY_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_TRAY_MANAGER))
#define XFCE_TRAY_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_TRAY_MANAGER, XfceTrayManagerClass))
#define XFCE_TRAY_MANAGER_ERROR           (xfce_tray_manager_error_quark())



GType                xfce_tray_manager_get_type               (void) G_GNUC_CONST;

GQuark               xfce_tray_manager_error_quark            (void);

XfceTrayManager     *xfce_tray_manager_new                    (void) G_GNUC_MALLOC;

gboolean             xfce_tray_manager_check_running          (GdkScreen        *screen);

gboolean             xfce_tray_manager_register               (XfceTrayManager  *manager,
                                                               GdkScreen        *screen,
                                                               GError          **error);

void                 xfce_tray_manager_unregister             (XfceTrayManager  *manager);

GtkOrientation       xfce_tray_manager_get_orientation        (XfceTrayManager  *manager);

void                 xfce_tray_manager_set_orientation        (XfceTrayManager  *manager,
                                                               GtkOrientation    orientation);

gchar               *xfce_tray_manager_get_application_name   (GtkWidget        *socket) G_GNUC_MALLOC;


#endif /* !__XFCE_TRAY_MANAGER_H__ */
