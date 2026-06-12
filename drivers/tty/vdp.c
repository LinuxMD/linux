#include <asm/io.h>
#include <linux/console.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>

#include <asm/vdp.h>

static struct tty_driver *vdp_tty_driver;
static struct tty_port vdp_port;
static int vdp_irq = -1;
static struct console vdp_console;

#define VDP_START_Y 4;
static unsigned int vdp_cur_x = 0;
static unsigned int vdp_cur_y = VDP_START_Y;

static inline void megadrive_vdp_register_set(u8 reg, u8 value)
{
        u16 _reg = reg;
        u16 tmp = (0x8000 | (_reg << 8) | value);
        iowrite16be(tmp, (void *) VDP_CTRL);
}

static inline void vdp_next_line(void)
{
	int x, y;

	vdp_cur_y++;
	if (vdp_cur_y == VDP_PLANE_AB_HEIGHT) {
		vdp_cur_y = VDP_START_Y;

		/* Crap *scrolling*  */
		for (y = 0; y < VDP_PLANE_AB_HEIGHT; y++)
			for (x = 0; x < VDP_PLANE_AB_WIDTH; x++)
				vdp_setc(' ', (y * VDP_PLANE_AB_WIDTH) + x);

		vdp_cur_x += 1;
	}

	vdp_cur_x = 0;
}

static void vdp_puts(const char *buf, unsigned int len)
{
	unsigned int i;

	for (i = 0; i < len; i++) {
		char ch = *buf++;

		if (ch == '\n') {
			vdp_next_line();
			continue;
		}

		vdp_setc(ch, (vdp_cur_y * VDP_PLANE_AB_WIDTH) + vdp_cur_x);

		vdp_cur_x += 1;
		if (vdp_cur_x == VDP_PLANE_AB_WIDTH)
			vdp_next_line();
	}
}

static void vdp_console_write(struct console *co, const char *buf, unsigned int len)
{
	vdp_puts(buf, len);
}

static struct tty_driver *vdp_console_device(struct console *co, int *index)
{
	*index = 0;
	return vdp_tty_driver;
}

static int vdp_console_setup(struct console *co, char *options)
{
	return 0;
}

static struct console vdp_console = {
        .name   = "ttyVDP",
        .write  = vdp_console_write,
        .device = vdp_console_device,
        .setup  = vdp_console_setup,
        .flags  = CON_PRINTBUFFER,
        .index  = 0,
};

static int vdp_tty_open(struct tty_struct *tty, struct file *filp)
{
        return tty_port_open(&vdp_port, tty, filp);
}

static void vdp_tty_close(struct tty_struct *tty, struct file *filp)
{
        tty_port_close(&vdp_port, tty, filp);
}

static ssize_t vdp_tty_write(struct tty_struct *tty, const unsigned char *buf, size_t count)
{
        vdp_puts(buf, count);
        return count;
}

static unsigned int vdp_tty_write_room(struct tty_struct *tty)
{
        return 30;
}

static const struct tty_operations vdp_tty_ops = {
        .open       = vdp_tty_open,
        .close      = vdp_tty_close,
        .write      = vdp_tty_write,
        .write_room = vdp_tty_write_room,
};

static irqreturn_t megadrive_vdp_irq_handler(int irq, void *dev_id)
{
        pr_debug("megadrive-vdp: VDP int\n");
        return IRQ_HANDLED;
}

static int __init megadrive_vdp_init(void)
{
        struct device_node *np;
        int ret;

        np = of_find_compatible_node(NULL, NULL, "sega,megadrive-vdp");
        if (!np)
                return 0;

        vdp_irq = of_irq_get(np, 0);
        of_node_put(np);

        if (vdp_irq < 0) {
                pr_err("megadrive-vdp: failed to get IRQ: %d\n", vdp_irq);
                return vdp_irq;
        }

        ret = request_irq(vdp_irq, megadrive_vdp_irq_handler, 0,
                          "megadrive-vdp", NULL);
        if (ret) {
                pr_err("megadrive-vdp: failed to request IRQ %d: %d\n",
                       vdp_irq, ret);
                return ret;
        }

        tty_port_init(&vdp_port);

        vdp_tty_driver = tty_alloc_driver(1, TTY_DRIVER_REAL_RAW |
                                             TTY_DRIVER_DYNAMIC_DEV);
        if (IS_ERR(vdp_tty_driver)) {
                ret = PTR_ERR(vdp_tty_driver);
                goto err_free_irq;
        }

        vdp_tty_driver->driver_name  = "ttyVDP";
        vdp_tty_driver->name         = "ttyVDP";
        vdp_tty_driver->major        = 0;
        vdp_tty_driver->minor_start  = 0;
        vdp_tty_driver->type         = TTY_DRIVER_TYPE_SERIAL;
        vdp_tty_driver->subtype      = SERIAL_TYPE_NORMAL;
        vdp_tty_driver->init_termios = tty_std_termios;
        tty_set_operations(vdp_tty_driver, &vdp_tty_ops);

        ret = tty_register_driver(vdp_tty_driver);
        if (ret) {
                tty_driver_kref_put(vdp_tty_driver);
                goto err_free_irq;
        }

        tty_port_link_device(&vdp_port, vdp_tty_driver, 0);
        register_console(&vdp_console);

        return 0;

err_free_irq:
        free_irq(vdp_irq, NULL);
        tty_port_destroy(&vdp_port);
        return ret;
}

static void __exit megadrive_vdp_exit(void)
{
        if (!vdp_tty_driver)
                return;

        unregister_console(&vdp_console);
        free_irq(vdp_irq, NULL);
        tty_unregister_driver(vdp_tty_driver);
        tty_driver_kref_put(vdp_tty_driver);
        tty_port_destroy(&vdp_port);
}

module_init(megadrive_vdp_init);
module_exit(megadrive_vdp_exit);

MODULE_LICENSE("GPL");
