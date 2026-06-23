// SPDX-License-Identifier: GPL-2.0-only

#include <linux/module.h>
#include <linux/kernel.h>

static int __init ocp_init(void)
{
    pr_info("successfully loaded ocp driver\n");
    return 0;
}

static void __exit ocp_exit(void)
{
    pr_info("successfully removed ocp driver\n");
}

module_init(ocp_init);
module_exit(ocp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nicola Ramacciotti");
MODULE_DESCRIPTION("SNP SVSM OCP Driver");
