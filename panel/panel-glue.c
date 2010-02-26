/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <exo/exo.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-private.h>
#include <panel/panel-glue.h>
#include <panel/panel-item-dialog.h>
#include <panel/panel-preferences-dialog.h>
#include <panel/panel-dialogs.h>
#include <panel/panel-dbus-service.h>
#include <panel/panel-plugin-external.h>
#include <panel/panel-window.h>




XfceScreenPosition
panel_glue_get_screen_position (PanelWindow *window)
{
  gboolean            horizontal;
  PanelWindowSnapEdge snap_edge;

  panel_return_val_if_fail (PANEL_IS_WINDOW (window), XFCE_SCREEN_POSITION_NONE);

  g_object_get (G_OBJECT (window), "horizontal", &horizontal,
                "snap-edge", &snap_edge, NULL);

  /* return the screen position */
  switch (snap_edge)
    {
      case PANEL_SNAP_EGDE_NONE:
        return horizontal ? XFCE_SCREEN_POSITION_FLOATING_H :
          XFCE_SCREEN_POSITION_FLOATING_V;

      case PANEL_SNAP_EGDE_NW:
        return horizontal ? XFCE_SCREEN_POSITION_NW_H :
          XFCE_SCREEN_POSITION_NW_V;

      case PANEL_SNAP_EGDE_NE:
        return horizontal ? XFCE_SCREEN_POSITION_NE_H :
          XFCE_SCREEN_POSITION_NE_V;

      case PANEL_SNAP_EGDE_SW:
        return horizontal ? XFCE_SCREEN_POSITION_SW_H :
          XFCE_SCREEN_POSITION_SW_V;

      case PANEL_SNAP_EGDE_SE:
        return horizontal ? XFCE_SCREEN_POSITION_SE_H :
          XFCE_SCREEN_POSITION_SE_V;

      case PANEL_SNAP_EGDE_W:
      case PANEL_SNAP_EGDE_WC:
        return horizontal ? XFCE_SCREEN_POSITION_FLOATING_H :
          XFCE_SCREEN_POSITION_W;

      case PANEL_SNAP_EGDE_E:
      case PANEL_SNAP_EGDE_EC:
        return horizontal ? XFCE_SCREEN_POSITION_FLOATING_H :
          XFCE_SCREEN_POSITION_E;

      case PANEL_SNAP_EGDE_S:
      case PANEL_SNAP_EGDE_SC:
        return horizontal ? XFCE_SCREEN_POSITION_S :
          XFCE_SCREEN_POSITION_FLOATING_V;

      case PANEL_SNAP_EGDE_N:
      case PANEL_SNAP_EGDE_NC:
        return horizontal ? XFCE_SCREEN_POSITION_N :
          XFCE_SCREEN_POSITION_FLOATING_V;
    }

  return XFCE_SCREEN_POSITION_NONE;
}


/* TODO move to window */
void
panel_glue_set_provider_info (XfcePanelPluginProvider *provider,
                              PanelWindow             *window)
{
  guint    size;
  gboolean horizontal;
  guint    background_alpha;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));
  panel_return_if_fail (PANEL_IS_WINDOW (window));

  /* read settings from the window */
  g_object_get (G_OBJECT (window), "size", &size,
                "horizontal", &horizontal, "background-alpha",
                &background_alpha, NULL);

  /* set the background alpha if the plugin is external */
  if (PANEL_IS_PLUGIN_EXTERNAL (provider))
    panel_plugin_external_set_background_alpha (PANEL_PLUGIN_EXTERNAL (provider), background_alpha);

  /* send plugin information */
  xfce_panel_plugin_provider_set_screen_position (provider, panel_glue_get_screen_position (window));
  xfce_panel_plugin_provider_set_size (provider, size);
  xfce_panel_plugin_provider_set_orientation (provider, horizontal ? GTK_ORIENTATION_HORIZONTAL
                                              : GTK_ORIENTATION_VERTICAL);
}
