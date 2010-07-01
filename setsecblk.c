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
	fprintf(exitval ? stderr : stdout, "Usage: setsecblk [--help] [--device id] <new string>\n");
	exit(exitval);
}

static void print_secblk(int id, const char *msg,
			 unsigned char vals[UGCI_SEC_VALUES])
{
	int t, ascii = 1;

	printf("UGCI(%d): %s:", id, msg);

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

int main(int argc, char *argv[])
{
	int rd, id = 0;
	unsigned char vals[UGCI_SEC_VALUES + 1];

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

	if (strlen(argv[optind]) > UGCI_SEC_VALUES) {
		fprintf(stderr, "Security string is too long (max 14 chars)\n");
		exit (1);
	}

	memset(vals, ' ', UGCI_SEC_VALUES);
	memcpy(vals, argv[optind], strlen(argv[optind]));

	rd = ugci_init(NULL, 0, 1);

	printf("Detected %d UGCI device%s\n", rd, rd == 1 ? "" : "s");

	if (!rd)
		exit(0);

	print_secblk(id, "New Block", vals);

	if (ugci_set_secblk(id, vals)) {
		fprintf(stderr, "UGCI(%d): ", id);
		perror("ugci_set_secblk");
	} else {
		print_secblk(id, "Updated Block", vals);
	}

	exit(0);
}
