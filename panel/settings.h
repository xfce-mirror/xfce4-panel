/*  $Id$
 *  
 *  Copyright 2002-2004 Jasper Huijsmans (jasper@xfce.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef __XFCE_XMLCONFIG_H__
#define __XFCE_XMLCONFIG_H__

#include <gmodule.h>
#include <libxml/tree.h>
#include <panel/global.h>

void write_panel_config (void);

void get_global_prefs (void);

void get_panel_config (void);

G_MODULE_IMPORT
xmlDocPtr xmlconfig;

#define DATA(node) xmlNodeListGetString(xmlconfig, node->children, 1)

#endif /* __XFCE_XMLCONFIG_H__ */
