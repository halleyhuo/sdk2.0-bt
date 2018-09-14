///////////////////////////////////////////////////////////////////
//
// audio_core_prot.c
//

#include <stdarg.h>

#include "type.h"

#include "audio_core.h"
#include "audio_core_prot.h"

__INLINE static uint32_t GetParams(uint8_t *cmd, uint8_t index)
{
	return Get32(cmd, (index*4)+3);
}

static void SendAudioCoreReport(int32_t * params, uint32_t paramsNums)
{
	FuartSend((uint8_t *)params, paramsNums * 4);
}

int16_t AudioCoreParseCommand(uint8_t *cmd)
{
	ACProtCode		code;
	uint8_t			paramsNums;

	if(cmd == NULL)
		return -1;


	if(cmd[0] != AC_PROT_TYPE_CMD)
		return -2;

	code = Get16(cmd, 1);
	paramsNums = cmd[3];

	switch(code)
	{
		case AC_PROT_CODE_QUERY_SOURCE:
			AudioCoreSourceQuery();
			break;

		case AC_PROT_CODE_QUERY_CPROC:
			AudioCoreProcQuery();
			break;

		case AC_PROT_CODE_QUERY_SINK:
			AudioCoreSinkQuery();
			break;

		case AC_PROT_CODE_QUERY_SOURCE_EFFECTS:
			break;
		case AC_PORT_CODE_QUERY_CPROC_EFFECTS:
			break;
		case AC_PORT_CODE_QUERY_SINK_EFFECTS:
			break;

		case AC_PROT_CODE_ALC_ENDIS:
			break;
		case AC_PROT_CODE_ALC_PARAMS:
			break;
		case AC_PROT_CODE_DAC_ENDIS:
			break;
		case AC_PROT_CODE_DAC_PARAMS:
			break;
		case AC_PROT_CODE_MIC_ENDIS:
			break;
		case AC_PROT_CODE_MIC_PARAMS:
			break;
		case AC_PROT_CODE_LINE1_ENDIS:
			break;
		case AC_PROT_CODE_LINE1_PARAMS:
			break;
		case AC_PROT_CODE_LINE23_ENDIS:
			break;
		case AC_PROT_CODE_LINE23_PARAMS:
			break;
		case AC_PROT_CODE_ADC_DIGITAL:
			break;



		case AC_PROT_CODE_AEC_ENDIS:
		case AC_PROT_CODE_EQ_ENDIS:
		case AC_PROT_CODE_VB_ENDIS:
		case AC_PROT_CODE_3D_ENDIS:
		case AC_PROT_CODE_ECHO_ENDIS:
		case AC_PROT_CODE_REVERB_ENDIS:
		case AC_PROT_CODE_DRC_ENDIS:
			{
				ACPos		pos;
				uint8_t		posIndex;
				uint8_t		effectIndex;
				uint8_t		en;

				pos = (ACPos)GetParams(cmd, 1);
				posIndex = (uint8_t)GetParams(cmd, 2);
				effectIndex = (uint8_t)GetParams(cmd, 3);
				en = (uint8_t)GetParams(cmd, 4);

				switch(pos)
				{
					case AC_SOURCE:
						if(en == 1)
							AudioCoreSourceEffectEnable(posIndex, effectIndex);
						else
							AudioCoreSourceEffectDisable(posIndex, effectIndex);
						break;

					case AC_CPROC:
						if(en == 1)
							AudioCoreProcEffectEnable(effectIndex);
						else
							AudioCoreProcEffectDisable(effectIndex);
						break;

					case AC_SINK:
						break;
				}
			}
			break;

		case AC_PROT_CODE_EQ_PARAMS:
			{
				ACPos		pos;
				uint8_t		posIndex;
				uint8_t		effectIndex;
				int32_t		*params = NULL;
				uint8_t		i;

				pos				= (ACPos)GetParams(cmd, 1);
				posIndex		= (uint8_t)GetParams(cmd, 2);
				effectIndex		= (uint8_t)GetParams(cmd, 3);
				params = (int32_t *)pvPortMalloc((paramsNums-3)*4);

				for(i = 0; i < paramsNums-3; i++)
				{
					params[i] = GetParams(cmd, i+4);
				}

				switch(pos)
				{
					case AC_SOURCE:
						AudioCoreSourceEffectSet(posIndex, effectIndex, EffectTypeEQ, params);
						break;

					case AC_CPROC:
						AudioCoreProcEffectSet(effectIndex, EffectTypeEQ, params);
						break;

					case AC_SINK:
						break;
				}
				if(params)
				{
					vPortFree(params);
				}
			}
			break;

		case AC_PROT_CODE_VB_PARAMS:
			{
				ACPos		pos;
				uint8_t		posIndex;
				uint8_t		effectIndex;
				int32_t		params[3];


				pos				= (ACPos)GetParams(cmd, 1);
				posIndex		= (uint8_t)GetParams(cmd, 2);
				effectIndex		= (uint8_t)GetParams(cmd, 3);
				params[0]		= GetParams(cmd, 4);
				params[1]		= GetParams(cmd, 5);
				params[2]		= GetParams(cmd, 6);

				switch(pos)
				{
					case AC_SOURCE:
						AudioCoreSourceEffectSet(posIndex, effectIndex, EffectTypeVB, params);
						break;

					case AC_CPROC:
						AudioCoreProcEffectSet(effectIndex, EffectTypeVB, params);
						break;

					case AC_SINK:
						break;
				}
			}
			break;

		case AC_PROT_CODE_3D_PARAMS:
			{
				ACPos		pos;
				uint8_t		posIndex;
				uint8_t		effectIndex;
				int32_t		params[3];


				pos				= (ACPos)GetParams(cmd, 1);
				posIndex		= (uint8_t)GetParams(cmd, 2);
				effectIndex		= (uint8_t)GetParams(cmd, 3);
				params[0]		= GetParams(cmd, 4);
				params[1]		= GetParams(cmd, 5);
				params[2]		= GetParams(cmd, 6);

				switch(pos)
				{
					case AC_SOURCE:
						AudioCoreSourceEffectSet(posIndex, effectIndex, EffectType3D, params);
						break;

					case AC_CPROC:
						AudioCoreProcEffectSet(effectIndex, EffectType3D, params);
						break;

					case AC_SINK:
						break;
				}
			}
			break;
		
		case AC_PROT_CODE_ECHO_PARAMS:
			break;
		
			break;
		case AC_PROT_CODE_REVERB_PARAMS:
			break;
		
			break;
		case AC_PROT_CODE_DRC_PARAMS:
			break;
		case AC_PROT_CODE_AEC_PARAMS:
			break;
	}

	return 0;
}

#define MAKE_FRAME_HEADER(t, c, n)		params[0] = (((n) << 24) | ((c) << 8) | (t))

void AudioCoreReport(ACProtCode code, ...)
{
	uint8_t				paramsNums;
	int32_t				*params;
	va_list				argList;
	uint8_t				i;

	va_start(argList,code);

	switch(code)
	{
		case AC_PROT_CODE_SOURCE_NUMS:
			paramsNums = 1;
			break;

		case AC_PROT_CODE_SOURCE_DESC:
			paramsNums = 4;
			break;
	}

	params = (int32_t *)pvPortMalloc((paramsNums + 1) * 4);
	if(params != NULL)
	{
		MAKE_FRAME_HEADER(AC_PROT_TYPE_RPT, code, paramsNums);
		for(i = 0; i < paramsNums; i++)
		{
			params[i+1] = va_arg(argList, uint32_t);
		}

		SendAudioCoreReport(params, (paramsNums + 1));
		vPortFree(params);		
	}

	va_end(argList);

}

