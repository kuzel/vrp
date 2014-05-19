#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>

MODULE_LICENSE("Dual BSD/GPL");

static char* fpgafn = "vrp_fpga.bin";
module_param(fpgafn, charp, S_IRUGO);

static int vrp6_init(void) {
  printk(KERN_ALERT "vrp6 init\n");
  return 0;
}

static void vrp6_exit(void) {
  printk(KERN_ALERT "vrp6 exit\n");
}

module_init(vrp6_init);
module_exit(vrp6_exit);

