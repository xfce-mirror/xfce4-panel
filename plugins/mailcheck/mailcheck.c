/*  mailcheck.c
 *
 *  Copyright (C) Jasper Huijsmans (j.b.huijsmans@hetnet.nl)
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

#include "global.h"
#include "xfce_support.h"
#include "icons.h"
#include "module.h"
#include "item.h"

/* mailcheck icons */
#include "icons/mail.xpm"
#include "icons/nomail.xpm"
#include "icons/oldmail.xpm"

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Globals
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static GtkTooltips *tooltips = NULL;

/* this is checked when the module is loaded */
int is_xfce_panel_module = 1;

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

/* module data structure */
typedef struct
{
    int size;                   /* local copy of settings.size */
    int status;
    int interval;
    char *mbox;

    GdkPixbuf *nomail_pb;
    GdkPixbuf *newmail_pb;
    GdkPixbuf *oldmail_pb;

    /* we overload the panel item struct a bit */
    PanelItem *item;
}
t_mailcheck;

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Icons
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static char *mailcheck_icon_names[] = {
    "nomail",
    "newmail",
    "oldmail"
};

static GdkPixbuf *get_mailcheck_pixbuf(int id)
{
    GdkPixbuf *pb;

    if(settings.icon_theme)
    {
        char *name = mailcheck_icon_names[id];
        pb = get_themed_pixbuf(name);

        if(GDK_IS_PIXBUF(pb))
            return pb;
    }

    if(id == NEW_MAIL)
        pb = gdk_pixbuf_new_from_xpm_data((const char **)mail_xpm);
    else if(id == OLD_MAIL)
        pb = gdk_pixbuf_new_from_xpm_data((const char **)oldmail_xpm);
    else
        pb = gdk_pixbuf_new_from_xpm_data((const char **)nomail_xpm);

    if(!GDK_IS_PIXBUF(pb))
        pb = get_pixbuf_by_id(UNKNOWN_ICON);

    return pb;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Configuration
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static xmlDocPtr doc = NULL;
#define XMLDATA(node) xmlNodeListGetString(doc, node->children, 1)

static get_mailcheck_config(t_mailcheck * mc)
{
    char *file;
    const char *mail, *logname;
    PanelItem *pi;

    pi = mc->item;

    file = get_read_file("mailcheckrc");

    if(file)
    {
        xmlKeepBlanksDefault(0);
        doc = xmlParseFile(file);
        g_free(file);
    }

    if(doc)
    {
        xmlNodePtr node;
        xmlChar *value;

        node = xmlDocGetRootElement(doc);

        if(!node)
            g_printerr(_("xfce: %s (line %d): empty document\n"), __FILE__,
                       __LINE__);

        if(!xmlStrEqual(node->name, (const xmlChar *)MAILCHECK_ROOT))
        {
            g_printerr(_("xfce: %s (line %d): wrong document type\n"),
                       __FILE__, __LINE__);

            node = NULL;
        }

        if(node)
        {
            /* Now parse the xml tree */
            for(node = node->children; node; node = node->next)
            {
                if(xmlStrEqual(node->name, (const xmlChar *)"Mbox"))
                {
                    value = XMLDATA(node);

                    if(value)
                        mc->mbox = (char *)value;
                }
                else if(xmlStrEqual(node->name, (const xmlChar *)"Command"))
                {
                    value = XMLDATA(node);

                    if(value)
                        mc->item->command = (char *)value;

                    value = xmlGetProp(node, "term");

                    if(value)
                    {
                        int n = atoi(value);

                        if(n == 1)
                            mc->item->in_terminal = TRUE;

                        g_free(value);
                    }
                }
                else if(xmlStrEqual(node->name, (const xmlChar *)"Interval"))
                {
                    value = XMLDATA(node);

                    if(value)
                    {
                        int n = atoi(value);

                        if(n > 0)
                            mc->interval = n;
                    }
                }
            }
        }

        xmlFreeDoc(doc);
    }

    /* the mbox */
    if(!mc->mbox)
    {
        mail = g_getenv("MAIL");

        if(mail)
            mc->mbox = g_strdup(mail);
        else
        {
            logname = g_getenv("LOGNAME");
            mc->mbox = g_strconcat("/var/spool/mail/", logname, NULL);
        }
    }

    if(!pi->command)
        pi->command = g_strdup("sylpheed");

    if(!pi->tooltip)
    {
        pi->tooltip = g_strdup(pi->command);
        g_ascii_toupper(*(pi->command));
    }
}

static write_mailcheck_config(t_mailcheck * mc)
{
    char *dir, *rcfile;
    xmlNodePtr root, node;
    char value[MAXSTRLEN + 1];

    rcfile = get_save_file("mailcheckrc");
    dir = g_path_get_dirname(rcfile);

    if(!g_file_test(dir, G_FILE_TEST_IS_DIR))
        mkdir(dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

    g_free(dir);

    doc = xmlNewDoc("1.0");
    doc->children = xmlNewDocRawNode(doc, NULL, MAILCHECK_ROOT, NULL);

    root = (xmlNodePtr) doc->children;
    xmlDocSetRootElement(doc, root);

    xmlNewTextChild(root, NULL, "Mbox", mc->mbox);

    node = xmlNewTextChild(root, NULL, "Command", mc->item->command);
    snprintf(value, 2, "%d", mc->item->in_terminal);
    xmlSetProp(node, "term", value);

    snprintf(value, MAXSTRLEN, "%d", mc->interval);
    xmlNewTextChild(root, NULL, "Interval", value);

    xmlSaveFormatFile(rcfile, doc, 1);

    xmlFreeDoc(doc);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Creation and destruction
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static t_mailcheck *mailcheck_new(PanelGroup * pg)
{
    PanelItem *pi;
    t_mailcheck *mailcheck;

    mailcheck = g_new(t_mailcheck, 1);
    pi = mailcheck->item = panel_item_new(pg);

    mailcheck->status = NO_MAIL;
    mailcheck->size = settings.size;
    mailcheck->interval = 30;
    mailcheck->mbox = NULL;

    mailcheck->nomail_pb = get_mailcheck_pixbuf(NO_MAIL);
    mailcheck->oldmail_pb = get_mailcheck_pixbuf(OLD_MAIL);
    mailcheck->newmail_pb = get_mailcheck_pixbuf(NEW_MAIL);

    if(!tooltips)
        tooltips = gtk_tooltips_new();

    get_mailcheck_config(mailcheck);

    create_panel_item(pi);
    g_object_unref(pi->pb);
    pi->pb = mailcheck->nomail_pb;
    panel_item_set_size(pi, mailcheck->size);

    return mailcheck;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void mailcheck_pack(PanelModule * pm, GtkBox * box)
{
    gtk_box_pack_start(box, pm->main, TRUE, TRUE, 0);
}

void mailcheck_unpack(PanelModule * pm, GtkContainer * container)
{
    gtk_container_remove(container, pm->main);
}

void mailcheck_free(PanelModule * pm)
{
    t_mailcheck *mailcheck = (t_mailcheck *) pm->data;

    g_object_ref(mailcheck->item->pb);
    panel_item_free(mailcheck->item);

    if(GTK_IS_WIDGET(pm->main))
        gtk_widget_destroy(pm->main);

    g_free(mailcheck->mbox);

    g_object_unref(mailcheck->nomail_pb);
    g_object_unref(mailcheck->oldmail_pb);
    g_object_unref(mailcheck->newmail_pb);

    g_free(mailcheck);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

static gboolean check_mail(PanelModule * pm)
{
    t_mailcheck *mailcheck = (t_mailcheck *) pm->data;
    PanelItem *pi = mailcheck->item;
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
            pi->pb = mailcheck->nomail_pb;
        else if(mail == OLD_MAIL)
            pi->pb = mailcheck->oldmail_pb;
        else
            pi->pb = mailcheck->newmail_pb;

        panel_item_set_size(pi, mailcheck->size);
    }

    return TRUE;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void mailcheck_set_size(PanelModule * pm, int size)
{
    t_mailcheck *mailcheck = (t_mailcheck *) pm->data;

    panel_item_set_size(mailcheck->item, size);

    mailcheck->size = size;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* configuration */

static GtkWidget *mbox_entry;
static GtkWidget *command_entry;
static GtkWidget *term_checkbutton;
static GtkWidget *spinbutton;

static void browse_cb(GtkWidget * b, GtkEntry * entry)
{
    const char *text = gtk_entry_get_text(entry);
    char *file;

    file = select_file_name(NULL, text, NULL);

    if(file)
        gtk_entry_set_text(entry, file);
}

void mailcheck_apply_configuration(PanelModule * pm)
{
    const char *tmp;
    t_mailcheck *mc = (t_mailcheck *) pm->data;

    tmp = gtk_entry_get_text(GTK_ENTRY(command_entry));

    if(tmp && *tmp)
    {
        gtk_tooltips_set_tip(tooltips, mc->item->button, tmp, NULL);
        g_free(mc->item->command);
        mc->item->command = g_strdup(tmp);
    }

    mc->item->in_terminal =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(term_checkbutton));

    tmp = gtk_entry_get_text(GTK_ENTRY(mbox_entry));

    if(tmp && *tmp)
    {
        g_free(mc->mbox);
        mc->mbox = g_strdup(tmp);
    }

    mc->interval = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton));

    write_mailcheck_config(mc);
}

void mailcheck_add_options(PanelModule * pm, GtkContainer * container)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *mbox_button;
    GtkWidget *command_button;
    GtkSizeGroup *sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
    t_mailcheck *mc = (t_mailcheck *) pm->data;

    vbox = gtk_vbox_new(TRUE, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    /* mbox location */
    hbox = gtk_hbox_new(FALSE, 4);

    label = gtk_label_new(_("Mail box:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    mbox_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(mbox_entry), mc->mbox);
    gtk_box_pack_start(GTK_BOX(hbox), mbox_entry, TRUE, TRUE, 0);

    mbox_button = gtk_button_new_with_label(" ... ");
    gtk_box_pack_start(GTK_BOX(hbox), mbox_button, FALSE, FALSE, 0);

    g_signal_connect(mbox_button, "clicked", G_CALLBACK(browse_cb), mbox_entry);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

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
    gtk_entry_set_text(GTK_ENTRY(command_entry), mc->item->command);
    gtk_box_pack_start(GTK_BOX(hbox), command_entry, TRUE, TRUE, 0);

    command_button = gtk_button_new_with_label(" ... ");
    gtk_box_pack_start(GTK_BOX(hbox), command_button, FALSE, FALSE, 0);

    g_signal_connect(command_button, "clicked", G_CALLBACK(browse_cb),
                     command_entry);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    /* run in terminal ? */
    hbox = gtk_hbox_new(FALSE, 4);

    label = gtk_label_new(_(""));
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    term_checkbutton = gtk_check_button_new_with_mnemonic(_("Run in _terminal"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(term_checkbutton),
                                 mc->item->in_terminal);
    gtk_box_pack_start(GTK_BOX(hbox), term_checkbutton, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    /* mail check interval */
    hbox = gtk_hbox_new(FALSE, 4);

    label = gtk_label_new(_("Interval (sec):"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    spinbutton = gtk_spin_button_new_with_range(1, 600, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton), mc->interval);
    gtk_box_pack_start(GTK_BOX(hbox), spinbutton, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    gtk_widget_show_all(vbox);

    gtk_container_add(container, vbox);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/* this must be called 'module_init', because that is what we look for 
 * when opening the gmodule */
void module_init(PanelModule * pm)
{
    t_mailcheck *mailcheck = mailcheck_new(pm->parent);

    pm->caption = g_strdup(_("Mail check"));
    pm->data = (gpointer) mailcheck;
    pm->main = mailcheck->item->button;

    pm->interval = 1000 * mailcheck->interval;  /* in msec */
    pm->update = (gpointer) check_mail;

    pm->pack = (gpointer) mailcheck_pack;
    pm->unpack = (gpointer) mailcheck_unpack;
    pm->free = (gpointer) mailcheck_free;

    pm->set_size = (gpointer) mailcheck_set_size;

    pm->add_options = (gpointer) mailcheck_add_options;
    pm->apply_configuration = (gpointer) mailcheck_apply_configuration;

    if(pm->parent)
        check_mail(pm);
}
