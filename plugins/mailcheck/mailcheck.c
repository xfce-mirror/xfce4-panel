/*  mailcheck.c
 *
 *  Copyright (C) 2002 Jasper Huijsmans (huysmans@users.sourceforge.net)
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

#include <sys/stat.h>
#include <gtk/gtk.h>

#include <xfce_iconbutton.h>

#include "global.h"
#include "xfce_support.h"
#include "icons.h"
#include "controls.h"
#include "settings.h"

#if 0
/* mailcheck icons */
#include "icons/mail.xpm"
#include "icons/nomail.xpm"
#include "icons/oldmail.xpm"
#endif

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Globals
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static GtkTooltips *tooltips = NULL;

/* this is checked when the control is loaded */
int is_xfce_panel_control = 1;

#define MAILCHECK_ROOT "Mailcheck"

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  General definitions
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
enum
{
    NO_MAIL,
    NEW_MAIL,
    OLD_MAIL
};

/* control data structure */
typedef struct
{
    int interval;
    int timeout_id;

    int status;
    char *mbox;

    GdkPixbuf *nomail_pb;
    GdkPixbuf *newmail_pb;
    GdkPixbuf *oldmail_pb;

    char *command;
    gboolean term;
    char *tooltip;

    GtkWidget *button;
}
t_mailcheck;

/*  Icons
 *  -----
*/
static char *mailcheck_icon_names[] = {
    "nomail",
    "newmail",
    "oldmail"
};

static GdkPixbuf *get_mailcheck_pixbuf(int id)
{
    GdkPixbuf *pb;
    char *name = mailcheck_icon_names[id];
    pb = get_themed_pixbuf(name);

    if(!pb || !GDK_IS_PIXBUF(pb))
        pb = get_pixbuf_by_id(UNKNOWN_ICON);

    return pb;
}

/*  Configuration
 *  -------------
*/
static void make_sensitive(GtkWidget *w)
{
    gtk_widget_set_sensitive(w, TRUE);
}

static void mailcheck_set_tip(t_mailcheck *mc)
{
    if(!tooltips)
        tooltips = gtk_tooltips_new();

    if (mc->tooltip)
    {
	gtk_tooltips_set_tip(tooltips, mc->button, mc->tooltip, NULL);
    }
    else
    {
	char *tip = g_strdup(mc->command);

	*tip = g_ascii_toupper(*tip);
	
	gtk_tooltips_set_tip(tooltips, mc->button, tip, NULL);

	g_free(tip);
    }
}

void mailcheck_read_config(PanelControl *pc, xmlNodePtr node)
{
    char *file;
    const char *mail, *logname;
    xmlChar *value;
    int n;

    t_mailcheck *mc = (t_mailcheck *)pc->data;
    
    if(!node || !node->children)
	return;

    node = node->children;

    if(!xmlStrEqual(node->name, (const xmlChar *)MAILCHECK_ROOT))
	return;

    value = xmlGetProp(node, (const xmlChar *)"interval");

    if (value)
    {
	n = atoi(value);

	if (n > 0)
	    mc->interval = n;

	g_free(value);
    }
    
    /* Now parse the xml tree */
    for(node = node->children; node; node = node->next)
    {
	if(xmlStrEqual(node->name, (const xmlChar *)"Mbox"))
	{
	    value = DATA(node);

	    if(value)
	    {
		g_free(mc->mbox);
		mc->mbox = (char *)value;
	    }
	}
	else if(xmlStrEqual(node->name, (const xmlChar *)"Command"))
	{
	    value = DATA(node);

	    if(value)
	    {
		g_free(mc->command);
		mc->command = (char *)value;
	    }

	    value = xmlGetProp(node, "term");

	    if(value)
	    {
		int n = atoi(value);

		if(n == 1)
		    mc->term = TRUE;
		else
		    mc->term = FALSE;

		g_free(value);
	    }
	}
	else if(xmlStrEqual(node->name, (const xmlChar *)"Tooltip"))
	{
	    value = DATA(node);

	    if(value)
	    {
		g_free(mc->tooltip);
		mc->tooltip = (char *)value;
	    }
	}
    }

    mailcheck_set_tip(mc);
}

void mailcheck_write_config(PanelControl *pc, xmlNodePtr parent)
{
    xmlNodePtr root, node;
    char value[MAXSTRLEN + 1];

    t_mailcheck *mc = (t_mailcheck *)pc->data;
    
    root = xmlNewTextChild(parent, NULL, MAILCHECK_ROOT, NULL);

    g_snprintf(value, 4, "%d", mc->interval);
    xmlSetProp(root, "interval", value);
    
    xmlNewTextChild(root, NULL, "Mbox", mc->mbox);

    node = xmlNewTextChild(root, NULL, "Command", mc->command);
    
    snprintf(value, 2, "%d", mc->term);
    xmlSetProp(node, "term", value);

    if (mc->tooltip)
	xmlNewTextChild(root, NULL, "Tooltip", mc->tooltip);
}

static void mailcheck_attach_callback(PanelControl *pc, const char *signal,
				      GCallback callback, gpointer data)
{
    t_mailcheck *mc = pc->data;

    g_signal_connect(mc->button, signal, callback, data);
}

static void run_mail_command(t_mailcheck *mc)
{
    exec_cmd(mc->command, mc->term);
}

static t_mailcheck *mailcheck_new(void)
{
    PanelItem *pi;
    t_mailcheck *mailcheck;
    const char *mail;

    mailcheck = g_new(t_mailcheck, 1);

    mailcheck->status = NO_MAIL;
    mailcheck->mbox = NULL;
    mailcheck->command = NULL;
    mailcheck->tooltip = NULL;
    mailcheck->interval = 30;
    mailcheck->timeout_id = 0;

    mailcheck->nomail_pb = get_mailcheck_pixbuf(NO_MAIL);
    mailcheck->oldmail_pb = get_mailcheck_pixbuf(OLD_MAIL);
    mailcheck->newmail_pb = get_mailcheck_pixbuf(NEW_MAIL);

    mail = g_getenv("MAIL");

    if(mail)
        mailcheck->mbox = g_strdup(mail);
    else
    {
        const char *logname = g_getenv("LOGNAME");
        mailcheck->mbox = g_strconcat("/var/spool/mail/", logname, NULL);
    }

    mailcheck->command = g_strdup("pine");
    mailcheck->term = TRUE;

    mailcheck->button = xfce_iconbutton_new_from_pixbuf(mailcheck->nomail_pb);
    gtk_widget_show(mailcheck->button);
    gtk_button_set_relief(GTK_BUTTON(mailcheck->button), GTK_RELIEF_NONE);

    g_signal_connect_swapped(mailcheck->button, 
	    		     "clicked", G_CALLBACK(run_mail_command), 
			     mailcheck);

    mailcheck_set_tip(mailcheck);
    
    return mailcheck;
}

void mailcheck_free(PanelControl * pc)
{
    t_mailcheck *mailcheck = (t_mailcheck *) pc->data;

    if (mailcheck->timeout_id > 0)
	g_source_remove(mailcheck->timeout_id);
    
    g_free(mailcheck->mbox);
    g_free(mailcheck->command);

    g_object_unref(mailcheck->nomail_pb);
    g_object_unref(mailcheck->oldmail_pb);
    g_object_unref(mailcheck->newmail_pb);

    g_free(mailcheck);
}

static gboolean check_mail(t_mailcheck *mailcheck)
{
    int mail;
    struct stat s;

    if(stat(mailcheck->mbox, &s) < 0)
        mail = NO_MAIL;
    else if(!s.st_size)
        mail = NO_MAIL;
    else if(s.st_mtime < s.st_atime)
        mail = OLD_MAIL;
    else
        mail = NEW_MAIL;

    if(mail != mailcheck->status)
    {
        mailcheck->status = mail;

        if(mail == NO_MAIL)
            xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(mailcheck->button), mailcheck->nomail_pb);
        else if(mail == OLD_MAIL)
            xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(mailcheck->button), mailcheck->oldmail_pb);
        else
            xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(mailcheck->button), mailcheck->newmail_pb);
    }

    /* keep the g_timeout running */
    return TRUE;
}

static void run_mailcheck(PanelControl *pc)
{
    t_mailcheck *mc = pc->data;

    if (mc->timeout_id > 0)
    {
	g_source_remove(mc->timeout_id);
	mc->timeout_id = 0;
    }

    if (mc->interval > 0)
    {
	mc->timeout_id = g_timeout_add(mc->interval*1000, 
		(GSourceFunc)check_mail, mc);
    }
}

static void mailcheck_set_theme(PanelControl *pc, const char *theme)
{
    t_mailcheck *mailcheck = (t_mailcheck *) pc->data;

    g_object_unref(mailcheck->nomail_pb);
    g_object_unref(mailcheck->oldmail_pb);
    g_object_unref(mailcheck->newmail_pb);

    mailcheck->nomail_pb = get_mailcheck_pixbuf(NO_MAIL);
    mailcheck->oldmail_pb = get_mailcheck_pixbuf(OLD_MAIL);
    mailcheck->newmail_pb = get_mailcheck_pixbuf(NEW_MAIL);

    if(mailcheck->status == NO_MAIL)
	xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(mailcheck->button), mailcheck->nomail_pb);
    else if(mailcheck->status == OLD_MAIL)
	xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(mailcheck->button), mailcheck->oldmail_pb);
    else
	xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(mailcheck->button), mailcheck->newmail_pb);
}

/*  Options dialog
 *  --------------
*/
static GtkWidget *mbox_entry;
static GtkWidget *command_entry;
static GtkWidget *term_checkbutton;
static GtkWidget *spinbutton;

static GtkWidget *revert_button;

struct BackUp 
{
    char *mbox;
    char *command;
    gboolean term;
    int interval;
    char *tooltip;
};

static struct BackUp backup;

static void create_mailcheck_backup(t_mailcheck *mc)
{
    backup.mbox = g_strdup(mc->mbox);
    backup.command = g_strdup(mc->command);
    backup.term = mc->term;
    backup.interval = mc->interval;
    backup.tooltip = g_strdup(mc->tooltip);
}

static void clean_mailcheck_backup(void)
{
    g_free(backup.mbox);
    backup.mbox = NULL;
    g_free(backup.command);
    backup.command = NULL;
    g_free(backup.tooltip);
}

static void update_options_box(t_mailcheck *mc)
{
    gtk_entry_set_text(GTK_ENTRY(mbox_entry), mc->mbox);
    gtk_entry_set_text(GTK_ENTRY(command_entry), mc->command);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(term_checkbutton),
                                 mc->term);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton), mc->interval);
}

static void mailcheck_revert_options(PanelControl *pc)
{
    t_mailcheck *mc = (t_mailcheck *) pc->data;

    g_free(mc->mbox);
    mc->mbox = g_strdup(backup.mbox);

    g_free(mc->command);
    mc->command = g_strdup(backup.command);

    mc->term = backup.term;
    mc->interval = backup.interval;

    update_options_box(mc);
}

static void mailcheck_apply_options(PanelControl * pc)
{
    const char *tmp;
    t_mailcheck *mc = (t_mailcheck *) pc->data;

    tmp = gtk_entry_get_text(GTK_ENTRY(command_entry));

    if(tmp && *tmp)
    {
        g_free(mc->command);
        mc->command = g_strdup(tmp);
    }

    mc->term =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(term_checkbutton));

    tmp = gtk_entry_get_text(GTK_ENTRY(mbox_entry));

    if(tmp && *tmp)
    {
        g_free(mc->mbox);
        mc->mbox = g_strdup(tmp);
    }

    mc->interval = 
	gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton));

    run_mailcheck(pc);

    mailcheck_set_tip(mc);
}

static void browse_cb(GtkWidget * b, GtkEntry * entry)
{
    const char *text = gtk_entry_get_text(entry);
    char *file;

    file = select_file_name(NULL, text, NULL);

    if(file)
    {
        gtk_entry_set_text(entry, file);
	make_sensitive(revert_button);
    }
}

gboolean mailcheck_entry_changed(PanelControl *pc)
{
    mailcheck_apply_options(pc);

    make_sensitive(revert_button);
    
    /* needed to let entry handle focus-out as well */
    return FALSE;
}

void mailcheck_add_options(PanelControl * pc, GtkContainer * container, 
			   GtkWidget *revert, GtkWidget *done)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *mbox_button;
    GtkWidget *command_button;
    GtkSizeGroup *sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
    t_mailcheck *mc = (t_mailcheck *) pc->data;

    create_mailcheck_backup(mc);
    revert_button = revert;
    
    vbox = gtk_vbox_new(TRUE, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    /* mbox location */
    hbox = gtk_hbox_new(FALSE, 4);

    label = gtk_label_new(_("Mail box:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    mbox_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), mbox_entry, TRUE, TRUE, 0);

    mbox_button = gtk_button_new_with_label(" ... ");
    gtk_box_pack_start(GTK_BOX(hbox), mbox_button, FALSE, FALSE, 0);

    g_signal_connect(mbox_button, "clicked", G_CALLBACK(browse_cb), mbox_entry);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    /* activate revert button when changing the label */
    g_signal_connect_swapped(mbox_entry, "insert-at-cursor",
                             G_CALLBACK(make_sensitive), revert_button);
    g_signal_connect(mbox_entry, "delete-from-cursor",
                     G_CALLBACK(make_sensitive), revert_button);

    /* only set label on focus out */
    g_signal_connect_swapped(mbox_entry, "focus-out-event",
                     G_CALLBACK(mailcheck_entry_changed), pc);

    /* mail command */
    hbox = gtk_hbox_new(FALSE, 4);

    label = gtk_label_new(_("Mail command:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    command_entry = gtk_entry_new();
    gtk_tooltips_set_tip(tooltips, command_entry,
                         _
                         ("Command to run when the button on the panel is clicked"),
                         NULL);
    gtk_box_pack_start(GTK_BOX(hbox), command_entry, TRUE, TRUE, 0);

    command_button = gtk_button_new_with_label(" ... ");
    gtk_box_pack_start(GTK_BOX(hbox), command_button, FALSE, FALSE, 0);

    g_signal_connect(command_button, "clicked", G_CALLBACK(browse_cb),
                     command_entry);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    /* activate revert button when changing the label */
    g_signal_connect_swapped(command_entry, "insert-at-cursor",
                             G_CALLBACK(make_sensitive), revert_button);
    g_signal_connect_swapped(command_entry, "delete-from-cursor",
                     	     G_CALLBACK(make_sensitive), revert_button);

    /* only set label on focus out */
    g_signal_connect_swapped(command_entry, "focus-out-event",
                     G_CALLBACK(mailcheck_entry_changed), pc);

    /* run in terminal ? */
    hbox = gtk_hbox_new(FALSE, 4);

    label = gtk_label_new(_(""));
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    term_checkbutton = gtk_check_button_new_with_mnemonic(_("Run in _terminal"));
    gtk_box_pack_start(GTK_BOX(hbox), term_checkbutton, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    g_signal_connect_swapped(term_checkbutton, "toggled",
                     	     G_CALLBACK(make_sensitive), revert_button);

    g_signal_connect_swapped(term_checkbutton, "toggled",
                     	     G_CALLBACK(mailcheck_apply_options), pc);

    /* mail check interval */
    hbox = gtk_hbox_new(FALSE, 4);

    label = gtk_label_new(_("Interval (sec):"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    spinbutton = gtk_spin_button_new_with_range(1, 600, 1);
    gtk_box_pack_start(GTK_BOX(hbox), spinbutton, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
    
    g_signal_connect_swapped(spinbutton, "value-changed",
                     	     G_CALLBACK(make_sensitive), revert_button);

    g_signal_connect_swapped(spinbutton, "value-changed",
                     	     G_CALLBACK(mailcheck_apply_options), pc);

    update_options_box(mc);
    
    gtk_widget_show_all(vbox);

    /* signals */
    g_signal_connect_swapped(revert, "clicked", 
	    	     	     G_CALLBACK(mailcheck_revert_options), pc);

    g_signal_connect_swapped(done, "clicked", 
	    	     	     G_CALLBACK(mailcheck_apply_options), pc);

    gtk_container_add(container, vbox);
}

/* this must be called 'module_init', because that is what we look for 
 * when opening the gmodule */
void module_init(PanelControl * pc)
{
    t_mailcheck *mailcheck = mailcheck_new();
    GtkWidget *b = mailcheck->button;

    gtk_container_add(GTK_CONTAINER(pc->base), b);

    pc->caption = g_strdup(_("Mail check"));
    pc->data = (gpointer) mailcheck;

    pc->free = mailcheck_free;
    pc->read_config = mailcheck_read_config;
    pc->write_config = mailcheck_write_config;
    pc->attach_callback = mailcheck_attach_callback;

    pc->add_options = (gpointer) mailcheck_add_options;

    pc->set_theme = mailcheck_set_theme;

    run_mailcheck(pc);
}
