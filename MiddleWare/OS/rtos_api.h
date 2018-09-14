/*
 * RTOS_API.c
 *
 *  Created on: Aug 30, 2016
 *      Author: peter
 */
 
#ifndef __RTOS_API_H__
#define __RTOS_API_H__

#include "type.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"


#define MessageHandle		QueueHandle_t

#define MSG_INVAILD			0x0000

typedef struct _MessageContext
{
	uint16_t		msgId;
	uint16_t		msgParams;
}MessageContext;


/**
* @brief
*	Register a message queue
*
* @param  maxMsgQueueNumber
*	The max number of message that the queue can receive.
*
* @retval
*	MessageHandle
*/
MessageHandle MessageRegister(uint16_t maxMsgQueueNumber);
#define MessageRegister(num) 			xQueueCreate((num), sizeof(MessageContext))

/**
* @brief
*	Deregister a message queue
*
* @param  msgHandle
*	
*
* @retval
*	NONE
*/
void MessageDeregister(MessageHandle msgHandle);
#define MessageDeregister(handle)		vQueueDelete((handle))

/**
* @brief
*	Clear all of message in the queue
*
* @param  msgHandle
*	The message queue which to be cleared
*
* @retval
*	NONE
*/
void MessageClear(MessageHandle msgHandle);
#define MessageClear(handle)			xQueueReset((handle))


/**
* @brief
*	Send Message
*
* @param 
*	msgHandle The message queue which will receive this message
* @param
*	msgContext The message to send
*
* @retval
*	TRUE or FALSE
*/
bool MessageSend(MessageHandle msgHandle,  MessageContext * msgContext);


/**
* @brief
*	Send Message extension function. 
*	It will not return until the message has been sent successful.
*	This function CAN NOT be used in interrupt functions
*
* @param 
*	msgHandle The message queue which will receive this message
* @param
*	msgContext The message to send
*
* @retval
*	NONE
*/
void MessageSendx(MessageHandle msgHandle,  MessageContext * msgContext);

/**
* @brief
*	Get a Message or Wait for a Message from a Queue.
*
* @param 
*	msgHandle The message queue handle
* @param 
*	millisec Timeout value or 0 in case of no time-out.
* @param
*	msgContext The message to send
*
* @retval
*	TRUE means received message and FALSE means timeout
*/
bool MessageRecv(MessageHandle msgHandle, MessageContext * msgContext, uint32_t millisec);

/**
* @brief
*	Init heap. 
*
* @param 
*	NONE
*
* @retval
*	NONE
*/
void InitialiseHeapArea(void);

#endif /*__RTOS_API_H__*/

