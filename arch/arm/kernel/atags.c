#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <asm/setup.h>

static size_t atags_size = 0;
static char * atags_buf;

static ssize_t atags_read(struct file *file, char __user *buf,
			  size_t len, loff_t *offset)
{
	ssize_t count;
	loff_t pos = *offset;

	if (!atags_buf)
		return 0;

	if (pos >= atags_size)
		return 0;

	count = min(len, (size_t)(atags_size - pos));
	if (copy_to_user(buf, atags_buf + pos, count))
		return -EFAULT;

	*offset += count;
	return count;
}

static const struct file_operations atags_file_ops = {
	.read = atags_read,
};

#define BOOT_PARAMS_SIZE (1 << 14)
static char __initdata atags_copy[BOOT_PARAMS_SIZE];

void __init save_atags(const struct tag *tags)
{
	memcpy(atags_copy, tags, sizeof(atags_copy));
}

static int __init init_atags_procfs(void)
{
	/*
	 * This cannot go into save_atags() because kmalloc and proc don't work
	 * yet when it is called.
	 */
	struct proc_dir_entry *tags_entry;
	struct tag *tag = (struct tag *)atags_copy;
	size_t size;

	if (tag->hdr.tag != ATAG_CORE) {
		pr_info("No ATAGs?");
		return -EINVAL;
	}

	for (; tag->hdr.size; tag = tag_next(tag)) {
		size = (char*)tag_next(tag) - (char*)atags_copy;
		if (size > sizeof(atags_copy)) {
			pr_info("Atags bigger than buffer!");
			return -EINVAL;
		}
	}

	/* include the terminating ATAG_NONE */
	atags_size = (char *)tag - (char *)atags_copy + sizeof(struct tag_header);

	WARN_ON(tag->hdr.tag != ATAG_NONE);

	atags_buf = kmalloc(atags_size, GFP_KERNEL);
	if (!atags_buf)
		goto nomem;

	memcpy(atags_buf, atags_copy, atags_size);

	tags_entry = create_proc_entry("atags", S_IFREG | S_IRUGO, NULL);

	if (!tags_entry)
		goto nomem;

	tags_entry->proc_fops = &atags_file_ops;
	tags_entry->uid = 0;
	tags_entry->gid = 0;
	tags_entry->size = atags_size;
	return 0;

nomem:
	kfree(atags_buf);
	pr_err("Exporting ATAGs: not enough memory\n");

	return -ENOMEM;
}
arch_initcall(init_atags_procfs);
