/* SPDX-License-Identifier: GPL-2.0 */
/*
 */

#ifndef _EVERDRIVE_FIFO_H
#define _EVERDRIVE_FIFO_H

#include <asm/io.h>

#define EVERDRIVE_FIFO_CMD_PREAMBLE	'+'
#define EVERDRIVE_FIFO_CMD_STATUS	0x10U
#define EVERDRIVE_FIFO_CMD_USB_WRITE	0x22U
#define EVERDRIVE_FIFO_CMD_FIFO_WRITE	0x23U

/* Disk commands */
#define EVERDRIVE_FIFO_CMD_DISK_INIT	0xC0U
#define EVERDRIVE_FIFO_CMD_DISK_RD	0xC1U
#define EVERDRIVE_FIFO_CMD_DISK_WR	0xC2U
#define EVERDRIVE_FIFO_CMD_F_DIR_OPN	0xC3U
#define EVERDRIVE_FIFO_CMD_F_DIR_RD	0xC4U
#define EVERDRIVE_FIFO_CMD_F_DIR_LD	0xC5U
#define EVERDRIVE_FIFO_CMD_F_DIR_SIZE	0xC6U
#define EVERDRIVE_FIFO_CMD_F_DIR_PATH	0xC7U
#define EVERDRIVE_FIFO_CMD_F_DIR_GET	0xC8U
#define EVERDRIVE_FIFO_CMD_F_FOPN	0xC9U
#define EVERDRIVE_FIFO_CMD_F_FRD	0xCAU
#define EVERDRIVE_FIFO_CMD_F_FRD_MEM	0xCBU
#define EVERDRIVE_FIFO_CMD_F_FWR	0xCCU
#define EVERDRIVE_FIFO_CMD_F_FWR_MEM	0xCDU
#define EVERDRIVE_FIFO_CMD_F_FCLOSE	0xCEU
#define EVERDRIVE_FIFO_CMD_F_FPTR	0xCFU
#define EVERDRIVE_FIFO_CMD_F_FINFO	0xD0U
#define EVERDRIVE_FIFO_CMD_F_FCRC	0xD1U
#define EVERDRIVE_FIFO_CMD_F_DIR_MK	0xD2U
#define EVERDRIVE_FIFO_CMD_F_DEL	0xD3U
#define EVERDRIVE_FIFO_CMD_F_SEEK_IDX	0xD4U
#define EVERDRIVE_FIFO_CMD_F_AVB	0xD5U
#define EVERDRIVE_FIFO_CMD_F_FCP	0xD6U
#define EVERDRIVE_FIFO_CMD_F_SEEK_PAT	0xD8U
#define EVERDRIVE_FIFO_CMD_F_DTEST	0xD9U
#define EVERDRIVE_FIFO_CMD_F_FTEST	0xDAU

struct __attribute__((packed)) everdrive_fifo_cmd {
	uint8_t preamble;
	uint8_t _preable;
	uint8_t cmd;
	uint8_t _cmd;
};

#define EVERDRIVE_FIFO_DEFINECMD(_cmd) { \
		(unsigned char) EVERDRIVE_FIFO_CMD_PREAMBLE, (unsigned char) ~EVERDRIVE_FIFO_CMD_PREAMBLE, \
	  	(unsigned char) _cmd, (unsigned char) ~_cmd \
	};

static const struct everdrive_fifo_cmd cmd_status =
		EVERDRIVE_FIFO_DEFINECMD(EVERDRIVE_FIFO_CMD_STATUS);
static const struct everdrive_fifo_cmd cmd_usbwr =
		EVERDRIVE_FIFO_DEFINECMD(EVERDRIVE_FIFO_CMD_USB_WRITE);

/* Disk */
static const struct everdrive_fifo_cmd everdrive_fifo_cmd_disk_init =
		EVERDRIVE_FIFO_DEFINECMD(EVERDRIVE_FIFO_CMD_DISK_INIT);
static const struct everdrive_fifo_cmd everdrive_fifo_cmd_disk_f_dir_ld =
		EVERDRIVE_FIFO_DEFINECMD(EVERDRIVE_FIFO_CMD_F_DIR_LD);
static const struct everdrive_fifo_cmd everdrive_fifo_cmd_disk_f_dir_size =
		EVERDRIVE_FIFO_DEFINECMD(EVERDRIVE_FIFO_CMD_F_DIR_SIZE);
static const struct everdrive_fifo_cmd everdrive_fifo_cmd_disk_f_dir_get =
		EVERDRIVE_FIFO_DEFINECMD(EVERDRIVE_FIFO_CMD_F_DIR_GET);
static const struct everdrive_fifo_cmd everdrive_fifo_cmd_disk_f_fopen =
		EVERDRIVE_FIFO_DEFINECMD(EVERDRIVE_FIFO_CMD_F_FOPN);

#define	EVERDRIVE_FILE_MODE_READ		0x01
#define	EVERDRIVE_FILE_MODE_WRITE		0x02
#define	EVERDRIVE_FILE_MODE_OPEN_EXISTING	0x00
#define	EVERDRIVE_FILE_MODE_CREATE_NEW		0x04
#define	EVERDRIVE_FILE_MODE_CREATE_ALWAYS	0x08
#define	EVERDRIVE_FILE_MODE_OPEN_ALWAYS		0x10
#define	EVERDRIVE_FILE_MODE_OPEN_APPEND		0x30

static const struct everdrive_fifo_cmd everdrive_fifo_cmd_disk_f_frd =
		EVERDRIVE_FIFO_DEFINECMD(EVERDRIVE_FIFO_CMD_F_FRD);
static const struct everdrive_fifo_cmd everdrive_fifo_cmd_disk_f_fclose =
		EVERDRIVE_FIFO_DEFINECMD(EVERDRIVE_FIFO_CMD_F_FCLOSE);
static const struct everdrive_fifo_cmd everdrive_fifo_cmd_disk_f_fptr =
		EVERDRIVE_FIFO_DEFINECMD(EVERDRIVE_FIFO_CMD_F_FPTR);
static const struct everdrive_fifo_cmd everdrive_fifo_cmd_disk_f_avb =
		EVERDRIVE_FIFO_DEFINECMD(EVERDRIVE_FIFO_CMD_F_AVB);

#define FIFO_CPU_RXF BIT(15)
#define FIFO_RXF_MSK 0x7FF

static inline void everdrive_fifo_write(volatile void *fifo, const u8 *src, unsigned int len)
{
	int i;

	for (i = 0; i < len; i++)
		iowrite16be(src[i], fifo);
}

static inline void everdrive_fifo_write_u8(volatile void *fifo, u8 value)
{
	uint8_t tmp = value;

	everdrive_fifo_write(fifo, (void *) &tmp, sizeof(tmp));
}

static inline void everdrive_fifo_write_u16(volatile void *fifo, u16 value)
{
	uint16_t tmp = value;

	everdrive_fifo_write(fifo, (void *) &tmp, sizeof(tmp));
}

static inline void everdrive_fifo_write_u32(volatile void *fifo, u32 value)
{
	uint32_t tmp = value;

	everdrive_fifo_write(fifo, (void *) &tmp, sizeof(tmp));
}

static inline void everdrive_fifo_write_str(volatile void *fifo, const unsigned char *str, size_t len)
{
	everdrive_fifo_write_u16(fifo, len);
	everdrive_fifo_write(fifo, (void *) str, len);
}

static inline void everdrive_fifo_read(volatile void *fifo, volatile u8 *dst, unsigned int len)
{
	int i;

	for (i = 0; i < len; i++) {
		while (!(ioread16be(fifo + 2) & FIFO_RXF_MSK)) {
			/* spin until there is something to read */
		}
		dst[i] = ioread16be(fifo);
	}
}

static inline int everdrive_fifo_read_u8(volatile void *fifo, uint8_t *value)
{
	uint8_t tmp;

	everdrive_fifo_read(fifo, (void *) &tmp, sizeof(tmp));

	*value = tmp;

	return 0;
}

static inline int everdrive_fifo_read_u16(volatile void *fifo, uint16_t *value)
{
	uint16_t tmp;

	everdrive_fifo_read(fifo, (void *) &tmp, sizeof(tmp));

	*value = tmp;

	return 0;
}

static inline int everdrive_fifo_read_u32(volatile void *fifo, uint32_t *value)
{
	uint32_t tmp;

	everdrive_fifo_read(fifo, (void *) &tmp, sizeof(tmp));

	*value = tmp;

	return 0;
}

static inline int everdrive_fifo_read_u64(volatile void *fifo, uint64_t *value)
{
	uint64_t tmp;

	everdrive_fifo_read(fifo, (void *) &tmp, sizeof(tmp));

	*value = tmp;

	return 0;
}

static inline int everdrive_fifo_read_str(volatile void *fifo, unsigned char *str, size_t limit)
{
	uint16_t sz;

	everdrive_fifo_read_u16(fifo, &sz);

	if (limit < sz)
		return -ENOMEM;

	everdrive_fifo_read(fifo, (void *) str, sz);

	return sz;
}

static inline u16 everdrive_fifo_read_status(volatile void *fifo)
{
	u16 status;

        everdrive_fifo_write(fifo, (u8*) &cmd_status, sizeof(cmd_status));
        everdrive_fifo_read_u16(fifo, &status);

	return status;
}

static inline void everdrive_fifo_sendcmd(volatile void *fifo, const struct everdrive_fifo_cmd *cmd)
{
	everdrive_fifo_write(fifo, (u8*) cmd, sizeof(*cmd));
}

#endif
