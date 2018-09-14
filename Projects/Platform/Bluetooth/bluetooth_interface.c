/**
 **************************************************************************************
 * @file    bluetooth_interface.c
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
#include "bt_config.h"
#include "gpio.h"
#include "uart.h"
#include "delay.h"
#include "bt_platform_interface.h"

#if (BT_RF_DEVICE == BTUartDeviceRTK8761)
#include "bt_rtk_driver.h"
#elif BT_RF_DEVICE == BTUartDeviceMTK662X
#include "bt_mtk_driver.h"
#endif



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

const uint8_t btDevAddr[6] = BT_ADDRESS;


/***************************************************************************************
 *
 * Internal varibles
 *
 */

extern PLATFORM_INTERFACE_BT_UART_T	pfiBtUart;
extern PLATFORM_INTERFACE_OS_T		pfiOS;
extern PLATFORM_INTERFACE_BT_DDB_T	pfiBtDdb;



/***************************************************************************************
 *
 * Internal functions
 *
 */

static void BtDevicePinConfig(void)
{
	ResetBuartMoudle();

	GpioBuartRxIoConfig(BT_UART_RX_PORT);
	GpioBuartTxIoConfig(BT_UART_TX_PORT);
	GpioBuartRtsIoConfig(BT_UART_RTS_PORT);
	GpioClk32kIoConfig(BT_UART_EXT_CLK_PORT);
}


bool BtPlatformInit(void)
{
	bool	ret = TRUE;

	SetBtPlatformInterface(&pfiBtUart, &pfiOS, &pfiBtDdb);

	return ret;
}


bool BtDeviceInit(void)
{
	bool	ret;

	BtDevicePinConfig();

	#if BT_RF_DEVICE == BTUartDeviceRTK8761
	{
		RtkConfigParam			params;

		memcpy(params.bdAddr, btDevAddr, 6);
		params.uartSettings.uartBaudrate	= PARAMS_UART_BAUDRATE;

		params.pcmSettings.scoInterface		= PARAMS_SCO_INTERFACE;
		params.pcmSettings.pcmFormat		= PARAMS_PCM_FORMAT;
		params.pcmSettings.pcmWidth			= PARAMS_PCM_WIDTH;

		params.radioPower.txPower			= PARAMS_TX_POWER;
		params.radioPower.txDac				= PARAMS_TX_DAC;

		params.enableFlowCtrl				= PARAMS_FLOW_CTRL;
		params.enableExtClk					= PARAMS_EXT_CLOCK;

		ret = BTDeviceInit_RTK8761(&params);
	}
	
	#elif BT_RF_DEVICE == BTUartDeviceMTK662X
	{
		MtkConfigParam			params;

		/*uart config*/
		params.uartSettings.baudrate		= PARAMS_UART_BAUDRATE;
		params.uartSettings.dataBits		= PARAMS_UART_DATA_BITS;
		params.uartSettings.stopBits		= PARAMS_UART_STOP_BITS;
		params.uartSettings.parityChk		= PARAMS_UART_PARITY_CHECK;
		params.uartSettings.flowCtrl		= PARAMS_UART_FLOW_CONTROL;

		/*pcm config*/
		params.pcmSettings.pcmMode			= PARAMS_PCM_MODE;
		params.pcmSettings.pcmClock			= PARAMS_PCM_CLOCK;
		params.pcmSettings.pcmInClkAbi		= PARAMS_PCM_CLK_ABI;
		params.pcmSettings.pcmSyncFormat	= PARAM_PCM_SYNC_FORMAT;
		params.pcmSettings.pcmEndianFormat	= PARAMS_PCM_ENDIAN;
		params.pcmSettings.pcmSignExt		= PARAMS_PCM_ENDIAN;

		/*local features*/
		params.localFeatures.enableAutoSniff		= PARAMS_AUTO_SNIFF;
		params.localFeatures.enableEV3				= PARAMS_EV3;
		params.localFeatures.enable3EV3				= PARAMS_3EV3;
		params.localFeatures.enableSimplePairing	= PARAMS_SIMPLE_PAIRING;

		/*tx power*/
		params.txPower		= PARAMS_TX_POWER;

		/*xo trim*/
		params.trimValue	= PARAMS_XO_TRIM;

		/* device address*/
		memcpy(params.bdAddr, btDevAddr, 6);

		ret = BTDeviceInit_MTK662x(&params);
	}
	#else
		#error "Select one bluetooth RF device";
	#endif

	return ret;
}
void BtStackRunNotify(void)
{
//	OSQueueMsgSend(MSG_NEED_BT_STACK_RUN, NULL, 0, MSGPRIO_LEVEL_HI, 0);
}


