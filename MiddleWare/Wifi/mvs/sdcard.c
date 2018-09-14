//#include "drivers.h"
//#include "app.h"
//#include "api.h"

//#include <drv_conf.h>
//#include <osdep_service.h>
//#include <drv_types.h>
//#include <rtw_byteorder.h>

//#include <hal_intf.h>
#include "rtos.h"
#include "sdcard.h"
#include "delay.h"
#include "irqs.h"
#include "app_config.h"
#include "chip_info.h"
#define  ENOMEM       12  /* Out of memory */
#define NULL 							0
#define SDIO_STATIC_FLAGS               ((uint32_t)0x000005FF)
#define SDIO_CMD0TIMEOUT                ((uint32_t)0x00002710)
#define SDIO_FIFO_Address               ((uint32_t)0x40012C80)

/* Mask for errors Card Status R1 (OCR Register) */
#define SD_OCR_ADDR_OUT_OF_RANGE        ((uint32_t)0x80000000)
#define SD_OCR_ADDR_MISALIGNED          ((uint32_t)0x40000000)
#define SD_OCR_BLOCK_LEN_ERR            ((uint32_t)0x20000000)
#define SD_OCR_ERASE_SEQ_ERR            ((uint32_t)0x10000000)
#define SD_OCR_BAD_ERASE_PARAM          ((uint32_t)0x08000000)
#define SD_OCR_WRITE_PROT_VIOLATION     ((uint32_t)0x04000000)
#define SD_OCR_LOCK_UNLOCK_FAILED       ((uint32_t)0x01000000)
#define SD_OCR_COM_CRC_FAILED           ((uint32_t)0x00800000)
#define SD_OCR_ILLEGAL_CMD              ((uint32_t)0x00400000)
#define SD_OCR_CARD_ECC_FAILED          ((uint32_t)0x00200000)
#define SD_OCR_CC_ERROR                 ((uint32_t)0x00100000)
#define SD_OCR_GENERAL_UNKNOWN_ERROR    ((uint32_t)0x00080000)
#define SD_OCR_STREAM_READ_UNDERRUN     ((uint32_t)0x00040000)
#define SD_OCR_STREAM_WRITE_OVERRUN     ((uint32_t)0x00020000)
#define SD_OCR_CID_CSD_OVERWRIETE       ((uint32_t)0x00010000)
#define SD_OCR_WP_ERASE_SKIP            ((uint32_t)0x00008000)
#define SD_OCR_CARD_ECC_DISABLED        ((uint32_t)0x00004000)
#define SD_OCR_ERASE_RESET              ((uint32_t)0x00002000)
#define SD_OCR_AKE_SEQ_ERROR            ((uint32_t)0x00000008)
#define SD_OCR_ERRORBITS                ((uint32_t)0xFDFFE008)

/* Masks for R6 Response */
#define SD_R6_GENERAL_UNKNOWN_ERROR     ((uint32_t)0x00002000)
#define SD_R6_ILLEGAL_CMD               ((uint32_t)0x00004000)
#define SD_R6_COM_CRC_FAILED            ((uint32_t)0x00008000)

#define SD_VOLTAGE_WINDOW_SD            ((uint32_t)0x80100000)
#define SD_HIGH_CAPACITY                ((uint32_t)0x40000000)
#define SD_STD_CAPACITY                 ((uint32_t)0x00000000)
#define SD_CHECK_PATTERN                ((uint32_t)0x000001AA)

#define SD_MAX_VOLT_TRIAL               ((uint32_t)0x0000FFFF)
#define SD_ALLZERO                      ((uint32_t)0x00000000)

#define SD_WIDE_BUS_SUPPORT             ((uint32_t)0x00040000)
#define SD_SINGLE_BUS_SUPPORT           ((uint32_t)0x00010000)
#define SD_CARD_LOCKED                  ((uint32_t)0x02000000)
#define SD_CARD_PROGRAMMING             ((uint32_t)0x00000007)
#define SD_CARD_RECEIVING               ((uint32_t)0x00000006)
#define SD_DATATIMEOUT                  ((uint32_t)0x000FFFFF)
#define SD_0TO7BITS                     ((uint32_t)0x000000FF)
#define SD_8TO15BITS                    ((uint32_t)0x0000FF00)
#define SD_16TO23BITS                   ((uint32_t)0x00FF0000)
#define SD_24TO31BITS                   ((uint32_t)0xFF000000)
#define SD_MAX_DATA_LENGTH              ((uint32_t)0x01FFFFFF)

#define SD_HALFFIFO                     ((uint32_t)0x00000008)
#define SD_HALFFIFOBYTES                ((uint32_t)0x00000020)

/* Command Class Supported */
#define SD_CCCC_LOCK_UNLOCK             ((uint32_t)0x00000080)
#define SD_CCCC_WRITE_PROT              ((uint32_t)0x00000040)
#define SD_CCCC_ERASE                   ((uint32_t)0x00000020)

/* Following commands are SD Card Specific commands.
   SDIO_APP_CMD should be sent before sending these commands. */
#define SDIO_SEND_IF_COND               ((uint32_t)0x00000008)

#define SDIO_INIT_CLK_DIV               ((uint8_t)0x80)

#if 1
#define SDIO_TRANSFER_CLK_DIV		    ((uint8_t)0x0)   //45m/(x+2)
#define ENABLE_DATA_WIDE 				0
#else
#define SDIO_TRANSFER_CLK_DIV		    ((uint8_t)0x6)   //45m/(x+2)
#define ENABLE_DATA_WIDE 	1			//4bitģʽ��ʱ�Ӳ���̫�ߣ���������CLK_DIV = 6�����ʸ�1bitģʽ���
#endif


struct sdio_cccr cccr;
//struct sdio_func func;

//static wait_event_t sd_waitq;
static OsMutex sd_mutex;
typedef unsigned short              u16;
typedef unsigned int                u32;
typedef unsigned char                u8;
static uint8_t convert_from_bytes_to_power_of_two(uint16_t NumberOfBytes);
int mmc_send_relative_addr( u16 *rca);
int sdio_read_cccr(void);
int sdio_read_common_cis(void);
int sdio_enable_wide(void);
int mmc_sdio_switch_hs(int enable);
int mmc_send_if_cond(u32 ocr);
#include "sdio.h"
#include "gpio.h"

//static wait_event_t sd_waitq;
//static mutex_t sd_mutex;
 unsigned char *SDIOTempBuf=(unsigned char*)(VMEM_ADDR+DECODER_MEM_OFFSET-SDIO_SEND_RECV_BUF_SIZE);
 //unsigned char SDIOTempBuf[1024];
/*wifi*/
void enable_sdio_int()
{
}


/* Private functions ---------------------------------------------------------*/

/*
 * ��������SD_Init
 * ����  ����ʼ��SD����ʹ�����ھ���״̬(׼����������)
 * ����  ����
 * ���  ��-SD_Error SD���������
 *         �ɹ�ʱ��Ϊ SD_OK
 * ����  ���ⲿ����
 */



int mmc_go_idle()
{
	SdioSendCommand(SDIO_GO_IDLE_STATE, 0, 20);
	return SD_OK;
}

/*
 * ��������SD_SelectDeselect
 * ����  ��Selects od Deselects the corresponding card
 * ����  ��-addr ѡ�񿨵ĵ�ַ
 * ���  ��-SD_Error SD���������
 *         �ɹ�ʱ��Ϊ SD_OK
 * ����  ���ⲿ����
 */
SD_Error SD_SelectDeselect(uint32_t addr)
{
	uint8_t Respons[5];
	if(SdioSendCommand(SDIO_SEL_DESEL_CARD,addr,10) == NO_ERR)
	{
		SdioGetCmdResp(Respons,5);
		return SD_OK;
	}
	else
	{
		printf("CMD_SDIO_SEL_DESEL_CARD error\n");
		return !SD_OK;
	}
}


int mmc_send_if_cond(u32 ocr)
{
	SdioSendCommand(SD_SEND_IF_COND, ocr, 20);
	return SD_OK;
}



/**
  * @brief  Converts the number of bytes in power of two and returns the
  *   power.
  * @param  NumberOfBytes: number of bytes.
  * @retval None
  */
static uint8_t convert_from_bytes_to_power_of_two(uint16_t NumberOfBytes)
{
	uint8_t count = 0;

	while (NumberOfBytes != 1)
	{
		NumberOfBytes >>= 1;
		count++;
	}
	return (count);
}



#include "delay.h"
int mmc_send_io_op_cond(u32 ocr, u32 *rocr)
{
	int i;
	u32 response;
	SD_Error errorstatus = SD_OK;
	uint8_t Respons[6];
    int ret;
	for (i = 2000; i; i--)
	{
		ret = SdioSendCommand(SD_IO_SEND_OP_COND,ocr,100);
//        printf("========mmc_send_io_op_cond== %d ==\r\n",ret);
//		SdioGetCmdResp(Respons,5);
        SdioGetCmdResp(&response,5);

//		printf("mmc_send_io_op_cond:%02X %02X %02X %02X %02X %02X\n",Respons[0],Respons[1],Respons[2],Respons[3],Respons[4],Respons[5]);
		//sleep(10);
		//if (Respons[0] & MMC_CARD_BUSY)
			break;
		OS_MsDelay(1000);
		//WaitMs(1000);
	}
	if (i == 0)
	{
		
		errorstatus = SD_ERROR;
		goto end;
	}
	if (rocr)
		*rocr = response;
 //       rocr = Respons;


end:
	if(errorstatus != SD_OK)
		printf("mmc_send_io_op_cond err:%x\n",errorstatus);

	return errorstatus;
}



int mmc_io_rw_direct_host(int write, unsigned fn,
		unsigned addr, u8 in, u8 *out)
{
	SD_Error errorstatus = SD_OK;
	uint8_t ResBuf[6];
	
	uint32_t SDIO_Argument;

	SDIO_Argument = write ? 0x80000000 : 0x00000000;
	SDIO_Argument |= fn << 28;
	SDIO_Argument |= (write && out) ? 0x08000000 : 0x00000000;
	SDIO_Argument |= addr << 9;
	SDIO_Argument |= in;
	OS_MutexLock(sd_mutex);
	
	if(SdioSendCommand(SD_IO_RW_DIRECT,SDIO_Argument,100) != NO_ERR)
	{
		errorstatus = !SD_OK;
		printf("CMD_SD_IO_RW_DIRECT_ERROE\n");
		goto end;
	}
	SdioGetCmdResp(ResBuf,6);
	if (ResBuf[0] & R5_ERROR)
	{
		errorstatus = SD_ILLEGAL_CMD;
	}
	if (ResBuf[0] & R5_FUNCTION_NUMBER)
	{
		errorstatus = SD_SDIO_UNKNOWN_FUNCTION;
	}
	if (ResBuf[0] & R5_OUT_OF_RANGE)
	{
		errorstatus = SD_CMD_OUT_OF_RANGE;
	}

	if (out)
		*out = ResBuf[4] & 0xff;

end:	
	OS_MutexUnLock(sd_mutex);
	
	return errorstatus;
}


int mmc_io_rw_direct(int write, unsigned fn, unsigned addr, u8 in, u8 *out)
{
	return mmc_io_rw_direct_host(write, fn, addr, in, out);
}

#define DCTRL_CLEAR_MASK         ((uint32_t)0xFFFFFF08)


#define CCR_ENABLE_Reset        ((uint32_t)0xFFFFFFFE)
extern void woal_sdio_interrupt(struct sdio_func *func);
//DECLARE_MONITOR_ITEM("SDIO INT CNT", sdio_int_cnt);

/*wifi*/
#if 1
int mmc_io_rw_extended_sram(int write, unsigned fn,
		unsigned addr, int incr_addr, u8 *buf, unsigned blocks, unsigned blksz)
{
	int ret;
	u8 out;
	uint32_t irq_flag;
	SD_Error errorstatus = SD_OK;
	uint8_t power = 0;
	uint32_t *tempbuff = (uint32_t *)buf;
	uint32_t TotalNumberOfBytes;
	uint32_t SDIO_Argument = 0;
	uint8_t ResBuf[6];
	
	OS_MutexLock(sd_mutex);
	power = convert_from_bytes_to_power_of_two(blksz);
	TotalNumberOfBytes = blocks * blksz;

	SDIO_Argument = write ? 0x80000000 : 0x00000000;
	SDIO_Argument |= fn << 28;
	SDIO_Argument |= incr_addr ? 0x04000000 : 0x00000000;
	SDIO_Argument |= addr << 9;
	if (blocks == 1 && blksz < 256)
		SDIO_Argument |= /*(blksz == 256) ? 0 : */blksz;	/* byte mode */
	else
		SDIO_Argument |= 0x08000000 | blocks;		/* block mode */
	SdioEnableClk();
	if(!write)//r
	{
//		printf("r\n");
		if (blocks == 1 && blksz < 256)
		{
//			printf("byte\n");
			SdioStartReciveData(SDIOTempBuf, TotalNumberOfBytes);
			SdioSendCommand(SD_IO_RW_EXTENDED,SDIO_Argument,100);
			SdioGetCmdResp(ResBuf,6);
			while(1)
			{
				if(SdioIsDatTransDone())
				{
					break;
				}
			}
			SdioEndDatTrans();
			memcpy(buf, SDIOTempBuf, TotalNumberOfBytes);
		}
		else
		{
			uint32_t i;
//			printf("r_blik\n");
			SdioStartReciveData(SDIOTempBuf, blksz);
//			blocks--;
			SdioSendCommand(SD_IO_RW_EXTENDED,SDIO_Argument,100);
			SdioGetCmdResp(ResBuf,6);
			while(!SdioIsDatTransDone());
			memcpy(buf, SDIOTempBuf, blksz);
			for(i=1;i<blocks;i++)
			{
                
                
   				SdioStartReciveData(SDIOTempBuf, blksz);
				while(!SdioIsDatTransDone());
				memcpy(buf+i*blksz, SDIOTempBuf, blksz);
                			}
		}
        
		SdioEndDatTrans();
        
	}
	else
	{	
		SdioSendCommand(SD_IO_RW_EXTENDED,SDIO_Argument,100);
		SdioGetCmdResp(ResBuf,6);
		if (blocks == 1 && blksz < 256)
		{
			memcpy(SDIOTempBuf,buf,TotalNumberOfBytes);
			SdioStartSendData(SDIOTempBuf, TotalNumberOfBytes);
			while(1)
			{
				if(SdioIsDatTransDone())
				{
					break;
				}
			}
		}
		else
		{
			uint32_t i;
			for(i=0;i<blocks;i++)
			{
				memcpy(SDIOTempBuf,buf+i*blksz,blksz);
				SdioStartSendData(SDIOTempBuf, blksz);
				while(!SdioIsDatTransDone());
			}
		}
	}
	SdioEndDatTrans();
	OS_MutexUnLock(sd_mutex);
	//printf("data :%02X %02X  %02X %02X\n",buf[0],buf[1],buf[2],buf[3]);
	return (errorstatus == SD_OK)?0:-1;
}
#endif
//uint8_t SDIO_DMA_BUFFER_A[64] __attribute__((at(0x20003000)));
uint8_t *SDIO_DMA_BUFFER_A = (uint8_t *)(VMEM_ADDR+DECODER_MEM_OFFSET-SDIO_SEND_RECV_BUF_SIZE/2);
int mmc_io_rw_extended_sdram(int write, unsigned fn, unsigned addr, int incr_addr, u8 *buf, unsigned blocks, unsigned blksz)
{
	SD_Error errorstatus = SD_OK;

	uint32_t TotalNumberOfBytes;
	uint32_t SDIO_Argument = 0;
	uint8_t  ResBuf[6];
	
	OS_MutexLock(sd_mutex);

	TotalNumberOfBytes = blocks * blksz;

	SDIO_Argument = write ? 0x80000000 : 0x00000000;
	SDIO_Argument |= fn << 28;
	SDIO_Argument |= incr_addr ? 0x04000000 : 0x00000000;
	SDIO_Argument |= addr << 9;
	if (blocks == 1 && blksz < 256)
		SDIO_Argument |= /*(blksz == 256) ? 0 : */blksz;	/* byte mode */
	else
		SDIO_Argument |= 0x08000000 | blocks;		/* block mode */
	if(!write)//r
	{
		if (blocks == 1 && blksz < 256)
		{
			SdioStartReciveData(SDIO_DMA_BUFFER_A, TotalNumberOfBytes); 
			SdioSendCommand(SD_IO_RW_EXTENDED,SDIO_Argument,100);
			SdioGetCmdResp(ResBuf,6);
			while(1)
			{
				if(SdioIsDatTransDone())
				{
					break;
				}
			}
            SdioEndDatTrans();
            memcpy(buf,(uint8_t*)SDIO_DMA_BUFFER_A,TotalNumberOfBytes);
		}
		else
		{
			uint32_t i;

			SdioStartReciveData(SDIO_DMA_BUFFER_A, blksz);
			SdioSendCommand(SD_IO_RW_EXTENDED,SDIO_Argument,100);
			SdioGetCmdResp(ResBuf,6);
            while(!SdioIsDatTransDone());
            memcpy(buf,SDIO_DMA_BUFFER_A,blksz);
			for(i=1;i<blocks;i++)
			{
                SdioStartReciveData(SDIO_DMA_BUFFER_A, blksz);
                while(!SdioIsDatTransDone());

                memcpy(buf+i*blksz,SDIO_DMA_BUFFER_A,blksz);
            }
            while(!SdioIsDatTransDone());
            SdioEndDatTrans();
		}
	}
	else
	{	
		SdioSendCommand(SD_IO_RW_EXTENDED,SDIO_Argument,100);
		SdioGetCmdResp(ResBuf,6);
		if (blocks == 1 && blksz < 256)
		{
			memcpy((uint8_t*)SDIO_DMA_BUFFER_A, buf, TotalNumberOfBytes);
            SdioStartSendData((uint8_t*)SDIO_DMA_BUFFER_A, TotalNumberOfBytes);
			while(1)
			{
				if(SdioIsDatTransDone())
				{
					break;
				}
			}
		}
		else
		{
			uint32_t i;
			for(i=0;i<blocks;i++)
			{
				memcpy((uint8_t*)SDIO_DMA_BUFFER_A, buf+i*blksz, blksz);
                SdioStartSendData((uint8_t*)SDIO_DMA_BUFFER_A, blksz);
				while(!SdioIsDatTransDone());
			}
		}
	}
	SdioEndDatTrans();
	OS_MutexUnLock(sd_mutex);

	return (errorstatus == SD_OK) ? 0 : -1;
}

int mmc_io_rw_extended(int write, unsigned fn, unsigned addr, int incr_addr, u8 *buf, unsigned blocks, unsigned blksz)
{
							  
	//printf(">");
	//printf("addr:%x ",((uint32_t)buf+blocks * blksz));
	if(((uint32_t)buf+blocks * blksz) <= 0x20010000)
	{
		//�ڲ�
		//printf("s");
		mmc_io_rw_extended_sram(write, fn,addr, incr_addr, buf, blocks, blksz);
	}
	else
	{
		//�ⲿ
		//printf("e");
		mmc_io_rw_extended_sdram(write, fn,addr, incr_addr, buf, blocks, blksz);
	}
	//printf("<\n");
}


int mmc_send_relative_addr( u16 *rca)
{
	uint8_t ResBuf[6];
	if (rca == 0)
		return SD_ERROR;
	if(SdioSendCommand(SDIO_SET_REL_ADDR,0,100) == NO_ERR)
	{
		SdioGetCmdResp(ResBuf,5);
		*rca = ResBuf[1];
		*rca = *rca<<8;
		*rca = *rca + ResBuf[2];
		return SD_OK;
	}
	printf("CMD_SDIO_SET_REL_ADDR ERROR\n");
	return SD_ERROR;
}

int sdio_read_cccr()
{
	int ret;
	int cccr_vsn;
	unsigned char data;

	memset(&cccr, 0, sizeof(struct sdio_cccr));
	ret = mmc_io_rw_direct(0, 0, SDIO_CCCR_CCCR, 0, &data);
	if (ret != SD_OK)
		goto out;

	cccr_vsn = data & 0x0f;
//	printf("SDIO_CCCR_CCCR:%x\n", cccr_vsn);

	if (cccr_vsn > SDIO_CCCR_REV_1_20)
	{
		printf("unrecognised CCCR structure version %d\n", cccr_vsn);
		return SD_ERROR;
	}

	cccr.sdio_vsn = (data & 0xf0) >> 4;

	ret = mmc_io_rw_direct(0, 0, SDIO_CCCR_CAPS, 0, &data);
	if (ret != SD_OK)
		goto out;

//	printf("SDIO_CCCR_CAPS:%x\n", data);
	if (data & SDIO_CCCR_CAP_SMB)
		cccr.multi_block = 1;
	if (data & SDIO_CCCR_CAP_LSC)
		cccr.low_speed = 1;
	if (data & SDIO_CCCR_CAP_4BLS)
		cccr.wide_bus = 1;

	if (cccr_vsn >= SDIO_CCCR_REV_1_10)
	{
		ret = mmc_io_rw_direct(0, 0, SDIO_CCCR_POWER, 0, &data);

//		printf("SDIO_CCCR_POWER:%x\n", data);
		if (ret != SD_OK)
			goto out;

		if (data & SDIO_POWER_SMPC)
			cccr.high_power = 1;
	}

	if (cccr_vsn >= SDIO_CCCR_REV_1_20)
	{
		ret = mmc_io_rw_direct(0, 0, SDIO_CCCR_SPEED, 0, &data);

//		printf("SDIO_CCCR_SPEED:%x\n", data);
		if (ret != SD_OK)
			goto out;

		if (data & SDIO_SPEED_SHS)
			cccr.high_speed = 1;
	}

out:
	if (ret != SD_OK)
		printf("sdio_read_cccr err\n");
	return ret;
}

int mmc_sdio_switch_hs(int enable)
{
	int ret;
	u8 speed;

	ret = mmc_io_rw_direct(0, 0, SDIO_CCCR_SPEED, 0, &speed);
	if (ret != SD_OK)
	{
		printf("mmc_sdio_switch_hs err:%d\n", ret);
		return ret;
	}
	if (enable)
		speed |= SDIO_SPEED_EHS;
	else
		speed &= ~SDIO_SPEED_EHS;

	ret = mmc_io_rw_direct(1, 0, SDIO_CCCR_SPEED, speed, NULL);
	if (ret != SD_OK)
		printf("mmc_sdio_switch_hs err1:%d\n", ret);
	else
		printf("mmc_sdio_switch_hs ok\n");

	return ret;
}

int sdio_enable_wide()
{
}

struct cis_tpl
{
	unsigned char code;
	unsigned char min_size;
	//	tpl_parse_t *parse;
};

struct sdio_func_tuple
{
	unsigned char code;
	unsigned char size;
	unsigned char data[128];
};

//struct sdio_func_tuple func_tuple[10];

static int sdio_read_cis(int f_n)
{
	int ret;
	struct sdio_func_tuple *this;
	unsigned i, ptr = 0, tuple_cnt = 0;
	unsigned char tpl_code, tpl_link;
	/*
	 * Note that this works for the common CIS (function number 0) as
	 * well as a function's CIS * since SDIO_CCCR_CIS and SDIO_FBR_CIS
	 * have the same offset.
	 */
	//printf("sdio_read_cis start\n");
	for (i = 0; i < 3; i++)
	{
		unsigned char x;
		//sleep(50);
		ret = mmc_io_rw_direct(0, f_n,
				SDIO_FBR_BASE(f_n) + SDIO_FBR_CIS + i, 0, &x);
		if (ret)
			return ret;
		ptr |= x << (i * 8);
	}

//	p_info("read_cis,fn:%d,addr:%d\n", f_n, ptr);
	
	do
	{
		//sleep(50);
		ret = mmc_io_rw_direct(0, f_n, ptr++, 0, &tpl_code);
		//printf(">>>>>>>>>>>>>>>>>>>>>>>ggg>>>>>>>>>>>>>>>read_cis,ret:%x,addr:%x\n", ret, ptr);
		if (ret)
			break;

		/* 0xff means we're done */
		if (tpl_code == 0xff)
			break;

		/* null entries have no link field or data */
		if (tpl_code == 0x00)
			continue;

		ret = mmc_io_rw_direct(0, f_n, ptr++, 0, &tpl_link);
		//printf(">>>>>>>>>>>>>>>>>>>>kk>>>>>>>>>>>>>>>>>>read_cis,ret:%x,addr:%x\n", ret, ptr);
		if (ret)
			break;

		/* a size of 0xff also means we're done */
		if (tpl_link == 0xff)
			break;

//		p_info("tpl code:%x,size:%d\n", tpl_code, tpl_link);


		if (tuple_cnt > 9 || tpl_link > 128)
		{
//			p_dbg("tuple_cnt over\n");
			break;
		}
//		func_tuple[tuple_cnt].size = tpl_link;
		
	//	this = (struct sdio_func_tuple *)mem_malloc(sizeof(*this) + tpl_link);
		if (!this)
			return -ENOMEM;

		for (i = 0; i < tpl_link; i++)
		{
			ret = mmc_io_rw_direct(0, f_n,
					ptr + i, 0, &this->data[i]);
			if (ret)
				break;
		}

//		dump_hex("cis", this->data, i);

		if (ret)
		{
		//	mem_free(this);
			break;
		}
		if(tpl_code == CISTPL_VERS_1)
//			p_info("%s\n", this->data + 2);
		
	//	mem_free(this);

		ptr += tpl_link;
		tuple_cnt += 1;
	}
	while (!ret);

	if (tpl_link == 0xff)
		ret = 0;

	return ret;
}

int sdio_read_common_cis()
{
	return sdio_read_cis(NULL);
}
/*wifi*/
int SD_Init(void)
{
    int i=0;
	int errorstatus = SD_OK;
	int ocr;
	u16 rca;
#if 0	
	sd_waitq = init_event();
	sd_mutex = mutex_init(__FUNCTION__);
	
	SDIO_PDN_HIGH;
	SDIO_RESET_LOW;
#endif
	OS_MutexInit(&sd_mutex);
	GpioSdIoConfig(1);
	SdioEnableClk();
	SdioSetClk(6);
	SdioControllerInit();
	SdioEnableClk();
	SdioSetClk(6);
again:
    OS_MsDelay(500);
	//mmc_go_idle();	
	//mmc_send_if_cond(SD_CHECK_PATTERN);
	errorstatus = mmc_send_io_op_cond(0, (u32*)&ocr);

    printf("SD_init  ocr = %X \r\n", ocr);
	if (errorstatus != SD_OK)
	{
        printf("1111 \r\n");

         goto out;
	}
#if 1

	errorstatus = mmc_send_io_op_cond(ocr, (u32*)&ocr);
	if (errorstatus != SD_OK)
	{
		goto out;
	}
	errorstatus = mmc_send_relative_addr( &rca);
    
    printf("SD_init  rca = %X \r\n", rca);
	if (errorstatus != SD_OK)
	{
		goto out;
	}
	errorstatus = SD_SelectDeselect(rca << 16);
	if (errorstatus != SD_OK)
	{
		goto out;
	}
	errorstatus = sdio_read_cccr();
	if (errorstatus != SD_OK)
	{
		goto out;
	}
//	sdio_read_common_cis();
	errorstatus = mmc_sdio_switch_hs(1);
	if (errorstatus != SD_OK)
	{
		goto out;
	}
#if ENABLE_DATA_WIDE
	errorstatus = sdio_enable_wide();
	if (errorstatus != SD_OK)
	{
		goto out;
	}
#endif
	
out:
	if (errorstatus != SD_OK)
	{
//		printf("SDIO card init failed! \r\n");
        OS_MsDelay(1000);
		goto again;
	}
	printf("SDIO�豸��ʼ��OK\n");
	SdioSetClk(0);
#endif    
	return errorstatus;

}

