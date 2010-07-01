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

#ifndef _UGCI_H
#define _UGCI_H

#ifdef __cplusplus
extern "C" {
#endif

#define LIBUGCI_VERSION		0x000300

extern const char *ugci_event_to_name[];

enum ugci_event_type {
	UGCI_EVENT_UNKNOWN = 0,		/* Should never be returned */
	UGCI_EVENT_COIN,		/* Coin button */
	UGCI_EVENT_PLAY,		/* Play button */
	UGCI_EVENT_WD,			/* Enable WD refresh in poll */
};

/* Maps the above enum to descriptive strings */
extern const char *ugci_id_to_name[];

/* The coin event is sent everytime the coin button is pressed. Normally
 * it returns the value of the coin counter. The coin button does not
 * produce a button-up event by itself. However, you can use
 * ugci_set_coin_simulate() to enable emulation of the button-up event. In
 * this case, this event sends a value of 1 for press, and 0 for release.
 * You can still use ugci_get_coin_count() to get the counter value.  */
#define UGCI_EVENT_MASK_COIN	0x0001

/* The play button event sends 1 for press and 0 for release.  */
#define UGCI_EVENT_MASK_PLAY	0x0002

/* Prototype for the user supplied callback. This is called everytime an
 * event that matches the event mask is received. The ID is basically the
 * player number, base 0. The first UGCI device can send ID's 0 and 1,
 * while the second device sends ID's 2 and 3, etc. The value is dependent
 * on the event. See the above mask defines for explanations of each.  */
typedef void (*ugci_callback_t)(int id, enum ugci_event_type type, int value);

/* Initializes the internal handlers. This will probe for UGCI devices and
 * open them. Returns the number of UGCI devices successfully probed and
 * opened. So the number of available players is twice this number. The
 * callback is explained above. The mask is any ugci_event_type's you want
 * to be sent to the callback. The info is zero normally, but can be
 * non-zero if you want slightly verbose output during probe. This will
 * return less than zero for an error condition.
 *
 * NOTE: it is possible to provide an empty mask or a NULL callback, or
 * both. It makes no sense to provide one without the other though. But
 * without the two, it is useful for just using some of the direct calls
 * into the devices (ugci_{get,set}_* for example).  */
int ugci_init(ugci_callback_t cb, unsigned int mask, int info);

/* Shutdown and close the UGCI system. */
void ugci_close(void);

/* Returns libugci's compiled version */
unsigned int ugci_get_version(void);

/* Poll the opened UGCI devices. Can only be called after successful call
 * to ugci_init(). The timeout is the same usage as poll(2). That is
 * timeout in milliseconds. Less than zero means infinite. This will
 * trigger callbacks if any events are read that match the mask. Returns
 * the number of events processed. */
int ugci_poll(int timeout);

/* Get the coin count for a particular Player ID. ID is the same as would
 * be passed to the callback routine. */
int ugci_get_coin_count(int id, unsigned short *count);

/* This will simulate a release event for the coin button. Internally, the
 * coin button only returns press events, since it is really just an
 * absolute counter. Setting the wait time, will produce a release event
 * after wait_time milliseconds has passed. This relies on the fact that
 * you actually call ugci_poll() as frequently as this value. Set this to
 * 0 in order to disable, which is the default.
 *
 * It is possible that you will receive press/release events faster than
 * this value. The fact that the coin button is a counter only means that
 * if we get another button press before the pseudo release, we can force
 * a release event at that point, and then an immediate press event. This
 * ensures that no press events are lost.
 *
 * NOTE: Introduced in the 0.2 version of libugci.  */
void ugci_set_coin_simulate(int wait_time);


/* Used to access the security (serial number) buffer in the UGCI. This is
 * non-volatile. See section 4.3 of the HAPP UGCI Spec. */
#define UGCI_SEC_VALUES		14
int ugci_set_secblk(int id, unsigned char values[UGCI_SEC_VALUES]);
int ugci_get_secblk(int id, unsigned char values[UGCI_SEC_VALUES]);


/* Start a watchdog thread. The seconds is what is reported to UGCI. The
 * watchdog timer will trigger if we do not send a watchdog event for this
 * period. We actually attempt to send 2 refreshes per period. E.g. if the
 * timer is set for 60 seconds, we will refresh every 30 seconds. NOTE,
 * this requires you to poll with a timeout of a maximum of the refresh
 * rate (1/2 of the timer) for optimal usage. See section 4.1 of the HAPP
 * UGCI Spec. */
int ugci_set_watchdog(int id, int type, unsigned short seconds);

#define UGCI_WD_BOOT		1
#define UGCI_WD_RUNTIME		2


/* Keyboard boot mode. "mode" is one of the below settings. See section
 * 4.6 of the HAPP UGCI Spec. */
int ugci_kbd_mode(int id, int mode, unsigned char delay);

#define UGCI_KBD_NONE		0
#define UGCI_KBD_HID		1
#define UGCI_KBD_BOOT		2 /* Requires delay */


/* Get the contents of the eeprom. data must be able to hold atleast 504
 * bytes. The actual length of data is returned in *len. */
int ugci_get_eeprom(int id, unsigned char *data, int *len);

#ifdef __cplusplus
}
#endif

#endif  /* _UGCI_H */
