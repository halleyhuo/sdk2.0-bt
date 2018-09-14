/**
 **************************************************************************************
 * @file    wxcloud_api.h
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


#ifndef __WXCLOUD_API_H__
#define __WXCLOUD_API_H__

#include "type.h"
#include "airkiss_types.h"
#include "airkiss_cloudapi.h"


#define DEV_LINCENSE_LEN	256

#define DEV_ID_LEN			64

#define DEV_TYPE_LEN		32




typedef int32_t				WxcloudStatus;


#define WXCLOUD_STATUS_SUCCESS			0

#define WXCLOUD_STATUS_FAILED			-1

#define WXCLOUD_STATUS_PARAMS_ERR		-2

#define WXCLOUD_STATUS_NO_MORE_DATA		-3

#define WXCLOUD_STATUS_NOT_FOUND		-4


typedef struct _WxcloudParams
{
	uint8_t					*devLicense;
	uint16_t				devLicenseLen;
	uint8_t					*wxBuffer;
	uint16_t				wxBufferLen;
	ak_mutex_t				*taskMutex;
	ak_mutex_t				*mallocMutex;
	ak_mutex_t				*loopMutex;
	uint8_t					*property;
	uint16_t				propertyLen;
	airkiss_callbacks_t		cbs;
} WxcloudParams;


typedef struct _WxcloudDevInfo
{
	uint8_t			devId[DEV_ID_LEN];
	uint8_t			devType[DEV_TYPE_LEN];	
}WxcloudDevInfo;

/**
 * @brief
 *		Wxcloud Init
 * @param
 * 	 	NONE
 * @return  
 *	 	Wxcloud status SUCCESS or FAILED
 */
WxcloudStatus WxcloudInit(void);

/**
 * @brief
 *		Wxcloud Deinit
 * @param
 * 	 	NONE
 * @return  
 *	 	Wxcloud status SUCCESS or FAILED
 */
WxcloudStatus WxcloudDeinit(void);

/**
 * @brief
 *		Wxcloud register configuration
 * @param
 * 	 	params				wxcloud params
 *		wxcloudDevInfo		wxcloud device information
 * @return  
 *	 	Wxcloud status SUCCESS or FAILED
 */
WxcloudStatus WxcloudRegister(WxcloudParams *params, WxcloudDevInfo *wxcloudDevInfo);

/**
 * @brief
 *		Wxcloud Deregister 
 * @param
 * 	 	NONE
 * @return  
 *	 	Wxcloud status SUCCESS or FAILED
 */
WxcloudStatus WxcloudDeregister(void);

/**
 * @brief
 *		Wxcloud Run 
 * @param
 * 	 	NONE
 * @return  
 *	 	
 */
uint32_t WxcloudRun(void);

/**
 * @brief
 *		Wxcloud Get Wx Speech Unread List
 * @param
 * 	 	listData		get unread list data
 *		listLen			get list data length
 *		devInfo			device information
 * @return  
 *	 	Wxcloud status SUCCESS or FAILED
 */
WxcloudStatus WxCloudGetSpeechUnreadList(uint8_t **listData, int32_t *listLen, WxcloudDevInfo *devInfo);

/**
 * @brief
 *		Wxcloud Get Media Start(Wx Speech)
 * @param
 * 	 	mediaId		media id(Wx Speech)
 * @return  
 *	 	Wxcloud status SUCCESS or FAILED
 */
WxcloudStatus WxcloudGetMediaStart(uint8_t *mediaId);

/**
 * @brief
 *		Wxcloud Get Media (Wx Speech)
 * @param
 * 	 	amrData			get amr file data
 *		dataLen			get amr file data length
 *		errCode			error id
 * @return  
 *	 	Wxcloud status SUCCESS or FAILED
 */
WxcloudStatus WxcloudGetMedia(uint8_t **amrData, int32_t *dataLen, int32_t *errCode);

/**
 * @brief
 *		Wxcloud Get Media Stop(Wx Speech)
 * @param
 * 	 	NONE
 * @return  
 *	 	Wxcloud status SUCCESS or FAILED
 */
WxcloudStatus WxcloudGetMediaStop(void);

/**
 * @brief
 *		Wxcloud Send Media (Wx Speech)
 * @param
 * 	 	amrData			get amr file data
 *		dataLen			get amr file data length
 *		mediaId			media id
 * @return  
 *	 	Wxcloud status SUCCESS or FAILED
 */
WxcloudStatus WxcloudSendMedia(uint8_t *amrData, int32_t dataLen, uint8_t *mediaId);

/**
 * @brief
 *		Wxcloud Send Media Massage (Wx Speech)
 * @param
 *		mediaId			media id
 * 	 	flag			
 * @return  
 *	 	Wxcloud status SUCCESS or FAILED
 */
WxcloudStatus WxCloudSendMediaMsg(uint8_t *mediaId, uint8_t flag);

#endif /*__WXCLOUD_API_H__*/
