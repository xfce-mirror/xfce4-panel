#!/usr/bin/env python3

# You can generate python documentation with the following commands:
# g-ir-doc-tool --language Python -o ./html ../libxfce4panel/Libxfce4panel-2.0.gir
# yelp ./html/

import gi.repository
# Set the search path to use the newly generated introspection files
gi.require_version('GIRepository', '2.0')
from gi.repository import GIRepository
GIRepository.Repository.prepend_search_path('../libxfce4panel/')
# Now import libxfce4panel
gi.require_version('Libxfce4panel', '2.0')
from gi.repository import Libxfce4panel

# xfce-panel API docs url: https://developer.xfce.org/xfce4-panel/
# print out some stuff to see if it works
print("list of the attributes and methods of Libxfce4panel \n", dir(Libxfce4panel))
print("Panel channel name:", Libxfce4panel.panel_get_channel_name())
print("Current Xfce-Panel Version: {}.{}.{}".format(Libxfce4panel.MAJOR_VERSION,
						     Libxfce4panel.MICRO_VERSION,
						     Libxfce4panel.MINOR_VERSION))
