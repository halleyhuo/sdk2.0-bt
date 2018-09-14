
#include "type.h"
#include "delay.h"
#include "uart.h"
#include "gpio.h"
#include "bt_platform_interface.h"
#include "bt_config.h"
#include "chip_info.h"




#define BUART_RX_TX_FIFO_ADDR			(VMEM_ADDR + 57 * 1024)
#define BUART_RX_FIFO_SIZE				(6 * 1024)
#define BUART_TX_FIFO_SIZE				(1 * 1024)

static bool BtPowerOn(void);
static bool BtUartInit(void);
static bool BtUartDeInit(void);
static bool BtUartChangeBaudRate(uint32_t baudrate);
static uint32_t BtUartSend(uint8_t * data, uint32_t dataLen, uint32_t timeout);
static uint32_t BtUartRecv(uint8_t * data, uint32_t dataLen, uint32_t timeout);
static void BtUartWaitMs(uint32_t ms);


PLATFORM_INTERFACE_BT_UART_T	pfiBtUart = {
		BtPowerOn,
		BtUartInit,
		BtUartDeInit,
		BtUartChangeBaudRate,
		BtUartSend,
		BtUartRecv,
		BtUartWaitMs,
};


static void BtLDOEn(bool enable)
{
	uint8_t	gpioPort;

	/* LDO pin*/
	switch(BT_LDOEN_GPIO_PORT)
	{
		case 1:
			gpioPort = GPIO_A_IN;
			break;
		case 2:
			gpioPort = GPIO_B_IN;
			break;
		case 3:
			gpioPort = GPIO_C_IN;
			break;
		default:
			return;
	}

	if(enable)
	{
		GpioClrRegOneBit(gpioPort + 2, ((uint32_t)1 << BT_LDOEN_GPIO_PIN));
		GpioSetRegOneBit(gpioPort + 3, ((uint32_t)1 << BT_LDOEN_GPIO_PIN));
		GpioSetRegOneBit(gpioPort + 1, ((uint32_t)1 << BT_LDOEN_GPIO_PIN));
	}
	else
	{
		GpioClrRegOneBit(gpioPort + 2, ((uint32_t)1 << BT_LDOEN_GPIO_PIN));
		GpioSetRegOneBit(gpioPort + 3, ((uint32_t)1 << BT_LDOEN_GPIO_PIN));
		GpioClrRegOneBit(gpioPort + 1, ((uint32_t)1 << BT_LDOEN_GPIO_PIN));
	}

#if BT_RF_DEVICE == BTUartDeviceMTK662X
/* MTK RF 6622 should operate RESET pin at the same time*/

	/*RESET pin*/
	switch(BT_REST_GPIO_PORT)
	{
		case 1:
			gpioPort = GPIO_A_IN;
			break;
		case 2:
			gpioPort = GPIO_B_IN;
			break;
		case 3:
			gpioPort = GPIO_C_IN;
			break;
		default:
			return;
	}

	if(enable)
	{
		GpioClrRegOneBit(gpioPort + 2, ((uint32_t)1 << BT_REST_GPIO_PIN));
		GpioSetRegOneBit(gpioPort + 3, ((uint32_t)1 << BT_REST_GPIO_PIN));
		GpioSetRegOneBit(gpioPort + 1, ((uint32_t)1 << BT_REST_GPIO_PIN));
	}
	else
	{
		GpioClrRegOneBit(gpioPort + 2, ((uint32_t)1 << BT_REST_GPIO_PIN));
		GpioSetRegOneBit(gpioPort + 3, ((uint32_t)1 << BT_REST_GPIO_PIN));
		GpioClrRegOneBit(gpioPort + 1, ((uint32_t)1 << BT_REST_GPIO_PIN));
	}
#endif
}


static bool BtPowerOn(void)
{
	BtLDOEn(FALSE);
	WaitMs(200);
	BtLDOEn(TRUE);
	WaitMs(200);
	return TRUE;
}

static bool BtUartInit(void)
{

	// 115200, 8, 0, 1

	BuartInit(115200, 8, 0, 1);

	BuartIOctl(UART_IOCTL_RXINT_SET,0);

	BuartIOctl(UART_IOCTL_TXINT_SET,0);

#if FLOW_CTRL == ENABLE
	BuartIOctl(BUART_IOCTL_RXRTS_FLOWCTL_SET, 2);
#endif

	BuartExFifoInit(BUART_RX_TX_FIFO_ADDR - PMEM_ADDR, BUART_RX_FIFO_SIZE, BUART_TX_FIFO_SIZE, 3);

	return TRUE;
}

static bool BtUartDeInit(void)
{

	return TRUE;
}

static bool BtUartChangeBaudRate(uint32_t baudrate)
{
	//change uart baud rate
	BuartIOctl(UART_IOCTL_DISENRX_SET,0); 

	BuartIOctl(UART_IOCTL_BAUDRATE_SET,baudrate);//different bt module may have the differnent bautrate!!

	BuartIOctl(UART_IOCTL_DISENRX_SET,1);

	return TRUE;
}

static uint32_t BtUartSend(uint8_t * data, uint32_t dataLen, uint32_t timeout)
{
	return BuartSend(data, dataLen);
}

static uint32_t BtUartRecv(uint8_t * data, uint32_t dataLen, uint32_t timeout)
{
	uint32_t	len;
	len = BuartRecv2(data, dataLen, timeout);
	return len;
//	return BuartRecv(data, dataLen, timeout);
}

static void BtUartWaitMs(uint32_t ms)
{
	WaitMs(ms);
}

