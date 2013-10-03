#!/usr/bin/env python
# Fujitsu U810 rotate utility
#
#  utility lo listen for fjbtndrv d-bus commands
#    for screen rotation events and make the
#    u810_tablet module know about the changes
#    so touchscreen works in rotated modes
#
# History:
# 0.1   09-03-31 zmiq2  Created
#
# Author:  zmiq2
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

import subprocess
from string import find
from time import sleep
import dbus, gobject

xrandr_path="xrandr"
xrandr_wait=3
fsc_btns_interface="org.freedesktop.Hal.Device"
fujitsu_utility_helper="/usr/bin/fujitsu_touchscreen_helper"

def setTouchscreenOrientation():
	#print "Screen orientation change detected"
	sleep(xrandr_wait)
	xrandr=subprocess.Popen(xrandr_path,stdout=subprocess.PIPE)
	xrandr_out=xrandr.stdout.read()
        xrandr_outa=xrandr_out.split("\n")
	for ln in xrandr_outa:
		if find(ln,"LVDS")>-1:
			#default
			u810_tablet_orientation=0

			lvds_a=ln.split(" ")
			i=0
			while i<len(lvds_a):
				orientation=lvds_a[i]

				if orientation == "left":
					u810_tablet_orientation=1
					break
				elif orientation == "inverted":
					u810_tablet_orientation=2
					break
				elif orientation == "right":
					u810_tablet_orientation=3
					break
				elif orientation == "(normal":
					orientation="normal"
					u810_tablet_orientation=0
					break
				else:
					i=i+1

			send_notification("fujitsu touchscreen", "rotating touchscreen to <b>"+orientation+"</b>")
			setOrientation(u810_tablet_orientation)

def setOrientation(id):
	cmdo=subprocess.Popen([fujitsu_utility_helper, "orientation", str(id)],stdout=subprocess.PIPE)


def send_notification(notify_title, notify_text):
	notify_iface.Notify("fujitsu touchscreen", 0, "", notify_title, notify_text, [], {}, -1)

def dbus_wrapper1(p1, p2):
	if str(p1) == "ButtonPressed" and (str(p2)=="tablet_mode" or str(p2)=="direction"):
		setTouchscreenOrientation()
		debug_print("w1",p1,p2)

def dbus_wrapper2(p1, p2):
	setTouchscreenOrientation()
	debug_print("w2",p1,p2)

def debug_print(s, p1, p2):
	f = open('/tmp/debug_print.txt', 'a')
	f.write(s+"\t"+str(p1)+"\t"+str(p2)+"\n")
	f.close()



from dbus.mainloop.glib import DBusGMainLoop
dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
bus = dbus.SystemBus()

bus.add_signal_receiver(dbus_wrapper1, dbus_interface=fsc_btns_interface, signal_name="Condition")
#bus.add_signal_receiver(dbus_wrapper2, dbus_interface=fsc_btns_interface, signal_name="PropertyModified")

sessbus = dbus.SessionBus()
notify_object = sessbus.get_object("org.freedesktop.Notifications", "/org/freedesktop/Notifications")
notify_iface=dbus.Interface(notify_object, "org.freedesktop.Notifications")

loop = gobject.MainLoop()
loop.run()
