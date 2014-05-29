#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>
#include <linux/kernel.h>

#include <asm/uaccess.h>	/* copy_*_user */

MODULE_LICENSE("Dual BSD/GPL");

static char* fpgafn = "vrp_fpga.bin";
static int burst_major = 0;
static int burst_minor = 0;
module_param(fpgafn, charp, S_IRUGO);
module_param(burst_major, int, S_IRUGO);
module_param(burst_minor, int, S_IRUGO);

static struct burst_dev {
  struct semaphore sem; /* mutual exclusion semaphore     */
  struct cdev cdev; /* Char device structure		*/
} burst_device;

static char message[100];
static int cmessage;

int burst_open(
    struct inode *inode,
    struct file *filp) {
  struct burst_dev *dev; /* device information */
  dev = container_of(inode->i_cdev, struct burst_dev, cdev);
  filp->private_data = dev; /* for other methods */
  printk("burst_open\n");
  return 0; /* success */
}

int burst_release(
    struct inode *inode,
    struct file *filp) {
  printk("burst_release\n");
  return 0;
}

ssize_t burst_read(
    struct file *filp,
    char __user *buf,
    size_t count,
    loff_t *f_pos) {
  struct burst_dev *dev = filp->private_data;
  ssize_t retval = 0;
  int len = 0;
  if(down_interruptible(&dev->sem))
    return -ERESTARTSYS;

  cmessage = jiffies;
  len = sprintf(message, "%d\n", cmessage);
  if(copy_to_user(buf, message, len)) {
    retval = -EFAULT;
    goto out;
  }
  retval = len;

  out: up(&dev->sem);
  return retval;
}

struct file_operations burst_fops = { .owner = THIS_MODULE,
//	.llseek =   burst_llseek,
    .read = burst_read,
//	.write =    burst_write,
//	.unlocked_ioctl = burst_ioctl,
    .open = burst_open,
    .release = burst_release, };

static int vrp6_init(
    void) {
  dev_t devno = 0;
  int result;
  if(burst_major) {
    devno = MKDEV(burst_major, burst_minor);
    result = register_chrdev_region(devno, 1, "burst");
  }
  else {
    result = alloc_chrdev_region(&devno, burst_minor, 1, "burst");
    burst_major = MAJOR(devno);
    burst_minor = MINOR(devno);
  }
  if(result < 0) {
    printk(KERN_WARNING "vrp: can't get major %d\n", burst_major);
    return result;
  }

  printk(
      KERN_ALERT "vrp6 init, major %d, minor %d\n",
      burst_major,
      burst_minor);

  sema_init(&burst_device.sem, 1);
  cdev_init(&burst_device.cdev, &burst_fops);
  burst_device.cdev.owner = THIS_MODULE;
  burst_device.cdev.ops = &burst_fops;
  result = cdev_add(&burst_device.cdev, devno, 1);
  /* Fail gracefully if need be */
  if(result)
    printk(KERN_NOTICE "Error %d adding burst", result);

  return 0;
}

static void vrp6_exit(
    void) {
  dev_t devno = MKDEV(burst_major, burst_minor);
  cdev_del(&burst_device.cdev);
  unregister_chrdev_region(devno, 1);
  printk(KERN_ALERT "vrp6 exit\n");
}

module_init(vrp6_init);
module_exit(vrp6_exit);

