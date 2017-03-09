/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : mem_int.c
* By      : gq.yang
* Version : v1.0
* Date    : 2012-11-3 20:13
* Descript: interrupt for platform mem
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include <asm/io.h>
#include "pm_i.h"

static void *GicDDisc;
static void *GicCDisc;

/*
*********************************************************************************************************
*                                       STANDBY INTERRUPT INITIALISE
*
* Description: mem interrupt initialise.
*
* Arguments  : none.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 mem_int_init(void)
{
	void __iomem *io_gic_dist_base = ioremap_nocache(SUNXI_GIC_DIST_PBASE, GIC_400_ENABLE_LEN * 4);
	void __iomem *io_gic_cpu_base = ioremap_nocache(SUNXI_GIC_CPU_PBASE, 0x100);
	__u32 i = 0; 
	// GicDDisc = (void *)IO_ADDRESS(SUNXI_GIC_DIST_PBASE);
	// GicCDisc = (void *)IO_ADDRESS(SUNXI_GIC_CPU_PBASE);
	GicDDisc = (void *)io_gic_dist_base;
	GicCDisc = (void *)io_gic_cpu_base;
	
	//printk("gic iar == 0x%x. \n", *(volatile __u32	 *)(IO_ADDRESS(SUNXI_GIC_CPU_PBASE)+0x0c));

	/* initialise interrupt enable and mask for mem */
	
	/*
	 * Disable all interrupts.  Leave the PPI and SGIs alone
	 * as these enables are banked registers.
	 */
	for (i = 4; i < (GIC_400_ENABLE_LEN); i += 4)
		iowrite32(0xffffffff, GicDDisc + GIC_DIST_ENABLE_CLEAR + i);
		// *(volatile __u32 *)(GicDDisc + GIC_DIST_ENABLE_CLEAR + i) = 0xffffffff;

	/*config cpu interface*/
#if 0
	*(volatile __u32 *)(GicCDisc + GIC_CPU_PRIMASK) = 0xf0;
	*(volatile __u32 *)(GicCDisc + GIC_CPU_CTRL) = 0x1;
#endif
#if 1
	/* clear external irq pending: needed */
	for (i = 4; i < (GIC_400_ENABLE_LEN); i += 4)
		iowrite32(0xffffffff, GicDDisc + GIC_DIST_PENDING_CLEAR + i);
		// *(volatile __u32 *)(GicDDisc + GIC_DIST_PENDING_CLEAR + i) = 0xffffffff;
#endif
	//the print info just to check the pending state, actually, after u read iar, u need to access end of interrupt reg;
	// i = *(volatile __u32   *)(GicCDisc + 0x0c);
	i = ioread32(GicCDisc + 0x0c);

	if(i != 0x3ff){
		//u need to 
		// *(volatile __u32 *)(GicCDisc + 0x10) = i;
		iowrite32(i, GicCDisc + 0x10);
		printk("notice: gic iar == 0x%x. \n", i);
	}
	
	
	iounmap(io_gic_dist_base);
	iounmap(io_gic_cpu_base);
	GicDDisc = NULL;
	GicCDisc = NULL;
	return 0;
}


/*
*********************************************************************************************************
*                                       STANDBY INTERRUPT INITIALISE
*
* Description: mem interrupt exit.
*
* Arguments  : none.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 mem_int_exit(void)
{
	void __iomem *io_gic_dist_base = ioremap_nocache(SUNXI_GIC_DIST_PBASE, GIC_400_ENABLE_LEN * 4);
	int i = 0;
	volatile __u32 enable_bit = 0;
	
	GicDDisc = (void *)io_gic_dist_base;
	//all the disable-int-src pending, need to be clear
	for(i = 0; i < GIC_400_ENABLE_LEN; i += 4){
		// enable_bit = *(volatile __u32 *)(GicDDisc + GIC_DIST_ENABLE_SET + i);
		// *(volatile __u32 *)(GicDDisc + GIC_DIST_PENDING_CLEAR + i) &= (~enable_bit);
		enable_bit = ioread32(GicDDisc + GIC_DIST_ENABLE_SET + i);
		iowrite32(ioread32(GicDDisc + GIC_DIST_PENDING_CLEAR + i) & ~enable_bit, GicDDisc + GIC_DIST_PENDING_CLEAR + i);
	}

	iounmap(io_gic_dist_base);
	GicDDisc = NULL;
	return 0;
}


/*
*********************************************************************************************************
*                                       QUERY INTERRUPT
*
* Description: enable interrupt.
*
* Arguments  : src  interrupt source number.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 mem_enable_int(enum interrupt_source_e src)
{
	void __iomem *io_gic_dist_base = ioremap_nocache(SUNXI_GIC_DIST_PBASE, GIC_400_ENABLE_LEN * 4);
	__u32   tmpGrp = (__u32)src >> 5;
	__u32   tmpSrc = (__u32)src & 0x1f;

	// GicDDisc = (void *)IO_ADDRESS(SUNXI_GIC_DIST_PBASE);
	// GicCDisc = (void *)IO_ADDRESS(SUNXI_GIC_CPU_PBASE);
	GicDDisc = (void *)io_gic_dist_base;

	//enable interrupt source
	// *(volatile __u32 *)(GicDDisc + GIC_DIST_ENABLE_SET + tmpGrp*4) |= (1<<tmpSrc);
	iowrite32(ioread32(GicDDisc + GIC_DIST_ENABLE_SET + tmpGrp*4) | (1<<tmpSrc), GicDDisc + GIC_DIST_ENABLE_SET + tmpGrp*4);
	//printk("GicDDisc + GIC_DIST_ENABLE_SET + tmpGrp*4 = 0x%x. tmpGrp = 0x%x.\n", GicDDisc + GIC_DIST_ENABLE_SET + tmpGrp*4, tmpGrp);
	//printk("tmpSrc = 0x%x. \n", tmpSrc);

	//need to care mask or priority?

	iounmap(io_gic_dist_base);
	GicDDisc = NULL;
	return 0;
}


/*
*********************************************************************************************************
*                                       QUERY INTERRUPT
*
* Description: query interrupt.
*
* Arguments  : src  interrupt source number.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 mem_query_int(enum interrupt_source_e src)
{
	void __iomem *io_gic_dist_base = ioremap_nocache(SUNXI_GIC_DIST_PBASE, GIC_400_ENABLE_LEN * 4);
	__s32   result = 0;
	__u32   tmpGrp = (__u32)src >> 5;
	__u32   tmpSrc = (__u32)src & 0x1f;
	// GicDDisc = (void *)IO_ADDRESS(SUNXI_GIC_DIST_PBASE);
	// GicCDisc = (void *)IO_ADDRESS(SUNXI_GIC_CPU_PBASE);
	GicDDisc = (void *)io_gic_dist_base;

	// result = *(volatile __u32 *)(GicDDisc + GIC_DIST_PENDING_SET + tmpGrp*4) & (1<<tmpSrc);
	result = ioread32(GicDDisc + GIC_DIST_PENDING_SET + tmpGrp*4) & (1<<tmpSrc);

	//printk("GicDDisc + GIC_DIST_PENDING_SET + tmpGrp*4 = 0x%x. tmpGrp = 0x%x.\n", GicDDisc + GIC_DIST_PENDING_SET + tmpGrp*4, tmpGrp);
	//printk("tmpSrc = 0x%x. result = 0x%x. \n", tmpSrc, result);

	iounmap(io_gic_dist_base);
	GicDDisc = NULL;
	return result? 0:-1;
}

