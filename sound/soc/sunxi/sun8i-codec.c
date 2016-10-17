/*
 * Copyright 2014 Emilio López <emilio@elopez.com.ar>
 * Copyright 2014 Jon Smirl <jonsmirl@gmail.com>
 * Copyright 2015 Maxime Ripard <maxime.ripard@free-electrons.com>
 * Copyright 2015 Adam Sampson <ats@offog.org>
 * Copyright 2016 Lawrence Yu <lyu@micile.com>
 *
 * Based on the Allwinner SDK driver, released under the GPL.
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
 */
 
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include <linux/gpio/consumer.h>
#include <linux/reset.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/initval.h>
#include <sound/dmaengine_pcm.h>

#define SUN8I_CODEC_DA_CTL				(0x000)
#define SUN8I_CODEC_DA_CTL_SDO_EN			(8)
#define SUN8I_CODEC_DA_CTL_ASS				(6)
#define SUN8I_CODEC_DA_CTL_TXEN				(2)
#define SUN8I_CODEC_DA_CTL_GEN				(0)

#define SUN8I_CODEC_DA_TXFIFO				(0x00C)

#define SUN8I_CODEC_DA_FCTL				(0x014)
#define SUN8I_CODEC_DA_FCTL_FTX				(25)
#define SUN8I_CODEC_DA_FCTL_TXTL			(12)
#define SUN8I_CODEC_DA_FCTL_TXIM			(2)

#define SUN8I_CODEC_DA_INT				(0x01C)
#define SUN8I_CODEC_DA_INT_TX_DRQ			(7)


#define SUN8I_CODEC_DA_CLKD				(0x024)
#define SUN8I_CODEC_DA_CLKD_MCLKO_EN			(7)
#define SUN8I_CODEC_DA_CLKD_BCLKDIV			(4)
#define SUN8I_CODEC_DA_CLKD_MCLKDIV			(0)

#define SUN8I_CODEC_MOD_CLK_ENA				(0x210)
#define SUN8I_CODEC_MOD_CLK_ENA_AIF1			(15)	   
#define SUN8I_CODEC_MOD_CLK_ENA_DAC			(2)	   
	
#define SUN8I_CODEC_MOD_RST_CTL				(0x214)
#define SUN8I_CODEC_MOD_RST_CTL_AIF1			(15)	   
#define SUN8I_CODEC_MOD_RST_CTL_DAC			(2)	   

#define SUN8I_CODEC_SYS_SR_CTRL				(0x218)
#define SUN8I_CODEC_SYS_SR_CTRL_AIF1_FS			(12)

#define SUN8I_CODEC_SYSCLK_CTL				(0x20C)
#define SUN8I_CODEC_SYSCLK_CTL_AIF1CLK_ENA		(11)
#define SUN8I_CODEC_SYSCLK_CTL_AIF1CLK_SRC		(8)
#define SUN8I_CODEC_SYSCLK_CTL_SYSCLK_ENA		(3)
	
#define SUN8I_CODEC_AIF1CLK_CTRL			(0x240)
#define SUN8I_CODEC_AIF1CLK_CTRL_AIF1_MSTR_MOD		(15)
#define SUN8I_CODEC_AIF1CLK_CTRL_AIF1_LRCK_DIV		(6)
#define SUN8I_CODEC_AIF1CLK_CTRL_AIF1_WORD_SIZ		(4)
#define SUN8I_CODEC_AIF1CLK_CTRL_DSP_MONO_PCM		(1)

#define SUN8I_CODEC_AIF1_DACDAT_CTRL			(0x248)
#define SUN8I_CODEC_AIF1_DACDAT_CTRL_AIF1_DA0L_ENA	(15)
#define SUN8I_CODEC_AIF1_DACDAT_CTRL_AIF1_DA0R_ENA	(14)

#define SUN8I_CODEC_AIF1_MXR_SRC			(0x24C)
#define SUN8I_CODEC_AIF1_MXR_SRC_AIF1_AD0L_MXL_SRC	(12)
#define SUN8I_CODEC_AIF1_MXR_SRC_AIF1_AD0L_MXR_SRC	(8)

#define SUN8I_CODEC_DAC_DIG_CTRL			(0x320)
#define SUN8I_CODEC_DAC_DIG_CTRL_ENDA			(15)

#define SUN8I_CODEC_DAC_MXR_SRC				(0x330)
#define SUN8I_CODEC_DAC_MXR_SRC_DACL_MXR_SRC		(12)
#define SUN8I_CODEC_DAC_MXR_SRC_DACR_MXR_SRC		(8)

#define SUN8I_CODEC_SRC2_CTRL4				(0x4FC)

#define SUN8I_CODEC_HP_VOLC				(0x00)
#define SUN8I_CODEC_LOMIXSC				(0x01)
#define SUN8I_CODEC_ROMIXSC				(0x02)
#define SUN8I_CODEC_DAC_PA_SRC				(0x03)
#define SUN8I_CODEC_PAEN_HP_CTRL			(0x07)


#define HP_VOLC                                          (0x00)

struct sun8i_codec {
	struct device	*dev;
	struct regmap	*regmap;
	struct clk	*clk_apb;
	struct clk	*clk_module;
	struct clk	*clk_adda;
	struct gpio_desc *gpio_pa;
	struct reset_control *reset;

	struct snd_dmaengine_dai_dma_data	playback_dma_data;
};

static unsigned int sun8i_codec_read_prcm_wvalue(unsigned int addr)
{
	unsigned int reg;
	void *p;
		
 	p = ioremap(0x01F015C0, 0x10);

 	reg = ioread32((char *)p);
	reg |= (0x1<<28);
	iowrite32(reg, (char*)p);

 	reg = ioread32((char *)p);
	reg &= ~(0x1<<24);
	iowrite32(reg, (char*)p);

 	reg = ioread32((char *)p);
	reg &= ~(0x1f<<16);
	reg |= (addr<<16);
	iowrite32(reg, (char*)p);

 	reg = ioread32((char *)p);
	reg &= (0xff<<0);
	iounmap(p);

	return reg;
}

static void sun8i_codec_write_prcm_wvalue(unsigned int addr, unsigned int val)
{
	unsigned int reg;
	void *p;
		
 	p = ioremap(0x01F015C0, 0x10);
	
 	reg = ioread32((char *)p);
	reg |= (0x1<<28);
	iowrite32(reg, (char*)p);

 	reg = ioread32((char *)p);
	reg &= ~(0x1f<<16);
	reg |= (addr<<16);
	iowrite32(reg, (char*)p);

 	reg = ioread32((char *)p);
	reg &= ~(0xff<<8);
	reg |= (val<<8);
	iowrite32(reg, (char*)p);

 	reg = ioread32((char *)p);
	reg |= (0x1<<24);
	iowrite32(reg, (char*)p);

 	reg = ioread32((char *)p);
	reg &= ~(0x1<<24);
	iowrite32(reg, (char*)p);
	
	iounmap(p);
}

static void sun8i_codec_start_playback(struct sun8i_codec *scodec)
{
	/* Flush TX FIFO */
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_DA_FCTL,
			   BIT(SUN8I_CODEC_DA_FCTL_FTX),
			   BIT(SUN8I_CODEC_DA_FCTL_FTX));

	/* Enable DAC DRQ */
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_DA_INT,
			   BIT(SUN8I_CODEC_DA_INT_TX_DRQ),
			   BIT(SUN8I_CODEC_DA_INT_TX_DRQ));
}

static void sun8i_codec_stop_playback(struct sun8i_codec *scodec)
{
	/* Disable DAC DRQ */
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_DA_INT,
			   BIT(SUN8I_CODEC_DA_INT_TX_DRQ),
			   0);
}

static int sun8i_codec_trigger(struct snd_pcm_substream *substream, int cmd,
			       struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sun8i_codec *scodec = snd_soc_card_get_drvdata(rtd->card);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			sun8i_codec_start_playback(scodec);
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			sun8i_codec_stop_playback(scodec);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int sun8i_codec_prepare_playback(struct snd_pcm_substream *substream,
					struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sun8i_codec *scodec = snd_soc_card_get_drvdata(rtd->card);

	/* Flush the TX FIFO */
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_DA_FCTL,
			   BIT(SUN8I_CODEC_DA_FCTL_FTX),
			   BIT(SUN8I_CODEC_DA_FCTL_FTX));

	/* Set TX FIFO Empty Trigger Level */
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_DA_FCTL,
			   0x3f << SUN8I_CODEC_DA_FCTL_TXTL,
			   0xf << SUN8I_CODEC_DA_FCTL_TXTL);

	/* Send zeros when we have an underrun */
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_DA_CTL,
			   BIT(SUN8I_CODEC_DA_CTL_ASS),
			   0);

	return 0;
};

static int sun8i_codec_prepare(struct snd_pcm_substream *substream,
			       struct snd_soc_dai *dai)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return sun8i_codec_prepare_playback(substream, dai);

	return -EINVAL;
}

static unsigned long sun8i_codec_get_mod_freq(struct snd_pcm_hw_params *params)
{
	unsigned int rate = params_rate(params);

	switch (rate) {
	case 44100:
	case 22050:
	case 11025:
printk("sun8i_codec_get_mod_freq %d\n",22579200);	
		return 22579200;

	case 192000:
	case 96000:
	case 48000:
	case 32000:
	case 24000:
	case 16000:
	case 12000:
	case 8000:
printk("sun8i_codec_get_mod_freq %d\n",24576000);	
		return 24576000;

	default:
		return 0;
	}
}

static unsigned long sun8i_codec_get_mclkdiv(struct snd_pcm_hw_params *params)
{
	unsigned int rate = params_rate(params);

	switch (rate) {
	case 192000:	// divide by 1
		return 0;
	case 96000:	// divide by 2
		return 1;
	case 48000:	// divide by 4
		return 2;
	case 44100:	// divide by 4
		return 2;
	case 32000:	// divide by 6
		return 3;
	case 24000:	// divide by 8
		return 4;
	case 22050:	// divide by 8
		return 4;
	case 16000:	// divide by 12
		return 5;		
	case 12000:	// divide by 16
		return 6;
	case 11025:	// divide by 16
		return 6;
	case 8000:	// divide by 24
		return 8;

	default:
		return -EINVAL;
	}
}

static int sun8i_codec_get_hw_rate(struct snd_pcm_hw_params *params)
{
	unsigned int rate = params_rate(params);

	switch (rate) {
	case 192000:
		return 10;
		
	case 96000:
		return 9;
		
	case 48000:
		return 8;
		
	case 44100:
		return 7;
		
	case 32000:
		return 6;
		
	case 24000:
		return 5;
		
	case 22050:
		return 4;
		
	case 16000:
		return 3;
		
	case 12000:
		return 2;
		
	case 11025:
		return 1;
		
	case 8000:
		return 0;

	default:
		return -EINVAL;
	}
}

static int sun8i_codec_hw_params_playback(struct sun8i_codec *scodec,
					  struct snd_pcm_hw_params *params,
					  unsigned int hwrate)
{
	u32 val;
	int div;
	
	/* Set DAC sample rate */
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_SYS_SR_CTRL,
			   0xF << SUN8I_CODEC_SYS_SR_CTRL_AIF1_FS,
			   hwrate << SUN8I_CODEC_SYS_SR_CTRL_AIF1_FS);

	/* Set the MCLKDIV to match the sampling rate * 64 */
	div = sun8i_codec_get_mclkdiv(params);	
	if (div < 0)
		return div;
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_DA_CLKD,
			   0xF << SUN8I_CODEC_DA_CLKD_MCLKDIV,
			   div << SUN8I_CODEC_DA_CLKD_MCLKDIV);

	/* Set the number of channels we want to use */
	if (params_channels(params) == 1)
		val = BIT(SUN8I_CODEC_AIF1CLK_CTRL_DSP_MONO_PCM);
	else
		val = 0;

	regmap_update_bits(scodec->regmap, SUN8I_CODEC_AIF1CLK_CTRL,
			   BIT(SUN8I_CODEC_AIF1CLK_CTRL_DSP_MONO_PCM),
			   val);

	/* Set the number of sample bits to either 16 or 24 bits */
	switch (hw_param_interval(params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS)->min) {
	case 24:
		regmap_update_bits(scodec->regmap, SUN8I_CODEC_AIF1CLK_CTRL,
				   0x3 << SUN8I_CODEC_AIF1CLK_CTRL_AIF1_WORD_SIZ,
				   0x3 << SUN8I_CODEC_AIF1CLK_CTRL_AIF1_WORD_SIZ);
		break;
	case 16:
		regmap_update_bits(scodec->regmap, SUN8I_CODEC_AIF1CLK_CTRL,
				   0x3 << SUN8I_CODEC_AIF1CLK_CTRL_AIF1_WORD_SIZ,
				   0x1 << SUN8I_CODEC_AIF1CLK_CTRL_AIF1_WORD_SIZ);
		break;
	case 8:
		regmap_update_bits(scodec->regmap, SUN8I_CODEC_AIF1CLK_CTRL,
				   0x3 << SUN8I_CODEC_AIF1CLK_CTRL_AIF1_WORD_SIZ,
				   0x0 << SUN8I_CODEC_AIF1CLK_CTRL_AIF1_WORD_SIZ);	
		break;
	default:
		return -EINVAL;
	}
	
	/* Set TX FIFO mode to padding the MSBs with 0 */
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_DA_FCTL,
			   BIT(SUN8I_CODEC_DA_FCTL_TXIM),
			   BIT(SUN8I_CODEC_DA_FCTL_TXIM));
	
	return 0;
}

static int sun8i_codec_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sun8i_codec *scodec = snd_soc_card_get_drvdata(rtd->card);
	unsigned long clk_freq;
	int ret, hwrate;

	clk_freq = sun8i_codec_get_mod_freq(params);
	if (!clk_freq)
		return -EINVAL;

 	ret = clk_set_rate(scodec->clk_module, clk_freq);
 	if (ret)
 		return ret;

	hwrate = sun8i_codec_get_hw_rate(params);
	if (hwrate < 0)
		return hwrate;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return sun8i_codec_hw_params_playback(scodec, params,
						      hwrate);

	return -EINVAL;
}

static int sun8i_codec_startup(struct snd_pcm_substream *substream,
			       struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sun8i_codec *scodec = snd_soc_card_get_drvdata(rtd->card);

	return clk_prepare_enable(scodec->clk_module);
}

static void sun8i_codec_shutdown(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sun8i_codec *scodec = snd_soc_card_get_drvdata(rtd->card);

	clk_disable_unprepare(scodec->clk_module);
}

static const struct snd_soc_dai_ops sun8i_codec_dai_ops = {
	.startup	= sun8i_codec_startup,
	.shutdown	= sun8i_codec_shutdown,
	.trigger	= sun8i_codec_trigger,
	.hw_params	= sun8i_codec_hw_params,
	.prepare	= sun8i_codec_prepare,
};

static struct snd_soc_dai_driver sun8i_codec_dai = {
	.name	= "Codec",
	.ops	= &sun8i_codec_dai_ops,
	.playback = {
		.stream_name	= "Codec Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rate_min	= 8000,
		.rate_max	= 192000,
		.rates		= SNDRV_PCM_RATE_8000_48000 |
				  SNDRV_PCM_RATE_96000 |
				  SNDRV_PCM_RATE_192000,
		.formats	= SNDRV_PCM_FMTBIT_S8 |
				  SNDRV_PCM_FMTBIT_S16_LE |
				  SNDRV_PCM_FMTBIT_S24_LE,
		.sig_bits	= 24,
	},
};

/*** Codec ***/


struct codec_mixer_control {
	int min;
	int max;
	int where;
	unsigned int mask;
	unsigned int reg;
	unsigned int rreg;
	unsigned int shift;
	unsigned int rshift;
	unsigned int invert;
	unsigned int value;
};



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

static void write_prcm_wvalue(unsigned int addr, unsigned int val)
{
	unsigned int reg;

	void __iomem *regs_prcm;

 	regs_prcm = ioremap(0x01F015C0, 0x10);

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

	iounmap(regs_prcm);
	
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
 
#define CODEC_SINGLE_VALUE(xreg, xshift, xmax, xinvert)\
	((unsigned long)&(struct codec_mixer_control)\
	{.reg = xreg, .shift = xshift, .rshift = xshift, .max = xmax, \
			.invert = xinvert})
 
#define CODEC_SINGLE(xname,    reg,    shift,  max,    invert)\
{      .iface  = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname,\
       .info   = snd_codec_info_volsw, .get = snd_codec_get_volsw,\
       .put    = snd_codec_put_volsw,\
       .private_value  = CODEC_SINGLE_VALUE(reg, shift, max, invert)}

/*     mixer control*/


static const struct snd_kcontrol_new sun8i_codec_controls[] = {
	CODEC_SINGLE("headphone volume control", HP_VOLC, 0, 0x3f, 0),
};

static int sun8i_soc_remove(struct snd_soc_codec *codec)
{
	return 0;
}


static int sun8i_soc_probe(struct snd_soc_codec *codec)
{
	/* Add virtual switch */
	snd_soc_add_codec_controls(codec, sun8i_codec_controls,
				   ARRAY_SIZE(sun8i_codec_controls));

	return 0;
}



/*
static const struct snd_kcontrol_new sun8i_codec_pa_mute =
	SOC_DAPM_SINGLE("Switch", SUN4I_CODEC_DAC_ACTL,
			SUN4I_CODEC_DAC_ACTL_PA_MUTE, 1, 0);
*/
// static DECLARE_TLV_DB_SCALE(sun4i_codec_pa_volume_scale, -6300, 100, 1);
// 
static const struct snd_kcontrol_new sun8i_codec_widgets[] = {
// 	SOC_SINGLE_TLV("Power Amplifier Volume", SUN4I_CODEC_DAC_ACTL,
// 		       SUN4I_CODEC_DAC_ACTL_PA_VOL, 0x3F, 0,
// 		       sun4i_codec_pa_volume_scale),
};
// 
// static const struct snd_kcontrol_new sun4i_codec_pa_mixer_controls[] = {
// 	SOC_DAPM_SINGLE("DAC Playback Switch", SUN4I_CODEC_DAC_ACTL,
// 			SUN4I_CODEC_DAC_ACTL_DACPAS, 1, 0),
// // 	SOC_DAPM_SINGLE("Mixer Playback Switch", SUN4I_CODEC_DAC_ACTL,
// // 			SUN4I_CODEC_DAC_ACTL_MIXPAS, 1, 0),
// };

static const struct snd_soc_dapm_widget sun8i_codec_codec_dapm_widgets[] = {
// 	// Power management for the DAC.  Flips the bit on the register provided to enable / disable
// 	SND_SOC_DAPM_SUPPLY("DAC", SUN4I_CODEC_DAC_DPC, SUN4I_CODEC_DAC_DPC_EN_DA, 0, NULL, 0),
/*
widget name = "DAC"
register = SUN4I_CODEC_DAC_DPC
shift = SUN4I_CODEC_DAC_DPC_EN_DA
invert = 0
event = NULL
flags = 0
*/

// 
// 	// Enabled / disables the Left and right DAC channels.  FLips the bit on the register provided
// 	SND_SOC_DAPM_DAC("Left DAC",  "Codec Playback", SUN4I_CODEC_DAC_ACTL, SUN4I_CODEC_DAC_ACTL_DACAENL, 0),
// 	SND_SOC_DAPM_DAC("Right DAC", "Codec Playback", SUN4I_CODEC_DAC_ACTL, SUN4I_CODEC_DAC_ACTL_DACAENR, 0),
// 
// 	/* Power Amplifier */
// 	SND_SOC_DAPM_MIXER("Power Amplifier", SUN4I_CODEC_ADC_ACTL,
// 			   SUN4I_CODEC_ADC_ACTL_PA_EN, 0,
// 			   sun4i_codec_pa_mixer_controls,
// 			   ARRAY_SIZE(sun4i_codec_pa_mixer_controls)),
/*
 	SND_SOC_DAPM_SWITCH("Power Amplifier Mute", SND_SOC_NOPM, 0, 0,
 			    &sun4i_codec_pa_mute), 
 */
 	// Headphone output
 	SND_SOC_DAPM_OUTPUT("HP Right"),
 	SND_SOC_DAPM_OUTPUT("HP Left"),
};

static const struct snd_soc_dapm_route sun8i_codec_codec_dapm_routes[] = {
// 	/* Left ADC / DAC Routes */
// 	{ "Left DAC", NULL, "DAC" },
// 
// 	/* Right ADC / DAC Routes */
// 	{ "Right DAC", NULL, "DAC" },
// 
// 	/* Power Amplifier Routes */
// 	{ "Power Amplifier", "DAC Playback Switch", "Left DAC" },
// 	{ "Power Amplifier", "DAC Playback Switch", "Right DAC" },
// 
// 	/* Headphone Output Routes */
// 	{ "Power Amplifier Mute", "Switch", "Power Amplifier" },
// 	{ "HP Right", NULL, "Power Amplifier Mute" },
// 	{ "HP Left", NULL, "Power Amplifier Mute" },

};

static struct snd_soc_codec_driver sun8i_codec_codec = {
	.probe			= sun8i_soc_probe,
	.remove		= sun8i_soc_remove,
	.component_driver = {
//		.controls		= sun8i_codec_widgets,
//		.num_controls		= ARRAY_SIZE(sun8i_codec_widgets),
		.dapm_widgets		= sun8i_codec_codec_dapm_widgets,
		.num_dapm_widgets	= ARRAY_SIZE(sun8i_codec_codec_dapm_widgets),
		.dapm_routes		= sun8i_codec_codec_dapm_routes,
		.num_dapm_routes	= ARRAY_SIZE(sun8i_codec_codec_dapm_routes),
	}
};

static const struct snd_soc_component_driver sun8i_codec_component = {
	.name = "sun8i-codec",
};

#define SUN8I_CODEC_RATES	SNDRV_PCM_RATE_8000_192000
#define SUN8I_CODEC_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE | \
				 SNDRV_PCM_FMTBIT_S32_LE)

static int sun8i_codec_dai_probe(struct snd_soc_dai *dai)
{
	struct snd_soc_card *card = snd_soc_dai_get_drvdata(dai);
	struct sun8i_codec *scodec = snd_soc_card_get_drvdata(card);

	snd_soc_dai_init_dma_data(dai, &scodec->playback_dma_data,
				  NULL);

	return 0;
}

static struct snd_soc_dai_driver dummy_cpu_dai = {
	.name	= "sun8i-codec-cpu-dai",
	.probe	= sun8i_codec_dai_probe,
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SUN8I_CODEC_RATES,
		.formats	= SUN8I_CODEC_FORMATS,
		.sig_bits	= 24,
	},
};

static const struct regmap_config sun8i_codec_regmap_config = {
	.reg_bits	= 32,
	.reg_stride	= 4,
	.val_bits	= 32,
	.max_register	= SUN8I_CODEC_SRC2_CTRL4,
};

static const struct of_device_id sun8i_codec_of_match[] = {
	{ .compatible = "allwinner,sun8i-a33-codec" },
	{}
};
MODULE_DEVICE_TABLE(of, sun8i_codec_of_match);

static struct snd_soc_dai_link *sun8i_codec_create_link(struct device *dev,
							int *num_links)
{
	struct snd_soc_dai_link *link = devm_kzalloc(dev, sizeof(*link),
						     GFP_KERNEL);
	if (!link)
		return NULL;

	link->name		= "cdc";
	link->stream_name	= "CDC PCM";
	link->codec_dai_name	= "Codec";
	link->cpu_dai_name	= dev_name(dev);
	link->codec_name	= dev_name(dev);
	link->platform_name	= dev_name(dev);
	link->dai_fmt		= SND_SOC_DAIFMT_I2S;

	*num_links = 1;

	return link;
};

static int sun8i_codec_spk_event(struct snd_soc_dapm_widget *w,
				 struct snd_kcontrol *k, int event)
{
	struct sun8i_codec *scodec = snd_soc_card_get_drvdata(w->dapm->card);


printk("MICILE -- SPEAKER EVENT!\n");
	if (scodec->gpio_pa)
		gpiod_set_value_cansleep(scodec->gpio_pa,
					 !!SND_SOC_DAPM_EVENT_ON(event));

	return 0;
}

static const struct snd_soc_dapm_widget sun8i_codec_card_dapm_widgets[] = {
	SND_SOC_DAPM_SPK("Speaker", sun8i_codec_spk_event),
};

static const struct snd_soc_dapm_route sun8i_codec_card_dapm_routes[] = {
	{ "Speaker", NULL, "HP Right" },
	{ "Speaker", NULL, "HP Left" },
};

static struct snd_soc_card *sun8i_codec_create_card(struct device *dev)
{
	struct snd_soc_card *card;

	card = devm_kzalloc(dev, sizeof(*card), GFP_KERNEL);
	if (!card)
		return NULL;

	card->dai_link = sun8i_codec_create_link(dev, &card->num_links);
	if (!card->dai_link)
		return NULL;

	card->dev		= dev;
	card->name		= "sun8i-codec";
	card->dapm_widgets	= sun8i_codec_card_dapm_widgets;
	card->num_dapm_widgets	= ARRAY_SIZE(sun8i_codec_card_dapm_widgets);
	card->dapm_routes	= sun8i_codec_card_dapm_routes;
	card->num_dapm_routes	= ARRAY_SIZE(sun8i_codec_card_dapm_routes);
	return card;
};

static int sun8i_codec_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card;
	struct sun8i_codec *scodec;
	struct resource *res;
	void __iomem *base;
	int ret;

	printk("MICILE - CODEC PROBE STARTED!\n");
	scodec = devm_kzalloc(&pdev->dev, sizeof(*scodec), GFP_KERNEL);
	if (!scodec)
		return -ENOMEM;

	scodec->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base)) {
		dev_err(&pdev->dev, "Failed to map the registers\n");
		return PTR_ERR(base);
	}

	scodec->regmap = devm_regmap_init_mmio(&pdev->dev, base,
					     &sun8i_codec_regmap_config);
	if (IS_ERR(scodec->regmap)) {
		dev_err(&pdev->dev, "Failed to create our regmap\n");
		return PTR_ERR(scodec->regmap);
	}

 	/* Get the clocks from the DT */
 	scodec->clk_apb = devm_clk_get(&pdev->dev, "apb");
 	if (IS_ERR(scodec->clk_apb)) {
 		dev_err(&pdev->dev, "Failed to get the APB clock\n");
 		return PTR_ERR(scodec->clk_apb);
	}

	scodec->clk_module = devm_clk_get(&pdev->dev, "codec");
	if (IS_ERR(scodec->clk_module)) {
		dev_err(&pdev->dev, "Failed to get the module clock\n");
		return PTR_ERR(scodec->clk_module);
	}

	scodec->clk_adda = devm_clk_get(&pdev->dev, "adda");
	if (IS_ERR(scodec->clk_adda)) {
		dev_err(&pdev->dev, "Failed to get the adda clock\n");
		return PTR_ERR(scodec->clk_adda);
	}

	/* Enable the bus clock */
	if (clk_prepare_enable(scodec->clk_apb)) {
		dev_err(&pdev->dev, "Failed to enable the APB clock\n");
		return -EINVAL;
	}

	/* Enable the adda clock */
	if (clk_prepare_enable(scodec->clk_adda)) {
		dev_err(&pdev->dev, "Failed to enable the ADDA clock\n");
		return -EINVAL;
	}
	
	/* Deassert the reset */
	scodec->reset = devm_reset_control_get(&pdev->dev, "adda");
	if (IS_ERR(scodec->reset)) {
 		dev_err(&pdev->dev, "Failed to get the ADDA reset\n");
 		return PTR_ERR(scodec->reset);		
	}
	reset_control_deassert(scodec->reset);

	/* Get the gpio for the power amplifier */
	scodec->gpio_pa = devm_gpiod_get_optional(&pdev->dev, "allwinner,pa",
						  GPIOD_OUT_LOW);
	if (IS_ERR(scodec->gpio_pa)) {
		ret = PTR_ERR(scodec->gpio_pa);
		if (ret != -EPROBE_DEFER)
			dev_err(&pdev->dev, "Failed to get pa gpio: %d\n", ret);
		return ret;
	}

	/* DMA configuration for TX FIFO */
	scodec->playback_dma_data.addr = res->start + SUN8I_CODEC_DA_TXFIFO;
	scodec->playback_dma_data.maxburst = 8;
	scodec->playback_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;

	ret = snd_soc_register_codec(&pdev->dev, &sun8i_codec_codec,
				     &sun8i_codec_dai, 1);
				     
	if (ret) {
		dev_err(&pdev->dev, "Failed to register our codec\n");
		goto err_clk_disable;
	}

	ret = devm_snd_soc_register_component(&pdev->dev,
					      &sun8i_codec_component,
					      &dummy_cpu_dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register our DAI\n");
		goto err_unregister_codec;
	}

	ret = devm_snd_dmaengine_pcm_register(&pdev->dev, NULL, 0);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register against DMAEngine\n");
		goto err_unregister_codec;
	}

	card = sun8i_codec_create_card(&pdev->dev);
	if (!card) {
		dev_err(&pdev->dev, "Failed to create our card\n");
		goto err_unregister_codec;
	}

	platform_set_drvdata(pdev, card);
	snd_soc_card_set_drvdata(card, scodec);

	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register our card\n");
		goto err_unregister_codec;
	}

	/* Enable the Codec and the DAC */
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_DA_CTL,
			   1 << SUN8I_CODEC_DA_CTL_SDO_EN,
			   1 << SUN8I_CODEC_DA_CTL_SDO_EN);
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_DA_CTL,
			   1 << SUN8I_CODEC_DA_CTL_TXEN,
			   1 << SUN8I_CODEC_DA_CTL_TXEN);
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_DA_CTL,
			   1 << SUN8I_CODEC_DA_CTL_GEN,
			   1 << SUN8I_CODEC_DA_CTL_GEN);
	
	/* Enable the DAC Clk, divide Audio PLL by 24 */	
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_DA_CLKD,
			   1 << SUN8I_CODEC_DA_CLKD_MCLKO_EN,
			   1 << SUN8I_CODEC_DA_CLKD_MCLKO_EN);

	/* Enable the Codec AIF1 clock */
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_SYSCLK_CTL,
			   1 << SUN8I_CODEC_SYSCLK_CTL_AIF1CLK_ENA,
			   1 << SUN8I_CODEC_SYSCLK_CTL_AIF1CLK_ENA);
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_SYSCLK_CTL,
			   0x03 << SUN8I_CODEC_SYSCLK_CTL_AIF1CLK_SRC,
			   0x03 << SUN8I_CODEC_SYSCLK_CTL_AIF1CLK_SRC);
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_SYSCLK_CTL,
			   1 << SUN8I_CODEC_SYSCLK_CTL_SYSCLK_ENA,
			   1 << SUN8I_CODEC_SYSCLK_CTL_SYSCLK_ENA);
	
	/* Enable the AIF1 and DAC */
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_MOD_CLK_ENA,
			   1 << SUN8I_CODEC_MOD_CLK_ENA_AIF1,
			   1 << SUN8I_CODEC_MOD_CLK_ENA_AIF1);
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_MOD_CLK_ENA,
			   1 << SUN8I_CODEC_MOD_CLK_ENA_DAC,
			   1 << SUN8I_CODEC_MOD_CLK_ENA_DAC);

	/* Deassert reset for the AIF1 and DAC */
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_MOD_RST_CTL,
			   1 << SUN8I_CODEC_MOD_RST_CTL_AIF1,
			   1 << SUN8I_CODEC_MOD_RST_CTL_AIF1);
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_MOD_RST_CTL,
			   1 << SUN8I_CODEC_MOD_RST_CTL_DAC,
			   1 << SUN8I_CODEC_MOD_RST_CTL_DAC);

	/* Setup AIF1 */
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_AIF1CLK_CTRL,
			   1 << SUN8I_CODEC_AIF1CLK_CTRL_AIF1_MSTR_MOD,
			   1 << SUN8I_CODEC_AIF1CLK_CTRL_AIF1_MSTR_MOD);
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_AIF1CLK_CTRL,
			   0x07 << SUN8I_CODEC_AIF1CLK_CTRL_AIF1_LRCK_DIV,
			   0x02 << SUN8I_CODEC_AIF1CLK_CTRL_AIF1_LRCK_DIV);
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_AIF1_DACDAT_CTRL,
			   1 << SUN8I_CODEC_AIF1_DACDAT_CTRL_AIF1_DA0L_ENA,
			   1 << SUN8I_CODEC_AIF1_DACDAT_CTRL_AIF1_DA0L_ENA);
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_AIF1_DACDAT_CTRL,
			   1 << SUN8I_CODEC_AIF1_DACDAT_CTRL_AIF1_DA0R_ENA,
			   1 << SUN8I_CODEC_AIF1_DACDAT_CTRL_AIF1_DA0R_ENA);
	
	/* Setup AIF1 Left channel to play DACL and AIF1 Right channel to play DACR */
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_AIF1_MXR_SRC,
			   0xF << SUN8I_CODEC_AIF1_MXR_SRC_AIF1_AD0L_MXL_SRC,
			   0x8 << SUN8I_CODEC_AIF1_MXR_SRC_AIF1_AD0L_MXL_SRC);
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_AIF1_MXR_SRC,
			   0xF << SUN8I_CODEC_AIF1_MXR_SRC_AIF1_AD0L_MXR_SRC,
			   0x8 << SUN8I_CODEC_AIF1_MXR_SRC_AIF1_AD0L_MXR_SRC);
	
	/* Enable the DAC */
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_DAC_DIG_CTRL,
			   1 << SUN8I_CODEC_DAC_DIG_CTRL_ENDA,
			   1 << SUN8I_CODEC_DAC_DIG_CTRL_ENDA);

	/* Setup the DAC Mixer */
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_DAC_MXR_SRC,
			   0xF << SUN8I_CODEC_DAC_MXR_SRC_DACL_MXR_SRC,
			   0x8 << SUN8I_CODEC_DAC_MXR_SRC_DACL_MXR_SRC);
	regmap_update_bits(scodec->regmap, SUN8I_CODEC_DAC_MXR_SRC,
			   0xF << SUN8I_CODEC_DAC_MXR_SRC_DACR_MXR_SRC,
			   0x8 << SUN8I_CODEC_DAC_MXR_SRC_DACR_MXR_SRC);

	/* The registers below are 8 bit registers accessed from the APB0 Bus */

	/* Volume control */
	sun8i_codec_write_prcm_wvalue(SUN8I_CODEC_HP_VOLC, 0x3A);
	
	/* Left Mixer Channel */
//	sun8i_codec_write_prcm_wvalue(SUN8I_CODEC_LOMIXSC, 0x03);

	/* Right Mixer Channel */
//	sun8i_codec_write_prcm_wvalue(SUN8I_CODEC_ROMIXSC, 0x03);

	/* DAC Analog Enable and PA Source Control Register */		
	sun8i_codec_write_prcm_wvalue(SUN8I_CODEC_DAC_PA_SRC, 0xCC);
	
	/* PA Enable and HP Control Register */
	sun8i_codec_write_prcm_wvalue(SUN8I_CODEC_PAEN_HP_CTRL, 0x84);

	/* Turn on the PA using gpio */
	gpiod_direction_output(scodec->gpio_pa, 1);
	gpiod_set_value(scodec->gpio_pa, 1);	

	return 0;

err_unregister_codec:
	snd_soc_unregister_codec(&pdev->dev);
err_clk_disable:
	clk_disable_unprepare(scodec->clk_apb);
	return ret;
}

static int sun8i_codec_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct sun8i_codec *scodec = snd_soc_card_get_drvdata(card);

	snd_soc_unregister_card(card);
	snd_soc_unregister_codec(&pdev->dev);
	clk_disable_unprepare(scodec->clk_apb);

	return 0;
}

static struct platform_driver sun8i_codec_driver = {
	.driver = {
		.name = "sun8i-codec",
		.of_match_table = sun8i_codec_of_match,
	},
	.probe = sun8i_codec_probe,
	.remove = sun8i_codec_remove,
};
module_platform_driver(sun8i_codec_driver);

MODULE_DESCRIPTION("Allwinner A33 codec driver");
MODULE_AUTHOR("Lawrence Yu <lyu@micile.com>");
MODULE_AUTHOR("Emilio LÃ³pez <emilio@elopez.com.ar>");
MODULE_AUTHOR("Jon Smirl <jonsmirl@gmail.com>");
MODULE_AUTHOR("Maxime Ripard <maxime.ripard@free-electrons.com>");
MODULE_LICENSE("GPL");
