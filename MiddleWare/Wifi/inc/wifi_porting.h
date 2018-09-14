/**
 **************************************************************************************
 * @file    wifi_porting.h
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


#ifndef __WIFI_PORTING_H__
#define __WIFI_PORTING_H__

#include "type.h"
#include "sys_app.h"

struct  ap_ssid_pwd {
	uint8_t ap_ssid[40];
    uint8_t ap_ssid_len;
    uint8_t ap_password[40];
    uint8_t ap_password_len;
};

struct ap_record {
	uint8_t total_items;
    uint8_t current_item;
    struct  ap_ssid_pwd ssid_pwd[10];
};

extern struct AP_RECORD *pApJoiningRecord;

/**
 * @brief
 *		switch to STA mode
 * @param
 * 	 NONE
 * @return  
 * 	 NONE
 */
void Switch2STAmode(void);

/**
 * @brief
 *		switch to SCONFIG mode
 * @param
 * 	 NONE
 * @return  
 * 	 SUCCESS or FAILED
 */
int16_t Switch2SCONFIGmode(void);

/**
 * @brief
 *		Printf wifi scan result
 *
 * @param
 *		NONE
 *
 * @return
 *		NONE
 */
void PrintfScanWifiResult(void);

/**
 * @brief
 *		Scan WiFi
 *
 * @param
 *		NONE
 *
 * @return
 *		NONE
 */
void ScanWifi(void);

/**
 * @brief
 *		Leave WiFi
 *
 * @param
 *		NONE
 *
 * @return
 *		NONE
 */
void LeaveWifi(void);

/**
 * @brief
 *		Join WiFi
 *
 * @param
 *		ssidName		network name
 *		ssidPassword	network password
 *
 * @return
 *		NONE
 */
void JoinWifi(const uint8_t *ssidName, const uint8_t *ssidPassword);

/**
 * @brief
 *		Get the Network Connection Status
 *
 * @param
 *		NONE
 *
 * @return
 *		SUCCESS or FAILED
 */
int16_t GetNetifStatus(void);

/**
 * @brief
 *		Get Ip Address
 *
 * @param
 *		NONE
 *
 * @return
 *		Ip Address
 */
uint32_t GetIpAdress(void);

/**
 * @brief
 *		find wether the ap information is in record wifi list
 *
 * @param
 *		n			item
 *		ap_count	ap count
 *		ap_list		ap information
 *
 * @return
 *		YES or NO
 */
uint8_t FindApInWifiList(uint8_t n, uint32_t ap_count, struct ssv6xxx_ieee80211_bss *ap_list);


#endif /*__WIFI_PORTING_H__*/

