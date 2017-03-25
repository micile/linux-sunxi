//#define __uxx_sxx_name
#include "pm_i.h"
#include <asm/io.h>


/*
*********************************************************************************************************
*                                   mem_twi_save
*
*Description: save twi status before enter super standby.
*
*Arguments  :
*
*Return     :
*
*********************************************************************************************************
*/
__s32 mem_twi_save(struct twi_state *ptwi_state)
{
	int i=0;
	void __iomem *io_twi0_base = ioremap_nocache(SUNXI_TWI0_PBASE, TWI_REG_LENGTH * 4);
	void __iomem *io_twi1_base = ioremap_nocache(SUNXI_TWI1_PBASE, TWI_REG_LENGTH * 4);
	void __iomem *io_twi2_base = ioremap_nocache(SUNXI_TWI2_PBASE, TWI_REG_LENGTH * 4);

	/*save all the twi reg*/
	for(i=0; i<(TWI_REG_LENGTH); i++){
		ptwi_state->twi0_reg_back[i] = ioread32(io_twi0_base + i*0x04);
		ptwi_state->twi1_reg_back[i] = ioread32(io_twi1_base + i*0x04);
		ptwi_state->twi2_reg_back[i] = ioread32(io_twi2_base + i*0x04);
	}
	iounmap(io_twi0_base);
	iounmap(io_twi1_base);
	iounmap(io_twi2_base);
	return 0;
}

/*
*********************************************************************************************************
*                                   mem_twi_restore
*
*Description: restore twi status after resume.
*
*Arguments  :
*
*Return     :
*
*********************************************************************************************************
*/
__s32 mem_twi_restore(struct twi_state *ptwi_state)
{
	int i=0;
	void __iomem *io_twi0_base = ioremap_nocache(SUNXI_TWI0_PBASE, TWI_REG_LENGTH * 4);
	void __iomem *io_twi1_base = ioremap_nocache(SUNXI_TWI1_PBASE, TWI_REG_LENGTH * 4);
	void __iomem *io_twi2_base = ioremap_nocache(SUNXI_TWI2_PBASE, TWI_REG_LENGTH * 4);

	// Setup the clocks, gating, and resets for the module before attempting to restore registers
	{
		// iomap the ccu registers
		void __iomem *io_ccu_base = ioremap_nocache(0x01c20000, 0x1000);
		// In order to restore the registers to a module, we need to setup and enable the clock to the module
//		*(volatile u32 *)(io_ccu_base + 0x104) = (1 << 31) | (2 << 24) | (0xF << 0);	// BE_CLK_REG = SCLK_GATING, PLL_PERIPH_X2, /16
		// Enable the bus clock gating register for the module
		*(volatile u32 *)(io_ccu_base + 0x06C) |= (7 << 0); // BUS_CLK_GATING_REG3 =| TWI2_GATING | TWI1_GATING | TWI0_GATING
		// Deassert the reset on the module
		*(volatile u32 *)(io_ccu_base + 0x2D8) |= (7 << 0); // BUS_SOFT_RST_REG4 =| TWI2_RST | TWI1_RST | TWI0_RST
		// unmap the ccu registers
		iounmap(io_ccu_base);
	}

	/*restore all the twi reg*/
	for(i=0; i<(TWI_REG_LENGTH); i++){
		iowrite32(ptwi_state->twi0_reg_back[i], io_twi0_base + i*0x04);
		//printk("RESTORING %d = 0x%X\n", i*0x04, ptwi_state->twi0_reg_back[i]);
		//printk("VERIFYING %d = 0x%x\n", i*0x04, ioread32(io_twi0_base + i*0x04));
		iowrite32(ptwi_state->twi1_reg_back[i], io_twi1_base + i*0x04);
		iowrite32(ptwi_state->twi2_reg_back[i], io_twi2_base + i*0x04);
	}
	iounmap(io_twi0_base);
	iounmap(io_twi1_base);
	iounmap(io_twi2_base);
	return 0;
}
