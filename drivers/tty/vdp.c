#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

static int megadrive_vdp_probe(struct platform_device *pdev)
{
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
