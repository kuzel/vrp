#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <asm/uaccess.h>
#include <asm/current.h>

MODULE_LICENSE("Dual BSD/GPL");

static char* fpgafn = "vrp_fpga.bin";
static int burst_major = 0;
static int burst_minor = 0;
static int divider = 1;
module_param(fpgafn, charp, S_IRUGO);
module_param(burst_major, int, S_IRUGO);
module_param(burst_minor, int, S_IRUGO);
module_param(divider, int, S_IRUGO);

static struct burst_dev {
  wait_queue_head_t inq;
  struct timer_list timer;
  struct semaphore sem;
  struct cdev cdev;
} burst_device;

static char message[100];
static int event = 0;

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
  int local_event;

  if(down_interruptible(&dev->sem))
    return -ERESTARTSYS;
  local_event = event;
  up(&dev->sem);

//  if(filp->f_flags & O_NONBLOCK)
//    return -EAGAIN;
//  printk(KERN_WARNING "\"%s\" reading: going to sleep\n", current->comm);
  if(wait_event_interruptible(dev->inq, (event != local_event)))
    return -ERESTARTSYS;
  if(down_interruptible(&dev->sem))
    return -ERESTARTSYS;

  len = sprintf(message, "HZ:%d, %d\n", HZ, event);
  if(copy_to_user(buf, message, len)) {
    retval = -EFAULT;
    goto out;
  }
  retval = len;

  out: up(&dev->sem);
  return retval;
}

void timer_cb(
    unsigned long arg) {
//  printk(KERN_WARNING "timer\n");
  /* -- TODO protect by spinlock or atomic increment? */
  event = event + 1;
  wake_up_interruptible(&burst_device.inq);
  mod_timer(&burst_device.timer, jiffies + HZ/divider);
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
  if(divider < 1) {
    printk(KERN_WARNING "bad divider");
    return -1;
  }
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
  KERN_ALERT "vrp6 init, major %d, minor %d\n", burst_major, burst_minor);

  init_waitqueue_head(&burst_device.inq);
  init_timer(&burst_device.timer);
  burst_device.timer.data = 0;
  burst_device.timer.function = timer_cb;
  sema_init(&burst_device.sem, 1);
  cdev_init(&burst_device.cdev, &burst_fops);
  burst_device.cdev.owner = THIS_MODULE;
  burst_device.cdev.ops = &burst_fops;
  result = cdev_add(&burst_device.cdev, devno, 1);
  /* Fail gracefully if need be */
  if(result)
    printk(KERN_NOTICE "Error %d adding burst", result);

  mod_timer(&burst_device.timer, jiffies + HZ/divider);

  return 0;
}

static void vrp6_exit(
    void) {
  dev_t devno = MKDEV(burst_major, burst_minor);
  cdev_del(&burst_device.cdev);
  unregister_chrdev_region(devno, 1);
  del_timer_sync(&burst_device.timer);
  printk(KERN_ALERT "vrp6 exit\n");
}

module_init(vrp6_init);
module_exit(vrp6_exit);

