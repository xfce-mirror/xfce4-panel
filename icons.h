#ifndef __XFCE__ICONS__H__
#define __XFCE__ICONS__H__

enum
{
  /* id for external icons 
     this is -1 to allow adding icons without having to change
     configuration files 
  */
  EXTERN_ICON=-1,
  /* general icons */
  DEFAULT_ICON,
  EDIT_ICON,
  FILE1_ICON,
  FILE2_ICON,
  GAMES_ICON,
  MAN_ICON,
  MULTIMEDIA_ICON,
  NETWORK_ICON,
  PAINT_ICON,
  PRINT_ICON,
  SCHEDULE_ICON,
  SOUND_ICON,
  TERMINAL_ICON,
  NUM_ICONS,
  /* icons for the panel */
  MINILOCK_ICON,
  MINIINFO_ICON,
  MINIPALET_ICON,
  MINIPOWER_ICON,
  MINI_ICONS,
  HANDLE_ICON,
  ADDICON_ICON,
  CLOSE_ICON,
  ICONIFY_ICON,
  UP_ICON,
  DOWN_ICON,
  /* program icons */
  DIAG_ICON,
  MENU_ICON,
  XFCE_ICON
};

char *icon_names [NUM_ICONS];

#define UNKNOWN_ICON DEFAULT_ICON

enum
{
  TRASH_EMPTY_ICON,
  TRASH_FULL_ICON,
  TRASH_ICONS
};

enum
{
  PPP_OFF_ICON,
  PPP_ON_ICON,
  PPP_CONNECTING_ICON,
  PPP_ICONS
};

void create_pixbufs (void);

GdkPixbuf *get_pixbuf_from_id (int id);

/* for modules */

GdkPixbuf *get_mailcheck_pixbuf (int id);

GdkPixbuf *get_trashcan_pixbuf (int id);

#endif
