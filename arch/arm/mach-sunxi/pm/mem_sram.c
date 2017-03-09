#include <asm/io.h>
#include "pm_types.h"
#include "pm_i.h"

/*
*********************************************************************************************************
*                                       MEM SRAM SAVE
*
* Description: mem sram save.
*
* Arguments  : none.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 mem_sram_save(struct sram_state *psram_state)
{
	void __iomem *io_sram_base = ioremap_nocache(AW_SRAMCTRL_BASE, SRAM_REG_LENGTH * 4);
	int i=0;

	/*save all the sram reg*/
	for(i=0; i<(SRAM_REG_LENGTH); i++){
		// psram_state->sram_reg_back[i] = *(volatile __u32 *)(IO_ADDRESS(AW_SRAMCTRL_BASE) + i*0x04);
		psram_state->sram_reg_back[i] = ioread32(io_sram_base + i*0x04);
	}
	iounmap(io_sram_base);
	return 0;
}

/*
*********************************************************************************************************
*                                       MEM sram restore
*
* Description: mem sram restore.
*
* Arguments  : none.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 mem_sram_restore(struct sram_state *psram_state)
{
	void __iomem *io_sram_base = ioremap_nocache(AW_SRAMCTRL_BASE, SRAM_REG_LENGTH * 4);
	int i=0;
	
	/*restore all the sram reg*/
	for(i=0; i<(SRAM_REG_LENGTH); i++){
		 // *(volatile __u32 *)(IO_ADDRESS(AW_SRAMCTRL_BASE) + i*0x04) = psram_state->sram_reg_back[i];
		 iowrite32(psram_state->sram_reg_back[i], io_sram_base + i*0x04);
	}

	iounmap(io_sram_base);
	return 0;
}
