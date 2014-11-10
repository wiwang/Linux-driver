#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#define GLOBALMEM_SIZE 0x1000
#define GLOBALMEM_MAGIC 'V'
#define MEM_CLEAR _IO(GLOBALMEM_MAGIC, 0)

typedef struct globalmem_dev
{
    struct cdev cdev;
    unsigned char mem[GLOBALMEM_SIZE];
}GlobalMem_t;

static int globalmem_major;
GlobalMem_t dev;

MODULE_LICENSE("Dual BSD/GPL");

int globalmem_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &dev;
    return 0;
}

static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    unsigned long p = *ppos;
    int ret = 0;
    
    if(p >= GLOBALMEM_SIZE)
        return count ? -ENXIO:0;

    if(count > GLOBALMEM_SIZE -p)
        count = GLOBALMEM_SIZE - p;
    
    if(copy_to_user(buf, (void *)(dev.mem + p), count))
    {
        ret = -EFAULT;   
    }
    else
    {
        *ppos += count;
        ret = count;
    }

    return ret;
}

static ssize_t globalmem_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    unsigned long p = *ppos;
    int ret = 0;

    if(p >= GLOBALMEM_SIZE)
        return count ? -ENXIO:0;

    if(count > GLOBALMEM_SIZE -p)
        count = GLOBALMEM_SIZE - p;

    if(copy_from_user(dev.mem+p, buf, count))
    {
        ret = -EFAULT;
    }
    else
    {
        *ppos += count;
        ret = count;
    }

    return ret;
}

static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig)
{
    loff_t ret;


    return ret;


}

static int globalmem_ioctl(struct inode *inodep, struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch(cmd)
    {
        case MEM_CLEAR:
             memset(dev.mem, 0, GLOBALMEM_SIZE);
             break;

        default:
             return -EINVAL;
    }

    return 0;
}

static const struct file_operations globalmem_fops = 
{
.owner = THIS_MODULE,
.llseek = globalmem_llseek,
.read = globalmem_read,
.write = globalmem_write,
.compat_ioctl = globalmem_ioctl
};

static void globalmem_setup_cdev()
{
    int err, devno = MKDEV(globalmem_major, 0);

    cdev_init(&dev.cdev, &globalmem_fops);
    dev.cdev.owner = THIS_MODULE;
    dev.cdev.ops = &globalmem_fops;
    err = cdev_add(&(dev.cdev), devno, 1);
    if(err)
        printk(KERN_NOTICE "Error %d adding globalmem", err);
}

int globalmem_init(void)
{
    int result;
    dev_t devno = MKDEV(globalmem_major, 0);

	result = alloc_chrdev_region(&devno, 0, 1, "globalmem");
	globalmem_major = MAJOR(devno);

    if(result < 0)
        return result;

    globalmem_setup_cdev();

    return 0;
}

int globalmem_exit(void)
{
    cdev_del(&dev.cdev);
    unregister_chrdev_region(MKDEV(globalmem_major, 0), 1);
	
	return 0;
}


module_init(globalmem_init);
module_exit(globalmem_exit);
