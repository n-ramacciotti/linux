// SPDX-License-Identifier: GPL-2.0-only

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>

#define OCP_CLASS "ocp"
#define OCP_DEVICE "ocp"

#define OCP_BUF_SIZE 4096

static struct class *cl;

struct ocp_dev {
    struct device dev;
    struct cdev cdev;
    dev_t dev_num;

    struct mutex buffer_mutex;

    char *buffer;
};

static void ocp_dev_release(struct device *dev)
{
    struct ocp_dev *ocp = container_of(dev, struct ocp_dev, dev);

    mutex_destroy(&ocp->buffer_mutex);

    kfree(ocp->buffer);

    kfree(ocp);
    pr_info("released ocp driver dev");
}

static int ocp_open(struct inode *inode, struct file *file)
{
    struct ocp_dev *ocp;

    ocp = container_of(inode->i_cdev, struct ocp_dev, cdev);
    
    file->private_data = ocp;

    return 0;
}

static struct file_operations ocp_fops = {
    .owner = THIS_MODULE,
    .open = ocp_open,
};

static int __init ocp_svsm_probe(struct platform_device *pdev)
{
    int ret;
    struct ocp_dev *ocp;

    cl = class_create(OCP_CLASS);
    if (IS_ERR(cl)) return PTR_ERR(cl);

    ocp = kzalloc(sizeof(*ocp),GFP_KERNEL);
    if(!ocp) {
        class_destroy(cl);
        return -ENOMEM;
    }

    ocp->buffer = kzalloc(OCP_BUF_SIZE,GFP_KERNEL);
    if(!ocp->buffer) {
        kfree(ocp);
        class_destroy(cl);
        return -ENOMEM;
    }

    mutex_init(&ocp->buffer_mutex);

    ret = alloc_chrdev_region(&ocp->dev_num, 0, 1, OCP_DEVICE);
    if(ret < 0) {
        kfree(ocp->buffer);
        kfree(ocp);
        class_destroy(cl);
        return ret;
    }

    device_initialize(&ocp->dev);
    ocp->dev.class = cl;
    ocp->dev.devt = ocp->dev_num;
    ocp->dev.parent = &pdev->dev;
    ocp->dev.release = ocp_dev_release;

    ret = dev_set_name(&ocp->dev, OCP_DEVICE);
    if (ret) {
        unregister_chrdev_region(ocp->dev_num,1);
        put_device(&ocp->dev);
        class_destroy(cl);
        return ret;
    }
    
    cdev_init(&ocp->cdev, &ocp_fops);

    platform_set_drvdata(pdev, ocp);
        
    ret = cdev_device_add(&ocp->cdev, &ocp->dev);

    if (ret) {
        unregister_chrdev_region(ocp->dev_num, 1);
        put_device(&ocp->dev);
        class_destroy(cl);
        return ret;
    }  

    pr_info("successfully loaded ocp driver\n");
    return 0;
}

static void __exit ocp_svsm_remove(struct platform_device *pdev)
{
    struct ocp_dev *ocp = platform_get_drvdata(pdev);

    cdev_device_del(&ocp->cdev, &ocp->dev);

    unregister_chrdev_region(ocp->dev_num, 1);

    put_device(&ocp->dev);

    class_destroy(cl);
    pr_info("successfully removed ocp driver\n");
}

/*
 * tpm_svsm_remove() lives in .exit.text. For drivers registered via
 * module_platform_driver_probe() this is ok because they cannot get unbound
 * at runtime. So mark the driver struct with __refdata to prevent modpost
 * triggering a section mismatch warning.
 */
static struct platform_driver ocp_svsm_driver __refdata = {
    .remove = __exit_p(ocp_svsm_remove),
    .driver = {
        .name = "ocp-svsm",
    },
};

module_platform_driver_probe(ocp_svsm_driver, ocp_svsm_probe);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nicola Ramacciotti");
MODULE_DESCRIPTION("SNP SVSM OCP Driver");
