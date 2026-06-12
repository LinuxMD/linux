#include <asm/io.h>
#include <linux/console.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>

#include <asm/everdrive.h>

static struct tty_driver *everdrive_tty_driver;
static struct tty_port everdrive_port;
static struct console everdrive_console;

static void everdrive_puts(const char *buf, unsigned int len)
{
	unsigned int i;

	for (i = 0; i < len; i++) {
		char ch = *buf++;

		if (ch == '\n')
			everdrive_usb_write('\r');

		everdrive_usb_write(ch);
	}
}

static void everdrive_console_write(struct console *co, const char *buf, unsigned int len)
{
	everdrive_puts(buf, len);
}

static struct tty_driver *everdrive_console_device(struct console *co, int *index)
{
	*index = 0;
	return everdrive_tty_driver;
}

static int everdrive_console_setup(struct console *co, char *options)
{
	return 0;
}

static struct console everdrive_console = {
        .name   = "ttyED",
        .write  = everdrive_console_write,
        .device = everdrive_console_device,
        .setup  = everdrive_console_setup,
        .flags  = CON_PRINTBUFFER,
        .index  = 0,
};

static int everdrive_tty_open(struct tty_struct *tty, struct file *filp)
{
        return tty_port_open(&everdrive_port, tty, filp);
}

static void everdrive_tty_close(struct tty_struct *tty, struct file *filp)
{
        tty_port_close(&everdrive_port, tty, filp);
}

static ssize_t everdrive_tty_write(struct tty_struct *tty, const unsigned char *buf, size_t count)
{
        everdrive_puts(buf, count);
        return count;
}

static unsigned int everdrive_tty_write_room(struct tty_struct *tty)
{
        return 30;
}

static const struct tty_operations everdrive_tty_ops = {
        .open       = everdrive_tty_open,
        .close      = everdrive_tty_close,
        .write      = everdrive_tty_write,
        .write_room = everdrive_tty_write_room,
};

static int __init everdrive_tty_init(void)
{
        struct device_node *np;
        int ret;

        np = of_find_compatible_node(NULL, NULL, "krikzz,everdrive-serial");
        if (!np)
                return 0;

        of_node_put(np);

        tty_port_init(&everdrive_port);

        everdrive_tty_driver = tty_alloc_driver(1, TTY_DRIVER_REAL_RAW |
                                             TTY_DRIVER_DYNAMIC_DEV);
        if (IS_ERR(everdrive_tty_driver)) {
                ret = PTR_ERR(everdrive_tty_driver);
                goto err_free_irq;
        }

        everdrive_tty_driver->driver_name  = "ttyED";
        everdrive_tty_driver->name         = "ttyED";
        everdrive_tty_driver->major        = 0;
        everdrive_tty_driver->minor_start  = 0;
        everdrive_tty_driver->type         = TTY_DRIVER_TYPE_SERIAL;
        everdrive_tty_driver->subtype      = SERIAL_TYPE_NORMAL;
        everdrive_tty_driver->init_termios = tty_std_termios;
        tty_set_operations(everdrive_tty_driver, &everdrive_tty_ops);

        ret = tty_register_driver(everdrive_tty_driver);
        if (ret) {
                tty_driver_kref_put(everdrive_tty_driver);
                goto err_free_irq;
        }

        tty_port_link_device(&everdrive_port, everdrive_tty_driver, 0);
        register_console(&everdrive_console);

        return 0;

err_free_irq:
        tty_port_destroy(&everdrive_port);
        return ret;
}

static void __exit everdrive_tty_exit(void)
{
        if (!everdrive_tty_driver)
                return;

        unregister_console(&everdrive_console);
        tty_unregister_driver(everdrive_tty_driver);
        tty_driver_kref_put(everdrive_tty_driver);
        tty_port_destroy(&everdrive_port);
}

module_init(everdrive_tty_init);
module_exit(everdrive_tty_exit);

MODULE_LICENSE("GPL");
