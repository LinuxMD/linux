#ifndef __EVERDRIVE_H
#define __EVERDRIVE_H

#include <asm/io.h>

#define MEGADRIVE_EVERDRIVE_MAILBOX 0xa130d0

static inline void everdrive_usb_write(u8 value)
{
	asm volatile (
		"move.w #0x2b, " STR(MEGADRIVE_EVERDRIVE_MAILBOX) "\n"
		"move.w #0xd4, " STR(MEGADRIVE_EVERDRIVE_MAILBOX) "\n"
		"move.w #0x22, " STR(MEGADRIVE_EVERDRIVE_MAILBOX) "\n"
		"move.w #0xdd, " STR(MEGADRIVE_EVERDRIVE_MAILBOX) "\n"
		"move.w #0x00, " STR(MEGADRIVE_EVERDRIVE_MAILBOX) "\n"
		"move.w #0x01, " STR(MEGADRIVE_EVERDRIVE_MAILBOX) "\n"
		"move.w %0,  " STR(MEGADRIVE_EVERDRIVE_MAILBOX) "\n"
	: : "d" (value));
}

#endif /* EVERDRIVE_H */
