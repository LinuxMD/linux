#include <asm/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>

#define VDP_DATA        0xc00000U
#define VDP_CTRL        0xc00004U

static inline void megadrive_vdp_register_set(u8 reg, u8 value)
{
        u16 _reg = reg;
        u16 tmp = (0x8000 | (_reg << 8) | value);
        writew(tmp, (void *) VDP_CTRL);
}

static irqreturn_t megadrive_vdp_irq_handler(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;

	dev_dbg(&pdev->dev, "VDP int\n");

	return IRQ_HANDLED;
}

static int megadrive_vdp_probe(struct platform_device *pdev)
{
	int irq;
	int ret;

//	printk("%s:%d\n", __func__, __LINE__);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "failed to get IRQ: %d\n", irq);
		return irq;
	}

	ret = devm_request_irq(&pdev->dev, irq, megadrive_vdp_irq_handler,
				0, dev_name(&pdev->dev), pdev);
	if (ret) {
		dev_err(&pdev->dev, "failed to request IRQ %d: %d\n", irq, ret);
		return ret;
	}

//	dev_info(&pdev->dev, "registered IRQ %d\n", irq);

	return 0;
}

static const struct of_device_id megadrive_vdp_of_match[] = {
    { .compatible = "sega,megadrive-vdp" },
    { }
};
MODULE_DEVICE_TABLE(of, megadrive_vdp_of_match);

static struct platform_driver megadrive_vdp_driver = {
	.probe  = megadrive_vdp_probe,
	.driver = {
		.name           = "megadrive-vdp",
		.of_match_table = megadrive_vdp_of_match,
	},
};
module_platform_driver(megadrive_vdp_driver);

MODULE_LICENSE("GPL");
