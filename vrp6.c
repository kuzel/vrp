#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>

MODULE_LICENSE("Dual BSD/GPL");

static char* fpgafn = "vrp_fpga.bin";
static int burst_major = 0;
static int burst_minor = 0;
module_param( fpgafn, charp, S_IRUGO);
module_param( burst_major, int, S_IRUGO);
module_param( burst_minor, int, S_IRUGO);

static int vrp6_init(void) {
	dev_t dev = 0;
	int result;
	if (burst_major) {
		dev = MKDEV(burst_major, burst_minor);
		result = register_chrdev_region(dev, 1, "burst");
	} else {
		result = alloc_chrdev_region(&dev, burst_minor, 1, "burst");
		burst_major = MAJOR(dev);
		burst_minor = MINOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "vrp: can't get major %d\n", burst_major);
		return result;
	}

	printk(KERN_ALERT "vrp6 init, major %d, minor %d\n", burst_major, burst_minor);
	return 0;
}

static void vrp6_exit(void) {
	dev_t devno = MKDEV(burst_major, burst_minor);
	unregister_chrdev_region(devno, 1);
	printk(KERN_ALERT "vrp6 exit\n");
}

module_init( vrp6_init);
module_exit( vrp6_exit);

