#ifndef __XFCE_WMHINTS_H__
#define __XFCE_WMHINTS_H__

void create_atoms (void);

void net_wm_support_check (void);

void watch_root_properties (void);

void net_current_desktop_set (int n);
void net_number_of_desktops_set (int n);
void net_desktop_name_set (int n, const char *name);

int net_current_desktop_get (void);
int net_number_of_desktops_get (void);
char **net_desktop_name_get (void);

#endif /* __XFCE_WMHINTS_H__ */
