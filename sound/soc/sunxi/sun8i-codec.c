/*
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 * Mylène Josserand <mylene.josserand@free-electrons.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/regmap.h>

#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include "sun8iw5_sndcodec.h"

/* CODEC_OFFSET represents the offset of the codec registers
 * and not all the DAI registers
*/
#define CODEC_OFFSET				0x200
#define CODEC_BASSADDRESS			0x01c22c00
#define SUN8I_SYSCLK_CTL			(0x20c - CODEC_OFFSET)
#define SUN8I_SYSCLK_CTL_AIF1CLK_ENA		(11)
#define SUN8I_SYSCLK_CTL_SYSCLK_ENA		(3)
#define SUN8I_SYSCLK_CTL_SYSCLK_SRC		(0)
#define SUN8I_MOD_CLK_ENA			(0x210 - CODEC_OFFSET)
#define SUN8I_MOD_CLK_ENA_AIF1			(15)
#define SUN8I_MOD_CLK_ENA_DAC			(2)
#define SUN8I_MOD_RST_CTL			(0x214 - CODEC_OFFSET)
#define SUN8I_MOD_RST_CTL_AIF1			(15)
#define SUN8I_MOD_RST_CTL_DAC			(2)
#define SUN8I_SYS_SR_CTRL			(0x218 - CODEC_OFFSET)
#define SUN8I_SYS_SR_CTRL_AIF1_FS		(12)
#define SUN8I_SYS_SR_CTRL_AIF2_FS		(8)
#define SUN8I_AIF1CLK_CTRL			(0x240 - CODEC_OFFSET)
#define SUN8I_AIF1CLK_CTRL_AIF1_MSTR_MOD	(15)
#define SUN8I_AIF1CLK_CTRL_AIF1_BCLK_INV	(14)
#define SUN8I_AIF1CLK_CTRL_AIF1_LRCK_INV	(13)
#define SUN8I_AIF1CLK_CTRL_AIF1_BCLK_DIV	(9)
#define SUN8I_AIF1CLK_CTRL_AIF1_LRCK_DIV	(6)
#define SUN8I_AIF1CLK_CTRL_AIF1_WORD_SIZ	(4)
#define SUN8I_AIF1CLK_CTRL_AIF1_DATA_FMT	(2)
#define SUN8I_AIF1_DACDAT_CTRL			(0x248 - CODEC_OFFSET)
#define SUN8I_AIF1_DACDAT_CTRL_AIF1_DA0L_ENA	(15)
#define SUN8I_AIF1_DACDAT_CTRL_AIF1_DA0R_ENA	(14)
#define SUN8I_DAC_DIG_CTRL			(0x320 - CODEC_OFFSET)
#define SUN8I_DAC_DIG_CTRL_ENDA		(15)
#define SUN8I_DAC_MXR_SRC			(0x330 - CODEC_OFFSET)
#define SUN8I_DAC_MXR_SRC_DACL_MXR_SRC_AIF1DA0L (15)
#define SUN8I_DAC_MXR_SRC_DACL_MXR_SRC_AIF1DA1L (14)
#define SUN8I_DAC_MXR_SRC_DACL_MXR_SRC_AIF2DACL (13)
#define SUN8I_DAC_MXR_SRC_DACL_MXR_SRC_ADCL	(12)
#define SUN8I_DAC_MXR_SRC_DACR_MXR_SRC_AIF1DA0R (11)
#define SUN8I_DAC_MXR_SRC_DACR_MXR_SRC_AIF1DA1R (10)
#define SUN8I_DAC_MXR_SRC_DACR_MXR_SRC_AIF2DACR (9)
#define SUN8I_DAC_MXR_SRC_DACR_MXR_SRC_ADCR	(8)

struct sun8i_codec {
	struct device	*dev;
	struct regmap	*regmap;
	struct clk	*clk_module;
	struct clk	*clk_apb;
};

static void __iomem *regs_prcm;

static unsigned int read_prcm_wvalue(unsigned int addr)
{
	unsigned int reg;

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

	return reg;
}

static void write_prcm_wvalue(unsigned int addr, unsigned int val)
{
	unsigned int reg;

	reg = readl(regs_prcm);
	reg |= (0x1 << 28);
	writel(reg, regs_prcm);

	reg = readl(regs_prcm);
	reg &= ~(0x1f << 16);
	reg |= (addr << 16);
	writel(reg, regs_prcm);

	reg = readl(regs_prcm);
	reg &= ~(0xff << 8);
	reg |= (val << 8);
	writel(reg, regs_prcm);

	reg = readl(regs_prcm);
	reg |= (0x1 << 24);
	writel(reg, regs_prcm);

	reg = readl(regs_prcm);
	reg &= ~(0x1 << 24);
	writel(reg, regs_prcm);
}

static int codec_wrreg_prcm_bits(unsigned short reg, unsigned int mask,
				 unsigned int value)
{
	unsigned int old, new;

	old	=	read_prcm_wvalue(reg);
	new	=	(old & ~mask) | value;
	write_prcm_wvalue(reg, new);

	return 0;
}

static int codec_wr_prcm_control(u32 reg, u32 mask, u32 shift, u32 val)
{
	u32 reg_val;

	reg_val = val << shift;
	mask = mask << shift;
	codec_wrreg_prcm_bits(reg, mask, reg_val);
	return 0;
}

int snd_codec_info_volsw(struct snd_kcontrol *kcontrol,
			 struct snd_ctl_elem_info *uinfo)
{
	struct codec_mixer_control *mc = (struct codec_mixer_control *)
		kcontrol->private_value;
	int max = mc->max;
	unsigned int shift  = mc->shift;
	unsigned int rshift = mc->rshift;

	if (max == 1)
		uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;//the info of type
	else
		uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;

	uinfo->count = shift == rshift ? 1 : 2;	//the info of elem count
	uinfo->value.integer.min = 0;			//the info of min value
	uinfo->value.integer.max = max;		//the info of max value
	return 0;
}

int snd_codec_get_volsw(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct codec_mixer_control *mc = (struct codec_mixer_control *)
		kcontrol->private_value;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int max = mc->max;
	/*fls(7) = 3,fls(1)=1,fls(0)=0,fls(15)=4,fls(3)=2,fls(23)=5*/
	unsigned int mask = (1 << fls(max)) - 1;
	unsigned int invert = mc->invert;
	unsigned int reg = mc->reg;

	ucontrol->value.integer.value[0] = (read_prcm_wvalue(reg) >> shift)
		& mask;
	if (shift != rshift)
		ucontrol->value.integer.value[1] =
			(read_prcm_wvalue(reg) >> rshift) & mask;

	if (invert) {
		ucontrol->value.integer.value[0] =
			max - ucontrol->value.integer.value[0];
		if (shift != rshift)
			ucontrol->value.integer.value[1] =
				max - ucontrol->value.integer.value[1];
		}

		return 0;
}

int snd_codec_put_volsw(struct	snd_kcontrol *kcontrol,
			struct	snd_ctl_elem_value *ucontrol)
{
	struct codec_mixer_control *mc = (struct codec_mixer_control *)
		kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int max = mc->max;
	unsigned int mask = (1 << fls(max)) - 1;
	unsigned int invert = mc->invert;
	unsigned int	val, val2, val_mask;

	val = (ucontrol->value.integer.value[0] & mask);
	if (invert)
		val = max - val;
	val <<= shift;
	val_mask = mask << shift;
	if (shift != rshift) {
		val2 = (ucontrol->value.integer.value[1] & mask);
		if (invert)
			val2 = max - val2;
		val_mask |= mask << rshift;
		val |= val2 << rshift;
	}

	return codec_wrreg_prcm_bits(reg, val_mask, val);
}

static int sun8i_dac_event(struct snd_soc_dapm_widget *w,
			   struct snd_kcontrol *kcontrol, int event)
{
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		codec_wr_prcm_control(ADDA_APT2, 0x1, ZERO_CROSS_EN, 0x0);

		/*mute headphone pa*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);

		/*enable dac_l and dac_r*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACAREN, 0x1);

		codec_wr_prcm_control(ROMIXSC, 0x7f, RMIXMUTE, 0x2);
		codec_wr_prcm_control(LOMIXSC, 0x7f, LMIXMUTE, 0x2);

		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);

		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);

		/*hpout negative mute*/
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, LTRNMUTE, 0x0);
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, RTLNMUTE, 0x0);

		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x1);
	} else {
		/*disable analog part*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
		/* Mute Mixers */
		codec_wr_prcm_control(ROMIXSC, 0x7f, RMIXMUTE, 0x0);
		codec_wr_prcm_control(LOMIXSC, 0x7f, LMIXMUTE, 0x0);
		/* disable output mixer */
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);
		/* disable dac analog*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACAREN, 0x0);
	}

	return 0;
}

static int sun8i_codec_unmute(struct snd_soc_dai *dai, int mute)
{
	if (mute == 0) {
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x1);
	}
	return 0;
}

static int sun8i_codec_get_hw_rate(struct snd_pcm_hw_params *params)
{
	unsigned int rate = params_rate(params);

	switch (rate) {
	case 8000:
	case 7350:
		return 0x0;
	case 11025:
		return 0x1;
	case 12000:
		return 0x2;
	case 16000:
		return 0x3;
	case 22050:
		return 0x4;
	case 24000:
		return 0x5;
	case 32000:
		return 0x6;
	case 44100:
		return 0x7;
	case 48000:
		return 0x8;
	case 96000:
		return 0x9;
	case 192000:
		return 0xa;
	default:
		return -EINVAL;
	}
}

static int sun8i_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct sun8i_codec *scodec = snd_soc_codec_get_drvdata(dai->codec);
	unsigned long value;

	/* clock masters */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS: /* DAI Slave */
		value = 0x0; /* Codec Master */
		break;
	case SND_SOC_DAIFMT_CBM_CFM: /* DAI Master */
		value = 0x1; /* Codec Slave */
		break;
	default:
		return -EINVAL;
	}
	regmap_update_bits(scodec->regmap, SUN8I_AIF1CLK_CTRL,
			   BIT(SUN8I_AIF1CLK_CTRL_AIF1_MSTR_MOD),
			   value << SUN8I_AIF1CLK_CTRL_AIF1_MSTR_MOD);

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF: /* Normal */
		value = 0x0;
		break;
	case SND_SOC_DAIFMT_IB_IF: /* Inversion */
		value = 0x1;
		break;
	default:
		return -EINVAL;
	}
	regmap_update_bits(scodec->regmap, SUN8I_AIF1CLK_CTRL,
			   BIT(SUN8I_AIF1CLK_CTRL_AIF1_BCLK_INV),
			   value << SUN8I_AIF1CLK_CTRL_AIF1_BCLK_INV);
	regmap_update_bits(scodec->regmap, SUN8I_AIF1CLK_CTRL,
			   BIT(SUN8I_AIF1CLK_CTRL_AIF1_LRCK_INV),
			   value << SUN8I_AIF1CLK_CTRL_AIF1_LRCK_INV);

	/* DAI format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		value = 0x0;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		value = 0x1;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		value = 0x2;
		break;
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		value = 0x3;
		break;
	default:
		return -EINVAL;
	}
	regmap_update_bits(scodec->regmap, SUN8I_AIF1CLK_CTRL,
			   BIT(SUN8I_AIF1CLK_CTRL_AIF1_DATA_FMT),
			   value << SUN8I_AIF1CLK_CTRL_AIF1_DATA_FMT);

	return 0;
}

static int sun8i_codec_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{
	int rs_value  = 0;
	u32 bclk_lrck_div = 0, sample_resolution;
	int sample_rate = 0;
	struct sun8i_codec *scodec = snd_soc_codec_get_drvdata(dai->codec);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		sample_resolution = 16;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		sample_resolution = 24;
		break;
	default:
		return -EINVAL;
	}

	/*calculate word select bit*/
	switch (sample_resolution) {
	case 8:
		rs_value = 0x0;
		break;
	case 16:
		rs_value = 0x1;
		break;
	case 20:
		rs_value = 0x2;
		break;
	case 24:
		rs_value = 0x3;
		break;
	default:
		break;
	}
	regmap_update_bits(scodec->regmap, SUN8I_AIF1CLK_CTRL,
			   0x3 << SUN8I_AIF1CLK_CTRL_AIF1_WORD_SIZ,
			   rs_value << SUN8I_AIF1CLK_CTRL_AIF1_WORD_SIZ);

	/* calculate bclk_lrck_div Ratio */
	bclk_lrck_div = sample_resolution * 2;
	switch (bclk_lrck_div) {
	case 16:
		bclk_lrck_div = 0;
		break;
	case 32:
		bclk_lrck_div = 1;
		break;
	case 64:
		bclk_lrck_div = 2;
		break;
	case 128:
		bclk_lrck_div = 3;
		break;
	case 256:
		bclk_lrck_div = 4;
		break;
	default:
		break;
	}
	regmap_update_bits(scodec->regmap, SUN8I_AIF1CLK_CTRL,
			   0x7 << SUN8I_AIF1CLK_CTRL_AIF1_LRCK_DIV,
			   bclk_lrck_div << SUN8I_AIF1CLK_CTRL_AIF1_LRCK_DIV);

	sample_rate = sun8i_codec_get_hw_rate(params);
	if (sample_rate < 0)
		return sample_rate;

	regmap_update_bits(scodec->regmap, SUN8I_SYS_SR_CTRL,
			   0xf << SUN8I_SYS_SR_CTRL_AIF1_FS,
			   sample_rate << SUN8I_SYS_SR_CTRL_AIF1_FS);
	regmap_update_bits(scodec->regmap, SUN8I_SYS_SR_CTRL,
			   0xf << SUN8I_SYS_SR_CTRL_AIF2_FS,
			   sample_rate << SUN8I_SYS_SR_CTRL_AIF2_FS);

	return 0;
}

static const struct snd_kcontrol_new sun8i_codec_controls[] = {
	CODEC_SINGLE("headphone volume control", HP_VOLC, 0, 0x3f, 0),
};

static const struct snd_kcontrol_new sun8i_output_left_mixer_controls[] = {
	SOC_DAPM_SINGLE("LSlot 0", SUN8I_DAC_MXR_SRC,
			SUN8I_DAC_MXR_SRC_DACL_MXR_SRC_AIF1DA0L, 1, 0),
	SOC_DAPM_SINGLE("LSlot 1", SUN8I_DAC_MXR_SRC,
			SUN8I_DAC_MXR_SRC_DACL_MXR_SRC_AIF1DA1L, 1, 0),
	SOC_DAPM_SINGLE("DACL", SUN8I_DAC_MXR_SRC,
			SUN8I_DAC_MXR_SRC_DACL_MXR_SRC_AIF2DACL, 1, 0),
	SOC_DAPM_SINGLE("ADCL", SUN8I_DAC_MXR_SRC,
			SUN8I_DAC_MXR_SRC_DACL_MXR_SRC_ADCL, 1, 0),
};

static const struct snd_kcontrol_new sun8i_output_right_mixer_controls[] = {
	SOC_DAPM_SINGLE("RSlot 0", SUN8I_DAC_MXR_SRC,
			SUN8I_DAC_MXR_SRC_DACR_MXR_SRC_AIF1DA0R, 1, 0),
	SOC_DAPM_SINGLE("RSlot 1", SUN8I_DAC_MXR_SRC,
			SUN8I_DAC_MXR_SRC_DACR_MXR_SRC_AIF1DA1R, 1, 0),
	SOC_DAPM_SINGLE("DACR", SUN8I_DAC_MXR_SRC,
			SUN8I_DAC_MXR_SRC_DACR_MXR_SRC_AIF2DACR, 1, 0),
	SOC_DAPM_SINGLE("ADCR", SUN8I_DAC_MXR_SRC,
			SUN8I_DAC_MXR_SRC_DACR_MXR_SRC_ADCR, 1, 0),
};

static const struct snd_soc_dapm_widget sun8i_codec_dapm_widgets[] = {
	/* Digital parts of the DACs */
	SND_SOC_DAPM_SUPPLY("DAC", SUN8I_DAC_DIG_CTRL, SUN8I_DAC_DIG_CTRL_ENDA,
			    0, sun8i_dac_event, SND_SOC_DAPM_PRE_PMU |
			    SND_SOC_DAPM_POST_PMD),

	/* Analog DAC */
	SND_SOC_DAPM_DAC("Left DAC", "Playback", SUN8I_AIF1_DACDAT_CTRL,
			 SUN8I_AIF1_DACDAT_CTRL_AIF1_DA0L_ENA, 0),
	SND_SOC_DAPM_DAC("Right DAC", "Playback", SUN8I_AIF1_DACDAT_CTRL,
			 SUN8I_AIF1_DACDAT_CTRL_AIF1_DA0R_ENA, 0),

	/* DAC Mixers */
	SND_SOC_DAPM_MIXER("Left DAC Mixer", SND_SOC_NOPM, 0, 0,
			   sun8i_output_left_mixer_controls,
			   ARRAY_SIZE(sun8i_output_left_mixer_controls)),
	SND_SOC_DAPM_MIXER("Right DAC Mixer", SND_SOC_NOPM, 0, 0,
			   sun8i_output_right_mixer_controls,
			   ARRAY_SIZE(sun8i_output_right_mixer_controls)),

	/* Clocks */
	SND_SOC_DAPM_SUPPLY("MODCLK AFI1", SUN8I_MOD_CLK_ENA,
			    SUN8I_MOD_CLK_ENA_AIF1, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("MODCLK DAC", SUN8I_MOD_CLK_ENA,
			    SUN8I_MOD_CLK_ENA_DAC, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("AIF1", SUN8I_SYSCLK_CTL,
			    SUN8I_SYSCLK_CTL_AIF1CLK_ENA, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("SYSCLK", SUN8I_SYSCLK_CTL,
			    SUN8I_SYSCLK_CTL_SYSCLK_ENA, 0, NULL, 0),
	// Are they mixers ?
	SND_SOC_DAPM_SUPPLY("AIF1 PLL", SUN8I_SYSCLK_CTL, 0x9, 0, NULL, 0),
	/* Invert as 0=AIF1, 1=AIF2 */
	SND_SOC_DAPM_SUPPLY("SYSCLK AIF1", SUN8I_SYSCLK_CTL,
			    SUN8I_SYSCLK_CTL_SYSCLK_SRC, 1, NULL, 0),

	/* Module reset */
	SND_SOC_DAPM_SUPPLY("RST AIF1", SUN8I_MOD_RST_CTL,
			    SUN8I_MOD_RST_CTL_AIF1, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("RST DAC", SUN8I_MOD_RST_CTL,
			    SUN8I_MOD_RST_CTL_DAC, 0, NULL, 0),

	/* Headphone outputs */
	SND_SOC_DAPM_OUTPUT("HPOUTL"),
	SND_SOC_DAPM_OUTPUT("HPOUTR"),

};

static const struct snd_soc_dapm_route sun8i_codec_dapm_routes[] = {
	/* Clock Routes */
	{ "AIF1", NULL, "SYSCLK AIF1" },
	{ "AIF1 PLL", NULL, "AIF1" },
	{ "RST AIF1", NULL, "AIF1 PLL" },
	{ "MODCLK AFI1", NULL, "RST AIF1" },
	{ "DAC", NULL, "MODCLK AFI1" },

	{ "RST DAC", NULL, "SYSCLK" },
	{ "MODCLK DAC", NULL, "RST DAC" },
	{ "DAC", NULL, "MODCLK DAC" },

	/* DAC Routes */
	{ "Left DAC", NULL, "DAC" },
	{ "Right DAC", NULL, "DAC" },

	/* DAC Mixer Routes */
	{ "Left DAC Mixer", "LSlot 0", "Left DAC"},
	{ "Right DAC Mixer", "RSlot 0", "Right DAC"},

	/* End of route : HP out */
	{ "HPOUTL", NULL, "Left DAC Mixer" },
	{ "HPOUTR", NULL, "Right DAC Mixer" },
};

static struct snd_soc_dai_ops sun8i_codec_dai_ops = {
	.hw_params = sun8i_codec_hw_params,
	.set_fmt = sun8i_set_fmt,
	.digital_mute = sun8i_codec_unmute,
};

static struct snd_soc_dai_driver sun8i_codec_dai = {
	.name = "sun8i",
	/* playback capabilities */
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_192000 |
			SNDRV_PCM_RATE_KNOT,
		.formats = SNDRV_PCM_FMTBIT_S8 |
			SNDRV_PCM_FMTBIT_S16_LE |
			SNDRV_PCM_FMTBIT_S18_3LE |
			SNDRV_PCM_FMTBIT_S20_3LE |
			SNDRV_PCM_FMTBIT_S24_LE |
			SNDRV_PCM_FMTBIT_S32_LE,
	},
	/* pcm operations */
	.ops = &sun8i_codec_dai_ops,
};
EXPORT_SYMBOL(sun8i_codec_dai);

static int sun8i_soc_probe(struct snd_soc_codec *codec)
{
	/* Add virtual switch */
	snd_soc_add_codec_controls(codec, sun8i_codec_controls,
				   ARRAY_SIZE(sun8i_codec_controls));

	return 0;
}

/* power down chip */
static int sun8i_soc_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static struct snd_soc_codec_driver sun8i_soc_codec_sndpcm = {
	.probe			= sun8i_soc_probe,
	.remove		= sun8i_soc_remove,
//	.controls		= sun8i_codec_widgets,
//	.num_controls		= ARRAY_SIZE(sun8i_codec_widgets),
	.dapm_widgets		= sun8i_codec_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(sun8i_codec_dapm_widgets),
	.dapm_routes		= sun8i_codec_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(sun8i_codec_dapm_routes),
};

static const struct regmap_config sun8i_codec_regmap_config = {
	.reg_bits	= 32,
	.reg_stride	= 4,
	.val_bits	= 32,
	.max_register	= SUN8I_DAC_MXR_SRC,
};

static int sun8i_codec_probe(struct platform_device *pdev)
{
	struct resource *res_base, *res_prcm;
	struct sun8i_codec *scodec;
	void __iomem *base;

	scodec = devm_kzalloc(&pdev->dev, sizeof(*scodec), GFP_KERNEL);
	if (!scodec)
		return -ENOMEM;

	scodec->dev = &pdev->dev;

	/* Get the clocks from the DT */
	scodec->clk_module = devm_clk_get(&pdev->dev, "codec");
	if (IS_ERR(scodec->clk_module)) {
		dev_err(&pdev->dev, "[audio] Failed to get the module clock\n");
		return PTR_ERR(scodec->clk_module);
	}
	if (clk_prepare_enable(scodec->clk_module))
		pr_err("[audio] err:open failed;\n");

	scodec->clk_apb = devm_clk_get(&pdev->dev, "apb");
	if (IS_ERR(scodec->clk_apb)) {
		dev_err(&pdev->dev, "[audio] Failed to get the apb clock\n");
		return PTR_ERR(scodec->clk_apb);
	}
	if (clk_prepare_enable(scodec->clk_apb))
		pr_err("[audio] err:open failed;\n");

	/* Get base resources, registers and regmap */
	res_base = platform_get_resource_byname(pdev, IORESOURCE_MEM, "audio");
	base = devm_ioremap_resource(&pdev->dev, res_base);
	if (IS_ERR(base)) {
		dev_err(&pdev->dev, "[audio] Failed to map the registers\n");
		return PTR_ERR(base);
	}

	scodec->regmap = devm_regmap_init_mmio(&pdev->dev, base,
					       &sun8i_codec_regmap_config);
	if (IS_ERR(scodec->regmap)) {
		dev_err(&pdev->dev, "Failed to create our regmap\n");
		return PTR_ERR(scodec->regmap);
	}

	/* Get PRCM resources and registers */
	res_prcm = platform_get_resource_byname(pdev, IORESOURCE_MEM, "prcm");
	regs_prcm = devm_ioremap_resource(&pdev->dev, res_prcm);
	if (IS_ERR(regs_prcm)) {
		dev_err(&pdev->dev, "[audio] Failed to map PRCM registers\n");
		return PTR_ERR(regs_prcm);
	}

	/* Codec initialisation */
	codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, HPPAEN, 0x1);

	/* Set the codec data as driver data */
	dev_set_drvdata(&pdev->dev, scodec);

	snd_soc_register_codec(&pdev->dev, &sun8i_soc_codec_sndpcm,
			       &sun8i_codec_dai, 1);

	return 0;
}

static int sun8i_codec_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct sun8i_codec *scodec = snd_soc_card_get_drvdata(card);

	snd_soc_unregister_codec(&pdev->dev);
	clk_disable_unprepare(scodec->clk_module);
	clk_disable_unprepare(scodec->clk_apb);

	return 0;
}

static const struct of_device_id sun8i_codec_of_match[] = {
	{ .compatible = "allwinner,sun8i-a33-codec" },
	{ .compatible = "allwinner,sun8i-a23-codec" },
	{}
};
MODULE_DEVICE_TABLE(of, sun8i_codec_of_match);

static struct platform_driver sun8i_codec_driver = {
	.driver = {
		.name = "sunxi-8i-codec",
		.owner = THIS_MODULE,
		.of_match_table = sun8i_codec_of_match,
	},
	.probe = sun8i_codec_probe,
	.remove = sun8i_codec_remove,
};
module_platform_driver(sun8i_codec_driver);

MODULE_DESCRIPTION("Allwinner A33 (sun8i) codec driver");
MODULE_AUTHOR("huanxin<huanxin@reuuimllatech.com>");
MODULE_AUTHOR("Mylène Josserand <mylene.josserand@free-electrons.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sun8i-codec");
