/**
 **************************************************************************************
 * @file    wxcloud_api.c
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


#include "type.h"
#include "wifi_api.h"
#include "airkiss_cloudapi.h"
#include "wxcloud_api.h"
#include "rtos.h"

/***************************************************************************************
 *
 * External defines
 *
 */


/***************************************************************************************
 *
 * Internal defines
 *
 */
#define	WX_FILE_HOST_NAME			"file.api.weixin.qq.com"

#define WX_FILE_HOST_PORT			80

#define MV_IOT_HOST_NAME			"www.yuanyiwei.top"

#define MV_IOT_HOST_PORT			80

#define WX_MAX_DATA_BUFFER_SIZE		1024
#define WX_SPEECH_LIST_BUFFER_SIZE	2048

#define WX_MAX_RECEIVE_TIMEOUT		3000


typedef struct _WxSocketContext
{
	IP_ADDR			ipAddr;
	int32_t			wxSocket;
	uint8_t			*wxDataBuff;
	uint8_t			*wxMediaId;

	/* for receive */
	bool			headerFlag;
	uint32_t		wxMediaTotalRecvLen;
	uint32_t		wxMediaReceivedLen;

	/* for send */
	uint32_t		wxMeidaTotalSendLen;
	uint32_t		wxMediaRemainLen;
}WxSocketContext;

/***************************************************************************************
 *
 * Internal varibles
 *
 */

const char mvIOTGetRequest[] = "GET /device_get_token.php HTTP/1.1\r\nUser-Agent: mv_net\r\nHost:www.yuanyiwei.top\r\nConnction: Keep-Alive\r\n\r\n";

const char mvIOTGetOtaUpgrade[] = "GET /device_mva/list.txt HTTP/1.1\r\nUser-Agent: mv_net\r\nHost:www.yuanyiwei.top\r\nConnction: Keep-Alive\r\n\r\n";
const char mvIOTGetOtaUpgradeS[] = "GET /device_mva/%s HTTP/1.1\r\nUser-Agent: mv_net\r\nHost:www.yuanyiwei.top\r\nConnction: Keep-Alive\r\n\r\n";


static uint8_t 					wxAccessToken[512];
static WxSocketContext			wxSocketFileHost;
//static WxSocketContext			wxSocketMVHost;
WxSocketContext					wxSocketMVHost;

/***************************************************************************************
 *
 * Internal functions
 *
 */
static bool IsIPaddressVaild(IP_ADDR* ipAddr)
{
	if(strlen(ipAddr->ipAddrStr) == 0)
		return FALSE;

	return TRUE;
}


static WxcloudStatus WxcloudConnectHost(uint8_t * hostName, IP_ADDR *ipAddr, int32_t port, int32_t * socket)
{
	WifiStatus			wifiStatus;


	if(IsIPaddressVaild(ipAddr))
	{
		wifiStatus = WifiConnectIP(ipAddr, socket, port);

		if(wifiStatus != WIFI_STATUS_SUCCESS)
			return WXCLOUD_STATUS_FAILED;

		return WXCLOUD_STATUS_SUCCESS;
	}

	
	if(ResolveIP(hostName, ipAddr) == WIFI_STATUS_SUCCESS)
	{
		wifiStatus = WifiConnectIP(ipAddr, socket, port);

		if(wifiStatus != WIFI_STATUS_SUCCESS)
			return WXCLOUD_STATUS_FAILED;
	}
	else
	{
		return WXCLOUD_STATUS_FAILED;
	}


	return WXCLOUD_STATUS_SUCCESS;	
}


static __inline WxcloudStatus WxcloudConnectFileServer(void)
{
	return WxcloudConnectHost(WX_FILE_HOST_NAME, &wxSocketFileHost.ipAddr, WX_FILE_HOST_PORT, &wxSocketFileHost.wxSocket);
}

static __inline WxcloudStatus WxcloudConnectMVIot(void)
{
	return WxcloudConnectHost(MV_IOT_HOST_NAME, &wxSocketMVHost.ipAddr, MV_IOT_HOST_PORT, &wxSocketMVHost.wxSocket);
}

extern uint32_t gSysTick;
static __inline uint32_t RandomNum(void)
{
	return gSysTick;
}


static WxcloudStatus WxHttpHeaderParse(uint8_t * buff, uint16_t buffLen, uint8_t * itemString, uint8_t * itemContent)
{
	uint32_t		len = 0;
	uint32_t		itemStringLen;


	itemStringLen = strlen(itemString);

	while(len < buffLen)
	{
		if(memcmp(&buff[len],itemString,strlen(itemString)) == 0)
		{
			uint16_t i;

			for(len += itemStringLen, i = 0; (buff[len] != '\r' && buff[len] != '\n'); len++, i++)
			{
				itemContent[i] = buff[len];
			}
			return WXCLOUD_STATUS_SUCCESS;
		}
	}

	return WXCLOUD_STATUS_FAILED;	
}

static int32_t WxHttpGetBodyPos(uint8_t * buff, uint16_t buffLen)
{
	int32_t			position = 0;

	if(buff == NULL || buffLen == 0)
		return -1;

	while(position < buffLen - 3)
	{
		if(buff[position] == '\r' && buff[position+1] == '\n' && buff[position+2] == '\r' && buff[position+3] == '\n')
			break;

		position++;
	}

	if(position >= buffLen - 3)
	{
		return -1;
	}

	return position + 4;
}

static int32_t WxGetUnreadListHead(uint8_t * buff, uint16_t buffLen)
{
	int32_t	position = 0;

	if(buff == NULL)
		return -1;

	while(position < buffLen)
	{
		if(buff[position] == '(')
			break;

		position++;
	}

	if(position >= buffLen - 1)
		return -1;

	return position;
}

/*
 * wx speech error feedback
 */
static WxcloudStatus WxParseErrcode(uint8_t *buff, uint32_t bufLen, int32_t* errCode)
{
	uint32_t len = 0;

	*errCode = 0;
	while(len < bufLen)
	{
		if(buff[len++] == '"')
		{
			if(memcmp(&buff[len],"errcode",7) == 0)
			{
				len += 9;
				if(buff[len] == '-')
				{
					*errCode = -1;
				}
				else
				{
					sscanf(&buff[len],"%d",errCode);
				}
				return WXCLOUD_STATUS_SUCCESS;
			}
		}
	}

	return WXCLOUD_STATUS_FAILED;
}

static WxcloudStatus WxcloudGetAccessToken(void)
{
	WxcloudStatus			wxStatus = WXCLOUD_STATUS_FAILED;
	int32_t					recvLen;
	uint16_t				receivedLen;
	uint8_t					*posStart, *posEnd;
	uint8_t					posLen;
	//uint8_t					wxDataBuff[1024];

	printf("WxcloudGetAccessToken.\n");

	wxStatus = WxcloudConnectMVIot();
	if(wxStatus != WXCLOUD_STATUS_SUCCESS)
		return wxStatus;

	wxSocketMVHost.wxDataBuff = (uint8_t *)MALLOC(WX_MAX_DATA_BUFFER_SIZE);
	if(!wxSocketMVHost.wxDataBuff)
	{
		return WXCLOUD_STATUS_FAILED;
	}
	memset(wxSocketMVHost.wxDataBuff, 0, WX_MAX_DATA_BUFFER_SIZE);

	strcpy(wxSocketMVHost.wxDataBuff, mvIOTGetRequest);

	WifiSend(wxSocketMVHost.wxSocket, wxSocketMVHost.wxDataBuff, strlen(wxSocketMVHost.wxDataBuff), 0);

	memset(wxSocketMVHost.wxDataBuff, 0, WX_MAX_DATA_BUFFER_SIZE);

	/* Receive AccessToken response*/
	receivedLen = 0;

	/*
	 * eg:(f927Bic3j2Eq8zndgZDHn7FcGshJ5MOSIiLo_M4s xmkp4vUZSjlcsnezH-slV4G-ztOfXoPs VWmJ0B5LC-8fzSYe9spFfqCrV0QUnmwD240GPRdADAXGI)
	 */
	while(1)
	{
		recvLen = WifiRecv(wxSocketMVHost.wxSocket, wxSocketMVHost.wxDataBuff + receivedLen, WX_MAX_DATA_BUFFER_SIZE, WX_MAX_RECEIVE_TIMEOUT);
		if(recvLen < 0)
		{
			wxStatus = WXCLOUD_STATUS_FAILED;
			goto END;
		}
		if(recvLen == 0)
		{
			continue;
		}
		printf("%s\n", wxSocketMVHost.wxDataBuff);
		posEnd = strstr(wxSocketMVHost.wxDataBuff, ")");
		if(posEnd != NULL)
		{
			posStart = strstr(wxSocketMVHost.wxDataBuff, "(");
			memset(wxAccessToken, 0 ,sizeof(wxAccessToken));
			memcpy(wxAccessToken, (posStart + 1), (*posEnd - *posStart - 1));
			wxStatus = WXCLOUD_STATUS_SUCCESS;
			printf("AccessToken: %s\n", wxAccessToken);
			break;
		}
	}
END:
	SocketClose(wxSocketMVHost.wxSocket);

	if(wxSocketMVHost.wxDataBuff)
		FREE(wxSocketMVHost.wxDataBuff);
	
	return wxStatus;
}


/***************************************************************************************
 *
 * APIs
 *
 */

WxcloudStatus WxcloudInit(void)
{

}

WxcloudStatus WxcloudDeinit(void)
{
	

}


WxcloudStatus WxcloudRegister(WxcloudParams *params, WxcloudDevInfo *wxcloudDevInfo)
{
	int32_t					ret;
	uint8_t					*deviceId;
	uint8_t					*deviceType;
	airkiss_callbacks_t		cbs;

	ret = airkiss_cloud_init(params->devLicense, 
							params->devLicenseLen, 
							params->taskMutex, 
							params->mallocMutex,
							params->loopMutex,
							params->wxBuffer,
							params->wxBufferLen,
							params->property,
							params->propertyLen);

	if(ret != 0)
	{
		return WXCLOUD_STATUS_FAILED;
	}

	deviceId = airkiss_get_deviceid();
	memcpy(wxcloudDevInfo->devId, deviceId, strlen(deviceId));

	deviceType = deviceId - 33;
	memcpy(wxcloudDevInfo->devType, deviceType, strlen(deviceType));

	cbs.m_eventcb			= params->cbs.m_eventcb;
	cbs.m_notifycb			= params->cbs.m_notifycb;
	cbs.m_respcb			= params->cbs.m_respcb;
	cbs.m_subdevnotifycb	= params->cbs.m_subdevnotifycb;


	airkiss_regist_callbacks(&cbs);

	return WXCLOUD_STATUS_SUCCESS;
}

WxcloudStatus WxcloudDeregister(void)
{
	airkiss_cloud_release();

	return WXCLOUD_STATUS_SUCCESS;
}

uint32_t WxcloudRun(void)
{
	return airkiss_cloud_loop();
}

/*
 * get wx speech unread list
 * 
 */
WxcloudStatus WxCloudGetSpeechUnreadList(uint8_t **listData, int32_t *listLen, WxcloudDevInfo *devInfo)
{
	WxcloudStatus		wxStatus = WXCLOUD_STATUS_SUCCESS;

	int32_t				recvLen = 0;
	uint32_t			receivedLen;
	uint32_t			posStart;
	uint32_t			*posEnd;
	uint32_t			posLen;
	int32_t				sentLen;

	if(*listData == NULL || listData == NULL)
		return WXCLOUD_STATUS_PARAMS_ERR;

	wxStatus = WxcloudConnectMVIot();
	if(wxStatus != WXCLOUD_STATUS_SUCCESS)
		return wxStatus;

	wxSocketFileHost.wxDataBuff = (uint8_t *)MALLOC(WX_SPEECH_LIST_BUFFER_SIZE);
	if(wxSocketFileHost.wxDataBuff == NULL)
		return WXCLOUD_STATUS_FAILED;

	memset(wxSocketFileHost.wxDataBuff, 0, WX_SPEECH_LIST_BUFFER_SIZE);

	sprintf((char*)wxSocketFileHost.wxDataBuff,"GET /device_get_mediaid.php?device_id=%s HTTP/1.1\r\nHost: www.yuanyiwei.top\r\nConnection: keep-alive\r\n\r\n",devInfo->devId);
	sentLen = WifiSend(wxSocketFileHost.wxSocket, wxSocketFileHost.wxDataBuff ,strlen(wxSocketFileHost.wxDataBuff), 0);

	if(sentLen < 0)
	{
		wxStatus = WXCLOUD_STATUS_FAILED;
		goto END;
	}

	memset(wxSocketFileHost.wxDataBuff, 0, WX_SPEECH_LIST_BUFFER_SIZE);

	recvLen = 0;
	receivedLen = 0;
	while(1)
	{
		recvLen = WifiRecv(wxSocketFileHost.wxSocket, &wxSocketFileHost.wxDataBuff[receivedLen], WX_MAX_DATA_BUFFER_SIZE, WX_MAX_RECEIVE_TIMEOUT);
		
		if(recvLen < 0)
		{
			wxStatus = WXCLOUD_STATUS_FAILED;
			goto END;
		}
		else if(recvLen == 0)
		{
			continue;
		}
		else
		{
			receivedLen += recvLen;
		}

		//printf("%s\n", wxSocketMVHost.wxDataBuff);
		posEnd = strstr(wxSocketMVHost.wxDataBuff, ")");
		if(posEnd != NULL)
		{
			posStart = WxGetUnreadListHead(wxSocketMVHost.wxDataBuff, receivedLen);
			if(posStart>0)
			{
				*listData = &wxSocketFileHost.wxDataBuff[posStart];
				*listLen = posEnd - posStart;
				wxStatus = WXCLOUD_STATUS_SUCCESS;
				
				/*
				 *eg:(mediaid;_XlXnU6TE5dQE7bd-DFnLoQ_9hCMpXczA8yJxSE7hvqdoYMetToajukSUBe1nsdX;
				 52HyZb2PYAje0g-2UJmMilxioOHSEUXY7S-t6IJY-0SN2Rd-wa8lxeqc8n8Wi6M-;
				 IPWZaakmWeUKY4ChfNVIhonabPgCPTaNW9nkZ_W0C-IAUKjGwNWHhlZm4AX34YkN;
				 96VOq_0ZQF74jxXrBh-kXkaWaDqPXRsIkDJ-XE3xIMhdZjlQgGLdvVkDoXUVgUwF)
				 */
				printf("%s\n", &wxSocketFileHost.wxDataBuff[posStart]);
			}
			break;
		}
	}
END:
	SocketClose(wxSocketMVHost.wxSocket);

	if(wxSocketMVHost.wxDataBuff)
		FREE(wxSocketMVHost.wxDataBuff);
	
	return wxStatus;
}

/*
 * get weixin speech data
 */
WxcloudStatus WxcloudGetMediaStart(uint8_t *mediaId)
{
	int32_t			sentLen;

	if(mediaId == NULL)
		return WXCLOUD_STATUS_PARAMS_ERR;

	WxcloudGetAccessToken();

	if(WxcloudConnectFileServer() != WXCLOUD_STATUS_SUCCESS)
	{
		return WXCLOUD_STATUS_FAILED;
	}

	wxSocketFileHost.wxDataBuff = (uint8_t *)MALLOC(WX_MAX_DATA_BUFFER_SIZE);
	if(wxSocketFileHost.wxDataBuff == NULL)
		return WXCLOUD_STATUS_FAILED;

	memset(wxSocketFileHost.wxDataBuff, 0, WX_MAX_DATA_BUFFER_SIZE);

	sprintf((char*)wxSocketFileHost.wxDataBuff,"GET /cgi-bin/media/get?access_token=%s&media_id=%s HTTP/1.1\r\nConnection: Keep-Alive\r\nAccept-Encoding:gzip, deflate\r\nAccept-Language: zh-CN,en,*\r\nUser-Agent: Mozilla/5.0\r\nHost: file.api.weixin.qq.com\r\n\r\n",wxAccessToken,mediaId);

	sentLen = WifiSend(wxSocketFileHost.wxSocket, wxSocketFileHost.wxDataBuff ,strlen(wxSocketFileHost.wxDataBuff), 0);

	if(sentLen > 0)
	{
		wxSocketFileHost.headerFlag = FALSE;
		return  WXCLOUD_STATUS_SUCCESS;
	}

	return WXCLOUD_STATUS_FAILED;

}

WxcloudStatus WxcloudGetMedia(uint8_t **amrData, int32_t *dataLen, int32_t *errCode)
{
	WxcloudStatus		wxStatus;
	int32_t				headLen;
	uint8_t				mediaType[20] = {0};
	uint8_t				mediaLength[20] = {0};
	int32_t				bodyPos;

	*errCode = 0;
	if(*amrData == NULL || amrData == NULL)
		return WXCLOUD_STATUS_PARAMS_ERR;

	if(wxSocketFileHost.headerFlag == FALSE)
	{

		headLen = WifiRecv(wxSocketFileHost.wxSocket, wxSocketFileHost.wxDataBuff, WX_MAX_DATA_BUFFER_SIZE, WX_MAX_RECEIVE_TIMEOUT);

		if(headLen < 0)
		{
			return WXCLOUD_STATUS_FAILED;
		}

		wxStatus = WxHttpHeaderParse(wxSocketFileHost.wxDataBuff, headLen, "Content-Type: ", mediaType);

		if(wxStatus != WXCLOUD_STATUS_SUCCESS)
			return WXCLOUD_STATUS_FAILED;

		if(strstr(mediaType, "audio/amr") != NULL)
		{
			WxParseErrcode(wxSocketFileHost.wxDataBuff, headLen, errCode);

			return WXCLOUD_STATUS_FAILED;
		}

		wxStatus = WxHttpHeaderParse(wxSocketFileHost.wxDataBuff, headLen, "Content-Length: ", mediaLength);
		sscanf(mediaLength,"%d",&wxSocketFileHost.wxMediaTotalRecvLen);
		
		if(wxStatus != WXCLOUD_STATUS_SUCCESS)
			return WXCLOUD_STATUS_FAILED;

		bodyPos = WxHttpGetBodyPos(wxSocketFileHost.wxDataBuff, headLen);

		if(bodyPos <= 0)
		{
			return WXCLOUD_STATUS_FAILED;
		}

		*amrData = &wxSocketFileHost.wxDataBuff[bodyPos];
		*dataLen = headLen - bodyPos;
		wxSocketFileHost.wxMediaReceivedLen = headLen - bodyPos;
		*errCode = 0;
		wxSocketFileHost.headerFlag = TRUE;

		return WXCLOUD_STATUS_SUCCESS;
	}
	else
	{
		if(wxSocketFileHost.wxMediaReceivedLen < wxSocketFileHost.wxMediaTotalRecvLen)
		{
			int32_t			recvLen;

			recvLen = WifiRecv(wxSocketFileHost.wxSocket, wxSocketFileHost.wxDataBuff, WX_MAX_DATA_BUFFER_SIZE, WX_MAX_RECEIVE_TIMEOUT);

			if(recvLen <= 0)
				return WXCLOUD_STATUS_NO_MORE_DATA;

			*amrData = wxSocketFileHost.wxDataBuff;
			*dataLen = recvLen;
			wxSocketFileHost.wxMediaReceivedLen += recvLen;
			*errCode = 0;
			return WXCLOUD_STATUS_SUCCESS;
		}
		else
		{
			return WXCLOUD_STATUS_NO_MORE_DATA;
		}
	}
}

WxcloudStatus WxcloudGetMediaStop(void)
{
	SocketClose(wxSocketFileHost.wxSocket);

	if(wxSocketFileHost.wxDataBuff)
		FREE(wxSocketFileHost.wxDataBuff);

	return WXCLOUD_STATUS_SUCCESS;
}


WxcloudStatus WxcloudSendMedia(uint8_t *amrData, int32_t dataLen, uint8_t *mediaId)
{
	WxcloudStatus	wxStatus = WXCLOUD_STATUS_FAILED;
	int32_t			sentLen;
	uint16_t		lenDataToSend;
	int16_t			recvLen;
	char			*posMediaIdBegin;
	char			*posMediaIdEnd;
	uint32_t		rand;

	
	/* Parameters check */
	if(amrData == NULL || dataLen == 0)
		return WXCLOUD_STATUS_PARAMS_ERR;

	WxcloudGetAccessToken();

	wxSocketFileHost.wxMeidaTotalSendLen = dataLen;
	wxSocketFileHost.wxMediaRemainLen = dataLen;

	rand = RandomNum();

	/* Connect Weixin file server and get a socket */
	if(WxcloudConnectFileServer() != WXCLOUD_STATUS_SUCCESS)
	{
		return WXCLOUD_STATUS_FAILED;
	}

	wxSocketFileHost.wxDataBuff = (uint8_t *)MALLOC(WX_MAX_DATA_BUFFER_SIZE);
	if(wxSocketFileHost.wxDataBuff == NULL)
		return WXCLOUD_STATUS_FAILED;

	memset(wxSocketFileHost.wxDataBuff, 0, WX_MAX_DATA_BUFFER_SIZE);
	
	/* Sent POST request */
	sprintf((char*)wxSocketFileHost.wxDataBuff,"POST /cgi-bin/media/upload?access_token=%s&type=voice HTTP/1.1\r\nHost: file.api.weixin.qq.com\r\nContent-Length: %d\r\nContent-Type: multipart/form-data; boundary=------------------------7e21b4c3%08x\r\n\r\n--------------------------7e21b4c3%08x\r\nContent-Disposition: form-data; name=\"media\"; filename=\"E:\\WWW\\1.amr\"\r\nContent-Type: application/octet-stream\r\n\r\n",wxAccessToken,157+dataLen+48,rand,rand);
	sentLen = WifiSend(wxSocketFileHost.wxSocket, wxSocketFileHost.wxDataBuff, strlen(wxSocketFileHost.wxDataBuff), 0);
	if(sentLen < 0)
	{
		goto END;
	}

	/* Send amr data */
	while(wxSocketFileHost.wxMediaRemainLen > 0)
	{
		lenDataToSend = wxSocketFileHost.wxMediaRemainLen >  WX_MAX_DATA_BUFFER_SIZE ? WX_MAX_DATA_BUFFER_SIZE : wxSocketFileHost.wxMediaRemainLen;

		memcpy(wxSocketFileHost.wxDataBuff, amrData + (wxSocketFileHost.wxMeidaTotalSendLen - wxSocketFileHost.wxMediaRemainLen), lenDataToSend);

		sentLen = WifiSend(wxSocketFileHost.wxSocket, wxSocketFileHost.wxDataBuff, lenDataToSend, 0);

		if(sentLen != lenDataToSend)
		{
			goto END;
		}

		wxSocketFileHost.wxMediaRemainLen -= sentLen;
	}

	/* Send END flag */
	sprintf((char*)wxSocketFileHost.wxDataBuff, "--------------------------7e21b4c3%08x--\r\n\r\n\r\n\r\n", rand);
	sentLen = WifiSend(wxSocketFileHost.wxSocket, wxSocketFileHost.wxDataBuff, strlen(wxSocketFileHost.wxDataBuff), 0);

	/* Get return strings*/
	memset(wxSocketFileHost.wxDataBuff, 0, WX_MAX_DATA_BUFFER_SIZE);
	recvLen = WifiRecv(wxSocketFileHost.wxSocket, wxSocketFileHost.wxDataBuff, WX_MAX_DATA_BUFFER_SIZE, WX_MAX_RECEIVE_TIMEOUT);
	if(recvLen < 0)
	{
		goto END;
	}
	/* Parse string to find meidaId */
	posMediaIdBegin = strstr(wxSocketFileHost.wxDataBuff, "media_id");
	if(posMediaIdBegin == NULL)
	{
		wxStatus = WXCLOUD_STATUS_NOT_FOUND;
		goto END;
	}

	/* Skip "media_id":"xxxxxx"*/
	posMediaIdBegin += 11;
	posMediaIdEnd = strstr(posMediaIdBegin, "\"");
	strncpy(mediaId, posMediaIdBegin, posMediaIdBegin - posMediaIdEnd);

	wxStatus = WXCLOUD_STATUS_SUCCESS;

END:
	if(wxSocketFileHost.wxDataBuff)
	{
		FREE(wxSocketFileHost.wxDataBuff);
	}

	SocketClose(wxSocketFileHost.wxSocket);

	return wxStatus;
}

WxcloudStatus WxCloudSendMediaMsg(uint8_t *mediaId, uint8_t flag)
{
	wxSocketFileHost.wxDataBuff = (uint8_t *)MALLOC(WX_MAX_DATA_BUFFER_SIZE);
	if(wxSocketFileHost.wxDataBuff == NULL)
		return WXCLOUD_STATUS_FAILED;

	memset(wxSocketFileHost.wxDataBuff ,0,1024);
    sprintf((char*)wxSocketFileHost.wxDataBuff,"{\"msg_type\":\"notify\",\"services\":{\"operation_status\":{\"status\":1},\"air_conditioner\":{\"tempe_indoor\":26,\"tempe_outdoor\":31,\"tempe_target\":26,\"fan_speed\":50}},\"data\":\"sendamr#%s#%d\"}",mediaId,flag);
    airkiss_cloud_sendmessage(1, (uint8_t*)wxSocketFileHost.wxDataBuff,strlen(wxSocketFileHost.wxDataBuff));

	if(wxSocketFileHost.wxDataBuff)
	{
		FREE(wxSocketFileHost.wxDataBuff);
	}

	return WXCLOUD_STATUS_SUCCESS;
}

WxcloudStatus WxcloudGetFirmwareVersion(uint8_t *Data)
{
	WxcloudStatus			wxStatus = WXCLOUD_STATUS_FAILED;
	int32_t					recvLen;
	uint16_t				receivedLen;
	uint8_t					*posStart, *posEnd;
	uint8_t					posLen;

	printf("WxcloudGetFirmwareVersion.\n");

	wxStatus = WxcloudConnectMVIot();
	if(wxStatus != WXCLOUD_STATUS_SUCCESS)
		return wxStatus;

	wxSocketMVHost.wxDataBuff = (uint8_t *)MALLOC(WX_MAX_DATA_BUFFER_SIZE);
	if(!wxSocketMVHost.wxDataBuff)
	{
		return WXCLOUD_STATUS_FAILED;
	}
	memset(wxSocketMVHost.wxDataBuff, 0, WX_MAX_DATA_BUFFER_SIZE);

	strcpy(wxSocketMVHost.wxDataBuff, mvIOTGetOtaUpgrade);

	WifiSend(wxSocketMVHost.wxSocket, wxSocketMVHost.wxDataBuff, strlen(wxSocketMVHost.wxDataBuff), 0);

	memset(wxSocketMVHost.wxDataBuff, 0, WX_MAX_DATA_BUFFER_SIZE);

	/* Receive AccessToken response*/
	receivedLen = 0;

	/*
	 * eg:(f927Bic3j2Eq8zndgZDHn7FcGshJ5MOSIiLo_M4s xmkp4vUZSjlcsnezH-slV4G-ztOfXoPs VWmJ0B5LC-8fzSYe9spFfqCrV0QUnmwD240GPRdADAXGI)
	 */
	while(1)
	{
		recvLen = WifiRecv(wxSocketMVHost.wxSocket, wxSocketMVHost.wxDataBuff + receivedLen, WX_MAX_DATA_BUFFER_SIZE, WX_MAX_RECEIVE_TIMEOUT);
		if(recvLen <= 0)
		{
			wxStatus = WXCLOUD_STATUS_FAILED;
			printf("Get Firmware Version Fail\n");
			goto END;
		}
		
		printf("%s\n", wxSocketMVHost.wxDataBuff);
		
		posEnd = strstr(wxSocketMVHost.wxDataBuff, ")");
		if(posEnd != NULL)
		{
			posStart = strstr(wxSocketMVHost.wxDataBuff, "(");
			memset(wxAccessToken, 0 ,sizeof(wxAccessToken));
			memcpy(wxAccessToken, (posStart + 1), (posEnd - posStart - 1));
			wxStatus = WXCLOUD_STATUS_SUCCESS;
			printf("AccessToken: %s\n", wxAccessToken);
			break;
		}
	}
END:
	SocketClose(wxSocketMVHost.wxSocket);

	if(wxSocketMVHost.wxDataBuff)
		FREE(wxSocketMVHost.wxDataBuff);
	
	return wxStatus;
}






