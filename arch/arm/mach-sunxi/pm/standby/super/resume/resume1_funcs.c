#include "./../super_i.h"

static __ccmu_reg_list_t   *CmuReg;
static __ccmu_pll1_reg0000_t		CmuReg_Pll1Ctl_tmp;
static __ccmu_sysclk_ratio_reg0050_t	CmuReg_SysClkDiv_tmp;

static __u32 cpu_freq = 0;
static __u32 overhead = 0;
static __u32 backup_perf_counter_ctrl_reg = 0;
static __u32 backup_perf_counter_enable_reg = 0;

static __u32 backup_ccu_uart            = 0;
static __u32 backup_ccu_uart_reset      = 0;
static __u32 backup_gpio_uart           = 0;
static __u32 serial_inited_flag = 0;

__ccmu_reg_list_t * mem_clk_init(__u32 mmu_flag )
{
	if(1 == mmu_flag){
		CmuReg = (__ccmu_reg_list_t *)IO_ADDRESS(AW_CCM_BASE);
	}else{
		CmuReg = (__ccmu_reg_list_t *)(AW_CCM_BASE);
	}

	return CmuReg;
}

void mem_status_init_nommu(void)
{
    return  ;
}

__s32 mem_clk_set_misc(struct clk_misc_t *clk_misc)
{
	CmuReg = (__ccmu_reg_list_t *)(AW_CCM_BASE);
	
	CmuReg->PllxBias[0]	= clk_misc->pll1_bias;
	
	//pll_periph's bias do not need config?
	CmuReg->Pll1Tun		= clk_misc->pll1_tun;

#ifdef CONFIG_ARCH_SUN8IW5P1	
	CmuReg->PllVedioBias		= clk_misc->PllVedioBias			;           //0x228,  pll vedio bias reg
	CmuReg->PllVeBias		= clk_misc->PllVeBias				;           //0x22c,  pll ve    bias reg
	CmuReg->PllVedioPattern		= clk_misc->PllVedioPattern			;           //0x288,  pll vedio pattern reg
	CmuReg->PllVePattern		= clk_misc->PllVePattern			;           //0x28c,  pll ve    pattern reg	
	CmuReg->Pll3Ctl			= clk_misc->Pll3Ctl				;	    //0x10, vedio
	CmuReg->Pll4Ctl			= clk_misc->Pll4Ctl				;	    //0x18, ve
#endif

#ifdef CONFIG_ARCH_SUN8IW6P1
	CmuReg->PllxBias[6]	=	clk_misc->Pll_C1_Bias;	 
	CmuReg->PllC1Tun	=	clk_misc->PllC1Tun;	
	CmuReg->PllC1Ctl	=	clk_misc->PllC1Ctl;	
#endif	

	//config axi ratio to 1+1 = 2;
	//axi can not exceed 300M;
	CmuReg_SysClkDiv_tmp.dwval = CmuReg->SysClkDiv.dwval;
	CmuReg_SysClkDiv_tmp.bits.AXIClkDiv = 1;
	CmuReg->SysClkDiv.dwval = CmuReg_SysClkDiv_tmp.dwval;
	return 0;
}

void init_perfcounters (__u32 do_reset, __u32 enable_divider)
{
	// in general enable all counters (including cycle counter)
	__u32 value = 1;

	// peform reset:
	if (do_reset)
	{
		value |= 2;     // reset all counters to zero.
		value |= 4;     // reset cycle counter to zero.
	}

	if (enable_divider)
		value |= 8;     // enable "by 64" divider for CCNT.

	value |= 16;

	// program the performance-counter control-register:
	asm volatile ("mcr p15, 0, %0, c9, c12, 0" : : "r"(value));

	// enable all counters:
	value = 0x8000000f;
	asm volatile ("mcr p15, 0, %0, c9, c12, 1" : : "r"(value));

	// clear overflows:
	asm volatile ("MCR p15, 0, %0, c9, c12, 3" : : "r"(value));

	return;
}

void change_runtime_env(void)
{
	__u32 start = 0;

	//init counters:
	//init_perfcounters (1, 0);
	// measure the counting overhead:
	start = get_cyclecount();
	overhead = get_cyclecount() - start;
	//busy_waiting();
	cpu_freq = mem_clk_get_cpu_freq();	
}

__u32 mem_clk_get_cpu_freq(void)
{
	__u32 FactorN = 1;
	__u32 FactorK = 1;
	__u32 FactorM = 1;
	__u32 FactorP = 1;
	__u32 reg_val  = 0;
	__u32 cpu_freq = 0;
	
	CmuReg_SysClkDiv_tmp.dwval = CmuReg->SysClkDiv.dwval;
	//get runtime freq: clk src + divider ratio
	//src selection
	reg_val = CmuReg_SysClkDiv_tmp.bits.CpuClkSrc;
	if(0 == reg_val){
	    //32khz osc
	    cpu_freq = 32;

	}else if(1 == reg_val){
	    //hosc, 24Mhz
	    cpu_freq = 24000; 			//unit is khz
	}else if(2 == reg_val || 3 == reg_val){
	    CmuReg_Pll1Ctl_tmp.dwval = CmuReg->Pll1Ctl.dwval;
	    FactorN = CmuReg_Pll1Ctl_tmp.bits.FactorN + 1;
	    FactorK = CmuReg_Pll1Ctl_tmp.bits.FactorK + 1;
	    FactorM = CmuReg_Pll1Ctl_tmp.bits.FactorM + 1;
#if defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1)
	    FactorP = 1<<(CmuReg_Pll1Ctl_tmp.bits.FactorP);
#endif
	    cpu_freq = raw_lib_udiv(24000*FactorN*FactorK, FactorP*FactorM);
	}
	//printk("cpu_freq = dec(%d). \n", cpu_freq);
	//busy_waiting();

	return cpu_freq;
}

static void set_serial_gpio(__u32 mmu_flag)
{
	__u32 port = 0;
	__u32 i = 0;
	volatile unsigned int 	*reg;
	
	// config uart gpio
	// config tx gpio
	//fpga not need care gpio config;

	if(mmu_flag){
		reg = (volatile unsigned int *)(IO_ADDRESS(AW_UART_GPIO_PA));
	}else{
		reg = (volatile unsigned int *)(AW_UART_GPIO_PA);
	}
	*reg &= ~(0x707 << (8 + port));
	for( i = 0; i < 100; i++ );
	*reg |=  (0x303 << (8 + port));

	return	;
}

static __u32 set_serial_clk(__u32 mmu_flag)
{
	__u32 			src_freq = 0;
	__u32 			p2clk;
	volatile unsigned int 	*reg;
	__ccmu_reg_list_t 	*ccu_reg;	
	__u32 port = 0;
	__u32 i = 0;

	ccu_reg = (__ccmu_reg_list_t   *)mem_get_ba();
	//check uart clk src is ok or not.
	//the uart clk src need to be pll6 & clk freq == 600M?
	//so the baudrate == p2clk/(16*div)
	switch(ccu_reg->Apb2Div.ClkSrc){
		case 0:
			src_freq = 32000;	//32k
			break;
		case 1:
			src_freq = 24000000;	//24M
			break;
		case 2:
			src_freq = 600000000;	//600M
			break;
		default:
			break;
	}

	//calculate p2clk.
	p2clk = src_freq/((ccu_reg->Apb2Div.DivM + 1) * (1<<(ccu_reg->Apb2Div.DivN)));

	/*notice:
	**	not all the p2clk is able to create the specified baudrate.
	**	unproper p2clk may result in unacceptable baudrate, just because
	**	the uartdiv is not proper and the baudrate err exceed the acceptable range. 
	*/
	if(mmu_flag){
		//backup apb2 gating;
		backup_ccu_uart = *(volatile unsigned int *)(IO_ADDRESS(AW_CCU_UART_PA));
		//backup uart reset 
		backup_ccu_uart_reset = *(volatile unsigned int *)(IO_ADDRESS(AW_CCU_UART_RESET_PA));
		//backup gpio
		backup_gpio_uart = *(volatile unsigned int *)(IO_ADDRESS(AW_UART_GPIO_PA));

		//de-assert uart reset
		reg = (volatile unsigned int *)(IO_ADDRESS(AW_CCU_UART_RESET_PA));
		*reg &= ~(1 << (16 + port));
		for( i = 0; i < 100; i++ );
		*reg |=  (1 << (16 + port));
		change_runtime_env();
		delay_us(1);	
		//config uart clk: apb2 gating.
		reg = (volatile unsigned int *)(IO_ADDRESS(AW_CCU_UART_PA));
		*reg &= ~(1 << (16 + port));
		for( i = 0; i < 100; i++ );
		*reg |=  (1 << (16 + port));

	}else{
		//de-assert uart reset
		reg = (volatile unsigned int *)(AW_CCU_UART_RESET_PA);
		*reg &= ~(1 << (16 + port));
		for( i = 0; i < 100; i++ );
		*reg |=  (1 << (16 + port));
		change_runtime_env();
		delay_us(1);	
		//config uart clk
		reg = (volatile unsigned int *)(AW_CCU_UART_PA);
		*reg &= ~(1 << (16 + port));
		for( i = 0; i < 100; i++ );
		*reg |=  (1 << (16 + port));

	}

	return p2clk;
	
}

void serial_init_nommu(void)
{
	__u32 df;
	__u32 lcr;
	__u32 p2clk;

	set_serial_gpio(0);
	p2clk = set_serial_clk(0);

	/* set baudrate */
	df = (p2clk + (SUART_BAUDRATE<<3))/(SUART_BAUDRATE<<4);
	lcr = readl(SUART_LCR_PA);
	writel(1, SUART_HALT_PA);
	writel(lcr|0x80, SUART_LCR_PA);
	writel(df>>8, SUART_DLH_PA);
	writel(df&0xff, SUART_DLL_PA);
	writel(lcr&(~0x80), SUART_LCR_PA);
	writel(0, SUART_HALT_PA);

	/* set mode, Set Lin Control Register*/
	writel(3, SUART_LCR_PA);
	/* enable fifo */
	writel(0xe1, SUART_FCR_PA);


	//set init complete flag;
	serial_inited_flag = 1;
	return	;
}

void save_mem_status_nommu(volatile __u32 val)
{
	*(volatile __u32 *)(STANDBY_STATUS_REG_PA  + 0x04) = val;
	return;
}

static struct saved_context default_copro_value = {
	/* CR0 */
	.cssr = 0x00000002,		 /* Cache Size Selection */
	/* CR1 */
#ifdef CORTEX_A8
	.cr = 0x00C52078,		/* Control */
	.acr = 0x00000002,		/* Auxiliary Control Register*/	
	.cacr = 0x00000000,		/* Coprocessor Access Control */
	.sccfgr = 0x00000000,		/* Secure Config Register*/	
	.scdbgenblr = 0x00000000,	/* Secure Debug Enable Register*/
	.nonscacctrlr= 0x00000000,	/* Nonsecure Access Control Register*/	
#elif defined(CORTEX_A9)
#elif defined(CORTEX_A7)
	.cr = 0x00C50878,		/* Control */
	.acr = 0x00006040,		/* Auxiliary Control Register: needed for smp*/	
	.cacr = 0x00000000,		/* Coprocessor Access Control */
	.sccfgr = 0x00000000,		/* Secure Config Register*/	
	.scdbgenblr = 0x00000000,	/* Secure Debug Enable Register*/
	.nonscacctrlr= 0x00000000,	/* Nonsecure Access Control Register*/	
#endif

	/* CR2 */
	.ttb_0r = 0x00000000,		/* Translation Table Base 0 */
	.ttb_1r = 0x00000000,		/* Translation Table Base 1 */
	.ttbcr = 0x00000000,		/* Translation Talbe Base Control */
	/* CR3 */
	.dacr = 0x00000000,		/* Domain Access Control */
	/* CR5 */
	.d_fsr = 0x00000000,		/* Data Fault Status */
	.i_fsr = 0x00000000,		/* Instruction Fault Status */
	.d_afsr = 0x00000000, 		/* Data Auxilirary Fault Status */
	.i_afsr = 0x00000000,		/* Instruction Auxilirary Fault Status */
	/* CR6 */
	.d_far = 0x00000000,		/* Data Fault Address */
	.i_far = 0x00000000,		/* Instruction Fault Address */
	/* CR7 */
	.par = 0x00000000,		/* Physical Address */
	/* CR9 */				/* FIXME: Are they necessary? */
	.pmcontrolr = 0x00000000,	/* Performance Monitor Control */
	.cesr = 0x00000000,		/* Count Enable Set */
	.cecr = 0x00000000,		/* Count Enable Clear */
	.ofsr = 0x00000000,		/* Overflow Flag Status */
#ifdef CORTEX_A8
	.sir = 0x00000000,		/* Software Increment */
#elif defined(CORTEX_A7)
	.sir = 0x00000000,		/* Software Increment */
#endif
	.pcsr = 0x00000000,		/* Performance Counter Selection */
	.ccr = 0x00000000,		/* Cycle Count */
	.esr = 0x00000000,		/* Event Selection */
	.pmcountr = 0x00000000,		/* Performance Monitor Count */
	.uer = 0x00000000,		/* User Enable */
	.iesr = 0x00000000,		/* Interrupt Enable Set */
	.iecr = 0x00000000,		/* Interrupt Enable Clear */
#ifdef CORTEX_A8
	.l2clr = 0x00000000,		/* L2 Cache Lockdown */
	.l2cauxctrlr = 0x00000042, 	/* L2 Cache Auxiliary Control */
#elif defined(CORTEX_A7)
#endif

	/* CR10 */
#ifdef CORTEX_A8
	.d_tlblr = 0x00000000,		/* Data TLB Lockdown Register */
	.i_tlblr = 0x00000000,		/* Instruction TLB Lockdown Register */
#elif defined(CORTEX_A7)
#endif
	.prrr = 0x000A81A4,		/* Primary Region Remap Register: tex[0], c, b = 010, + A => normal memory 
					*                        tex[0], c, b = 100 || 101, + 1 => strong order or device
					*/
	.nrrr = 0x44E048E0,		/* Normal Memory Remap Register */
	//.prrr = 0xFF0A81A8,		/* Primary Region Remap Register */
	//.nrrr = 0x40E040E0,		/* Normal Memory Remap Register */
	/* CR11 */
#ifdef CORTEX_A8
	.pleuar = 0x00000000,		/* PLE User Accessibility */
	.plecnr = 0x00000000,		/* PLE Channel Number */
	.plecr = 0x00000000,		/* PLE Control */
	.pleisar = 0x00000000,		/* PLE Internal Start Address */
	.pleiear = 0x00000000,		/* PLE Internal End Address */
	.plecidr = 0x00000000,		/* PLE Context ID */
#elif defined(CORTEX_A7)
#endif	

	/* CR12 */
#ifdef CORTEX_A8
	.snsvbar = 0x00000000,		/* Secure or Nonsecure Vector Base Address */
	.monvecbar = 0x00000000,	/*Monitor Vector Base*/
#elif defined(CORTEX_A9)
#elif defined(CORTEX_A7)

#endif	

	/* CR13 */
	.fcse = 0x00000000,		/* FCSE PID */
	.cid = 0x00000000,		/* Context ID */
	.urwtpid = 0x00000000,		/* User read/write Thread and Process ID */
	.urotpid = 0x00000000,		/* User read-only Thread and Process ID */
	.potpid = 0x00000000,		/* Privileged only Thread and Process ID */
	
};

void set_copro_default(void)
{
	struct saved_context *ctxt = &default_copro_value;
	/* CR0 */
	asm volatile ("mcr p15, 2, %0, c0, c0, 0" : : "r"(ctxt->cssr));
	/* CR1 */
#ifdef CORTEX_A8
	//busy_waiting();
	asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r"(ctxt->cr)); //will effect visible addr space
	asm volatile ("mcr p15, 0, %0, c1, c0, 1" : : "r"(ctxt->acr)); //?
	asm volatile ("mcr p15, 0, %0, c1, c0, 2" : : "r"(ctxt->cacr));
	asm volatile ("mcr p15, 0, %0, c1, c1, 0" : : "r"(ctxt->sccfgr)); //?
	asm volatile ("mcr p15, 0, %0, c1, c1, 1" : : "r"(ctxt->scdbgenblr)); //?
	asm volatile ("mcr p15, 0, %0, c1, c1, 2" : : "r"(ctxt->nonscacctrlr)); //?
#elif defined(CORTEX_A7)
	asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r"(ctxt->cr)); //will effect visible addr space
	asm volatile ("mcr p15, 0, %0, c1, c0, 1" : : "r"(ctxt->acr)); //?
	asm volatile ("mcr p15, 0, %0, c1, c0, 2" : : "r"(ctxt->cacr));
	asm volatile ("mcr p15, 0, %0, c1, c1, 0" : : "r"(ctxt->sccfgr)); //?
	asm volatile ("mcr p15, 0, %0, c1, c1, 1" : : "r"(ctxt->scdbgenblr)); //?
	asm volatile ("mcr p15, 0, %0, c1, c1, 2" : : "r"(ctxt->nonscacctrlr)); //?
#endif	
	
	/* CR2 */
	asm volatile ("mcr p15, 0, %0, c2, c0, 0" : : "r"(ctxt->ttb_0r));
	//flush_tlb_all();
	asm volatile ("mcr p15, 0, %0, c2, c0, 1" : : "r"(ctxt->ttb_1r));
	asm volatile ("mcr p15, 0, %0, c2, c0, 2" : : "r"(ctxt->ttbcr));
	/* CR3 */
	asm volatile ("mcr p15, 0, %0, c3, c0, 0" : : "r"(ctxt->dacr));
	/* CR5 */
	asm volatile ("mcr p15, 0, %0, c5, c0, 0" : : "r"(ctxt->d_fsr));
	asm volatile ("mcr p15, 0, %0, c5, c0, 1" : : "r"(ctxt->i_fsr));
	asm volatile ("mcr p15, 0, %0, c5, c1, 0" : : "r"(ctxt->d_afsr));
	asm volatile ("mcr p15, 0, %0, c5, c1, 1" : : "r"(ctxt->i_afsr));
	/* CR6 */
	asm volatile ("mcr p15, 0, %0, c6, c0, 0" : : "r"(ctxt->d_far));
	asm volatile ("mcr p15, 0, %0, c6, c0, 2" : : "r"(ctxt->i_far));
	/* CR7 */
	asm volatile ("mcr p15, 0, %0, c7, c4, 0" : : "r"(ctxt->par));
	
	/* CR9 */
	asm volatile ("mcr p15, 0, %0, c9, c14, 0" : : "r"(ctxt->uer));
	asm volatile ("mcr p15, 0, %0, c9, c14, 1" : : "r"(ctxt->iesr));
	asm volatile ("mcr p15, 0, %0, c9, c14, 2" : : "r"(ctxt->iecr));
#ifdef CORTEX_A8
	asm volatile ("mcr p15, 1, %0, c9, c0, 0" : : "r"(ctxt->l2clr));
	asm volatile ("mcr p15, 1, %0, c9, c0, 2" : : "r"(ctxt->l2cauxctrlr)); //?
#elif defined(CORTEX_A7)
#endif

	/* CR10 */
#ifdef CORTEX_A8
	asm volatile ("mcr p15, 0, %0, c10, c0, 0" : : "r"(ctxt->d_tlblr));
	asm volatile ("mcr p15, 0, %0, c10, c0, 1" : : "r"(ctxt->i_tlblr));
#elif defined(CORTEX_A7)
#endif
	asm volatile ("mcr p15, 0, %0, c10, c2, 0" : : "r"(ctxt->prrr));
	asm volatile ("mcr p15, 0, %0, c10, c2, 1" : : "r"(ctxt->nrrr));
		
	/* CR11 */
#ifdef CORTEX_A8
	asm volatile ("mcr p15, 0, %0, c11, c1, 0" : : "r"(ctxt->pleuar));
	asm volatile ("mcr p15, 0, %0, c11, c2, 0" : : "r"(ctxt->plecnr));
	asm volatile ("mcr p15, 0, %0, c11, c4, 0" : : "r"(ctxt->plecr));
	asm volatile ("mcr p15, 0, %0, c11, c5, 0" : : "r"(ctxt->pleisar));
	asm volatile ("mcr p15, 0, %0, c11, c7, 0" : : "r"(ctxt->pleiear));
	asm volatile ("mcr p15, 0, %0, c11, c15, 0" : : "r"(ctxt->plecidr));
#elif defined(CORTEX_A7)
#endif

	/* CR12 */
#ifdef CORTEX_A8
	asm volatile ("mcr p15, 0, %0, c12, c0, 0" : : "r"(ctxt->snsvbar));
	asm volatile ("mcr p15, 0, %0, c12, c0, 1" : : "r"(ctxt->monvecbar)); //?
#elif defined(CORTEX_A7)
//	asm volatile ("mcr p15, 0, %0, c12, c0, 0" : : "r"(ctxt->vbar));
// I COMMENTED OUT THE MVBAR LINE BECAUSE WE ALREADY SET IT UP	
//	asm volatile ("mcr p15, 0, %0, c12, c0, 1" : : "r"(ctxt->mvbar));
	//asm volatile ("mcr p15, 0, %0, c12, c1, 0" : : "r"(ctxt->isr));
#endif

	/* CR13 */
	//asm volatile ("mcr p15, 0, %0, c13, c0, 0" : : "r"(ctxt->fcse));
	asm volatile ("mcr p15, 0, %0, c13, c0, 1" : : "r"(ctxt->cid));
	asm volatile ("mcr p15, 0, %0, c13, c0, 2" : : "r"(ctxt->urwtpid));
	asm volatile ("mcr p15, 0, %0, c13, c0, 3" : : "r"(ctxt->urotpid));
	asm volatile ("mcr p15, 0, %0, c13, c0, 4" : : "r"(ctxt->potpid));

	asm volatile ("dsb");
	asm volatile ("isb");
	
	return;
}


void set_copro_default_nonsec(void)
{
	struct saved_context *ctxt = &default_copro_value;
	/* CR0 */
	asm volatile ("mcr p15, 2, %0, c0, c0, 0" : : "r"(ctxt->cssr));
	/* CR1 */
#ifdef CORTEX_A8
	//busy_waiting();
	asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r"(ctxt->cr)); //will effect visible addr space
	asm volatile ("mcr p15, 0, %0, c1, c0, 1" : : "r"(ctxt->acr)); //?
	asm volatile ("mcr p15, 0, %0, c1, c0, 2" : : "r"(ctxt->cacr));
	asm volatile ("mcr p15, 0, %0, c1, c1, 0" : : "r"(ctxt->sccfgr)); //?
	asm volatile ("mcr p15, 0, %0, c1, c1, 1" : : "r"(ctxt->scdbgenblr)); //?
	asm volatile ("mcr p15, 0, %0, c1, c1, 2" : : "r"(ctxt->nonscacctrlr)); //?
#elif defined(CORTEX_A7)
	asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r"(ctxt->cr)); //will effect visible addr space
	asm volatile ("mcr p15, 0, %0, c1, c0, 1" : : "r"(ctxt->acr)); //?
	asm volatile ("mcr p15, 0, %0, c1, c0, 2" : : "r"(ctxt->cacr));
//	asm volatile ("mcr p15, 0, %0, c1, c1, 0" : : "r"(ctxt->sccfgr)); //?
//	asm volatile ("mcr p15, 0, %0, c1, c1, 1" : : "r"(ctxt->scdbgenblr)); //?
//	asm volatile ("mcr p15, 0, %0, c1, c1, 2" : : "r"(ctxt->nonscacctrlr)); //?
#endif	
	
	/* CR2 */
	asm volatile ("mcr p15, 0, %0, c2, c0, 0" : : "r"(ctxt->ttb_0r));
	//flush_tlb_all();
	asm volatile ("mcr p15, 0, %0, c2, c0, 1" : : "r"(ctxt->ttb_1r));
	asm volatile ("mcr p15, 0, %0, c2, c0, 2" : : "r"(ctxt->ttbcr));
	/* CR3 */
	asm volatile ("mcr p15, 0, %0, c3, c0, 0" : : "r"(ctxt->dacr));
	/* CR5 */
	asm volatile ("mcr p15, 0, %0, c5, c0, 0" : : "r"(ctxt->d_fsr));
	asm volatile ("mcr p15, 0, %0, c5, c0, 1" : : "r"(ctxt->i_fsr));
	asm volatile ("mcr p15, 0, %0, c5, c1, 0" : : "r"(ctxt->d_afsr));
	asm volatile ("mcr p15, 0, %0, c5, c1, 1" : : "r"(ctxt->i_afsr));
	/* CR6 */
	asm volatile ("mcr p15, 0, %0, c6, c0, 0" : : "r"(ctxt->d_far));
	asm volatile ("mcr p15, 0, %0, c6, c0, 2" : : "r"(ctxt->i_far));
	/* CR7 */
	asm volatile ("mcr p15, 0, %0, c7, c4, 0" : : "r"(ctxt->par));
	
	/* CR9 */
	asm volatile ("mcr p15, 0, %0, c9, c14, 0" : : "r"(ctxt->uer));
	asm volatile ("mcr p15, 0, %0, c9, c14, 1" : : "r"(ctxt->iesr));
	asm volatile ("mcr p15, 0, %0, c9, c14, 2" : : "r"(ctxt->iecr));
#ifdef CORTEX_A8
	asm volatile ("mcr p15, 1, %0, c9, c0, 0" : : "r"(ctxt->l2clr));
	asm volatile ("mcr p15, 1, %0, c9, c0, 2" : : "r"(ctxt->l2cauxctrlr)); //?
#elif defined(CORTEX_A7)
#endif

	/* CR10 */
#ifdef CORTEX_A8
	asm volatile ("mcr p15, 0, %0, c10, c0, 0" : : "r"(ctxt->d_tlblr));
	asm volatile ("mcr p15, 0, %0, c10, c0, 1" : : "r"(ctxt->i_tlblr));
#elif defined(CORTEX_A7)
#endif
	asm volatile ("mcr p15, 0, %0, c10, c2, 0" : : "r"(ctxt->prrr));
	asm volatile ("mcr p15, 0, %0, c10, c2, 1" : : "r"(ctxt->nrrr));
		
	/* CR11 */
#ifdef CORTEX_A8
	asm volatile ("mcr p15, 0, %0, c11, c1, 0" : : "r"(ctxt->pleuar));
	asm volatile ("mcr p15, 0, %0, c11, c2, 0" : : "r"(ctxt->plecnr));
	asm volatile ("mcr p15, 0, %0, c11, c4, 0" : : "r"(ctxt->plecr));
	asm volatile ("mcr p15, 0, %0, c11, c5, 0" : : "r"(ctxt->pleisar));
	asm volatile ("mcr p15, 0, %0, c11, c7, 0" : : "r"(ctxt->pleiear));
	asm volatile ("mcr p15, 0, %0, c11, c15, 0" : : "r"(ctxt->plecidr));
#elif defined(CORTEX_A7)
#endif

	/* CR12 */
#ifdef CORTEX_A8
	asm volatile ("mcr p15, 0, %0, c12, c0, 0" : : "r"(ctxt->snsvbar));
	asm volatile ("mcr p15, 0, %0, c12, c0, 1" : : "r"(ctxt->monvecbar)); //?
#elif defined(CORTEX_A7)
//	asm volatile ("mcr p15, 0, %0, c12, c0, 0" : : "r"(ctxt->vbar));
// I COMMENTED OUT THE MVBAR LINE BECAUSE WE ALREADY SET IT UP	
//	asm volatile ("mcr p15, 0, %0, c12, c0, 1" : : "r"(ctxt->mvbar));
	//asm volatile ("mcr p15, 0, %0, c12, c1, 0" : : "r"(ctxt->isr));
#endif

	/* CR13 */
	//asm volatile ("mcr p15, 0, %0, c13, c0, 0" : : "r"(ctxt->fcse));
	asm volatile ("mcr p15, 0, %0, c13, c0, 1" : : "r"(ctxt->cid));
	asm volatile ("mcr p15, 0, %0, c13, c0, 2" : : "r"(ctxt->urwtpid));
	asm volatile ("mcr p15, 0, %0, c13, c0, 3" : : "r"(ctxt->urotpid));
	asm volatile ("mcr p15, 0, %0, c13, c0, 4" : : "r"(ctxt->potpid));

	asm volatile ("dsb");
	asm volatile ("isb");
	
	return;
}

__s32 mem_clk_setdiv(struct clk_div_t *clk_div)
{
	if(!clk_div){
		return -1;
	}

	CmuReg = (__ccmu_reg_list_t *)(AW_CCM_BASE);
	
	//1st: set axi ratio
	CmuReg_SysClkDiv_tmp.dwval = CmuReg->SysClkDiv.dwval;
	CmuReg_SysClkDiv_tmp.bits.AXIClkDiv = clk_div->axi_div;
	CmuReg->SysClkDiv.dwval = CmuReg_SysClkDiv_tmp.dwval;
	
	//set ahb1/apb1 clock divide ratio
	//first, config ratio; 
	*(volatile __u32 *)(&CmuReg->Ahb1Div) = (((clk_div->ahb_apb_div)&(~0x3000)) | (0x1000));
	delay_us(10);
	//sec, config src.
	*(volatile __u32 *)(&CmuReg->Ahb1Div) = (clk_div->ahb_apb_div);
	//notice: pll6 is enabled by cpus.
	//the relationship between pll6&mbus&dram?

	return 0;
}

__s32 mem_clk_set_pll_factor(struct pll_factor_t *pll_factor)
{
	CmuReg = (__ccmu_reg_list_t *)(AW_CCM_BASE);
	CmuReg_Pll1Ctl_tmp.dwval = CmuReg->Pll1Ctl.dwval;
	//set pll factor: notice: when raise freq, N must be the last to set
#if defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1)
	CmuReg_Pll1Ctl_tmp.bits.FactorP = pll_factor->FactorP;
#endif
	CmuReg_Pll1Ctl_tmp.bits.FactorM = pll_factor->FactorM;
#ifndef CONFIG_ARCH_SUN8IW6P1
	CmuReg_Pll1Ctl_tmp.bits.FactorK = pll_factor->FactorK;
#endif
	CmuReg_Pll1Ctl_tmp.bits.FactorN = pll_factor->FactorN;
	CmuReg->Pll1Ctl.dwval = CmuReg_Pll1Ctl_tmp.dwval;
	//need delay?
	//busy_waiting();
	
	return 0;
}

void restore_mmu_state(struct mmu_state *saved_mmu_state)
{
	printk_nommu("cssr    = 0x%x\n", saved_mmu_state->cssr);
	printk_nommu("cacr    = 0x%x\n", saved_mmu_state->cacr);
	printk_nommu("dacr    = 0x%x\n", saved_mmu_state->dacr);
	printk_nommu("ttbr_0r = 0x%x\n", saved_mmu_state->ttb_0r);
	printk_nommu("ttbr_1r = 0x%x\n", saved_mmu_state->ttb_1r);
	printk_nommu("ttbcr   = 0x%x\n", saved_mmu_state->ttbcr);
	printk_nommu("cr      = 0x%x\n", saved_mmu_state->cr);

	//busy_waiting();
	/* CR0 */
	asm volatile ("mcr p15, 2, %0, c0, c0, 0" : : "r"(saved_mmu_state->cssr));
	/* CR1 */
	asm volatile ("mcr p15, 0, %0, c1, c0, 2" : : "r"(saved_mmu_state->cacr));
	/* CR3 */
	asm volatile ("mcr p15, 0, %0, c3, c0, 0" : : "r"(saved_mmu_state->dacr));
	
	/* CR2 */
	/*when translate 0x0000,0000, use ttb0, while ttb0 shoudbe the same with ttb1*/
	asm volatile ("mcr p15, 0, %0, c2, c0, 0" : : "r"(saved_mmu_state->ttb_1r));
	asm volatile ("mcr p15, 0, %0, c2, c0, 1" : : "r"(saved_mmu_state->ttb_1r));
	asm volatile ("mcr p15, 0, %0, c2, c0, 2" : : "r"(saved_mmu_state->ttbcr));
	asm("b __turn_mmu_on");
	asm(".align 5");
	asm(".type __turn_mmu_on, %function");
	asm("__turn_mmu_on:");	
	asm("mov r0, r0");
	
	/* CR1 */
	/*cr: will effect visible addr space*/		
	asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r"(saved_mmu_state->cr)); 
	/*read id reg*/
	asm volatile ("mrc p15, 0, r3, c0, c0, 0" : : ); 	
	asm("mov r3, r3"); 
	asm("mov r3, r3"); 
	asm("isb");

	return;
}

void restore_mmu_state_sec(struct mmu_state *saved_mmu_state)
{

	//busy_waiting();
	/* CR0 */
//	asm volatile ("mcr p15, 2, %0, c0, c0, 0" : : "r"(saved_mmu_state->cssr));
	/* CR1 */
//	asm volatile ("mcr p15, 0, %0, c1, c0, 2" : : "r"(saved_mmu_state->cacr));
	/* CR3 */
	asm volatile ("mcr p15, 0, %0, c3, c0, 0" : : "r"(saved_mmu_state->dacr));
	
	/* CR2 */
	/*when translate 0x0000,0000, use ttb0, while ttb0 shoudbe the same with ttb1*/
	asm volatile ("mcr p15, 0, %0, c2, c0, 0" : : "r"(saved_mmu_state->ttb_0r));
	asm volatile ("mcr p15, 0, %0, c2, c0, 1" : : "r"(saved_mmu_state->ttb_1r));
	asm volatile ("mcr p15, 0, %0, c2, c0, 2" : : "r"(saved_mmu_state->ttbcr));
/*	
	asm("b __turn_mmu_on");
	asm(".align 5");
	asm(".type __turn_mmu_on, %function");
	asm("__turn_mmu_on:");	
	asm("mov r0, r0");
*/	
 	/* CR1 */
 	/*cr: will effect visible addr space*/		
 	asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r"(saved_mmu_state->cr)); 
 	/*read id reg*/
 	asm volatile ("mrc p15, 0, r3, c0, c0, 0" : : ); 	
 	asm("mov r3, r3"); 
 	asm("mov r3, r3"); 
	asm("isb");

	return;
}

static void serial_put_char_nommu(char c)
{
	while (!(readl(SUART_USR_PA) & 2));
	writel(c, SUART_THR_PA);

	return	;
}

__s32 serial_puts_nommu(const char *string)
{
	//ASSERT(string != NULL);
	
	if(0 == serial_inited_flag){
	    return FAIL;
	}

	while(*string != '\0')
	{
		if(*string == '\n')
		{
			// if current character is '\n', 
			// insert output with '\r'.
			serial_put_char_nommu('\r');
		}
		serial_put_char_nommu(*string++);
	}
	
	return OK;
}

void delay_us(__u32 us)
{
	__u32 us_cnt = 0;
	__u32 cur = 0;
	__u32 target = 0;
	__u32 counter_enable = 0;
	//__u32 cnt = 0;


	if(cpu_freq > 1000){
		us_cnt = ((raw_lib_udiv(cpu_freq, 1000)) + 1)*us;
	}else{
		//32 <--> 32k, 1cycle = 1s/32k =32us 
	    goto out;
	}
	
	cur = get_cyclecount();
	target = cur - overhead + us_cnt;

	//judge whether counter is enable
	asm volatile ("mrc p15, 0, %0, c9, c12, 1" :"=r"(counter_enable));
	if( 0x8000000f != (counter_enable&0x8000000f) ){
	    init_perfcounters (1, 0);
	}
#if 1
	while(!counter_after_eq(cur, target)){
		cur = get_cyclecount();
		//cnt++;
	}
#endif
	

#if 0
	__s32 s_cur = 0;
	__s32 s_target = 0;
	__s32 result = 0;

	s_cur = (__s32)(cur);
	s_target = (__s32)(target);
	result = s_cur - s_target;
	if(s_cur - s_target >= 0){
		cnt++;
	}
	while((typecheck(__u32, cur) && \
			typecheck(__u32, target) && \
			((__s32)(cur) - (__s32)(target) >= 0))){
		
			s_cur = (__s32)(cur);
			s_target = (__s32)(target);
			if(s_cur - s_target >= 0){
				cnt++;				
			}
			cur = get_cyclecount();
	}
#endif
	//busy_waiting();

out:
	
	asm("dmb");
	asm("isb");
	return;
}

void delay_ms(__u32 ms)
{
	delay_us(ms*1000);
	
	return;
}

__ccmu_reg_list_t * mem_get_ba(void)
{

	return CmuReg;
}

static void serial_put_char(char c)
{
	while (!(readl(SUART_USR) & 2));
	writel(c, SUART_THR);

	return	;
}

__s32 serial_puts(const char *string)
{
	//ASSERT(string != NULL);
	
	if(0 == serial_inited_flag){
	    return FAIL;
	}

	while(*string != '\0')
	{
		if(*string == '\n')
		{
			// if current character is '\n', 
			// insert output with '\r'.
			serial_put_char('\r');
		}
		serial_put_char(*string++);
	}
	
	return OK;
}

__u32 get_cyclecount (void)
{
	__u32 value;
	// Read CCNT Register
	asm volatile ("MRC p15, 0, %0, c9, c13, 0\t\n": "=r"(value));  
	return value;
}

