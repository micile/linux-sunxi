/***********************************************************************************************************************
;                             Copyright(C), 2012-2015, Newbie Microelectronic Co., Ltd.
;                                                  All Rights Reserved
;
; File Name : resume1.S
;
; Author : yanggq
;
; Version : 1.1.0
;
; Date : 2012.7.11
;
; Description :
;
; Functions list : none.
;
; Others : None at present.
;
;
; History :
;
;  <Author>        <time>       <version>      <description>
;
; gq.Yang       2012.7.11      1.1.0        build the file
;
***********************************************************************************************************************/
#include "./../../../pm_assembler_i.h"

#include <linux/linkage.h>

.arch_extension sec

#define  ARMV7_USR_MODE        0x10
#define  ARMV7_FIQ_MODE        0x11
#define  ARMV7_IRQ_MODE        0x12
#define  ARMV7_SVC_MODE        0x13
#define  ARMV7_MON_MODE        0x16
#define  ARMV7_ABT_MODE        0x17
#define  ARMV7_UND_MODE        0x1b
#define  ARMV7_SYSTEM_MODE     0x1f
#define  ARMV7_MODE_MASK       0x1f
#define  ARMV7_FIQ_MASK        0x40
#define  ARMV7_IRQ_MASK        0x80


#define   ARMV7_THUMB_MASK    (1<<5)
#define   ARMV7_END_MASK      (1<<9)

#define   ARMV7_IT_MASK       ((0x3f<<10)|(0x03<<25))
#define   ARMV7_GE_MASK       (0x0f<<16)
#define   ARMV7_JAVA_MASK     (1<<24)

#define   ARMV7_QFLAG_BIT     (1 << 27)
#define   ARMV7_CC_V_BIT      (1 << 28)
#define   ARMV7_CC_C_BIT      (1 << 29)
#define   ARMV7_CC_Z_BIT      (1 << 30)
#define   ARMV7_CC_N_BIT      (1 << 31)
#define   ARMV7_CC_E_BIT      (1 << 9)

#define   ARMV7_C1_M_BIT         ( 1 << 0 )
#define   ARMV7_C1_A_BIT         ( 1 << 1 )
#define   ARMV7_C1_C_BIT         ( 1 << 2 )
#define   ARMV7_C1_Z_BIT         ( 1 << 11 )
#define   ARMV7_C1_I_BIT         ( 1 << 12 )

	.extern resume1_c_part
	.text
	.arm
/*********************************the begin of initializing system********************************/
	.globl _start
_start:
	mrs r0, cpsr
	bic r0, r0, #ARMV7_MODE_MASK
	orr r0, r0, #ARMV7_SVC_MODE
	orr r0, r0, #( ARMV7_IRQ_MASK | ARMV7_FIQ_MASK )    ;// After reset, ARM automaticly disables IRQ and FIQ, and runs in SVC mode.
	bic r0, r0, #ARMV7_CC_E_BIT                         ;// set little-endian
	msr cpsr_c, r0

	//config smp
	
	// configure memory system : disable MMU,cache and write buffer; set little_endian;
	mrc p15, 0, r0, c1, c0
	bic r0, r0, #( ARMV7_C1_M_BIT | ARMV7_C1_C_BIT )  ;// disable MMU, data cache
	bic r0, r0, #( ARMV7_C1_I_BIT | ARMV7_C1_Z_BIT )  ;// disable instruction cache, disable flow prediction
	bic r0, r0, #( ARMV7_C1_A_BIT)                    ;// disable align
	mcr p15, 0, r0, c1, c0

	dsb
	isb

#if defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1)
	ldr r0, =0x01c00024
	ldr r1, [r0]
	// 1 -> bit15
	orr r1, #0x8000
	str r1, [r0]
	// read version
	ldr r1, [r0]
	lsr r2, r1, #16
	ldr r3, =0x1661
	cmp r2, r3
	beq config_61

	//setting to 0x22223a22
config_50:
	ldr r0, =0x01c00044
        ldr r1,[r0]
	orr r1,#0x1800
	str r1,[r0]
	b   config_end

	//setting to 0x222222e2
config_61:
	ldr r0, =0x01c00044
        ldr r1,[r0]
	orr r1,#0xc0
	str r1,[r0]

config_end:
#endif

	// set SP for SVC mode	
	mrc p15, 0, r0, c0, c0, 5
	//each core have seperate 512k bytes stack.
	lsl	r0, r0, #9
	add	sp, r0, #(SP_IN_SRAM_START)

/* ================================================== 	*/
/*		ENABLE JTAG, GPIO PE4 AS OUTPUT		*/
/* ================================================== 	*/
// enable the jtag
	ldr r0, =0x01c208b4
	movw r1,#0x3733
	movt r1,#0x37
	str r1, [r0]

/* [0x01c20890] = 0x77717777 */
/* [0x01c208A0] = 0x00000010 */
// // enable the debug gpio
// 	ldr r0, =0x1c20890
// 	movw r1,#0x7777
// 	movt r1,#0x7771
// 	str r1, [r0]
//
// // make the gpio go low
// 	ldr r0, =0x01c208A0
// 	movw r1,#0x0000
// 	movt r1,#0x0000
// 	str r1, [r0]
//
// // make the gpio go high
// 	ldr r0, =0x1c208a0
// 	movw r1,#0x0010
// 	movt r1,#0x0000
// 	str r1, [r0]

// jtag_wait_loop:
// 	nop
// 	b jtag_wait_loop

// load up the new monitor vector table
	ldr	r5, =_monitor_setup_vectors
	mcr	p15, 0, r5, c12, c0, 1
	isb

// use smc instruction enter secure monitor exception
	ldr	r0, =_finish
	mov	ip, r0
	mov	r0, r1
	mov	r1, r2
	mov	r2, r3
	smc	#0

/* ================================================== 	*/
/*		EXCEPTION VECTOR AND HANDLER		*/
/* ================================================== 	*/
.align	5
_monitor_setup_vectors:
	.word 0					// reset
	.word 0					// undefined instruction
	adr pc, _mon_setup_smc_exception	// smc / svc
	.word 0					// prefetech
	.word 0					// data
	.word 0					// not used
	.word 0					// irq
	.word 0					// fiq

_mon_setup_smc_exception:
	ldr sp, =secure_stack_area_end
	push {ip}
	bl exception_handler_mon_setup_smc
	pop {ip}

	mrc	p15, 0, r5, c1, c1, 0		@ read SCR
	bic	r5, r5, #0x4a			@ clear IRQ, EA, nET bits
	orr	r5, r5, #0x31			@ enable NS, AW, FW bits
//	orr	r5, r5, #0x30			@ uncomment to test in secure mode
						@ FIQ preserved for secure mode
	mov	r6, #ARMV7_SVC_MODE		@ default mode is SVC
	mcr	p15, 0, r5, c1, c1, 0		@ write SCR (with NS bit set)
	isb

	mov	lr, ip
	movs	pc, lr				@ ERET to non-secure

/* ================================================== 	*/
/*		SECURE STACK				*/
/* ================================================== 	*/
.balign 4
secure_stack_area_start:
	.skip 396
secure_stack_area_end:
	.skip 4

_finish:
/**********************************the end of initializing system*********************************/
	bl set_pll
	bl  resume1_c_part

 	b .                                 ;// infinite loop

	.end
