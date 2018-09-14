/**
 **************************************************************************************
 * @file    media_service.h
 * @brief   
 *
 * @author  kk
 * @version V1.0.0
 *
 * $Created: 2017-3-17 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __MEDIA_SERVICE_H__
#define __MEDIA_SERVICE_H__

#include "type.h"

#include "fsinfo.h"
#include "fat_file.h"
#include "folder.h"
#include "audio_decoder.h"

typedef enum _PLAYER_STATE
{
    PLAYER_STATE_IDLE = 0,    // ����
    PLAYER_STATE_PLAYING,     // ����
    PLAYER_STATE_PAUSE,       // ��ͣ
    PLAYER_STATE_FF,          // �����
    PLAYER_STATE_FB,          // ������
} PLAYER_STATE;


typedef enum _AUDIO_PLAYER_PLAY_MODE_
{
    PLAY_MODE_REPEAT_ALL,     // ȫ��˳��ѭ������
    PLAY_MODE_REPEAT_FOLDER,  // �ļ���˳��ѭ������
    PLAY_MODE_REPEAT_ONE,     // ����ѭ������
    PLAY_MODE_RANDOM_ALL,     // ȫ���������
	PLAY_MODE_RANDOM_FOLDER,  // �ļ����������
    PLAY_MODE_PREVIEW_PLAY,   // Ԥ�����ţ����ļ�����У�ѡ�и�������Ԥ�����ż����ӣ�
    PLAY_MODE_SUM,
} AUDIO_PLAYER_MODE;

///////////////////////////////////////////////////////
// ����ģʽ�����ݽṹ������ϵͳ�����е����ĸ���ģʽ����(����ʱ����ѭ������)
#define REPEAT_CLOSED  0 // �����ر�
#define REPEAT_A_SETED 1 // �����ø������
#define REPEAT_OPENED  2 // ������

typedef struct _REPEAT_AB_
{
	uint32_t StartTime; // ��ʼʱ��(MS)������ʱ��ѡ�񸴶�ģʽ���������
	uint32_t Times;      // ����ʱ�䳤��(MS)�������ͨ�����ý���ʱ��ķ�ʽ���������յĸ���ʱ���Դ�Ϊ׼����Ԥ��ֵ
	uint8_t LoopCount;  // ѭ����������Ԥ������(EPROM)��Ĭ��3��
	uint8_t RepeatFlag; // ��ǰ�Ƿ񸴶�ģʽ(0 - �����ر�, 1 - ���������, 2 - �������յ㣬����ʼ����)
} REPEAT_AB;

///////////////////////////////////////////////////////
typedef struct _AUDIO_PLAYER_CONTROL_
{
	uint8_t		CurPlayState;		// ������״̬ (uint8_t)
    uint8_t		CurPlayMode;		// ��ǰ����ѭ��ģʽ
    
    uint32_t	CurPlayTime;		// ����ʱ��, ms
	uint32_t	LastPlayTime;		// �����ϴ�UIˢ��ʱ�Ĳ���ʱ��ֵ������ˢ���Ż�

	FAT_FILE	PlayerFile;         // ��ǰ�����ļ�
	SongInfo*	CurSongInfo;		// ��ǰ������Ϣ
	uint8_t*	CurSongTitlePtr;	// ��ǰ�������⡢�����ҵ���Ϣ
	uint16_t	CurFileIndex;		// ��ǰ�ļ��ţ������ȫ��ID��
	uint16_t	TotalFileSumInDisk;	// ȫ���е��ļ�����
	
    FOLDER		PlayerFolder;		// ��ǰ�ļ��о����Ĭ���Ǵ��̸�Ŀ¼��
	uint16_t	CurFolderIndex;		// ��ǰ�ļ���ȫ��ID�ţ������ļ��и�Ŀ¼����Ŀ¼���������ļ��в���ģʽ
    uint16_t	ValidFolderSumInDisk;//ȫ������Ч���ļ������������˿��ļ���(�����������ļ����ļ���)


#ifdef FUNC_LRC_EN
	uint8_t		LrcFlag;			// �����ʾ��־(EPROM������)
	uint8_t		IsLrcRunning;
	LRC_ROW		LrcRow;
#endif

//	SW_TIMER	DelayDoTimer;		// �ӳٴ���Timer�������������л���������������˵Ȱ������ӳ��ۼӴ���
    
    REPEAT_AB	RepeatAB;			// ����ģʽ������Ϣ

//#ifdef PLAYER_SPECTRUM 				// Ƶ����ʾ
//	uint8_t		SpecTrumFlag;		// Ƶ�״򿪱�ʶ
//	SPECTRUM_INFO SpecTrumInfo;
//#endif

//	uint16_t	error_times;		// ĳЩ�������������Ĵ������û��ָ�����
//	uint8_t		IsBPMached;			// �Ƿ�FS��ƥ���˶ϵ���Ϣ����Ҫ���ڷ�ֹ������Ч�Ķϵ���Ϣ(����ģʽ��ʼ��ʧ�ܺ���Ҫ�ı�־�ж��Ƿ񱣴�ϵ���Ϣ)

//	uint16_t	InputNumber;		// ��������ѡȡ
//	SW_TIMER	NumKeyTimer;

//#ifdef FUNC_REC_PLAYBACK_EN
//	uint8_t		RecBackPlayFlag;
//#endif

//#ifndef FUNC_PLAY_RECORD_FOLDER_EN
//	uint16_t	RecFileStartId;
//	uint16_t	RecFileEndId;
//	uint16_t	RecFolderId;
//#endif

//#ifdef FUNC_BROWSER_EN
//	bool	BrowseFlag;
//#endif
} AUDIO_DISK_PLAYER;

extern AUDIO_DISK_PLAYER* gpDiskPlayer;


bool AudioPlayerInitialize(int32_t FileIndex, uint32_t FolderIndex);


/**
 * @brief
 *		Create Media service.
 * @param
 * 
 * @return  
 */
int32_t MediaServiceCreate(void);

/**
 * @brief
 *		Start Media service.
 * @param
 * 
 * @return  
 */
void MediaServiceStart(void);

/**
 * @brief
 *		Stop Media service.
 * @param
 * 
 * @return  
 */
void MediaServiceStop(void);

/**
 * @brief
 *		Kill Media service.
 * @param
 * 
 * @return  
 */
void MediaServiceKill(void);

MessageHandle GetMediaMessageHandle(void);

void * GetPtMediaFile(void);

#endif /*__MEDIA_SERVICE_H__*/

