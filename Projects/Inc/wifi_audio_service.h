


#ifndef __WIFI_AUDIO_SERVICE_H__
#define __WIFI_AUDIO_SERVICE_H__

#include "type.h"
#include "audio_utility.h"


MessageHandle GetWifiAudioServiceMsgHandle(void);

ServiceState GetWifiAudioServiceState(void);

void WifiAudioServiceConnect(const char * url, uint16_t urlLen);

MemHandle * GetWifiAudioServiceBuf(void);

int32_t GetMediaType(void);

int32_t WifiAudioServiceCreate(MessageHandle parentMsgHandle);

void WifiAudioServiceInit(uint32_t thresholdLen);

int32_t WifiAudioServiceKill(void);

int32_t WifiAudioServiceStart(void);

int32_t WifiAudioServicePause(void);

int32_t WifiAudioServiceResume(void);

int32_t WifiAudioServiceStop(void);

#endif

