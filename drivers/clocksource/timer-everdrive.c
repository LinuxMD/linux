// SPDX-License-Identifier: GPL-2.0

#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched_clock.h>
#include <linux/of_irq.h>
#include "timer-of.h"

static u64 everdrive_timer_read_cnt(void)
{
	void *reg = (void *) 0xa130d6;

	return ioread16be(reg);
}

static irqreturn_t everdrive_timer_irq(int irq, void *dev_id)
{
	unsigned long flags;

	local_irq_save(flags);
	legacy_timer_tick(1);
	local_irq_restore(flags);

	return IRQ_HANDLED;
}

#define VDP_CTRL                        0xc00004U

#define VDP_REG_MODE1                   0x00U
#define VDP_MODE1_M4                    BIT(2)
#define VDP_MODE1_IE0                   BIT(4)

#define VDP_REG_MODE2                   0x01U
#define VDP_MODE2_M5                    BIT(2)
#define VDP_MODE2_IE1                   BIT(5)
#define VDP_MODE2_DE                    BIT(6)


static inline void vdp_register_set(u8 reg, u8 value)
{
        u16 _reg = reg;
        u16 tmp = (0x8000 | (_reg << 8) | value);
        iowrite16be(tmp, (void *) VDP_CTRL);
}

static const char *irq_name = "everdrive-timer";

static int __init everdrive_timer_init_of(struct device_node *np)
{
	unsigned long rate = 1000;
	int ret;
	int irq;

	sched_clock_register(everdrive_timer_read_cnt, 16, rate);

	irq = of_irq_get(np, 0);
	if (irq < 0)
		return irq;

	/* We need to pass *something* for dev_id .. */
	ret = request_irq(irq, everdrive_timer_irq, IRQF_TIMER | IRQF_SHARED, irq_name, irq_name);
	if (ret)
		return ret;

	/* HACK O RAMA! */
//	vdp_register_set(VDP_REG_MODE1, VDP_MODE1_M4 | VDP_MODE1_IE0);
        vdp_register_set(VDP_REG_MODE2, VDP_MODE2_M5 | VDP_MODE2_DE | VDP_MODE2_IE1);

	return 0;
}

TIMER_OF_DECLARE(everdrive, "krikzz,everdrive-timer", everdrive_timer_init_of);
