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
static unsigned int read_prcm_wvalue(unsigned int addr)
{
	unsigned int reg;

	void __iomem *regs_prcm;

 	regs_prcm = ioremap(0x01F015C0, 0x10);


	reg = readl(regs_prcm);
	reg |= (0x1 << 28);
	writel(reg, regs_prcm);

	reg = readl(regs_prcm);
	reg &= ~(0x1 << 24);
	writel(reg, regs_prcm);

	reg = readl(regs_prcm);
	reg &= ~(0x1f << 16);
	reg |= (addr << 16);
	writel(reg, regs_prcm);

	reg = readl(regs_prcm);
	reg &= (0xff << 0);

	iounmap(regs_prcm);

	return reg;
}

static ssize_t codec_regs_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	void *p;
	u32 val;
	
	buf[0] = 0;
	sprintf(buf + strlen(buf), "CODEC CCU Registers\n");
	p = ioremap(0x01C20000, 0x300);
	val = ioread32((char*)p + 0x008); sprintf(buf + strlen(buf), "PLL_AUDIO_CTRL_REG = 0x%08X\n", val);
	val = ioread32((char*)p + 0x068); sprintf(buf + strlen(buf), "BUS_CLK_GATING_REG2 = 0x%08X\n", val);
	val = ioread32((char*)p + 0x140); sprintf(buf + strlen(buf), "ADDA_DIG_CLK_REG = 0x%08X\n", val);
	val = ioread32((char*)p + 0x2D0); sprintf(buf + strlen(buf), "BUS_SOFT_RST_REG3 = 0x%08X\n", val);
	iounmap(p);
	
	p = ioremap(0x01C22C00, 0x500);
	val = ioread32((char*)p + 0x000); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "DA_CTL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x004); if (val != 0x0000000C) {  sprintf(buf + strlen(buf), "DA_FAT0 = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x008); if (val != 0x00004020) {  sprintf(buf + strlen(buf), "DA_FAT1 = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x00C); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "DA_TXFIFO = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x010); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "DA_RXFIFO = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x014); if (val != 0x000400F0) {  sprintf(buf + strlen(buf), "DA_FCTL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x018); if (val != 0x10800000) {  sprintf(buf + strlen(buf), "DA_FSTA = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x01C); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "DA_INT = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x020); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "DA_ISTA = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x024); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "DA_CLKD = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x028); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "DA_TXCNT = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x02C); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "DA_RXCNT = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x030); if (val != 0x00000001) {  sprintf(buf + strlen(buf), "DA_TXCHSEL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x034); if (val != 0x76543210) {  sprintf(buf + strlen(buf), "DA_TXCHMAP = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x038); if (val != 0x00000001) {  sprintf(buf + strlen(buf), "DA_RXCHSEL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x03C); if (val != 0x00003210) {  sprintf(buf + strlen(buf), "DA_RXCHMAP = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x200); if (val != 0x00000101) {  sprintf(buf + strlen(buf), "CHIP_AUDIO_RST = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x20C); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "SYSCLK_CTL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x210); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "MOD_CLK_ENA = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x214); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "MOD_RST_CTL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x218); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "SYS_SR_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x21C); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "SYS_SRC_CLK = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x220); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "SYS_DVC_MOD = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x240); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF1CLK_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x244); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF1_ADCDAT_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x248); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF1_DACDAT_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x24C); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF1_MXR_SRC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x250); if (val != 0x0000A0A0) {  sprintf(buf + strlen(buf), "AIF1_VOL_CTRL1 = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x254); if (val != 0x0000A0A0) {  sprintf(buf + strlen(buf), "AIF1_VOL_CTRL2 = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x258); if (val != 0x0000A0A0) {  sprintf(buf + strlen(buf), "AIF1_VOL_CTRL3 = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x25C); if (val != 0x0000A0A0) {  sprintf(buf + strlen(buf), "AIF1_VOL_CTRL4 = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x260); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF1_MXR_GAIN = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x264); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF1_RXD_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x280); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF2_CLK_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x284); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF2_ADCDAT_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x288); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF2_DACDAT_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x28C); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF2_MXR_SRC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x290); if (val != 0x0000A0A0) {  sprintf(buf + strlen(buf), "AIF2_VOL_CTRL1 = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x298); if (val != 0x0000A0A0) {  sprintf(buf + strlen(buf), "AIF2_VOL_CTRL2 = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x2A0); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF2_MXR_GAIN = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x2A4); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF2_RXD_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x2C0); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF3_CLK_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x2C4); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF3_ADCDAT_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x2C8); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF3_DACDAT_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x2CC); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF3_SGP_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x2E4); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AIF3_RXD_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x300); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "ADC_DIG_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x304); if (val != 0x0000A0A0) {  sprintf(buf + strlen(buf), "ADC_VOL_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x308); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "ADC_DBG_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x320); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "DAC_DIG_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x324); if (val != 0x0000A0A0) {  sprintf(buf + strlen(buf), "DAC_VOL_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x328); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "DAC_DBG_CTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x330); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "DAC_MXR_SRC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x334); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "DAC_MXR_GAIN = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x400); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AC_ADC_DAPLSTA = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x404); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AC_ADC_DAPRSTA = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x408); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AC_ADC_DAPLCTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x40C); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AC_ADC_DAPRCTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x410); if (val != 0x00002C28) {  sprintf(buf + strlen(buf), "AC_ADC_DAPLTL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x414); if (val != 0x00002C28) {  sprintf(buf + strlen(buf), "AC_ADC_DAPRTL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x418); if (val != 0x00000005) {  sprintf(buf + strlen(buf), "AC_ADC_DAPLHAC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x41C); if (val != 0x00001EB8) {  sprintf(buf + strlen(buf), "AC_ADC_DAPLLAC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x420); if (val != 0x00000005) {  sprintf(buf + strlen(buf), "AC_ADC_DAPRHAC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x424); if (val != 0x00001EB8) {  sprintf(buf + strlen(buf), "AC_ADC_DAPRLAC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x428); if (val != 0x0000001F) {  sprintf(buf + strlen(buf), "AC_ADC_DAPLDT = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x42C); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AC_ADC_DAPLAT = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x430); if (val != 0x0000001F) {  sprintf(buf + strlen(buf), "AC_ADC_DAPRDT = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x434); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AC_ADC_DAPRAT = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x438); if (val != 0x00001E1E) {  sprintf(buf + strlen(buf), "AC_ADC_DAPNTH = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x43C); if (val != 0x00000005) {  sprintf(buf + strlen(buf), "AC_ADC_DAPLHNAC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x440); if (val != 0x00001EB8) {  sprintf(buf + strlen(buf), "AC_ADC_DAPLLNAC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x444); if (val != 0x00000005) {  sprintf(buf + strlen(buf), "AC_ADC_DAPRHNAC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x448); if (val != 0x00001EB8) {  sprintf(buf + strlen(buf), "AC_ADC_DAPRLNAC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x44C); if (val != 0x000000FF) {  sprintf(buf + strlen(buf), "AC_DAPHHPFC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x450); if (val != 0x0000FAC1) {  sprintf(buf + strlen(buf), "AC_DAPLHPFC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x454); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AC_DAPOPT = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x480); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AC_DAC_DAPCTRL = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x484); if (val != 0x000000FF) {  sprintf(buf + strlen(buf), "AC_DAC_DAPHHPFC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x488); if (val != 0x0000FAC1) {  sprintf(buf + strlen(buf), "AC_DAC_DAPLHPFC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x48C); if (val != 0x00000100) {  sprintf(buf + strlen(buf), "AC_DAC_DAPLHAVC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x490); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AC_DAC_DAPLLAVC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x494); if (val != 0x00000100) {  sprintf(buf + strlen(buf), "AC_DAC_DAPRHAVC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x498); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AC_DAC_DAPRLAVC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x49C); if (val != 0x00000100) {  sprintf(buf + strlen(buf), "AC_DAC_DAPHGDEC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4A0); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AC_DAC_DAPLGDEC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4A4); if (val != 0x00000100) {  sprintf(buf + strlen(buf), "AC_DAC_DAPHGATC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4A8); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AC_DAC_DAPLGATC = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4AC); if (val != 0x000004FB) {  sprintf(buf + strlen(buf), "AC_DAC_DAPHETHD = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4B0); if (val != 0x00009ED0) {  sprintf(buf + strlen(buf), "AC_DAC_DAPLETHD = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4B4); if (val != 0x00000780) {  sprintf(buf + strlen(buf), "AC_DAC_DAPHGKPA = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4B8); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AC_DAC_DAPLGKPA = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4BC); if (val != 0x00000100) {  sprintf(buf + strlen(buf), "AC_DAC_DAPHGOPA = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4C0); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AC_DAC_DAPLGOPA = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4C4); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AC_DAC_DAPOPT = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4D0); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "AGC_ENA = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4D4); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "DRC_ENA = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4E0); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "SRC1_CTRL1 = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4E4); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "SRC1_CTRL2 = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4E8); if (val != 0x00000040) {  sprintf(buf + strlen(buf), "SRC1_CTRL3 = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4EC); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "SRC1_CTRL4 = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4F0); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "SRC2_CTRL1 = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4F4); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "SRC2_CTRL2 = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4F8); if (val != 0x00000040) {  sprintf(buf + strlen(buf), "SRC2_CTRL3 = 0x%08X\n", val); }
	val = ioread32((char*)p + 0x4FC); if (val != 0x00000000) {  sprintf(buf + strlen(buf), "SRC2_CTRL4 = 0x%08X\n", val); }
	iounmap(p);

	val = read_prcm_wvalue(0x00);	if (val != 0x00) { sprintf(buf + strlen(buf), "HP_VOLC = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x01);	if (val != 0x00) { sprintf(buf + strlen(buf), "LOMIXSC = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x02);	if (val != 0x00) { sprintf(buf + strlen(buf), "ROMIXSC = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x03);	if (val != 0x00) { sprintf(buf + strlen(buf), "DAC_PA_SRC = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x04);	if (val != 0x3) { sprintf(buf + strlen(buf), "PHONEIN_GCTRL = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x05);	if (val != 0x33) { sprintf(buf + strlen(buf), "LINEIN_GCTRL = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x06);	if (val != 0x33) { sprintf(buf + strlen(buf), "MICIN_GCTRL = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x07);	if (val != 0x14) { sprintf(buf + strlen(buf), "PAEN_HP_CTRL = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x08);	if (val != 0x60) { sprintf(buf + strlen(buf), "PHONEOUT_CTRL = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x09);	if (val != 0x04) { sprintf(buf + strlen(buf), "PHONEP-N_GAIN_CTR = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x0A);	if (val != 0x40) { sprintf(buf + strlen(buf), "MIC2G_LINEEN_CTRL = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x0B);	if (val != 0x14) { sprintf(buf + strlen(buf), "MIC1G_MICBIAS_CTRL = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x0C);	if (val != 0x00) { sprintf(buf + strlen(buf), "LADCMIXSC = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x0D);	if (val != 0x00) { sprintf(buf + strlen(buf), "RADCMIXSC = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x0E);	if (val != 0x00) { sprintf(buf + strlen(buf), "RESERVED = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x0F);	if (val != 0x03) { sprintf(buf + strlen(buf), "ADC_AP_EN = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x12);	if (val != 0x42) { sprintf(buf + strlen(buf), "ADDA_APT2 = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x17);	if (val != 0x20) { sprintf(buf + strlen(buf), "BIASCALI = 0x%02X\n", val); }
	val = read_prcm_wvalue(0x18);	if (val != 0x20) { sprintf(buf + strlen(buf), "BIASVERIFY = 0x%02X\n", val); }
	
	return strlen(buf);
}

static ssize_t gpio_regs_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	void *iorp;
	u32 regs[9];
	int i;
	int cfg;
	int pul;
	int drv;
	int val;
	int portnum;
	
	// ABCDEFGHIJKL
	// x0123456xxx0
	
	int pins[] = {	 0, /* (0)  A */
			 8, /* (1)  B */
			17, /* (2)  C */
			22, /* (3)  D */
			18, /* (4)  E */
		 	 6, /* (5)  F */
			14, /* (6)  G */
			10, /* (7)  H */
			 0, /* (8)  I */
			 0, /* (9)  J */
			 0, /* (10) K */
			12, /* (11) L */};

	buf[0] = 0;
	for (portnum = 0; portnum < 12; portnum++) {
		// skip if no pins
		if (pins[portnum] == 0) continue;
		
		// dump thep pins
		if (portnum < 7) {
			iorp = ioremap_nocache(0x01C20800 + portnum * 0x24, 0x30);
		} else {
			iorp = ioremap_nocache(0x01f02c00, 0x30);
		}
		for (i = 0; i < 9; i++) {
			regs[i] = ioread32(iorp + 0x04 * i);
		}
		sprintf(buf + strlen(buf), "P%c_CFG0 = 0x%08X\n", 'A' + portnum, regs[0]);
		sprintf(buf + strlen(buf), "P%c_CFG1 = 0x%08X\n", 'A' + portnum, regs[1]);
		sprintf(buf + strlen(buf), "P%c_CFG2 = 0x%08X\n", 'A' + portnum, regs[2]);
		sprintf(buf + strlen(buf), "P%c_CFG3 = 0x%08X\n", 'A' + portnum, regs[3]);
		sprintf(buf + strlen(buf), "P%c_DAT  = 0x%08X\n", 'A' + portnum, regs[4]);
		sprintf(buf + strlen(buf), "P%c_DRV0 = 0x%08X\n", 'A' + portnum, regs[5]);
		sprintf(buf + strlen(buf), "P%c_DRV1 = 0x%08X\n", 'A' + portnum, regs[6]);
		sprintf(buf + strlen(buf), "P%c_PUL0 = 0x%08X\n", 'A' + portnum, regs[7]);
		sprintf(buf + strlen(buf), "P%c_PUL1 = 0x%08X\n", 'A' + portnum, regs[8]);

		for (i = 0; i < pins[portnum]; i++) {
			cfg = (regs[0 /*REG_CFG*/ + (i / 8)] >> ((i % 8) * 4)) & 0x0F;
			val = (regs[4 /*REG_DAT*/] >> i) & 0x01; 
			drv = (regs[5 /*REG_DRV*/ + (i / 16)] >> (i % 16)*2) & 0x03; 
			pul = (regs[7 /*REG_PUL*/ + (i / 16)] >> (i % 16)*2) & 0x03;
			sprintf(buf + strlen(buf), "\t P%c%02d cfg=0x%X val=%d drv=0x%X pul=0x%X\n", 'A' + portnum, i, cfg, val, drv, pul);
		}
		
		iounmap(iorp);	
	}
	return strlen(buf);
}

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


static struct kobj_attribute codec_regs_attribute = __ATTR_RO(codec_regs);
static struct kobj_attribute gpio_regs_attribute  = __ATTR_RO(gpio_regs);

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
    
    &codec_regs_attribute.attr,
    &gpio_regs_attribute.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};


// --------------------------------------------------
//  Module Init and Exit
// --------------------------------------------------
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
