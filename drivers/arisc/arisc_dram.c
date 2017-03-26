/*
 *  arch/arm/mach-sun6i/arisc/arisc_dram.c
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

#include "arisc_i.h"
//#include <mach/sys_config.h>
#include <asm/cacheflush.h>

typedef struct __DRAM_PARA
{
	//normal configuration
	unsigned int dram_clk;
	unsigned int dram_type; //dram_type DDR2: 2 DDR3: 3 LPDDR2: 6 DDR3L: 31
	unsigned int dram_zq;
	unsigned int dram_odt_en;

	//control configuration
	unsigned int dram_para1;
	unsigned int dram_para2;

	//timing configuration
	unsigned int dram_mr0;
	unsigned int dram_mr1;
	unsigned int dram_mr2;
	unsigned int dram_mr3;
	unsigned int dram_tpr0;
	unsigned int dram_tpr1;
	unsigned int dram_tpr2;
	unsigned int dram_tpr3;
	unsigned int dram_tpr4;
	unsigned int dram_tpr5;
	unsigned int dram_tpr6;

	//reserved for future use
	unsigned int dram_tpr7;
	unsigned int dram_tpr8;
	unsigned int dram_tpr9;
	unsigned int dram_tpr10;
	unsigned int dram_tpr11;
	unsigned int dram_tpr12;
	unsigned int dram_tpr13;

}__dram_para_t;

static __dram_para_t arisc_dram_paras;

static int arisc_read_dt_node(const struct device_node *np, const char *name)
{
	void *ptr;

	ptr = (void*)of_get_property(np, name, NULL);
	if (ptr) return be32_to_cpup((const void*)ptr);
	printk("**** Error!  Could not read dts node %s!  This will cause suspend to hang on resume!\n", name);
	return 0;
}


static int arisc_get_dram_cfg(const struct device_node *np)
{
// 	script_item_u val;
// 	script_item_value_type_e type;
// 
// 	type = script_get_item("dram_para", "dram_clk", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_clk is %#x\n", val.val);
// 	arisc_dram_paras.dram_clk = val.val;
// 
// 	type = script_get_item("dram_para", "dram_type", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_type is %#x\n", val.val);
// 	arisc_dram_paras.dram_type = val.val;
// 
// 	type = script_get_item("dram_para", "dram_zq", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_zq is %#x\n", val.val);
// 	arisc_dram_paras.dram_zq = val.val;
// 
// 	type = script_get_item("dram_para", "dram_odt_en", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_odt_en is %#x\n", val.val);
// 	arisc_dram_paras.dram_odt_en = val.val;
// 
// 	type = script_get_item("dram_para", "dram_para1", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_para1 is %#x\n", val.val);
// 	arisc_dram_paras.dram_para1 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_para2", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_para2 is %#x\n", val.val);
// 	arisc_dram_paras.dram_para2 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_mr0", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_mr0 is %#x\n", val.val);
// 	arisc_dram_paras.dram_mr0 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_mr1", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_mr1 is %#x\n", val.val);
// 	arisc_dram_paras.dram_mr1 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_mr2", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_mr2 is %#x\n", val.val);
// 	arisc_dram_paras.dram_mr2 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_mr3", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_mr3 is %#x\n", val.val);
// 	arisc_dram_paras.dram_mr3 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_tpr0", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_tpr0 is %#x\n", val.val);
// 	arisc_dram_paras.dram_tpr0 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_tpr1", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_tpr1 is %#x\n", val.val);
// 	arisc_dram_paras.dram_tpr1 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_tpr2", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_tpr2 is %#x\n", val.val);
// 	arisc_dram_paras.dram_tpr2 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_tpr3", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_tpr3 is %#x\n", val.val);
// 	arisc_dram_paras.dram_tpr3 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_tpr4", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_tpr4 is %#x\n", val.val);
// 	arisc_dram_paras.dram_tpr4 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_tpr5", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_tpr5 is %#x\n", val.val);
// 	arisc_dram_paras.dram_tpr5 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_tpr6", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_tpr6 is %#x\n", val.val);
// 	arisc_dram_paras.dram_tpr6 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_tpr7", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_tpr7 is %#x\n", val.val);
// 	arisc_dram_paras.dram_tpr7 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_tpr8", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_tpr8 is %#x\n", val.val);
// 	arisc_dram_paras.dram_tpr8 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_tpr9", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_tpr9 is %#x\n", val.val);
// 	arisc_dram_paras.dram_tpr9 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_tpr10", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_tpr10 is %#x\n", val.val);
// 	arisc_dram_paras.dram_tpr10 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_tpr11", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_tpr11 is %#x\n", val.val);
// 	arisc_dram_paras.dram_tpr11 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_tpr12", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_tpr12 is %#x\n", val.val);
// 	arisc_dram_paras.dram_tpr12 = val.val;
// 
// 	type = script_get_item("dram_para", "dram_tpr13", &val);
// 	if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
// 	{
// 		ARISC_ERR("dram para type err %d!", __LINE__);
// 	}
// 	ARISC_INF("dram_tpr13 is %#x\n", val.val);
// 	arisc_dram_paras.dram_tpr13 = val.val;




	arisc_dram_paras.dram_clk    = arisc_read_dt_node(np, "arisc_dram_paras,dram_clk");
	arisc_dram_paras.dram_type   = arisc_read_dt_node(np, "arisc_dram_paras,dram_type");
	arisc_dram_paras.dram_zq     = arisc_read_dt_node(np, "arisc_dram_paras,dram_zq");
	arisc_dram_paras.dram_odt_en = arisc_read_dt_node(np, "arisc_dram_paras,dram_odt_en");
	arisc_dram_paras.dram_para1  = arisc_read_dt_node(np, "arisc_dram_paras,dram_para1");
	arisc_dram_paras.dram_para2  = arisc_read_dt_node(np, "arisc_dram_paras,dram_para2");
	arisc_dram_paras.dram_mr0    = arisc_read_dt_node(np, "arisc_dram_paras,dram_mr0");
	arisc_dram_paras.dram_mr1    = arisc_read_dt_node(np, "arisc_dram_paras,dram_mr1");
	arisc_dram_paras.dram_mr2    = arisc_read_dt_node(np, "arisc_dram_paras,dram_mr2");
	arisc_dram_paras.dram_mr3    = arisc_read_dt_node(np, "arisc_dram_paras,dram_mr3");
	arisc_dram_paras.dram_tpr0   = arisc_read_dt_node(np, "arisc_dram_paras,dram_tpr0");
	arisc_dram_paras.dram_tpr1   = arisc_read_dt_node(np, "arisc_dram_paras,dram_tpr1");
	arisc_dram_paras.dram_tpr2   = arisc_read_dt_node(np, "arisc_dram_paras,dram_tpr2");
	arisc_dram_paras.dram_tpr3   = arisc_read_dt_node(np, "arisc_dram_paras,dram_tpr3");
	arisc_dram_paras.dram_tpr4   = arisc_read_dt_node(np, "arisc_dram_paras,dram_tpr4");
	arisc_dram_paras.dram_tpr5   = arisc_read_dt_node(np, "arisc_dram_paras,dram_tpr5");
	arisc_dram_paras.dram_tpr6   = arisc_read_dt_node(np, "arisc_dram_paras,dram_tpr6");
	arisc_dram_paras.dram_tpr7   = arisc_read_dt_node(np, "arisc_dram_paras,dram_tpr7");
	arisc_dram_paras.dram_tpr8   = arisc_read_dt_node(np, "arisc_dram_paras,dram_tpr8");
	arisc_dram_paras.dram_tpr9   = arisc_read_dt_node(np, "arisc_dram_paras,dram_tpr9");
	arisc_dram_paras.dram_tpr10  = arisc_read_dt_node(np, "arisc_dram_paras,dram_tpr10");
	arisc_dram_paras.dram_tpr11  = arisc_read_dt_node(np, "arisc_dram_paras,dram_tpr11");
	arisc_dram_paras.dram_tpr12  = arisc_read_dt_node(np, "arisc_dram_paras,dram_tpr12");
	arisc_dram_paras.dram_tpr13  = arisc_read_dt_node(np, "arisc_dram_paras,dram_tpr13");

	printk("arisc_dram_paras.clk         = 0x%x\n", arisc_dram_paras.clk);
	printk("arisc_dram_paras.dram_type   = 0x%x\n", arisc_dram_paras.dram_type);
	printk("arisc_dram_paras.dram_zq     = 0x%x\n", arisc_dram_paras.dram_zq);
	printk("arisc_dram_paras.dram_odt_en = 0x%x\n", arisc_dram_paras.dram_odt_en);
	printk("arisc_dram_paras.dram_para1  = 0x%x\n", arisc_dram_paras.dram_para1);
	printk("arisc_dram_paras.dram_para2  = 0x%x\n", arisc_dram_paras.dram_para2);
	printk("arisc_dram_paras.dram_mr0    = 0x%x\n", arisc_dram_paras.dram_mr0);
	printk("arisc_dram_paras.dram_mr1    = 0x%x\n", arisc_dram_paras.dram_mr1);
	printk("arisc_dram_paras.dram_mr2    = 0x%x\n", arisc_dram_paras.dram_mr2);
	printk("arisc_dram_paras.dram_mr3    = 0x%x\n", arisc_dram_paras.dram_mr3);
	printk("arisc_dram_paras.dram_tpr0   = 0x%x\n", arisc_dram_paras.dram_tpr0);
	printk("arisc_dram_paras.dram_tpr1   = 0x%x\n", arisc_dram_paras.dram_tpr1);
	printk("arisc_dram_paras.dram_tpr2   = 0x%x\n", arisc_dram_paras.dram_tpr2);
	printk("arisc_dram_paras.dram_tpr3   = 0x%x\n", arisc_dram_paras.dram_tpr3);
	printk("arisc_dram_paras.dram_tpr4   = 0x%x\n", arisc_dram_paras.dram_tpr4);
	printk("arisc_dram_paras.dram_tpr5   = 0x%x\n", arisc_dram_paras.dram_tpr5);
	printk("arisc_dram_paras.dram_tpr6   = 0x%x\n", arisc_dram_paras.dram_tpr6);
	printk("arisc_dram_paras.dram_tpr7   = 0x%x\n", arisc_dram_paras.dram_tpr7);
	printk("arisc_dram_paras.dram_tpr8   = 0x%x\n", arisc_dram_paras.dram_tpr8);
	printk("arisc_dram_paras.dram_tpr9   = 0x%x\n", arisc_dram_paras.dram_tpr9);
	printk("arisc_dram_paras.dram_tpr10  = 0x%x\n", arisc_dram_paras.dram_tpr10);
	printk("arisc_dram_paras.dram_tpr11  = 0x%x\n", arisc_dram_paras.dram_tpr11);
	printk("arisc_dram_paras.dram_tpr12  = 0x%x\n", arisc_dram_paras.dram_tpr12);
	printk("arisc_dram_paras.dram_tpr13  = 0x%x\n", arisc_dram_paras.dram_tpr13);

	return 0;
}

int arisc_config_dram_paras(const struct device_node *np)
{
	struct arisc_message *pmessage;
	u32 *dram_para;
	u32 index;

	/* parse dram config paras */
	arisc_get_dram_cfg(np);

	/* update dram config paras to arisc system */
	pmessage = arisc_message_allocate(0);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	dram_para = (u32 *)(&arisc_dram_paras);
	for (index = 0; index < (sizeof(arisc_dram_paras) / 4); index++) {
		/* initialize message */
		pmessage->type       = ARISC_SET_DRAM_PARAS;
		pmessage->attr       = ARISC_MESSAGE_ATTR_HARDSYN;
		pmessage->paras[0]   = index;
		pmessage->paras[1]   = 1;
		pmessage->paras[2]   = dram_para[index];
		pmessage->state      = ARISC_MESSAGE_INITIALIZED;
		pmessage->cb.handler = NULL;
		pmessage->cb.arg     = NULL;

		/* send message */
		arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
	}
	/* free message */
	arisc_message_free(pmessage);

	return 0;
}
