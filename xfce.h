#ifndef __XFCE_H__
#define __XFCE_H__

#define _(x) x
#define N_(x) x

#define MAXSTRLEN 1024

#define NBSCREENS 12
#define NBGROUPS 6
#define NBITEMS 10

#define SMALL_PANEL_ICONS 30
#define MEDIUM_PANEL_ICONS 45
#define LARGE_PANEL_ICONS 60

#define SMALL_POPUP_ICONS 24
#define MEDIUM_POPUP_ICONS 32
#define LARGE_POPUP_ICONS 48

#define SCREEN_BUTTON_WIDTH 80

#define TOPHEIGHT 16

#define SMALL_TOPHEIGHT 14
#define MEDIUM_TOPHEIGHT TOPHEIGHT
#define LARGE_TOPHEIGHT TOPHEIGHT

#define RCDIR		".xfce4"
#define RCFILE		"xfce4rc"
#define SYSCONFDIR 	"/etc/X11/xfce4"
#define DATADIR		"/usr/share/xfce4"
#define LIBDIR		"/usr/lib/xfce4"

#include <string.h>
#include <gtk/gtk.h>

#define strequal(s1,s2) !strcmp (s1, s2)
#define strnequal(s1,s2,n) !strncmp (s1, s2, n)

/* panel sides */
enum
{ LEFT, RIGHT };

/* panel styles */
enum
{ OLD_STYLE, NEW_STYLE };

/* panel sizes */
enum
{ SMALL, MEDIUM, LARGE };

#define DEFAULT_SIZE MEDIUM

/* types of panel items */
enum
{ ICON, MODULE };

/*****************************************************************************/

typedef struct _XfcePanel XfcePanel;
typedef struct _CentralPanel CentralPanel;
typedef struct _ScreenButton ScreenButton;
typedef struct _SidePanel SidePanel;
typedef struct _PanelGroup PanelGroup;
typedef struct _MoveHandle MoveHandle;
typedef struct _PanelPopup PanelPopup;
typedef struct _PopupItem PopupItem;
typedef struct _PanelItem PanelItem;
typedef struct _PanelModule PanelModule;

/*****************************************************************************/

struct _XfcePanel
{
  /* position */
  int x;
  int y;

  int style;			/* OLD_STYLE or NEW_STYLE */
  char *icon_theme;

/* SMALL, MEDIUM or LARGE. These are symbolic sizes. 
   They are mapped to real sizes */
  int size;
  int popup_size;

  int num_screens;		/* between 1 and NBSCREENS */
  int current_screen;

  int num_groups_left;		/* between 1 and NBGROUPS */
  int num_groups_right;		/* between 1 and NBGROUPS */

  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *hbox;

  CentralPanel *central_panel;
  SidePanel *left_panel;
  SidePanel *right_panel;

  gpointer xmlnode;
};

/* panel is a global structure private to xfce.c */

/*****************************************************************************/
void quit (void);
void restart (void);

void panel_set_as_move_handle (GtkWidget * widget);
void set_transient_for_panel (GtkWidget * widget);
void iconify_panel (void);
void panel_set_tooltip (GtkWidget * widget, const char *tip);

int icon_size (int size);
int popup_icon_size (int size);
int top_height (int size);

/*****************************************************************************/
/* xsettings ? */
void panel_set_size (int size);
void panel_set_style (int style);
void panel_set_popup_size (int size);
void panel_set_left_groups (int n);
void panel_set_right_groups (int n);

/*****************************************************************************/
/* root properties: change and react to changes */
void change_current_desktop (int n);
void change_number_of_desktops (int n);
void change_desktop_name (int n, char *name);

void current_desktop_changed (int n);
void number_of_desktops_changed (int n);
void desktop_name_changed (int n, char *name);

/*****************************************************************************/

#endif /* __XFCE_H__ */
