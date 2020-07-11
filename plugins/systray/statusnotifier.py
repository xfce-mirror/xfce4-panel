 #
 # Copyright (C) 2020 Simon Steinbeiß <simon@xfce.org>
 #
 # This program is free software; you can redistribute it and/or modify
 # it under the terms of the GNU General Public License as published by
 # the Free Software Foundation; either version 2 of the License, or
 # (at your option) any later version.
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License along
 # with this program; if not, write to the Free Software Foundation, Inc.,
 # 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 #


"""Minimalistic StatusIcon test for the Xfce Panel
"""

import sys
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GLib
gi.require_version('AppIndicator3', '0.1')
from gi.repository import AppIndicator3 as appindicator



class XfceStatusIcon():

    def __init__(self):
        icon_theme = Gtk.IconTheme.get_default()
        icon_name = "xfce4-logo"

        # Create StatusNotifierItem
        self.indicator = appindicator.Indicator.new(
            "xfce-sample-statusicon",
            icon_name,
            appindicator.IndicatorCategory.APPLICATION_STATUS)
        self.indicator.set_status(appindicator.IndicatorStatus.ACTIVE)

        # Create menu
        status_menu = Gtk.Menu()

        # Add dummy icon check
        check_menu_item = Gtk.CheckMenuItem.new_with_label("Update icon")
        check_menu_item.connect("activate", self.check_menu_item_cb)
        status_menu.append(check_menu_item)

        # Add quit menu item
        quit_menu_item = Gtk.MenuItem.new_with_label("Quit")
        quit_menu_item.connect("activate", self.quit_menu_item_cb)
        status_menu.append(quit_menu_item)

        # Set the menu
        status_menu.show_all()
        self.indicator.set_menu(status_menu)

        # Handle icon theme changes
        icon_theme.connect('changed', self.icon_theme_changed_cb)

    def update_icon(self, icon_name):
        self.indicator.set_icon(icon_name)

    def check_menu_item_cb(self, widget, data=None):
        if widget.get_active():
            icon_name = "parole"
        else:
            icon_name = "xfce4-logo"
        self.update_icon(icon_name)

    def icon_theme_changed_cb(self, theme):
        self.update_icon("xfce4-logo")

    def quit_menu_item_cb(self, widget, data=None):
        Gtk.main_quit()



if __name__ == '__main__':
    try:
        XfceStatusIcon()
        Gtk.main()
    except:
        sys.exit(-1)
