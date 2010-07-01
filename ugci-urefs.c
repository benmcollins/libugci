/* 
 * Copyright (C) 2006 Ben Collins <bcollins@ubuntu.com>
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

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#include <linux/types.h>
#include <linux/hiddev.h>

#include "ugci.h"
#include "ugci-private.h"

struct ugci_reports {
	enum ugci_report_type type;
	struct hiddev_usage_ref uref;
	int num_values;
};

static const struct ugci_reports reports[UGCI_UREFS_MAX] = {
	{
	.type		= UGCI_UREF_P1_COIN,
	.num_values	= 1,
	.uref = {
		.report_type	= HID_REPORT_TYPE_INPUT,
		.report_id	= UGCI_PLAYER_1_REPORT,
		.field_index	= 0,
		.usage_index	= 0,
		.usage_code	= UGCI_PLAYER_UCODE_COIN,
		},
	},
        {
        .type           = UGCI_UREF_P1_PLAY,
	.num_values	= 1,
	.uref = {
                .report_type    = HID_REPORT_TYPE_INPUT,
                .report_id      = UGCI_PLAYER_1_REPORT,
                .field_index    = 1,
                .usage_index    = 0,
                .usage_code     = UGCI_PLAYER_UCODE_PLAY,
                }
        },
        {
        .type           = UGCI_UREF_P2_COIN,
        .num_values     = 1,
	.uref = {
                .report_type    = HID_REPORT_TYPE_INPUT,
                .report_id      = UGCI_PLAYER_2_REPORT,
                .field_index    = 0,
                .usage_index    = 0,
                .usage_code     = UGCI_PLAYER_UCODE_COIN,
                }
        },
        {
        .type           = UGCI_UREF_P2_PLAY,
        .num_values     = 1,
	.uref = {
                .report_type    = HID_REPORT_TYPE_INPUT,
                .report_id      = UGCI_PLAYER_2_REPORT,
                .field_index    = 1,
                .usage_index    = 0,
                .usage_code     = UGCI_PLAYER_UCODE_PLAY,
                }
        },
        {
        .type           = UGCI_UREF_SERIAL_READ_1,
	.num_values	= 7,
	.uref = {
                .report_type    = HID_REPORT_TYPE_INPUT,
                .report_id      = 9,
                .field_index    = 0,
                .usage_index    = 0,
                .usage_code     = 0xff0011,
                }
        },
	{
        .type           = UGCI_UREF_SERIAL_READ_2,
        .num_values     = 7,
	.uref = {
                .report_type    = HID_REPORT_TYPE_INPUT,
                .report_id      = 10,
                .field_index    = 0,
                .usage_index    = 0,
                .usage_code     = 0xff0021,
                }
        },
	{
        .type           = UGCI_UREF_SERIAL_WRITE_1,
        .num_values     = 7,
	.uref = {
                .report_type    = HID_REPORT_TYPE_OUTPUT,
                .report_id      = 11,
                .field_index    = 0,
                .usage_index    = 0,
                .usage_code     = 0xff0031,
                }
        },
	{
        .type           = UGCI_UREF_SERIAL_WRITE_2,
        .num_values     = 7,
	.uref = {
                .report_type    = HID_REPORT_TYPE_OUTPUT,
                .report_id      = 12,
                .field_index    = 0,
                .usage_index    = 0,
                .usage_code     = 0xff0041,
                }
        },
	{
        .type           = UGCI_UREF_WD_ACTION,
        .num_values     = 1,
	.uref = {
                .report_type    = HID_REPORT_TYPE_OUTPUT,
                .report_id      = 6,
                .field_index    = 1,
                .usage_index    = 0,
                .usage_code     = 0x910043,
                }
        },
	{
        .type           = UGCI_UREF_WD_TIMEOUT,
        .num_values     = 1,
	.uref = {
                .report_type    = HID_REPORT_TYPE_OUTPUT,
                .report_id      = 6,
                .field_index    = 0,
                .usage_index    = 0,
                .usage_code     = 0x910041,
                }
        },
	{
	.type		= UGCI_UREF_KBD_MODE,
	.num_values	= 2,
	.uref = {
		.report_type	= HID_REPORT_TYPE_OUTPUT,
		.report_id	= 16,
		.field_index	= 0,
		.usage_index	= 0,
		.usage_code	= 0xff0061,
		}
	},
	{
	.type		= UGCI_UREF_EEPROM_READ,
	.num_values	= 504,
	.uref = {
		.report_type	= HID_REPORT_TYPE_FEATURE,
		.report_id	= 82,
		.field_index	= 0,
		.usage_index	= 0,
		.usage_code	= 0x140030,
		}
	},
};

void ugci_fill_uref(enum ugci_report_type type, struct hiddev_usage_ref_multi *uref_multi)
{
	const struct ugci_reports *report = &reports[type];

	/* Just a sanity check */
	if (report->type != type) {
		fprintf(stderr, "UGCI: Inconsistent report table!!\n");
		return;
	}

	memcpy(&uref_multi->uref, &report->uref, sizeof(struct hiddev_usage_ref));

	uref_multi->num_values = report->num_values;

	return;
}

int ugci_commit_uref(struct ugci_dev_info *dev, enum ugci_report_type type)
{
	const struct ugci_reports *report = &reports[type];
	struct hiddev_report_info rinfo;

	/* Just a sanity check */
	if (report->type != type)
		return -1;

	rinfo.report_type = report->uref.report_type;
	rinfo.report_id = report->uref.report_id;
	rinfo.num_fields = 0;

	if (ioctl(dev->fd, HIDIOCSREPORT, &rinfo) < 0)
		return -1;

	return 0;
}
