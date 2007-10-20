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


#ifndef __XFCE_PAGER_H__
#define __XFCE_PAGER_H__ 

#define XFCE_TYPE_PAGER             xfce_pager_get_type()
#define XFCE_PAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_PAGER, XfcePager))
#define XFCE_IS_PAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_PAGER))
#define XFCE_PAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_PAGER, XfcePagerClass))
#define XFCE_IS_PAGER_CLASS(klass) ( G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_PAGER))

typedef struct _XfcePagerPrivate XfcePagerPrivate;
typedef struct _XfcePager        XfcePager;
typedef struct _XfcePagerClass   XfcePagerClass;

struct _XfcePager
{
    WnckPager __parent__;
    XfcePagerPrivate *priv;
};

struct _XfcePagerClass
{
    WnckPagerClass __parent__;
};

GType      xfce_pager_get_type();
GtkWidget *xfce_pager_new                     (WnckScreen *screen);

void       xfce_pager_set_workspace_scrolling (XfcePager *pager, 
                                               gboolean   scrolling);

#endif /* __XFCE_PAGER_H__ */
