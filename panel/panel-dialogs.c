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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <exo/exo.h>
#include <libxfce4util/libxfce4util.h>

#include <panel/panel-private.h>
#include <panel/panel-dialogs.h>

void
panel_dialogs_show_about (void)
{
  /* our names */
  static const gchar *authors[] =
  {
    "Jasper Huijsmans <jasper@xfce.org>",
    "Nick Schermer <nick@xfce.org>",
    NULL
  };

  /* set exo hooks for urls and email */
  gtk_about_dialog_set_email_hook (exo_url_about_dialog_hook, NULL, NULL);
  gtk_about_dialog_set_url_hook (exo_url_about_dialog_hook, NULL, NULL);

  /* show the dialog */
  gtk_show_about_dialog (NULL,
                         "authors", authors,
                         "comments", _("The panel of the Xfce Desktop Environment"),
                         "copyright", "Copyright \302\251 2004-2008 Jasper Huijsmans",
                         "destroy-with-parent", TRUE,
                         "license", XFCE_LICENSE_GPL,
#if GTK_CHECK_VERSION(2,12,0)
                         "program-name", PACKAGE_NAME,
#else
                         "name", PACKAGE_NAME,
#endif
                         "translator-credits", _("translator-credits"),
                         "version", PACKAGE_VERSION,
                         "website", "http://www.xfce.org/",
                         "logo-icon-name", PACKAGE_NAME,
                         NULL);
}
