/*
 * lunix-chrdev.c
 *
 * Implementation of character devices
 * for Lunix:TNG
 *
 * Danassis Panayiotis
 * Aronis Panagiotis
 *
 */

#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mmzone.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>

#include "lunix.h"
#include "lunix-chrdev.h"
#include "lunix-lookup.h"

#define CDEV_NAME "LunixCharDev"

/*
 * Global data
 */
struct cdev lunix_chrdev_cdev;

/*
 * Just a quick [unlocked] check to see if the cached
 * chrdev state needs to be updated from sensor measurements.
 */
static int lunix_chrdev_state_needs_refresh(struct lunix_chrdev_state_struct *state)
{
	struct lunix_sensor_struct *sensor;
	debug("entering lunix_chrdev_state_needs_refresh\n");

	WARN_ON ( !(sensor = state->sensor));
	/* ? */
	if (state->buf_timestamp < sensor->msr_data[state->type]->last_update) {
		debug("leaving lunix_chrdev_state_needs_refresh\n");
		return 1;
	} else {
		debug("again!\n");
		return 0;
	}
}

/*
 * Updates the cached state of a character device
 * based on sensor data. Must be called with the
 * character device state lock held.
 */
static int lunix_chrdev_state_update(struct lunix_chrdev_state_struct *state)
{
	//struct lunix_sensor_struct *sensor;
	unsigned long flags;
	int need_refresh, n;
	long int data;
	uint32_t raw_data, raw_time;
	char c;
	
	debug("entering lunix_chrdev_state_update\n");

	/*
	 * Grab the raw data quickly, hold the
	 * spinlock for as little as possible.
	 */
	/* ? */
	/* Why use spinlocks? See LDD3, p. 119 */

	/*
	 * Any new data available?
	 */
	/* ? */
	need_refresh = lunix_chrdev_state_needs_refresh(state);
	if (need_refresh) {
		debug("Spinlock almost at hand.\n");
		spin_lock_irqsave(&(state->sensor->lock), flags);
		debug("Spinlock at hand.\n");
		raw_data = state->sensor->msr_data[state->type]->values[0];
		raw_time = state->sensor->msr_data[state->type]->last_update;
		spin_unlock_irqrestore(&(state->sensor->lock), flags);
	}
	else {
		debug("Again!\n");		
		return -EAGAIN; /* No new data */
	}
	/*
	 * Now we can take our time to format them,
	 * holding only the private state semaphore
	 */
	/* ? */
	
	switch(state->type) {
		debug("case 0\n");
		case BATT: data = lookup_voltage[raw_data]; break;
		debug("case 1\n");
		case TEMP: data = lookup_temperature[raw_data]; break;
		debug("case 2\n");
		case LIGHT: data = lookup_light[raw_data]; break;
		default: ;
		debug("case default\n");
	}
	c = (data >= 0) ? '+' : '-';
	data = (data > 0) ? data : (-data);
	n = sprintf(state->buf_data, "%c%ld.%ld\n", c, data/1000, data%1000 );
	state->buf_timestamp = raw_time;
	state->buf_lim = n;
	
	debug("leaving\n");
	return 0;
}

/*************************************
 * Implementation of file operations
 * for the Lunix character device
 *************************************/

static int lunix_chrdev_open(struct inode *inode, struct file *filp)
{
	/* Declarations */
	/* ? */
	struct lunix_chrdev_state_struct *state;
	struct lunix_sensor_struct *sensor;
	unsigned int minor_number, sensor_number;
	
	int ret;

	debug("entering\n");
	ret = -ENODEV;
	if ((ret = nonseekable_open(inode, filp)) < 0)
		goto out;

	/*
	 * Associate this open file with the relevant sensor based on
	 * the minor number of the device node [/dev/sensor<NO>-<TYPE>]
	 */	 
	minor_number = iminor(inode);
	sensor_number = minor_number / 8;
	minor_number = minor_number % 8;
	
	sensor = &lunix_sensors[sensor_number];
	debug("relevant sensor\n");
	
	/* Allocate a new Lunix character device private state structure */
	/* ? */
	state = kmalloc(sizeof (struct lunix_chrdev_state_struct), GFP_KERNEL);
	if (!state) {
		/* the allocation failed */
		debug("the allocation of lunix_chrdev_state_struct failed\n");
		ret = -ENOMEM;
		goto out;
	}
	debug("allocation of state completed\n");
	
	state->sensor = sensor;
	state->type = minor_number;
	state->buf_timestamp = get_seconds();
	state->buf_lim = 0;
	sema_init(&(state->lock), 1);
	debug("sema_init secceded\n");
	
	filp->private_data = state;
	filp->f_pos = 0;
	
	ret = 0;
	
out:
	debug("leaving, with ret = %d\n", ret);
	return ret;
}

static int lunix_chrdev_release(struct inode *inode, struct file *filp)
{
	/* ? */
	kfree(filp->private_data);
	return 0;
}

static long lunix_chrdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	/* Why? */
	return -EINVAL;
}

static ssize_t lunix_chrdev_read(struct file *filp, char __user *usrbuf, size_t cnt, loff_t *f_pos)
{
	ssize_t ret;
	struct lunix_sensor_struct *sensor;
	struct lunix_chrdev_state_struct *state;
	
	int di,bytes;
	
	debug("entering\n");
	
	state = filp->private_data;
	WARN_ON(!state);

	sensor = state->sensor;
	WARN_ON(!sensor);

	/* Lock? */
	di = down_interruptible(&(state->lock));
	if (di) {
		/* Failed to V */
		debug("failed down_interruptible\n");
		ret = -EINTR;
		goto out;
	}
	
	/*
	 * If the cached character device state needs to be
	 * updated by actual sensor data (i.e. we need to report
	 * on a "fresh" measurement, do so
	 */
	if (*f_pos == 0) {
		while (lunix_chrdev_state_update(state) == -EAGAIN) {
			/* ? */
			/* The process needs to sleep */
			/* See LDD3, page 153 for a hint */
			
			up(&(state->lock));
			if (wait_event_interruptible(sensor->wq, lunix_chrdev_state_needs_refresh(state)))
				return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
			di = down_interruptible(&(state->lock));
			if (di) {
				/* Failed to V */
				debug("failed down_interruptible\n");
				ret = -EINTR;
				goto out;
			}
		}
	}

	debug("ready to copy_to_user\n");

	bytes = (cnt < state->buf_lim - *f_pos) ? cnt : state->buf_lim-*f_pos;

  	ret = copy_to_user(usrbuf, state->buf_data + *f_pos, bytes);
	if (ret) {
		/* Failed to copy */
		debug("copy_to_user failed\n");
	}
	
	/* Auto-rewind on EOF mode? */
	/* ? */
	
	(*f_pos)+=bytes;
	if(*f_pos >= state->buf_lim) *f_pos=0;
	ret = bytes;
	
out:
	/* Unlock? */
	up(&(state->lock));
	return ret;
}

static int lunix_chrdev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return -EINVAL;
}

static struct file_operations lunix_chrdev_fops = 
{
    	.owner          = THIS_MODULE,
	.open           = lunix_chrdev_open,
	.release        = lunix_chrdev_release,
	.read           = lunix_chrdev_read,
	.unlocked_ioctl = lunix_chrdev_ioctl,
	.mmap           = lunix_chrdev_mmap
};

int lunix_chrdev_init(void)
{
	/*
	 * Register the character device with the kernel, asking for
	 * a range of minor numbers (number of sensors * 8 measurements / sensor)
	 * beginning with LINUX_CHRDEV_MAJOR:0
	 */
	int ret;
	dev_t dev_no;
	unsigned int lunix_minor_cnt = lunix_sensor_cnt << 3;
	
	debug("initializing character device\n");
	cdev_init(&lunix_chrdev_cdev, &lunix_chrdev_fops);
	lunix_chrdev_cdev.owner = THIS_MODULE;
	
	dev_no = MKDEV(LUNIX_CHRDEV_MAJOR, 0);
	/* ? */
	lunix_chrdev_cdev.ops = &lunix_chrdev_fops;	
	
	ret = register_chrdev_region(dev_no, lunix_minor_cnt, CDEV_NAME);
	/* register_chrdev_region? */
	if (ret < 0) {
		debug("failed to register region, ret = %d\n", ret);
		goto out;
	}	
	/* ? */
	
	ret = cdev_add(&lunix_chrdev_cdev, dev_no, lunix_minor_cnt);
	/* cdev_add? */
	if (ret < 0) {
		debug("failed to add character device\n");
		goto out_with_chrdev_region;
	}
	debug("completed successfully\n");
	return 0;

out_with_chrdev_region:
	unregister_chrdev_region(dev_no, lunix_minor_cnt);
out:
	return ret;
}

void lunix_chrdev_destroy(void)
{
	dev_t dev_no;
	unsigned int lunix_minor_cnt = lunix_sensor_cnt << 3;
		
	debug("entering\n");
	dev_no = MKDEV(LUNIX_CHRDEV_MAJOR, 0);
	cdev_del(&lunix_chrdev_cdev);
	unregister_chrdev_region(dev_no, lunix_minor_cnt);
	debug("leaving\n");
}
