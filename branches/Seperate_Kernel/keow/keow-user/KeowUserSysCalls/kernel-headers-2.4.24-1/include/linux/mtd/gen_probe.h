/*
 * (C) 2001, 2001 Red Hat, Inc.
 * GPL'd
 * $Id: gen_probe.h,v 1.1.2.1 2005-06-12 00:57:33 paul-sf Exp $
 */

#ifndef __LINUX_MTD_GEN_PROBE_H__
#define __LINUX_MTD_GEN_PROBE_H__

#include <linux/mtd/flashchip.h>
#include <linux/mtd/map.h> 
#include <linux/mtd/cfi.h>

struct chip_probe {
	char *name;
	int (*probe_chip)(struct map_info *map, __u32 base,
			  struct flchip *chips, struct cfi_private *cfi);

};

struct mtd_info *mtd_do_chip_probe(struct map_info *map, struct chip_probe *cp);

#endif /* __LINUX_MTD_GEN_PROBE_H__ */