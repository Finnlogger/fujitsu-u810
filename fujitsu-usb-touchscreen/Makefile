# Makefile stolen from "fsc_btns" driver.

KERNELRELEASE	:= $(shell uname -r)
KERNEL_SOURCE	:= /lib/modules/$(KERNELRELEASE)/build
PWD		:= $(shell pwd)

MODULE_NAME	:= fujitsu_usb_touchscreen

ifdef DEFS
EXTRA_CFLAGS	+= $(DEFS)
endif


obj-m		+= $(MODULE_NAME).o

all: $(MODULE_NAME).ko fujitsu_touchscreen_helper
modules: $(MODULE_NAME).ko
modules_install: install

fujitsu_touchscreen_helper: fujitsu_touchscreen_helper.c
	gcc -o fujitsu_touchscreen_helper fujitsu_touchscreen_helper.c

$(MODULE_NAME).ko: $(MODULE_NAME).c
	$(MAKE) -C $(KERNEL_SOURCE) M=$(PWD) modules

install: $(MODULE_NAME).ko fujitsu_touchscreen_helper fujitsu_touchscreen_rotate.py fujitsu_touchscreen_calibration.py
	$(MAKE) -C $(KERNEL_SOURCE) M=$(PWD) modules_install
	if test -x /sbin/depmod; then /sbin/depmod -a $(KERNELRELEASE); fi
	cp fujitsu_touchscreen_helper /usr/bin
	cp fujitsu_touchscreen_rotate.py /usr/bin
	cp fujitsu_touchscreen_calibration.py /usr/bin
	cp fujitsu_touchscreen /etc/init.d/
	ln -fs ../init.d/fujitsu_touchscreen /etc/rc2.d/S99fujitsu_touchscreen
	ln -fs ../init.d/fujitsu_touchscreen /etc/pm/sleep.d/99_fujitsu_touchscreen
	chmod +s /usr/bin/fujitsu_touchscreen_helper
	/etc/init.d/fujitsu_touchscreen start
	if test ! -f /etc/hal/fdi/policy/$(MODULE_NAME).fdi; then /usr/bin/fujitsu_touchscreen_helper writecalibrate 124 266 3827 3940 ; fi

uninstall:
	rmmod fujitsu_usb_touchscreen
	rm -f /lib/modules/$(KERNELRELEASE)/extra/$(MODULE_NAME).ko
	rm -f /usr/bin/fujitsu_touchscreen_helper
	rm -f /usr/bin/fujitsu_touchscreen_rotate.py
	rm -f /usr/bin/fujitsu_touchscreen_calibration.py
	rm -f /etc/modprobe.d/$(MODULE_NAME).conf
	rm -f /etc/hal/fdi/policy/$(MODULE_NAME).fdi
	unlink /etc/rc2.d/S99fujitsu_touchscreen
	unlink /etc/pm/sleep.d/99_fujitsu_touchscreen
	rm -f /etc/init.d/fujitsu_touchscreen

clean:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(PWD) clean
	rm -f fujitsu_touchscreen_helper modules.order Module.markers

distclean: clean
	rm -f Module.symvers

distdir: Makefile $(MODULE_NAME).c
	cp -p $^ $(distdir)

%:
	@echo; \
	 echo " *** unknown target $@ *** "; \
	 echo

.PHONY: all install modules modules_install clean
