// SPDX-License-Identifier: GPL-2.0-only

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/stat.h>
#include <linux/moduleparam.h>
#include <linux/types.h>

#include <asm/sev.h>

static int which_test = 0;
module_param(which_test, int, S_IRUGO);
static int which_call = 0;
module_param(which_call, int, S_IRUGO);

static int __init test_ocp_write(void)
{
    pr_info("Testing OCP write source\n");
    pr_err("This test is not implemented yet\n");
    return -1;
}

static int __init test_ocp_read(void)
{
    u8 *buffer;
    int ret;
    u64 source_idx;
	u64 bytes_to_read;
	u64 bytes_read= 0;
    u64 offset;
    bool print_buffer = true;
    pr_info("Testing OCP read source\n");

    buffer  = kzalloc(PAGE_SIZE, GFP_KERNEL);
    if (!buffer) {
		pr_err("buffer allocation failed\n");
		return -ENOMEM;
	}

    switch (which_test)
    {
    case 0:
        // start from offset 0 and read 5 bytes
        source_idx = 0;
        bytes_to_read = 5;
        offset = 0;
        break;
    case 1:
        // start from offset 0 and read 400 bytes
        // the current source is less than 400 bytes
        source_idx = 0;
        bytes_to_read = 400;
        offset = 0;
        break;
    case 2:
        // invalid source index, this should fail
        source_idx = 100;
        bytes_to_read = 16;
        offset = 0;
        break;
    default:
        pr_err("Invalid parameter %d\n", which_test);
        kfree(buffer);
        return -1;
    }

    ret = snp_svsm_ocp_read_source(
        buffer,
        source_idx,
        bytes_to_read,
        offset,
        &bytes_read
    );

    switch (which_test)
    {
    case 0:
        // This should work, returning 5 bytes
        if (ret != 0 && bytes_read != 5) {
            pr_err("This should have returned 5 bytes\nError: %d, Bytes: %lld", ret, bytes_read);
            kfree(buffer);
            return -1;
        }
        print_buffer = true;
        break;
    case 1:
        // This should work, returning less than 400 bytes
        if (ret != 0 && bytes_read >= 400) {
            pr_err("This should have returned less than 400 bytes\nError: %d, Bytes: %lld", ret, bytes_read);
            kfree(buffer);
            return -1;
        }
        pr_err("This returned %lld bytes, expected less than 400 bytes", bytes_read);
        print_buffer = true;
        break;
    case 2:
        // This should fail as the source index is invalid
        if (ret >= 0) {
            pr_err("This should have returned with error %d\n", ret );
            kfree(buffer);
            return -1;
        }
        break;
    default:
        pr_err("Invalid parameter %d\n", which_test);
        kfree(buffer);
        return -1;
    }

    if (print_buffer) {

        print_hex_dump(
            KERN_ERR, "BUFFER: ", DUMP_PREFIX_OFFSET,
            16, 1, buffer, bytes_read, true
        );
    }

    kfree(buffer);
    return 0;
}

static int __init test_ocp_list(void)
{
    u8 *buffer;
    int ret;
    u64 first_entry;
	u64 num_entries;
	u64 entries_returned= 0;
    bool print_entries = true;

    buffer  = kzalloc(PAGE_SIZE, GFP_KERNEL);

	if (!buffer) {
		pr_err("buffer allocation failed\n");
		return -ENOMEM;
	}

    switch (which_test)
    {
    case 0:
        // This should work, returning a single entry
        first_entry = 0;
	    num_entries = 1;
        break;
    case 1:
        // This should not fail for the current svsm implementatino
	    // Even if first_entry is 0  and is the only entry
	    // this should return 0
        first_entry = 1;
	    num_entries = 1;

        break;
    case 2:
        // This should work with returned entries = 1 even with 2 entries asked
        first_entry = 0;
	    num_entries = 2;

        break;
    case 3:
        // this should fail as num entries is zero
	    // invalid parameter from svsm
        first_entry = 0;
	    num_entries = 0;

        break;
    default:
        pr_err("Invalid parameter %d\n", which_test);
        kfree(buffer);
        return -1;
    }

    ret = snp_svsm_ocp_list_sources(
        buffer,
        first_entry,
        num_entries,
        &entries_returned
    );

    switch (which_test)
    {
    case 0:
        if (ret != 0 && entries_returned != 1) {
            pr_err("This should have returned one entry\nError: %d, Entries: %lld", ret, entries_returned);
            kfree(buffer);
            return -1;
        }
        break;
    case 1:
        if (ret != 0 && entries_returned != 0) {
            pr_err("This should have returned zero entry\nError: %d, Entries: %lld", ret, entries_returned);
            kfree(buffer);
            return -1;
        }
        print_entries = false;
        break;
    case 2:
        if (ret != 0 && entries_returned != 1) {
            pr_err("This should have returned one entry\nError: %d, Entries: %lld", ret, entries_returned);
            kfree(buffer);
            return -1;
        }
        break;
    case 3:
        if (ret >= 0) {
            pr_err("This should have returned with error %d\n", ret );
            kfree(buffer);
            return -1;
        }
        pr_err("This returned with error. Expected behaviour");
        print_entries = false;
        break;
    default:
        pr_err("Invalid parameter %d\n", which_test);
        kfree(buffer);
        return -1;
    }

    if (print_entries) {

        print_hex_dump(
            KERN_ERR, "ENTRIES: ", DUMP_PREFIX_OFFSET,
            16, 1, buffer, 128, true
        );
    }

    kfree(buffer);

    return 0;
}

static int __init ocp_init(void)
{
    int ret;

    switch (which_call)
    {
    case 0:
        pr_info("Testing OCP list sources\n");
        ret = test_ocp_list();
        break;
    case 1:
        pr_info("Testing OCP read source\n");
        ret = test_ocp_read();
        break;
    case 2:
        pr_info("Testing OCP write source\n");
        ret = test_ocp_write();
        break;
    default:
        pr_err("Invalid parameter %d\n", which_call);
        return -1;
    }    

    if(ret<0) {
        pr_err("Error during init %d\n",ret);
        return -1;
    }

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
