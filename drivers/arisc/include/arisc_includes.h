/*
 *  arch/arm/mach-sunxi/arisc/include/arisc_includes.h
 *
 * Copyright (c) 2012 Allwinner.
 * 2012-05-01 Written by sunny (sunny@allwinnertech.com).
 * 2012-10-01 Written by superm (superm@allwinnertech.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ARISC_INCLUDES_H
#define __ARISC_INCLUDES_H

#define CONFIG_ARCH_SUN8IW5P1

#define IO_ADDRESS(x)                    IOMEM((x) + 0xf0000000)
#define SUNXI_SRAM_A2_PBASE              0x00040000
#define SUNXI_SRAM_A2_VBASE              IO_ADDRESS(SUNXI_SRAM_A2_PBASE      )
#define PLL_DDR0_CLK                     "pll_ddr0"
#define PLL_DDR1_CLK                     "pll_ddr1"
#define PLL_PERIPH_CLK                   "pll_periph"
#define HOSC_CLK                         "hosc"
#define LOSC_CLK                         "losc"
#define SUNXI_SRAM_A2_SIZE               0x00014000
#define SUNXI_R_CPUCFG_PBASE             0x01f01c00
#define SUNXI_GIC_START                  32
#define SUNXI_IRQ_MBOX                   (SUNXI_GIC_START + 49)  /* 81 */
#define SUNXI_MSGBOX_PBASE               0x01c17000
#define SUNXI_SPINLOCK_PBASE             0x01c18000
#define SUNXI_SRAMCTRL_PBASE             0x01c00000

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/arisc/hwmsgbox.h>
#include <linux/arisc/hwspinlock.h>
//#include <mach/irqs.h>
//#include <mach/hardware.h>
//#include <mach/platform.h>

/* configure and debugger */
#include "./../arisc_i.h"
#include "./arisc_cfgs.h"
#include "./arisc_dbgs.h"

/* messages define */
#include "./arisc_messages.h"
#include "./arisc_message_manager.h"

/* driver headers */
#include "./arisc_hwmsgbox.h"
#include "./arisc_hwspinlock.h"

/* global functions */
extern int arisc_axp_int_notify(struct arisc_message *pmessage);
extern int arisc_audio_perdone_notify(struct arisc_message *pmessage);
extern int arisc_set_debug_level(unsigned int level);
extern int arisc_dvfs_cfg_vf_table(void);
extern int arisc_set_uart_baudrate(u32 baudrate);
extern int arisc_set_dram_crc_paras(unsigned int dram_crc_en, unsigned int dram_crc_srcaddr, unsigned int dram_crc_len);
extern int arisc_query_set_standby_info(struct standby_info_para *para, arisc_rw_type_e op);
extern int arisc_sysconfig_sstpower_paras(void);

/* global vars */
extern unsigned long arisc_sram_a2_vbase;

#endif /* __ARISC_INCLUDES_H */
