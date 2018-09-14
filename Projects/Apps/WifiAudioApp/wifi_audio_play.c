/**
 **************************************************************************************
 * @file    wifi_audio_play.c
 * @brief   
 *
 * @author  halley
 * @version V1.0.0
 *
 * $Created: 2016-6-29 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include <string.h>
#include <stdio.h>

#include "chip_info.h"

#include "type.h"
#include "rtos_api.h"
#include "audio_utility.h"
#include "mv_ring_buffer.h"
#include "main_app.h"
#include "app_message.h"
#include "audio_decoder.h"
#include "decoder_service.h"
#include "wifi_audio_play.h"
#include "pcm_transfer.h"
#include "dac.h"
#include "audio_path.h"
#include "app_config.h"
#include "clk.h"
#include "pcm_fifo.h"
#include "key.h"

#include "audio_core.h"

#include "audio_core_service.h"
#include "decoder_service.h"
#include "wifi_audio_service.h"

#include "sys_app.h"
#include "audio_decoder.h"

/***************************************************************************************
 *
 * External defines
 *
 */

#define WIFI_AUDIO_PLAY_TASK_STACK_SIZE		1024
#define WIFI_AUDIO_PLAY_TASK_PRIO			3

#define WIFI_THRESHOLD_LEN					(20*1024)

/***************************************************************************************
 *
 * Internal defines
 *
 */

typedef struct _WifiAudioPlayContext
{
	xTaskHandle 		taskHandle;
	MessageHandle		msgHandle;
	MessageHandle		parentMsgHandle;

	ServiceState		state;

	ServiceState		wifiAudioServiceState;
	ServiceState		decoderServiceState;

	uint32_t			AppState;
	APP_PlayEvent		AppEvent;

	MessageHandle		wifiAudioServiceMsgHandle;
	MessageHandle		decoderServiceMsgHandle;

	uint8_t				*snkBuf;
	uint32_t			snkBufSize;
	
	bool				gIsCurFileDownLoadFinished;

}WifiAudioPlayContext;


/***************************************************************************************
 *
 * Internal varibles
 *
 */

#define WIFI_AUDIO_NUM_MESSAGE_QUEUE		10

static WifiAudioPlayContext		wifiAudioPlayCt;


/** wifi audio play task name*/
const char wifiAudioPlayName[] = "WifiAudioPlay";

static MessageId wifiAudioKeyMap[][8] = 
{
	{MSG_NONE, MSG_WIFI_AUDIO_PLAY_PAUSE,	MSG_WIFI_AUDIO_STOP,		MSG_NONE,					MSG_NONE				},
	{MSG_NONE, MSG_WIFI_AUDIO_PRE,			MSG_WIFI_AUDIO_VOLDOWN, 	MSG_WIFI_AUDIO_VOLDOWN,		MSG_NONE				},
	{MSG_NONE, MSG_WIFI_AUDIO_NEXT,			MSG_WIFI_AUDIO_VOLUP,		MSG_WIFI_AUDIO_VOLUP,		MSG_NONE				},
	{MSG_NONE, MSG_WIFI_AUDIO_READ_WX,		MSG_NONE,					MSG_NONE,					MSG_NONE				},
	{MSG_NONE, MSG_NONE,					MSG_WIFI_AUDIO_RECORD_WX,	MSG_NONE,					MSG_WIFI_AUDIO_SEND_WX},
	{MSG_NONE, MSG_NONE,					MSG_NONE,					MSG_NONE,					MSG_NONE				},
	{MSG_NONE, MSG_NONE,					MSG_NONE,					MSG_NONE,					MSG_NONE				},
	{MSG_NONE, MSG_SMARTLINK,				MSG_NONE,					MSG_NONE,					MSG_NONE				}
};

uint32_t vbEnabled = 0;


/***************************************************************************************
 *
 * Internal functions
 *
 */

MessageId GetWifiAudioKeyMap(KeyValue value, KeyEvent event)
{
	return wifiAudioKeyMap[value][event];
}


static uint16_t WifiAudioSinkPutData(int16_t * buf, uint16_t samples, uint16_t channelNums)
{
	if(CheckXrTx() && PcmFifoGetRemainSamples() <= (PCM_FIFO_SIZE/4 - samples))
	{
		if(GetTx())
		{
			if(PcmFifoIsEmpty())
			{
				DBG("Empty\n");
			}

			{
				PcmTxTransferData(buf, buf, samples);

				return 1;
			}
		}
	}

	return 0;
}

static void ConfigPhub4WifiAudioPlay(void)
{
	/* DAC initialization */
	CodecDacInit(TRUE);
	CodecDacMuteSet(TRUE, TRUE);
	CodecDacChannelSel(DAC_CH_DECD_L | DAC_CH_DECD_R);
	DacVolumeSet(0x200, 0x200);
	DacNoUseCapacityToPlay();
	DacConfig(MDAC_DATA_FROM_DEC, USB_MODE);
	/* PCMFIFO_MIX_DACOUT */
	PhubPathSel(PCMFIFO_MIX_DACOUT);
	CodecDacMuteSet(FALSE, FALSE);

	PcmFifoInitialize(PCM_FIFO_OFFSET, PCM_FIFO_SIZE, 0, 0);
}

static void DeconfigPhub2WifiAudioPlay(void)
{

}

static void ConfigMemory4WifiAudioPlay(void)
{


}

static void DeconfigMemory4WifiAudioPlay(void)
{
	
}

static void DeocderCallback(DecoderServiceEvent event, DecoderCallbackParams * param)
{
	switch(event)
	{

		case DS_EVENT_DECODE_INITED:
			PcmTxSetPcmDataMode((PCM_DATA_MODE)param->songInfo->pcm_data_mode);
			DacAdcSampleRateSet(param->songInfo->sampling_rate, USB_MODE);
			DBG("samplerate = %d\n",param->songInfo->sampling_rate);
			NVIC_EnableIRQ(DECODER_IRQn);

			break;

		default:
			break;
	}
}

static void WifiAudioPlayModeStart(void)
{

}

static void WifiAudioPlayModeStop(void)
{

}

static void WifiAudioPlayServicesCreate(void)
{
	// Create decoder service
	DecoderServiceCreate(wifiAudioPlayCt.msgHandle);

	// Create wifi audio servcie
	WifiAudioServiceCreate(wifiAudioPlayCt.msgHandle);
}

static void MsgProcessServiceCreated(uint16_t msgParams)
{
	MessageContext		msgSend;

	if(msgParams == MSG_PARAM_WIFI_AUDIO_SERVICE)
	{
		wifiAudioPlayCt.wifiAudioServiceState = ServiceStateReady;
		wifiAudioPlayCt.wifiAudioServiceMsgHandle = GetWifiAudioServiceMsgHandle();
	}
	else if(msgParams == MSG_PARAM_DECODER_SERVICE)
	{
		wifiAudioPlayCt.decoderServiceState = ServiceStateReady;
		wifiAudioPlayCt.decoderServiceMsgHandle = GetDecoderServiceMsgHandle();
	}


	if(wifiAudioPlayCt.decoderServiceState == ServiceStateReady)
	{
		if(wifiAudioPlayCt.wifiAudioServiceState == ServiceStateReady)
		{
			wifiAudioPlayCt.state = ServiceStateReady;

			/* 
			 * All of services is created
			 * Send CREATED message to parent
			 */
			msgSend.msgId		= MSG_SERVICE_CREATED;
			msgSend.msgParams	= MSG_PARAM_WIFI_AUDIO_PLAY;
			MessageSend(wifiAudioPlayCt.parentMsgHandle, &msgSend);
		}
		else if(wifiAudioPlayCt.wifiAudioServiceState == ServiceStatePaused)
		{
			DecoderServiceStart();
			WifiAudioServiceResume();
		}
	}
}

static void MsgProcessServiceStarted(uint16_t msgParams)
{
	MessageContext		msgSend;
	
	if(msgParams == MSG_PARAM_WIFI_AUDIO_SERVICE)
	{
		wifiAudioPlayCt.wifiAudioServiceState = ServiceStateRunning;
	}
	else if(msgParams == MSG_PARAM_DECODER_SERVICE)
	{
		wifiAudioPlayCt.decoderServiceState = ServiceStateRunning;
	}


	if( wifiAudioPlayCt.decoderServiceState == ServiceStateRunning
		&& wifiAudioPlayCt.wifiAudioServiceState == ServiceStateRunning)
	{
		wifiAudioPlayCt.state = ServiceStateRunning;
		
		/* 
		 * All of services is started
		 * Send STARTED message to parent
		 */
		msgSend.msgId		= MSG_SERVICE_STARTED;
		msgSend.msgParams	= MSG_PARAM_WIFI_AUDIO_PLAY;

		MessageSend(wifiAudioPlayCt.parentMsgHandle, &msgSend);
	}
}

static void MsgProcessServicePaused(uint16_t msgParams)
{
	MessageContext		msgSend;

	if(msgParams == MSG_PARAM_WIFI_AUDIO_SERVICE)
	{
		wifiAudioPlayCt.wifiAudioServiceState = ServiceStatePaused;
	}

	if( wifiAudioPlayCt.decoderServiceState == ServiceStateStopped
		&& wifiAudioPlayCt.wifiAudioServiceState == ServiceStatePaused )
	{
		wifiAudioPlayCt.state = ServiceStatePaused;

		msgSend.msgId		= MSG_SERVICE_PAUSED;
		msgSend.msgParams	= MSG_PARAM_WIFI_AUDIO_PLAY;

		MessageSend(wifiAudioPlayCt.parentMsgHandle, &msgSend);
	}
}

static void MsgProcessServiceStopped(uint16_t msgParams)
{
	MessageContext		msgSend;

	if(msgParams == MSG_PARAM_WIFI_AUDIO_SERVICE)
	{
		wifiAudioPlayCt.wifiAudioServiceState = ServiceStateStopped;
		WifiAudioServiceKill();
	}
	else if(msgParams == MSG_PARAM_DECODER_SERVICE)
	{
		wifiAudioPlayCt.decoderServiceState = ServiceStateStopped;
		DecoderServiceKill();
	}


	if(wifiAudioPlayCt.decoderServiceState == ServiceStateStopped )
	{
		if(wifiAudioPlayCt.wifiAudioServiceState == ServiceStateStopped)
		{
			wifiAudioPlayCt.state = ServiceStateStopped;
			/* 
			 * All of services is stopped
			 * Send STOPPED message to parent
			 */
			msgSend.msgId		= MSG_SERVICE_STOPPED;
			msgSend.msgParams	= MSG_PARAM_WIFI_AUDIO_PLAY;

			MessageSend(wifiAudioPlayCt.parentMsgHandle, &msgSend);
		}
		else if(wifiAudioPlayCt.wifiAudioServiceState == ServiceStatePaused)
		{
			wifiAudioPlayCt.state = ServiceStatePaused;
			
			msgSend.msgId		= MSG_SERVICE_PAUSED;
			msgSend.msgParams	= MSG_PARAM_WIFI_AUDIO_PLAY;

			MessageSend(wifiAudioPlayCt.parentMsgHandle, &msgSend);
		}
	}
}

static void MsgProcessServiceStart(uint16_t msgParams)
{

	ServiceState		state;

	state = wifiAudioPlayCt.state;

	switch(state)
	{
		case ServiceStateReady:
			{
				// Init & start releated services
				wifiAudioPlayCt.state = ServiceStateStarting;
				WifiAudioServiceInit(WIFI_THRESHOLD_LEN);
//				WifiAudioServiceStart();
			}
			break;
		default:
			break;
	}
}

static void MsgProcessServicePause(uint16_t msgParams)
{

	ServiceState		state;

	state = wifiAudioPlayCt.state;

	switch(state)
	{
		case ServiceStateRunning:
			{
				wifiAudioPlayCt.state = ServiceStatePausing;
				WifiAudioServicePause();
				DecoderServiceStop();
			}
			break;

		default:
			break;
	}
}

static void MsgProcessServiceResume(uint16_t msgParams)
{

	ServiceState		state;

	state = wifiAudioPlayCt.state;
	switch(state)
		{
			case ServiceStatePaused:
				{
					wifiAudioPlayCt.state = ServiceStateResuming;

					// Resume wifi audio service
					WifiAudioServiceResume();
					// Create decoder service
					DecoderServiceCreate(wifiAudioPlayCt.msgHandle);
					
				}
				break;
	
			default:
				break;
		}
}

static void MsgProcessServiceStop(uint16_t msgParams)
{
	wifiAudioPlayCt.state = ServiceStateStopping;


	if(wifiAudioPlayCt.decoderServiceState != ServiceStateStopped)
	{
		DecoderServiceStop();
	}

	if(wifiAudioPlayCt.wifiAudioServiceState != ServiceStateStopped)
	{
		WifiAudioServiceStop();
	}
}

static void MsgProcessWifiDataEmpty(void)
{
	 DBG("MsgProcessWifiDataEmpty\n");
}

static void MsgProcessWifiDataReady(void)
{
	int32_t					ret;
	SongInfo				*songInfo;
	ACPcmInfo				acPcmInfo;

	DBG("MsgProcessWifiDataReady\n");

	// Initialize decoder
	ret = DecoderInit(GetWifiAudioServiceBuf(), IO_TYPE_MEMORY, MP3_DECODER);

	if(ret != RT_SUCCESS)
	{
		DBG("Decoder Init Error\n");
		return;
 	}
 
	songInfo = GetSongInfo();
	acPcmInfo.bitRate = songInfo->bitrate;
	acPcmInfo.channelNums = songInfo->num_channels;
	acPcmInfo.pcmDataMode = songInfo->pcm_data_mode;
	acPcmInfo.samplingRate = songInfo->sampling_rate;

	AudioCoreSourceConfigPcmInfo(1, &acPcmInfo);//临时代码，该模式下只有decoder，编号为1

	DecoderServiceStart();
}

static void WifitAudioPlaykEntrance(void * param)
{
	MessageContext		msgRecv;
	MessageContext		msgSend;
	MessageHandle		msgHandle;

	// Create services
	WifiAudioPlayServicesCreate();

	while(1)
	{
		MessageRecv(wifiAudioPlayCt.msgHandle, &msgRecv, MAX_RECV_MSG_TIMEOUT);

		switch(msgRecv.msgId)
		{
			case MSG_SERVICE_CREATED:
				{
					MsgProcessServiceCreated(msgRecv.msgParams);
				}
				break;

			case MSG_SERVICE_STARTED:
				{
					MsgProcessServiceStarted(msgRecv.msgParams);
				}
				break;

			case MSG_SERVICE_PAUSED:
				{
					MsgProcessServicePaused(msgRecv.msgParams);
				}
				break;

			case MSG_SERVICE_STOPPED:
				{
					MsgProcessServiceStopped(msgRecv.msgParams);
				}
				break;

			case MSG_SERVICE_START:
				{
					MsgProcessServiceStart(msgRecv.msgParams);
				}
				break;

			case MSG_SERVICE_PAUSE:
				{
					MsgProcessServicePause(msgRecv.msgParams);
				}
				break;

			case MSG_SERVICE_RESUME:
				{
					MsgProcessServiceResume(msgRecv.msgParams);
				}
				break;

			case MSG_SERVICE_STOP:
				{
					MsgProcessServiceStop(msgRecv.msgParams);
				}
				break;

			case MSG_WIFI_AUDIO_SERVICE_DATA_READY:
				{
					MsgProcessWifiDataReady();
				}
				break;

			case MSG_WIFI_AUDIO_SERVICE_DATA_EMPTY:
				{
					MsgProcessWifiDataEmpty();
				}
				break;
			case MSG_WIFI_AUDIO_SERVICE_DATA_NOT_ENOUGH:
				{
					if(TRUE == wifiAudioPlayCt.gIsCurFileDownLoadFinished)
					{
						uint8_t *playUrl;
						DecoderServiceStop();
						
						GetNextPreSongURL(TRUE);//下一首
						playUrl = GetWifiPlayUrl();
						//playUrl = GetNextPreSongURL(TRUE);//下一首
						WifiAudioServiceConnect(playUrl, 192);
						
						WifiAudioServiceStart();
						wifiAudioPlayCt.gIsCurFileDownLoadFinished = FALSE;
					}
				}
				break;
			case MSG_DECODER_SERVICE_FIFO_EMPTY:
				if(FALSE == wifiAudioPlayCt.gIsCurFileDownLoadFinished)
				{
					break;
				}
				DBG("\nwifi song end, play next song\n");
			case MSG_WIFI_AUDIO_SONG_PLAY_END://准备播放下一首歌曲
				{
					uint8_t *playUrl;
					DecoderServiceStop();
					
					GetNextPreSongURL(TRUE);//下一首
					playUrl = GetWifiPlayUrl();
					WifiAudioServiceConnect(playUrl, 192);
					
					WifiAudioServiceStart();
					wifiAudioPlayCt.gIsCurFileDownLoadFinished = FALSE;
				}
				break;
			case MSG_WIFI_AUDIO_SONG_PLAY_NEXT:
				DBG("MSG_WIFI_AUDIO_SONG_PLAY_NEXT\n");
				{
					uint8_t *playUrl;
					
					GetNextPreSongURL(TRUE);//下一首
					playUrl = GetWifiPlayUrl();
					WifiAudioServiceConnect(playUrl, 192);
					
					WifiAudioServiceStart();
					wifiAudioPlayCt.gIsCurFileDownLoadFinished = FALSE;
				}
				break;
			case MSG_APP_MODE_START:
				WifiAudioPlayModeStart();
				break;
			case MSG_APP_MODE_STOP:
				WifiAudioPlayModeStop();
				break;

			case MSG_WIFI_AUDIO_PUSH_SONG:
				DBG("MSG_WIFI_AUDIO_PUSH_SONG\n");
				{
					uint8_t *playUrl;
					DecoderServiceStop();
					
					playUrl = GetWifiPlayUrl();
					WifiAudioServiceConnect(playUrl, 192);
					
					WifiAudioServiceStart();
				}
				break;

			case MSG_WIFI_AUDIO_PLAY_PAUSE:
				DBG("MSG_WIFI_AUDIO_PLAY_PAUSE\n");
				break;
			
			case MSG_WIFI_AUDIO_STOP:
				DBG("MSG_WIFI_AUDIO_STOP\n");
				break;
			
			case MSG_WIFI_AUDIO_NEXT:
				DBG("MSG_WIFI_AUDIO_NEXT\n");
				{
					DecoderServiceStop();
					WifiAudioDownLoadStop();
					wifiAudioPlayCt.AppEvent = AppEventNext;
				}
				break;
			case MSG_WIFI_AUDIO_PRE:
				DBG("MSG_WIFI_AUDIO_PRE\n");
				break;
			
			case MSG_WIFI_AUDIO_VOLUP:
				DBG("MSG_WIFI_AUDIO_VOLUP\n");
				break;
			
			case MSG_WIFI_AUDIO_VOLDOWN:
				DBG("MSG_WIFI_AUDIO_VOLDOWN\n");
				break;
			
			case MSG_WIFI_AUDIO_READ_WX:
				DBG("MSG_WIFI_AUDIO_READ_WX\n");
				break;
			
			case MSG_WIFI_AUDIO_RECORD_WX:
				DBG("MSG_WIFI_AUDIO_RECORD_WX\n");
				break;
			
			case MSG_WIFI_AUDIO_SEND_WX:
				DBG("MSG_WIFI_AUDIO_SEND_WX\n");
				break;


			case MSG_WIFI_AUDIO_DOWLOAD_START:
				DBG("MSG_WIFI_AUDIO_DOWLOAD_START\n");
				break;
				
			case MSG_WIFI_AUDIO_DOWLOAD_FINISHED:
				DBG("MSG_WIFI_AUDIO_DOWLOAD_FINISHED\n");
				wifiAudioPlayCt.gIsCurFileDownLoadFinished = TRUE;
				break;

			case MSG_WIFI_AUDIO_SERVICE_DOWLOAD_STOPPED:
				DBG("MSG_WIFI_AUDIO_SERVICE_DOWNLODA_STOPPED\n");
				if(AppEventNext == wifiAudioPlayCt.AppEvent)
				{
					uint8_t *playUrl;
					
					GetNextPreSongURL(TRUE);//下一首
					playUrl = GetWifiPlayUrl();
					WifiAudioServiceConnect(playUrl, 192);
					
					WifiAudioServiceStart();
					wifiAudioPlayCt.gIsCurFileDownLoadFinished = FALSE;
				}
				break;


			case MSG_SMARTLINK:
				DBG("MSG_SMARTLINK\n");

				msgSend.msgId		= MSG_SMARTLINK;
				msgSend.msgParams	= NULL;
				MessageSend(wifiAudioPlayCt.parentMsgHandle, &msgSend);

				break;
		}

	}
}


static int32_t WAP_Init(MessageHandle parentMsgHandle)
{
	ACPcmInfo		pcmInfo;

	memset(&wifiAudioPlayCt, 0, sizeof(WifiAudioPlayContext));

	/* message handle */
	wifiAudioPlayCt.msgHandle = MessageRegister(WIFI_AUDIO_NUM_MESSAGE_QUEUE);

	/* Parent message handle */
	wifiAudioPlayCt.parentMsgHandle = parentMsgHandle;

	wifiAudioPlayCt.wifiAudioServiceState = ServiceStateNone;
	wifiAudioPlayCt.decoderServiceState = ServiceStateNone;
	wifiAudioPlayCt.gIsCurFileDownLoadFinished = FALSE;

	ConfigPhub4WifiAudioPlay();

	// config AudioCore


	pcmInfo.channelNums = 2;
	pcmInfo.samplingRate = 44100;

	AudioCoreSourceRegister(AC_SRC_DECODE, GetDecodedPcmData, &pcmInfo);

	AudioCoreProcSet(AC_PROC_MIX, &pcmInfo);

	AudioCoreProcEffectAdd(EffectTypeVB);
	AudioCoreProcEffectAdd(EffectType3D);

	wifiAudioPlayCt.snkBuf = (uint8_t *)(VMEM_ADDR + AUDIOC_CORE_TRANSFER_OFFSET);
	wifiAudioPlayCt.snkBufSize = AUDIOC_CORE_TRANSFER_SIZE;
	AudioCoreSinkRegister(AC_SNK_DAC, WifiAudioSinkPutData, wifiAudioPlayCt.snkBuf, wifiAudioPlayCt.snkBufSize, &pcmInfo);


	return 0;
}

static void WAP_Deinit(void)
{
	MessageContext		msgSend;
	MessageHandle		mainHandle;

	/* Deconfig audio path*/
	DeconfigPhub2WifiAudioPlay();


	/* Write storage parameters */
//	PrefrenceSave(btAudioPlayStorName, &BT_AUDIO_PLAY(params), sizeof(BtAudioPlayParams));

	/* Send message to main app */
	mainHandle = GetMainMessageHandle();

	msgSend.msgId		= MSG_SERVICE_STOPPED;
	msgSend.msgParams	= MSG_PARAM_WIFI_AUDIO_PLAY;
	MessageSend(mainHandle, &msgSend);

}

/***************************************************************************************
 *
 * APIs
 *
 */
MessageHandle GetWifiAudioPlayMessageHandle(void)
{
	return wifiAudioPlayCt.msgHandle;
}

int32_t WifiAudioPlayCreate(MessageHandle parentMsgHandle)
{
	WAP_Init(parentMsgHandle);
	xTaskCreate(WifitAudioPlaykEntrance, 
				wifiAudioPlayName,
				WIFI_AUDIO_PLAY_TASK_STACK_SIZE,
				NULL, WIFI_AUDIO_PLAY_TASK_PRIO,
				&wifiAudioPlayCt.taskHandle);

	return 0;
}

int32_t WifiAudioPlayKill(void)
{
	WAP_Deinit();
	vTaskDelete(wifiAudioPlayCt.taskHandle);

	return 0;
}

int32_t WifiAudioPlayStart(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_SERVICE_START;
	msgSend.msgParams	= NULL;
	MessageSend(wifiAudioPlayCt.msgHandle, &msgSend);

	return 0;
}

int32_t WifiAudioPlayPause(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_SERVICE_PAUSE;
	msgSend.msgParams	= NULL;
	MessageSend(wifiAudioPlayCt.msgHandle, &msgSend);

	return 0;
}

int32_t WifiAudioPlayResume(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_SERVICE_RESUME;
	msgSend.msgParams	= NULL;
	MessageSend(wifiAudioPlayCt.msgHandle, &msgSend);

	return 0;
}

int32_t WifiAudioPlayStop(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_SERVICE_STOP;
	msgSend.msgParams	= NULL;
	MessageSend(wifiAudioPlayCt.msgHandle, &msgSend);

	return 0;
}


