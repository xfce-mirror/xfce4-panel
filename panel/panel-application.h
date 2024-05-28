/*
 * Copyright (C) 2008-2010 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __PANEL_APPLICATION_H__
#define __PANEL_APPLICATION_H__

#include "panel-window.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PANEL_TYPE_APPLICATION (panel_application_get_type ())
G_DECLARE_FINAL_TYPE (PanelApplication, panel_application, PANEL, APPLICATION, GObject)

typedef enum
{
  SAVE_PLUGIN_PROVIDERS = 1 << 1,
  SAVE_PLUGIN_IDS = 1 << 2,
  SAVE_PANEL_IDS = 1 << 3,
} PanelSaveTypes;
#define SAVE_EVERYTHING (SAVE_PLUGIN_PROVIDERS | SAVE_PLUGIN_IDS | SAVE_PANEL_IDS)


PanelApplication *
panel_application_get (void);

gboolean
panel_application_load (PanelApplication *application,
                        gboolean disable_wm_check);

void
panel_application_save (PanelApplication *application,
                        PanelSaveTypes save_types);

void
panel_application_save_window (PanelApplication *application,
                               PanelWindow *window,
                               PanelSaveTypes save_types);

void
panel_application_take_dialog (PanelApplication *application,
                               GtkWindow *dialog);

void
panel_application_destroy_dialogs (PanelApplication *application);

void
panel_application_add_new_item (PanelApplication *application,
                                PanelWindow *window,
                                const gchar *plugin_name,
                                gchar **arguments);

PanelWindow *
panel_application_new_window (PanelApplication *application,
                              GdkScreen *screen,
                              gint id,
                              gboolean new_window);

void
panel_application_remove_window (PanelApplication *application,
                                 PanelWindow *window);

GSList *
panel_application_get_windows (PanelApplication *application);

PanelWindow *
panel_application_get_window (PanelApplication *application,
                              gint panel_id);

void
panel_application_window_select (PanelApplication *application,
                                 PanelWindow *window);

void
panel_application_windows_blocked (PanelApplication *application,
                                   gboolean blocked);

gboolean
panel_application_get_locked (PanelApplication *application);

void
panel_application_logout (void);

G_END_DECLS

#endif /* !__PANEL_APPLICATION_H__ */
