// --------------------------------------------------
//  Includes
// --------------------------------------------------
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
//#include <plat/sys_config.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/io.h>

// --------------------------------------------------
//  Constants
// --------------------------------------------------
#define IO_BASE   0x1C20000
#define IO_BASE_SIZE 0x70

// --------------------------------------------------
//  Global Static Variables
// --------------------------------------------------
static struct kobject *micile_iospy_kobj;
static int            micile_iospy_io_addr = 0;
void			      *micile_iospy_ioremap;

// --------------------------------------------------
//  File Accessor Functions
// --------------------------------------------------
static ssize_t io_addr_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	// show the register 
    return sprintf(buf, "0x%x\n", micile_iospy_io_addr);
}

static ssize_t io_addr_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%x", &micile_iospy_io_addr);
    return count;
}

static ssize_t io_val_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    u32 val;
    val = ioread32((char*)micile_iospy_ioremap + micile_iospy_io_addr);
    return sprintf(buf, "0x%08X\n", val);
}

static ssize_t io_val_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    u32 val;
    sscanf(buf, "%x", &val);
    iowrite32(val, (char*)micile_iospy_ioremap + micile_iospy_io_addr);
    return count;
}

static ssize_t test_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "Hello This is a test!\n");
}

static ssize_t test_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    printk("writing test!\n");
    return count;
}


static struct kobj_attribute io_addr_attribute = __ATTR(io_addr, 0660, io_addr_show, io_addr_store);
static struct kobj_attribute io_val_attribute = __ATTR(io_val, 0660, io_val_show, io_val_store);
static struct kobj_attribute test_attribute = __ATTR(test, 0660, test_show, test_store);
//static struct kobj_attribute gpio_dump_attribute = __ATTR(test, 0660, gpio_dump_show, gpio_dump_store);


// --------------------------------------------------
//  KObject Attributes and Attribute Groups
// --------------------------------------------------
static struct attribute *attrs[] = {
    &io_addr_attribute.attr,
    &io_val_attribute.attr,
//    &gpio_dump_attribute.attr,
    &test_attribute.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};


// --------------------------------------------------
//  Module Init and Exit
// --------------------------------------------------
#define CFG0 0x00
#define CFG1 0x04
#define CFG2 0x08
#define CFG3 0x0C
#define DAT  0x10
#define DRV0 0x14
#define DRV1 0x18
#define PUL0 0x1C
#define PUL1 0x20

#define REG_CFG 0
#define REG_DAT 4
#define REG_DRV 5
#define REG_PUL 7

#define PORTB_PINS 8
#define PORTL_PINS 12


static void dump_pio(int portnum) {
	void *iorp;
	u32 regs[9];
	int i;
    int cfg;
    int pul;
    int drv;
    int val;
	
	// ABCDEFGHIJKL
	// x0123456xxx0
	


	int pins[] = {0,
	 8 /* (0)  B   */, 
	          17 /* (1)  C   */,
	          22 /* (2)  D   */,
	          18 /* (3)  E   */,
	           6 /* (4)  F   */,
	          14 /* (5)  G   */,
	          10 /* (6)  H   */,
	          0  /* (7)    I */,
	          0  /* (8)    J */,
	          0  /* (9)    K */,
	          12 /* (10) L   */};

	if (portnum < 7) {
		iorp = ioremap_nocache(0x01C20800 + portnum * 0x24, 0x30);
	} else {
		iorp = ioremap_nocache(0x01f02c00, 0x30);
	}
	for (i = 0; i < 9; i++) {
		regs[i] = ioread32(iorp + 0x04 * i);
	}
	printk("P%c_CFG0 = 0x%08X\n", 'A' + portnum, regs[0]);
	printk("P%c_CFG1 = 0x%08X\n", 'A' + portnum, regs[1]);
	printk("P%c_CFG2 = 0x%08X\n", 'A' + portnum, regs[2]);
	printk("P%c_CFG3 = 0x%08X\n", 'A' + portnum, regs[3]);
	printk("P%c_DAT  = 0x%08X\n", 'A' + portnum, regs[4]);
	printk("P%c_DRV0 = 0x%08X\n", 'A' + portnum, regs[5]);
	printk("P%c_DRV1 = 0x%08X\n", 'A' + portnum, regs[6]);
	printk("P%c_PUL0 = 0x%08X\n", 'A' + portnum, regs[7]);
	printk("P%c_PUL1 = 0x%08X\n", 'A' + portnum, regs[8]);

	         

	for (i = 0; i < pins[portnum]; i++) {
		cfg = (regs[REG_CFG + (i / 8)] >> ((i % 8) * 4)) & 0x0F;
		val = (regs[REG_DAT] >> i) & 0x01; 
		drv = (regs[REG_DRV + (i / 16)] >> (i % 16)*2) & 0x03; 
		pul = (regs[REG_PUL + (i / 16)] >> (i % 16)*2) & 0x03;
//		printk("\t P%C%02d cfg=0x%X val=%d drv=0x%X pul=0x%X\n", 'B' + portnum, i, cfg, val, drv, pul);
		printk("\t P%c%02d cfg=0x%X val=%d drv=0x%X pul=0x%X\n", 'A' + portnum, i, cfg, val, drv, pul);
	}
	
	iounmap(iorp);	
}
static int __init micile_iospy_init(void) {
    // Variables
        int retval;

        printk("micile_iospy_init\n");

    // Create the /sys/micile kobject and add a refcount for it
        micile_iospy_kobj = kobject_create_and_add("micile", kernel_kobj);
        if (!micile_iospy_kobj) return -ENOMEM;

    // Create the attributes under /sys/micile
        retval = sysfs_create_group(micile_iospy_kobj, &attr_group);
        if (retval) kobject_put(micile_iospy_kobj);

    // Remap the io registers to memory space
		micile_iospy_ioremap = ioremap_nocache(0x01c20000, 0x300);

dump_pio(0);
dump_pio(1);
dump_pio(2);
dump_pio(3);
dump_pio(4);
dump_pio(5);
dump_pio(6);
    // Success?
        return retval;
}

static void __exit micile_iospy_exit(void) {
	// unmap the io space
    	iounmap(micile_iospy_ioremap);

    // Decrement the ref count of micile_kobj to zero
        kobject_put(micile_iospy_kobj);
}

module_init(micile_iospy_init);
module_exit(micile_iospy_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lawrence Yu Micile Inc.<lyu@micile.com>");
