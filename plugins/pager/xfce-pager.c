/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libwnck/libwnck.h>
#include <libxfce4panel/xfce-panel-macros.h>

#include "xfce-pager.h"

#define XFCE_PAGER_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), XFCE_TYPE_PAGER, XfcePagerPrivate))



struct _XfcePagerPrivate
{
    WnckScreen *screen;
    guint       workspace_scrolling : 1;
};



static void        xfce_pager_init         (XfcePager      *pager);
static void        xfce_pager_class_init   (XfcePagerClass *pager);
static gboolean    xfce_pager_scroll_event (GtkWidget      *widget,
                                            GdkEventScroll *event);
static WnckScreen *xfce_pager_get_screen   (XfcePager      *pager);



GType
xfce_pager_get_type ()
{
    static GType xfce_pager_type = 0;

    if (!xfce_pager_type)
    {
        static const GTypeInfo xfce_pager_info = 
        {
            sizeof (XfcePagerClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) xfce_pager_class_init,
            (GClassFinalizeFunc) NULL,
            NULL,
            sizeof (XfcePager),
            0,
            (GInstanceInitFunc) xfce_pager_init,
        NULL
        };

        xfce_pager_type = g_type_register_static (WNCK_TYPE_PAGER, I_("XfcePager"), &xfce_pager_info, 0);
    }
    
    return xfce_pager_type;
}



static void
xfce_pager_class_init (XfcePagerClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    g_type_class_add_private (klass, sizeof (XfcePagerPrivate));

    widget_class->scroll_event = xfce_pager_scroll_event;
}



static void
xfce_pager_init(XfcePager *pager)
{
     pager->priv = XFCE_PAGER_GET_PRIVATE (pager);
}



GtkWidget *
xfce_pager_new (WnckScreen *screen)
{
    GtkWidget *pager = g_object_new (XFCE_TYPE_PAGER, NULL);

    XFCE_PAGER (pager)->priv->screen = screen;
    wnck_pager_set_screen (WNCK_PAGER (pager), screen);

    return pager;
}



void
xfce_pager_set_workspace_scrolling (XfcePager *pager, 
                                    gboolean   scrolling)
{
    pager->priv->workspace_scrolling = scrolling;
}



static gboolean
xfce_pager_scroll_event (GtkWidget      *widget, 
                         GdkEventScroll *event)
{
    XfcePager     *pager;
    gint           n, active;
    WnckWorkspace *ws = NULL;
    WnckScreen    *screen;

    g_return_val_if_fail (event != NULL, FALSE);
    g_return_val_if_fail (widget != NULL, FALSE);

    pager = XFCE_PAGER (widget);
    
    if (!pager->priv->workspace_scrolling)
	return FALSE;
    
    screen = xfce_pager_get_screen (pager);
    n = wnck_screen_get_workspace_count (screen);
    active = wnck_workspace_get_number (wnck_screen_get_active_workspace(screen));

    switch (event->direction)
    {
        case GDK_SCROLL_UP:
        case GDK_SCROLL_LEFT:
            if (active > 0)
            {
                ws = wnck_screen_get_workspace (screen, active - 1);
            }
            else
            {
                ws = wnck_screen_get_workspace (screen, n - 1);
            }
            wnck_workspace_activate (ws, 0);
            break;
            
        case GDK_SCROLL_DOWN:
        case GDK_SCROLL_RIGHT:
            if (active < n - 1)
            {
                ws = wnck_screen_get_workspace (screen, active + 1);
            }
            else
            {
                ws = wnck_screen_get_workspace (screen, 0);
            }
            wnck_workspace_activate (ws, 0);
            break;
            
        default:
            break;
    }

    return TRUE;
}



static WnckScreen *
xfce_pager_get_screen (XfcePager *pager)
{
    return pager->priv->screen;
}
