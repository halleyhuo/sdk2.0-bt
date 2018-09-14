/**
 **************************************************************************************
 * @file    audio_core.h
 * @brief   audio core 
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2016-6-29 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __AUDIO_CORE_H__
#define __AUDIO_CORE_H__

typedef enum
{
	AC_SOURCE,
	AC_CPROC,
	AC_SINK
}ACPos;

typedef enum
{
	 AC_SRC_DECODE,
	 AC_SRC_MIC,
	 AC_SRC_LINEIN
}ACSrcType;
 
typedef enum
{
	 AC_PROC_NONE,
	 AC_PROC_MIX,
	 AC_PROC_AEC
}ACProcType;
 
typedef enum
{
	 AC_SNK_DAC,
	 AC_SNK_IIS
}ACSnkType;

typedef enum
{
	EffectTypeVB,
	EffectType3D,
	EffectTypeEQ,
}EffectType;

typedef struct _ACPcmInfo
{
	uint16_t		channelNums;
	uint16_t		pcmDataMode;
	uint32_t		samplingRate;
	uint32_t		bitRate;
}ACPcmInfo;


typedef struct _ParamsVB
{
	int32_t			cutoffFreq;
	int32_t			intensity;
	int32_t			enhanced;
}ParamsVB;

typedef struct _Params3D
{
	int32_t			depth;
	int32_t			preGain;
	int32_t			postGain;
}Params3D;

typedef struct _ParamsEQ
{

}ParamsEQ;

typedef uint16_t (*ACGetDataFunc)(int16_t * buf, uint16_t samples, uint16_t channelNums);
typedef uint16_t (*ACPutDataFunc)(int16_t * buf, uint16_t samples, uint16_t channelNums);


void AudioCoreSourceRegister(ACSrcType srcType, ACGetDataFunc getSrcData, ACPcmInfo *pcmInfo);

void AudioCoreSourceDeregister(uint8_t srcIndex);

int16_t AudioCoreSourceEnable(uint8_t srcIndex);

int16_t AudioCoreSourceDisable(uint8_t srcIndex);

int16_t AudioCoreSourceStatusGet(uint8_t srcIndex);

int16_t AudioCoreSourceGainSet(uint8_t srcIndex, uint16_t gain);

int16_t AudioCoreSourceGainGet(uint8_t srcIndex);

int16_t AudioCoreSourceEffectAdd(uint8_t srcIndex, EffectType effectType);

int16_t AudioCoreSourceEffectDel(uint8_t srcIndex, uint8_t effectIndex);

int16_t AudioCoreSourceEffectEnable(uint8_t srcIndex, uint8_t effectIndex);

int16_t AudioCoreSourceEffectDisable(uint8_t srcIndex, uint8_t effectIndex);

int16_t AudioCoreSourceEffectSet(uint8_t srcIndex, uint8_t effectIndex, EffectType effectType, int32_t *effectParams);

int16_t AudioCoreSourceEffectQuery(uint8_t srcIndex, uint8_t effectIndex);

int16_t AudioCoreProcSet(ACProcType procType, ACPcmInfo *pcmInfo);

int16_t AudioCoreProcGet(ACProcType *procType);

int16_t AudioCoreProcGainSet(uint16_t gain);

int16_t AudioCoreProcGainGet(uint16_t *gain);

int16_t AudioCoreProcEffectAdd(EffectType effectType);

int16_t AudioCoreProcEffectDel(uint8_t effectIndex);

int16_t AudioCoreProcEffectEnable(uint8_t effectIndex);

int16_t AudioCoreProcEffectDisable(uint8_t effectIndex);

int16_t AudioCoreProcEffectSet(uint8_t effectIndex, EffectType effectType, uint32_t *effectParams);

int16_t AudioCoreProcEffectQuery(uint8_t effectIndex);

int16_t AudioCoreSinkRegister(ACSnkType snkType, ACPutDataFunc putSnkData, uint8_t * snkBuf, uint32_t snkBufSize, ACPcmInfo *pcmInfo);

int16_t AudioCoreSinkDeregister(uint8_t snkIndex);

int16_t AudioCoreSinkEnable(uint8_t snkIndex);

int16_t AudioCoreSinkDisable(uint8_t snkIndex);

int16_t AudioCoreSinkStatusGet(uint8_t snkIndex, uint8_t *status);

int16_t AudioCoreSinkGainSet(uint8_t snkIndex, uint16_t gain);

int16_t AudioCoreSinkGainGet(uint8_t snkIndex, uint16_t *gain);


BOOL AudioCoreInit(void);

void AudioCoreDeinit(void);

void AudioCoreRun(void);

#endif
