/******************************************************************************

                  ��Ȩ���� (C), 2017-2027, �Ϻ�ɽ�����ɵ�·�ɷ����޹�˾

 ******************************************************************************
  �� �� ��   : prefrence_interface.c
  �� �� ��   : ����
  ��������   : ƫ�ô洢�������С�����������ݽ��д洢��ȡ�Ȳ���
  �����б�   :
  �޸���ʷ   :

******************************************************************************/

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#include	"type.h"
#include "spi_flash.h"
#include "file.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "prefrence_interface.h"
/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �ⲿ����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ȫ�ֱ���                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

 #define PIPE_FREERTOS 1
 #if PIPE_FREERTOS==0
   //����1
 typedef enum STORAGE_TYPE//����������ʱ��ͨѶ��ǰ�洢�����ں�
 {
     SpiFlash=1,
     SdCard,
     UsbPan,
     I2cFlash,
     UartTerminal,
     WifiTerminal,
     WifiCloudPan
 }STORAGE_TYPE_ENUM;
 typedef struct STORERAGE_AREA_MES
{
     uint8_t sName[4];
     uint8_t sIandle;
     uint8_t sIndex;
     STORAGE_TYPE_ENUM sStoretype;
     uint32_t sStartAddr;
     uint16_t sStoreareasize;//0 to chose the min block size              
}STORERAGE_AREA_MES_STRU;


#else
//����2   ����OS

typedef enum MEDIA_ACCESS_TYPE
{
     SpiFlashMEM=1,
     SpiFlashMXM,
     SdCard,
     UsbPan,
     WifiLed,
     UartDebuger          
}MEDIA_ACCESS_TYPE_ENUM;
typedef struct PIPELINE_CONTROL_MES
{
    uint32_t sName;
    int32_t sHandle;  
    uint32_t sStartAddr;
    uint32_t sLength; 
	  uint8_t sInOutMode;//write only  read only  both
    MEDIA_ACCESS_TYPE_ENUM sVirtualPipeline;
	
	  uint8_t sIndex;  //�ܵ�������  
}PIPELINE_CONTROL_MES_STRU;
typedef struct PIPELINE_MES_NODE
{
   PIPELINE_CONTROL_MES_STRU sPcms;
   struct PIPELINE_MES_NODE_STRU *next;	
}PIPELINE_MES_NODE_STRU;

/*��̬�ܵ�����  */
#define PIPELINE_1   1
#define PIPELINE_2   2
#define PIPELINE_3   3
#define PIPELINE_4   4
//Init pipeline
#define Pipeline1Init(name) 
//DeInit pipeline
#define Pipeline1DeInit(handle)  
//Open pipeline
#define PipelineOpen(name) Open(name,0x04)
//Close pipeline
#define PipelineClose(handle)  Close(handle)
//load data by pipeline
#define Pipeline1LoadData(handle,buf,len)  Read(handle, buf,len) 
//save data by pipeline
#define Pipeline1SaveData(handle,buf,len) Write(handle, buf,len) 
//pipeline num set
#define PIPELINE_STATIC_CHN_NUM 1
PIPELINE_CONTROL_MES_STRU  gStaticPipeline[PIPELINE_STATIC_CHN_NUM];

/*��̬�ܵ�����  */

//��ַӳ�����   ��ÿ��ͨ���ĵ�ַӳ�䵽��Ӧ��0x00000000��ʼ
//���򱣻�����  ��֤ÿ��ͨ�����ᱻд���    �жϹܵ���Ϣ����
//�ļ����������뿼�ǡ�ͨ���ź���������������
//���ٵ��ٹܵ����� ���ٹܵ���Ϊ��̬�ܴ����侲̬������ϣ����ٹܵ���Ϊ��̬�ܵ�����ʱ�������꼴��  ��̬Ϊ0-15 ��̬Ϊ16-255

/*****************************************************************************
 �� �� ��  : PipelineRegister
 ��������  : ���ݹܵ�ע��
 
      0-15ͨ��Ϊ�̶�ͨ���������ٶȺ�Ч�ʱȽϿ��   ����ͨ������һ���Գ�ʼ�������ڿ�ͨ
      16-255 Ϊ��̬ͨ�����ɵ�����������   ����ͨ��ʹ��Ƶ�ʲ��ߣ�ÿ��ʹ�����֮�󣬰���Ӧ����Դ�ͷţ���ע����ע��


      �����ӹ̶�ͨ����Ϣ��ȫ�� ����Ϊ��̬�����Զ����
   
 �������  : void* prs
 �������  : ��
 �� �� ֵ  : uint32_t
 ���ú���  : 
 ��������  : 
 �����㼶  :
*****************************************************************************/
#if PIPE_FREERTOS==1
  #define PreferenceInit(mes)  PipelineRegister(mes)//�궨��API�ӿ�
#else

#endif
int32_t PipelineRegister(void* prs,uint8_t chnnum )
{ 
    
}
/*****************************************************************************
 �� �� ��  : PipelineCancel
 ��������  :�ܵ�ע��
 �������  : void* prs
 �������  : ��
 �� �� ֵ  : uint32_t
 ���ú���  : 
 ��������  : 
 �����㼶  :
*****************************************************************************/
#if PIPE_FREERTOS==1
  #define PreferenceDeinit(handle)  PipelineCancel(handle)//�궨��API�ӿ�
#else

#endif
int32_t PipelineCancel(void* prs,,uint8_t chnnum)
{
   
}

/*****************************************************************************
 �� �� ��  : PipelineOpen
 ��������  : �ܵ���
 �������  : 
 �������  : ��
 �� �� ֵ  : int32_t
 ���ú���  : 
 ��������  : 
 �����㼶  :
*****************************************************************************/
int32_t PipelineOpen(int16_t pipenum)
{
     int32_t handle;
	PIPELINE_CONTROL_MES_STRU* p;
	p=prs;
   // PIPELINE_CONTROL_MES_STRU pcms = *(PIPELINE_CONTROL_MES_STRU*)prs;
    //ע�����֮����Ϣ��ô�洢
    //������


    switch( (*p).sVirtualPipeline )
    {
        case PIPELINE_1 :
 
            handle = Pipeline1Init((char*)(*p).sHandle) ;
            break;
        case PIPELINE_2 :
            break;
        case PIPELINE_3 :           
            break;
        case PIPELINE_4 :           
            break;     
        default:
          //�ӵ����� ��̬�ܵ�
            break;  
    }
    return handle;
}

/*****************************************************************************
 �� �� ��  : PipelineClose
 ��������  : �ܵ��ر�
 �������  : 
 �������  : ��
 �� �� ֵ  : int32_t
 ���ú���  : 
 ��������  : 
 �����㼶  :
*****************************************************************************/
int32_t PipelineClose(int16_t pipenum)
{
     PIPELINE_CONTROL_MES_STRU* p = prs;
    //ע����Ӧ�ļ�����Ϣ
     switch ( (*p).sVirtualPipeline)
    {
        case PIPELINE_1 :
        Pipeline1DeInit((*p).sName) ;
            break;
        case PIPELINE_2 :
            break;
        case PIPELINE_3 :            
            break;
        case PIPELINE_4 :            
            break;
       
        default:
          //������ ��̬�ܵ�
            break;
    }
    return 0;

}



/*****************************************************************************
 �� �� ��  : PipleLoadData
 ��������  : ����洢��װ������
 �������  : 
 �������  : ��
 �� �� ֵ  : uint32_t
 ���ú���  : 
 ��������  : 
 �����㼶  :
*****************************************************************************/
int32_t PiplelineLoad( void* prs, void* buf,uint32_t offset,uint32_t len)
{
   //���򱣻�����  ��Խ��
 //   ��ַ������32bit   len Ҳ������32bit  ������Ϊ�˰��������ӿ�
// ָ����void*?
	PIPELINE_CONTROL_MES_STRU* p = prs;
    //�����ź��� ����֤
    switch ( (*p).sVirtualPipeline)
    {
        case PIPELINE_1 :
            Pipeline1LoadData(offset,(char *)buf,len);
            break;
        case PIPELINE_2:
            break;
        case PIPELINE_3 :
            break;
        case PIPELINE_4:            
            break;
       
        default:
						break;
            
    }
		
		return 0;
   
}

/*****************************************************************************
 �� �� ��  : PiplelineSave
 ��������  : ����洢�� ��������
 �������  : 
 �������  : ��
 �� �� ֵ  : uint32_t
 ���ú���  : 
 ��������  : 
 �����㼶  :
*****************************************************************************/
int32_t  PiplelineSave( void* prs, void* buf,uint32_t offset,uint32_t len)
{
   //���򱣻�����  ��Խ��
	PIPELINE_CONTROL_MES_STRU* p = prs;
    switch ((*p).sVirtualPipeline)
    {
        case PIPELINE_1 :
            Pipeline1SaveData(offset,buf,len);
            break;
        case PIPELINE_2:
            break;
        case PIPELINE_3 :
            break;
        case PIPELINE_4:            
            break;
       
        default:
        
            break;
    }
		return 0;
}

#endif 
