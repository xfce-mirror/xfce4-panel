#include <sys/stat.h>
#include <gtk/gtk.h>

#include "global.h"
#include "module.h"
#include "item.h"

/* mailcheck icons */
#include "icons/mail.xpm"
#include "icons/nomail.xpm"
#include "icons/oldmail.xpm"

static GtkTooltips *tooltips = NULL;

/* this is checked when the module is loaded */
int is_xfce_panel_module = 1;

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Mailcheck module

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

enum
{
    NO_MAIL,
    NEW_MAIL,
    OLD_MAIL
};

typedef struct
{
    char *mbox;

    int status;

    int size;

    GdkPixbuf *nomail_pb;
    GdkPixbuf *newmail_pb;
    GdkPixbuf *oldmail_pb;

    /* we overload the panel item struct a bit */
    PanelItem *item;
}
t_mailcheck;

static char *icon_suffix[] = {
    "png",
    "xpm",
    NULL
};

static char *mailcheck_icon_names[] = {
    "nomail",
    "newmail",
    "oldmail"
};

static char **get_icon_paths(void)
{
    char **dirs = g_new0(char *, 3);

    dirs[0] = g_build_filename(g_getenv("HOME"), RCDIR, "panel/themes", NULL);
    dirs[1] = g_build_filename(DATADIR, "panel/themes", NULL);

    return dirs;
}

static GdkPixbuf *get_themed_mailcheck_pixbuf(int id, const char *theme)
{
    GdkPixbuf *pb = NULL;
    char *name = mailcheck_icon_names[id];
    char **icon_paths = get_icon_paths();
    char **p;

    if(!theme)
        return NULL;

    for(p = icon_paths; *p; p++)
    {
        char **suffix;

        for(suffix = icon_suffix; *suffix; suffix++)
        {
            char *path = g_strconcat(*p, "/", theme, "/", name, ".", *suffix, NULL);

            if(g_file_test(path, G_FILE_TEST_EXISTS))
                pb = gdk_pixbuf_new_from_file(path, NULL);

            g_free(path);

            if(pb)
                break;
        }

        if(pb)
            break;
    }

    g_strfreev(icon_paths);

    if(pb)
        return pb;
    else
        return NULL;
}

static GdkPixbuf *get_mailcheck_pixbuf(int id)
{
    GdkPixbuf *pb;

    if (settings.icon_theme)
	pb = get_themed_mailcheck_pixbuf(id, settings.icon_theme);
    
    if(id == NEW_MAIL)
        pb = gdk_pixbuf_new_from_xpm_data((const char **)mail_xpm);
    else if(id == OLD_MAIL)
        pb = gdk_pixbuf_new_from_xpm_data((const char **)oldmail_xpm);
    else
        pb = gdk_pixbuf_new_from_xpm_data((const char **)nomail_xpm);

    return pb;
}

static t_mailcheck *mailcheck_new(PanelGroup *pg)
{
    t_mailcheck *mailcheck = g_new(t_mailcheck, 1);
    PanelItem *pi;
    const char *mail, *logname;

    /* the mbox */
    mail = g_getenv("MAIL");

    if(mail)
        mailcheck->mbox = g_strdup(mail);
    else
    {
        logname = g_getenv("LOGNAME");
        mailcheck->mbox = g_strconcat("/var/spool/mail/", logname, NULL);
    }

    if (!tooltips)
	tooltips = gtk_tooltips_new();

    /* TODO : read xml config here */

    mailcheck->status = NO_MAIL;

    mailcheck->size = MEDIUM;

    mailcheck->nomail_pb = get_mailcheck_pixbuf(NO_MAIL);
    mailcheck->oldmail_pb = get_mailcheck_pixbuf(OLD_MAIL);
    mailcheck->newmail_pb = get_mailcheck_pixbuf(NEW_MAIL);

    pi = mailcheck->item = panel_item_new(pg);
    pi->command = g_strdup("sylpheed");
    pi->tooltip = g_strdup("Sylpheed");

    create_panel_item(pi);

    g_object_unref (mailcheck->item->pb);
    pi->pb = mailcheck->nomail_pb;
    panel_item_set_size(pi, mailcheck->size);
    
    return mailcheck;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void mailcheck_pack(PanelModule *pm, GtkBox *box)
{
    gtk_box_pack_start(box, pm->main, TRUE, TRUE, 0);
}

void mailcheck_unpack(PanelModule *pm, GtkContainer *container)
{
    gtk_container_remove(container, pm->main);
}

void mailcheck_free(PanelModule *pm)
{
    t_mailcheck *mailcheck = (t_mailcheck *) pm->data;

    g_object_ref(mailcheck->item->pb);
    panel_item_free(mailcheck->item);
    
    if (GTK_IS_WIDGET(pm->main))
	gtk_widget_destroy(pm->main);
    
    g_free(mailcheck->mbox);

    g_object_unref(mailcheck->nomail_pb);
    g_object_unref(mailcheck->oldmail_pb);
    g_object_unref(mailcheck->newmail_pb);

    g_free(mailcheck);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

static gboolean check_mail(PanelModule *pm)
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

	panel_item_set_size (pi, mailcheck->size);
    }

    return TRUE;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void mailcheck_set_size(PanelModule *pm, int size)
{
    t_mailcheck *mailcheck = (t_mailcheck *) pm->data;
    
    panel_item_set_size(mailcheck->item, size);

    mailcheck->size = size;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void mailcheck_configure(PanelModule * pm)
{
    GtkWidget *dialog;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *mbox_entry;
    GtkWidget *mbox_button;
    GtkWidget *command_entry;
    GtkWidget *command_button;
    GtkWidget *spinbutton;
    GtkSizeGroup *sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    int response;
    t_mailcheck *mc = (t_mailcheck *) pm->data;

    dialog = gtk_dialog_new_with_buttons(_("Mail check options"), NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_APPLY, GTK_RESPONSE_OK,
                                         NULL);

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

    set_transient_for_dialog(dialog);

    vbox = gtk_vbox_new(TRUE, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), vbox, FALSE, TRUE, 0);

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

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

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

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 4);

    label = gtk_label_new(_("Interval (sec):"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    spinbutton = gtk_spin_button_new_with_range(1, 600, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton), pm->interval / 1000);
    gtk_box_pack_start(GTK_BOX(hbox), spinbutton, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    gtk_widget_show_all(vbox);

    response = gtk_dialog_run(GTK_DIALOG(dialog));

    if(response == GTK_RESPONSE_OK)
    {
        const char *tmp;

        tmp = gtk_entry_get_text(GTK_ENTRY(command_entry));

        if(tmp && *tmp)
        {
            gtk_tooltips_set_tip(tooltips, mc->item->button, tmp, NULL);
            g_free(mc->item->command);
            mc->item->command = g_strdup(tmp);
        }

        tmp = gtk_entry_get_text(GTK_ENTRY(mbox_entry));

        if(tmp && *tmp)
        {
            g_free(mc->mbox);
            mc->mbox = g_strdup(tmp);
        }
    }

    gtk_widget_destroy(dialog);
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
    
    pm->interval = 5000; /* 5 sec */
    pm->update = (gpointer) check_mail;
    
    pm->pack = (gpointer) mailcheck_pack;
    pm->unpack = (gpointer) mailcheck_unpack;
    pm->free = (gpointer) mailcheck_free;
    
    pm->set_size = (gpointer) mailcheck_set_size;
    pm->configure = (gpointer) mailcheck_configure;
    
    if (pm->parent)
	check_mail(pm);
}
