/**
 **************************************************************************************
 * @file    audio_core.c
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
#include <string.h>

#include "type.h"
#include "freertos.h"
#include "audio_core.h"
#include "audio_core_prot.h"

#include "virtual_bass.h"
#include "three_d.h"
#include "eq.h"


#define DEFAULT_SRC_GET_SAMPLES_LEN					192


#define DEFAULT_VB_CUTOFF_FREQ						100
#define DEFAULT_VB_INTENSITY						35
#define DEFAULT_VB_ENHANCED							1

#define DEFAULT_3D_DEPTH							50
#define DEFAULT_3D_PREGAIN							32768
#define DEFAULT_3D_POSTGAIN							32768


typedef enum
{
	AC_RUN_GET,
	AC_RUN_PROC,
	AC_RUN_SAVE,
	AC_RUN_PUT,
}AudioCoreRunState;

typedef struct _Effect
{
	EffectType			type;
	uint8_t				index;
	BOOL				enable;
	uint32_t			gain;
	union
	{
		ParamsVB		vbParams;
		Params3D		tdParams;
		ParamsEQ		eqParams;
	}effectParams;

	// internal use	
	BOOL				inited;
	union
	{
		VBContext		vbContext;
		ThreeDContext	tdContext;
		SwEqContext		eqContext;
	} effectContext;
	
}Effect;

typedef struct _EffectNode
{
	struct _EffectNode	*next;
	struct _EffectNode	*pre;
	Effect				effect;
}EffectNode;


typedef struct _ACEffectList
{
	uint8_t			effectNums;
	EffectNode		*head;
	EffectNode		*tail;
}ACEffectList;

typedef struct _AudioCoreSource
{
	ACSrcType				srcType;
	uint8_t					index;
	BOOL					enable;
	uint16_t				gain;
	ACGetDataFunc			funcGetData;
	ACPcmInfo				pcmInfo;
	ACEffectList			effectList;
	uint8_t					*buf;
	uint16_t				vaildSampleLen;
}AudioCoreSource;

typedef struct _AudioCoreSourceNode
{
	struct _AudioCoreSourceNode * next;
	struct _AudioCoreSourceNode * pre;
	AudioCoreSource			acSrc;
}AudioCoreSourceNode;

typedef struct _AudioCoreSourceList
{
	uint8_t					srcNums;
	AudioCoreSourceNode		*head;
	AudioCoreSourceNode		*tail;
}AudioCoreSourceList;



typedef struct _AudioCoreProcess
{
	ACProcType				type;
	uint16_t				gain;
	ACPcmInfo				pcmInfo;
	ACEffectList			effectList;
	uint16_t				samplesLen;
	uint8_t					*buf;
}AudioCoreProcess;

typedef enum
{
	SINK_PUT_NONE,
	SINK_PUT_TRANSING
}SinkPutState;

typedef struct _AudioCoreSink
{
	ACSnkType				type;
	uint8_t					index;
	BOOL					enable;
	uint16_t				gain;
	ACPutDataFunc			funcPutData;
//	BOOL					sinkDone;
	SinkPutState			sinkPutState;
//	ACEffectList			effectList;
	ACPcmInfo				pcmInfo;
	uint8_t					*buf;
	uint32_t				bufSize;
	uint32_t				bufDataLen;
	uint16_t				bufSampleLen;
}AudioCoreSink;

typedef struct _AudioCoreSinkNode
{
	struct _AudioCoreSinkNode * next;
	struct _AudioCoreSinkNode * pre;
	AudioCoreSink				acSnk;
}AudioCoreSinkNode;

typedef struct _AudioCoreSinkList
{
	uint8_t					snkNums;
	AudioCoreSinkNode		*head;
	AudioCoreSinkNode		*tail;
}AudioCoreSinkList;

typedef enum
{
	AC_CMD_SOURCE_QUERY,
	AC_CMD_PROC_QUERY,
	AC_CMD_SINK_QUERY,
	AC_CMD_SOURCE_REGISTER,
	AC_CMD_SOURCE_CONFIG_PCM_INFO,
	AC_CMD_SOURCE_DEREGISTER,
	AC_CMD_SOURCE_ENABLE,
	AC_CMD_SOURCE_DISABLE,
	AC_CMD_SOURCE_STATUS_GET,
	AC_CMD_SOURCE_GAIN_SET,
	AC_CMD_SOURCE_GAIN_GET,
	AC_CMD_SOURCE_EFFECT_ADD,
	AC_CMD_SOURCE_EFFECT_DEL,
	AC_CMD_SOURCE_EFFECT_ENABLE,
	AC_CMD_SOURCE_EFFECT_DISABLE,
	AC_CMD_SOURCE_EFFECT_SET,
	AC_CMD_SOURCE_EFFECT_QUERY,
	AC_CMD_SOURCE_EFFECT_DESC_QUERY,
	AC_CMD_PROC_SET,
	AC_CMD_PROC_GET,
	AC_CMD_PROC_GAIN_SET,
	AC_CMD_PROC_GAIN_GET,
	AC_CMD_PROC_EFFECT_ADD,
	AC_CMD_PROC_EFFECT_DEL,
	AC_CMD_PROC_EFFECT_ENABLE,
	AC_CMD_PROC_EFFECT_DISABLE,
	AC_CMD_PROC_EFFECT_SET,
	AC_CMD_PROC_EFFECT_QUERY,
	AC_CMD_PROC_EFFECT_DESC_QUERY,
	AC_CMD_SINK_REGISTER,
	AC_CMD_SINK_DEREGISTER,
	AC_CMD_SINK_ENABLE,
	AC_CMD_SINK_DISABLE,
	AC_CMD_SINK_GAIN_SET,
	AC_CDM_SINK_GAIN_GET
}ACCommandType;

typedef struct _AudioCoreCommand
{
	ACCommandType		cmdType;

	
	union{

		
		// source register command
		struct _SrcRegister
		{
			ACSrcType srcType;
			ACGetDataFunc getSrcData;
			ACPcmInfo pcmInfo;
		} srcRegister;

		uint8_t			srcIndex;

		struct _SrcPcmInfo
		{
			uint8_t 	srcIndex;
			ACPcmInfo 	pcmInfo;
		} srcPcmInfo;

		struct _GainSet
		{
			uint8_t		srcIndex;
			uint16_t	gain;
		} gainSet;

		struct _SrcEffectAdd
		{
			uint8_t		srcIndex;
			EffectType	effectType;
		} srcEffectAdd;

		struct _SrcEffect
		{
			uint8_t		srcIndex;
			uint8_t		effectIndex;
		} srcEffect;

		struct _SrcEffectSet
		{
			uint8_t		srcIndex;
			uint8_t		effectIndex;
			EffectType	effectType;
			union
			{
				ParamsVB	vbParams;
				Params3D	tdParams;
				ParamsEQ	eqParams;
			} effectParams;
		} srcEffectSet;

		struct _ProcSet
		{
			ACProcType		procType;
			ACPcmInfo		pcmInfo;
		} procSet;

		EffectType	effectType;

		uint8_t		effectIndex;

		struct _ProcEffectSet
		{
			uint8_t		effectIndex;
			EffectType	effectType;
			union
			{
				ParamsVB	vbParams;
				Params3D	tdParams;
				ParamsEQ	eqParams;
			} effectParams;
		} procEffectSet;

		struct _SnkRegister
		{
			ACSnkType		snkType;
			ACPutDataFunc	putSnkData;
			uint8_t			*snkBuf;
			uint32_t		snkBufSize;
			ACPcmInfo		pcmInfo;
		} snkRegister;

		uint8_t		snkIndex;
	}cmdParams;
	
}AudioCoreCommand;

typedef struct _ACCommandNode
{
	struct _ACCommandNode	*next;
	struct _ACCommandNode	*pre;
	AudioCoreCommand		command;
}ACCommandNode;

typedef struct _ACCommandQueue
{
	ACCommandNode		*head;
	ACCommandNode		*tail;
}ACCommandQueue;

typedef struct _AudioCoreContext
{
	AudioCoreSourceList		srcList;
	AudioCoreProcess		proc;
	AudioCoreSinkList		snkList;
	ACCommandQueue			cmdQueue;
	AudioCoreRunState		runState;
}AudioCoreContext;



static AudioCoreContext		audioCore;


////////////////////////////////////////////////////////////////////////////////////////////////
//
//
static void EffectProcess(ACEffectList *effectList, int16_t * buf, uint32_t sampleLen, ACPcmInfo *pcmInfo)
{
	EffectNode			*node;

	node = effectList->head;
	while(node)
	{
		if(node->effect.enable)
		{
			switch(node->effect.type)
			{
				case EffectTypeVB:
					if(!node->effect.inited)
					{
						if(vb_init(&node->effect.effectContext.vbContext,
									pcmInfo->channelNums,
									pcmInfo->samplingRate,
									node->effect.effectParams.vbParams.cutoffFreq) == VB_ERROR_OK)
						{
							node->effect.inited = TRUE;
						}
					}

					if(node->effect.inited)
					{
						vb_apply(&node->effect.effectContext.vbContext,
								(int16_t*)buf,
								(int16_t*)buf,
								sampleLen,
								node->effect.effectParams.vbParams.intensity,
								node->effect.effectParams.vbParams.enhanced);
					}
					break;

				case EffectType3D:
					if(!node->effect.inited)
					{
						if(init_3d(&node->effect.effectContext.tdContext,
									pcmInfo->channelNums,
									pcmInfo->samplingRate) == THREE_D_ERROR_OK)
						{
							node->effect.inited = TRUE;
						}
					}
					if(node->effect.inited)
					{
						apply_3d(&node->effect.effectContext.tdContext,
								(int16_t*)buf,
								(int16_t*)buf,
								sampleLen,
								node->effect.effectParams.tdParams.depth,
								node->effect.effectParams.tdParams.preGain,
								node->effect.effectParams.tdParams.postGain);
					}
					break;
			}
		}
		node = node->next;
	}
}

static void QueueCommand(ACCommandNode * cmd)
{
	if(cmd == NULL)
		return;

	if(audioCore.cmdQueue.head == NULL)
	{
		audioCore.cmdQueue.head = cmd;
		audioCore.cmdQueue.tail = cmd;
	}
	else
	{
		audioCore.cmdQueue.tail->next = cmd;
		audioCore.cmdQueue.tail = cmd;
	}
}

static ACCommandNode * DequeueCommand(void)
{
	ACCommandNode	*node;

	node = audioCore.cmdQueue.head;

	if(node != NULL)
		audioCore.cmdQueue.head = audioCore.cmdQueue.head->next;

	return node;
}

static int16_t AC_SourceQuery(void)
{
	AudioCoreSourceNode		*srcNode;
	
	AudioCoreReport(AC_PROT_CODE_SOURCE_NUMS, audioCore.srcList.srcNums);

	srcNode = audioCore.srcList.head;

	while(srcNode)
	{
		AudioCoreReport(AC_PROT_CODE_SOURCE_DESC,
						srcNode->acSrc.srcType,
						srcNode->acSrc.index,
						srcNode->acSrc.enable,
						srcNode->acSrc.effectList.effectNums);
		srcNode = srcNode->next;
	}

	return 0;
}

void AudioCoreSourceQuery(void)
{
	ACCommandNode			*node;

	node = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	node->next = NULL;
	node->pre = NULL;
	node->command.cmdType = AC_CMD_SOURCE_QUERY;

	QueueCommand(node);
}

static int16_t AC_ProcQuery(void)
{
		AudioCoreReport(AC_PROT_CODE_CPROC_DESC,
						audioCore.proc.type,
						audioCore.proc.effectList.effectNums);

	return 0;
}

void AudioCoreProcQuery(void)
{
	ACCommandNode			*node;

	node = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	node->next = NULL;
	node->pre = NULL;
	node->command.cmdType = AC_CMD_PROC_QUERY;

	QueueCommand(node);
}

static int16_t AC_SinkQuery(void)
{
	AudioCoreSinkNode		*snkNode;
	
	AudioCoreReport(AC_PROT_CODE_SINK_NUMS, audioCore.snkList.snkNums);

	snkNode = audioCore.snkList.head;

	while(snkNode)
	{
		AudioCoreReport(AC_PROT_CODE_SINK_DESC,
						snkNode->acSnk.type,
						snkNode->acSnk.index,
						snkNode->acSnk.enable);
		snkNode = snkNode->next;
	}

	return 0;
}

void AudioCoreSinkQuery(void)
{
	ACCommandNode			*node;

	node = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	node->next = NULL;
	node->pre = NULL;
	node->command.cmdType = AC_CMD_SINK_QUERY;

	QueueCommand(node);
}

static int16_t AC_SourceRegister(ACSrcType srcType, ACGetDataFunc getSrcData, ACPcmInfo *pcmInfo)
{
	AudioCoreSourceNode		*acSrcNode;
	uint8_t					srcIndex;
	uint32_t				srcBufSize;

	acSrcNode = (AudioCoreSourceNode *)pvPortMalloc(sizeof(AudioCoreSourceNode));
	if(acSrcNode == NULL)
		return -1;

	acSrcNode->next = NULL;
	acSrcNode->pre = NULL;

	srcBufSize = DEFAULT_SRC_GET_SAMPLES_LEN * pcmInfo->channelNums * 2;
	acSrcNode->acSrc.buf = (uint8_t *)pvPortMalloc(srcBufSize);
	if(acSrcNode->acSrc.buf == NULL)
	{
		vPortFree(acSrcNode);
		return -2;
	}
	memset(acSrcNode->acSrc.buf, 0, srcBufSize);

	acSrcNode->acSrc.srcType = srcType;
	acSrcNode->acSrc.enable = TRUE;
	acSrcNode->acSrc.gain = 0;
	acSrcNode->acSrc.funcGetData = getSrcData;
	acSrcNode->acSrc.effectList.effectNums = 0;
	acSrcNode->acSrc.effectList.head = NULL;
	acSrcNode->acSrc.effectList.tail = NULL;
	memcpy(&acSrcNode->acSrc.pcmInfo, pcmInfo, sizeof(ACPcmInfo));

	if(audioCore.srcList.srcNums == 0) /* First node*/
	{
		/* set Handle */
		acSrcNode->acSrc.index = srcIndex = 1;

		audioCore.srcList.head = acSrcNode;
		audioCore.srcList.tail = acSrcNode;
		audioCore.srcList.srcNums = 1;
	}
	else
	{
		acSrcNode->acSrc.index = srcIndex = audioCore.srcList.tail->acSrc.index + 1;

		acSrcNode->pre = audioCore.srcList.tail;
		audioCore.srcList.tail->next = acSrcNode;
		audioCore.srcList.tail = acSrcNode;
		audioCore.srcList.srcNums++;
	}

	return srcIndex;

}

void AudioCoreSourceRegister(ACSrcType srcType, ACGetDataFunc getSrcData, ACPcmInfo *pcmInfo)
{
	ACCommandNode			*node;

	node = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	node->next = NULL;
	node->pre = NULL;
	node->command.cmdType = AC_CMD_SOURCE_REGISTER;
	node->command.cmdParams.srcRegister.srcType = srcType;
	node->command.cmdParams.srcRegister.getSrcData = getSrcData;
	memcpy(&node->command.cmdParams.srcRegister.pcmInfo, pcmInfo, sizeof(ACPcmInfo));

	QueueCommand(node);
}

static int16_t AC_SourceConfigPcmInfo(uint8_t srcIndex, ACPcmInfo *pcmInfo)
{
	AudioCoreSourceNode			*tmpSrcNode;

	tmpSrcNode = audioCore.srcList.head;
	while(tmpSrcNode)
	{
		if(tmpSrcNode->acSrc.index == srcIndex)
			break;

		tmpSrcNode = tmpSrcNode->next;
	}

	if(tmpSrcNode == NULL)
		return -1;

	memcpy(&tmpSrcNode->acSrc.pcmInfo, pcmInfo, sizeof(ACPcmInfo));
	
	return 0;
}

void AudioCoreSourceConfigPcmInfo(uint8_t srcIndex, ACPcmInfo *pcmInfo)
{
	ACCommandNode			*node;

	node = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	node->next = NULL;
	node->pre = NULL;
	node->command.cmdType = AC_CMD_SOURCE_CONFIG_PCM_INFO;
	node->command.cmdParams.srcPcmInfo.srcIndex = srcIndex;
	memcpy(&node->command.cmdParams.srcPcmInfo.pcmInfo, pcmInfo, sizeof(ACPcmInfo));

	QueueCommand(node);
}

static int16_t AC_SourceDeregister(uint8_t srcIndex)
{
	AudioCoreSourceNode			*tmpSrcNode;


	tmpSrcNode = audioCore.srcList.head;
	while(tmpSrcNode)
	{
		if(tmpSrcNode->acSrc.index == srcIndex)
			break;

		tmpSrcNode = tmpSrcNode->next;
	}

	if(tmpSrcNode != NULL)
	{
		if(tmpSrcNode == audioCore.srcList.head)		// head node
		{
			audioCore.srcList.head = tmpSrcNode->next;
			if(tmpSrcNode->next != NULL)
				tmpSrcNode->next->pre = NULL;
		}
		else if(tmpSrcNode == audioCore.srcList.tail)	// tail node
		{
			audioCore.srcList.tail = tmpSrcNode->pre;
			audioCore.srcList.tail->next = NULL;
		}
		else
		{
			AudioCoreSourceNode		*preNode;
			AudioCoreSourceNode		*nextNode;

			preNode = tmpSrcNode->pre;
			nextNode = tmpSrcNode->next;

			preNode->next = nextNode;
			nextNode->pre = preNode;
		}

		vPortFree(tmpSrcNode);
		audioCore.srcList.srcNums--;
	}
	else
	{
		return -1;
	}

	return 0;
}

void AudioCoreSourceDeregister(uint8_t srcIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SOURCE_DEREGISTER;

	cmdNode->command.cmdParams.srcIndex = srcIndex;
	QueueCommand(cmdNode);

}
static int16_t AC_SourceEnable(uint8_t srcIndex)
{
	AudioCoreSourceNode			*tmpSrcNode;

	tmpSrcNode = audioCore.srcList.head;
	while(tmpSrcNode)
	{
		if(tmpSrcNode->acSrc.index == srcIndex)
			break;

		tmpSrcNode = tmpSrcNode->next;
	}

	if(tmpSrcNode == NULL)
		return -1;

	tmpSrcNode->acSrc.enable = TRUE;

	return 0;	
}
int16_t AudioCoreSourceEnable(uint8_t srcIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SOURCE_ENABLE;

	cmdNode->command.cmdParams.srcIndex = srcIndex;
	QueueCommand(cmdNode);


	return 0;
}

static int16_t AC_SourceDisable(uint8_t srcIndex)
{
	AudioCoreSourceNode			*tmpSrcNode;

	tmpSrcNode = audioCore.srcList.head;
	while(tmpSrcNode)
	{
		if(tmpSrcNode->acSrc.index == srcIndex)
			break;

		tmpSrcNode = tmpSrcNode->next;
	}

	if(tmpSrcNode == NULL)
		return -1;

	tmpSrcNode->acSrc.enable = FALSE;

	return 0;
}

int16_t AudioCoreSourceDisable(uint8_t srcIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SOURCE_DISABLE;

	cmdNode->command.cmdParams.srcIndex = srcIndex;
	QueueCommand(cmdNode);

	return 0;
}

static int16_t AC_SourceStatusGet(uint8_t srcIndex, uint8_t *status)
{
	AudioCoreSourceNode			*tmpSrcNode;

	tmpSrcNode = audioCore.srcList.head;
	while(tmpSrcNode)
	{
		if(tmpSrcNode->acSrc.index == srcIndex)
			break;

		tmpSrcNode = tmpSrcNode->next;
	}

	if(tmpSrcNode == NULL)
		return -1;

	*status = tmpSrcNode->acSrc.enable;

	return 0;	
}

int16_t AudioCoreSourceStatusGet(uint8_t srcIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SOURCE_STATUS_GET;
	cmdNode->command.cmdParams.srcIndex = srcIndex;
	QueueCommand(cmdNode);

	return 0;
}

static int16_t AC_SourceGainSet(uint8_t srcIndex, uint16_t gain)
{
	AudioCoreSourceNode			*tmpSrcNode;

	tmpSrcNode = audioCore.srcList.head;
	while(tmpSrcNode)
	{
		if(tmpSrcNode->acSrc.index == srcIndex)
			break;

		tmpSrcNode = tmpSrcNode->next;
	}

	if(tmpSrcNode == NULL)
		return -1;

	tmpSrcNode->acSrc.gain = gain;

	return 0;
}

int16_t AudioCoreSourceGainSet(uint8_t srcIndex, uint16_t gain)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SOURCE_GAIN_SET;
	cmdNode->command.cmdParams.gainSet.srcIndex = srcIndex;
	cmdNode->command.cmdParams.gainSet.gain = gain;
	QueueCommand(cmdNode);

	return 0;
}

static int16_t AC_SourceGainGet(uint8_t srcIndex, uint16_t *gain)
{
	AudioCoreSourceNode			*tmpSrcNode;

	tmpSrcNode = audioCore.srcList.head;
	while(tmpSrcNode)
	{
		if(tmpSrcNode->acSrc.index == srcIndex)
			break;

		tmpSrcNode = tmpSrcNode->next;
	}

	if(tmpSrcNode == NULL)
		return -1;

	*gain = tmpSrcNode->acSrc.gain;

	return 0;
}

int16_t AudioCoreSourceGainGet(uint8_t srcIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SOURCE_GAIN_GET;
	cmdNode->command.cmdParams.srcIndex = srcIndex;
	QueueCommand(cmdNode);

	return 0;
}

static int16_t AC_SourceEffectAdd(uint8_t srcIndex, EffectType effectType)
{
	AudioCoreSourceNode		*tmpSrcNode;
	EffectNode				*effectNode;
	uint8_t					effectIndex;

	// Find the source 
	tmpSrcNode = audioCore.srcList.head;
	while(tmpSrcNode)
	{
		if(tmpSrcNode->acSrc.index == srcIndex)
			break;

		tmpSrcNode = tmpSrcNode->next;
	}

	if(tmpSrcNode == NULL)
		return -1;

	// Make & init an effect node
	effectNode = (EffectNode*)pvPortMalloc(sizeof(EffectNode));
	if(effectNode == NULL)
	{
		return -2;
	}
	
	effectNode->pre = NULL;
	effectNode->next = NULL;
	effectNode->effect.type = effectType;
	effectNode->effect.enable = TRUE;
	effectNode->effect.gain = 0;
	switch(effectType)
	{
		case EffectTypeVB:
			effectNode->effect.effectParams.vbParams.cutoffFreq	= DEFAULT_VB_CUTOFF_FREQ;
			effectNode->effect.effectParams.vbParams.intensity	= DEFAULT_VB_INTENSITY;
			effectNode->effect.effectParams.vbParams.enhanced	= DEFAULT_VB_ENHANCED;
			break;

		case EffectType3D:
			effectNode->effect.effectParams.tdParams.depth		= DEFAULT_3D_DEPTH;
			effectNode->effect.effectParams.tdParams.preGain	= DEFAULT_3D_PREGAIN;
			effectNode->effect.effectParams.tdParams.postGain	= DEFAULT_3D_POSTGAIN;
			break;

		case EffectTypeEQ:
			break;

		default:
			break;
	}
	effectNode->effect.inited = FALSE;

	// Add the new effect to the effectList
	if(tmpSrcNode->acSrc.effectList.effectNums == 0)
	{
		effectNode->effect.index = effectIndex = 1;
		tmpSrcNode->acSrc.effectList.head = effectNode;
		tmpSrcNode->acSrc.effectList.tail = effectNode;
		tmpSrcNode->acSrc.effectList.effectNums = 1;
	}
	else
	{
		effectNode->effect.index = effectIndex = tmpSrcNode->acSrc.effectList.tail->effect.index + 1;

		tmpSrcNode->acSrc.effectList.tail->next = effectNode;
		effectNode->pre = tmpSrcNode->acSrc.effectList.tail;
		tmpSrcNode->acSrc.effectList.tail = effectNode;
	}

	return effectIndex;

}
int16_t AudioCoreSourceEffectAdd(uint8_t srcIndex, EffectType effectType)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SOURCE_EFFECT_ADD;

	cmdNode->command.cmdParams.srcEffectAdd.srcIndex = srcIndex;
	cmdNode->command.cmdParams.srcEffectAdd.effectType = effectType;

	QueueCommand(cmdNode);

	return 0;
}
static int16_t AC_SourceEffectDel(uint8_t srcIndex, uint8_t effectIndex)
{
	AudioCoreSourceNode 	*tmpSrcNode;
	EffectNode				*tmpEffectNode;


	// Find the source 
	tmpSrcNode = audioCore.srcList.head;
	while(tmpSrcNode)
	{
		if(tmpSrcNode->acSrc.index == srcIndex)
			break;

		tmpSrcNode = tmpSrcNode->next;
	}

	if(tmpSrcNode == NULL)
		return -1;

	
	// Find the effect with Index
	tmpEffectNode = tmpSrcNode->acSrc.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;

	if(tmpEffectNode == tmpSrcNode->acSrc.effectList.head) // Head node
	{
		tmpSrcNode->acSrc.effectList.head = tmpEffectNode->next;
		if(tmpEffectNode->next != NULL)
			tmpEffectNode->next->pre = NULL;
	}
	else if(tmpEffectNode == tmpSrcNode->acSrc.effectList.tail) // tail node
	{
		tmpSrcNode->acSrc.effectList.tail = tmpEffectNode->pre;
		tmpSrcNode->acSrc.effectList.tail->next = NULL;
	}
	else
	{
		EffectNode		*preNode;
		EffectNode		*nextNode;

		preNode = tmpEffectNode->pre;
		nextNode = tmpEffectNode->next;

		preNode->next = nextNode;
		nextNode->pre = preNode;
	}

	vPortFree(tmpEffectNode);
	tmpSrcNode->acSrc.effectList.effectNums--;

	return 0;
}

int16_t AudioCoreSourceEffectDel(uint8_t srcIndex, uint8_t effectIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SOURCE_EFFECT_DEL;

	cmdNode->command.cmdParams.srcEffect.srcIndex = srcIndex;
	cmdNode->command.cmdParams.srcEffect.effectIndex = effectIndex;

	QueueCommand(cmdNode);

	return 0;
}

static int16_t AC_SourceEffectEnable(uint8_t srcIndex, uint8_t effectIndex)
{
	AudioCoreSourceNode		*tmpSrcNode;
	EffectNode				*tmpEffectNode;
	ACProtCode				protCode;


	// Find the source 
	tmpSrcNode = audioCore.srcList.head;
	while(tmpSrcNode)
	{
		if(tmpSrcNode->acSrc.index == srcIndex)
			break;

		tmpSrcNode = tmpSrcNode->next;
	}

	if(tmpSrcNode == NULL)
		return -1;

	
	// Find the effect with Index
	tmpEffectNode = tmpSrcNode->acSrc.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;


	tmpEffectNode->effect.enable = TRUE;


	switch(tmpEffectNode->effect.type)
	{
		case EffectTypeVB:
			protCode = AC_PROT_CODE_VB_ENDIS;
			break;

		case EffectType3D:
			protCode = AC_PROT_CODE_3D_ENDIS;
			break;

		case EffectTypeEQ:
			protCode = AC_PROT_CODE_EQ_ENDIS;
			break;
	}

	AudioCoreReport(protCode, AC_SOURCE, srcIndex, effectIndex, tmpEffectNode->effect.enable);

	return 0;

}

int16_t AudioCoreSourceEffectEnable(uint8_t srcIndex, uint8_t effectIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SOURCE_EFFECT_ENABLE;

	cmdNode->command.cmdParams.srcEffect.srcIndex = srcIndex;
	cmdNode->command.cmdParams.srcEffect.effectIndex = effectIndex;

	QueueCommand(cmdNode);	

	return 0;
}

static int16_t AC_SourceEffectDisable(uint8_t srcIndex, uint8_t effectIndex)
{
	AudioCoreSourceNode		*tmpSrcNode;
	EffectNode				*tmpEffectNode;
	ACProtCode				protCode;


	// Find the source 
	tmpSrcNode = audioCore.srcList.head;
	while(tmpSrcNode)
	{
		if(tmpSrcNode->acSrc.index == srcIndex)
			break;

		tmpSrcNode = tmpSrcNode->next;
	}

	if(tmpSrcNode == NULL)
		return -1;

	
	// Find the effect with Index
	tmpEffectNode = tmpSrcNode->acSrc.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;

	tmpEffectNode->effect.enable = FALSE;

	switch(tmpEffectNode->effect.type)
	{
		case EffectTypeVB:
			protCode = AC_PROT_CODE_VB_ENDIS;
			break;

		case EffectType3D:
			protCode = AC_PROT_CODE_3D_ENDIS;
			break;

		case EffectTypeEQ:
			protCode = AC_PROT_CODE_EQ_ENDIS;
			break;
	}

	AudioCoreReport(protCode, AC_SOURCE, srcIndex, effectIndex, tmpEffectNode->effect.enable);

	return 0;
}

int16_t AudioCoreSourceEffectDisable(uint8_t srcIndex, uint8_t effectIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SOURCE_EFFECT_DISABLE;

	cmdNode->command.cmdParams.srcEffect.srcIndex = srcIndex;
	cmdNode->command.cmdParams.srcEffect.effectIndex = effectIndex;

	QueueCommand(cmdNode);

	return 0;
}

static int16_t AC_SourceEffectSet(uint8_t srcIndex, uint8_t effectIndex, EffectType effectType, uint32_t *effectParams)
{
	AudioCoreSourceNode 	*tmpSrcNode;
	EffectNode				*tmpEffectNode;


	// Find the source 
	tmpSrcNode = audioCore.srcList.head;
	while(tmpSrcNode)
	{
		if(tmpSrcNode->acSrc.index == srcIndex)
			break;

		tmpSrcNode = tmpSrcNode->next;
	}

	if(tmpSrcNode == NULL)
		return -1;


	// Find the effect with Index
	tmpEffectNode = tmpSrcNode->acSrc.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;

	if(effectType != tmpEffectNode->effect.type) // check the type whether is equal
		return -3;


	switch(effectType)
	{
		case EffectTypeVB:
			{
				int32_t			oldCutoffFreq;
				int32_t			newCutoffFreq;

				oldCutoffFreq = tmpEffectNode->effect.effectParams.vbParams.cutoffFreq;
				newCutoffFreq = effectParams[0];
				
				if(oldCutoffFreq != newCutoffFreq)
				{
					// need to be re-initialize
					tmpEffectNode->effect.inited = FALSE;
				}
				tmpEffectNode->effect.effectParams.vbParams.cutoffFreq = newCutoffFreq;
				tmpEffectNode->effect.effectParams.vbParams.intensity = effectParams[1];
				tmpEffectNode->effect.effectParams.vbParams.enhanced = effectParams[2];
			}
			break;

		case EffectType3D:
			{
				int32_t 		newDepth;
				int32_t			newPreGain;
				int32_t			newPostGain;


				newDepth	= effectParams[0];
				newPreGain	= effectParams[1];
				newPostGain	= effectParams[2];

				tmpEffectNode->effect.effectParams.tdParams.depth		= newDepth;
				tmpEffectNode->effect.effectParams.tdParams.preGain		= newPreGain;
				tmpEffectNode->effect.effectParams.tdParams.postGain	= newPostGain;
			}
			break;

		case EffectTypeEQ:
			break;

		default:
			break;
	}

	return 0;
}


int16_t AudioCoreSourceEffectSet(uint8_t srcIndex, uint8_t effectIndex, EffectType effectType, int32_t *effectParams)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SOURCE_EFFECT_SET;

	cmdNode->command.cmdParams.srcEffectSet.srcIndex = srcIndex;
	cmdNode->command.cmdParams.srcEffectSet.effectIndex = effectIndex;
	cmdNode->command.cmdParams.srcEffectSet.effectType = effectType;
	switch(effectType)
	{
		case EffectTypeVB:
			cmdNode->command.cmdParams.srcEffectSet.effectParams.vbParams.cutoffFreq = effectParams[0];
			cmdNode->command.cmdParams.srcEffectSet.effectParams.vbParams.intensity = effectParams[1];
			cmdNode->command.cmdParams.srcEffectSet.effectParams.vbParams.enhanced = effectParams[2];
			break;

		case EffectType3D:
			cmdNode->command.cmdParams.srcEffectSet.effectParams.tdParams.depth = effectParams[0];
			cmdNode->command.cmdParams.srcEffectSet.effectParams.tdParams.preGain = effectParams[1];
			cmdNode->command.cmdParams.srcEffectSet.effectParams.tdParams.postGain = effectParams[2];
			break;

		case EffectTypeEQ:
			break;
	}

	QueueCommand(cmdNode);

	return 0;
}

static int16_t AC_SourceEffectQuery(uint8_t srcIndex, uint8_t effectIndex)
{
	AudioCoreSourceNode 	*tmpSrcNode;
	EffectNode				*tmpEffectNode;
	EffectType				effectType;


	// Find the source 
	tmpSrcNode = audioCore.srcList.head;
	while(tmpSrcNode)
	{
		if(tmpSrcNode->acSrc.index == srcIndex)
			break;

		tmpSrcNode = tmpSrcNode->next;
	}

	if(tmpSrcNode == NULL)
		return -1;


	// Find the effect with Index
	tmpEffectNode = tmpSrcNode->acSrc.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;

	effectType = tmpEffectNode->effect.type;

	AudioCoreReport(AC_PORT_CODE_SOURCE_EFFECTS, srcIndex, effectIndex, effectType);

	return 0;
}

int16_t AudioCoreSourceEffectQuery(uint8_t srcIndex, uint8_t effectIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SOURCE_EFFECT_QUERY;

	cmdNode->command.cmdParams.srcEffect.srcIndex = srcIndex;
	cmdNode->command.cmdParams.srcEffect.effectIndex = effectIndex;

	QueueCommand(cmdNode);

	return 0;
}


static int16_t AC_SourceEffectDescQuery(uint8_t srcIndex, uint8_t effectIndex)
{
	AudioCoreSourceNode 	*tmpSrcNode;
	EffectNode				*tmpEffectNode;
	EffectType				effectType;


	// Find the source 
	tmpSrcNode = audioCore.srcList.head;
	while(tmpSrcNode)
	{
		if(tmpSrcNode->acSrc.index == srcIndex)
			break;

		tmpSrcNode = tmpSrcNode->next;
	}

	if(tmpSrcNode == NULL)
		return -1;


	// Find the effect with Index
	tmpEffectNode = tmpSrcNode->acSrc.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;

	effectType = tmpEffectNode->effect.type;

	switch(effectType)
	{
		case EffectTypeVB:
			{
				AudioCoreReport(AC_PROT_CODE_VB_ENDIS, AC_SOURCE, srcIndex, effectIndex, tmpEffectNode->effect.enable);
				AudioCoreReport(AC_PROT_CODE_VB_PARAMS, AC_SOURCE, srcIndex, effectIndex,
								tmpEffectNode->effect.effectParams.vbParams.cutoffFreq,
								tmpEffectNode->effect.effectParams.vbParams.intensity,
								tmpEffectNode->effect.effectParams.vbParams.enhanced);
			}
			break;

		case EffectType3D:
			{
				AudioCoreReport(AC_PROT_CODE_3D_ENDIS, AC_SOURCE, srcIndex, effectIndex, tmpEffectNode->effect.enable);
				AudioCoreReport(AC_PROT_CODE_3D_PARAMS, AC_SOURCE, srcIndex, effectIndex,
								tmpEffectNode->effect.effectParams.tdParams.depth,
								tmpEffectNode->effect.effectParams.tdParams.preGain,
								tmpEffectNode->effect.effectParams.tdParams.postGain);
			}
			break;

		case EffectTypeEQ:
			break;

		default:
			break;
	}

	return 0;
}

int16_t AudioCoreSourceEffectDescQuery(uint8_t srcIndex, uint8_t effectIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SOURCE_EFFECT_DESC_QUERY;

	cmdNode->command.cmdParams.srcEffect.srcIndex = srcIndex;
	cmdNode->command.cmdParams.srcEffect.effectIndex = effectIndex;

	QueueCommand(cmdNode);

	return 0;
}


static int16_t AC_ProcSet(ACProcType procType, ACPcmInfo *pcmInfo)
{
	
	if(procType == AC_PROC_NONE)
	{
		// If procType == NONE, Deinit current process
		// Add deinit code here
		// ...
		return 0;
	}

	if(audioCore.proc.type != AC_PROC_NONE)
		return -1;

	audioCore.proc.type = procType;
	audioCore.proc.gain = 0;
	audioCore.proc.effectList.effectNums = 0;
	audioCore.proc.samplesLen = 0;
	audioCore.proc.effectList.head = NULL;
	audioCore.proc.effectList.tail = NULL;
	memcpy(&audioCore.proc.pcmInfo, pcmInfo, sizeof(ACPcmInfo));
	audioCore.proc.buf = (uint8_t *)pvPortMalloc(DEFAULT_SRC_GET_SAMPLES_LEN * pcmInfo->channelNums * 2);
	if(audioCore.proc.buf == NULL)
	{
		audioCore.proc.type = AC_PROC_NONE;
		return -1;
	}

	return 0;
}

int16_t AudioCoreProcSet(ACProcType procType, ACPcmInfo *pcmInfo)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_PROC_SET;

	cmdNode->command.cmdParams.procSet.procType = procType;
	memcpy(&cmdNode->command.cmdParams.procSet.pcmInfo, pcmInfo, sizeof(ACPcmInfo));

	QueueCommand(cmdNode);	

	return 0;
}

int16_t AudioCoreProcGet(ACProcType *procType)
{
	if(procType == NULL)
		return -1;

	*procType = audioCore.proc.type;

	return 0;
}

int16_t AudioCoreProcGainSet(uint16_t gain)
{
	audioCore.proc.gain = gain;
	return 0;
}

int16_t AudioCoreProcGainGet(uint16_t *gain)
{
	if(gain == NULL)
		return -1;

	*gain = audioCore.proc.gain;

	return 0;
}
static int16_t AC_ProcEffectAdd(EffectType effectType)
{
	EffectNode		*effectNode;
	uint8_t			effectIndex;
	
	// Make & init an effect node
	effectNode = (EffectNode*)pvPortMalloc(sizeof(EffectNode));
	if(effectNode == NULL)
	{
		return -2;
	}
	
	effectNode->pre = NULL;
	effectNode->next = NULL;
	effectNode->effect.type = effectType;
	effectNode->effect.enable = TRUE;
	effectNode->effect.gain = 0;
	switch(effectType)
	{
		case EffectTypeVB:
			effectNode->effect.effectParams.vbParams.cutoffFreq	= DEFAULT_VB_CUTOFF_FREQ;
			effectNode->effect.effectParams.vbParams.intensity	= DEFAULT_VB_INTENSITY;
			effectNode->effect.effectParams.vbParams.enhanced	= DEFAULT_VB_ENHANCED;
			break;

		case EffectType3D:
			effectNode->effect.effectParams.tdParams.depth		= DEFAULT_3D_DEPTH;
			effectNode->effect.effectParams.tdParams.preGain	= DEFAULT_3D_PREGAIN;
			effectNode->effect.effectParams.tdParams.postGain	= DEFAULT_3D_POSTGAIN;
			break;

		case EffectTypeEQ:
			break;

		default:
			break;
	}
	effectNode->effect.inited = FALSE;

	// Add the new effect to the effectList
	if(audioCore.proc.effectList.effectNums == 0)
	{
		effectNode->effect.index = effectIndex = 1;
		audioCore.proc.effectList.head = effectNode;
		audioCore.proc.effectList.tail = effectNode;
		audioCore.proc.effectList.effectNums = 1;
	}
	else
	{
		effectNode->effect.index = effectIndex = audioCore.proc.effectList.tail->effect.index + 1;

		audioCore.proc.effectList.tail->next = effectNode;
		effectNode->pre = audioCore.proc.effectList.tail;
		audioCore.proc.effectList.tail = effectNode;
	}

	return effectIndex;
}

int16_t AudioCoreProcEffectAdd(EffectType effectType)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_PROC_EFFECT_ADD;

	cmdNode->command.cmdParams.effectType = effectType;

	QueueCommand(cmdNode);

	return 0;
}

static int16_t AC_ProcEffectDel(uint8_t effectIndex)
{
	EffectNode		*tmpEffectNode;


	// Find the effect with Index
	tmpEffectNode = audioCore.proc.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;

	if(tmpEffectNode == audioCore.proc.effectList.head) // Head node
	{
		audioCore.proc.effectList.head = tmpEffectNode->next;
		if(tmpEffectNode->next != NULL)
			tmpEffectNode->next->pre = NULL;
	}
	else if(tmpEffectNode == audioCore.proc.effectList.tail) // tail node
	{
		audioCore.proc.effectList.tail = tmpEffectNode->pre;
		audioCore.proc.effectList.tail->next = NULL;
	}
	else
	{
		EffectNode		*preNode;
		EffectNode		*nextNode;

		preNode = tmpEffectNode->pre;
		nextNode = tmpEffectNode->next;

		preNode->next = nextNode;
		nextNode->pre = preNode;
	}

	vPortFree(tmpEffectNode);
	audioCore.proc.effectList.effectNums--;

	return 0;

}

int16_t AudioCoreProcEffectDel(uint8_t effectIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_PROC_EFFECT_DEL;

	cmdNode->command.cmdParams.effectIndex = effectIndex;

	QueueCommand(cmdNode);

	return 0;
}

static int16_t AC_ProcEffectEnable(uint8_t effectIndex)
{
	EffectNode				*tmpEffectNode;
	ACProtCode				protCode;

	// Find the effect with Index
	tmpEffectNode = audioCore.proc.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;


	tmpEffectNode->effect.enable = TRUE;

	switch(tmpEffectNode->effect.type)
	{
		case EffectTypeVB:
			protCode = AC_PROT_CODE_VB_ENDIS;
			break;

		case EffectType3D:
			protCode = AC_PROT_CODE_3D_ENDIS;
			break;

		case EffectTypeEQ:
			protCode = AC_PROT_CODE_EQ_ENDIS;
			break;
	}

	AudioCoreReport(protCode, AC_CPROC, 0, effectIndex, tmpEffectNode->effect.enable);

	return 0;
}


int16_t AudioCoreProcEffectEnable(uint8_t effectIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_PROC_EFFECT_ENABLE;

	cmdNode->command.cmdParams.effectIndex = effectIndex;

	QueueCommand(cmdNode);

	return 0;
}


static int16_t AC_ProcEffectDisable(uint8_t effectIndex)
{
	EffectNode				*tmpEffectNode;
	ACProtCode				protCode;

	// Find the effect with Index
	tmpEffectNode = audioCore.proc.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;


	tmpEffectNode->effect.enable = FALSE;

	switch(tmpEffectNode->effect.type)
	{
		case EffectTypeVB:
			protCode = AC_PROT_CODE_VB_ENDIS;
			break;

		case EffectType3D:
			protCode = AC_PROT_CODE_3D_ENDIS;
			break;

		case EffectTypeEQ:
			protCode = AC_PROT_CODE_EQ_ENDIS;
			break;
	}

	AudioCoreReport(protCode, AC_CPROC, 0, effectIndex, tmpEffectNode->effect.enable);

	return 0;
}

int16_t AudioCoreProcEffectDisable(uint8_t effectIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_PROC_EFFECT_DEL;

	cmdNode->command.cmdParams.effectIndex = effectIndex;

	QueueCommand(cmdNode);

	return 0;
}

static int16_t AC_ProcEffectSet(uint8_t effectIndex, EffectType effectType, uint32_t *effectParams)
{
	EffectNode				*tmpEffectNode;

	// Find the effect with Index
	tmpEffectNode = audioCore.proc.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;

	if(effectType != tmpEffectNode->effect.type) // check the type whether is equal
		return -3;


	switch(effectType)
	{
		case EffectTypeVB:
			{
				int32_t 		oldCutoffFreq;
				int32_t 		newCutoffFreq;

				oldCutoffFreq = tmpEffectNode->effect.effectParams.vbParams.cutoffFreq;
				newCutoffFreq = effectParams[0];
				
				if(oldCutoffFreq != newCutoffFreq)
				{
					// need to be re-initialize
					tmpEffectNode->effect.inited = FALSE;
				}
				tmpEffectNode->effect.effectParams.vbParams.cutoffFreq = newCutoffFreq;
				tmpEffectNode->effect.effectParams.vbParams.intensity = effectParams[1];
				tmpEffectNode->effect.effectParams.vbParams.enhanced = effectParams[2];
			}
			break;

		case EffectType3D:
			{
				int32_t 		newDepth;
				int32_t 		newPreGain;
				int32_t 		newPostGain;


				newDepth	= effectParams[0];
				newPreGain	= effectParams[1];
				newPostGain = effectParams[2];

				tmpEffectNode->effect.effectParams.tdParams.depth		= newDepth;
				tmpEffectNode->effect.effectParams.tdParams.preGain 	= newPreGain;
				tmpEffectNode->effect.effectParams.tdParams.postGain	= newPostGain;
			}
			break;

		case EffectTypeEQ:
			break;

		default:
			break;
	}

	return 0;
}

int16_t AudioCoreProcEffectSet(uint8_t effectIndex, EffectType effectType, uint32_t *effectParams)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_PROC_EFFECT_SET;

	cmdNode->command.cmdParams.procEffectSet.effectIndex = effectIndex;
	cmdNode->command.cmdParams.procEffectSet.effectType = effectType;
	switch(effectType)
	{
		case EffectTypeVB:
			cmdNode->command.cmdParams.procEffectSet.effectParams.vbParams.cutoffFreq = effectParams[0];
			cmdNode->command.cmdParams.procEffectSet.effectParams.vbParams.intensity = effectParams[1];
			cmdNode->command.cmdParams.procEffectSet.effectParams.vbParams.enhanced = effectParams[2];
			break;

		case EffectType3D:
			cmdNode->command.cmdParams.procEffectSet.effectParams.tdParams.depth = effectParams[0];
			cmdNode->command.cmdParams.procEffectSet.effectParams.tdParams.preGain = effectParams[1];
			cmdNode->command.cmdParams.procEffectSet.effectParams.tdParams.postGain = effectParams[2];
			break;

		case EffectTypeEQ:
			break;
	}

	QueueCommand(cmdNode);

	return 0;
}

static int16_t AC_ProcEffectQuery(uint8_t effectIndex)
{
	EffectNode				*tmpEffectNode;
	EffectType				effectType;

	// Find the effect with Index
	tmpEffectNode = audioCore.proc.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;

	effectType = tmpEffectNode->effect.type;

	AudioCoreReport(AC_PORT_CODE_CPROC_EFFECTS, effectIndex, effectType);

	return 0;
}

int16_t AudioCoreProcEffectQuery(uint8_t effectIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_PROC_EFFECT_QUERY;

	cmdNode->command.cmdParams.effectIndex = effectIndex;

	QueueCommand(cmdNode);

	return 0;
}


static int16_t AC_ProcEffectDescQuery(uint8_t effectIndex)
{
	EffectNode				*tmpEffectNode;
	EffectType				effectType;

	// Find the effect with Index
	tmpEffectNode = audioCore.proc.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;

	effectType = tmpEffectNode->effect.type;


	switch(effectType)
	{
		case EffectTypeVB:
			{
				AudioCoreReport(AC_PROT_CODE_VB_ENDIS, AC_CPROC, 0, effectIndex, tmpEffectNode->effect.enable);
				AudioCoreReport(AC_PROT_CODE_VB_PARAMS, AC_CPROC, 0, effectIndex,
								tmpEffectNode->effect.effectParams.vbParams.cutoffFreq,
								tmpEffectNode->effect.effectParams.vbParams.intensity,
								tmpEffectNode->effect.effectParams.vbParams.enhanced);
			}
			break;

		case EffectType3D:
			{
				AudioCoreReport(AC_PROT_CODE_3D_ENDIS, AC_CPROC, 0, effectIndex, tmpEffectNode->effect.enable);
				AudioCoreReport(AC_PROT_CODE_3D_PARAMS, AC_CPROC, 0, effectIndex,
								tmpEffectNode->effect.effectParams.tdParams.depth,
								tmpEffectNode->effect.effectParams.tdParams.preGain,
								tmpEffectNode->effect.effectParams.tdParams.postGain);
			}
			break;

		case EffectTypeEQ:
			break;

		default:
			break;
	}

	return 0;
}

int16_t AudioCoreProcEffectDescQuery(uint8_t effectIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_PROC_EFFECT_DESC_QUERY;

	cmdNode->command.cmdParams.effectIndex = effectIndex;

	QueueCommand(cmdNode);

	return 0;
}

static int16_t AC_SinkRegister(ACSnkType snkType, ACPutDataFunc putSnkData, uint8_t * snkBuf, uint32_t snkBufSize, ACPcmInfo *pcmInfo)
{
	AudioCoreSinkNode		*acSnkNode;
	uint8_t					snkIndex;

	acSnkNode = (AudioCoreSinkNode *)pvPortMalloc(sizeof(AudioCoreSinkNode));
	if(acSnkNode == NULL)
		return -1;

	acSnkNode->next = NULL;
	acSnkNode->pre = NULL;

	acSnkNode->acSnk.buf = snkBuf;
	memset(acSnkNode->acSnk.buf, 0, snkBufSize);
	acSnkNode->acSnk.bufSize = snkBufSize;
	acSnkNode->acSnk.bufDataLen = 0;
	acSnkNode->acSnk.bufSampleLen = 0;

	acSnkNode->acSnk.type = snkType;
	acSnkNode->acSnk.enable = TRUE;
	acSnkNode->acSnk.gain = 0;
	acSnkNode->acSnk.funcPutData = putSnkData;
//	acSnkNode->acSnk.sinkDone = FALSE;
	acSnkNode->acSnk.sinkPutState = SINK_PUT_NONE,
//	acSnkNode->acSnk.effectList.effectNums = 0;
//	acSnkNode->acSnk.effectList.head = NULL;
//	acSnkNode->acSnk.effectList.tail = NULL;
	memcpy(&acSnkNode->acSnk.pcmInfo, pcmInfo, sizeof(ACPcmInfo));

	if(audioCore.snkList.snkNums == 0) /* First node*/
	{
		/* set Handle */
		acSnkNode->acSnk.index = snkIndex = 1;

		audioCore.snkList.head = acSnkNode;
		audioCore.snkList.tail = acSnkNode;
		audioCore.snkList.snkNums = 1;
	}
	else
	{
		acSnkNode->acSnk.index = snkIndex = audioCore.snkList.tail->acSnk.index + 1;

		acSnkNode->pre = audioCore.snkList.tail;
		audioCore.snkList.tail->next = acSnkNode;
		audioCore.snkList.tail = acSnkNode;
		audioCore.snkList.snkNums++;
	}

	return snkIndex;

}

int16_t AudioCoreSinkRegister(ACSnkType snkType, ACPutDataFunc putSnkData, uint8_t * snkBuf, uint32_t snkBufSize, ACPcmInfo *pcmInfo)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SINK_REGISTER;

	cmdNode->command.cmdParams.snkRegister.snkType = snkType;
	cmdNode->command.cmdParams.snkRegister.putSnkData = putSnkData;
	cmdNode->command.cmdParams.snkRegister.snkBuf = snkBuf;
	cmdNode->command.cmdParams.snkRegister.snkBufSize = snkBufSize;
	memcpy(&cmdNode->command.cmdParams.snkRegister.pcmInfo, pcmInfo, sizeof(ACPcmInfo));

	QueueCommand(cmdNode);

	return 0;
}

static int16_t AC_SinkDeregister(uint8_t snkIndex)
{
	AudioCoreSinkNode 		*tmpSnkNode;


	tmpSnkNode = audioCore.snkList.head;
	while(tmpSnkNode)
	{
		if(tmpSnkNode->acSnk.index == snkIndex)
			break;

		tmpSnkNode = tmpSnkNode->next;
	}

	if(tmpSnkNode != NULL)
	{
		if(tmpSnkNode == audioCore.snkList.head)		// head node
		{
			audioCore.snkList.head = tmpSnkNode->next;
			if(tmpSnkNode->next != NULL)
				tmpSnkNode->next->pre = NULL;
		}
		else if(tmpSnkNode == audioCore.snkList.tail)	// tail node
		{
			audioCore.snkList.tail = tmpSnkNode->pre;
			audioCore.snkList.tail->next = NULL;
		}
		else
		{
			AudioCoreSinkNode 	*preNode;
			AudioCoreSinkNode 	*nextNode;

			preNode = tmpSnkNode->pre;
			nextNode = tmpSnkNode->next;

			preNode->next = nextNode;
			nextNode->pre = preNode;
		}

		vPortFree(tmpSnkNode);
		audioCore.snkList.snkNums--;
	}
	else
	{
		return -1;
	}

	return 0;
}

int16_t AudioCoreSinkDeregister(uint8_t snkIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SINK_DEREGISTER;

	cmdNode->command.cmdParams.snkIndex = snkIndex;

	QueueCommand(cmdNode);

	return 0;
}

static int16_t AC_SinkEnable(uint8_t snkIndex)
{
	AudioCoreSinkNode 		*tmpSnkNode;

	tmpSnkNode = audioCore.snkList.head;
	while(tmpSnkNode)
	{
		if(tmpSnkNode->acSnk.index == snkIndex)
			break;

		tmpSnkNode = tmpSnkNode->next;
	}

	if(tmpSnkNode == NULL)
		return -1;

	tmpSnkNode->acSnk.enable = TRUE;

	return 0;
}

int16_t AudioCoreSinkEnable(uint8_t snkIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SINK_ENABLE;

	cmdNode->command.cmdParams.snkIndex = snkIndex;

	QueueCommand(cmdNode);

	return 0;
}

static int16_t AC_SinkDisable(uint8_t snkIndex)
{
	AudioCoreSinkNode 		*tmpSnkNode;

	tmpSnkNode = audioCore.snkList.head;
	while(tmpSnkNode)
	{
		if(tmpSnkNode->acSnk.index == snkIndex)
			break;

		tmpSnkNode = tmpSnkNode->next;
	}

	if(tmpSnkNode == NULL)
		return -1;

	tmpSnkNode->acSnk.enable = FALSE;

	return 0;
}

int16_t AudioCoreSinkDisable(uint8_t snkIndex)
{
	ACCommandNode			*cmdNode;

	cmdNode = (ACCommandNode*)pvPortMalloc(sizeof(ACCommandNode));

	cmdNode->next = NULL;
	cmdNode->pre = NULL;
	cmdNode->command.cmdType = AC_CMD_SINK_DISABLE;

	cmdNode->command.cmdParams.snkIndex = snkIndex;

	QueueCommand(cmdNode);

	return 0;
}

int16_t AudioCoreSinkStatusGet(uint8_t snkIndex, uint8_t *status)
{
	AudioCoreSinkNode 		*tmpSnkNode;

	tmpSnkNode = audioCore.snkList.head;
	while(tmpSnkNode)
	{
		if(tmpSnkNode->acSnk.index == snkIndex)
			break;

		tmpSnkNode = tmpSnkNode->next;
	}

	if(tmpSnkNode == NULL)
		return -1;

	*status = tmpSnkNode->acSnk.enable;

	return 0;
}


int16_t AudioCoreSinkGainSet(uint8_t snkIndex, uint16_t gain)
{
	AudioCoreSinkNode 		*tmpSnkNode;

	tmpSnkNode = audioCore.snkList.head;
	while(tmpSnkNode)
	{
		if(tmpSnkNode->acSnk.index == snkIndex)
			break;

		tmpSnkNode = tmpSnkNode->next;
	}

	if(tmpSnkNode == NULL)
		return -1;

	tmpSnkNode->acSnk.gain = gain;

	return 0;
}

int16_t AudioCoreSinkGainGet(uint8_t snkIndex, uint16_t *gain)
{
	AudioCoreSinkNode 		*tmpSnkNode;

	tmpSnkNode = audioCore.snkList.head;
	while(tmpSnkNode)
	{
		if(tmpSnkNode->acSnk.index == snkIndex)
			break;

		tmpSnkNode = tmpSnkNode->next;
	}

	if(tmpSnkNode == NULL)
		return -1;

	*gain = tmpSnkNode->acSnk.gain;

	return 0;
}

#if 0
int16_t AudioCoreSinkEffectAdd(uint8_t snkIndex, EffectType effectType)
{
	AudioCoreSinkNode		*tmpSnkNode;
	EffectNode				*effectNode;
	uint8_t					effectIndex;

	// Find the source 
	tmpSnkNode = audioCore.snkList.head;
	while(tmpSnkNode)
	{
		if(tmpSnkNode->acSnk.index == snkIndex)
			break;

		tmpSnkNode = tmpSnkNode->next;
	}

	if(tmpSnkNode == NULL)
		return -1;

	// Make & init an effect node
	effectNode = (EffectNode*)pvPortMalloc(sizeof(EffectNode));
	if(effectNode == NULL)
	{
		return -2;
	}
	
	effectNode->pre = NULL;
	effectNode->next = NULL;
	effectNode->effect.type = effectType;
	effectNode->effect.enable = TRUE;
	effectNode->effect.gain = 0;
	switch(effectType)
	{
		case EffectTypeVB:
			effectNode->effect.effectParams.vbParams.cutoffFreq	= DEFAULT_VB_CUTOFF_FREQ;
			effectNode->effect.effectParams.vbParams.intensity	= DEFAULT_VB_INTENSITY;
			effectNode->effect.effectParams.vbParams.enhanced	= DEFAULT_VB_ENHANCED;
			break;

		case EffectType3D:
			effectNode->effect.effectParams.tdParams.depth		= DEFAULT_3D_DEPTH;
			effectNode->effect.effectParams.tdParams.preGain	= DEFAULT_3D_PREGAIN;
			effectNode->effect.effectParams.tdParams.postGain	= DEFAULT_3D_POSTGAIN;
			break;

		case EffectTypeEQ:
			break;

		default:
			break;
	}
	effectNode->effect.inited = FALSE;

	// Add the new effect to the effectList
	if(tmpSnkNode->acSnk.effectList.effectNums == 0)
	{
		effectNode->index = effectIndex = 1;
		tmpSnkNode->acSnk.effectList.head = effectNode;
		tmpSnkNode->acSnk.effectList.tail = effectNode;
		tmpSnkNode->acSnk.effectList.effectNums = 1;
	}
	else
	{
		effectNode->index = effectIndex = tmpSnkNode->acSnk.effectList.tail->effect.index + 1;

		tmpSnkNode->acSnk.effectList.tail->next = effectNode;
		effectNode->pre = tmpSnkNode->acSnk.effectList.tail;
		tmpSnkNode->acSnk.effectList.tail = effectNode;
	}

	return effectIndex;

}

int16_t AudioCoreSinkEffectDel(uint8_t snkIndex, uint8_t effectIndex)
{
	AudioCoreSinkNode		*tmpSnkNode;
	EffectNode				*tmpEffectNode;


	// Find the source 
	tmpSnkNode = audioCore.snkList.head;
	while(tmpSnkNode)
	{
		if(tmpSnkNode->acSnk.index == snkIndex)
			break;

		tmpSnkNode = tmpSnkNode->next;
	}

	if(tmpSnkNode == NULL)
		return -1;

	
	// Find the effect with Index
	tmpEffectNode = tmpSnkNode->acSnk.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;

	if(tmpEffectNode == tmpSnkNode->acSnk.effectList.head) // Head node
	{
		tmpSnkNode->acSnk.effectList.head = tmpEffectNode->next;
		if(tmpEffectNode->next != NULL)
			tmpEffectNode->next->pre = NULL;
	}
	else if(tmpEffectNode == tmpSnkNode->acSnk.effectList.tail) // tail node
	{
		tmpSnkNode->acSnk.effectList.tail = tmpEffectNode->pre;
		tmpSnkNode->acSnk.effectList.tail->next = NULL;
	}
	else
	{
		EffectNode		*preNode;
		EffectNode		*nextNode;

		preNode = tmpEffectNode->pre;
		nextNode = tmpEffectNode->next;

		preNode->next = nextNode;
		nextNode->pre = preNode;
	}

	vPortFree(tmpEffectNode);
	tmpSnkNode->acSnk.effectList.effectNums--;

	return 0;

}

int16_t AudioCoreSinkEffectEnable(uint8_t snkIndex, uint8_t effectIndex)
{
	AudioCoreSinkNode		*tmpSnkNode;
	EffectNode				*tmpEffectNode;


	// Find the source 
	tmpSnkNode = audioCore.snkList.head;
	while(tmpSnkNode)
	{
		if(tmpSnkNode->acSnk.index == snkIndex)
			break;

		tmpSnkNode = tmpSnkNode->next;
	}

	if(tmpSnkNode == NULL)
		return -1;

	
	// Find the effect with Index
	tmpEffectNode = tmpSnkNode->acSnk.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;


	tmpEffectNode->effect.enable = TRUE;

	return 0;

}

int16_t AudioCoreSinkEffectDisable(uint8_t snkIndex, uint8_t effectIndex)
{
	AudioCoreSinkNode		*tmpSnkNode;
	EffectNode				*tmpEffectNode;


	// Find the source 
	tmpSnkNode = audioCore.snkList.head;
	while(tmpSnkNode)
	{
		if(tmpSnkNode->acSnk.index == snkIndex)
			break;

		tmpSnkNode = tmpSnkNode->next;
	}

	if(tmpSnkNode == NULL)
		return -1;

	
	// Find the effect with Index
	tmpEffectNode = tmpSnkNode->acSnk.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;

	tmpEffectNode->effect.enable = FALSE;

	return 0;

}


int16_t AudioCoreSinkEffectSet(uint8_t snkIndex, uint8_t effectIndex, EffectType effectType, uint32_t *effectParams)
{
	AudioCoreSinkNode	 	*tmpSnkNode;
	EffectNode				*tmpEffectNode;


	// Find the source 
	tmpSnkNode = audioCore.snkList.head;
	while(tmpSnkNode)
	{
		if(tmpSnkNode->acSnk.index == snkIndex)
			break;

		tmpSnkNode = tmpSnkNode->next;
	}

	if(tmpSnkNode == NULL)
		return -1;


	// Find the effect with Index
	tmpEffectNode = tmpSnkNode->acSnk.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;

	if(effectType != tmpEffectNode->effect.type) // check the type whether is equal
		return -3;


	switch(effectType)
	{
		case EffectTypeVB:
			{
				int32_t			oldCutoffFreq;
				int32_t			newCutoffFreq;

				oldCutoffFreq = tmpEffectNode->effect.effectParams.vbParams.cutoffFreq;
				newCutoffFreq = effectParams[0];
				
				if(oldCutoffFreq != newCutoffFreq)
				{
					// need to be re-initialize
					tmpEffectNode->effect.inited = FALSE;
				}
				tmpEffectNode->effect.effectParams.vbParams.cutoffFreq = newCutoffFreq;
				tmpEffectNode->effect.effectParams.vbParams.intensity = effectParams[1];
				tmpEffectNode->effect.effectParams.vbParams.enhanced = effectParams[2];
			}
			break;

		case EffectType3D:
			{
				int32_t 		newDepth;
				int32_t			newPreGain;
				int32_t			newPostGain;


				newDepth	= effectParams[0];
				newPreGain	= effectParams[1];
				newPostGain	= effectParams[2];

				tmpEffectNode->effect.effectParams.tdParams.depth		= newDepth;
				tmpEffectNode->effect.effectParams.tdParams.preGain		= newPreGain;
				tmpEffectNode->effect.effectParams.tdParams.postGain	= newPostGain
			}
			break;

		case EffectTypeEQ:
			break;

		default:
			break;
	}

	return 0;

}

int16_t AudioCoreSinkEffectGet(uint8_t snkIndex, uint8_t effectIndex, EffectType *effectType, uint32_t *effectParams)
{
	AudioCoreSinkNode	 	*tmpSnkNode;
	EffectNode				*tmpEffectNode;


	// Find the source 
	tmpSnkNode = audioCore.snkList.head;
	while(tmpSnkNode)
	{
		if(tmpSnkNode->acSnk.index == snkIndex)
			break;

		tmpSnkNode = tmpSnkNode->next;
	}

	if(tmpSnkNode == NULL)
		return -1;


	// Find the effect with Index
	tmpEffectNode = tmpSnkNode->acSnk.effectList.head;
	while(tmpEffectNode)
	{
		if(tmpEffectNode->effect.index == effectIndex)
			break;

		tmpEffectNode = tmpEffectNode->next;
	}

	if(tmpEffectNode == NULL)
		return -2;

	if(effectType != NULL)
		*effectType = tmpEffectNode->effect.type;
	else
		return -3;

	switch(*effectType)
	{
		case EffectTypeVB:
			{
				effectParams[0] = tmpEffectNode->effect.effectParams.vbParams.cutoffFreq;
				effectParams[1] = tmpEffectNode->effect.effectParams.vbParams.intensity;
				effectParams[2] = tmpEffectNode->effect.effectParams.vbParams.enhanced;
			}
			break;

		case EffectType3D:
			{
				effectParams[0] = tmpEffectNode->effect.effectParams.tdParams.depth;
				effectParams[1] = tmpEffectNode->effect.effectParams.tdParams.preGain;
				effectParams[2] = tmpEffectNode->effect.effectParams.tdParams.postGain;
			}
			break;

		case EffectTypeEQ:
			break;

		default:
			break;
	}

	return 0;

}
#endif


static void CommandProcess(void)
{
	ACCommandNode	*cmdNode;


	cmdNode = DequeueCommand();
	if(cmdNode != NULL)
	{
		ACCommandType		cmdType;
		int16_t				ret;


		cmdType = cmdNode->command.cmdType;
		switch(cmdType)
		{
			case AC_CMD_SOURCE_QUERY:
				AC_SourceQuery();
				break;

			case AC_CMD_PROC_QUERY:
				AC_ProcQuery();
				break;

			case AC_CMD_SINK_QUERY:
				AC_SinkQuery();
				break;

			case AC_CMD_SOURCE_EFFECT_QUERY:
				{
					uint8_t		srcIndex;
					uint8_t		effectIndex;

					AC_SourceEffectQuery(srcIndex, effectIndex);
				}
				break;

			case AC_CMD_SOURCE_REGISTER:
				{
					ACSrcType		srcType;
					ACGetDataFunc	getSrcData;
					ACPcmInfo		*pcmInfo;

					srcType = cmdNode->command.cmdParams.srcRegister.srcType;
					getSrcData = cmdNode->command.cmdParams.srcRegister.getSrcData;
					pcmInfo = &cmdNode->command.cmdParams.srcRegister.pcmInfo;
					ret = AC_SourceRegister(srcType, getSrcData, pcmInfo);
				}
				break;

			case AC_CMD_SOURCE_DEREGISTER:
				ret = AC_SourceDeregister(cmdNode->command.cmdParams.srcIndex);
				break;
			case AC_CMD_SOURCE_CONFIG_PCM_INFO:
				ret = AC_SourceConfigPcmInfo(cmdNode->command.cmdParams.srcPcmInfo.srcIndex, 
											&cmdNode->command.cmdParams.srcPcmInfo.pcmInfo);
				break;
			case AC_CMD_SOURCE_ENABLE:
				ret = AC_SourceEnable(cmdNode->command.cmdParams.srcIndex);
				break;

			case AC_CMD_SOURCE_DISABLE:
				ret = AC_SourceDisable(cmdNode->command.cmdParams.srcIndex);
				break;

			case AC_CMD_SOURCE_STATUS_GET:
				{
					uint8_t		status;
					ret = AC_SourceStatusGet(cmdNode->command.cmdParams.srcIndex, &status);
				}
				break;

			case AC_CMD_SOURCE_GAIN_SET:
				{
					ret = AC_SourceGainSet(cmdNode->command.cmdParams.gainSet.srcIndex,cmdNode->command.cmdParams.gainSet.gain);
				}
				break;

			case AC_CMD_SOURCE_GAIN_GET:
				{
					uint16_t		gain;

					ret = AC_SourceGainGet(cmdNode->command.cmdParams.srcIndex, &gain);
				}
				break;

			case AC_CMD_SOURCE_EFFECT_ADD:
				{
					ret = AC_SourceEffectAdd(cmdNode->command.cmdParams.srcEffectAdd.srcIndex, cmdNode->command.cmdParams.srcEffectAdd.effectType);
				}
				break;

			case AC_CMD_SOURCE_EFFECT_DEL:	
				{
					ret = AC_SourceEffectDel(cmdNode->command.cmdParams.srcEffect.srcIndex, cmdNode->command.cmdParams.srcEffect.effectIndex);
				}
				break;

			case AC_CMD_SOURCE_EFFECT_ENABLE:
				{
					ret = AC_SourceEffectEnable(cmdNode->command.cmdParams.srcEffect.srcIndex, cmdNode->command.cmdParams.srcEffect.effectIndex);
				}
				break;

			case AC_CMD_SOURCE_EFFECT_DISABLE:
				{
					ret = AC_SourceEffectDisable(cmdNode->command.cmdParams.srcEffect.srcIndex, cmdNode->command.cmdParams.srcEffect.effectIndex);
				}
				break;

			case AC_CMD_SOURCE_EFFECT_SET:
				{
					EffectType		effectType;
					uint32_t			*effectParams;

					effectType = cmdNode->command.cmdParams.srcEffectSet.effectType;
					switch(effectType)
					{
						case EffectTypeVB:
							effectParams = (uint32_t*)pvPortMalloc(sizeof(ParamsVB));
							if(effectParams)
							{
								effectParams[0] = cmdNode->command.cmdParams.srcEffectSet.effectParams.vbParams.cutoffFreq;
								effectParams[1] = cmdNode->command.cmdParams.srcEffectSet.effectParams.vbParams.intensity;
								effectParams[2] = cmdNode->command.cmdParams.srcEffectSet.effectParams.vbParams.enhanced;
							}
							break;

						case EffectType3D:
							effectParams = (uint32_t*)pvPortMalloc(sizeof(Params3D));
							if(effectParams)
							{
								effectParams[0] = cmdNode->command.cmdParams.srcEffectSet.effectParams.tdParams.depth;
								effectParams[1] = cmdNode->command.cmdParams.srcEffectSet.effectParams.tdParams.preGain;
								effectParams[2] = cmdNode->command.cmdParams.srcEffectSet.effectParams.tdParams.postGain;
							}
							break;

						case EffectTypeEQ:
							effectParams = (uint32_t*)pvPortMalloc(sizeof(ParamsEQ));
							
							if(effectParams)
							{
							}
							break;							
					}

					if(effectParams)
					{
						ret = AC_SourceEffectSet(cmdNode->command.cmdParams.srcEffectSet.srcIndex,
											cmdNode->command.cmdParams.srcEffectSet.effectIndex,
											effectType,effectParams);

						vPortFree(effectParams);
					}
				}
				break;

			case AC_CMD_PROC_SET:
				{
					ret = AC_ProcSet(cmdNode->command.cmdParams.procSet.procType, &cmdNode->command.cmdParams.procSet.pcmInfo);
				}
				break;

			case AC_CMD_PROC_GET:
				break;

			case AC_CMD_PROC_EFFECT_ADD:
				{
					ret = AC_ProcEffectAdd(cmdNode->command.cmdParams.effectType);
				}
				break;

			case AC_CMD_PROC_EFFECT_DEL:
				{
					ret = AC_ProcEffectDel(cmdNode->command.cmdParams.effectIndex);
				}
				break;

			case AC_CMD_PROC_EFFECT_ENABLE:
				ret = AC_ProcEffectEnable(cmdNode->command.cmdParams.effectIndex);
				break;

			case AC_CMD_PROC_EFFECT_DISABLE:
				ret = AC_ProcEffectDisable(cmdNode->command.cmdParams.effectIndex);
				break;

			case AC_CMD_PROC_EFFECT_SET:
				{
					EffectType		effectType;
					uint32_t		*effectParams;

					effectType = cmdNode->command.cmdParams.procEffectSet.effectType;
					switch(effectType)
					{
						case EffectTypeVB:
							effectParams = (uint32_t*)pvPortMalloc(sizeof(ParamsVB));
							if(effectParams)
							{
								effectParams[0] = cmdNode->command.cmdParams.procEffectSet.effectParams.vbParams.cutoffFreq;
								effectParams[1] = cmdNode->command.cmdParams.procEffectSet.effectParams.vbParams.intensity;
								effectParams[2] = cmdNode->command.cmdParams.procEffectSet.effectParams.vbParams.enhanced;
							}
							break;

						case EffectType3D:
							effectParams = (uint32_t*)pvPortMalloc(sizeof(Params3D));
							if(effectParams)
							{
								effectParams[0] = cmdNode->command.cmdParams.procEffectSet.effectParams.tdParams.depth;
								effectParams[1] = cmdNode->command.cmdParams.procEffectSet.effectParams.tdParams.preGain;
								effectParams[2] = cmdNode->command.cmdParams.procEffectSet.effectParams.tdParams.postGain;
							}
							break;

						case EffectTypeEQ:
							effectParams = (uint32_t*)pvPortMalloc(sizeof(ParamsEQ));
							
							if(effectParams)
							{
							}
							break;							
					}

					if(effectParams)
					{
						ret = AC_ProcEffectSet(cmdNode->command.cmdParams.srcEffectSet.effectIndex,
											effectType,effectParams);

						vPortFree(effectParams);
					}
				}
				break;

			case AC_CMD_SINK_REGISTER:
				{
					ACSnkType		snkType;
					ACPutDataFunc	putSnkData;
					ACPcmInfo		*pcmInfo;
					uint8_t			*snkBuf;
					uint32_t		snkBufSize;

					snkType = cmdNode->command.cmdParams.snkRegister.snkType;
					putSnkData = cmdNode->command.cmdParams.snkRegister.putSnkData;
					pcmInfo = &cmdNode->command.cmdParams.snkRegister.pcmInfo;
					snkBuf = cmdNode->command.cmdParams.snkRegister.snkBuf;
					snkBufSize = cmdNode->command.cmdParams.snkRegister.snkBufSize;
					ret = AC_SinkRegister(snkType, putSnkData, snkBuf, snkBufSize, pcmInfo);
				}
				break;

			case AC_CMD_SINK_DEREGISTER:
				break;

			case AC_CMD_SINK_ENABLE:
				break;

			case AC_CMD_SINK_DISABLE:
				break;

			case AC_CMD_SINK_GAIN_SET:
				break;

		}
		vPortFree(cmdNode);
	}
}



static int16_t MixProc(AudioCoreSourceList * srcList, AudioCoreProcess * proc)
{
	AudioCoreSourceNode		*srcNode;

	int16_t 				*srcBuf;
	int16_t 				*mixedBuf;


	if(srcList == NULL || proc == NULL)
		return -1;

	srcNode = srcList->head;
	mixedBuf = (int16_t *)proc->buf;
	memset(mixedBuf, 0, DEFAULT_SRC_GET_SAMPLES_LEN*proc->pcmInfo.channelNums*2);
	proc->samplesLen = 0;

	while(srcNode)
	{
		if(srcNode->acSrc.enable)
		{
			uint16_t		sampleLen;
			sampleLen = srcNode->acSrc.vaildSampleLen;
			if(sampleLen != 0)
			{
				uint16_t	i;
				if(srcNode->acSrc.pcmInfo.channelNums == 1) // mono
				{
				}
				else if(srcNode->acSrc.pcmInfo.channelNums == 2) // stereo
				{
					srcBuf = (int16_t *)srcNode->acSrc.buf;
					for(i = 0; i < sampleLen; i++)
					{
						mixedBuf[i * 2] 	= __ssat(mixedBuf[i * 2 + 0] + (int16_t)(srcBuf[i * 2]), 16);
						mixedBuf[i * 2 + 1] = __ssat(mixedBuf[i * 2 + 1] + (int16_t)(srcBuf[i * 2 + 1]), 16);
					}
					
				}
			}
			proc->samplesLen = proc->samplesLen > sampleLen ? proc->samplesLen : sampleLen;
		}

		srcNode = srcNode->next;
	}

	EffectProcess(&proc->effectList, mixedBuf, proc->samplesLen, &proc->pcmInfo);

	return 0;
}

static void AecProc(AudioCoreSourceList * srcList, AudioCoreProcess *proc)
{
	
}

static BOOL AudioCoreSourceSamplesGet(void)
{
	AudioCoreSourceNode		*node;
	uint16_t				tmpSamplesLen = 0;

	node = audioCore.srcList.head;
	while(node)
	{
		if(node->acSrc.enable)
		{
			node->acSrc.vaildSampleLen = node->acSrc.funcGetData(node->acSrc.buf, DEFAULT_SRC_GET_SAMPLES_LEN, node->acSrc.pcmInfo.channelNums);

			tmpSamplesLen = tmpSamplesLen > node->acSrc.vaildSampleLen ? tmpSamplesLen : node->acSrc.vaildSampleLen;

			if(node->acSrc.vaildSampleLen != 0)
			{
				EffectProcess(&node->acSrc.effectList, (int16_t *)node->acSrc.buf, node->acSrc.vaildSampleLen, &node->acSrc.pcmInfo);
			}
		}
		node = node->next;
	}

	if(tmpSamplesLen == 0)
		return FALSE;

	return TRUE;
}

static void AduioCoreProcecss(void)
{
	switch(audioCore.proc.type)
	{
		case AC_PROC_MIX:
			MixProc(&audioCore.srcList, &audioCore.proc);
			break;

		case AC_PROC_AEC:
			AecProc(&audioCore.srcList, &audioCore.proc);
			break;

		default:
			break;
	}
}


void SnkDone(uint8_t index)
{
	AudioCoreSinkNode	*snkNode;

	snkNode = audioCore.snkList.head;

	while(snkNode)
	{
		if(snkNode->acSnk.index == index)
			break;

		snkNode = snkNode->next;
	}

	if(snkNode != NULL)
	{
		snkNode->acSnk.bufDataLen = 0;
		snkNode->acSnk.bufSampleLen = 0;
		snkNode->acSnk.sinkPutState = SINK_PUT_NONE;
	}
}

static BOOL AudioCoreSave(void)
{
	BOOL				ret = FALSE;
	AudioCoreSinkNode	*snkNode;

	snkNode = audioCore.snkList.head;

	while(snkNode)
	{
		uint8_t 	*snkBuf;
		uint32_t	snkBufSize;
		uint32_t	snkBufDataSize;

		uint8_t 	*procBuf;
		uint32_t	procDataLen;

		snkBuf = snkNode->acSnk.buf;
		snkBufSize = snkNode->acSnk.bufSize;
		snkBufDataSize = snkNode->acSnk.bufDataLen;

		procDataLen = audioCore.proc.samplesLen * audioCore.proc.pcmInfo.channelNums * 2;
		procBuf = audioCore.proc.buf;

		if(snkNode->acSnk.enable)
		{
			if(snkNode->acSnk.sinkPutState == SINK_PUT_NONE)
			{
				if(procDataLen <= (snkBufSize - snkBufDataSize))
				{
					memcpy(snkBuf + snkBufDataSize, procBuf, procDataLen);
					snkNode->acSnk.bufDataLen += procDataLen;
					snkNode->acSnk.bufSampleLen += audioCore.proc.samplesLen;
					ret = TRUE;

				}
			}
		}

		snkNode = snkNode->next;
	}

	return ret;
}
static BOOL AudioCoreSinkSamplesPut(void)
{
	BOOL				ret = TRUE;
	AudioCoreSinkNode	*snkNode;

	snkNode = audioCore.snkList.head;

	while(snkNode)
	{
		uint8_t		*snkBuf;
		uint32_t	snkBufSize;
		uint32_t	snkBufDataSize;

		uint8_t		*procBuf;
		uint32_t	procDataLen;

		snkBuf = snkNode->acSnk.buf;
		snkBufSize = snkNode->acSnk.bufSize;
		snkBufDataSize = snkNode->acSnk.bufDataLen;

		procDataLen = audioCore.proc.samplesLen * audioCore.proc.pcmInfo.channelNums * 2;
		procBuf = audioCore.proc.buf;

		if(snkNode->acSnk.funcPutData(snkBuf, snkNode->acSnk.bufSampleLen, snkNode->acSnk.pcmInfo.channelNums) == 1)
		{
			snkNode->acSnk.sinkPutState = SINK_PUT_TRANSING;
		}

		snkNode = snkNode->next;
	}

	return ret;
}

BOOL AudioCoreInit(void)
{
	memset(&audioCore, 0, sizeof(AudioCoreContext));
	audioCore.runState = AC_RUN_GET;

	return TRUE;
}

void AudioCoreDeinit(void)
{

}

void AudioCoreRun(void)
{
	BOOL		ret;

	CommandProcess();
	while(1)
	{
		switch(audioCore.runState)
		{
			case AC_RUN_GET:
				ret = AudioCoreSourceSamplesGet();
				if(ret == FALSE)
				{
					return;
				}
				audioCore.runState = AC_RUN_PROC;

			case AC_RUN_PROC:
				AduioCoreProcecss();
				audioCore.runState = AC_RUN_SAVE;

			case AC_RUN_SAVE:
				ret = AudioCoreSave();
				if(ret == FALSE)
				{
					AudioCoreSinkSamplesPut();
					return;
				}
				audioCore.runState = AC_RUN_PUT;

			case AC_RUN_PUT:
				ret = AudioCoreSinkSamplesPut();
				if(ret == FALSE)
				{
					return;
				}
				audioCore.runState = AC_RUN_GET;

			default:
				break;
		}
	}
}


