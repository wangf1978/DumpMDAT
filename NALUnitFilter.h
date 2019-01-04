#ifndef __NAL_UNIT_FILTER_H__
#define __NAL_UNIT_FILTER_H__

#include <stdint.h>
#include <vector>

struct NALUnitFilter
{
	struct PPS
	{
		uint8_t		pic_parameter_set_id;
		uint8_t		seq_parameter_set_id;
		uint8_t		entropy_coding_mode_flag;
		uint8_t		bottom_field_pic_order_in_frame_present_flag;
		uint8_t		num_slice_groups_minus1;
	};

	int			nMaxNALUnitSize;
	int			nMaxAACRawDataBlockSize;
	bool		bStartFromIDR;
	bool		bAVCBaseline;
	bool		frame_mbs_only_flag;
	uint8_t		first_sps_id;
	uint8_t		last_sps_id;
	uint8_t		first_pps_id;
	uint8_t		last_pps_id;
	uint8_t		profile_idc;
	uint8_t		level_idc;
	uint8_t		chroma_format_idc;
	bool		separate_colour_plane_flag;
	uint8_t		bit_depth_luma_minus8;
	uint8_t		bit_depth_chroma_minus8;
	uint8_t		log2_max_frame_num_minus4;
	uint8_t		pic_order_cnt_type;
	uint8_t		log2_max_pic_order_cnt_lsb_minus4;
	uint8_t		delta_pic_order_always_zero_flag;
	int32_t		offset_for_non_ref_pic;
	int32_t		offset_for_top_to_bottom_field;
	uint8_t		num_ref_frames_in_pic_order_cnt_cycle;
	std::vector<int32_t>
				offset_for_ref_frames;
	uint8_t		max_num_ref_frames;
	bool		gaps_in_frame_num_value_allowed_flag;
	uint32_t	pic_width_in_mbs_minus1;
	uint32_t	pic_height_in_map_units_minus1;

	uint8_t		non_VCL_rbsp[65536];	// working buffer rbsp

	uint8_t		prev_frame_num;
	int			prev_idr_pic_id;
	bool		consequent_idr_pic_id;

	NALUnitFilter()
		: nMaxNALUnitSize(4 * 1024 * 1024)
		, nMaxAACRawDataBlockSize(1024 * 1024)
		, bStartFromIDR(true)
		, bAVCBaseline(true)
		, frame_mbs_only_flag(true)
		, first_sps_id(0)
		, last_sps_id(0)
		, first_pps_id(0)
		, last_pps_id(0)
		, profile_idc(66)
		, level_idc(0)
		, separate_colour_plane_flag(false)
		, bit_depth_luma_minus8(0)
		, bit_depth_chroma_minus8(0)
		, log2_max_frame_num_minus4(0)
		, pic_order_cnt_type(0)
		, prev_frame_num(0)
		, prev_idr_pic_id(-1)
		, consequent_idr_pic_id(0)
	{
	}
};

struct AudioSpecificConfig
{
	uint8_t audioObjectType;
	uint8_t samplingFrequencyIndex;
	uint32_t samplingFrequency;
	uint8_t channelConfiguration;

	uint8_t adts_fixed_header_bytes[4];
};

#endif

