#ifndef __DUMP_AV_H__
#define __DUMP_AV_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
	/*!	@brief Set the AVC SPS and PPS data for the broken/un-finalized MP4 file
		@param pSPSData the AVC SPS data buffer
		@param cbSPSData the size of AVC SPS data buffer 
		@param pPPSData the AVC PPS data buffer
		@param cbPPSData the size of AVC PPS data buffer
		@retval  0 Success
		@retval -1 Fail
	*/
	int SetAVCParameters(uint8_t* pSPSData, int cbSPSData, uint8_t* pPPSData, int cbPPSData);

	/*!	@brief Set the AAC AudioSpecificConfig stored in MP4 ES_Descriptor 
		@param pASCData AAC AudioSpecificConfig information
		@param cbSize the buffer size of AAC AudioSpecificConfig information
		@retval  0 Success
		@retval -1 Fail
	*/
	int SetAACAudioSpecificConfig(uint8_t* pASCData, int cbSize);

	/*!	@brief Extract the audio and video stream from the broken/un-finalized MP4 file
		@param szBrokenMP4File the broken/un-finalized MP4 file name
		@param szH264File the file path of H264 annex-b bitstream extracted from broken/un-finalized MP4 file
		@param szAACFile the file path of H264 annex-b bitstream extracted from broken/un-finalized MP4 file
		@retval 0: Success
		@retval -1: Fail
	*/
	int ExtractAVStream(const char* szBrokenMP4File, const char* szH264File, const char* szAACFile);

#ifdef __cplusplus
}
#endif

#endif

