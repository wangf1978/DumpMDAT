#include "DumpAV.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "LibPlatform/platdef.h"

#define MAX_SPS_BUF_SIZE	65536	// 64KB
#define MAX_PPS_BUF_SIZE	65536	// 64KB

int main(int argc, const char* argv[])
{
	if (argc < 3)
	{
		printf("Usage: DumpMDAT mp4_filename h264_filename aac_filename\n");
		return -1;
	}

	// Try to load AVC sps and pps with the same file name
	FILE* fp = nullptr;
	const char* pTmp = strrchr(argv[1], '.');
	char szSPSFilePath[1024], szPPSFilePath[1024], szADTSFilePath[1024];

	memset(szSPSFilePath, 0, sizeof(szSPSFilePath));
	memset(szPPSFilePath, 0, sizeof(szPPSFilePath));
	memset(szADTSFilePath, 0, sizeof(szADTSFilePath));

	if (pTmp == nullptr)
	{
		strcpy(szSPSFilePath, argv[1]);
		strcpy(szPPSFilePath, argv[1]);
		strcpy(szADTSFilePath, argv[1]);
	}
	else
	{
		memcpy(szSPSFilePath, argv[1], pTmp - argv[1]);
		memcpy(szPPSFilePath, argv[1], pTmp - argv[1]);
		memcpy(szADTSFilePath, argv[1], pTmp - argv[1]);
	}

	strcat(szSPSFilePath, ".sps");
	strcat(szPPSFilePath, ".pps");
	strcat(szADTSFilePath, ".adts");

	uint8_t* pSPSBuf = new uint8_t[MAX_SPS_BUF_SIZE];
	uint8_t* pPPSBuf = new uint8_t[MAX_PPS_BUF_SIZE];
	uint8_t bufAudioSpecificConfig[7];

	int cbSPS, cbPPS, cbAudioSpecificConfig;

	//
	// Read AVC SPS file into the buffer
	if (fopen_s(&fp, szSPSFilePath, "rb") != 0 || fp == nullptr)
	{
		printf("Failed to find the AVC SPS file.\n");
		goto done;
	}

	if ((cbSPS = (int)fread(pSPSBuf, 1, MAX_SPS_BUF_SIZE, fp)) <= 0)
	{
		printf("Failed to read data from SPS file.\n");
		goto done;
	}

	fclose(fp);
	fp = nullptr;

	//
	// Read AVC PPS file into the buffer.
	if (fopen_s(&fp, szPPSFilePath, "rb") != 0 || fp == nullptr)
	{
		printf("Failed to find the AVC PPS file.\n");
		goto done;
	}

	if ((cbPPS = (int)fread(pPPSBuf, 1, MAX_SPS_BUF_SIZE, fp)) <= 0)
	{
		printf("Failed to read data from PPS file.\n");
		goto done;
	}

	fclose(fp);
	fp = nullptr;

	//
	// Read ADTS file into the buffer, it may not exist
	if (fopen_s(&fp, szADTSFilePath, "rb") == 0 && fp != nullptr)
	{
		if ((cbAudioSpecificConfig = fread(bufAudioSpecificConfig, 1, sizeof(bufAudioSpecificConfig), fp)) >= 2)
		{
			if (SetAACAudioSpecificConfig(bufAudioSpecificConfig, cbAudioSpecificConfig) < 0)
			{
				printf("Failed to set the ADTS header info.\n");
				goto done;
			}
		}

		fclose(fp);
		fp = nullptr;
	}

	//
	// Set the AVC parameter sets
	if (SetAVCParameters(pSPSBuf, cbSPS, pPPSBuf, cbPPS) < 0)
	{
		printf("Failed to set the AVC parameter set information.\n");
		goto done;
	}

	//
	// Begin extracting the AAC and AVC streams from broken MP4 file
	ExtractAVStream(argv[1], argv[2], argc>3?argv[3]:nullptr);

done:
	if (pSPSBuf)
		delete[] pSPSBuf;

	if (pPPSBuf)
		delete[] pPPSBuf;

	if (fp)
		fclose(fp);

	return 0;
}