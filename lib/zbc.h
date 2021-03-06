/*
 * This file is part of libzbc.
 *
 * Copyright (C) 2009-2014, HGST, Inc. All rights reserved.
 * Copyright (C) 2016, Western Digital. All rights reserved.
 *
 * This software is distributed under the terms of the BSD 2-clause license,
 * "as is," without technical support, and WITHOUT ANY WARRANTY, without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. You should have received a copy of the BSD 2-clause license along
 * with libzbc. If not, see  <http://opensource.org/licenses/BSD-2-Clause>.
 *
 * Authors: Damien Le Moal (damien.lemoal@wdc.com)
 *          Christoph Hellwig (hch@infradead.org)
 */

#ifndef __LIBZBC_INTERNAL_H__
#define __LIBZBC_INTERNAL_H__

#include "config.h"
#include "libzbc/zbc.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>

/**
 * Device operations.
 */
struct zbc_ops {

	/**
	 * Open device.
	 */
	int		(*zbd_open)(const char *, int, struct zbc_device **);

	/**
	 * Close device.
	 */
	int		(*zbd_close)(struct zbc_device *);

	/**
	 * Report a device zone information.
	 */
	int		(*zbd_report_zones)(struct zbc_device *, uint64_t,
					    enum zbc_reporting_options,
					    struct zbc_zone *, unsigned int *);

	/**
	 * Execute a zone operation.
	 */
	int		(*zbd_zone_op)(struct zbc_device *, uint64_t,
				       enum zbc_zone_op, unsigned int);

	/**
	 * Read from a ZBC device
	 */
	ssize_t		(*zbd_pread)(struct zbc_device *, void *,
				     size_t, uint64_t);

	/**
	 * Write to a ZBC device
	 */
	ssize_t		(*zbd_pwrite)(struct zbc_device *, const void *,
				      size_t, uint64_t);

	/**
	 * Flush to a ZBC device cache.
	 */
	int		(*zbd_flush)(struct zbc_device *);

	/**
	 * Change a device zone configuration.
	 * For emulated drives only (optional).
	 */
	int		(*zbd_set_zones)(struct zbc_device *,
					 uint64_t, uint64_t);

	/**
	 * Change a zone write pointer.
	 * For emulated drives only (optional).
	 */
	int		(*zbd_set_wp)(struct zbc_device *,
				      uint64_t, uint64_t);

};

/**
 * Device descriptor.
 */
struct zbc_device {

	/**
	 * Device file path.
	 */
	char			*zbd_filename;

	/**
	 * Device file descriptor.
	 */
	int			zbd_fd;

	/**
	 * File descriptor used for SG_IO. For block devices, this
	 * may be different from zbd_fd.
	 */
	int			zbd_sg_fd;

	/**
	 * Device operations.
	 */
	struct zbc_ops		*zbd_ops;

	/**
	 * Device info.
	 */
	struct zbc_device_info	zbd_info;

	/**
	 * Device flags set by backend drivers.
	 */
	unsigned int		zbd_flags;

	/**
	 * Command execution error info.
	 */
	struct zbc_errno	zbd_errno;

};

/**
 * Test if a device is zoned.
 */
#define zbc_dev_model(dev)	((dev)->zbd_info.zbd_model)
#define zbc_dev_is_zoned(dev)	(zbc_dev_model(dev) == ZBC_DM_HOST_MANAGED || \
				 zbc_dev_model(dev) == ZBC_DM_HOST_AWARE)

/**
 * Internal device flag: Indicates that the device is in test mode,
 * resulting in reduced argument value checks to allow invalid commands
 * to be sent to the device. This flag should not be used outside of
 * the execution of libzbc test suite (test/zbc_test.sh)
 */
#define ZBC_DEVTEST	0x80000000

/**
 * Test if a device is in test mode.
 */
#ifdef HAVE_DEVTEST
#define zbc_test_mode(dev)	((dev)->zbd_flags & ZBC_DEVTEST)
#else
#define zbc_test_mode(dev)	(false)
#endif

/**
 * Block device operations (requires kernel support).
 */
extern struct zbc_ops zbc_block_ops;

/**
 * ZAC (ATA) device operations (uses SG_IO).
 */
extern struct zbc_ops zbc_ata_ops;

/**
 * ZBC (SCSI) device operations (uses SG_IO).
 */
extern struct zbc_ops zbc_scsi_ops;

/**
 * ZBC emulation (file or block device).
 */
extern struct zbc_ops zbc_fake_ops;

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * Reporting option mask.
 */
#define zbc_ro_mask(ro)		((ro) & 0x3f)

/**
 * Logical block to sector conversion.
 */
#define zbc_dev_sect2lba(dev, sect)	zbc_sect2lba(&(dev)->zbd_info, sect)
#define zbc_dev_lba2sect(dev, lba)	zbc_lba2sect(&(dev)->zbd_info, lba)

/**
 * Check sector alignment to logical block.
 */
#define zbc_dev_sect_laligned(dev, sect)	\
	((((sect) << 9) & ((dev)->zbd_info.zbd_lblock_size - 1)) == 0)

/**
 * Check sector alignment to physical block.
 */
#define zbc_dev_sect_paligned(dev, sect)	\
	((((sect) << 9) & ((dev)->zbd_info.zbd_pblock_size - 1)) == 0)

/**
 * The block backend driver uses the SCSI backend information and
 * some zone operation.
 */
extern int zbc_scsi_get_zbd_characteristics(struct zbc_device *dev);
extern int zbc_scsi_zone_op(struct zbc_device *dev, uint64_t start_lba,
			    enum zbc_zone_op op, unsigned int flags);

/**
 * Log levels.
 */
enum {
	ZBC_LOG_NONE = 0,
	ZBC_LOG_WARNING,
	ZBC_LOG_ERROR,
	ZBC_LOG_INFO,
	ZBC_LOG_DEBUG,
	ZBC_LOG_MAX
};

/**
 * Library log level.
 */
extern int zbc_log_level;

#define zbc_print(stream,format,args...)		\
	do {						\
		fprintf((stream), format, ## args);     \
		fflush(stream);                         \
	} while (0)

/**
 * Log level controlled messages.
 */
#define zbc_print_level(l,stream,format,args...)		\
	do {							\
		if ((l) <= zbc_log_level)			\
			zbc_print((stream), "(libzbc) " format,	\
				  ## args);			\
	} while (0)

#define zbc_warning(format,args...)	\
	zbc_print_level(ZBC_LOG_WARNING, stderr, "[WARNING] " format, ##args)

#define zbc_error(format,args...)	\
	zbc_print_level(ZBC_LOG_ERROR, stderr, "[ERROR] " format, ##args)

#define zbc_info(format,args...)	\
	zbc_print_level(ZBC_LOG_INFO, stdout, format, ##args)

#define zbc_debug(format,args...)	\
	zbc_print_level(ZBC_LOG_DEBUG, stdout, format, ##args)

#define zbc_panic(format,args...)	\
	do {						\
		zbc_print_level(ZBC_LOG_ERROR,		\
				stderr,			\
				"[PANIC] " format,      \
				##args);                \
		assert(0);                              \
	} while (0)

#define zbc_assert(cond)					\
	do {							\
		if (!(cond))					\
			zbc_panic("Condition %s failed\n",	\
				  # cond);			\
	} while (0)

#endif

/* __LIBZBC_INTERNAL_H__ */
