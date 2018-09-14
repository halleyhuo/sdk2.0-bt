

#include <string.h>
#include "type.h"
#include "FreeRTOS.h"
#include "audio_core_service.h"
#include "chip_info.h"
#include "app_config.h"

#include "audio_core_mix.h"
#include "virtual_bass.h"
#include "three_d.h"
#include "eq.h"
#include "pcm_fifo.h"
#include "audio_utility.h"
/***************************************************************************************
 *
 * External defines
 *
 */

#define EFFECT_PARAM_VB_CUTOFF_FREQ			(300)
#define EFFECT_PARAM_VB_INTENSITY			(100)
#define EFFECT_PARAM_VB_ENHANCED			(1)

#define EFFECT_PARAM_3D_DEPTH				(50)
#define EFFECT_PARAM_3D_PREGAIN				(32768)
#define EFFECT_PARAM_3D_POSTGAIN			(32768)

/***************************************************************************************
 *
 * Internal defines
 *
 */

#define DEFAULT_MIX_SRC_SAMPLE_LEN				192
#define MIX_SRC_BUFFER_LEN						(DEFAULT_MIX_SRC_SAMPLE_LEN * 4)

#define DEFAULT_MIX_SAMPLE_LEN					DEFAULT_MIX_SRC_SAMPLE_LEN
#define MIXED_BUFFER_LEN						(DEFAULT_MIX_SRC_SAMPLE_LEN * 4)

#define MAX_SOURCE_NUM						3
#define MAX_SINK_NUM						1

typedef enum
{
	None,
	Unprocessed,
	Processing,
	Processed
} MarkFlag;

typedef enum
{
	MixStateStateGetData,
	MixStateMix,
	MixStatePuttingData
}MixState;

typedef int32_t (*EffectParamsConfig)( void * effectParams);
typedef int32_t (*EffectInitFunc)(AudioCorePcmParams *pcmParams);
typedef int32_t (*EffectProcessFunc)(int16_t * pcmIn, int16_t * pcmOut, uint32_t samples);

typedef struct _ParamsVB
{
	int32_t		cutoffFreq;
	int32_t		intensity;
	int32_t		enhanced;
}ParamsVB;

typedef struct _Params3D
{
	int32_t		depth;
	int32_t		preGain;
	int32_t		postGain;
}Params3D;

typedef struct _ParamsEQ
{

}ParamsEQ;


typedef struct _EffectList
{
	uint8_t				effectNums;
	EffectNode			*head;
	EffectNode			*tail;
}EffectList;

typedef uint8_t						SourceType;

#define SOURCE_TYPE_DECODER			1

#define SOURCE_TYPE_MIC				2

#define SOURCE_TYPE_LINE			3


typedef uint8_t						SinkType;

#define SINK_TYPE_DAC				1

#define SINK_TYPE_IIS				2

typedef struct _MixSource
{
	SourceType			srcType;
	BOOL				enable;
	AudioCorePcmParams	pcmParams;
	uint32_t			gain;
//	AudioCoreGetData	funcGetData;
	uint16_t			validSampleLen;
	uint8_t				*buf;
	EffectList			effectList;
}MixSource;

typedef struct _MixSink
{
//	SinkType			sinkType;
//	BOOL				enable;
	AudioCorePcmParams	pcmParams;
	uint32_t			gain;
//	AudioCorePutData	funcPutData;
//	BOOL				sinkPutDone;
}MixSink;

typedef struct _VbEffect
{
	VBContext			vbContext;
	BOOL				enable;
	uint32_t			cutoffFreq;
	uint32_t			intensity;
	uint32_t			enhanced;
}VbEffect;

typedef struct _ThrDimEffect
{
	ThreeDContext		thrDimContext;
	BOOL				enable;
	uint32_t			depth;
	uint32_t			preGain;
	uint32_t			postGain;
}ThrDimEffect;

typedef struct _MixAcContext
{
	MixSource			mixSource[MAX_SOURCE_NUM];
	MixSink				mixSink;

	MixState			mixState;
	AudioCorePcmParams	mixedPcmParams;
	uint32_t			mixedSampleLen;
	int16_t				*mixedBuf;
	EffectList			postEffectList;

	MixCommandType		mixCmdType;
	uint32_t			*mixCmdParams;
}MixAcContext;


/***************************************************************************************
 *
 * Internal varibles
 *
 */

static MixAcContext		mixAudioCore;

/***************************************************************************************
 *
 * Internal functions
 *
 */
static int32_t EffectVbInit(VBContext *vbContext, ParamsVB *paramsVB, AudioCorePcmParams *pcmParams)
{
	return vb_init(vbContext, pcmParams->channelNums, pcmParams->samplingRate, paramsVB->cutoffFreq);
}

static void EffectVbProcess(VBContext *vbContext, ParamsVB *paramsVB, int16_t * pcmIn, int16_t *pcmOut, int32_t samples)
{
	vb_apply(vbContext, pcmIn, pcmOut, samples, paramsVB->intensity, paramsVB->enhanced);
}

static int32_t Effect3DInit(ThreeDContext * thdContext, Params3D *params3D, AudioCorePcmParams *pcmParams)
{
	return init_3d(thdContext, pcmParams->channelNums, pcmParams->samplingRate);
}

static void Effect3DProcess(ThreeDContext * thdContext, Params3D *params3D,int16_t * pcmIn, int16_t *pcmOut, int32_t samples)
{
	apply_3d(thdContext, pcmIn, pcmOut, samples, params3D->depth, params3D->preGain, params3D->postGain);
}

static void EffectListProcess(EffectNode * head, int16_t * pcmIn, int16_t * pcmOut, int32_t samples)
{
	EffectNode			*node;

	node = head;

	while(node)
	{
		if(node->enable)
		{
			switch(node->type)
			{
				case EffectTypeVB:
					if(!node->inited)
					{
						if(EffectVbInit(&node->effectContext.vbContext,
										&node->effectParams.vbParams,
										&node->pcmParams) == VB_ERROR_OK)
						{
							node->inited = TRUE;
						}
					}

					if(node->inited)
					{
						EffectVbProcess(&node->effectContext.vbContext,
										&node->effectParams.vbParams,
										pcmIn,
										pcmOut,
										samples);
					}
					break;

				case EffectType3D:
					if(!node->inited)
					{
						if(Effect3DInit(&node->effectContext.tdContext,
										&node->effectParams.tdParams,
										&node->pcmParams) == THREE_D_ERROR_OK)
						{
							node->inited = TRUE;
						}
					}

					if(node->inited)
					{
						Effect3DProcess(&node->effectContext.tdContext,
										&node->effectParams.tdParams, 
										pcmIn,
										pcmOut,
										samples);
					}

					break;
/*
				case EffectTypeEQ:
					if(!node->inited)
					{
						if(EffectEqInit(&node->effectContext.eqContext,
										&node->effectParams.eqParams,
										pcmParams) == RT_SUCCESS)
						{
							node->inited = TRUE;
						}
					}

					if(node->inited)
					{
						EffectEqProcess(&node->effectContext.eqContext,
										&node->effectParams.eqParams,
										pcmIn,
										pcmOut,
										samples);
					}
					break;
*/
				default:
					break;
			}
		}

		node = node->next;
	}
}
/*
static void MixPreProcess(uint8_t srcIndex)
{
	EffectListProcess(mixAudioCore.mixSource[srcIndex].effectList.head,
						mixAudioCore.mixSource[srcIndex].buf,
						mixAudioCore.mixSource[srcIndex].buf,
						mixAudioCore.mixSource[srcIndex].validSampleLen);
}
*/

static BOOL MixGetAndPreProcess(void)
{
	uint16_t				tempSampleLen = 0;
	uint8_t					i;


	for(i = 0; i < MAX_SOURCE_NUM; i++)
	{
		mixAudioCore.mixSource[i].validSampleLen = 0;

		if(mixAudioCore.mixSource[i].enable)
		{
			switch(mixAudioCore.mixSource[i].srcType)
			{
				case SOURCE_TYPE_DECODER:
					mixAudioCore.mixSource[i].validSampleLen = 
							GetDecodedPcmData(mixAudioCore.mixSource[i].buf, DEFAULT_MIX_SRC_SAMPLE_LEN, mixAudioCore.mixSource[i].pcmParams.channelNums);
					break;
/*
				case SOURCE_TYPE_MIC:
					mixAudioCore.mixSource[i].validSampleLen = 
							MicGetData(mixAudioCore.mixSource[i].buf, DEFAULT_MIX_SRC_SAMPLE_LEN, mixAudioCore.mixSource[i].pcmParams.channelNums);
					break;

				case SOURCE_TYPE_LINE:
					mixAudioCore.mixSource[i].validSampleLen = 
							LineInGetData(mixAudioCore.mixSource[i].buf, DEFAULT_MIX_SRC_SAMPLE_LEN, mixAudioCore.mixSource[i].pcmParams.channelNums);
					break;
*/
				default:
					break;
			}
/*
			if(mixAudioCore.mixSource[i].validSampleLen != 0)
			{
				MixPreProcess(i);
			}
*/
		}

		tempSampleLen = tempSampleLen > mixAudioCore.mixSource[i].validSampleLen ? 
						tempSampleLen : mixAudioCore.mixSource[i].validSampleLen;

	}

	if(tempSampleLen == 0)
		return FALSE;

	return TRUE;
}

static uint8_t MixPostProcess(void)
{
	EffectListProcess(mixAudioCore.postEffectList.head,
						mixAudioCore.mixedBuf,
						mixAudioCore.mixedBuf,
						mixAudioCore.mixedSampleLen);

	return 1;
}

static BOOL MixProcess(void)
{
	uint8_t					index;


	/*Mix data*/
	mixAudioCore.mixedSampleLen = 0;
	memset(mixAudioCore.mixedBuf, 0, MIXED_BUFFER_LEN);


	for(index = 0; index < MAX_SOURCE_NUM; index++)
	{
		uint16_t		sampleLen;

		sampleLen = mixAudioCore.mixSource[index].validSampleLen;
		if(mixAudioCore.mixSource[index].enable)
		{
			if(sampleLen != 0)
			{
				switch(mixAudioCore.mixSource[index].pcmParams.channelNums)
				{
					case 1:			/* mono */
						break;

					case 2:			/* stereo */
						{
							uint16_t		i;
							int16_t			*srcBuf;
							int16_t 		*mixedBuf;

							srcBuf = mixAudioCore.mixSource[index].buf;
							mixedBuf = mixAudioCore.mixedBuf;

							for(i = 0; i < sampleLen; i++)
							{
								mixedBuf[i * 2]		= __ssat(mixedBuf[i * 2 + 0] + (int16_t)(srcBuf[i * 2]), 16);
								mixedBuf[i * 2 + 1]	= __ssat(mixedBuf[i * 2 + 1] + (int16_t)(srcBuf[i * 2 + 1]), 16);
							}
						}
						break;

					default:
						break;
				}
			}
		}

		mixAudioCore.mixedSampleLen = sampleLen > mixAudioCore.mixedSampleLen ? 
										sampleLen : mixAudioCore.mixedSampleLen;
	}

	/* TO DO : Post-Process */
	MixPostProcess();

	return TRUE;
}

uint8_t 	*putBuf;
uint16_t	dataLen;
uint16_t	putBufSize;

static BOOL DacI2sPutData(uint8_t * buf, uint16_t samples)
{
	BOOL		ret = TRUE;
	uint32_t	temp;
	
	if((PcmFifoGetStatus() == 1) && (PcmFifoGetRemainSamples() > PCM_FIFO_SIZE/8))
	{
		PcmFifoPlay();
	}

	
	if(samples * 4 >= putBufSize - dataLen)
	{
//		DBG("Full\n");
		ret = FALSE;
	}
	else
	{
		memcpy(putBuf+dataLen, buf, samples*4);
		dataLen += samples*4;
	}

	if(CheckXrTx() && PcmFifoGetRemainSamples() <= (PCM_FIFO_SIZE/4 - DEFAULT_MIX_SRC_SAMPLE_LEN))
	{
		if(GetTx())
		{
			if(PcmFifoIsEmpty())
			{
				DBG("Empty\n");
			}

//			if(dataLen != 0)
			{
				PcmTxTransferData(putBuf, putBuf, dataLen/4);
//				dataLen = 0;
//				DBG("%d\n", temp);
			}
		}
	}

	return ret;
}

static BOOL MixPut(void)
{
	uint8_t				index;
	uint8_t				ret;

	if(DacI2sPutData(mixAudioCore.mixedBuf, mixAudioCore.mixedSampleLen))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


static void InitEffectList(EffectList * effectList)
{
	effectList->effectNums = 0;
	effectList->head = NULL;
}

static int16_t InsertEffectToList(EffectList *effectList, EffectNode *node)
{
	EffectNode			*pNode;

	if(effectList == NULL || node == NULL)
		return -1;

	pNode = effectList->head;

	if(pNode == NULL)
	{
		effectList->head = node;
		effectList->effectNums++;
		node->index = effectList->effectNums;
		return node->index;
	}

	while(pNode)
	{
		if(pNode->next == NULL)
		{
			break;
		}

		pNode = pNode->next;
	}

	pNode->next = node;
	effectList->effectNums++;
	node->index = effectList->effectNums;

	return node->index;
}

static EffectNode * MakeEffectNode(EffectType effectType)
{
	EffectNode 			*node;

	node = (EffectNode*)pvPortMalloc(sizeof(EffectNode));
	if(node == NULL)
		return NULL;

	memset(node, 0, sizeof(EffectNode));

	node->type = effectType;
	node->inited = FALSE;

	return node;
}

static void EnableEffectNode(EffectList *effectList, uint8_t index)
{
	EffectNode			*pNode;

	if(effectList == NULL || index == 0)
		return;

	pNode = effectList->head;
	while(pNode && pNode->index != index)
	{
		pNode = pNode->next;
	}

	if(pNode == NULL)
		return;

	pNode->enable = TRUE;	
}

static void DisableEffectNode(EffectList *effectList, uint8_t index)
{
	EffectNode			*pNode;

	if(effectList == NULL || index == 0)
		return;

	pNode = effectList->head;
	while(pNode && pNode->index != index)
	{
		pNode = pNode->next;
	}

	if(pNode == NULL)
		return;

	pNode->enable = FALSE;	
}

static void SetGain(EffectList *effectList, uint8_t index, uint32_t gain)
{
	EffectNode			*pNode;

	if(effectList == NULL || index == 0)
		return;

	pNode = effectList->head;
	while(pNode && pNode->index != index)
	{
		pNode = pNode->next;
	}

	if(pNode == NULL)
		return;

	pNode->gain = gain;	
}

static int32_t GetGain(EffectList *effectList, uint8_t index)
{
	EffectNode			*pNode;

	if(effectList == NULL || index == 0)
		return -1;

	pNode = effectList->head;
	while(pNode && pNode->index != index)
	{
		pNode = pNode->next;
	}

	if(pNode == NULL)
		return -1;

	return pNode->gain;
}

static void PcmParamsConfig(EffectList *effectList, uint8_t index, AudioCorePcmParams *pcmParams)
{
	EffectNode			*pNode;

	if(effectList == NULL || index == 0)
		return;

	pNode = effectList->head;
	while(pNode && pNode->index != index)
	{
		pNode = pNode->next;
	}

	if(pNode == NULL)
		return;

//	memcpy(&pNode->pcmParams, pcmParams, sizeof(AudioCorePcmParams));
	pNode->pcmParams.channelNums	= pcmParams->channelNums;
	pNode->pcmParams.pcmDataMode	= pcmParams->pcmDataMode;
	pNode->pcmParams.samplingRate	= pcmParams->samplingRate;
	pNode->pcmParams.bitRate		= pcmParams->bitRate;
	
}

static void SetVbParams(ParamsVB *paramsVB)
{
	EffectNode			*pNode;
	ParamsVB			oldParams;

	pNode = mixAudioCore.postEffectList.head;
	while(pNode && pNode->type != EffectTypeVB)
	{
		pNode = pNode->next;
	}

	if(pNode == NULL)
		return;

	oldParams.cutoffFreq	= pNode->effectParams.vbParams.cutoffFreq;
	oldParams.intensity		= pNode->effectParams.vbParams.intensity;
	oldParams.enhanced		= pNode->effectParams.vbParams.enhanced;

	if(oldParams.cutoffFreq != paramsVB->cutoffFreq)
	{
		pNode->inited = FALSE;
	}

//	memcpy(&pNode->effectParams.vbParams, paramsVB, sizeof(ParamsVB));
	pNode->effectParams.vbParams.cutoffFreq = paramsVB->cutoffFreq;
	pNode->effectParams.vbParams.enhanced = paramsVB->enhanced;
	pNode->effectParams.vbParams.intensity = paramsVB->intensity;
	
}

static void GetVbParams(ParamsVB *paramsVB)
{
	EffectNode			*pNode;

	if(paramsVB == NULL)
		return;

	memset(paramsVB, 0, sizeof(ParamsVB));

	pNode = mixAudioCore.postEffectList.head;
	while(pNode && pNode->type != EffectTypeVB)
	{
		pNode = pNode->next;
	}

	if(pNode == NULL)
		return;

//	memcpy(paramsVB, &pNode->effectParams.vbParams, sizeof(ParamsVB));
	paramsVB->cutoffFreq = pNode->effectParams.vbParams.cutoffFreq;
	paramsVB->enhanced = pNode->effectParams.vbParams.enhanced;
	paramsVB->intensity = pNode->effectParams.vbParams.intensity;
}

static void Set3DParams(Params3D *params3D)
{
	EffectNode			*pNode;


	pNode = mixAudioCore.postEffectList.head;
	while(pNode && pNode->type != EffectType3D)
	{
		pNode = pNode->next;
	}

	if(pNode == NULL)
		return;

//	memcpy(&pNode->effectParams.tdParams, params3D, sizeof(Params3D));
	pNode->effectParams.tdParams.depth = params3D->depth;
	pNode->effectParams.tdParams.preGain = params3D->preGain;
	pNode->effectParams.tdParams.postGain = params3D->postGain;
	
}

static void Get3DParams(Params3D *params3D)
{
	EffectNode			*pNode;

	if(params3D == NULL)
		return;

	memset(params3D, 0, sizeof(Params3D));

	pNode = mixAudioCore.postEffectList.head;
	while(pNode && pNode->type != EffectType3D)
	{
		pNode = pNode->next;
	}

	if(pNode == NULL)
		return;

//	memcpy(params3D, &pNode->effectParams.tdParams, sizeof(Params3D));
	params3D->depth = pNode->effectParams.tdParams.depth;
	params3D->preGain = pNode->effectParams.tdParams.preGain;
	params3D->postGain = pNode->effectParams.tdParams.postGain;

}

static BOOL MixProcessCommand(MixCommandType cmdType, uint32_t * params)
{
	uint32_t		*tmpParams;
	BOOL			ret = FALSE;

	if(cmdType == MixCmdNone)
		return FALSE;

	switch(cmdType)
	{
		case MixCmdVBEn:
			tmpParams = params;
			if(*tmpParams == 0)
				DisableEffectNode(&mixAudioCore.postEffectList, 1);
			else
				EnableEffectNode(&mixAudioCore.postEffectList, 1);

			ret = TRUE;
			break;

		case MixCmdVBCutoff:
			{
				ParamsVB	paramsVB;
				
				tmpParams = params;

				GetVbParams(&paramsVB);
				paramsVB.cutoffFreq = *tmpParams;
				SetVbParams(&paramsVB);
			}
			ret = TRUE;
			break;

		case MixCmdVBIntensity:
			{
				ParamsVB	paramsVB;
				
				tmpParams = params;

				GetVbParams(&paramsVB);
				paramsVB.intensity = *tmpParams;
				SetVbParams(&paramsVB);
			}
			ret = TRUE;
			break;

		case MixCmdVBEnhanced:
			{
				ParamsVB	paramsVB;
				
				tmpParams = params;

				GetVbParams(&paramsVB);
				paramsVB.enhanced = *tmpParams;
				SetVbParams(&paramsVB);
			}
			ret = TRUE;
			break;

		case MixCmd3DEn:
			tmpParams = params;
			if(*tmpParams == 0)
				DisableEffectNode(&mixAudioCore.postEffectList, 2);
			else
				EnableEffectNode(&mixAudioCore.postEffectList, 2);
			ret = TRUE;
			break;

		case MixCmd3DDepth:
			{
				Params3D	params3D;

				tmpParams = params;
				Get3DParams(&params3D);
				params3D.depth = *tmpParams;
				Set3DParams(&params3D);
			}
			ret = TRUE;
			break;

		case MixCmd3DPreGain:
			{
				Params3D	params3D;

				tmpParams = params;
				Get3DParams(&params3D);
				params3D.preGain = *tmpParams;
				Set3DParams(&params3D);
			}
			ret = TRUE;
			break;

		case MixCmd3DPostGain:
			{
				Params3D	params3D;

				tmpParams = params;
				Get3DParams(&params3D);
				params3D.postGain = *tmpParams;
				Set3DParams(&params3D);
			}
			ret = TRUE;
			break;

		case MixCmdNone:
		default:
			break;
	}

	return ret;

}

/***************************************************************************************
 *
 * APIs
 *
 */


BOOL AudioCoreMixInit(void)
{
	uint8_t				i;
	uint8_t				tmpIndex;
	EffectNode			*tmpEffectNode;
	AudioCorePcmParams	tmpPcmParams;
	ParamsVB			tmpParamsVb;
	Params3D			tmpParams3D;
	
	memset(&mixAudioCore, 0, sizeof(MixAcContext));

	mixAudioCore.mixSource[0].srcType = SOURCE_TYPE_DECODER;
	mixAudioCore.mixSource[0].enable = FALSE;
	mixAudioCore.mixSource[0].gain = 4095;
	mixAudioCore.mixSource[0].validSampleLen = 0;
	mixAudioCore.mixSource[0].buf = (uint8_t *)pvPortMalloc(MIX_SRC_BUFFER_LEN);


	mixAudioCore.mixSource[1].srcType = SOURCE_TYPE_MIC;
	mixAudioCore.mixSource[1].enable = FALSE;
	mixAudioCore.mixSource[1].gain = 4095;
	mixAudioCore.mixSource[1].validSampleLen = 0;
	mixAudioCore.mixSource[1].buf = (uint8_t *)pvPortMalloc(MIX_SRC_BUFFER_LEN);


	mixAudioCore.mixSource[2].srcType = SOURCE_TYPE_LINE;
	mixAudioCore.mixSource[2].enable = FALSE;
	mixAudioCore.mixSource[2].gain = 4095;
	mixAudioCore.mixSource[2].validSampleLen = 0;
	mixAudioCore.mixSource[2].buf = (uint8_t *)pvPortMalloc(MIX_SRC_BUFFER_LEN);


	/* Allocate mixed buffer */
//	mixAudioCore.mixedBuf = (int16_t *)(VMEM_ADDR + AUDIOC_CORE_TRANSFER_OFFSET);
	mixAudioCore.mixedBuf = (int16_t *)pvPortMalloc(MIXED_BUFFER_LEN);

	if(mixAudioCore.mixedBuf == NULL)
		return FALSE;

	mixAudioCore.postEffectList.effectNums = 2;
	mixAudioCore.postEffectList.head = NULL;

	mixAudioCore.mixCmdType = MixCmdNone;

	tmpPcmParams.samplingRate = 44100;
	tmpPcmParams.channelNums = 2;

	InitEffectList(&mixAudioCore.postEffectList);

	tmpEffectNode = MakeEffectNode(EffectTypeVB);
	tmpIndex = InsertEffectToList(&mixAudioCore.postEffectList, tmpEffectNode);
	EnableEffectNode(&mixAudioCore.postEffectList, tmpIndex);
	PcmParamsConfig(&mixAudioCore.postEffectList, tmpIndex, &tmpPcmParams);

	tmpParamsVb.cutoffFreq	= 100;
	tmpParamsVb.intensity	= 35;
	tmpParamsVb.enhanced	= 1;
	SetVbParams(&tmpParamsVb);

	tmpEffectNode = MakeEffectNode(EffectType3D);
	tmpIndex = InsertEffectToList(&mixAudioCore.postEffectList, tmpEffectNode);
	EnableEffectNode(&mixAudioCore.postEffectList, tmpIndex);
	PcmParamsConfig(&mixAudioCore.postEffectList, tmpIndex, &tmpPcmParams);

	tmpParams3D.depth		= 50;
	tmpParams3D.preGain 	= 32768;
	tmpParams3D.postGain	= 32768;
	Set3DParams(&tmpParams3D);

	putBuf = (int16_t *)(VMEM_ADDR + AUDIOC_CORE_TRANSFER_OFFSET);
	dataLen = 0;
	putBufSize = AUDIOC_CORE_TRANSFER_SIZE;


	return TRUE;
}

BOOL AudioCoreMixDeinit(void)
{
	uint8_t			i;

	/* Free allocated source buffer */
	for(i = 0; i < MAX_SOURCE_NUM; i++)
	{
		if(mixAudioCore.mixSource[i].buf)
		{
			vPortFree(mixAudioCore.mixSource[i].buf);
			mixAudioCore.mixSource[i].buf = NULL;
		}
	}

	/* Free allocated mixed buffer */
	if(mixAudioCore.mixedBuf)
	{
		vPortFree(mixAudioCore.mixedBuf);
		mixAudioCore.mixedBuf = NULL;
	}

	return TRUE;
}

/**
 * Source related functions
 */

BOOL AudioCoreMixSourceEnable(AudioCoreIndex srcIndex)
{
	if(srcIndex >= MAX_SOURCE_NUM)
		return FALSE;

	mixAudioCore.mixSource[srcIndex].enable = TRUE;
}

BOOL AudioCoreMixSourceDisable(AudioCoreIndex srcIndex)
{
	if(srcIndex >= MAX_SOURCE_NUM)
		return FALSE;

	mixAudioCore.mixSource[srcIndex].enable = FALSE;
}

BOOL AudioCoreMixSourcePcmConfig(AudioCoreIndex srcIndex, AudioCorePcmParams *pcmParams)
{
	if(srcIndex >= MAX_SOURCE_NUM)
		return FALSE;

	memcpy(&mixAudioCore.mixSource[srcIndex].pcmParams, pcmParams, sizeof(AudioCorePcmParams));

	return TRUE;
}

BOOL AudioCoreMixSourceGainConfig(AudioCoreIndex srcIndex, int16_t gain)
{
	if(srcIndex >= MAX_SOURCE_NUM)
		return FALSE;

	mixAudioCore.mixSource[srcIndex].gain = gain;
}

/**
 * Sink related functions
 */

void MixCommand(MixCommandType cmdType, uint32_t * cmdParams)
{
	mixAudioCore.mixCmdType = cmdType;
	mixAudioCore.mixCmdParams = cmdParams;
}

void AudioCoreMixProcess(void)
{
	BOOL		ret;
	
//	DBG("M%d\n", mixAudioCore.mixState);
	while(1)
	{
		if(MixProcessCommand(mixAudioCore.mixCmdType, mixAudioCore.mixCmdParams))
		{
			mixAudioCore.mixCmdType = MixCmdNone;
		}

		switch(mixAudioCore.mixState)
		{
			case MixStateStateGetData:
				ret = MixGetAndPreProcess();
				if(!ret)
				{
					return;
				}
				mixAudioCore.mixState = MixStateMix;

			case MixStateMix:
				ret = MixProcess();
				if(!ret)
				{
					return;
				}
				mixAudioCore.mixState = MixStatePuttingData;

			case MixStatePuttingData:
				ret = MixPut();
				if(!ret)
				{
					return;
				}
				mixAudioCore.mixState = MixStateStateGetData;

			default:
				break;
		}
	}
}


