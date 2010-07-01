/*
 * Copyright (C) 2003,2006 Ben Collins <bcollins@ubuntu.com>
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

#include "ugci.h"

#define SIM_WAIT_MS	100

static void mycallback(int id, enum ugci_event_type type, int value)
{
	printf("UGCI: Player %d: %s button: %d\n",
		id + 1, ugci_event_to_name[type], value);
}

static void usage(int exitval) __attribute__((__noreturn__));
static void usage(int exitval)
{
	fprintf(exitval ? stderr : stdout, "Usage: testugci [--help] [--simul]\n");
	exit(exitval);
}

int main(int argc, char *argv[])
{
	int i, rd, simul = 0;
	unsigned char vals[UGCI_SEC_VALUES + 1];

	if (argc > 1) {
		if (argc > 2 || strcmp(argv[1], "--help") == 0)
			usage(argc > 2 ? 1 : 0);
		else if (strcmp(argv[1], "--simul") == 0)
			simul = 1;
		else
			usage(1);
	}
	
	rd = ugci_init(mycallback, UGCI_EVENT_MASK_COIN |
		       UGCI_EVENT_MASK_PLAY, 1);

	printf("Detected %d UGCI device%s\n", rd, rd == 1 ? "" : "s");

	if (!rd)
		exit(0);

	for (i = 0; i < (rd * 2); i++) {
		unsigned short count;
		printf("  Player %d: coin count ", i + 1);
		if (ugci_get_coin_count(i, &count))
			printf("(error reading)\n");
		else
			printf("%u\n", count);
	}

	for (i = 0; i < rd; i++) {
		if (ugci_get_secblk(i, vals)) {
			fprintf(stderr, "UGCI(%d): ", i);
			perror("ugci_get_secblk");
		} else {
			int t, ascii = 1;
			printf("UGCI(%d): Security Block:", i);
			for (t = 0; t < UGCI_SEC_VALUES; t++) {
				printf("%c%02x", t ? ':' : ' ', vals[t]);
				if (vals[t] && !isprint(vals[t]))
					ascii = 0;
			}
			printf("\n");
			if (ascii) {
				vals[UGCI_SEC_VALUES] = '\0';
				printf("  '%s'\n", vals);
			}
		}
	}

	if (simul) {
		printf("\nEnabling coin release simulation at %dms\n", SIM_WAIT_MS);
		ugci_set_coin_simulate(SIM_WAIT_MS);
	}

	printf("\nPolling...\n");

	while (ugci_poll(100) >= 0)
		/* Do nothing */;

	exit(0);
}
