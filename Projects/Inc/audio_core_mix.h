


typedef enum
{
	VbCmdEnDis,
	VbCmdCutoffFreq,
	VbCmdIntensity,
	VbCmdEnhanced
}EffectVbCmd;

typedef enum
{
	ThrDimEnDis,
	ThrDimDepth,
	ThrDimPreGain,
	ThrDimPostGain
}EffectThrDimCmd;

typedef enum
{
	MixCmdNone = 0,

	MixCmdVBEn,			// params: 0 - disable VB, 1 - enable VB
	MixCmdVBCutoff,		// params: 
	MixCmdVBIntensity,	//
	MixCmdVBEnhanced,	// params: 0, or 1

	MixCmd3DEn,			// params: 0 - disable VB, 1 - enable VB
	MixCmd3DDepth,		//
	MixCmd3DPreGain,	//
	MixCmd3DPostGain,	//
}MixCommandType;

BOOL AudioCoreMixInit(void);

BOOL AudioCoreMixDeinit(void);

/**
 * Source related functions
 */
BOOL AudioCoreMixSourceRegister(AudioCoreIndex srcIndex, AudioCoreGetData getSrcData);

BOOL AudioCoreMixEnable(AudioCoreIndex srcIndex);

BOOL AudioCoreMixSourceDisable(AudioCoreIndex srcIndex);

BOOL AudioCoreMixSourcePcmConfig(AudioCoreIndex srcIndex, AudioCorePcmParams *pcmParams);

BOOL AudioCoreMixSourceGainConfig(AudioCoreIndex srcIndex, int16_t gain);

/**
 * Sink related functions
 */
BOOL AudioCoreMixSinkRegister(AudioCoreIndex sinkIndex, AudioCorePutData putSinkData);

BOOL AudioCoreMixSinkEnable(AudioCoreIndex sinkIndex);

BOOL AudioCoreMixSinkDisable(AudioCoreIndex sinkIndex);

//BOOL AudioCoreMixSinkPcmConfig(AudioCoreIndex sinkIndex, AudioCorePcmParams *pcmParams);

BOOL AudioCoreMixSinkGainConfig(AudioCoreIndex sinkIndex, int16_t gain);

BOOL AudioCoreMixedPcmConfig(AudioCorePcmParams *pcmParams);
uint32_t AudioCoreMixedEffectControl(EffectType effectType, uint32_t cmd, uint32_t param);


void AudioCoreMixProcess(void);
