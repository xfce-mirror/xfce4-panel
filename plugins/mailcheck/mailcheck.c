/*  $Id$
 *
 *  Copyright (C) 2002-2004 Jasper Huijsmans (jasper@xfce.org)
 *
 *  - 2004 Add Maildir support Julien NOEL (dev@no-l.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include <stdio.h>

#include <gtk/gtk.h>

#include <netdb.h>
#include <sys/socket.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/xfce_iconbutton.h>

#include <panel/xfce.h>
#include <panel/settings.h>
#include <panel/plugins.h>
#include <panel/item_dialog.h>

#include <dirent.h>

#define MAILCHECK_ROOT "Mailcheck"

#define BORDER 6

static GtkTooltips *tooltips = NULL;

typedef struct
{
    char *mbox;
    char *command;
    char *newmail_command;
    gboolean term;
    gboolean use_sn;
    int interval;

    gboolean pop3;
    char pop3_username[256];
    char pop3_password[256];
    char pop3_hostname[256];

    int timeout_id;
    int status;

    GdkPixbuf *nomail_pb;
    GdkPixbuf *newmail_pb;
    GdkPixbuf *oldmail_pb;

    GtkWidget *button;
}
t_mailcheck;

/*  Icons
 *  -----
*/
enum
{
    NO_MAIL,
    NEW_MAIL,
    OLD_MAIL
};

static char *mailcheck_icon_names[] = {
    "xfce-mail",
    "mail",
    "gnome-mailcheck",
    NULL
};

static GdkPixbuf *
get_mail_pixbuf (void)
{
    GdkPixbuf *pb;

    pb = themed_pixbuf_from_name_list (mailcheck_icon_names,
				       icon_size[settings.size]);

    if (G_UNLIKELY (!pb))
    {
	g_critical ("Mail icon could not be found");
	pb = get_pixbuf_by_id (UNKNOWN_ICON);
    }

    return pb;
}

static void
_dimm_icon (GdkPixbuf * pixbuf)
{
    int x, y, pixel_stride, row_stride;
    guchar *row, *pixels;
    int w, h;

    if (!pixbuf)
        return;
        
    if (!gdk_pixbuf_get_has_alpha (pixbuf))
    {
        gdk_pixbuf_saturate_and_pixelate (pixbuf, pixbuf, 0, TRUE);
        return;
    }

    w = gdk_pixbuf_get_width (pixbuf);
    h = gdk_pixbuf_get_height (pixbuf);

    pixel_stride = 4;

    row = gdk_pixbuf_get_pixels (pixbuf);
    row_stride = gdk_pixbuf_get_rowstride (pixbuf);

    for (y = 0; y < h; y++)
    {
        pixels = row;

        for (x = 0; x < w; x++)
        {
            pixels[3] /= 2;

            pixels += pixel_stride;
        }

        row += row_stride;
    }

    gdk_pixbuf_saturate_and_pixelate (pixbuf, pixbuf, 0, TRUE);
}

static void
reset_mailcheck_icons (t_mailcheck * mc)
{
    if (mc->newmail_pb)
	g_object_unref (mc->newmail_pb);

    if (mc->nomail_pb)
	g_object_unref (mc->nomail_pb);

    if (mc->oldmail_pb)
	g_object_unref (mc->oldmail_pb);

    mc->newmail_pb = get_mail_pixbuf ();

    g_return_if_fail (mc->newmail_pb != NULL);

    mc->nomail_pb = gdk_pixbuf_copy (mc->newmail_pb);
    _dimm_icon (mc->nomail_pb);

    mc->oldmail_pb = mc->nomail_pb;
    g_object_ref (mc->oldmail_pb);
}

/*  Configuration
 *  -------------
*/
static void
mailcheck_set_tip (t_mailcheck * mc)
{
    char *tip;

    if (!tooltips)
	tooltips = gtk_tooltips_new ();

    if (mc->command && strlen (mc->command))
    {
	tip = g_strdup (mc->command);

	tip[0] = g_ascii_toupper (tip[0]);

	gtk_tooltips_set_tip (tooltips, mc->button, tip, NULL);

	g_free (tip);
    }
}

static gboolean
set_mail_icon (t_mailcheck * mc)
{
    if (mc->status == NO_MAIL)
    {
	xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (mc->button),
				    mc->nomail_pb);
    }
    else if (mc->status == OLD_MAIL)
    {
	xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (mc->button),
				    mc->oldmail_pb);
    }
    else
    {
	xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (mc->button),
				    mc->newmail_pb);
    }

    return FALSE;
}

/* POP3 routines */

static int
pop3_read_response (int fd, char *buff, int size)
{
    int bytes_read = 0;
    int state = 0;

    while (bytes_read < size && state != 2)
    {
	bytes_read += read (fd, &buff[bytes_read], sizeof (char));

	switch (state)
	{
	    case 0:
		if (buff[bytes_read - 1] == '\r')
		    state = 1;
		break;
	    case 1:
		if (buff[bytes_read - 1] == '\n')
		    state = 2;
		else
		    state = 0;
		break;
	}
    }

    if (state == 2)
	buff[bytes_read - 2 * sizeof (char)] = '\0';

    if (strncmp (buff, "+OK", 3 * sizeof (char)) == 0)
	return 1;
    else
	return 0;
}

static int
pop3_send_command (int fd, char *buff)
{
    return write (fd, buff, strlen (buff));
}

static int
pop3_check_mail (char *username, char *password, char *hostname)
{
    struct sockaddr_in sa;
    struct hostent *hp;
    int msg_count;
    char buff[1024];
    char command[256];
    int port = 110;
    int sd;
    int i;

    /* Lookup hostname */
    hp = gethostbyname (hostname);
    if (hp == NULL)
	return NO_MAIL;

    sa.sin_port = htons (port);
    sa.sin_family = hp->h_addrtype;

    /* Create the socket */
    sd = socket (AF_INET, SOCK_STREAM, 0);
    if (sd == -1)
	return NO_MAIL;

    /* Try each address until we get one that works */
    for (i = 0;; i++)
    {
	if (hp->h_addr_list[i] == NULL)
	{
	    close (sd);
	    return NO_MAIL;
	}

	memcpy ((char *) &sa.sin_addr, hp->h_addr_list[i], hp->h_length);

	if (connect (sd, (struct sockaddr *) &sa, sizeof (sa)) == 0)
	    break;
    }

    if (!pop3_read_response (sd, buff, 1024))
    {
	close (sd);
	return NO_MAIL;
    }

    g_snprintf (command, 256, "USER %s\r\n", username);
    pop3_send_command (sd, command);
    if (!pop3_read_response (sd, buff, 1024))
    {
	pop3_send_command (sd, "QUIT\r\n");
	close (sd);
	return NO_MAIL;
    }

    g_snprintf (command, 256, "PASS %s\r\n", password);
    pop3_send_command (sd, command);
    if (!pop3_read_response (sd, buff, 1024))
    {
	pop3_send_command (sd, "QUIT\r\n");
	close (sd);
	return NO_MAIL;
    }

    pop3_send_command (sd, "STAT\r\n");
    if (!pop3_read_response (sd, buff, 1024))
    {
	pop3_send_command (sd, "QUIT\r\n");
	close (sd);
	return NO_MAIL;
    }

    sscanf (buff, "+OK %d", &msg_count);

    pop3_send_command (sd, "QUIT\r\n");

    close (sd);

    if (msg_count > 0)
	return NEW_MAIL;
    else
	return NO_MAIL;
}

static gboolean
exec_newmail_cmd (const char *cmd)
{
    exec_cmd (cmd, FALSE, FALSE);

    return FALSE;
}

static gboolean
check_mail (t_mailcheck * mailcheck)
{
    int mail = NO_MAIL;
    struct stat s;

    DBG ("Checking mail ... ");

    if (mailcheck->pop3 == TRUE)
    {
	mail = pop3_check_mail (mailcheck->pop3_username,
				mailcheck->pop3_password,
				mailcheck->pop3_hostname);
    }
    else
    {
	if (stat (mailcheck->mbox, &s) < 0)
	{
	    mail = NO_MAIL;
	}
	else if (S_ISREG (s.st_mode))
	{
	    DBG ("mbox format");

	    if (!s.st_size)
		mail = NO_MAIL;
	    else if (s.st_mtime <= s.st_atime)
		mail = OLD_MAIL;
	    else
		mail = NEW_MAIL;

	}
	else if (S_ISDIR (s.st_mode))
	{
	    DIR *dr;
	    struct dirent *de;
	    char *c_list[] = { "tmp", "cur", "new" };
	    char *c_tmp;
	    int i_tmp;

	    /* mailbox is a maildir */
	    DBG ("maildir format");

	    mail = NO_MAIL;

	    /* Verify the Maildir integrity */
	    for (i_tmp = 0; i_tmp < 3; i_tmp++)
	    {
		c_tmp =
		    g_build_filename (mailcheck->mbox, c_list[i_tmp], NULL);

		if (stat (c_tmp, &s) >= 0 && S_ISDIR (s.st_mode))
		{
		    dr = opendir (c_tmp);

		    if (dr != NULL)
		    {
			while ((de = readdir (dr)))
			{
			    if (strlen (de->d_name) >= 1 &&
				de->d_name[0] != '.')
			    {
				if (i_tmp < 2)
				{
				    mail = OLD_MAIL;
				    break;
				}
				else
				{
				    mail = NEW_MAIL;
				    break;
				}
			    }
			}

			closedir (dr);
		    }
		}
	    }
	}
    }

    DBG ("Done");

    if (mail != mailcheck->status)
    {
	if (mail == NEW_MAIL && mailcheck->status != NEW_MAIL &&
	    mailcheck->newmail_command &&
	    strlen (mailcheck->newmail_command) != 0)
	{
	    g_idle_add ((GSourceFunc) exec_newmail_cmd,
			mailcheck->newmail_command);
	}

	mailcheck->status = mail;
	g_idle_add ((GSourceFunc) set_mail_icon, mailcheck);
    }

    DBG ("Done\n");

    /* keep the g_timeout running */
    return TRUE;
}


static void
run_mailcheck (t_mailcheck * mc)
{
    if (mc->timeout_id > 0)
	return;

    if (mc->interval > 0)
    {
	mc->timeout_id = g_timeout_add (mc->interval * 1000,
					(GSourceFunc) check_mail, mc);
    }
}

/* set mailbox type */
static void
set_mbox_type(t_mailcheck *mc)
{
    if (strncmp (mc->mbox, "pop3://", 7 * sizeof (char)) == 0)
    {
        mc->pop3 = TRUE;
        sscanf (mc->mbox, "pop3://%[^:]:%[^@]@%s",
		mc->pop3_username,
	    	mc->pop3_password, mc->pop3_hostname);
    }
} 

static void
mailcheck_read_config (Control * control, xmlNodePtr node)
{
    xmlChar *value;
    int n;

    t_mailcheck *mc = (t_mailcheck *) control->data;

    if (!node || !node->children)
	return;

    node = node->children;

    if (!xmlStrEqual (node->name, (const xmlChar *) MAILCHECK_ROOT))
	return;

    value = xmlGetProp (node, (const xmlChar *) "interval");

    if (value)
    {
	n = atoi (value);

	if (n > 0)
	    mc->interval = n;

	g_free (value);
    }

    for (node = node->children; node; node = node->next)
    {
	if (xmlStrEqual (node->name, (const xmlChar *) "Mbox"))
	{
	    value = DATA (node);
	    if (value)
	    {
		g_free (mc->mbox);
		mc->mbox = (char *) value;
		
		set_mbox_type(mc);
	    }
	}
	else if (xmlStrEqual
		 (node->name, (const xmlChar *) "newmail-command"))
	{
	    value = DATA (node);

	    if (value)
	    {
		g_free (mc->newmail_command);
		mc->newmail_command = (char *) value;
	    }
	}
	else if (xmlStrEqual (node->name, (const xmlChar *) "Command"))
	{
	    value = DATA (node);

	    if (value)
	    {
		g_free (mc->command);
		mc->command = (char *) value;
	    }

	    value = xmlGetProp (node, "term");

	    if (value)
	    {
		int n = atoi (value);

		if (n == 1)
		    mc->term = TRUE;
		else
		    mc->term = FALSE;

		g_free (value);
	    }

	    value = xmlGetProp (node, "sn");

	    if (value)
	    {
		int n = atoi (value);

		if (n == 1)
		    mc->use_sn = TRUE;
		else
		    mc->use_sn = FALSE;

		g_free (value);
	    }
	}
    }

    run_mailcheck (mc);

    mailcheck_set_tip (mc);
}

static void
mailcheck_write_config (Control * control, xmlNodePtr parent)
{
    xmlNodePtr root, node;
    char value[MAXSTRLEN + 1];

    t_mailcheck *mc = (t_mailcheck *) control->data;

    root = xmlNewTextChild (parent, NULL, MAILCHECK_ROOT, NULL);

    g_snprintf (value, 4, "%d", mc->interval);
    xmlSetProp (root, "interval", value);

    xmlNewTextChild (root, NULL, "Mbox", mc->mbox);

    node = xmlNewTextChild (root, NULL, "Command", mc->command);

    g_snprintf (value, 2, "%d", mc->term);
    xmlSetProp (node, "term", value);

    g_snprintf (value, 2, "%d", mc->use_sn);
    xmlSetProp (node, "sn", value);

    xmlNewTextChild (root, NULL, "newmail-command", mc->newmail_command);
}

static void
mailcheck_attach_callback (Control * control, const char *signal,
			   GCallback callback, gpointer data)
{
    t_mailcheck *mc = control->data;

    g_signal_connect (mc->button, signal, callback, data);
}

static void
run_mail_command (t_mailcheck * mc)
{
    exec_cmd (mc->command, mc->term, mc->use_sn);
}

static t_mailcheck *
mailcheck_new (void)
{
    t_mailcheck *mailcheck;
    const char *mail;

    mailcheck = g_new0 (t_mailcheck, 1);

    mailcheck->status = NO_MAIL;
    mailcheck->interval = 30;
    mailcheck->timeout_id = 0;

    reset_mailcheck_icons (mailcheck);

    mailcheck->newmail_command = g_strdup ("");

    mail = g_getenv ("MAIL");

    if (mail)
    {
	mailcheck->mbox = g_strdup (mail);
    }
    else
    {
	const char *logname = g_getenv ("LOGNAME");

	mailcheck->mbox = g_strconcat ("/var/spool/mail/", logname, NULL);
    }

    mailcheck->button =
	xfce_iconbutton_new_from_pixbuf (mailcheck->nomail_pb);
    gtk_widget_show (mailcheck->button);
    gtk_button_set_relief (GTK_BUTTON (mailcheck->button), GTK_RELIEF_NONE);

    g_signal_connect_swapped (mailcheck->button,
			      "clicked", G_CALLBACK (run_mail_command),
			      mailcheck);

    mailcheck_set_tip (mailcheck);

    return mailcheck;
}

static void
mailcheck_free (Control * control)
{
    t_mailcheck *mailcheck = (t_mailcheck *) control->data;

    if (mailcheck->timeout_id > 0)
	g_source_remove (mailcheck->timeout_id);

    g_free (mailcheck->mbox);
    g_free (mailcheck->command);
    g_free (mailcheck->newmail_command);

    g_object_unref (mailcheck->nomail_pb);
    g_object_unref (mailcheck->oldmail_pb);
    g_object_unref (mailcheck->newmail_pb);

    g_free (mailcheck);
}

static void
mailcheck_set_theme (Control * control, const char *theme)
{
    t_mailcheck *mailcheck = (t_mailcheck *) control->data;

    reset_mailcheck_icons (mailcheck);

    if (mailcheck->status == NO_MAIL)
    {
	xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (mailcheck->button),
				    mailcheck->nomail_pb);
    }
    else if (mailcheck->status == OLD_MAIL)
    {
	xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (mailcheck->button),
				    mailcheck->oldmail_pb);
    }
    else
    {
	xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (mailcheck->button),
				    mailcheck->newmail_pb);
    }
}

/*  Options dialog
 *  --------------
*/
typedef struct
{
    t_mailcheck *mc;

    /* control dialog */
    GtkWidget *dialog;

    /* options */
    GtkWidget *mbox_entry;
    GtkWidget *interval_spin;
    GtkWidget *newmail_cmd_entry;
    CommandOptions *cmd_opts;
}
MailDialog;

static void
free_maildialog (MailDialog * md)
{
    g_free (md);
}

/* update control */
static void
mailcheck_apply_options (MailDialog * md)
{
    const char *tmp;
    t_mailcheck *mc = md->mc;

    g_free (mc->command);

    command_options_get_command (md->cmd_opts, &(mc->command), &(mc->term),
				 &(mc->use_sn));

    tmp = gtk_entry_get_text (GTK_ENTRY (md->mbox_entry));

    if (tmp && *tmp)
    {
	g_free (mc->mbox);
	mc->mbox = g_strdup (tmp);
	set_mbox_type(mc);
    }

    tmp = gtk_entry_get_text (GTK_ENTRY (md->newmail_cmd_entry));

    if (tmp && *tmp)
    {
	g_free (mc->newmail_command);
	mc->newmail_command = g_strdup (tmp);
    }

    mc->interval =
	gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
					  (md->interval_spin));

    mailcheck_set_tip (mc);

    run_mailcheck (mc);
}

/* mbox */
static void
mbox_browse_cb (GtkWidget * b, MailDialog * md)
{
    const char *text;
    char *file;

    text = gtk_entry_get_text (GTK_ENTRY (md->mbox_entry));

    file = select_file_name (NULL, text, md->dialog);

    if (file)
    {
	gtk_entry_set_text (GTK_ENTRY (md->mbox_entry), file);
	g_free (file);
	mailcheck_apply_options (md);
    }
}

/* newmail command */
static void
newmail_cmd_brows_cb (GtkWidget * b, MailDialog * md)
{
    const char *text;
    char *file;

    text = gtk_entry_get_text (GTK_ENTRY (md->newmail_cmd_entry));

    file = select_file_name (NULL, text, md->dialog);

    if (file)
    {
	gtk_entry_set_text (GTK_ENTRY (md->newmail_cmd_entry), file);
	g_free (file);
	mailcheck_apply_options (md);
    }
}

static gboolean
entry_lost_focus (MailDialog * md)
{
    mailcheck_apply_options (md);

    /* needed to let entry handle focus-out as well */
    return FALSE;
}

static void
add_mbox_box (GtkWidget * vbox, GtkSizeGroup * sg, MailDialog * md)
{
    GtkWidget *hbox, *label, *button, *image;
    t_mailcheck *mc = md->mc;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Mail box:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    md->mbox_entry = gtk_entry_new ();
    if (mc->mbox)
	gtk_entry_set_text (GTK_ENTRY (md->mbox_entry), mc->mbox);
    gtk_widget_show (md->mbox_entry);
    gtk_box_pack_start (GTK_BOX (hbox), md->mbox_entry, TRUE, TRUE, 0);

    button = gtk_button_new ();
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

    image = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (image);
    gtk_container_add (GTK_CONTAINER (button), image);

    g_signal_connect (button, "clicked", G_CALLBACK (mbox_browse_cb), md);

    /* only set label on focus out */
    g_signal_connect_swapped (md->mbox_entry, "focus-out-event",
			      G_CALLBACK (entry_lost_focus), md);
}

static void
add_newmail_cmd_box (GtkWidget * vbox, GtkSizeGroup * sg, MailDialog * md)
{
    GtkWidget *hbox, *label, *button, *image;
    t_mailcheck *mc = md->mc;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("New mail command:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    md->newmail_cmd_entry = gtk_entry_new ();
    if (mc->newmail_command)
	gtk_entry_set_text (GTK_ENTRY (md->newmail_cmd_entry),
			    mc->newmail_command);
    gtk_widget_show (md->newmail_cmd_entry);
    gtk_box_pack_start (GTK_BOX (hbox), md->newmail_cmd_entry, TRUE, TRUE, 0);

    gtk_tooltips_set_tip (tooltips, md->newmail_cmd_entry,
			  _("Command to run when new mail arrives"), NULL);

    button = gtk_button_new ();
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

    image = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (image);
    gtk_container_add (GTK_CONTAINER (button), image);

    g_signal_connect (button, "clicked", G_CALLBACK (newmail_cmd_brows_cb),
		      md);

    /* only set label on focus out */
    g_signal_connect_swapped (md->mbox_entry, "focus-out-event",
			      G_CALLBACK (entry_lost_focus), md);
}

/* command */
static void
add_command_box (GtkWidget * vbox, GtkSizeGroup * sg, MailDialog * md)
{
    md->cmd_opts = create_command_options (sg);

    command_options_set_command (md->cmd_opts, md->mc->command,
				 md->mc->term, md->mc->use_sn);

    gtk_box_pack_start (GTK_BOX (vbox), md->cmd_opts->base, FALSE, TRUE, 0);

    gtk_tooltips_set_tip (tooltips, md->cmd_opts->command_entry,
			  _("Command to run when the button "
			    "on the panel is clicked"), NULL);
}

/* interval */
static void
interval_changed (GtkSpinButton * spin, MailDialog * md)
{
    md->mc->interval = gtk_spin_button_get_value_as_int (spin);
}

static void
add_interval_box (GtkWidget * vbox, GtkSizeGroup * sg, MailDialog * md)
{
    GtkWidget *hbox, *label;
    t_mailcheck *mc = md->mc;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Interval (sec):"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    md->interval_spin = gtk_spin_button_new_with_range (1, 600, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (md->interval_spin),
			       mc->interval);
    gtk_widget_show (md->interval_spin);
    gtk_box_pack_start (GTK_BOX (hbox), md->interval_spin, FALSE, FALSE, 0);

    g_signal_connect (md->interval_spin, "value-changed",
		      G_CALLBACK (interval_changed), md);
}

/* the dialog */
static void
mailcheck_create_options (Control * control, GtkContainer * container,
			  GtkWidget * done)
{
    GtkWidget *vbox;
    GtkSizeGroup *sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    MailDialog *md;

    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    md = g_new0 (MailDialog, 1);

    md->mc = (t_mailcheck *) control->data;

    md->dialog = gtk_widget_get_toplevel (done);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);

    add_mbox_box (vbox, sg, md);

    add_interval_box (vbox, sg, md);

    add_newmail_cmd_box (vbox, sg, md);

    add_command_box (vbox, sg, md);

    /* signals */
    g_signal_connect_swapped (done, "clicked",
			      G_CALLBACK (mailcheck_apply_options), md);

    g_signal_connect_swapped (md->dialog, "destroy-event",
			      G_CALLBACK (free_maildialog), md);

    gtk_container_add (container, vbox);
}

/* create panel control */
static gboolean
create_mailcheck_control (Control * control)
{
    t_mailcheck *mailcheck = mailcheck_new ();
    GtkWidget *b = mailcheck->button;

    gtk_container_add (GTK_CONTAINER (control->base), b);

    control->data = (gpointer) mailcheck;
    control->with_popup = FALSE;


    return TRUE;
}

G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    cc->name = "mailcheck";
    cc->caption = _("Mail checker");

    cc->create_control = (CreateControlFunc) create_mailcheck_control;

    cc->free = mailcheck_free;
    cc->read_config = mailcheck_read_config;
    cc->write_config = mailcheck_write_config;
    cc->attach_callback = mailcheck_attach_callback;

    cc->create_options = (gpointer) mailcheck_create_options;

    cc->set_theme = mailcheck_set_theme;
}


XFCE_PLUGIN_CHECK_INIT
