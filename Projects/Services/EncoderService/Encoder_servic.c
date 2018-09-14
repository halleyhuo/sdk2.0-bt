/******************************************************************************

                  ��Ȩ���� (C), 2016-2026, �Ϻ�ɽ�����ɵ�·�ɷ����޹�˾

 ******************************************************************************
  �� �� ��   : Encoder_servic.c
  �� �� ��   : ����
  ��    ��   : Ben
  ��������   : 2017��3��27��
  ����޸�   :
  ��������   : ��Ƶ������
                            ���罫PCM���ݱ��MP3/MP2��
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2017��3��27��
    ��    ��   : Ben
    �޸�����   : �����ļ�

******************************************************************************/

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#include "Audio_encoder.h"
#include "Adpcm_encoder.h"
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
typedef enum ENCODER_STATE
{
    eEncoderStateNone = 0, 
    eEncoderStateReady,
    eEncoderStateInitilized,
    eEncoderStateEncoding,
    eEncoderStateWaitXrDone,
    eEncoderStateWaitTxDone,
}ENCODER_STATE_ENUM;

typedef struct ENCODER_SERVICE_CONTEX
{
    xTaskHandle sTaskHandle;
    MessageHandle sMsgHandle;
    ServiceState sServiceState;
    MemHandle *sMemHande;
    ENCODER_STATE sEncoderState;
}ENCODER_SERVICE_CONTEX_STRU;

const char encoderServiceName[]="EncoderService";

static ENCODER_SERVICE_CONTEX_STRU encoderServiceCt;

                 *
 *----------------------------- * �ڲ����� internal functions              *
 *----------------------------------------------*/
void audio_encoder_interrupt_callback(int32_t intterupt_type)
{

    ReleaseAudioMutex();
}

static int32_t EncoderServiceInit(void)
{


    return 0;
}

static int32_t EncoderServiceDeinit(void)
{



    return 0;
}
//encoderMode enum
static  void EncoderInit(uint16_t encoderMode)
{
    switch ( encoderMode )
    {
        case 1 :
            
            break;
        case 2 :
            
            break;
        case 3 :
            
            break;
        default:
            break;
    }

}

static void EncoderDeinit(void)
{
    encoderServiceCt.encoderState = eEncoderStateReady;
}

static void EncoderProcess(void)
{
    if(RT_SUCCESS == audio_encoder_initialize(...)
   {

        while(1)

        {

           if(pcm_samples_avariable() >= SAMPLES_PER_FRAME)

          {

              if(RT_SUCCESS == audio_encoder_encode(...))

                {

                  //Get the encoded data to save or transfer

                  //TODO

               }

          }

        }

    }


}

static void EncoderServiceEntrance(void* param)
{

}
  /*----------------------------------------------*
-----*/
#define RECORD_TASK_STACK_SIZE  1024
#define RECORD_TASK_PR *----------------------------------------------*/

 MessageHandle GetEncoderMessageHandle(void)
 {
    MessageHandle encoderServiceCt;
    return encoderServiceCt.msgHandle;
 }

 ServiceState GetEncodeServiceState(void)
 {


 }

#define ENCODER_SERVICE_SIZE    1024
#define ENCODER_SERVICE_PRIO   3
#define ENCODER_SERVICE_TIMEOUT  1
 int32_t EncoderServiceCreate(void)
 {
        EncoderServiceInit();
        xTaskCreate(EncoderServiceEntrance,decoderServiceName)

 }

