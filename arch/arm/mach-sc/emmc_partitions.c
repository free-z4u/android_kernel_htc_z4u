/* arch/arm/mach-sc/nand_partitions.c
 *
 * Code to extract partition information from ATAG set up by the
 * bootloader.
 *
 * Copyright (C) 2007 Google, Inc.
 * Author: Brian Swetland <swetland@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <asm/mach/flash.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <mach/board.h>

#if 0
#include <mach/msm_iomap.h>
#endif

#define ATAG_SC_PARTITION 0x4d534D70 /* SCp */

struct sc_ptbl_entry {
	char name[16];
	__u32 offset;
	__u32 size;
	__u32 flags;
};

#define SC_MAX_PARTITIONS 64

static struct mtd_partition sc_nand_partitions[SC_MAX_PARTITIONS];
static char sc_nand_names[SC_MAX_PARTITIONS * 16];

int emmc_partition_read_proc(char *page, char **start, off_t off,
			   int count, int *eof, void *data)
{
	struct mtd_partition *ptn = sc_nand_partitions;
	char *p = page;
	int i;
	uint64_t offset;
	uint64_t size;
	if (!ptn) {
		printk(KERN_INFO "emmc_partition_read_proc: !ptn\n");
		dump_stack();
	}
	p += sprintf(p, "dev:        size     erasesize name\n");
	for (i = 0; i < SC_MAX_PARTITIONS && ptn->name; i++, ptn++) {
		offset = ptn->offset;
		size = ptn->size;
		p += sprintf(p, "mmcblk0p%llu: %08llx %08x \"%s\"\n", offset, size * 512, 512, ptn->name);
	}

	return p - page;
}

static int __init parse_tag_sc_partition(const struct tag *tag)
{
	struct mtd_partition *ptn = sc_nand_partitions;
	char *name = sc_nand_names;
	struct sc_ptbl_entry *entry = (void *) &tag->u;
	unsigned count, n;
	unsigned have_kpanic = 0;

	if (!ptn) {
		printk(KERN_INFO "parse_tag_sc_partition: !ptn\n");
		dump_stack();
	}

	count = (tag->hdr.size - 2) /
		(sizeof(struct sc_ptbl_entry) / sizeof(__u32));

	if (count > SC_MAX_PARTITIONS)
		count = SC_MAX_PARTITIONS;

	for (n = 0; n < count; n++) {
		memcpy(name, entry->name, 15);
		name[15] = 0;

		if (!strcmp(name, "kpanic"))
			have_kpanic = 1;

		ptn->name = name;
		ptn->offset = entry->offset;
		ptn->size = entry->size;

		name += 16;
		entry++;
		ptn++;
	}

#ifdef CONFIG_VIRTUAL_KPANIC_PARTITION
	if (!have_kpanic) {
		int i;
		uint64_t kpanic_off = 0;

		if (count == SC_MAX_PARTITIONS) {
			printk(KERN_ERR "Cannot create virtual 'kpanic' partition\n");
			goto out;
		}

		for (i = 0; i < count; i++) {
			ptn = &sc_nand_partitions[i];
			if (!strcmp(ptn->name, CONFIG_VIRTUAL_KPANIC_SRC)) {
				ptn->size -= CONFIG_VIRTUAL_KPANIC_PSIZE;
				kpanic_off = ptn->offset + ptn->size;
				break;
			}
		}
		if (i == count) {
			printk(KERN_ERR "Partition %s not found\n",
			       CONFIG_VIRTUAL_KPANIC_SRC);
			goto out;
		}

		ptn = &sc_nand_partitions[count];
		ptn->name = "kpanic";
		ptn->offset = kpanic_off;
		ptn->size = CONFIG_VIRTUAL_KPANIC_PSIZE;

		printk(KERN_INFO "Virtual mtd partition '%s' created @%llx (%llu)\n",
		       ptn->name, ptn->offset, ptn->size);

		count++;
	}
out:
#endif /* CONFIG_VIRTUAL_KPANIC_SRC */
	sc_nand_data.nr_parts = count;
	sc_nand_data.parts = sc_nand_partitions;

	return 0;
}

__tagtable(ATAG_SC_PARTITION, parse_tag_sc_partition);
