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

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include <linux/types.h>
#include <linux/hiddev.h>

#include "ugci.h"
#include "ugci-private.h"


const char *ugci_event_to_name[] = { "unknown", "coin", "play" };

static struct ugci_dev_info devs[UGCI_MAX_DEVS];

static const char *dev_names[UGCI_MAX_DEVS] = { "1/2", "3/4", "5/6", "7/8" };

static const char *dev_path_fmts[] = {
	"/dev/hiddev%d",
	"/dev/usb/hiddev%d",
	"/dev/usb/hid/hiddev%d",
	NULL,
};

static int hiddev_ok = 1;
static int hiddev_ver_shown;
static int initialized;
static int info_out;
static int ugci_event_mask;
static int sim_coin_wait;

static ugci_callback_t ugci_cb;

static struct ugci_dev_info *get_dev_info(int id)
{
	if (id >= UGCI_MAX_DEVS)
		return NULL;

	if (devs[id].fd < 0)
		return NULL;

	return &devs[id];
}

/* Checks a device for UGCI signatures */
static int is_happ_ugci(int fd)
{
	int i = 0, ret;
	struct hiddev_devinfo dinfo;
	unsigned int version;

	while ((ret = ioctl(fd, HIDIOCAPPLICATION, i)) > 0 && ret != UGCI_PLAYER_APP)
		i++;

	if (ret != UGCI_PLAYER_APP)
		return 0;

	ioctl(fd, HIDIOCGDEVINFO, &dinfo);
	if (dinfo.vendor != USB_VENDOR_ID_HAPP)
		return 0;

	ioctl(fd, HIDIOCGVERSION, &version);
	if (version < MIN_HID_VERSION) {
		fprintf(stderr, "  HID Version is %d.%d.%d. Need a "
			"minimum of %d.%d.%d.\n",
			version >> 16, (version >> 8) & 0xff, version & 0xff,
			MIN_HID_VERSION >> 16, (MIN_HID_VERSION >> 8) & 0xff,
			MIN_HID_VERSION & 0xff);
		hiddev_ok = 0;
		return 0;
	}

	if (info_out && ! hiddev_ver_shown++)
		printf("  HID device driver version is %d.%d.%d\n",
			version >> 16, (version >> 8) & 0xff, version & 0xff);

	if (info_out)
		printf("  HID Bus(%d) DevNum(%d) IFNum(%d)\n",
			dinfo.busnum, dinfo.devnum, dinfo.ifnum);

	return 1;
}


int ugci_init (ugci_callback_t cb, unsigned int mask, int info)
{
	int i, id;

	if (info) {
		info_out = 1;
		printf("UGCI: Version %d.%d.%d initializing...\n",
			LIBUGCI_VERSION >> 16, (LIBUGCI_VERSION >> 8) & 0xff,
			LIBUGCI_VERSION & 0xff);
	}

	if (info_out) {
		if (! cb && mask)
			printf("UGCI: WARNING: Event mask supplied, yet no callback registered.\n");
		else if (cb && ! mask)
			printf("UGCI: WARNING: Callback registered, yet no event mask supplied.\n");
	}

	for (id = 0; id < UGCI_MAX_DEVS; id++)
		devs[id].fd = -1;

	for (i = id = 0; i < 8 && id < UGCI_MAX_DEVS && hiddev_ok; i++) {
		struct hiddev_usage_ref_multi uref_multi;
		int t, fd = -1;
		char devname[32];
		char name[256];

		for (t = 0; dev_path_fmts[t]; t++) {
			sprintf(devname, dev_path_fmts[t], i);
			if ((fd = open(devname, O_RDONLY)) >= 0)
				break;
		}

		if (fd < 0)
			continue;

		if (! is_happ_ugci(fd)) {
			close(fd);
			continue;
		}

		/* Ok, so we know we have a legit coin/start device. Let's
		 * save it for later use. */
		memset(&devs[id], 0, sizeof(devs[id]));
		devs[id].fd = fd;
		devs[id].id = id;

		ioctl(fd, HIDIOCGNAME(sizeof(name)), name);

		/* Enable events */
		t = HIDDEV_FLAG_UREF | HIDDEV_FLAG_REPORT;
		ioctl(fd, HIDIOCSFLAG, &t);

		/* Make sure the reports for the hiddev are initialized */
		ioctl(fd, HIDIOCINITREPORT, 0);

		if (info_out)
			printf("    Players %s: %s: %s\n", dev_names[id], devname, name);

		/* Now, let's get the eeprom. */
		ugci_fill_uref(UGCI_UREF_EEPROM_READ, &uref_multi);
		if (ioctl(fd, HIDIOCGUSAGES, &uref_multi) < 0)
			fprintf(stderr, "UGCI(%d): Error reading eeprom\n", id);
		else {
			for (t = 0; t < uref_multi.num_values; t++)
				devs[id].eeprom[t] = (unsigned char)uref_multi.values[t];
			if (info_out) {
				char *leader = "               :";

				printf("%s Key mapping %sabled\n", leader,
					devs[id].eeprom[0] & 0x01 ? "en" : "dis");

				printf("%s %d byte EEPROM\n", leader,
					devs[id].eeprom[0] & 0x02 ? 512 : 128);

				if (devs[id].eeprom[0] & 0x04)
					printf("%s Surface mount board (rev C)\n", leader);
				else
					printf("%s Thru hole board (rev C)\n", leader);
			}

			devs[id].eeprom_valid = 1;
			devs[id].eeprom_len = (devs[id].eeprom[0] & 0x02) ? 504 : 120;
		}
	
		id++;
	}

	if (! hiddev_ok)
		return -1;

	ugci_cb = cb;
	ugci_event_mask = mask;
	initialized = 1;

	return id;
}


static void disable_dev(int id)
{
	int i, valid;
	struct ugci_dev_info *dev = get_dev_info(id);

	if (!dev)
		return;

	close(dev->fd);
	dev->fd = -1;

	for (i = valid = 0; i < UGCI_MAX_DEVS; i++)
		if (devs[i].fd >= 0)
			valid++;

	/* If we have no more valid devs, we are basically shutdown */
	if (!valid)
		initialized = 0;
}


void ugci_close(void)
{
	int i;

	if (! initialized)
		return;

	initialized = 0;

	if (info_out) {
		info_out = 0;
		printf("UGCI: Shutting down\n");
	}

	for (i = 0; i < UGCI_MAX_DEVS; i++)
		disable_dev(i);
}


int ugci_get_coin_count(int id, unsigned short *count)
{
	struct hiddev_usage_ref_multi uref_multi;
	struct ugci_dev_info *dev = get_dev_info(id / 2);
	enum ugci_report_type type = (id & 1) ? UGCI_UREF_P2_COIN :
		UGCI_UREF_P1_COIN;

	if (!dev)
		return -1;

	ugci_fill_uref(type, &uref_multi);

	if (ioctl(dev->fd, HIDIOCGUSAGES, &uref_multi))
		return -1;

	/* XXX Not endian safe */
	*count = uref_multi.values[0];

	return 0;
}

/* The security buffer (AKA serial buffer) is a 14 byte non-volatile area.
 * It must be read in 2 7-byte reads. */
int ugci_get_secblk(int id, unsigned char values[UGCI_SEC_VALUES])
{
	struct hiddev_usage_ref_multi uref_multi;
	struct ugci_dev_info *dev = get_dev_info(id);
	int i;

	if (!dev)
		return -1;

	ugci_fill_uref(UGCI_UREF_SERIAL_READ_1, &uref_multi);

	if (ioctl(dev->fd, HIDIOCGUSAGES, &uref_multi) < 0)
		return -1;

	for (i = 0; i < uref_multi.num_values; i++)
		values[i] = ((unsigned int)uref_multi.values[i]) & 0xff;

	ugci_fill_uref(UGCI_UREF_SERIAL_READ_2, &uref_multi);

	if (ioctl(dev->fd, HIDIOCGUSAGES, &uref_multi) < 0)
		return -1;

	for (i = 0; i < 7; i++)
		values[i + 7] = ((unsigned int)uref_multi.values[i]) & 0xff;

	return 0;
}

int ugci_set_secblk(int id, unsigned char values[UGCI_SEC_VALUES])
{
	struct hiddev_usage_ref_multi uref_multi;
	struct ugci_dev_info *dev = get_dev_info(id);
	int i;

	if (!dev)
		return -1;

	/* Handle first 7 bytes */
	ugci_fill_uref(UGCI_UREF_SERIAL_WRITE_1, &uref_multi);

	for (i = 0; i < uref_multi.num_values; i++)
		uref_multi.values[i] = (unsigned int)values[i];

	if (ioctl(dev->fd, HIDIOCSUSAGES, &uref_multi) < 0)
		return -1;

	if (ugci_commit_uref(dev, UGCI_UREF_SERIAL_WRITE_1))
		return -1;


	/* Now the second half */
	ugci_fill_uref(UGCI_UREF_SERIAL_WRITE_2, &uref_multi);

	for (i = 0; i < uref_multi.num_values; i++)
		uref_multi.values[i] = (unsigned int)values[i + 7];

	if (ioctl(dev->fd, HIDIOCSUSAGES, &uref_multi) < 0)
		return -1;

	if (ugci_commit_uref(dev, UGCI_UREF_SERIAL_WRITE_2))
		return -1;

	/* Reread so caller can easily verify */
	return ugci_get_secblk(id, values);
}

int ugci_set_watchdog(int id, int type, unsigned short seconds)
{
	struct hiddev_usage_ref_multi uref_multi;
	struct ugci_dev_info *dev = get_dev_info(id);

	if (!dev)
		return -1;

	if (type != UGCI_WD_BOOT && type != UGCI_WD_RUNTIME)
		return -1;

	if (info_out) {
		if (seconds)
			printf("UGCI(%d): Setting watchdog %s timer for %u second interval\n",
			       id, type == UGCI_WD_BOOT ? "boot" : "runtime", seconds);
		else
			printf("UGCI(%d): Disabling watchdog %s timer\n", id,
			       type == UGCI_WD_BOOT ? "boot" : "runtime");
	}

	ugci_fill_uref(UGCI_UREF_WD_ACTION, &uref_multi);
	uref_multi.values[0] = type;
	if (ioctl(dev->fd, HIDIOCSUSAGES, &uref_multi) < 0)
		return -1;		

	ugci_fill_uref(UGCI_UREF_WD_TIMEOUT, &uref_multi);
	uref_multi.values[0] = (unsigned int)seconds;
	if (ioctl(dev->fd, HIDIOCSUSAGES, &uref_multi) < 0)
		return -1;

	/* Write the changes to the device. Both of these are on the same
	 * report_id, and must be committed together. */
	if (ugci_commit_uref(dev, UGCI_UREF_WD_ACTION))
		return -1;

	/* Set our interval */
	if (type == UGCI_WD_RUNTIME) {
		dev->wd_interval = seconds;
		dev->last_wd = time(NULL);
	}

	return 0;
}


int ugci_get_eeprom(int id, unsigned char *data, int *len)
{
	struct ugci_dev_info *dev = get_dev_info(id);

	if (!dev || data == NULL || !dev->eeprom_valid)
		return -1;

	memcpy(data, dev->eeprom, dev->eeprom_len);
	*len = dev->eeprom_len;

	return 0;
}

int ugci_kbd_mode(int id, int mode, unsigned char delay)
{
	struct hiddev_usage_ref_multi uref_multi;
	struct ugci_dev_info *dev = get_dev_info(id);

	if (!dev)
		return -1;

	if (mode < UGCI_KBD_NONE || mode > UGCI_KBD_BOOT)
		return -1;

	if (info_out) {
		printf("UGCI(%d): Setting keyboard mode to %s (%u delay)\n", id,
		       mode == UGCI_KBD_NONE ? "NONE" : mode == UGCI_KBD_HID ? "HID" : "BOOT",
		       delay);
        }

	ugci_fill_uref(UGCI_UREF_KBD_MODE, &uref_multi);
	uref_multi.values[0] = mode;
	uref_multi.values[0] = delay;
        if (ioctl(dev->fd, HIDIOCSUSAGES, &uref_multi) < 0)
                return -1;

	if (ugci_commit_uref(dev, UGCI_UREF_KBD_MODE))
		return -1;

	return 0;
}

unsigned int ugci_get_version(void)
{
	return LIBUGCI_VERSION;
}


void ugci_set_coin_simulate(int wait_time)
{
	sim_coin_wait = wait_time;
}


static void ugci_send_event(int id, enum ugci_event_type type, int value)
{
	DPRINT("UGCI(%d): Sending Player %d %s button: %d\n",
	       id / 2, id + 1, ugci_event_to_name[type], value);

	if (ugci_cb)
		ugci_cb(id, type, value);
}


static inline unsigned long long ugci_tv_to_msec(struct timeval *tv)
{
	return ((unsigned long long)tv->tv_sec * 1000) + ((unsigned long long)tv->tv_usec / 1000);
}


int ugci_poll(int timeout)
{
	int i, fds, events, rd;
	struct pollfd pfd[UGCI_MAX_DEVS];
	struct ugci_dev_info *dev;

	if (! initialized)
		return -1;

	for (i = fds = 0; i < UGCI_MAX_DEVS; i++) {
		if (!(dev = get_dev_info(i)))
			continue;

		pfd[fds].events = POLLIN;
		pfd[fds].fd = dev->fd;
		pfd[fds].revents = 0;

		fds++;
	}

	if (! fds)
		return 0;

	rd = poll(pfd, fds, timeout);

	for (i = events = 0; i < UGCI_MAX_DEVS; i++) {
		struct hiddev_usage_ref ev[64];
		int t, p;

		if (!(dev = get_dev_info(i)))
			continue;

		for (p = 0; p < fds; p++)
			if (pfd[p].fd == dev->fd)
				break;

		if (pfd[p].revents & (POLLNVAL | POLLERR)) {
			fprintf(stderr, "UGCI(%d): Error polling, disabling\n", i);
			disable_dev(i);
			continue;
		}

		if (pfd[p].revents & POLLIN) {
			rd = read(dev->fd, ev, sizeof(ev));

			if (rd < (int) sizeof(ev[0])) {
				fprintf(stderr, "UGCI(%d): Error reading, disabling\n", i);
				perror("read");
				disable_dev(i);
				continue;
			}

			for (t = 0; t < (rd / sizeof(ev[0])); t++) {
				enum ugci_event_type type = 0;
				int value;
				int id = ev[t].report_id == UGCI_PLAYER_1_REPORT ? 0 : 1;
				int player = id + (i * 2);

				switch (ev[t].usage_code) {
					case UGCI_PLAYER_UCODE_PLAY:
						if (! (ugci_event_mask & UGCI_EVENT_MASK_PLAY))
							continue;

						type = UGCI_EVENT_PLAY;
						value = ev[t].value;
						break;
					case UGCI_PLAYER_UCODE_COIN:
						if (! (ugci_event_mask & UGCI_EVENT_MASK_COIN))
							continue;

						type = UGCI_EVENT_COIN;
						if (sim_coin_wait) {
							/* See if we need to force a premature release */
							if (dev->coin_pressed[id]) {
								events++;
								ugci_send_event(player, type, 0);
							} else
								dev->coin_pressed[id] = 1;

							gettimeofday(&dev->last_tv[id], NULL);
							value = 1;
						} else {
							value = ev[t].value;
						}
						break;
						
					default:
						continue;
				}

				events++;
				ugci_send_event(player, type, value);
			}
		}

		/* Now check for psuedo coin-release events */
		if (sim_coin_wait) {
			for (t = 0; t < 2; t++) {
				int player = t + (i * 2);
				struct timeval tv;

				if (! dev->coin_pressed[t])
					continue;

				gettimeofday(&tv, NULL);

				if (ugci_tv_to_msec(&dev->last_tv[t]) + sim_coin_wait <
				    ugci_tv_to_msec(&tv)) {
					events++;
					ugci_send_event(player, UGCI_EVENT_COIN, 0);
					dev->coin_pressed[t] = 0;
				}
			}
		}

		/* Now check watchdog timer */
		if (dev->wd_interval) {
			int checktime = (dev->wd_interval / 2) ?: 1;

			if (dev->last_wd + checktime <= time(NULL)) {
				int old_info = info_out;
				info_out = 0;
				ugci_set_watchdog(dev->id, UGCI_WD_RUNTIME, dev->wd_interval);
				info_out = old_info;
			}
		}
	}

	return events;
}
