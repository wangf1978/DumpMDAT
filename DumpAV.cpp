#include "StdAfx.h"
#include "DumpAV.h"
#include "Bitstream.h"
#include "NALUnitFilter.h"
#include "AACStruct.h"

//#define ENABLE_JNI

#ifdef ENABLE_JNI
#include "com_superlab_ss_Mp4RawDataFixer.h"
#endif

extern NALUnitFilter g_NALUnitFilter;
extern std::vector<uint8_t> g_vSPS;
extern std::vector<uint8_t> g_vPPS;
extern AudioSpecificConfig g_AudioSpecificConfig;
extern bool g_AACConfiged;
extern NeAACDecStruct *g_hDecoder;

extern int ConvertEBSP2RBSP(uint8_t* pEBSP, int cbEBSP, uint8_t* pRBSP, int cbRBSP);

extern int GetUE(CBitstream& bs, uint64_t& val);

extern int GetSE(CBitstream& bs, int64_t& val);

extern int VerifyPPSRBSP(CBitstream& bs);

extern int VerifySPSRBSP(CBitstream& bs);

extern int SplitMDATBoxToStreams(const char* szMP4FilePath, const char* szH264File, const char* szAACFile, NALUnitFilter& filter);

const char* Audio_Object_Type_Names[42] = {
	/*0  */ "NULL",
	/*1  */	"AAC MAIN",
	/*2  */	"AAC LC",
	/*3  */	"AAC SSR",
	/*4  */	"AAC LTP",
	/*5  */	"SBR",
	/*6  */	"AAC scalable",
	/*7  */	"TwinVQ",
	/*8  */	"CELP",
	/*9  */	"HVXC",
	/*10 */	"reserved",
	/*11 */	"reserved",
	/*12 */	"TTSI",
	/*13 */	"Main synthetic",
	/*14 */	"Wavetable synthesis",
	/*15 */	"General MIDI",
	/*16 */	"Algorithmic Synthesis and Audio FX",
	/*17 */	"ER AAC LC",
	/*18 */	"reserved",
	/*19 */	"ER AAC LTP",
	/*20 */	"ER AAC scalable",
	/*21 */	"ER Twin VQ",
	/*22 */	"ER BSAC",
	/*23 */	"ER AAC LD",
	/*24 */	"ER CELP",
	/*25 */	"ER HVXC",
	/*26 */	"ER HILN",
	/*27 */	"ER Parametric",
	/*28 */	"SSC",
	/*29 */	"PS",
	/*30 */	"MPEG Surround",
	/*31 */	"escape",
	/*32 */	"Layer-1",
	/*33 */	"Layer-2",
	/*34 */	"Layer-3",
	/*35 */	"DST",
	/*36 */	"ALS",
	/*37 */	"SLS",
	/*38 */	"SLS non-core",
	/*39 */	"ER AAC ELD",
	/*40 */	"SMR Simple",
	/*41 */	"SMR Main",
};

const std::tuple<int, const char*> samplingFrequencyIndex_names[16] = {
	/*0x0*/{ 96000, "96000HZ" },
	/*0x1*/{ 88200, "88200HZ" },
	/*0x2*/{ 64000, "64000HZ" },
	/*0x3*/{ 48000, "48000HZ" },
	/*0x4*/{ 44100, "44100HZ" },
	/*0x5*/{ 32000, "32000HZ" },
	/*0x6*/{ 24000, "24000HZ" },
	/*0x7*/{ 22050, "22050HZ" },
	/*0x8*/{ 16000, "16000HZ" },
	/*0x9*/{ 12000, "12000HZ" },
	/*0xa*/{ 11025, "11025HZ" },
	/*0xb*/{ 8000,  "8000HZ" },
	/*0xc*/{ -7350,  "7350HZ" },
	/*0xd*/{ -1, "reserved" },
	/*0xe*/{ -1, "reserved" },
	/*0xf*/{ -1, "escape value" }
};

const std::tuple<int, const char*, const char*> channelConfiguration_names[16] = {
	/*0	*/{ 0, "", "defined in AOT related SpecificConfig" },
	/*1	*/{ 1, "single_channel_element", "center front speaker" },
	/*2	*/{ 2, "channel_pair_element", "left, right front speakers" },
	/*3	*/{ 3, "single_channel_element, channel_pair_element", "center front speaker, left, right front speakers" },
	/*4	*/{ 4, "single_channel_element, channel_pair_element, single_channel_element", "center front speaker, left, right center front speakers,rear surround speakers" },
	/*5	*/{ 5, "single_channel_element, channel_pair_element, channel_pair_element", "center front speaker, left, right front speakers,left surround, right surround rear speakers" },
	/*6	*/{ 6, "single_channel_element, channel_pair_element, channel_pair_element,lfe _element", "center front speaker, left, right front speakers,left surround, right surround rear speakers,front low frequency effects speaker" },
	/*7	*/{ 8, "single_channel_element, channel_pair_element, channel_pair_element,channel_pair_element,lfe_element", "center front speaker left, right center front speakers,left, right outside front speakers,left surround, right surround rear speakers,front low frequency effects speaker" },
	/*8	*/{ 0, "", "reserved" },
	/*9	*/{ 0, "", "reserved" },
	/*10*/{ 0, "", "reserved" },
	/*11*/{ 0, "", "reserved" },
	/*12*/{ 0, "", "reserved" },
	/*13*/{ 0, "", "reserved" },
	/*14*/{ 0, "", "reserved" },
	/*15*/{ 0, "", "reserved" }
};

int SetAVCParameters(uint8_t* pSPSData, int cbSPSData, uint8_t* pPPSData, int cbPPSData)
{
	if (pSPSData == nullptr || cbSPSData <= 0)
	{
		printf("Need specify the valid SPS data {pSPSData: %p, cbSPSData: %d}.\n", pSPSData, cbSPSData);
		return -1;
	}

	if (pPPSData == nullptr || cbPPSData <= 0)
	{
		printf("Need specify the valid PPS data {pPPSData: %p, cbPPSData: %d}.\n", pPPSData, cbPPSData);
		return -1;
	}

	// Trim the start_code_prefix
	if (cbSPSData > 4 && pSPSData[0] == 0 && pSPSData[1] == 0 && pSPSData[2] == 0 && pSPSData[3] == 1)
	{
		cbSPSData -= 4;
		pSPSData += 4;
	}

	if (cbPPSData > 4 && pPPSData[0] == 0 && pPPSData[1] == 0 && pPPSData[2] == 0 && pPPSData[3] == 1)
	{
		cbPPSData -= 4;
		pPPSData += 4;
	}

	// Parse the SPS and PPS, and store the key value to NALUnitFilter
	try
	{
		int cbRBSP = 0;
		uint64_t u64Val = UINT32_MAX;
		uint8_t sps_nal_unit_type = pSPSData[0] & 0x1F;

		if (sps_nal_unit_type != 7)
		{
			printf("The current input SPS data seems not to be valid.\n");
			return -1;
		}

		if ((cbRBSP = ConvertEBSP2RBSP(pSPSData, cbSPSData, g_NALUnitFilter.non_VCL_rbsp, sizeof(g_NALUnitFilter.non_VCL_rbsp))) <= 0)
		{
			printf("Failed to convert EBSP to rbsp.\n");
			return -1;
		}

		CBitstream bs(g_NALUnitFilter.non_VCL_rbsp, cbRBSP << 3);

		if (VerifySPSRBSP(bs))
		{
			printf("The SPS data seems to be corrupt.\n");
			return -1;
		}

		bs.Seek(0);
		g_NALUnitFilter.profile_idc = (uint8_t)bs.GetByte();

		// Skip constraint set bits
		bs.SkipBits(8);

		g_NALUnitFilter.level_idc = (uint8_t)bs.GetByte();

		if (GetUE(bs, u64Val) < 0 || u64Val > 0xFF)
		{
			printf("Invalid SPS data with invalid seq_parameter_set_id(%" PRIu64 ").\n", u64Val);
			return -1;
		}

		if (g_NALUnitFilter.first_sps_id > (uint8_t)u64Val)
			g_NALUnitFilter.first_sps_id = (uint8_t)u64Val;

		if (g_NALUnitFilter.last_sps_id < (uint8_t)u64Val)
			g_NALUnitFilter.last_sps_id = (uint8_t)u64Val;

		if (g_NALUnitFilter.profile_idc == 100 || g_NALUnitFilter.profile_idc == 110 ||
			g_NALUnitFilter.profile_idc == 122 || g_NALUnitFilter.profile_idc == 244 || g_NALUnitFilter.profile_idc == 44 ||
			g_NALUnitFilter.profile_idc == 83 || g_NALUnitFilter.profile_idc == 86 || g_NALUnitFilter.profile_idc == 118 ||
			g_NALUnitFilter.profile_idc == 128 || g_NALUnitFilter.profile_idc == 138 || g_NALUnitFilter.profile_idc == 139 ||
			g_NALUnitFilter.profile_idc == 134 || g_NALUnitFilter.profile_idc == 135)
		{
			if (GetUE(bs, u64Val) < 0 || u64Val > 3)
			{
				printf("Invalid chroma_format_idc value (%" PRIu64 ").\n", u64Val);
				return -1;
			}

			g_NALUnitFilter.chroma_format_idc = (uint8_t)u64Val;

			if (g_NALUnitFilter.chroma_format_idc == 3)
				g_NALUnitFilter.separate_colour_plane_flag = bs.GetBits(1) ? true : false;

			if (GetUE(bs, u64Val) < 0 || u64Val > 6)
			{
				printf("Invalid bit_depth_luma_minus8 value(%" PRIu64 ").\n", u64Val);
				return -1;
			}
			g_NALUnitFilter.bit_depth_luma_minus8 = (uint8_t)u64Val;

			if (GetUE(bs, u64Val) < 0 || u64Val > 6)
			{
				printf("Invalid bit_depth_chroma_minus8 value(%" PRIu64 ").\n", u64Val);
				return -1;
			}
			g_NALUnitFilter.bit_depth_chroma_minus8 = (uint8_t)u64Val;

			bs.GetBits(1);	// Skip qpprime_y_zero_transform_bypass_flag
			if (bs.GetBits(1))	// seq_scaling_matrix_present_flag
			{
				printf("Does not support seq_scaling_matrix_present_flag set to 1.\n");
				return -1;
			}
		}

		if (GetUE(bs, u64Val) < 0 || u64Val > 12)
		{
			printf("Invalid log2_max_frame_num_minus4 value(%" PRIu64 ").\n", u64Val);
			return -1;
		}
		g_NALUnitFilter.log2_max_frame_num_minus4 = (uint8_t)u64Val;

		if (GetUE(bs, u64Val) < 0 || u64Val > 2)
		{
			printf("Invalid pic_order_cnt_type value(%" PRIu64 ").\n", u64Val);
			return -1;
		}
		g_NALUnitFilter.pic_order_cnt_type = (uint8_t)u64Val;

		if (g_NALUnitFilter.pic_order_cnt_type == 0)
		{
			if (GetUE(bs, u64Val) < 0 || u64Val > 12)
			{
				printf("Invalid log2_max_pic_order_cnt_lsb_minus4 value(%" PRIu64 ").\n", u64Val);
				return -1;
			}
			g_NALUnitFilter.log2_max_pic_order_cnt_lsb_minus4 = (uint8_t)u64Val;
		}
		else if (g_NALUnitFilter.pic_order_cnt_type == 1)
		{

		}

	}
	catch (...)
	{
		printf("Failed to analyze the SPS NAL unit.\n");
		return -1;
	}

	try
	{
		int cbRBSP = 0;
		uint64_t u64Val = UINT32_MAX;
		uint8_t sps_nal_unit_type = pPPSData[0] & 0x1F;

		if (sps_nal_unit_type != 8)
		{
			printf("The current input PPS data seems not to be valid.\n");
			return -1;
		}

		if ((cbRBSP = ConvertEBSP2RBSP(pPPSData, cbPPSData, g_NALUnitFilter.non_VCL_rbsp, sizeof(g_NALUnitFilter.non_VCL_rbsp))) <= 0)
		{
			printf("Failed to convert EBSP to rbsp.\n");
			return -1;
		}

		CBitstream bs(g_NALUnitFilter.non_VCL_rbsp, cbRBSP << 3);

		if (VerifyPPSRBSP(bs) < 0)
		{
			printf("The PPS data seems to be corrupt.\n");
			return -1;
		}

		if (GetUE(bs, u64Val) < 0 || u64Val > 0xFF)
		{
			printf("Invalid pic_parameter_set_id value(%" PRIu64 ").\n", u64Val);
			return -1;
		}

		if (g_NALUnitFilter.first_pps_id > (uint8_t)u64Val)
			g_NALUnitFilter.first_pps_id = (uint8_t)u64Val;

		if (g_NALUnitFilter.last_pps_id < (uint8_t)u64Val)
			g_NALUnitFilter.last_pps_id = (uint8_t)u64Val;

	}
	catch (...)
	{
		printf("Failed to analyze the PPS NAL unit.\n");
		return -1;
	}

	g_vSPS.clear();
	g_vSPS.resize(cbSPSData);
	memcpy(&g_vSPS[0], pSPSData, cbSPSData);

	g_vPPS.clear();
	g_vPPS.resize(cbPPSData);
	memcpy(&g_vPPS[0], pPPSData, cbPPSData);

	return 0;
}

int SetAACAudioSpecificConfig(uint8_t* pASCData, int cbSize)
{
	if (pASCData == nullptr || cbSize < 2)
	{
		printf("The AAC AudioSpecificConfig(%p) is invalid, or cbSize(%d) is less than 2.\n", pASCData, cbSize);
		return -1;
	}

	CBitstream bs(pASCData, cbSize << 3);

	uint8_t audioObjectTypeExt_1 = 0;
	uint8_t audioObjectType_1 = (uint8_t)bs.GetBits(5);
	if (audioObjectType_1 == 31)
		audioObjectTypeExt_1 = (uint8_t)bs.GetBits(6);

	g_AudioSpecificConfig.audioObjectType = (audioObjectType_1 == 31) ? (32 + audioObjectTypeExt_1) : audioObjectType_1;

	g_AudioSpecificConfig.samplingFrequencyIndex = (uint8_t)bs.GetBits(4);
	if (g_AudioSpecificConfig.samplingFrequencyIndex == 0xf)
		g_AudioSpecificConfig.samplingFrequency = (uint32_t)bs.GetBits(24);

	g_AudioSpecificConfig.channelConfiguration = (unsigned char)bs.GetBits(4);

	if (g_AudioSpecificConfig.audioObjectType >= sizeof(Audio_Object_Type_Names) / sizeof(Audio_Object_Type_Names[0]))
	{
		printf("Unsupported AAC audio object Type (%d).\n", g_AudioSpecificConfig.audioObjectType);
		return -1;
	}

	printf("Audio Object Type: %s\n", Audio_Object_Type_Names[g_AudioSpecificConfig.audioObjectType]);
	printf("samplingFrequencyIndex: %d, %s\n", g_AudioSpecificConfig.samplingFrequencyIndex, 
		std::get<1>(samplingFrequencyIndex_names[g_AudioSpecificConfig.samplingFrequencyIndex]));

	if (g_AudioSpecificConfig.samplingFrequencyIndex == 0xf)
		printf("samplingFrequency: %d\n", g_AudioSpecificConfig.samplingFrequency);

	printf("channelConfiguration: %d, %s\n", g_AudioSpecificConfig.channelConfiguration, 
		std::get<2>(channelConfiguration_names[g_AudioSpecificConfig.channelConfiguration]));

	if (g_AudioSpecificConfig.audioObjectType > 4 || g_AudioSpecificConfig.audioObjectType == 0)
	{
		printf("For ADTS header, only AAC Main, AAC LC, AAC SSR and AAC LTP are supported.\n");
		return -1;
	}

	// generate the fixed ADTS header bytes
	g_AudioSpecificConfig.adts_fixed_header_bytes[0]  = 0xFF;
	g_AudioSpecificConfig.adts_fixed_header_bytes[1]  = 0xF1;
	g_AudioSpecificConfig.adts_fixed_header_bytes[2]  = ((g_AudioSpecificConfig.audioObjectType - 1) << 6);
	g_AudioSpecificConfig.adts_fixed_header_bytes[2] |= ((g_AudioSpecificConfig.samplingFrequencyIndex & 0xF) << 2);
	g_AudioSpecificConfig.adts_fixed_header_bytes[2] |= 0;
	g_AudioSpecificConfig.adts_fixed_header_bytes[2] |= ((g_AudioSpecificConfig.channelConfiguration>>2)&0x1);
	g_AudioSpecificConfig.adts_fixed_header_bytes[3]  = ((g_AudioSpecificConfig.channelConfiguration & 0x3) << 6);

	g_AACConfiged = true;

	return 0;
}

int ExtractAVStream(const char* szBrokenMP4File, const char* szH264File, const char* szAACFile)
{
	int iRet = SplitMDATBoxToStreams(szBrokenMP4File, szH264File, szAACFile, g_NALUnitFilter);

	return iRet;
}

#ifdef ENABLE_JNI
JNIEXPORT jint JNICALL Java_com_superlab_ss_Mp4RawDataFixer_setAVCParameters(JNIEnv *env, jobject objThis, jbyteArray sps, jbyteArray pps)
{
	uint8_t *nativeSPSBuffer = NULL, *nativePPSBuffer = NULL;
	int	bufferSPSLen = -1, bufferPPSLen = -1;
	jboolean isCopy;

	if (sps == NULL || cbsps <= 0)
		return -1;

	if (pps == NULL || cbpps <= 0)
		return -1;

	bufferSPSLen = env->GetArrayLength(sps);
	nativeSPSBuffer = (uint8_t*)env->GetPrimitiveArrayCritical(sps, &isCopy);

	bufferPPSLen = env->GetArrayLength(pps);
	nativePPSBuffer = (uint8_t*)env->GetPrimitiveArrayCritical(pps, &isCopy);

	return SetAVCParameters(nativeSPSBuffer, bufferSPSLen, nativePPSBuffer, bufferPPSLen);

}

JNIEXPORT jint JNICALL Java_com_superlab_ss_Mp4RawDataFixer_setAACAudioSpecificConfig(JNIEnv *env, jobject objThis, jbyteArray audioSpecificConfig)
{
	uint8_t *nativeBuffer = NULL;
	int	bufferLen = -1;
	jboolean isCopy;

	if (audioSpecificConfig == NULL || cbAudioSpecificConfig <= 0)
		return -1;

	bufferLen = env->GetArrayLength(audioSpecificConfig);
	nativeBuffer = (uint8_t*)env->GetPrimitiveArrayCritical(sps, &isCopy);

	return SetAACAudioSpecificConfig(nativeBuffer, bufferLen);
}

JNIEXPORT jint JNICALL Java_com_superlab_ss_Mp4RawDataFixer_extractAVStream(JNIEnv *env, jobject objThis, jstring MP4File, jstring H264File, jstring AACFile)
{
	char *utf8MP4Path = NULL, *utf8H264Path = NULL, *utf8AACPath = NULL;

	utf8MP4Path = (char*)env->GetStringUTFChars(MP4File, NULL);
	utf8H264Path = (char*)env->GetStringUTFChars(H264File, NULL);
	utf8AACPath = (char*)env->GetStringUTFChars(AACFile, NULL);

	return ExtractAVStream(utf8MP4Path, utf8H264Path, utf8AACPath);
}
#endif
