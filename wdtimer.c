/*
 * Copyright (C) 2006 Ben Collins <bcollins@ubuntu.com>
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
#include <getopt.h>

#include "ugci.h"

static void usage(int exitval) __attribute__((__noreturn__));
static void usage(int exitval)
{
	fprintf(exitval ? stderr : stdout, "Usage: wdtimer [--help] [--device id] <interval>\n");
	exit(exitval);
}

int main(int argc, char *argv[])
{
	int rd, id = 0, interval;

	while (1) {
		int c;
		static struct option long_options[] = {
			{"help",	0, NULL, 'h'},
			{"device",	1, NULL, 'd'},
			{ 0 },
		};

		c = getopt_long(argc, argv, "hd:", long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				usage(0);
				break;

			case 'd':
				id = atoi(optarg);
				break;
		}
	}

	if (argc - optind != 1)
		usage(1);

	interval = atoi(argv[optind]);

	rd = ugci_init(NULL, UGCI_EVENT_WD, 1);

	printf("Detected %d UGCI device%s\n", rd, rd == 1 ? "" : "s");

	if (!rd)
		exit(0);

	ugci_set_watchdog(id, UGCI_WD_RUNTIME, interval);

	while (ugci_poll(1000) >= 0)
		; //printf("Polling...\n");
		/* Do Nothing */

	exit(0);
}
