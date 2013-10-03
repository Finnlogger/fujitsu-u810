/*
 *  Copyright (c) 2009, 2010 zmiq2
 *
 *  Rotate touchscreen helper for Fujitsu USB Touchscreen U810, P1620
 *
 *  2009.04.08 zmiq2: created
 *  2009.05.18 zmiq2: updated to allow writing of calibration values
 *  2009.07.15 zmiq2: fixed bug to work on a T1010 IA64
 *                   
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#define MODULE	"fujitsu_usb_touchscreen"

#define FDI_FNAME	"/etc/hal/fdi/policy/" MODULE ".fdi"
#define MOD_FNAME	"/etc/modprobe.d/" MODULE ".conf"
#define MOD_SYS_PATH	"/sys/module/" MODULE "/parameters"

#define SYS_ORIENTATION	MOD_SYS_PATH "/orientation"
#define SYS_CALIBRATE	MOD_SYS_PATH "/calibrate"
#define SYS_MINX	MOD_SYS_PATH "/touch_minx"
#define SYS_MINY	MOD_SYS_PATH "/touch_miny"
#define SYS_MAXX	MOD_SYS_PATH "/touch_maxx"
#define SYS_MAXY	MOD_SYS_PATH "/touch_maxy"
#define SYS_CALIBMINX	MOD_SYS_PATH "/calib_minx"
#define SYS_CALIBMINY	MOD_SYS_PATH "/calib_miny"
#define SYS_CALIBMAXX	MOD_SYS_PATH "/calib_maxx"
#define SYS_CALIBMAXY	MOD_SYS_PATH "/calib_maxy"

#define RMMOD		"/sbin/rmmod"
#define MODPROBE	"/sbin/modprobe"
#define RMMOD_MODULE	RMMOD " " MODULE
#define MODPROBE_MODULE	MODPROBE " " MODULE

#include <stdio.h>
#include <stdlib.h>

void show_help(void) {
	printf("fujitsu usb touchscreen utility\n");
	printf("usage:\n");
	printf("	orientation 0|1|2|3: set driver orientation\n");
	printf("	calibrate 0|1: activate calibration mode\n");
	printf("	writecalibrate a b c d: write calibration values\n");
}

void set_param(char * ppath, int ival, char * sval)
{
	FILE *fd;
	fd=fopen(ppath,"w");
        if(fd) {
	  if(ival!=-2) fprintf(fd,"%d",ival);
	  else         fprintf(fd,"%s",sval);
	  fclose(fd);
        }
        else {
          printf("%s::error::parameter not available [%s]\n",MODULE,ppath);
        }
}

main(int argc, char** argv)
{
	FILE *fd;
	int pval;

	if(argc<2) {
		show_help();
		exit(1);
	}

	/* set driver orientation */
	if(strncmp(argv[1],"orientation",11)==0) {
		if(argc<3) {
			show_help();
			exit(1);
		}

		pval = -1;

		if(strncmp(argv[2],"0",1)==0) pval=0;
		if(strncmp(argv[2],"1",1)==0) pval=1;
		if(strncmp(argv[2],"2",1)==0) pval=2;
		if(strncmp(argv[2],"3",1)==0) pval=3;

		if(pval>-1) set_param(SYS_ORIENTATION, pval, "");
	}

	/* activate calibration */
	if(strncmp(argv[1],"calibrate",9)==0) {
		if(argc<3) {
			show_help();
			exit(1);
		}

		pval = -1;

		if(strncmp(argv[2],"0",1)==0) pval=0;
		if(strncmp(argv[2],"1",1)==0) pval=1;

		if(pval>-1) set_param(SYS_CALIBRATE, pval, "");
	}

	/* reset calibration values */
	if(strncmp(argv[1],"resetcalibrate",14)==0) {
		set_param(SYS_CALIBMINX, 0, "");
		set_param(SYS_CALIBMINY, 0, "");
		set_param(SYS_CALIBMAXX, 0, "");
		set_param(SYS_CALIBMAXY, 0, "");
	}

	/* write calibration values to files */
	if(strncmp(argv[1],"writecalibrate",14)==0) {
		if(argc<6) {
			show_help();
			exit(1);
		}

		set_param(SYS_MINX, -2, argv[2]);
		set_param(SYS_MINY, -2, argv[3]);
		set_param(SYS_MAXX, -2, argv[4]);
		set_param(SYS_MAXY, -2, argv[5]);

		fd=fopen(FDI_FNAME,"w");
                if(fd) {
			fprintf(fd,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
\n<deviceinfo version=\"0.3.5\">\
\n    <device>\
\n        <match key=\"info.product\" contains=\"Fujitsu Component USB Touch Panel\">\
\n            <merge key=\"input.x11_driver\" type=\"string\">evdev</merge>\
\n            <merge key=\"input.x11_options.ReportingMode\" type=\"string\">Raw</merge>\
\n            <merge key=\"input.x11_options.SendCoreEvents\" type=\"string\">True</merge>\
\n\
\n            <merge key=\"input.x11_options.MinX\" type=\"string\">%s</merge>\
\n            <merge key=\"input.x11_options.MinY\" type=\"string\">%s</merge>\
\n            <merge key=\"input.x11_options.MaxX\" type=\"string\">%s</merge>\
\n            <merge key=\"input.x11_options.MaxY\" type=\"string\">%s</merge>\
\n\
\n            <merge key=\"input.x11_options.emulate3buttons\" type=\"string\">false</merge>\
\n            <merge key=\"input.x11_options.MoveLimit\" type=\"string\">4</merge>\
\n            <merge key=\"input.x11_options.TapTimer\" type=\"string\">10</merge>\
\n        </match>\
\n    </device>\
\n</deviceinfo>", argv[2], argv[3], argv[4], argv[5]);
			fclose(fd);
                }
        	else {
        	  printf("%s::error::cannot create file [%s]\n",MODULE,FDI_FNAME);
        	}

		fd=fopen(MOD_FNAME,"w");
                if(fd) {
			fprintf(fd,"options fujitsu_usb_touchscreen touch_minx=%s touch_miny=%s touch_maxx=%s touch_maxy=%s\n", argv[2], argv[3], argv[4], argv[5]);
			fclose(fd);
                }
        	else {
        	  printf("%s::error::cannot create file [%s]\n",MODULE,MOD_FNAME);
        	}

		system(RMMOD_MODULE);
		system(MODPROBE_MODULE);
        }
}
