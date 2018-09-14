/**
 **************************************************************************************
 * @file    audio_core_manager.c
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
#include "type.h"
#include "rtos_api.h"
#include "app_message.h"
#include "main_app.h"
#include "audio_core_service.h"
#include "app_config.h"

/***************************************************************************************
 *
 * External defines
 *
 */

#define AUDIO_CORE_SERVICE_SIZE			1024
#define AUDIO_CORE_SERVICE_PRIO			3

#define AUDIO_CORE_SERVICE_TIMEOUT		1		/** 1 ms */


/***************************************************************************************
 *
 * Internal defines
 *
 */

typedef BOOL (*funcAcInit)(void);
typedef BOOL (*funcAcDeinit)(void);
typedef void (*funcAcRun)(void);

typedef struct _AudioCoreServiceContext
{
	xTaskHandle			taskHandle;
	MessageHandle		msgHandle;
	MessageHandle		parentMsgHandle;

	ServiceState		serviceState;

}AudioCoreServiceContext;

/** audio core servcie name*/
const char audioCoreServiceName[]		= "AudioCoreService";


/***************************************************************************************
 *
 * Internal varibles
 *
 */

#define ACS_NUM_MESSAGE_QUEUE		4


static AudioCoreServiceContext		audioCoreServiceCt;


/***************************************************************************************
 *
 * Internal functions
 *
 */

/**
 * @brief
 *		Audio core servcie init
 *
 * @param
 *		NONE
 *
 * @return
 *		0 for success
 */
static int32_t ACS_Init(MessageHandle parentMsgHandle)
{
	memset(&audioCoreServiceCt, 0, sizeof(AudioCoreServiceContext));

	/* register message handle */
	audioCoreServiceCt.msgHandle = MessageRegister(ACS_NUM_MESSAGE_QUEUE);

	audioCoreServiceCt.serviceState = ServiceStateCreating;

	audioCoreServiceCt.parentMsgHandle = parentMsgHandle;

	AudioCoreInit();

	return 0;
}


static void ACS_Deinit(void)
{
	AudioCoreDeinit();
	audioCoreServiceCt.msgHandle = NULL;
	audioCoreServiceCt.serviceState = ServiceStateNone;
	audioCoreServiceCt.parentMsgHandle = NULL;
}

static void AudioCoreServiceEntrance(void * param)
{
	MessageContext		msgRecv;
	MessageContext		msgSend;

	audioCoreServiceCt.serviceState = ServiceStateReady;

	/* Send message to parent */
	msgSend.msgId		= MSG_SERVICE_CREATED;
	msgSend.msgParams	= MSG_PARAM_AUDIO_CORE_SERVICE;
	MessageSend(audioCoreServiceCt.parentMsgHandle, &msgSend);

	while(1)
	{
		MessageRecv(audioCoreServiceCt.msgHandle, &msgRecv, AUDIO_CORE_SERVICE_TIMEOUT);
		switch(msgRecv.msgId)
		{
			case MSG_SERVICE_START:
				if(audioCoreServiceCt.serviceState == ServiceStateReady)
				{
					audioCoreServiceCt.serviceState = ServiceStateRunning;

					msgSend.msgId		= MSG_SERVICE_STARTED;
					msgSend.msgParams	= MSG_PARAM_AUDIO_CORE_SERVICE;
					MessageSend(audioCoreServiceCt.parentMsgHandle, &msgSend);
				}
				break;

			case MSG_SERVICE_STOP:
				if(audioCoreServiceCt.serviceState == ServiceStateRunning)
				{
					audioCoreServiceCt.serviceState = ServiceStateReady;

					msgSend.msgId		= MSG_SERVICE_STOPPED;
					msgSend.msgParams	= MSG_PARAM_AUDIO_CORE_SERVICE;
					MessageSend(audioCoreServiceCt.parentMsgHandle, &msgSend);
				}
				break;
		}

		if(audioCoreServiceCt.serviceState == ServiceStateRunning)
			AudioCoreRun();
	}
}


/***************************************************************************************
 *
 * APIs
 *
 */


/**
 * @brief
 *		Get message receive handle of audio core manager
 * 
 * @param
 *		NONE
 *
 * @return
 *		MessageHandle
 */
MessageHandle GetAudioCoreServiceMsgHandle(void)
{
	return audioCoreServiceCt.msgHandle;
}


ServiceState GetAudioCoreServiceState(void)
{
	return audioCoreServiceCt.serviceState;
}


/**
 * @brief
 *		Start audio core service.
 * @param
 * 	 NONE
 * @return  
 */
int32_t AudioCoreServiceCreate(MessageHandle parentMsgHandle)
{
	ACS_Init(parentMsgHandle);
	xTaskCreate(AudioCoreServiceEntrance, 
				audioCoreServiceName,
				AUDIO_CORE_SERVICE_SIZE,
				NULL, AUDIO_CORE_SERVICE_PRIO,
				&audioCoreServiceCt.taskHandle);
	return 0;
}

int32_t AudioCoreServiceStart(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_SERVICE_START;
	msgSend.msgParams	= NULL;
	MessageSend(audioCoreServiceCt.msgHandle, &msgSend);
}

/**
 * @brief
 *		Exit audio core service.
 * @param
 * 	 NONE
 * @return  
 */
void AudioCoreServiceStop(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_SERVICE_STOP;
	msgSend.msgParams	= NULL;
	MessageSend(audioCoreServiceCt.msgHandle, &msgSend);
}

int32_t AudioCoreServiceKill(void)
{
	ACS_Deinit();

	vTaskDelete(audioCoreServiceCt.taskHandle);

	return 0;
}

