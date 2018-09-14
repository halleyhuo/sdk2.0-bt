/**
 **************************************************************************************
 * @file    tip_sound.h
 * @brief   
 *
 * @author  kk
 * @version V1.0.0
 *
 * $Created: 2017-3-17 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __TIP_SOUND_H__
#define __TIP_SOUND_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"
#include "audio_utility.h"
#include "mv_ring_buffer.h"


typedef enum _SOUND_ID
{
 	SOUND_KJ = 0,						//1����	
 	SOUND_MYLW,							//2û����������������
 	SOUND_QXPW,							//3û���ҵ����磬����������
 	SOUND_LWCG,							//4�������ӳɹ�
 	SOUND_WLDK,							//5�����ѶϿ�
 	SOUND_SSWL,							//6������������
 	SOUND_SZWL,							//7���APP�����õ�ǰ����
	SOUND_SJRJ,							//8�����µ�����汾�����������
	SOUND_SDXX,							//9�յ�����Ϣ
	SOUND_DBGQ,							//10���APP�㲥����
	SOUND_SJCG,							//11��������ɹ�
    SOUND_KSSJ,							//12��ʼ����������������ɹ�֮ǰ�벻Ҫ�ϵ�
    SOUND_BYSJ,							//13����������°汾������Ҫ����
    SOUND_SCCG,							//14�ղسɹ�
    SOUND_DD,							//15"��"
    SOUND_XX,							//16"��"
    SOUND_QXSC,							//17��ȡ���ղ�
    SOUND_QCJL,							//18�����������¼
    SOUND_XXGD,							//19��Ϣ����
    SOUND_SJSB,							//20�������ʧ��
    SOUND_XTFM,							//21ϵͳ��æ�����Ժ�����
    SOUND_XXGQ,							//22��Ϣ�ѹ���
    SOUND_LJSB,							//23��������ʧ��
    SOUND_DLWX,							//24���ڵ�½΢�ţ����Ե�
    SOUND_XZXX,							//25����δ����Ϣ
    SOUND_JSCD,							//26��������,�뼰ʱ���
    SOUND_ZDGJ,							//27��������,�Զ��ػ�
} SOUND_ID;

uint32_t TipSoundFillStreamCallback(void *buffer, uint32_t length);

MemHandle * GetTipSoundDecoderBuffer(void);


MvRingBuffer * GetTipSoundAudioCoreBuffer(void);


int32_t SaveTipSoundDecodedPcmData(int32_t * data, uint16_t dataLen, uint32_t channels);


void GetTipSoundMutex(void);


void ReleaseTipSoundMutex(void);


void GetTipSoundPcmBufMutex(void);


void ReleaseTipSoundPcmBufMutex(void);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif /* __TIP_SOUND_H__ */

