#!/usr/bin/python

import subprocess
import curses, os, sys, traceback

# global variables
fujitsu_utility_helper="/usr/bin/fujitsu_touchscreen_helper"

MODULE = "fujitsu_usb_touchscreen"
MODULE_PATH = "/sys/module/"+MODULE+"/parameters/"

scrn = None
o_minx = o_maxx = o_miny = o_maxy = -1
l_minx = l_maxx = l_miny = l_maxy = -1
c_minx = c_maxx = c_miny = c_maxy = -1

def runpsax():
    p = os.popen('ps ax','r')
    gb.cmdoutlines = []
    row = 0
    for ln in p:
      # don't allow line wraparound, so truncate long lines
      ln = ln[:curses.COLS]
      # remove EOLN if it is still there
      if ln[-1] == '\n': ln = ln[:-1]
      gb.cmdoutlines.append(ln)

    p.close()

def replaceline(l,tx,att=curses.A_NORMAL):
  global scrn
  scrn.move(l,0)
  scrn.clrtoeol()
  scrn.addstr(l,0,tx,att)
  scrn.refresh()

def get_module_param(pname):
  global MODULE_PATH
  p = open(MODULE_PATH+pname)
  pval=p.read()
  p.close()
  return pval.strip()

def get_calib_coordinates():
  global c_minx, c_maxx, c_miny, c_maxy
  c_minx=get_module_param("calib_minx")
  c_maxx=get_module_param("calib_maxx")
  c_miny=get_module_param("calib_miny")
  c_maxy=get_module_param("calib_maxy")

def get_calib_initial():
  global o_minx, o_maxx, o_miny, o_maxy
  o_minx=get_module_param("touch_minx")
  o_maxx=get_module_param("touch_maxx")
  o_miny=get_module_param("touch_miny")
  o_maxy=get_module_param("touch_maxy")

def set_values():
  global c_minx, c_maxx, c_miny, c_maxy
  cmdo=subprocess.Popen([fujitsu_utility_helper, "writecalibrate", c_minx, c_miny, c_maxx, c_maxy],stdout=subprocess.PIPE)

def reset_calib():
  cmdo=subprocess.Popen([fujitsu_utility_helper, "resetcalibrate"],stdout=subprocess.PIPE)

def set_calibrationmode(m):
  if m=="ON":
    cmdo=subprocess.Popen([fujitsu_utility_helper, "calibrate", "1"],stdout=subprocess.PIPE)
  else:
    cmdo=subprocess.Popen([fujitsu_utility_helper, "calibrate", "0"],stdout=subprocess.PIPE)


def main():
  global scrn

  curses_set()
  display_screen()
  set_calibrationmode("ON")

  get_calib_initial()
  reset_calib()
  while True:
    get_calib_coordinates()
    display_values()
    swap_c2l()

    c = scrn.getch()

    if c==-1: curses.napms(350) 
    elif c==113 or c==81: break
    elif c==114 or c==82:
      reset_calib()
    elif c==115 or c==83:
      set_values()
      break

  set_calibrationmode("OFF")
  curses_reset()

def swap_c2l():
  global l_minx, l_maxx, l_miny, l_maxy
  global c_minx, c_maxx, c_miny, c_maxy
  l_minx=c_minx
  l_maxx=c_maxx
  l_miny=c_miny
  l_maxy=c_maxy

def display_screen():
  global MODULE
  replaceline(0,MODULE+" module calibration tool",curses.A_REVERSE)
  replaceline(curses.LINES-3,"Press Q to quit")
  replaceline(curses.LINES-2,"      S to set calibration to displayed values and quit")
  replaceline(curses.LINES-1,"      R to reset calibration values")

def display_values():
  replaceline(2,"Old Calibration: x:["+str(o_minx)+","+str(o_maxx)+"] y:["+str(o_miny)+","+str(o_maxy)+"]")
  replaceline(3,"New:  last read: x:["+str(l_minx)+","+str(l_maxx)+"] y:["+str(l_miny)+","+str(l_maxy)+"]")
  replaceline(4,"New:  this read: x:["+str(c_minx)+","+str(c_maxx)+"] y:["+str(c_miny)+","+str(c_maxy)+"]")
  
def curses_set():
  global scrn
  scrn = curses.initscr()
  curses.noecho()
  curses.cbreak()
  curses.flushinp()
  scrn.keypad(1)
  scrn.nodelay(1)

def curses_reset():
  global scrn
  scrn.keypad(0)
  curses.nocbreak()
  curses.echo()
  curses.endwin()

if __name__ =='__main__':
  try:
    main()
  except:
    set_calibrationmode("OFF")
    if scrn!=None:
      curses_reset()
    # print error message re exception
    traceback.print_exc()

