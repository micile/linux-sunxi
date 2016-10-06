/*
 * sound\soc\sunxi\audiocodec\sun8iw5-sndcodec.h
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#ifndef _SUN8IW5_CODEC_H
#define _SUN8IW5_CODEC_H

/* PRCM registers */
#define HP_VOLC                                          (0x00)
#define LOMIXSC                                          (0x01)
#define ROMIXSC                                          (0x02)
#define DAC_PA_SRC                               (0x03)
#define PHONEIN_GCTRL                    (0x04)
#define LINEIN_GCTRL                     (0x05)
#define MICIN_GCTRL                              (0x06)
#define PAEN_HP_CTRL                     (0x07)
#define PHONEOUT_CTRL                    (0x08)
#define LINEOUT_VOLC                     (0x09)
#define MIC2G_LINEEN_CTRL                (0x0A)
#define MIC1G_MICBIAS_CTRL               (0x0B)
#define LADCMIXSC                                (0x0C)
#define RADCMIXSC                                (0x0D)
#define ADC_AP_EN                                (0x0F)
#define ADDA_APT0                                (0x10)
#define ADDA_APT1                                (0x11)
#define ADDA_APT2                                (0x12)
#define BIAS_AD16_CAL_CTRL               (0x13)
#define BIAS_DA16_CAL_CTRL               (0x14)
#define DA16CALI                                 (0x15)
#define DA16VERIFY                               (0x16)
#define BIASCALI                                 (0x17)
#define BIASVERIFY                               (0x18)

/*
*      apb0 base
*      0x00 HP_VOLC
*/
#define PA_CLK_GC              (7)
#define HPVOL                  (0)

/*
*      apb0 base
*      0x01 LOMIXSC
*/
#define LMIXMUTE                                 (0)
#define LMIXMUTEDACR                     (0)
#define LMIXMUTEDACL                     (1)
#define LMIXMUTELINEINL                          (2)
#define LMIXMUTEPHONEN                   (3)
#define LMIXMUTEPHONEPN                          (4)
#define LMIXMUTEMIC2BOOST                (5)
#define LMIXMUTEMIC1BOOST                (6)

/*
*      apb0 base
*      0x02 ROMIXSC
*/
#define RMIXMUTE                                 (0)
#define RMIXMUTEDACL                     (0)
#define RMIXMUTEDACR                     (1)
#define RMIXMUTELINEINR                          (2)
#define RMIXMUTEPHONEP                   (3)
#define RMIXMUTEPHONEPN                          (4)
#define RMIXMUTEMIC2BOOST                (5)
#define RMIXMUTEMIC1BOOST                (6)

/*
*      apb0 base
*      0x03 DAC_PA_SRC
*/
#define DACAREN                        (7)
#define DACALEN                        (6)
#define RMIXEN                 (5)
#define LMIXEN                 (4)
#define RHPPAMUTE              (3)
#define LHPPAMUTE              (2)
#define RHPIS                  (1)
#define LHPIS                  (0)

/*
*      apb0 base
*      0x04 PHONEIN_GCTRL
*/
#define PHONEPG                        (4)
#define PHONENG                        (0)

/*
*      apb0 base
*      0x05 LINEIN_GCTRL
*/
#define LINEING                        (4)
#define PHONEG                 (0)

/*
*      apb0 base
*      0x06 MICIN_GCTRL
*/
#define MIC1G                  (4)
#define MIC2G                  (0)

/*
*      apb0 base
*      0x07 PAEN_HP_CTRL
*/
#define HPPAEN                  (7)
#define HPCOM_FC                (5)
#define COMPTEN                         (4)
#define PA_ANTI_POP_CTRL (2)
#define LTRNMUTE                (1)
#define RTLNMUTE                (0)

/*
*      apb0 base
*      0x08 PHONEOUT_CTRL
*/
#define PHONEOUTG               (5)
#define PHONEOUT_EN             (4)
#define PHONEOUTS3              (3)
#define PHONEOUTS2              (2)
#define PHONEOUTS1              (1)
#define PHONEOUTS0              (0)

/*
*      apb0 base
*      0x09 LINEOUT_VOLC
*/
#define LINEOUTVOL              (3)
#define PHONEPREG               (0)

/*
*      apb0 base
*      0x0A MIC2G_LINEEN_CTRL
*/
#define MIC2AMPEN               (7)
#define MIC2BOOST               (4)
#define LINEOUTL_EN             (3)
#define LINEOUTR_EN             (2)
#define LINEOUTL_SS             (1)
#define LINEOUTR_SS             (0)

/*
*      apb0 base
*      0x0B MIC1G_MICBIAS_CTRL
*/
#define HMICBIASEN              (7)
#define MMICBIASEN              (6)
#define HMICBIAS_MODE   (5)
#define MIC2_SS                         (4)
#define MIC1AMPEN               (3)
#define MIC1BOOST               (0)

/*
*      apb0 base
*      0x0C LADCMIXSC
*/
#define LADCMIXMUTE                              (0)
#define LADCMIXMUTEMIC1BOOST     (6)
#define LADCMIXMUTEMIC2BOOST     (5)
#define LADCMIXMUTEPHONEPN               (4)
#define LADCMIXMUTEPHONEN                (3)
#define LADCMIXMUTELINEINL               (2)
#define LADCMIXMUTELOUTPUT               (1)
#define LADCMIXMUTEROUTPUT               (0)

/*
*      apb0 base
*      0x0D RADCMIXSC
*/
#define RADCMIXMUTE                      (0)
#define RADCMIXMUTEMIC1BOOST     (6)
#define RADCMIXMUTEMIC2BOOST     (5)
#define RADCMIXMUTEPHONEPN               (4)
#define RADCMIXMUTEPHONEP                (3)
#define RADCMIXMUTELINEINR               (2)
#define RADCMIXMUTEROUTPUT               (1)
#define RADCMIXMUTELOUTPUT               (0)

/*
*      apb0 base
*      0x0E PA_ANTI_POP_CTRL_SLOP
*/
#define PA_ANTI_POP_CTRL_SLOP    (0)

/*
*      apb0 base
*      0x0F ADC_AP_EN
*/
#define ADCREN                  (7)
#define ADCLEN                  (6)
#define ADCG                    (0)

/*
*      apb0 base
*      0x10 ADDA_APT0
*/
#define OPDRV_OPCOM_CUR        (6)
#define OPADC1_BIAS_CUR        (4)
#define OPADC2_BIAS_CUR        (2)
#define OPAAF_BIAS_CUR (0)

/*
*      apb0 base
*      0x11 ADDA_APT1
*/
#define OPMIC_BIAS_CUR (6)
#define OPVR_BIAS_CUR  (4)
#define OPDAC_BIAS_CUR (2)
#define OPMIX_BIAS_CUR (0)

/*
*      apb0 base
*      0x12 ADDA_APT2
*/
#define ZERO_CROSS_EN                  (7)
#define TIMEOUT_ZERO_CROSS_EN  (6)
#define PTDBS                                  (4)
#define PA_SLOPE_SELECT                        (3)
#define USB_BIAS_CUR                   (0)

extern int snd_codec_info_volsw(struct snd_kcontrol *kcontrol,
				struct  snd_ctl_elem_info *uinfo);
extern int snd_codec_get_volsw(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol);
extern int snd_codec_put_volsw(struct  snd_kcontrol *kcontrol,
			       struct  snd_ctl_elem_value *ucontrol);

/*
* Convenience kcontrol builders
*/
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

#endif
