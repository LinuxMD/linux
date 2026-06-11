// SPDX-License-Identifier: GPL-2.0-only
/*
 */

#include <linux/math.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>

#include <asm/everdrive.h>
#include <asm/everdrive-fifo.h>

#define EVERDRIVE_BLK_DRIVER_NAME	"everdrive-blk"
#define EVERDRIVE_BLK_DEVICE_NAME	"edblk"

static volatile void *everdrive_fifo = (void *) MEGADRIVE_EVERDRIVE_MAILBOX;

struct everdrive_dev {
	struct device *dev;
	struct gendisk *disk;
	struct blk_mq_tag_set tag_set;

	const char *filename;
	u64 sz;
};

static int everdrive_file_seek(sector_t whence)
{
	unsigned long flags;
	u16 status;

	local_irq_save(flags);
	everdrive_fifo_sendcmd(everdrive_fifo, &everdrive_fifo_cmd_disk_f_fptr);
	everdrive_fifo_write_u32(everdrive_fifo, whence << SECTOR_SHIFT);
	status = everdrive_fifo_read_status(everdrive_fifo);
	local_irq_restore(flags);

        return 0;
}

static int everdrive_file_read(volatile void *dst)
{
	u32 amount = SECTOR_SIZE;
	unsigned long flags;
	u16 status;
	u8 resp;

	local_irq_save(flags);
	everdrive_fifo_sendcmd(everdrive_fifo, &everdrive_fifo_cmd_disk_f_frd);
	everdrive_fifo_write_u32(everdrive_fifo, amount);
	everdrive_fifo_read_u8(everdrive_fifo, &resp);
	everdrive_fifo_read(everdrive_fifo, (volatile u8 *) dst, amount);
	status = everdrive_fifo_read_status(everdrive_fifo);
	local_irq_restore(flags);

	//printk("status 0x%x\n", (unsigned) status);

        return 0;
}


static blk_status_t everdrive_queue_rq(struct blk_mq_hw_ctx *hctx,
				       const struct blk_mq_queue_data *bd)
{
	struct request *rq = bd->rq;
	struct req_iterator iter;
	struct bio_vec bvec;
	unsigned int sectors;
	volatile void *dst;
	sector_t pos;
	int i;

	printk("%s:%d\n", __func__, __LINE__);

	switch (req_op(rq)) {
	case REQ_OP_READ:
		blk_mq_start_request(rq);

		pos = blk_rq_pos(rq);
		everdrive_file_seek(pos);

		rq_for_each_segment(bvec, rq, iter) {
			dst = page_address(bvec.bv_page) + bvec.bv_offset;
			sectors = bvec.bv_len >> SECTOR_SHIFT;

			printk("read %d starting at %d to %px\n", (int) sectors, (int) iter.iter.bi_sector, dst);

			for (i = 0; i < sectors; i++) {
				everdrive_file_read(dst);
				dst += SECTOR_SIZE;
			}
		}

		blk_mq_end_request(rq, BLK_STS_OK);
		return BLK_STS_OK;
	default:
		return BLK_STS_IOERR;
	}
}

static const struct blk_mq_ops everdrive_mq_ops = {
	.queue_rq = everdrive_queue_rq,
};

static const struct block_device_operations everdrive_fops = {
	.owner = THIS_MODULE,
};

static int everdrive_probe(struct platform_device *pdev)
{
	struct queue_limits lim = {
		.logical_block_size  = SECTOR_SIZE,
		.physical_block_size = SECTOR_SIZE,
	};
	struct device *dev = &pdev->dev;
	struct everdrive_dev *edev;
        unsigned long flags;
	u16 status;
	int ret;

	edev = devm_kzalloc(dev, sizeof(*edev), GFP_KERNEL);
	if (!edev)
		return -ENOMEM;

	edev->dev = dev;

	ret = of_property_read_string(dev->of_node, "filename", &edev->filename);
	if (ret) {
		dev_err(dev, "need 'filename'\n");
		return ret;
	}

	ret = blk_mq_alloc_sq_tag_set(&edev->tag_set, &everdrive_mq_ops, 1,
				      BLK_MQ_F_BLOCKING);
	if (ret)
		return ret;

	edev->disk = blk_mq_alloc_disk(&edev->tag_set, &lim, edev);
	if (IS_ERR(edev->disk)) {
		ret = PTR_ERR(edev->disk);
		goto err_free_tagset;
	}

	edev->disk->fops = &everdrive_fops;
	edev->disk->private_data = edev;
	snprintf(edev->disk->disk_name, DISK_NAME_LEN, EVERDRIVE_BLK_DEVICE_NAME);

	platform_set_drvdata(pdev, edev);

        local_irq_save(flags);
        everdrive_fifo_sendcmd(everdrive_fifo, &everdrive_fifo_cmd_disk_init);
	status = everdrive_fifo_read_status(everdrive_fifo);
	local_irq_restore(flags);

	//printk("status 0x%x\n", (unsigned int) status);

        local_irq_save(flags);
	everdrive_fifo_sendcmd(everdrive_fifo, &everdrive_fifo_cmd_disk_f_fopen);
	everdrive_fifo_write_u8(everdrive_fifo, EVERDRIVE_FILE_MODE_READ);
	everdrive_fifo_write_str(everdrive_fifo, edev->filename, strlen(edev->filename));
	status = everdrive_fifo_read_status(everdrive_fifo);
	local_irq_restore(flags);

	//printk("status 0x%x\n", (unsigned int) status);

        local_irq_save(flags);
	everdrive_fifo_sendcmd(everdrive_fifo, &everdrive_fifo_cmd_disk_f_avb);
        everdrive_fifo_read_u64(everdrive_fifo, &edev->sz);
	status = everdrive_fifo_read_status(everdrive_fifo);
	local_irq_restore(flags);

	//printk("status 0x%x\n", (unsigned int) status);

	set_capacity(edev->disk, edev->sz >> SECTOR_SHIFT);

	ret = add_disk(edev->disk);
	if (ret)
		goto err_put_disk;

	dev_info(dev, "Everdrive blk created for %s, size is %llu\n",
		 edev->filename, (unsigned long long) edev->sz);

	return 0;

err_put_disk:
	put_disk(edev->disk);
err_free_tagset:
	blk_mq_free_tag_set(&edev->tag_set);
	return ret;
}

static void everdrive_remove(struct platform_device *pdev)
{
	struct everdrive_dev *edev = platform_get_drvdata(pdev);

	del_gendisk(edev->disk);
	put_disk(edev->disk);
	blk_mq_free_tag_set(&edev->tag_set);
}

static const struct of_device_id everdrive_of_match[] = {
	{ .compatible = "krikzz,everdrive-blk" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, everdrive_of_match);

static struct platform_driver everdrive_driver = {
	.probe  = everdrive_probe,
	.remove = everdrive_remove,
	.driver = {
		.name           = EVERDRIVE_BLK_DRIVER_NAME,
		.of_match_table = everdrive_of_match,
	},
};
module_platform_driver(everdrive_driver);

MODULE_LICENSE("GPL v2");
