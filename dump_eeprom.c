/*
 * Copyright (C) 2006 Ben Collins <bcollins@debian.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>

#include "ugci.h"

static void usage(int exitval) __attribute__((__noreturn__));
static void usage(int exitval)
{
	fprintf(exitval ? stderr : stdout, "Usage: dump_eeprom [--help] [--device id] [--raw]\n");
	exit(exitval);
}

int main(int argc, char *argv[])
{
	int i;
	int rd, id = 0, raw = 0;
	unsigned char eeprom[512];
	int eeprom_len;

	while (1) {
		int c;
		static struct option long_options[] = {
			{"help",	0, NULL, 'h'},
			{"device",	1, NULL, 'd'},
			{"raw",		0, NULL, 'r'},
			{ 0 },
		};

		c = getopt_long(argc, argv, "hd:r", long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				usage(0);
				break;

			case 'd':
				id = atoi(optarg);
				break;

			case 'r':
				raw = 1;
				break;
		}
	}

	if (argc != optind)
		usage(1);

	rd = ugci_init(NULL, UGCI_EVENT_WD, raw ? 0 : 1);

	if (!raw)
		printf("Detected %d UGCI device%s\n", rd, rd == 1 ? "" : "s");

	if (!rd)
		exit(0);

	ugci_get_eeprom(id, eeprom, &eeprom_len);

	/* Standard binary output */
	if (raw) {
		if (write(1, eeprom, eeprom_len) == -1)
			exit(1);
		exit(0);
	}

	/* Hex ascii display */
	for (i = 0; i < eeprom_len; i++) {
		if (!(i % 24))
			printf("\n");
		printf(" %02x", eeprom[i]);
	}
	printf("\n");

	exit(0);
}
