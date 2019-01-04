// DumpMDAT.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "AMRingBuffer.h"
#include "Bitstream.h"
#include "NALUnitFilter.h"
#include "aacsyntax.h"

std::vector<uint8_t> g_vSPS;
std::vector<uint8_t> g_vPPS;

NALUnitFilter g_NALUnitFilter;
AudioSpecificConfig g_AudioSpecificConfig;
bool g_AACConfiged = false;
NeAACDecStruct *g_hDecoder = NULL;

#define MAX_AAC_FRAME_SIZE		((2<<13) -1)

using MP4_Boxes_Layout = std::vector<std::tuple<uint32_t/*box type*/, int64_t/*start pos*/, int64_t/*end_pos*/>>;

inline uint8_t quick_log2(uint32_t v)
{
	for (int i = 31; i >= 0; i--)
		if ((v&(1 << i)))
			return i;
	return 0;
}

inline uint8_t quick_ceil_log2(uint32_t v)
{
	uint8_t qlog2_value = quick_log2(v);
	return (1 << qlog2_value) != (int)v ? (qlog2_value + 1) : (qlog2_value);
}

int ListBoxes(FILE* fp, int level, int64_t start_pos, int64_t end_pos, MP4_Boxes_Layout* box_layouts = NULL, int32_t verbose = 0)
{
	int iRet = 0;
	uint8_t box_header[8];
	uint8_t uuid_str[16];
	uint16_t used_size = 0;
	memset(uuid_str, 0, sizeof(uuid_str));

	if (_fseeki64(fp, start_pos, SEEK_SET) != 0 || feof(fp) || start_pos >= end_pos)
		return 0;

	if (fread_s(box_header, sizeof(box_header), 1U, sizeof(box_header), fp) < sizeof(box_header))
		return -1;

	int64_t box_size = ENDIANULONG(*(uint32_t*)(&box_header[0]));
	uint32_t box_type = ENDIANULONG(*(uint32_t*)(&box_header[4]));
	used_size += sizeof(box_header);

	if (box_size == 1)
	{
		if (fread_s(box_header, sizeof(box_header), 1U, sizeof(box_header), fp) < sizeof(box_header))
			return -1;

		box_size = ENDIANUINT64(*(uint64_t*)(&box_header[0]));
		used_size += sizeof(box_header);
	}

	if (box_type == 'uuid')
	{
		if (fread_s(uuid_str, sizeof(uuid_str), 1U, sizeof(uuid_str), fp) < sizeof(uuid_str))
			return -1;

		used_size += sizeof(uuid_str);
	}

	if (box_layouts != NULL)
		box_layouts->push_back({ box_type, start_pos, box_size == 0 ? end_pos : (start_pos + box_size) });

	// print the current box information
	if (verbose > 0)
	{
		for (int i = 0; i < level; i++)
			printf("\t");
		if (box_type == 'uuid')
			printf("uuid[%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X], size: %" PRId64 "\n",
				uuid_str[0x0], uuid_str[0x1], uuid_str[0x2], uuid_str[0x3], uuid_str[0x4], uuid_str[0x5], uuid_str[0x6], uuid_str[0x7],
				uuid_str[0x8], uuid_str[0x9], uuid_str[0xa], uuid_str[0xb], uuid_str[0xc], uuid_str[0xd], uuid_str[0xe], uuid_str[0xf], box_size);
		else
			printf("%c%c%c%c, size: %" PRId64 "\n", (box_type >> 24) & 0xFF, (box_type >> 16) & 0xFF, (box_type >> 8) & 0xFF, box_type & 0xFF, box_size);
	}

	if (box_size == 0)
		return 0;

	start_pos += box_size;

	if (start_pos >= end_pos)
		return 0;

	return ListBoxes(fp, level, start_pos, end_pos, box_layouts, verbose);
}

int LocateMDATBox(FILE* fp, int64_t& nMDATBoxStart, int64_t& nMDATBoxRealSize)
{
	int iRet = -1;
	if (fp == nullptr)
		return -1;

	int64_t s = 0, /*e = 0, */file_size;

	_fseeki64(fp, 0, SEEK_END);
	file_size = _ftelli64(fp);
	_fseeki64(fp, 0, SEEK_SET);

	MP4_Boxes_Layout mp4_boxes_layout;

	// At first visit all boxes in MP4 file, and generated a table of boxes
	if ((iRet = ListBoxes(fp, 0, 0, file_size, &mp4_boxes_layout, 1)) == -1)
	{
		goto done;
	}

	for (auto& box_layout : mp4_boxes_layout)
	{
		if (std::get<0>(box_layout) == 'mdat')
		{
			nMDATBoxStart = std::get<1>(box_layout);
			nMDATBoxRealSize = std::get<2>(box_layout) - nMDATBoxStart;

			if (nMDATBoxRealSize < 0)
				nMDATBoxRealSize = file_size - nMDATBoxStart;

			iRet = 0;
			break;
		}
	}

done:
	return iRet;
}

/*!	@brief Verify the current buffer is a complete AAC raw_data_block or not. 
	@retval <0 invalid raw data buffer
	@retval  0 insufficient buffer for an AAC raw_data_block
	@retval others the size of AAC raw_data_block */
int VerifyAACRawDataBlock(uint8_t* pBuf, int cbBuf, bool bDrain)
{
	int iRet = -1;
	CBitstream bs(pBuf, cbBuf << 3);

	NeAACDecStruct* hDecoder = g_hDecoder;
	NeAACDecFrameInfo frameInfo;
	drc_info drc;

	hDecoder->fr_channels = 0;
	hDecoder->fr_ch_ele = 0;
	hDecoder->first_syn_ele = 25;
	hDecoder->has_lfe = 0;

	if (!bDrain && cbBuf < MAX_AAC_FRAME_SIZE)
		return 0;

	try
	{
		raw_data_block(hDecoder, &frameInfo, bs, &hDecoder->pce, &drc);

		if (frameInfo.error > 0)
		{
			printf("[AAC] Failed to extract a raw_data_block {error: %d}.\n", frameInfo.error);
			return -1;
		}

		uint64_t left_bits = 0;
		uint64_t nPos = bs.Tell(&left_bits);
		if (((nPos + 7) >> 3) > MAX_AAC_FRAME_SIZE)
			iRet = -1;
		else
		{
			iRet = (int)((nPos + 7) >> 3);
			printf("[AAC] Found an AAC raw_data_block, raw_data_block_size: %d.\n", iRet);
		}	
	}
	catch (...)
	{
		return -1;
	}

	return iRet;
}

int CommitAACBuffer(AMLinearRingBuffer& lrb_a, uint8_t* pStart, uint8_t* pEnd, FILE* afp, bool bDrain)
{
	int iRet = -1;
	// Copy the buffer the linear buffer
	int cbWriteBuf = 0, cbSize = 0;
	uint8_t* pStartBuf = NULL, *pBuf = NULL, *pCurParseStartBuf = NULL;

	if (pEnd <= pStart && !bDrain)
		return -1;

	if (g_AACConfiged == false)
		return -1;

	do
	{
		uint8_t* pWriteBuf = AM_LRB_GetWritePtr(lrb_a, &cbWriteBuf);

		if (pWriteBuf == nullptr || cbWriteBuf < (int)(pEnd - pStart))
		{
			AM_LRB_Reform(lrb_a);
			if ((pWriteBuf = AM_LRB_GetWritePtr(lrb_a, &cbWriteBuf)) == nullptr)
			{
				return -1;
			}
		}

		if (pEnd > pStart)
		{
			int nCpy = cbWriteBuf;
			if (nCpy > (int)(pEnd - pStart))
				nCpy = (int)(pEnd - pStart);

			memcpy(pWriteBuf, pStart, nCpy);

			pStart += nCpy;

			AM_LRB_SkipWritePtr(lrb_a, nCpy);
		}

		if ((pStartBuf = AM_LRB_GetReadPtr(lrb_a, &cbSize)) == NULL || cbSize <= 0)
			return -1;

		pBuf = pCurParseStartBuf = pStartBuf;

		while (cbSize > 0)
		{
			int cbRawDataBlockSize = -1;
			if ((cbRawDataBlockSize = VerifyAACRawDataBlock(pBuf, cbSize, bDrain)) < 0)
			{
				pBuf++;
				cbSize--;
			}
			else if (cbRawDataBlockSize == 0)
			{
				// Need more data
				break;
			}
			else
			{
				// Write the raw_data_block to the file
				uint8_t adts_header_bytes[7];
				memcpy(adts_header_bytes, g_AudioSpecificConfig.adts_fixed_header_bytes, 4);
				adts_header_bytes[3] |= ((cbRawDataBlockSize + 7) >> 11) & 0x3;
				adts_header_bytes[4] = ((cbRawDataBlockSize + 7) >> 3) & 0xFF;
				adts_header_bytes[5] = (((cbRawDataBlockSize + 7) & 0x7) << 5) | 0x1F;
				adts_header_bytes[6] = 0xFC;

				if (afp != nullptr)
				{
					if (fwrite(adts_header_bytes, 1, sizeof(adts_header_bytes), afp) != sizeof(adts_header_bytes))
					{
						printf("Failed to write the ADTS header to audio stream file.\n");
						return -1;
					}

					if (fwrite(pBuf, 1, cbRawDataBlockSize, afp) != (size_t)cbRawDataBlockSize)
					{
						printf("Failed to write the raw_data_block to audio stream file.\n");
						return -1;
					}
				}

				pBuf += cbRawDataBlockSize;
				cbSize -= cbRawDataBlockSize;
			}
		}

		AM_LRB_SkipReadPtr(lrb_a, (int)(pBuf - pCurParseStartBuf));
		pCurParseStartBuf = pBuf;

	} while (pEnd > pStart);

	return -1;
}

int GetUE(CBitstream& bs, uint64_t& val)
{
	int leadingZeroBits = -1;
	for (bool b = false; !b; leadingZeroBits++)
		b = bs.GetBits(1) ? true : false;

	if (leadingZeroBits >= 64)
		return -1;

	val = (1LL << leadingZeroBits) - 1LL + bs.GetBits(leadingZeroBits);
	return 0;
}

int GetSE(CBitstream& bs, int64_t& val)
{
	int leadingZeroBits = -1;
	for (bool b = false; !b; leadingZeroBits++)
		b = bs.GetBits(1) ? true : false;

	if (leadingZeroBits >= 64)
		throw - 1;

	uint64_t codeNum = (1LL << leadingZeroBits) - 1LL + bs.GetBits(leadingZeroBits);

	val = (int64_t)(codeNum % 2 ? ((codeNum >> 1) + 1) : (~(codeNum >> 1) + 1));
	return 0;
}

int ConvertEBSP2RBSP(uint8_t* pEBSP, int cbEBSP, uint8_t* pRBSP, int cbRBSP)
{
	int NumBytesInRBSP = 0;
	int nalUnitHeaderBytes = 1;
	uint8_t* s = pEBSP;
	uint8_t* d = pRBSP;

	if (pEBSP == nullptr || cbEBSP <= 0)
	{
		printf("EBSP data is invalid.\n");
		return -1;
	}

	if (pRBSP == nullptr || cbRBSP <= 0)
	{
		printf("RBSP data is invalid.\n");
		return -1;
	}

	uint8_t nal_unit_type = (*s++) & 0x1F;

	if (nal_unit_type == 14 || nal_unit_type == 20 || nal_unit_type == 21)
	{
		printf("Does not support SVC extension or 3D extension NAL unit.\n");
		return -1;
	}

	for (int i = nalUnitHeaderBytes; i < cbEBSP; i++)
	{
		if (i + 2 < cbEBSP && *s == 0 && *(s + 1) == 0 && *(s + 2) == 3)
		{
			if (NumBytesInRBSP + 2 > cbRBSP)
			{
				printf("Insufficient rbsp buffer.\n");
				return -1;
			}

			*d++ = *s++;
			*d++ = *s++;
			i += 2;
			NumBytesInRBSP += 2;
			s++;	// Skip emulation_prevention_three_byte
		}
		else
		{
			if (NumBytesInRBSP + 1 > cbRBSP)
			{
				printf("Insufficient rbsp buffer.\n");
				return -1;
			}

			*d++ = *s++;
			NumBytesInRBSP++;
		}
	}

	return NumBytesInRBSP;
}

inline bool IsTrailByte(uint8_t b)
{
	int num_1_bit = 0;
	for (int i = 0; i < 8; i++)
		if (((b >> i) & 1))
			num_1_bit++;

	return num_1_bit == 1 ? true : false;
}

int VerifyPPSRBSP(CBitstream& bs)
{
	uint64_t u64Val = UINT64_MAX;
	try
	{
		uint8_t pic_parameter_set_id, seq_parameter_set_id, entropy_coding_mode_flag, bottom_field_pic_order_in_frame_present_flag, num_slice_groups_minus1;

		if (GetUE(bs, u64Val) < 0 || u64Val > 0xFF)
		{
			printf("Invalid pic_parameter_set_id value(%" PRIu64 ").\n", u64Val);
			return -1;
		}
		pic_parameter_set_id = (uint8_t)u64Val;

		if (GetUE(bs, u64Val) < 0 || u64Val > 0xFF)
		{
			printf("Invalid seq_parameter_set_id value(%" PRIu64 ").\n", u64Val);
			return -1;
		}
		seq_parameter_set_id = (uint8_t)u64Val;

		if (seq_parameter_set_id < g_NALUnitFilter.first_sps_id ||
			seq_parameter_set_id > g_NALUnitFilter.last_sps_id)
		{
			printf("seq_parameter_set_id(%d) is unexpected.\n", seq_parameter_set_id);
			return -1;
		}

		entropy_coding_mode_flag = (uint8_t)bs.GetBits(1);
		bottom_field_pic_order_in_frame_present_flag = (uint8_t)bs.GetBits(1);

		if (g_NALUnitFilter.profile_idc == 66 && entropy_coding_mode_flag != 0)
		{
			printf("Picture parameter sets shall have entropy_coding_mode_flag equal to 0.\n");
			return -1;
		}

		if (GetUE(bs, u64Val) < 0 || u64Val > 7)
		{
			printf("Invalid seq_parameter_set_id value(%" PRIu64 ").\n", u64Val);
			return -1;
		}
		num_slice_groups_minus1 = (uint8_t)u64Val;

		uint8_t slice_group_map_type;
		if (num_slice_groups_minus1 > 0)
		{
			if (GetUE(bs, u64Val) < 0 || u64Val > 6)
			{
				printf("Invalid num_slice_groups_minus1 value(%" PRIu64 ").\n", u64Val);
				return -1;
			}
			slice_group_map_type = (uint8_t)u64Val;

			if (slice_group_map_type == 0)
			{
				for (uint8_t iGroup = 0; iGroup <= num_slice_groups_minus1; iGroup++)
				{
					if (GetUE(bs, u64Val) < 0 || u64Val > 4096)
					{
						printf("Invalid run_length_minus1 value(%" PRIu64 ").\n", u64Val);
						return -1;
					}
				}
			}
			else if (slice_group_map_type == 2)
			{
				for (uint8_t iGroup = 0; iGroup <= num_slice_groups_minus1; iGroup++)
				{
					uint64_t top_left, bottom_right;
					if (GetUE(bs, top_left) < 0 || top_left > 4096)
					{
						printf("Invalid top_left[%d] value(%" PRIu64 ").\n", iGroup, top_left);
						return -1;
					}

					if (GetUE(bs, bottom_right) < 0 || bottom_right > 4096)
					{
						printf("Invalid bottom_right[%d] value(%" PRIu64 ").\n", iGroup, bottom_right);
						return -1;
					}

					if (top_left > bottom_right)
					{
						return -1;
					}
				}
			}
			else if (slice_group_map_type >= 3 && slice_group_map_type <= 5)
			{
				uint8_t slice_group_change_direction_flag = (uint8_t)bs.GetBits(1);
				if (GetUE(bs, u64Val) < 0 || u64Val > 4096)
				{
					printf("Invalid slice_group_change_rate_minus1 value(%" PRIu64 ").\n", u64Val);
					return -1;
				}
			}
			else if (slice_group_map_type == 6)
			{
				if (GetUE(bs, u64Val) < 0 || u64Val > 4096)
				{
					printf("Invalid pic_size_in_map_units_minus1 value(%" PRIu64 ").\n", u64Val);
					return -1;
				}

				uint8_t v = quick_ceil_log2(num_slice_groups_minus1 + 1);
				for (int i = 0; i < (int)u64Val; i++)
				{
					uint32_t slice_group_id = (uint32_t)bs.GetBits(v);
				}
			}
		}

		uint8_t num_ref_idx_l0_default_active_minus1, num_ref_idx_l1_default_active_minus1;
		uint8_t weighted_pred_flag, weighted_bipred_idc;

		if (GetUE(bs, u64Val) < 0 || u64Val > 31)
		{
			printf("Invalid num_ref_idx_l0_default_active_minus1 value(%" PRIu64 ").\n", u64Val);
			return -1;
		}

		num_ref_idx_l0_default_active_minus1 = (uint8_t)u64Val;

		if (GetUE(bs, u64Val) < 0 || u64Val > 31)
		{
			printf("Invalid num_ref_idx_l1_default_active_minus1 value(%" PRIu64 ").\n", u64Val);
			return -1;
		}

		num_ref_idx_l1_default_active_minus1 = (uint8_t)u64Val;

		weighted_pred_flag = (uint8_t)bs.GetBits(1);
		weighted_bipred_idc = (uint8_t)bs.GetBits(2);

		int64_t i64Val = INT64_MIN;
		if (GetSE(bs, i64Val) < 0 || i64Val > 25)
		{
			printf("Invalid pic_init_qp_minus26 value(%" PRId64 ").\n", i64Val);
			return -1;
		}

		if (GetSE(bs, i64Val) < 0 || i64Val > 25 || i64Val < -26)
		{
			printf("Invalid pic_init_qs_minus26 value(%" PRId64 ").\n", i64Val);
			return -1;
		}

		if (GetSE(bs, i64Val) < 0 || i64Val > 12 || i64Val < -12)
		{
			printf("Invalid chroma_qp_index_offset value(%" PRIu64 ").\n", i64Val);
			return -1;
		}

		bs.SkipBits(3);

		uint64_t left_bits = 0;
		bs.Tell(&left_bits);

		if (left_bits > 8)
		{
			if (g_NALUnitFilter.profile_idc == 66)
			{
				printf("For baseline profile, The syntax elements transform_8x8_mode_flag, pic_scaling_matrix_present_flag, and second_chroma_qp_index_offset shall not be present in picture parameter sets.\n");
				return -1;
			}

			uint8_t transform_8x8_mode_flag = (uint8_t)bs.GetBits(1);
			uint8_t pic_scaling_matrix_present_flag = (uint8_t)bs.GetBits(1);

			if (!pic_scaling_matrix_present_flag)
			{
				bs.Tell(&left_bits);
				if ((left_bits + 7) / 8 > 2)
					return -1;
			}
		}

	}
	catch (...)
	{
		return -1;
	}

	return 0;
}

int VerifySPSRBSP(CBitstream& bs)
{
	uint64_t u64Val = UINT64_MAX;
	try
	{
		uint8_t profile_idc, constraint_set_bits, level_idc;
		uint8_t seq_parameter_set_id, chroma_format_idc = 1, separate_colour_plane_flag = 0;
		uint8_t bit_depth_luma_minus8 = 0, bit_depth_chroma_minus8 = 0, qpprime_y_zero_transform_bypass_flag = 0, seq_scaling_matrix_present_flag = 0;
		uint8_t log2_max_frame_num_minus4, pic_order_cnt_type, log2_max_pic_order_cnt_lsb_minus4, delta_pic_order_always_zero_flag;
		uint8_t max_num_ref_frames, gaps_in_frame_num_value_allowed_flag;
		uint16_t pic_width_in_mbs_minus1, pic_height_in_map_units_minus1;
		uint8_t frame_mbs_only_flag, mb_adaptive_frame_field_flag, direct_8x8_inference_flag, frame_cropping_flag;

		profile_idc = (uint8_t)bs.GetByte();
		constraint_set_bits = (uint8_t)bs.GetByte();
		level_idc = (uint8_t)bs.GetByte();

		if (GetUE(bs, u64Val) < 0 || u64Val > 0xFF)
		{
			printf("Invalid seq_parameter_set_id value(%" PRIu64 ").\n", u64Val);
			return -1;
		}
		seq_parameter_set_id = (uint8_t)u64Val;

		if (profile_idc == 100 || profile_idc == 110 ||
			profile_idc == 122 || profile_idc == 244 || profile_idc == 44 ||
			profile_idc == 83 || profile_idc == 86 || profile_idc == 118 ||
			profile_idc == 128 || profile_idc == 138 || profile_idc == 139 ||
			profile_idc == 134 || profile_idc == 135)
		{
			if (GetUE(bs, u64Val) < 0 || u64Val > 3)
			{
				printf("Invalid chroma_format_idc value (%" PRIu64 ").\n", u64Val);
				return -1;
			}
			chroma_format_idc = (uint8_t)u64Val;

			if (chroma_format_idc == 3)
				separate_colour_plane_flag = bs.GetBits(1) ? 1 : 0;

			if (GetUE(bs, u64Val) < 0 || u64Val > 6)
			{
				printf("Invalid bit_depth_luma_minus8 value(%" PRIu64 ").\n", u64Val);
				return -1;
			}
			bit_depth_luma_minus8 = (uint8_t)u64Val;

			if (GetUE(bs, u64Val) < 0 || u64Val > 6)
			{
				printf("Invalid bit_depth_chroma_minus8 value(%" PRIu64 ").\n", u64Val);
				return -1;
			}
			bit_depth_chroma_minus8 = (uint8_t)u64Val;

			qpprime_y_zero_transform_bypass_flag = (uint8_t)bs.GetBits(1);	// Skip qpprime_y_zero_transform_bypass_flag
			if ((seq_scaling_matrix_present_flag = (uint8_t)bs.GetBits(1)))	// seq_scaling_matrix_present_flag
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
		log2_max_frame_num_minus4 = (uint8_t)u64Val;

		if (GetUE(bs, u64Val) < 0 || u64Val > 2)
		{
			printf("Invalid pic_order_cnt_type value(%" PRIu64 ").\n", u64Val);
			return -1;
		}
		pic_order_cnt_type = (uint8_t)u64Val;

		if (pic_order_cnt_type == 0)
		{
			if (GetUE(bs, u64Val) < 0 || u64Val > 12)
			{
				printf("Invalid log2_max_pic_order_cnt_lsb_minus4 value(%" PRIu64 ").\n", u64Val);
				return -1;
			}
			log2_max_pic_order_cnt_lsb_minus4 = (uint8_t)u64Val;
		}
		else if (pic_order_cnt_type == 1)
		{
			delta_pic_order_always_zero_flag = (uint8_t)bs.GetBits(1);

			int64_t i64Val;
			if (GetSE(bs, i64Val) < 0)
			{
				printf("Invalid offset_for_non_ref_pic value(%" PRId64 ").\n", i64Val);
				return -1;
			}

			if (GetSE(bs, i64Val) < 0)
			{
				printf("Invalid offset_for_non_ref_pic value(%" PRId64 ").\n", i64Val);
				return -1;
			}

			if (GetUE(bs, u64Val) < 0 || u64Val > 0xFF)
			{
				printf("Invalid num_ref_frames_in_pic_order_cnt_cycle value(%" PRIu64 ").\n", u64Val);
				return -1;
			}

			for (int i = 0; i < (int)u64Val; i++)
			{
				if (GetSE(bs, i64Val) < 0)
				{
					printf("Invalid offset_for_non_ref_pic value(%" PRId64 ").\n", i64Val);
					return -1;
				}
			}
		}

		if (GetUE(bs, u64Val) < 0 || u64Val > 0xFF)
		{
			printf("Invalid max_num_ref_frames value(%" PRIu64 ").\n", u64Val);
			return -1;
		}
		max_num_ref_frames = (uint8_t)u64Val;

		gaps_in_frame_num_value_allowed_flag = (uint8_t)bs.GetBits(1);

		if (GetUE(bs, u64Val) < 0 || u64Val > 4096/16 -1)
		{
			printf("Invalid pic_width_in_mbs_minus1 value(%" PRIu64 ").\n", u64Val);
			return -1;
		}
		pic_width_in_mbs_minus1 = (uint16_t)u64Val;

		if (GetUE(bs, u64Val) < 0 || u64Val > 4096 / 16 - 1)
		{
			printf("Invalid pic_height_in_map_units_minus1 value(%" PRIu64 ").\n", u64Val);
			return -1;
		}
		pic_height_in_map_units_minus1 = (uint16_t)u64Val;

		frame_mbs_only_flag = (uint8_t)bs.GetBits(1);

		if (!frame_mbs_only_flag)
		{
			mb_adaptive_frame_field_flag = (uint8_t)bs.GetBits(1);
		}

		direct_8x8_inference_flag = (uint8_t)bs.GetBits(1);
		frame_cropping_flag = (uint8_t)bs.GetBits(1);
	}
	catch (...)
	{
		return -1;
	}

	return 0;
}

int CommitAVCNALUnit(uint8_t* pNalUnitBuf, int nal_unit_buf_size, FILE* vfp)
{
	static bool s_bFirstAccessUnit = true;
	static int s_prev_pic_parameter_set_id = -1;
	static bool s_bSPSFound = false;
	static bool s_bPPSFound = false;
	static int s_nConsequentIDRPicIDCount = 0;

	uint8_t four_bytes_start_prefixes[4] = { 0, 0, 0, 1 };
	uint8_t three_bytes_start_prefixes[3] = { 0, 0, 1 };

	if (pNalUnitBuf == nullptr || nal_unit_buf_size <= 0)
		return -1;

	uint8_t nal_unit_type = pNalUnitBuf[0] & 0x1F;

	if (nal_unit_type == 9)
	{
		s_bFirstAccessUnit = true;
	}

	if (nal_unit_type == 1 || nal_unit_type == 2 || nal_unit_type == 5)
	{
		// Skip the header part
		CBitstream bs(pNalUnitBuf, nal_unit_buf_size << 3);

		uint32_t first_mb_in_slice;
		uint8_t slice_type;
		uint8_t pic_parameter_set_id;
		uint32_t frame_num;
		uint16_t idr_pic_id = 0;

		bs.SkipBits(8);	// Skip nal_unit_type

		uint64_t u64Tmp = UINT64_MAX;
		if (GetUE(bs, u64Tmp) < 0 || u64Tmp > UINT32_MAX)
		{
			printf("Failed to get the field first_mb_in_slice(%" PRIu64 ") from slice_header().\n", u64Tmp);
			return -1;
		}

		first_mb_in_slice = (uint32_t)u64Tmp;

		u64Tmp = UINT64_MAX;
		if (GetUE(bs, u64Tmp) < 0 || u64Tmp > 9)
		{
			printf("Failed to get the field slice_type(%" PRIu64 ") from slice_header().\n", u64Tmp);
			return -1;
		}
		slice_type = (uint8_t)u64Tmp;

		u64Tmp = UINT64_MAX;
		if (GetUE(bs, u64Tmp) < 0 || u64Tmp > UINT8_MAX)
		{
			printf("Failed to get the field pic_parameter_set_id(%" PRIu64 ") from slice_header().\n", u64Tmp);
			return -1;
		}
		pic_parameter_set_id = (uint8_t)u64Tmp;

		if (pic_parameter_set_id < g_NALUnitFilter.first_pps_id || pic_parameter_set_id > g_NALUnitFilter.last_pps_id)
		{
			printf("Invalid pic_parameter_set_id(%d) in the slice_header().\n", pic_parameter_set_id);
			return -1;
		}

		frame_num = (uint8_t)bs.GetBits(g_NALUnitFilter.log2_max_frame_num_minus4 + 4);

		uint8_t field_pic_flag = 0, bottom_field_flag = 0;
		if (!g_NALUnitFilter.frame_mbs_only_flag)
		{
			field_pic_flag = (uint8_t)bs.GetBits(1);
			if (field_pic_flag)
				bottom_field_flag = (uint8_t)bs.GetBits(1);
		}

		if (nal_unit_type == 5)
		{
			u64Tmp = UINT64_MAX;
			if (GetUE(bs, u64Tmp) < 0 || u64Tmp > UINT16_MAX)
			{
				printf("Failed to get the field idr_pic_id(%" PRIu64 ") from slice_header().\n", u64Tmp);
				return -1;
			}

			idr_pic_id = (uint16_t)u64Tmp;
			printf("idr_pic_id: %d\n", idr_pic_id);
		}

		uint16_t pic_order_cnt_lsb = 0;
		if (g_NALUnitFilter.pic_order_cnt_type == 0)
		{
			pic_order_cnt_lsb = (uint16_t)bs.GetBits(g_NALUnitFilter.log2_max_pic_order_cnt_lsb_minus4 + 4);
			//if (g_NALUnitFilter.)
		}

		if (nal_unit_type == 5)
		{
			if (g_NALUnitFilter.prev_idr_pic_id >= 0 && (g_NALUnitFilter.prev_idr_pic_id + 1) % 0xFFFF == idr_pic_id)
			{
				s_nConsequentIDRPicIDCount++;
				if (s_nConsequentIDRPicIDCount > 3)
					g_NALUnitFilter.consequent_idr_pic_id = true;
			}
			else if(g_NALUnitFilter.prev_idr_pic_id >= 0 && g_NALUnitFilter.consequent_idr_pic_id)
			{
				printf("The idr_pic_id is expected to be the same.\n");
				return -1;
			}

			g_NALUnitFilter.prev_idr_pic_id = idr_pic_id;
		}

		// Everything seems to be ok
		if (s_bFirstAccessUnit == false)
		{
			if (frame_num != (g_NALUnitFilter.prev_frame_num + 1) % (1UL<<(g_NALUnitFilter.log2_max_frame_num_minus4 + 4)) ||
				pic_parameter_set_id != s_prev_pic_parameter_set_id)
				s_bFirstAccessUnit = true;
		}

		s_prev_pic_parameter_set_id = pic_parameter_set_id;
		g_NALUnitFilter.prev_frame_num = frame_num;
	}
	else if (nal_unit_type == 6)
	{
		// The last byte should be 0x80
		if (pNalUnitBuf[nal_unit_buf_size - 1] != 0x80)
			return -1;

#if 0
		CBitstream bs(pNalUnitBuf, nal_unit_buf_size << 3);

		try {
			int sei_rbsp_size = 1;
			bs.SkipBits(8);

			do
			{
				uint64_t payloadType;
				uint8_t valByte = 0;
				while ((valByte = bs.GetByte()) == 0xFF)
				{
					payloadType += 255;
					sei_rbsp_size++;
				}
				payloadType += valByte;
				sei_rbsp_size++;

				uint64_t payloadSize = 0;
				while ((valByte = bs.GetByte()) == 0xFF)
				{
					payloadSize += 255;
					sei_rbsp_size++;
				}
				payloadSize += valByte;
				sei_rbsp_size++;



			} while (sei_rbsp_size < nal_unit_buf_size);
		}
		catch (...)
		{
			return -1;
		}
#endif
	}

	if (nal_unit_type == 7)
	{
		int cbRBSP = 0;
		if ((cbRBSP = ConvertEBSP2RBSP(pNalUnitBuf, nal_unit_buf_size, g_NALUnitFilter.non_VCL_rbsp, sizeof(g_NALUnitFilter.non_VCL_rbsp))) <= 0)
		{
			printf("Failed to convert EBSP to rbsp.\n");
			return -1;
		}

		CBitstream bs(g_NALUnitFilter.non_VCL_rbsp, cbRBSP << 3);
		if (VerifySPSRBSP(bs) < 0)
		{
			printf("The SPS rbsp seems to be corrupted.\n");
			return -1;
		}
		
		s_bSPSFound = true;
	}

	if (nal_unit_type == 8)
	{
		int cbRBSP = 0;
		if ((cbRBSP = ConvertEBSP2RBSP(pNalUnitBuf, nal_unit_buf_size, g_NALUnitFilter.non_VCL_rbsp, sizeof(g_NALUnitFilter.non_VCL_rbsp))) <= 0)
		{
			printf("Failed to convert EBSP to rbsp.\n");
			return -1;
		}

		CBitstream bs(g_NALUnitFilter.non_VCL_rbsp, cbRBSP << 3);
		if (VerifyPPSRBSP(bs) < 0)
		{
			printf("The PPS rbsp seems to be corrupted.\n");
			return -1;
		}

		s_bPPSFound = true;
	}

	if (!s_bSPSFound)
	{
		// Save to the file
		fwrite(four_bytes_start_prefixes, 1, sizeof(four_bytes_start_prefixes), vfp);
		fwrite(&g_vSPS[0], 1, g_vSPS.size(), vfp);
		s_bFirstAccessUnit = false;
		s_bSPSFound = true;
	}

	if (!s_bPPSFound)
	{
		// Save to the file
		fwrite(four_bytes_start_prefixes, 1, sizeof(four_bytes_start_prefixes), vfp);
		fwrite(&g_vPPS[0], 1, g_vPPS.size(), vfp);
		s_bFirstAccessUnit = false;
		s_bPPSFound = true;
	}

	if (s_bFirstAccessUnit || nal_unit_type == 7 || nal_unit_type == 8)
	{
		fwrite(four_bytes_start_prefixes, 1, sizeof(four_bytes_start_prefixes), vfp);
		s_bFirstAccessUnit = false;
	}
	else
	{
		fwrite(three_bytes_start_prefixes, 1, sizeof(three_bytes_start_prefixes), vfp);
	}

	fwrite(pNalUnitBuf, 1, nal_unit_buf_size, vfp);

	return 0;
}

drc_info *drc_init(real_t cut, real_t boost)
{
	drc_info *drc = (drc_info*)malloc(sizeof(drc_info));
	memset(drc, 0, sizeof(drc_info));

	drc->ctrl1 = cut;
	drc->ctrl2 = boost;

	drc->num_bands = 1;
	drc->band_top[0] = 1024 / 4 - 1;
	drc->dyn_rng_sgn[0] = 1;
	drc->dyn_rng_ctl[0] = 0;

	return drc;
}

void drc_end(drc_info *drc)
{
	if (drc) free(drc);
}

void InitAACSyntaxParser()
{
	if (g_AACConfiged == false)
	{
		printf("[AAC] AudioSpecificConfig is not specified, don't initialize and config AAC syntax parser.\n");
		return;
	}

	if (g_hDecoder != nullptr)
	{
		delete g_hDecoder;
		g_hDecoder = nullptr;
	}

	g_hDecoder = new NeAACDecStruct();
	memset(g_hDecoder, 0, sizeof(NeAACDecStruct));

	NeAACDecStruct* hDecoder = g_hDecoder;

	const unsigned char mes[] = { 0x67,0x20,0x61,0x20,0x20,0x20,0x6f,0x20,0x72,0x20,0x65,0x20,0x6e,0x20,0x20,0x20,0x74,0x20,0x68,0x20,0x67,0x20,0x69,0x20,0x72,0x20,0x79,0x20,0x70,0x20,0x6f,0x20,0x63 };

	hDecoder->cmes = mes;
	hDecoder->config.outputFormat = FAAD_FMT_16BIT;
	hDecoder->config.defObjectType = MAIN;
	hDecoder->config.defSampleRate = 44100; /* Default: 44.1kHz */
	hDecoder->config.downMatrix = 0;
	hDecoder->adts_header_present = 0;
	hDecoder->adif_header_present = 0;
	hDecoder->latm_header_present = 0;
#ifdef ERROR_RESILIENCE
	hDecoder->aacSectionDataResilienceFlag = 0;
	hDecoder->aacScalefactorDataResilienceFlag = 0;
	hDecoder->aacSpectralDataResilienceFlag = 0;
#endif
	hDecoder->frameLength = 1024;

	hDecoder->frame = 0;
	hDecoder->sample_buffer = NULL;

	hDecoder->__r1 = 1;
	hDecoder->__r2 = 1;

	for (uint8_t i = 0; i < MAX_CHANNELS; i++)
	{
		hDecoder->window_shape_prev[i] = 0;
		hDecoder->time_out[i] = NULL;
		hDecoder->fb_intermed[i] = NULL;
#ifdef SSR_DEC
		hDecoder->ssr_overlap[i] = NULL;
		hDecoder->prev_fmd[i] = NULL;
#endif
#ifdef MAIN_DEC
		hDecoder->pred_stat[i] = NULL;
#endif
#ifdef LTP_DEC
		hDecoder->ltp_lag[i] = 0;
		hDecoder->lt_pred_stat[i] = NULL;
#endif
	}

#ifdef SBR_DEC
	for (i = 0; i < MAX_SYNTAX_ELEMENTS; i++)
	{
		hDecoder->sbr[i] = NULL;
	}
#endif

	hDecoder->drc = drc_init(REAL_CONST(1.0), REAL_CONST(1.0));

	hDecoder->sf_index = g_AudioSpecificConfig.samplingFrequencyIndex;
	hDecoder->object_type = g_AudioSpecificConfig.audioObjectType;
	hDecoder->channelConfiguration = g_AudioSpecificConfig.channelConfiguration > 6 ? 2 : g_AudioSpecificConfig.channelConfiguration;
}

void UninitAACSyntaxParser()
{
	if (g_hDecoder != nullptr)
	{
		delete g_hDecoder;
		g_hDecoder = nullptr;
	}
}

/*
|_______________________________________________________________|
\_____________ The suspicious NAL Unit buffer, commit the buffer ahead the start pointer

*/

int SplitMDATBoxToStreams(const char* szMP4FilePath, const char* szH264File, const char* szAACFile, NALUnitFilter& filter)
{
	int iRet = -1;
	const int read_unit_size = 2048;
	const int minimum_parse_buffer_size = 5;	// 4 NAL length plus 1 NAL header
	int slice_id_of_data_partition_a = -1, slice_id_of_data_partition_b = -1, slice_id_of_data_partition_c = -1;
	int cbSize = 0;
	bool part_picture_nal_ref_idc_is_zero = false;
	uint8_t* pStartBuf = NULL, *pBuf = NULL, *pCurParseStartBuf = NULL;
	uint64_t cur_byte_pos, cur_start_file_pos;
	int32_t cur_AVC_NAL_size = -1;
	int64_t nFoundNALUnits = 0;
	bool bEOF = false;
	uint8_t buf[8];
	AMLinearRingBuffer lrb_nv = nullptr, lrb = nullptr;
	uint64_t mdat_start, mdat_size;

#ifndef _WIN32
	const char* path_seperator = "/";
#else
	const char* path_seperator = "\\";
#endif

	if (szMP4FilePath == nullptr)
	{
		printf("Please specify a MP4 file name.\n");
		return -1;
	}

	FILE* fp = NULL, *vfp = nullptr, *afp = nullptr;
	if (fopen_s(&fp, szMP4FilePath, "rb") != 0 || fp == nullptr)
		return -1;

	int64_t mdat_box_byte_start = -1, mdat_box_real_size = -1;
	if (LocateMDATBox(fp, mdat_box_byte_start, mdat_box_real_size) < 0)
	{
		fclose(fp);
		return -1;
	}

	if (_fseeki64(fp, mdat_box_byte_start, SEEK_SET) != 0)
	{
		fclose(fp);
		return -1;
	}

	// The linear ring-buffer which is for non-AVC-NAL-Unit
	lrb_nv = AM_LRB_Create(filter.nMaxAACRawDataBlockSize);
	lrb = AM_LRB_Create(filter.nMaxNALUnitSize);

	// get the actual size and actual start position of raw data
	if (fread(buf, 1, 8, fp) < 8)
	{
		printf("Failed to get 4 bytes of 'size' field.\n");
		goto done;
	}

	mdat_start = mdat_box_byte_start;
	mdat_size = *((uint32_t*)buf);
	ULONG_FIELD_ENDIAN(mdat_size);

	mdat_start += 8;

	if (mdat_size == 0)
		mdat_size = UINT64_MAX;
	else if (mdat_size == 1)
	{
		if (fread(buf, 1, 8, fp) < 8)
		{
			printf("Failed to get 8 bytes of 'largesize' field.\n");
			goto done;
		}

		mdat_start += 8;
		mdat_size = *((uint64_t*)buf);
		UINT64_FIELD_ENDIAN(mdat_size);
	}

	if (fopen_s(&vfp, szH264File, "wb") != 0 || vfp == nullptr)
	{
		printf("Failed to open the video dump file: %s\n", szH264File);
		goto done;
	}

	if (szAACFile != nullptr)
	{
		if (fopen_s(&afp, szAACFile, "wb") != 0 || afp == nullptr)
		{
			printf("Failed to open the audio dump file: %s\n", szAACFile);
			goto done;
		}
	}

	InitAACSyntaxParser();

	cur_start_file_pos = cur_byte_pos = mdat_start;
	// Read the data to the linear ring buffer, and find the video sample one by one
	do
	{
		int read_size = read_unit_size;
		pBuf = AM_LRB_GetWritePtr(lrb, &read_size);
		if (pBuf == NULL || read_size < read_unit_size)
		{
			// Try to reform linear ring buffer
			AM_LRB_Reform(lrb);
			if ((pBuf = AM_LRB_GetWritePtr(lrb, &read_size)) == NULL || read_size < read_unit_size)
			{
				iRet = RET_CODE_ERROR;
				break;
			}
		}

		if ((read_size = (int)fread(pBuf, 1, read_unit_size, fp)) <= 0)
		{
			// Check FILE EOF
			if (feof(fp))
			{
				bEOF = true;
			}
			else
			{
				iRet = -1;
				break;
			}
		}

		if (!bEOF)
		{
			cur_byte_pos += read_size;

			AM_LRB_SkipWritePtr(lrb, (unsigned int)read_size);
		}

		if ((pStartBuf = AM_LRB_GetReadPtr(lrb, &cbSize)) == NULL || cbSize <= 0)
		{
			iRet = RET_CODE_ERROR;
			break;
		}

		pCurParseStartBuf = pBuf = pStartBuf;
		while (cbSize >= minimum_parse_buffer_size || (cur_AVC_NAL_size > 0 && cbSize > 0))
		{
			if (cur_AVC_NAL_size == -1)
			{
				while (cbSize >= minimum_parse_buffer_size &&
					!(pBuf[0] == 0 && (pBuf[4] & 0x80) == 0 && (pBuf[4] & 0x1F) < 16 && (pBuf[4] & 0x1F) != 0 && (pBuf[4] & 0x1F) != 14))
				{
					cbSize--;
					pBuf++;
				}

				if (pBuf > pCurParseStartBuf)
				{
					printf("[AAC] Commit the file data in the range [%" PRIu64 ", %" PRIu64 ").\n", cur_start_file_pos, cur_start_file_pos + (pBuf - pCurParseStartBuf));
					CommitAACBuffer(lrb_nv, pCurParseStartBuf, pBuf, afp, bEOF);

					AM_LRB_SkipReadPtr(lrb, (unsigned int)(pBuf - pCurParseStartBuf));

					cur_start_file_pos += pBuf - pCurParseStartBuf;

					pCurParseStartBuf = pBuf;
				}

				// The buffer is not enough, jump out the current loop, and continue reading data
				if (cbSize < minimum_parse_buffer_size)
				{
					if (bEOF)
					{
						pBuf += cbSize;
						cbSize = 0;
						CommitAACBuffer(lrb_nv, pCurParseStartBuf, pBuf, afp, bEOF);

						AM_LRB_SkipReadPtr(lrb, (unsigned int)(pBuf - pCurParseStartBuf));

						cur_start_file_pos += pBuf - pCurParseStartBuf;

						pCurParseStartBuf = pBuf;
					}

					break;
				}

				uint8_t tmp_nal_unit_type = pBuf[4] & 0x1F;
				uint8_t tmp_nal_ref_idc = (pBuf[4] >> 5) & 0x3;
				int NAL_Unit_len = (pBuf[0] << 24) | (pBuf[1] << 16) | (pBuf[2] << 8) | pBuf[3];

				// The VCL picture unit should not be greater than required MaxNALUnitSize
				if (NAL_Unit_len > filter.nMaxNALUnitSize)
				{
					cbSize--;
					pBuf++;
					continue;
				}

				// The below NAL unit size should not exceed 64KB
				if (tmp_nal_unit_type == 7 || tmp_nal_unit_type == 8 || tmp_nal_unit_type == 9 || tmp_nal_unit_type == 10 ||
					tmp_nal_unit_type == 11 || tmp_nal_unit_type == 13 || tmp_nal_unit_type == 15)
				{
					if (NAL_Unit_len > 0xFFFF)
					{
						cbSize--;
						pBuf++;
						continue;
					}
				}

				// For End_of_sequence and End_of_stream, the NAL unit length should not be greater than 1
				if ((tmp_nal_unit_type == 11 || tmp_nal_unit_type == 10) && NAL_Unit_len > 1)
				{
					cbSize--;
					pBuf++;
					continue;
				}

				//
				// nal_ref_idc shall not be equal to 0 for sequence parameter set or sequence parameter set extension or 
				// subset sequence parameter set or picture parameter set NAL units.
				//
				// nal_ref_idc shall not be equal to 0 for NAL units with nal_unit_type equal to 5.
				if (tmp_nal_ref_idc == 0 && (tmp_nal_unit_type == 7 || tmp_nal_unit_type == 8 || tmp_nal_unit_type == 13 || tmp_nal_unit_type == 15 || tmp_nal_unit_type == 5))
				{
					cbSize--;
					pBuf++;
					continue;
				}

				// nal_ref_idc shall be equal to 0 for all NAL units having nal_unit_type equal to 6, 9, 10, 11, or 12.
				if (tmp_nal_ref_idc != 0 && (tmp_nal_unit_type == 6 || tmp_nal_unit_type == 9 || tmp_nal_unit_type == 10 || tmp_nal_unit_type == 11 || tmp_nal_unit_type == 12))
				{
					cbSize--;
					pBuf++;
					continue;
				}

				// When nal_ref_idc is equal to 0 for one NAL unit with nal_unit_type in the range of 1 to 4, inclusive, 
				// of a particular picture, it shall be equal to 0 for all NAL units with nal_unit_type in the range of 1 to 4, inclusive, of the picture.
				if (part_picture_nal_ref_idc_is_zero && (tmp_nal_unit_type >= 1 && tmp_nal_unit_type <= 4 && tmp_nal_ref_idc != 0))
				{
					cbSize--;
					pBuf++;
					continue;
				}

				if (tmp_nal_unit_type >= 1 && tmp_nal_unit_type <= 5)
				{
					// For base-line profile
					// NAL unit streams shall not contain nal_unit_type values in the range of 2 to 4, inclusive
					if (g_NALUnitFilter.profile_idc == 66 && (tmp_nal_unit_type >= 2 && tmp_nal_unit_type <= 4))
					{
						cbSize--;
						pBuf++;
						continue;
					}
					
					if (tmp_nal_unit_type != 5 && nFoundNALUnits == 0 && filter.bStartFromIDR)
					{
						cbSize--;
						pBuf++;
						continue;
					}

					// Buffer is not enough to do the advance parse, read more buffer
					if (cbSize <= 5)
						break;

					CBitstream bs(pBuf + 5, (cbSize - 5) << 3);

					if (tmp_nal_unit_type == 3)
					{
						if (slice_id_of_data_partition_a < 0)
						{
							cbSize--;
							pBuf++;
							continue;
						}

						uint64_t slice_id = UINT64_MAX;
						if (GetUE(bs, slice_id) < 0 || slice_id > 8160)
						{
							cbSize--;
							pBuf++;
							continue;
						}

						slice_id_of_data_partition_b = (int)slice_id;
						slice_id_of_data_partition_a = -1;
					}
					else if (tmp_nal_unit_type == 4)
					{
						if (slice_id_of_data_partition_b < 0)
						{
							cbSize--;
							pBuf++;
							continue;
						}

						uint64_t slice_id = UINT64_MAX;
						if (GetUE(bs, slice_id) < 0 || slice_id > 8160)
						{
							cbSize--;
							pBuf++;
							continue;
						}

						slice_id_of_data_partition_c = (int)slice_id;
						slice_id_of_data_partition_b = -1;
					}
					else
					{
						try
						{
							uint64_t first_mb_in_slice = UINT64_MAX;
							if (GetUE(bs, first_mb_in_slice) < 0)
							{
								cbSize--;
								pBuf++;
								continue;
							}

							uint64_t slice_type = UINT64_MAX;
							if (GetUE(bs, slice_type) < 0 || slice_type > 9)
							{
								cbSize--;
								pBuf++;
								continue;
							}

							if (g_NALUnitFilter.profile_idc == 66)
							{
								if (slice_type == 1 || slice_type == 6)
								{
									cbSize--;
									pBuf++;
									continue;
								}
							}

							uint64_t pic_parameter_set_id = UINT64_MAX;
							if (GetUE(bs, pic_parameter_set_id) < 0 || pic_parameter_set_id > 255)
							{
								cbSize--;
								pBuf++;
								continue;
							}

							if (pic_parameter_set_id < g_NALUnitFilter.first_pps_id ||
								pic_parameter_set_id > g_NALUnitFilter.last_pps_id)
							{
								cbSize--;
								pBuf++;
								continue;
							}

							if (g_NALUnitFilter.separate_colour_plane_flag)
							{
								bs.SkipBits(2);	 //	colour_plane_id
							}

							uint64_t idr_pic_id = UINT64_MAX;
							uint8_t frame_num = (uint8_t)bs.GetBits(g_NALUnitFilter.log2_max_frame_num_minus4 + 4);

							if (tmp_nal_unit_type == 5)
							{
								if (frame_num != 0 || (slice_type != 2 && slice_type != 4 && slice_type != 7 && slice_type != 9))
								{
									cbSize--;
									pBuf++;
									continue;
								}
							}
							else
							{
								// FIXME
								// Workaround code here
								if (g_NALUnitFilter.profile_idc == 66 && (first_mb_in_slice != 0 || slice_type == 2 || slice_type == 4 || slice_type == 7 || slice_type == 9))
								{
									cbSize--;
									pBuf++;
									continue;
								}

								if (frame_num != (g_NALUnitFilter.prev_frame_num + 1) % (1 << (g_NALUnitFilter.log2_max_frame_num_minus4 + 4)))
								{
									cbSize--;
									pBuf++;
									continue;
								}
							}

							if (!g_NALUnitFilter.frame_mbs_only_flag)
							{
								uint8_t field_pic_flag = (uint8_t)bs.GetBits(1);
								uint8_t bottom_field_flag = 0;
								if (field_pic_flag)
									bottom_field_flag = (uint8_t)bs.GetBits(1);
							}

							if (tmp_nal_unit_type == 5)
								GetUE(bs, idr_pic_id);

							printf("slice_type: %" PRIu64 ", frame_num: %d, pic_parameter_set_id: %" PRIu64 ".\n", slice_type, frame_num, pic_parameter_set_id);

						}
						catch (...)
						{
							cbSize--;
							pBuf++;
							continue;
						}

						if (tmp_nal_unit_type == 2)
							slice_id_of_data_partition_a = 0;
						else
							slice_id_of_data_partition_a = -1;

						slice_id_of_data_partition_b = -1;
						slice_id_of_data_partition_c = -1;
					}
				}
				else if (tmp_nal_unit_type == 7 || tmp_nal_unit_type == 15)
				{
					if (cbSize <= 8)
						break;

					uint8_t profile_idc = pBuf[5];

					// constraint baseline profile
					if (profile_idc != 66)
					{
						cbSize--;
						pBuf++;
						continue;
					}

					uint8_t constraint_set = pBuf[6];
					uint8_t level_idc = pBuf[7];
				}
				else if (tmp_nal_unit_type == 8)
				{
					if (cbSize <= 7)
						break;

					uint64_t tmp_val = UINT64_MAX;
					uint8_t pic_parameter_set_id, seq_parameter_set_id;
					uint8_t entropy_coding_mode_flag, bottom_field_pic_order_in_frame_present_flag;

					CBitstream bs(pBuf + 5, (cbSize - 5) << 3);

					if (GetUE(bs, tmp_val) < 0 || tmp_val > 255)
					{
						cbSize--;
						pBuf++;
						continue;
					}

					pic_parameter_set_id = (uint8_t)tmp_val;

					if (GetUE(bs, tmp_val) < 0 || tmp_val > 255)
					{
						cbSize--;
						pBuf++;
						continue;
					}

					seq_parameter_set_id = (uint8_t)tmp_val;
					entropy_coding_mode_flag = (uint8_t)bs.GetBits(1);
					bottom_field_pic_order_in_frame_present_flag = (uint8_t)bs.GetBits(1);

					if (GetUE(bs, tmp_val) < 0 || tmp_val > 7)
					{
						cbSize--;
						pBuf++;
						continue;
					}
				}
				else if (tmp_nal_unit_type == 9)
				{
					if (NAL_Unit_len != 2)
					{
						cbSize--;
						pBuf++;
						continue;
					}

					uint8_t primary_pic_type = (pBuf[5] >> 5) & 0x7;
				}
				else if (tmp_nal_unit_type == 13)
				{
					if (NAL_Unit_len > 16)
					{
						cbSize--;
						pBuf++;
						continue;
					}
				}
				else if (tmp_nal_unit_type == 12)
				{
					int i = 0;
					int nAvailNUSize = AMP_MIN(cbSize - 5, NAL_Unit_len);
					for (i = 0; i < nAvailNUSize; i++)
						if (pBuf[5 + i] != 0xFF)
							break;

					if (i >= nAvailNUSize || IsTrailByte(pBuf[5 + i]) || NAL_Unit_len != 2 + i)
					{
						cbSize--;
						pBuf++;
						continue;
					}
				}

				printf("[NU#%" PRId64 "] nal_unit_type: %d, nal_unit length: %d.\n", nFoundNALUnits, tmp_nal_unit_type, NAL_Unit_len);

				// TODO
				// Add more check here

				// Deem it as a AVC NAL unit
				cur_AVC_NAL_size = NAL_Unit_len;
			}

			// the available read buffer can reach the previous NAL unit length
			if (cbSize >= cur_AVC_NAL_size + 4)
			{
				if (CommitAVCNALUnit(pBuf + 4, cur_AVC_NAL_size, vfp) < 0)
				{
					printf("Seems not to be a correct NAL unit.\n");
					cur_AVC_NAL_size = -1;
					cbSize--;
					pBuf++;
					continue;
				}
				else
				{
					nFoundNALUnits++;
				}

				AM_LRB_SkipReadPtr(lrb, cur_AVC_NAL_size + 4);

				cur_start_file_pos += cur_AVC_NAL_size + 4;

				pBuf += cur_AVC_NAL_size + 4;
				cbSize -= cur_AVC_NAL_size + 4;

				pCurParseStartBuf = pBuf;

				cur_AVC_NAL_size = -1;
			}
			else if (bEOF)
			{
				printf("Seems not to be a correct NAL unit.\n");
				cur_AVC_NAL_size = -1;
				cbSize--;
				pBuf++;
				continue;
			}
			else
			{
				// Buffer is not enough, try to read more buffer
				break;
			}
		}

	} while (!bEOF);

	CommitAACBuffer(lrb_nv, nullptr, nullptr, afp, true);

done:

	if (lrb != nullptr)
		AM_LRB_Destroy(lrb);

	if (lrb_nv != nullptr)
		AM_LRB_Destroy(lrb_nv);

	if (fp != nullptr)
		fclose(fp);

	if (afp != nullptr)
		fclose(afp);

	if (vfp != nullptr)
		fclose(vfp);

	UninitAACSyntaxParser();

	return iRet;
}

