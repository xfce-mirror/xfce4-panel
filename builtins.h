#ifndef __XFCE_BUILTINS_H__
#define __XFCE_BUILTINS_H__

#include "xfce.h"
#include "module.h"

/* builtin modules */
gboolean create_clock_module (PanelModule *pm);

gboolean create_trash_module (PanelModule *pm);

gboolean create_mailcheck_module (PanelModule *pm);

#endif /* __XFCE_BUILTINS_H__ */
