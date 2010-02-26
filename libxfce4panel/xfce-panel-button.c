/* $Id$ */
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <common/panel-private.h>
#include <libxfce4panel/xfce-panel-button.h>

#define XFCE_PANEL_BUTTON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                            XFCE_TYPE_PANEL_BUTTON, \
                                            XfcePanelButtonPrivate))

/* blink count, number should be even */
#define MAX_BLINKING_COUNT 16



static void xfce_panel_button_finalize (GObject *object);



struct _XfcePanelButtonPrivate
{
  /* blinking timeout id */
  guint          blinking_timeout_id;

  /* counter to make the blinking stop when
   * MAX_BLINKING_COUNT is reached */
  guint          blinking_counter;

  /* button relief when the blinking starts */
  GtkReliefStyle relief;
};



G_DEFINE_TYPE (XfcePanelButton, xfce_panel_button, GTK_TYPE_TOGGLE_BUTTON)



static void
xfce_panel_button_class_init (XfcePanelButtonClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (XfcePanelButtonPrivate));

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_panel_button_finalize;
}



static void
xfce_panel_button_init (XfcePanelButton *button)
{
  /* set private pointer */
  button->priv = XFCE_PANEL_BUTTON_GET_PRIVATE (button);

  /* initialize button values */
  button->priv->blinking_timeout_id = 0;
  button->priv->blinking_counter = 0;
  button->priv->relief = GTK_RELIEF_NORMAL;

  /* set some widget properties */
  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_DEFAULT | GTK_CAN_FOCUS);
  gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
}



static void
xfce_panel_button_finalize (GObject *object)
{
  XfcePanelButton *button = XFCE_PANEL_BUTTON (object);

  if (button->priv->blinking_timeout_id != 0)
    g_source_remove (button->priv->blinking_timeout_id);

  (*G_OBJECT_CLASS (xfce_panel_button_parent_class)->finalize) (object);
}



static gboolean
xfce_panel_button_blinking_timeout (gpointer user_data)
{
  XfcePanelButton *button = XFCE_PANEL_BUTTON (user_data);
  GtkStyle        *style;
  GtkRcStyle      *rc;

  rc = gtk_widget_get_modifier_style (GTK_WIDGET (button));
  if(PANEL_HAS_FLAG (rc->color_flags[GTK_STATE_NORMAL], GTK_RC_BG)
     || button->priv->blinking_timeout_id == 0)
    {
      gtk_button_set_relief (GTK_BUTTON (button), button->priv->relief);
      PANEL_UNSET_FLAG (rc->color_flags[GTK_STATE_NORMAL], GTK_RC_BG);
      gtk_widget_modify_style (GTK_WIDGET (button), rc);
    }
  else
    {
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NORMAL);
      PANEL_SET_FLAG (rc->color_flags[GTK_STATE_NORMAL], GTK_RC_BG);
      style = gtk_widget_get_style (GTK_WIDGET (button));
      rc->bg[GTK_STATE_NORMAL] = style->bg[GTK_STATE_SELECTED];
      gtk_widget_modify_style(GTK_WIDGET (button), rc);
    }

  return (button->priv->blinking_counter++ < MAX_BLINKING_COUNT);
}



static void
xfce_panel_button_blinking_timeout_destroyed (gpointer user_data)
{
  XfcePanelButton *button = XFCE_PANEL_BUTTON (user_data);

  button->priv->blinking_timeout_id = 0;
  button->priv->blinking_counter = 0;
}



PANEL_SYMBOL_EXPORT GtkWidget *
xfce_panel_button_new (void)
{
  return g_object_new (XFCE_TYPE_PANEL_BUTTON, NULL);
}



PANEL_SYMBOL_EXPORT gboolean
xfce_panel_button_get_blinking (XfcePanelButton *button)
{
  g_return_val_if_fail (XFCE_IS_PANEL_BUTTON (button), FALSE);
  return !!(button->priv->blinking_timeout_id != 0);
}



PANEL_SYMBOL_EXPORT void
xfce_panel_button_set_blinking (XfcePanelButton *button,
                                gboolean         blinking)
{
  g_return_if_fail (XFCE_IS_PANEL_BUTTON (button));

  if (blinking)
    {
      /* store the relief of the button */
      button->priv->relief = gtk_button_get_relief (GTK_BUTTON (button));

      if (button->priv->blinking_timeout_id == 0)
        {
          /* start blinking timeout */
          button->priv->blinking_timeout_id =
              g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, 500,
                                  xfce_panel_button_blinking_timeout, button,
                                  xfce_panel_button_blinking_timeout_destroyed);
        }
    }
  else if (button->priv->blinking_timeout_id != 0)
    {
      /* stop the blinking timeout */
      g_source_remove (button->priv->blinking_timeout_id);
    }

  /* start with a blinking or make sure the button is normal */
  xfce_panel_button_blinking_timeout (button);
}
