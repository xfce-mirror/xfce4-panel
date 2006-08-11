/* $Id: frap-icon-entry.h 22609 2006-08-01 13:27:16Z benny $ */
/*-
 * Copyright (c) 2005-2006 Benedikt Meurer <benny@xfce.org>
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

#ifndef __FRAP_ICON_ENTRY_H__
#define __FRAP_ICON_ENTRY_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS;

typedef struct _FrapIconEntryPrivate FrapIconEntryPrivate;
typedef struct _FrapIconEntryClass   FrapIconEntryClass;
typedef struct _FrapIconEntry        FrapIconEntry;

#define FRAP_TYPE_ICON_ENTRY            (frap_icon_entry_get_type ())
#define FRAP_ICON_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FRAP_TYPE_ICON_ENTRY, FrapIconEntry))
#define FRAP_ICON_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FRAP_TYPE_ICON_ENTRY, FrapIconEntryClass))
#define FRAP_IS_ICON_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FRAP_TYPE_ICON_ENTRY))
#define FRAP_IS_ICON_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FRAP_TYPE_ICON_ENTRY))
#define FRAP_ICON_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FRAP_TYPE_ICON_ENTRY, FrapIconEntryClass))

struct _FrapIconEntryClass
{
  GtkEntryClass __parent__;
};

struct _FrapIconEntry
{
  GtkEntry              __parent__;
  GdkWindow            *icon_area;
  FrapIconEntryPrivate *priv;
};
  
GType        frap_icon_entry_get_type     (void) G_GNUC_CONST;

GtkWidget   *frap_icon_entry_new          (void) G_GNUC_MALLOC;

GtkIconSize  frap_icon_entry_get_size     (FrapIconEntry *icon_entry);
void         frap_icon_entry_set_size     (FrapIconEntry *icon_entry,
                                           GtkIconSize    size);

const gchar *frap_icon_entry_get_stock_id (FrapIconEntry *icon_entry);
void         frap_icon_entry_set_stock_id (FrapIconEntry *icon_entry,
                                           const gchar   *stock_id);

G_END_DECLS;

#endif /* !__FRAP_ICON_ENTRY_H__ */
