/* 
 * Copyright (C) 2003,2006 Ben Collins <bcollins@debian.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef UGCI_PRIVATE_H
#define UGCI_PRIVATE_H 1

#ifdef DEBUG
#define DPRINT(fmt, args...) fprintf(stderr, fmt, ## args)
#else
#define DPRINT(fmt, args...) do{}while(0)
#endif

/* Enough for 8 players should be a good default */
#define UGCI_MAX_DEVS			4

/* We need the support of urefs and collections */
#define MIN_HID_VERSION 0x010004

#if HID_VERSION < MIN_HID_VERSION
#error Your HIDDev header is too old.
#endif

struct ugci_dev_info {
	int id;

	int fd;

	/* Simul */
	int coin_pressed[2];
	struct timeval last_tv[2];

	/* Watchdog */
	unsigned int wd_interval;
	time_t last_wd;

	/* EEPROM */
	unsigned char eeprom[504];
	int eeprom_valid;
	int eeprom_len;
};


enum ugci_report_type {
	UGCI_UREF_P1_COIN = 0,
	UGCI_UREF_P1_PLAY,
	UGCI_UREF_P2_COIN,
	UGCI_UREF_P2_PLAY,
	UGCI_UREF_SERIAL_READ_1,
	UGCI_UREF_SERIAL_READ_2,
	UGCI_UREF_SERIAL_WRITE_1,
	UGCI_UREF_SERIAL_WRITE_2,
	UGCI_UREF_WD_ACTION,
	UGCI_UREF_WD_TIMEOUT,
	UGCI_UREF_KBD_MODE,
	UGCI_UREF_EEPROM_READ,
	UGCI_UREFS_MAX /* Final entry */
};

void ugci_fill_uref(enum ugci_report_type type, struct hiddev_usage_ref_multi *uref_multi);
int ugci_commit_uref(struct ugci_dev_info *dev, enum ugci_report_type type);

#define USB_VENDOR_ID_HAPP		0x078b
#define USB_DEVICE_ID_UGCI_DRIVING	0x0010
#define USB_DEVICE_ID_UGCI_FLYING	0x0020
#define USB_DEVICE_ID_UGCI_FIGHTING	0x0030

#define UGCI_PLAYER_APP			0x910002
#define UGCI_PLAYER_1_REPORT		3
#define UGCI_PLAYER_2_REPORT		4
#define UGCI_PLAYER_UCODE_COIN		0x910035
#define UGCI_PLAYER_UCODE_PLAY		0x910036


#define UGCI_JOYSTICK_1_REPORT		1
#define UGCI_JOYSTICK_2_REPORT		2
#define   UGCI_JOYSTICK_APP		0x010004
#define     UGCI_JOYSTICK_FIELD_AXIS	0
#define       UGCI_JOYSTICK_USAGE_X	0
#define       UGCI_JOYSTICK_UCODE_X	0x010030
#define       UGCI_JOYSTICK_USAGE_Y	1
#define       UGCI_JOYSTICK_UCODE_Y	0x010031
#define     UGCI_JOYSTICK_FIELD_BUT	1
#define       UGCI_JOYSTICK_USAGE_BUT_1	0
#define       UGCI_JOYSTICK_UCODE_BUT_1	0x090001
#define       UGCI_JOYSTICK_USAGE_BUT_2	1
#define       UGCI_JOYSTICK_UCODE_BUT_2	0x090002
#define       UGCI_JOYSTICK_USAGE_BUT_3	2
#define       UGCI_JOYSTICK_UCODE_BUT_3	0x090003
#define       UGCI_JOYSTICK_USAGE_BUT_4	3
#define       UGCI_JOYSTICK_UCODE_BUT_4	0x090004
#define       UGCI_JOYSTICK_USAGE_BUT_5	4
#define       UGCI_JOYSTICK_UCODE_BUT_5	0x090005
#define       UGCI_JOYSTICK_USAGE_BUT_6	5
#define       UGCI_JOYSTICK_UCODE_BUT_6	0x090006
#define       UGCI_JOYSTICK_USAGE_BUT_7	6
#define       UGCI_JOYSTICK_UCODE_BUT_7	0x090007

#endif /* UGCI_PRIVATE_H */
