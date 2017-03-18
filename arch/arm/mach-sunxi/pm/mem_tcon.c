#include <asm/io.h>
#include "pm_types.h"
#include "pm_i.h"
/*
*********************************************************************************************************
*                                       MEM tcon INITIALISE
*
* Description: mem tcon initialise.
*
* Arguments  : none.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 mem_tcon_save(struct tcon_state *ptcon_state)
{
	int i=0;
	void __iomem *io_pio_base = ioremap_nocache(SUNXI_TCON_BASE, TCON_REG_LENGTH * 4);

	/*save all the tcon reg*/
	for(i=0; i<(TCON_REG_LENGTH); i++){
		ptcon_state->tcon_reg_back[i] = ioread32(io_pio_base + i*0x04);
//		printk("SAVING TCON REG 0x%X = 0x%x\n", SUNXI_TCON_BASE + i*0x04, ptcon_state->tcon_reg_back[i]);
	}
	iounmap(io_pio_base);
	return 0;
}

/*
*********************************************************************************************************
*                                       MEM tcon INITIALISE
*
* Description: mem tcon initialise.
*
* Arguments  : none.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 mem_tcon_restore(struct tcon_state *ptcon_state)
{
	int i=0;
	void __iomem *io_pio_base = ioremap_nocache(SUNXI_TCON_BASE, TCON_REG_LENGTH * 4);

	// Setup the clocks, gating, and resets for the module before attempting to restore registers
	{
		// iomap the ccu registers
		void __iomem *io_ccu_base = ioremap_nocache(0x01c20000, 0x1000);
		// In order to restore the registers to a module, we need to setup and enable the clock to the module
		*(volatile u32 *)(io_ccu_base + 0x118) = (1 << 31) | (2 << 24) | (0xF << 0);	// BE_CLK_REG = SCLK_GATING, PLL_PERIPH_X2, /16
		// Enable the bus clock gating register for the module
		*(volatile u32 *)(io_ccu_base + 0x064) |= (1 << 4); // BUS_CLK_GATING_REG1 =| LCD_GATING
		// Deassert the reset on the module
		*(volatile u32 *)(io_ccu_base + 0x2C4) |= (1 << 4); // BUS_SOFT_RST_REG1 =| LCD_RST
		// unmap the ccu registers
		iounmap(io_ccu_base);
	}

	/*restore all the tcon reg*/
	for(i=0; i<(TCON_REG_LENGTH); i++){
		iowrite32(ptcon_state->tcon_reg_back[i], io_pio_base + i*0x04);
	}

	iounmap(io_pio_base);
	return 0;
}

__s32 mem_debe_save(struct debe_state *pdebe_state)
{
	int i=0;
	void __iomem *io_debe_base = ioremap_nocache(SUNXI_DEBE_BASE, DEBE_REG_LENGTH * 4);

	/*save all the debe reg*/
	for(i=0x200; i<(DEBE_REG_LENGTH); i++){
		pdebe_state->debe_reg_back[i] = ioread32(io_debe_base + i*0x04);
	}
	iounmap(io_debe_base);
	return 0;
}

__s32 mem_debe_restore(struct debe_state *pdebe_state)
{
	int i=0;
	void __iomem *io_debe_base;

	// Setup the clocks, gating, and resets for the module before attempting to restore registers
	{
		// iomap the ccu registers
		void __iomem *io_ccu_base = ioremap_nocache(0x01c20000, 0x1000);
		// In order to restore the registers to a module, we need to setup and enable the clock to the module
		*(volatile u32 *)(io_ccu_base + 0x104) = (1 << 31) | (2 << 24) | (0xF << 0);	// BE_CLK_REG = SCLK_GATING, PLL_PERIPH_X2, /16
		// Enable the bus clock gating register for the module
		*(volatile u32 *)(io_ccu_base + 0x064) |= (1 << 12); // BUS_CLK_GATING_REG1 =| BE_GATING
		// Deassert the reset on the module
		*(volatile u32 *)(io_ccu_base + 0x2C4) |= (1 << 12); // BUS_SOFT_RST_REG1 =| BE_RST
		// unmap the ccu registers
		iounmap(io_ccu_base);
	}
	
	/*restore all the debe reg*/
	io_debe_base = ioremap_nocache(SUNXI_DEBE_BASE, DEBE_REG_LENGTH * 4);
	for(i=0x200; i<(DEBE_REG_LENGTH); i++){
		iowrite32(pdebe_state->debe_reg_back[i], io_debe_base + i*0x04);
	}
	iounmap(io_debe_base);
	return 0;
}

__s32 mem_defe_save(struct defe_state *pdefe_state)
{
	int i=0;
	void __iomem *io_pio_base = ioremap_nocache(SUNXI_DEFE_BASE, DEFE_REG_LENGTH * 4);

	/*save all the defe reg*/
	for(i=0; i<(DEFE_REG_LENGTH); i++){
		pdefe_state->defe_reg_back[i] = ioread32(io_pio_base + i*0x04);
	}
	iounmap(io_pio_base);
	return 0;
}

__s32 mem_defe_restore(struct defe_state *pdefe_state)
{
	int i=0;
	void __iomem *io_defe_base;

	// Setup the clocks, gating, and resets for the module before attempting to restore registers
	{
		// iomap the ccu registers
		void __iomem *io_ccu_base = ioremap_nocache(0x01c20000, 0x1000);
		// In order to restore the registers to a module, we need to setup and enable the clock to the module
		*(volatile u32 *)(io_ccu_base + 0x10C) = (1 << 31) | (2 << 24) | (0xF << 0);	// FE_CLK_REG = SCLK_GATING, PLL_PERIPH_X2, /16
		// Enable the bus clock gating register for the module
		*(volatile u32 *)(io_ccu_base + 0x064) |= (1 << 14); // BUS_CLK_GATING_REG1 =| FE_GATING
		// Deassert the reset on the module
		*(volatile u32 *)(io_ccu_base + 0x2C4) |= (1 << 14); // BUS_SOFT_RST_REG1 =| FE_RST
		// unmap the ccu registers
		iounmap(io_ccu_base);
	}

	/*restore all the defe reg*/
	io_defe_base = ioremap_nocache(SUNXI_DEFE_BASE, DEFE_REG_LENGTH * 4);
	for(i=0; i<(DEFE_REG_LENGTH); i++){
		 iowrite32(pdefe_state->defe_reg_back[i], io_defe_base + i*0x04);
	}
	iounmap(io_defe_base);
	return 0;
}
