
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>


MODULE_LICENSE("GPL");

int __init init_module(void);
void __exit cleanup_module(void);
static int device_open(struct inode *inode, struct file *file);
static int device_release(struct inode *inode, struct file *file);
static ssize_t device_read(struct file *flip, char __user *buffer, size_t length, loff_t *offset);
static ssize_t device_write(struct file *filp, const char __user *buffer, size_t length, loff_t *offset);


#define DEVICE_NAME "kmem_inspect"

static int major_num;
static dev_t device_num;
static struct class *cls;
static struct device *dev;
static u8 *addr;
static int msg_len;

static int content_len = 1;
module_param(content_len, int, 0);

static struct file_operations kmem_inspect_fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
};


static int device_open(struct inode *inode, struct file *file) {
	try_module_get(THIS_MODULE);
	return 0;
}


static int device_release(struct inode *inode, struct file *file) {
	module_put(THIS_MODULE);
	return 0;
}


static ssize_t device_read(struct file *flip, char __user *buffer, size_t length, loff_t *offset) {
	int bytes_read = 0;
	char current_byte[5] = {0, 0, 0, 0, 0};

	if (*offset >= msg_len) {
		*offset = 0;
		return 0; //EOF
	}

	int is_lower_nibble = 0;
	while (length && (*offset < msg_len)) {
		if (*offset < msg_len - 1){
			if (!is_lower_nibble) {
				sprintf(current_byte, "%x", *(addr + (*offset / 2)));
			}
		} else {
			current_byte[0] = '\n';
			current_byte[1] = '\n';

		}
		put_user(current_byte[is_lower_nibble] , buffer);
		buffer += 1;
		bytes_read += 1;
		length -= 1;
		*offset += 1;
		is_lower_nibble = 1 - is_lower_nibble;
	}

	return bytes_read;
}

static ssize_t device_write(struct file *filp, const char __user *buffer, size_t length, loff_t *offset) {
	int bytes_written;
	int ret;
	unsigned long long res_addr = 0;

	ret = kstrtoull_from_user(buffer, length, 16, &res_addr);
	if (ret) {
		pr_alert("Fail in retrieving an address from the user. Error: %d\n", ret);
		addr = 0;
		bytes_written = ret;
	} else {
		pr_info("The current requested address is 0x%llx\n", res_addr);
		addr = (u8*) res_addr;
		bytes_written = length;
	}
	return bytes_written;
}


int __init init_module(void) {
	major_num = register_chrdev(0, DEVICE_NAME, &kmem_inspect_fops);
	if (major_num < 0) {
		pr_alert("Fail in registering the device. Error: %d\n", major_num);
		return major_num;
	}
	device_num = MKDEV(major_num, 0);

	cls = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(cls)) {
		pr_alert("Fail in creating the class structure. Error: %ld\n", PTR_ERR(cls));
		return PTR_ERR(cls);
	}

	dev = device_create(cls, NULL, device_num, NULL, DEVICE_NAME);
	if (IS_ERR(dev)) {
		pr_alert("Fail in creating the device. Error: %ld\n", PTR_ERR(dev));
		return PTR_ERR(dev);
	}

	pr_info("content_len is %d\n", content_len);
	msg_len = content_len * 2 + 1; //"*2" because every byte is 2 ascii characters, +1" for the '\n' character
	
	return 0;
}


void __exit cleanup_module(void) {
	device_destroy(cls, device_num);
	class_destroy(cls);
	unregister_chrdev(major_num, DEVICE_NAME);
}