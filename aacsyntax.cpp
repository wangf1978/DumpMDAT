#include "StdAfx.h"
#include "aacsyntax.h"
#include "huffman.h"

/* First object type that has ER */
#define ER_OBJECT_START 17

#define LEN_SE_ID 3
#define LEN_TAG   4
#define LEN_BYTE  8

#define EXT_FIL            0
#define EXT_FILL_DATA      1
#define EXT_DATA_ELEMENT   2
#define EXT_DYNAMIC_RANGE 11
#define ANC_DATA           0

/* Syntax elements */
#define ID_SCE 0x0
#define ID_CPE 0x1
#define ID_CCE 0x2
#define ID_LFE 0x3
#define ID_DSE 0x4
#define ID_PCE 0x5
#define ID_FIL 0x6
#define ID_END 0x7

#ifdef LD_DEC
ALIGN static const uint8_t num_swb_512_window[] =
{
	0, 0, 0, 36, 36, 37, 31, 31, 0, 0, 0, 0
};
ALIGN static const uint8_t num_swb_480_window[] =
{
	0, 0, 0, 35, 35, 37, 30, 30, 0, 0, 0, 0
};
#endif

ALIGN static const uint8_t num_swb_960_window[] =
{
	40, 40, 45, 49, 49, 49, 46, 46, 42, 42, 42, 40
};

ALIGN static const uint8_t num_swb_1024_window[] =
{
	41, 41, 47, 49, 49, 51, 47, 47, 43, 43, 43, 40
};

ALIGN static const uint8_t num_swb_128_window[] =
{
	12, 12, 12, 14, 14, 14, 15, 15, 15, 15, 15, 15
};

ALIGN static const uint16_t swb_offset_1024_96[] =
{
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
	64, 72, 80, 88, 96, 108, 120, 132, 144, 156, 172, 188, 212, 240,
	276, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1024
};

ALIGN static const uint16_t swb_offset_128_96[] =
{
	0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128
};

ALIGN static const uint16_t swb_offset_1024_64[] =
{
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
	64, 72, 80, 88, 100, 112, 124, 140, 156, 172, 192, 216, 240, 268,
	304, 344, 384, 424, 464, 504, 544, 584, 624, 664, 704, 744, 784, 824,
	864, 904, 944, 984, 1024
};

ALIGN static const uint16_t swb_offset_128_64[] =
{
	0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128
};

ALIGN static const uint16_t swb_offset_1024_48[] =
{
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
	80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
	320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
	768, 800, 832, 864, 896, 928, 1024
};

#ifdef LD_DEC
ALIGN static const uint16_t swb_offset_512_48[] =
{
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 68, 76, 84,
	92, 100, 112, 124, 136, 148, 164, 184, 208, 236, 268, 300, 332, 364, 396,
	428, 460, 512
};

ALIGN static const uint16_t swb_offset_480_48[] =
{
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72 ,80 ,88,
	96, 108, 120, 132, 144, 156, 172, 188, 212, 240, 272, 304, 336, 368, 400,
	432, 480
};
#endif

ALIGN static const uint16_t swb_offset_128_48[] =
{
	0, 4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112, 128
};

ALIGN static const uint16_t swb_offset_1024_32[] =
{
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
	80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
	320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
	768, 800, 832, 864, 896, 928, 960, 992, 1024
};

#ifdef LD_DEC
ALIGN static const uint16_t swb_offset_512_32[] =
{
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80,
	88, 96, 108, 120, 132, 144, 160, 176, 192, 212, 236, 260, 288, 320, 352,
	384, 416, 448, 480, 512
};

ALIGN static const uint16_t swb_offset_480_32[] =
{
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 72, 80,
	88, 96, 104, 112, 124, 136, 148, 164, 180, 200, 224, 256, 288, 320, 352,
	384, 416, 448, 480
};
#endif

ALIGN static const uint16_t swb_offset_1024_24[] =
{
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68,
	76, 84, 92, 100, 108, 116, 124, 136, 148, 160, 172, 188, 204, 220,
	240, 260, 284, 308, 336, 364, 396, 432, 468, 508, 552, 600, 652, 704,
	768, 832, 896, 960, 1024
};

#ifdef LD_DEC
ALIGN static const uint16_t swb_offset_512_24[] =
{
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68,
	80, 92, 104, 120, 140, 164, 192, 224, 256, 288, 320, 352, 384, 416,
	448, 480, 512
};

ALIGN static const uint16_t swb_offset_480_24[] =
{
	0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68, 80, 92, 104, 120,
	140, 164, 192, 224, 256, 288, 320, 352, 384, 416, 448, 480
};
#endif

ALIGN static const uint16_t swb_offset_128_24[] =
{
	0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 128
};

ALIGN static const uint16_t swb_offset_1024_16[] =
{
	0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 100, 112, 124,
	136, 148, 160, 172, 184, 196, 212, 228, 244, 260, 280, 300, 320, 344,
	368, 396, 424, 456, 492, 532, 572, 616, 664, 716, 772, 832, 896, 960, 1024
};

ALIGN static const uint16_t swb_offset_128_16[] =
{
	0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 128
};

ALIGN static const uint16_t swb_offset_1024_8[] =
{
	0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 172,
	188, 204, 220, 236, 252, 268, 288, 308, 328, 348, 372, 396, 420, 448,
	476, 508, 544, 580, 620, 664, 712, 764, 820, 880, 944, 1024
};

ALIGN static const uint16_t swb_offset_128_8[] =
{
	0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 60, 72, 88, 108, 128
};

ALIGN static const uint16_t *swb_offset_1024_window[] =
{
	swb_offset_1024_96,      /* 96000 */
	swb_offset_1024_96,      /* 88200 */
	swb_offset_1024_64,      /* 64000 */
	swb_offset_1024_48,      /* 48000 */
	swb_offset_1024_48,      /* 44100 */
	swb_offset_1024_32,      /* 32000 */
	swb_offset_1024_24,      /* 24000 */
	swb_offset_1024_24,      /* 22050 */
	swb_offset_1024_16,      /* 16000 */
	swb_offset_1024_16,      /* 12000 */
	swb_offset_1024_16,      /* 11025 */
	swb_offset_1024_8        /* 8000  */
};

#ifdef LD_DEC
ALIGN static const uint16_t *swb_offset_512_window[] =
{
	0,                       /* 96000 */
	0,                       /* 88200 */
	0,                       /* 64000 */
	swb_offset_512_48,       /* 48000 */
	swb_offset_512_48,       /* 44100 */
	swb_offset_512_32,       /* 32000 */
	swb_offset_512_24,       /* 24000 */
	swb_offset_512_24,       /* 22050 */
	0,                       /* 16000 */
	0,                       /* 12000 */
	0,                       /* 11025 */
	0                        /* 8000  */
};

ALIGN static const uint16_t *swb_offset_480_window[] =
{
	0,                       /* 96000 */
	0,                       /* 88200 */
	0,                       /* 64000 */
	swb_offset_480_48,       /* 48000 */
	swb_offset_480_48,       /* 44100 */
	swb_offset_480_32,       /* 32000 */
	swb_offset_480_24,       /* 24000 */
	swb_offset_480_24,       /* 22050 */
	0,                       /* 16000 */
	0,                       /* 12000 */
	0,                       /* 11025 */
	0                        /* 8000  */
};
#endif

ALIGN static const  uint16_t *swb_offset_128_window[] =
{
	swb_offset_128_96,       /* 96000 */
	swb_offset_128_96,       /* 88200 */
	swb_offset_128_64,       /* 64000 */
	swb_offset_128_48,       /* 48000 */
	swb_offset_128_48,       /* 44100 */
	swb_offset_128_48,       /* 32000 */
	swb_offset_128_24,       /* 24000 */
	swb_offset_128_24,       /* 22050 */
	swb_offset_128_16,       /* 16000 */
	swb_offset_128_16,       /* 12000 */
	swb_offset_128_16,       /* 11025 */
	swb_offset_128_8         /* 8000  */
};

#define bit_set(A, B) ((A) & (1<<(B)))

uint8_t max_pred_sfb(const uint8_t sr_index)
{
	static const uint8_t pred_sfb_max[] =
	{
		33, 33, 38, 40, 40, 40, 41, 41, 37, 37, 37, 34
	};


	if (sr_index < 12)
		return pred_sfb_max[sr_index];

	return 0;
}

/* 4.5.2.3.4 */
/*
- determine the number of windows in a window_sequence named num_windows
- determine the number of window_groups named num_window_groups
- determine the number of windows in each group named window_group_length[g]
- determine the total number of scalefactor window bands named num_swb for
the actual window type
- determine swb_offset[swb], the offset of the first coefficient in
scalefactor window band named swb of the window actually used
- determine sect_sfb_offset[g][section],the offset of the first coefficient
in section named section. This offset depends on window_sequence and
scale_factor_grouping and is needed to decode the spectral_data().
*/
uint8_t window_grouping_info(NeAACDecStruct *hDecoder, ic_stream *ics)
{
	uint8_t i, g;

	uint8_t sf_index = hDecoder->sf_index;

	switch (ics->window_sequence) {
	case ONLY_LONG_SEQUENCE:
	case LONG_START_SEQUENCE:
	case LONG_STOP_SEQUENCE:
		ics->num_windows = 1;
		ics->num_window_groups = 1;
		ics->window_group_length[ics->num_window_groups - 1] = 1;
#ifdef LD_DEC
		if (hDecoder->object_type == LD)
		{
			if (hDecoder->frameLength == 512)
				ics->num_swb = num_swb_512_window[sf_index];
			else /* if (hDecoder->frameLength == 480) */
				ics->num_swb = num_swb_480_window[sf_index];
		}
		else {
#endif
			if (hDecoder->frameLength == 1024)
				ics->num_swb = num_swb_1024_window[sf_index];
			else /* if (hDecoder->frameLength == 960) */
				ics->num_swb = num_swb_960_window[sf_index];
#ifdef LD_DEC
		}
#endif

		if (ics->max_sfb > ics->num_swb)
		{
			return 32;
		}

		/* preparation of sect_sfb_offset for long blocks */
		/* also copy the last value! */
#ifdef LD_DEC
		if (hDecoder->object_type == LD)
		{
			if (hDecoder->frameLength == 512)
			{
				for (i = 0; i < ics->num_swb; i++)
				{
					ics->sect_sfb_offset[0][i] = swb_offset_512_window[sf_index][i];
					ics->swb_offset[i] = swb_offset_512_window[sf_index][i];
				}
			}
			else /* if (hDecoder->frameLength == 480) */ {
				for (i = 0; i < ics->num_swb; i++)
				{
					ics->sect_sfb_offset[0][i] = swb_offset_480_window[sf_index][i];
					ics->swb_offset[i] = swb_offset_480_window[sf_index][i];
				}
			}
			ics->sect_sfb_offset[0][ics->num_swb] = hDecoder->frameLength;
			ics->swb_offset[ics->num_swb] = hDecoder->frameLength;
			ics->swb_offset_max = hDecoder->frameLength;
		}
		else {
#endif
			for (i = 0; i < ics->num_swb; i++)
			{
				ics->sect_sfb_offset[0][i] = swb_offset_1024_window[sf_index][i];
				ics->swb_offset[i] = swb_offset_1024_window[sf_index][i];
			}
			ics->sect_sfb_offset[0][ics->num_swb] = hDecoder->frameLength;
			ics->swb_offset[ics->num_swb] = hDecoder->frameLength;
			ics->swb_offset_max = hDecoder->frameLength;
#ifdef LD_DEC
		}
#endif
		return 0;
	case EIGHT_SHORT_SEQUENCE:
		ics->num_windows = 8;
		ics->num_window_groups = 1;
		ics->window_group_length[ics->num_window_groups - 1] = 1;
		ics->num_swb = num_swb_128_window[sf_index];

		if (ics->max_sfb > ics->num_swb)
		{
			return 32;
		}

		for (i = 0; i < ics->num_swb; i++)
			ics->swb_offset[i] = swb_offset_128_window[sf_index][i];
		ics->swb_offset[ics->num_swb] = hDecoder->frameLength / 8;
		ics->swb_offset_max = hDecoder->frameLength / 8;

		for (i = 0; i < ics->num_windows - 1; i++) {
			if (bit_set(ics->scale_factor_grouping, 6 - i) == 0)
			{
				ics->num_window_groups += 1;
				ics->window_group_length[ics->num_window_groups - 1] = 1;
			}
			else {
				ics->window_group_length[ics->num_window_groups - 1] += 1;
			}
		}

		/* preparation of sect_sfb_offset for short blocks */
		for (g = 0; g < ics->num_window_groups; g++)
		{
			uint16_t width;
			uint8_t sect_sfb = 0;
			uint16_t offset = 0;

			for (i = 0; i < ics->num_swb; i++)
			{
				if (i + 1 == ics->num_swb)
				{
					width = (hDecoder->frameLength / 8) - swb_offset_128_window[sf_index][i];
				}
				else {
					width = swb_offset_128_window[sf_index][i + 1] -
						swb_offset_128_window[sf_index][i];
				}
				width *= ics->window_group_length[g];
				ics->sect_sfb_offset[g][sect_sfb++] = offset;
				offset += width;
			}
			ics->sect_sfb_offset[g][sect_sfb] = offset;
		}
		return 0;
	default:
		return 32;
	}
}

/* Table 4.4.27 */
static void tns_data(ic_stream *ics, tns_info *tns, CBitstream& bs)
{
	uint8_t w, filt, i, start_coef_bits, coef_bits;
	uint8_t n_filt_bits = 2;
	uint8_t length_bits = 6;
	uint8_t order_bits = 5;

	if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
	{
		n_filt_bits = 1;
		length_bits = 4;
		order_bits = 3;
	}

	for (w = 0; w < ics->num_windows; w++)
	{
		tns->n_filt[w] = (uint8_t)bs.GetBits(n_filt_bits);
#if 0
		printf("%d\n", tns->n_filt[w]);
#endif

		if (tns->n_filt[w])
		{
			if ((tns->coef_res[w] = (uint8_t)bs.GetBits(1)) & 1)
			{
				start_coef_bits = 4;
			}
			else {
				start_coef_bits = 3;
			}
#if 0
			printf("%d\n", tns->coef_res[w]);
#endif
		}

		for (filt = 0; filt < tns->n_filt[w]; filt++)
		{
			tns->length[w][filt] = (uint8_t)bs.GetBits(length_bits);
#if 0
			printf("%d\n", tns->length[w][filt]);
#endif
			tns->order[w][filt] = (uint8_t)bs.GetBits(order_bits);
#if 0
			printf("%d\n", tns->order[w][filt]);
#endif
			if (tns->order[w][filt])
			{
				tns->direction[w][filt] = (uint8_t)bs.GetBits(1);
#if 0
				printf("%d\n", tns->direction[w][filt]);
#endif
				tns->coef_compress[w][filt] = (uint8_t)bs.GetBits(1);
#if 0
				printf("%d\n", tns->coef_compress[w][filt]);
#endif

				coef_bits = start_coef_bits - tns->coef_compress[w][filt];
				for (i = 0; i < tns->order[w][filt]; i++)
				{
					tns->coef[w][filt][i] = (uint8_t)bs.GetBits(coef_bits);
#if 0
					printf("%d\n", tns->coef[w][filt][i]);
#endif
				}
			}
		}
	}
}

#ifdef LTP_DEC
/* Table 4.4.28 */
static uint8_t ltp_data(NeAACDecStruct *hDecoder, ic_stream *ics, ltp_info *ltp, CBitstream &bs)
{
	uint8_t sfb, w;

	ltp->lag = 0;

#ifdef LD_DEC
	if (hDecoder->object_type == LD)
	{
		ltp->lag_update = (uint8_t)faad_getbits(ld, 1
			DEBUGVAR(1, 142, "ltp_data(): lag_update"));

		if (ltp->lag_update)
		{
			ltp->lag = (uint16_t)faad_getbits(ld, 10
				DEBUGVAR(1, 81, "ltp_data(): lag"));
		}
	}
	else {
#endif
		ltp->lag = (uint16_t)bs.GetBits(11);
#ifdef LD_DEC
	}
#endif

	/* Check length of lag */
	if (ltp->lag > (hDecoder->frameLength << 1))
		return 18;

	ltp->coef = (uint8_t)bs.GetBits(3);

	if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
	{
		for (w = 0; w < ics->num_windows; w++)
		{
			if ((ltp->short_used[w] = (uint8_t)bs.GetBits(1)) & 1)
			{
				ltp->short_lag_present[w] = (uint8_t)bs.GetBits(1);
				if (ltp->short_lag_present[w])
				{
					ltp->short_lag[w] = (uint8_t)bs.GetBits(4);
				}
			}
		}
	}
	else {
		ltp->last_band = (ics->max_sfb < MAX_LTP_SFB ? ics->max_sfb : MAX_LTP_SFB);

		for (sfb = 0; sfb < ltp->last_band; sfb++)
		{
			ltp->long_used[sfb] = (uint8_t)bs.GetBits(1);
		}
	}

	return 0;
}
#endif

/* Table 4.4.6 */
static uint8_t ics_info(NeAACDecStruct *hDecoder, ic_stream *ics, CBitstream& bs, uint8_t common_window)
{
	uint8_t retval = 0;
	uint8_t ics_reserved_bit;

	ics_reserved_bit = (uint8_t)bs.GetBits(1);
	if (ics_reserved_bit != 0)
		return 32;
	ics->window_sequence = (uint8_t)bs.GetBits(2);
	ics->window_shape = (uint8_t)bs.GetBits(1);

#ifdef LD_DEC
	/* No block switching in LD */
	if ((hDecoder->object_type == LD) && (ics->window_sequence != ONLY_LONG_SEQUENCE))
		return 32;
#endif

	if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
	{
		ics->max_sfb = (uint8_t)bs.GetBits(4);
		ics->scale_factor_grouping = (uint8_t)bs.GetBits(7);
	}
	else {
		ics->max_sfb = (uint8_t)bs.GetBits(6);
	}

	/* get the grouping information */
	if ((retval = window_grouping_info(hDecoder, ics)) > 0)
		return retval;


	/* should be an error */
	/* check the range of max_sfb */
	if (ics->max_sfb > ics->num_swb)
		return 16;

	if (ics->window_sequence != EIGHT_SHORT_SEQUENCE)
	{
		if ((ics->predictor_data_present = (uint8_t)bs.GetBits(1)) & 1)
		{
			if (hDecoder->object_type == MAIN) /* MPEG2 style AAC predictor */
			{
				uint8_t sfb;

				uint8_t limit = AMP_MIN(ics->max_sfb, max_pred_sfb(hDecoder->sf_index));
#ifdef MAIN_DEC
				ics->pred.limit = limit;
#endif

				if ((
#ifdef MAIN_DEC
					ics->pred.predictor_reset =
#endif
					(uint8_t)bs.GetBits(1)) & 1)
				{
#ifdef MAIN_DEC
					ics->pred.predictor_reset_group_number =
#endif
						(uint8_t)bs.GetBits(5);
				}

				for (sfb = 0; sfb < limit; sfb++)
				{
#ifdef MAIN_DEC
					ics->pred.prediction_used[sfb] = (uint8_t)
#endif
						bs.GetBits(1);
				}
			}
#ifdef LTP_DEC
			else { /* Long Term Prediction */
				{
					if ((ics->ltp.data_present = (uint8_t)bs.GetBits(1)) & 1)
					{
						if ((retval = ltp_data(hDecoder, ics, &(ics->ltp), bs)) > 0)
						{
							return retval;
						}
					}
					if (common_window)
					{
						if ((ics->ltp2.data_present = (uint8_t)bs.GetBits(1)) & 1)
						{
							if ((retval = ltp_data(hDecoder, ics, &(ics->ltp2), bs)) > 0)
							{
								return retval;
							}
						}
					}
				}
			}
#endif
		}
	}

	return retval;
}

/*
*  decode_scale_factors()
*   decodes the scalefactors from the bitstream
*/
/*
* All scalefactors (and also the stereo positions and pns energies) are
* transmitted using Huffman coded DPCM relative to the previous active
* scalefactor (respectively previous stereo position or previous pns energy,
* see subclause 4.6.2 and 4.6.3). The first active scalefactor is
* differentially coded relative to the global gain.
*/
static uint8_t decode_scale_factors(ic_stream *ics, CBitstream& bs)
{
	uint8_t g, sfb;
	int16_t t;
	int8_t noise_pcm_flag = 1;

	int16_t scale_factor = ics->global_gain;
	int16_t is_position = 0;
	int16_t noise_energy = ics->global_gain - 90;

	for (g = 0; g < ics->num_window_groups; g++)
	{
		for (sfb = 0; sfb < ics->max_sfb; sfb++)
		{
			switch (ics->sfb_cb[g][sfb])
			{
			case ZERO_HCB: /* zero book */
				ics->scale_factors[g][sfb] = 0;
				//#define SF_PRINT
#ifdef SF_PRINT
				printf("%d\n", ics->scale_factors[g][sfb]);
#endif
				break;
			case INTENSITY_HCB: /* intensity books */
			case INTENSITY_HCB2:

				/* decode intensity position */
				t = huffman_scale_factor(bs);
				is_position += (t - 60);
				ics->scale_factors[g][sfb] = is_position;
#ifdef SF_PRINT
				printf("%d\n", ics->scale_factors[g][sfb]);
#endif

				break;
			case NOISE_HCB: /* noise books */

#ifndef DRM
							/* decode noise energy */
				if (noise_pcm_flag)
				{
					noise_pcm_flag = 0;
					t = (int16_t)bs.GetBits(9);
				}
				else {
					t = huffman_scale_factor(bs);
					t -= 60;
				}
				noise_energy += t;
				ics->scale_factors[g][sfb] = noise_energy;
#ifdef SF_PRINT
				printf("%d\n", ics->scale_factors[g][sfb]);
#endif
#else
							/* PNS not allowed in DRM */
				return 29;
#endif

				break;
			default: /* spectral books */

					 /* ics->scale_factors[g][sfb] must be between 0 and 255 */

				ics->scale_factors[g][sfb] = 0;

				/* decode scale factor */
				t = huffman_scale_factor(bs);
				scale_factor += (t - 60);
				if (scale_factor < 0 || scale_factor > 255)
					return 4;
				ics->scale_factors[g][sfb] = scale_factor;
#ifdef SF_PRINT
				printf("%d\n", ics->scale_factors[g][sfb]);
#endif

				break;
			}
		}
	}

	return 0;
}

/* Table 4.4.25 */
static uint8_t section_data(NeAACDecStruct *hDecoder, ic_stream *ics, CBitstream& bs)
{
	uint8_t g;
	uint8_t sect_esc_val, sect_bits;

	if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
		sect_bits = 3;
	else
		sect_bits = 5;
	sect_esc_val = (1 << sect_bits) - 1;

#if 0
	printf("\ntotal sfb %d\n", ics->max_sfb);
	printf("   sect    top     cb\n");
#endif

	for (g = 0; g < ics->num_window_groups; g++)
	{
		uint8_t k = 0;
		uint8_t i = 0;

		while (k < ics->max_sfb)
		{
#ifdef ERROR_RESILIENCE
			uint8_t vcb11 = 0;
#endif
			uint8_t sfb;
			uint8_t sect_len_incr;
			uint16_t sect_len = 0;
			uint8_t sect_cb_bits = 4;

			/* if "faad_getbits" detects error and returns "0", "k" is never
			incremented and we cannot leave the while loop */
			//if (ld->error != 0)
			//	return 14;

#ifdef ERROR_RESILIENCE
			if (hDecoder->aacSectionDataResilienceFlag)
				sect_cb_bits = 5;
#endif

			ics->sect_cb[g][i] = (uint8_t)bs.GetBits(sect_cb_bits);

			if (ics->sect_cb[g][i] == 12)
				return 32;

#if 0
			printf("%d\n", ics->sect_cb[g][i]);
#endif

#ifndef DRM
			if (ics->sect_cb[g][i] == NOISE_HCB)
				ics->noise_used = 1;
#else
			/* PNS not allowed in DRM */
			if (ics->sect_cb[g][i] == NOISE_HCB)
				return 29;
#endif
			if (ics->sect_cb[g][i] == INTENSITY_HCB2 || ics->sect_cb[g][i] == INTENSITY_HCB)
				ics->is_used = 1;

#ifdef ERROR_RESILIENCE
			if (hDecoder->aacSectionDataResilienceFlag)
			{
				if ((ics->sect_cb[g][i] == 11) ||
					((ics->sect_cb[g][i] >= 16) && (ics->sect_cb[g][i] <= 32)))
				{
					vcb11 = 1;
				}
			}
			if (vcb11)
			{
				sect_len_incr = 1;
			}
			else {
#endif
				sect_len_incr = (uint8_t)bs.GetBits(sect_bits);
#ifdef ERROR_RESILIENCE
			}
#endif
			while ((sect_len_incr == sect_esc_val) /* &&
												   (k+sect_len < ics->max_sfb)*/)
			{
				sect_len += sect_len_incr;
				sect_len_incr = (uint8_t)bs.GetBits(sect_bits);
			}

			sect_len += sect_len_incr;

			ics->sect_start[g][i] = k;
			ics->sect_end[g][i] = k + sect_len;

#if 0
			printf("%d\n", ics->sect_start[g][i]);
#endif
#if 0
			printf("%d\n", ics->sect_end[g][i]);
#endif

			if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
			{
				if (k + sect_len > 8 * 15)
					return 15;
				if (i >= 8 * 15)
					return 15;
			}
			else {
				if (k + sect_len > MAX_SFB)
					return 15;
				if (i >= MAX_SFB)
					return 15;
			}

			for (sfb = k; sfb < k + sect_len; sfb++)
			{
				ics->sfb_cb[g][sfb] = ics->sect_cb[g][i];
#if 0
				printf("%d\n", ics->sfb_cb[g][sfb]);
#endif
			}

#if 0
			printf(" %6d %6d %6d\n",
				i,
				ics->sect_end[g][i],
				ics->sect_cb[g][i]);
#endif

			k += sect_len;
			i++;
		}
		ics->num_sec[g] = i;

		/* the sum of all sect_len_incr elements for a given window
		* group shall equal max_sfb */
		if (k != ics->max_sfb)
		{
			return 32;
		}
#if 0
		printf("%d\n", ics->num_sec[g]);
#endif
	}

#if 0
	printf("\n");
#endif

	return 0;
}

/* Table 4.4.26 */
static uint8_t scale_factor_data(NeAACDecStruct *hDecoder, ic_stream *ics, CBitstream& bs)
{
	uint8_t ret = 0;
#ifdef PROFILE
	int64_t count = faad_get_ts();
#endif

#ifdef ERROR_RESILIENCE
	if (!hDecoder->aacScalefactorDataResilienceFlag)
	{
#endif
		ret = decode_scale_factors(ics, bs);
#ifdef ERROR_RESILIENCE
	}
	else {
		/* In ER AAC the parameters for RVLC are seperated from the actual
		data that holds the scale_factors.
		Strangely enough, 2 parameters for HCR are put inbetween them.
		*/
		ret = rvlc_scale_factor_data(ics, ld);
	}
#endif

#ifdef PROFILE
	count = faad_get_ts() - count;
	hDecoder->scalefac_cycles += count;
#endif

	return ret;
}

/* Table 4.4.7 */
static uint8_t pulse_data(ic_stream *ics, pulse_info *pul, CBitstream& bs)
{
	uint8_t i;

	pul->number_pulse = (uint8_t)bs.GetBits(2);
	pul->pulse_start_sfb = (uint8_t)bs.GetBits(6);

	/* check the range of pulse_start_sfb */
	if (pul->pulse_start_sfb > ics->num_swb)
		return 16;

	for (i = 0; i < pul->number_pulse + 1; i++)
	{
		pul->pulse_offset[i] = (uint8_t)bs.GetBits(5);
#if 0
		printf("%d\n", pul->pulse_offset[i]);
#endif
		pul->pulse_amp[i] = (uint8_t)bs.GetBits(4);
#if 0
		printf("%d\n", pul->pulse_amp[i]);
#endif
	}

	return 0;
}

/* Table 4.4.12 */
#ifdef SSR_DEC
static void gain_control_data(CBitstream& bs, ic_stream *ics)
{
	uint8_t bd, wd, ad;
	ssr_info *ssr = &(ics->ssr);

	ssr->max_band = (uint8_t)bs.GetBits(2);
	if (ics->window_sequence == ONLY_LONG_SEQUENCE)
	{
		for (bd = 1; bd <= ssr->max_band; bd++)
		{
			for (wd = 0; wd < 1; wd++)
			{
				ssr->adjust_num[bd][wd] = (uint8_t)bs.GetBits(3);

				for (ad = 0; ad < ssr->adjust_num[bd][wd]; ad++)
				{
					ssr->alevcode[bd][wd][ad] = (uint8_t)bs.GetBits(4);
					ssr->aloccode[bd][wd][ad] = (uint8_t)bs.GetBits(5);
				}
			}
		}
	}
	else if (ics->window_sequence == LONG_START_SEQUENCE) {
		for (bd = 1; bd <= ssr->max_band; bd++)
		{
			for (wd = 0; wd < 2; wd++)
			{
				ssr->adjust_num[bd][wd] = (uint8_t)bs.GetBits(3);

				for (ad = 0; ad < ssr->adjust_num[bd][wd]; ad++)
				{
					ssr->alevcode[bd][wd][ad] = (uint8_t)bs.GetBits(4);
					if (wd == 0)
					{
						ssr->aloccode[bd][wd][ad] = (uint8_t)bs.GetBits(4);
					}
					else {
						ssr->aloccode[bd][wd][ad] = (uint8_t)bs.GetBits(2);
					}
				}
			}
		}
	}
	else if (ics->window_sequence == EIGHT_SHORT_SEQUENCE) {
		for (bd = 1; bd <= ssr->max_band; bd++)
		{
			for (wd = 0; wd < 8; wd++)
			{
				ssr->adjust_num[bd][wd] = (uint8_t)bs.GetBits(3);

				for (ad = 0; ad < ssr->adjust_num[bd][wd]; ad++)
				{
					ssr->alevcode[bd][wd][ad] = (uint8_t)bs.GetBits(4);
					ssr->aloccode[bd][wd][ad] = (uint8_t)bs.GetBits(2);
				}
			}
		}
	}
	else if (ics->window_sequence == LONG_STOP_SEQUENCE) {
		for (bd = 1; bd <= ssr->max_band; bd++)
		{
			for (wd = 0; wd < 2; wd++)
			{
				ssr->adjust_num[bd][wd] = (uint8_t)bs.GetBits(3);

				for (ad = 0; ad < ssr->adjust_num[bd][wd]; ad++)
				{
					ssr->alevcode[bd][wd][ad] = (uint8_t)bs.GetBits(4);

					if (wd == 0)
					{
						ssr->aloccode[bd][wd][ad] = (uint8_t)bs.GetBits(4);
					}
					else {
						ssr->aloccode[bd][wd][ad] = (uint8_t)bs.GetBits(5);
					}
				}
			}
		}
	}
}
#endif

static uint8_t side_info(NeAACDecStruct *hDecoder, element *ele, CBitstream& bs, ic_stream *ics, uint8_t scal_flag)
{
	uint8_t result;

	ics->global_gain = (uint8_t)bs.GetBits(8);

	if (!ele->common_window && !scal_flag)
	{
		if ((result = ics_info(hDecoder, ics, bs, ele->common_window)) > 0)
			return result;
	}

	if ((result = section_data(hDecoder, ics, bs)) > 0)
		return result;

	if ((result = scale_factor_data(hDecoder, ics, bs)) > 0)
		return result;

	if (!scal_flag)
	{
		/**
		**  NOTE: It could be that pulse data is available in scalable AAC too,
		**        as said in Amendment 1, this could be only the case for ER AAC,
		**        though. (have to check this out later)
		**/
		/* get pulse data */
		if ((ics->pulse_data_present = (uint8_t)bs.GetBits(1)) & 1)
		{
			if ((result = pulse_data(ics, &(ics->pul), bs)) > 0)
				return result;
		}

		/* get tns data */
		if ((ics->tns_data_present = (uint8_t)bs.GetBits(1)) & 1)
		{
			tns_data(ics, &(ics->tns), bs);
		}

		/* get gain control data */
		if ((ics->gain_control_data_present = (uint8_t)bs.GetBits(1)) & 1)
		{
#ifdef SSR_DEC
			if (hDecoder->object_type != SSR)
				return 1;
			else
				gain_control_data(bs, ics);
#else
			return 1;
#endif
		}
	}

	return 0;
}

/* Table 4.4.29 */
static uint8_t spectral_data(NeAACDecStruct *hDecoder, ic_stream *ics, CBitstream& bs, int16_t *spectral_data)
{
	int8_t i;
	uint8_t g;
	uint16_t inc, k, p = 0;
	uint8_t groups = 0;
	uint8_t sect_cb;
	uint8_t result;
	uint16_t nshort = hDecoder->frameLength / 8;

#ifdef PROFILE
	int64_t count = faad_get_ts();
#endif

	for (g = 0; g < ics->num_window_groups; g++)
	{
		p = groups*nshort;

		for (i = 0; i < ics->num_sec[g]; i++)
		{
			sect_cb = ics->sect_cb[g][i];

			inc = (sect_cb >= FIRST_PAIR_HCB) ? 2 : 4;

			switch (sect_cb)
			{
			case ZERO_HCB:
			case NOISE_HCB:
			case INTENSITY_HCB:
			case INTENSITY_HCB2:
				//#define SD_PRINT
#ifdef SD_PRINT
			{
				int j;
				for (j = ics->sect_sfb_offset[g][ics->sect_start[g][i]]; j < ics->sect_sfb_offset[g][ics->sect_end[g][i]]; j++)
				{
					printf("%d\n", 0);
				}
			}
#endif
			//#define SFBO_PRINT
#ifdef SFBO_PRINT
			printf("%d\n", ics->sect_sfb_offset[g][ics->sect_start[g][i]]);
#endif
			p += (ics->sect_sfb_offset[g][ics->sect_end[g][i]] -
				ics->sect_sfb_offset[g][ics->sect_start[g][i]]);
			break;
			default:
#ifdef SFBO_PRINT
				printf("%d\n", ics->sect_sfb_offset[g][ics->sect_start[g][i]]);
#endif
				for (k = ics->sect_sfb_offset[g][ics->sect_start[g][i]];
					k < ics->sect_sfb_offset[g][ics->sect_end[g][i]]; k += inc)
				{
					if ((result = huffman_spectral_data(sect_cb, bs, &spectral_data[p])) > 0)
						return result;
#ifdef SD_PRINT
					{
						int j;
						for (j = p; j < p + inc; j++)
						{
							printf("%d\n", spectral_data[j]);
						}
					}
#endif
					p += inc;
				}
				break;
			}
		}
		groups += ics->window_group_length[g];
	}

#ifdef PROFILE
	count = faad_get_ts() - count;
	hDecoder->spectral_cycles += count;
#endif

	return 0;
}

uint8_t pulse_decode(ic_stream *ics, int16_t *spec_data, uint16_t framelen)
{
	uint8_t i;
	uint16_t k;
	pulse_info *pul = &(ics->pul);

	k = AMP_MIN(ics->swb_offset[pul->pulse_start_sfb], ics->swb_offset_max);

	for (i = 0; i <= pul->number_pulse; i++)
	{
		k += pul->pulse_offset[i];

		if (k >= framelen)
			return 15; /* should not be possible */

		if (spec_data[k] > 0)
			spec_data[k] += pul->pulse_amp[i];
		else
			spec_data[k] -= pul->pulse_amp[i];
	}

	return 0;
}

/* Table 4.4.24 */
static uint8_t individual_channel_stream(NeAACDecStruct *hDecoder, element *ele, CBitstream& bs, ic_stream *ics, uint8_t scal_flag, int16_t *spec_data)
{
	uint8_t result;

	result = side_info(hDecoder, ele, bs, ics, scal_flag);
	if (result > 0)
		return result;

	if (hDecoder->object_type >= ER_OBJECT_START)
	{
		if (ics->tns_data_present)
			tns_data(ics, &(ics->tns), bs);
	}


	/* decode the spectral data */
	if ((result = spectral_data(hDecoder, ics, bs, spec_data)) > 0)
	{
		return result;
	}

	/* pulse coding reconstruction */
	if (ics->pulse_data_present)
	{
		if (ics->window_sequence != EIGHT_SHORT_SEQUENCE)
		{
			if ((result = pulse_decode(ics, spec_data, hDecoder->frameLength)) > 0)
				return result;
		}
		else {
			return 2; /* pulse coding not allowed for short blocks */
		}
	}

	return 0;
}

static uint8_t single_lfe_channel_element(NeAACDecStruct *hDecoder, CBitstream& bs, uint8_t channel, uint8_t *tag)
{
	uint8_t retval = 0;
	element sce = { 0 };
	ic_stream *ics = &(sce.ics1);
	int16_t spec_data[1024] = { 0 };

	sce.element_instance_tag = (uint8_t)bs.GetBits(4);

	*tag = sce.element_instance_tag;
	sce.channel = channel;
	sce.paired_channel = -1;

	retval = individual_channel_stream(hDecoder, &sce, bs, ics, 0, spec_data);
	if (retval > 0)
		return retval;

	/* IS not allowed in single channel */
	if (ics->is_used)
		return 32;

#ifdef SBR_DEC
	/* check if next bitstream element is a fill element */
	/* if so, read it now so SBR decoding can be done in case of a file with SBR */
	if (faad_showbits(ld, LEN_SE_ID) == ID_FIL)
	{
		faad_flushbits(ld, LEN_SE_ID);

		/* one sbr_info describes a channel_element not a channel! */
		if ((retval = fill_element(hDecoder, ld, hDecoder->drc, hDecoder->fr_ch_ele)) > 0)
		{
			return retval;
		}
	}
#endif

	return 0;
}

/* Table 4.4.5 */
static uint8_t channel_pair_element(NeAACDecStruct *hDecoder, CBitstream& bs, uint8_t channels, uint8_t *tag)
{
	ALIGN int16_t spec_data1[1024] = { 0 };
	ALIGN int16_t spec_data2[1024] = { 0 };
	element cpe = { 0 };
	ic_stream *ics1 = &(cpe.ics1);
	ic_stream *ics2 = &(cpe.ics2);
	uint8_t result;

	cpe.channel = channels;
	cpe.paired_channel = channels + 1;

	cpe.element_instance_tag = (uint8_t)bs.GetBits(LEN_TAG);
	*tag = cpe.element_instance_tag;

	if ((cpe.common_window = (uint8_t)bs.GetBits(1)) & 1)
	{
		/* both channels have common ics information */
		if ((result = ics_info(hDecoder, ics1, bs, cpe.common_window)) > 0)
			return result;

		ics1->ms_mask_present = (uint8_t)bs.GetBits(2);
		if (ics1->ms_mask_present == 3)
		{
			/* bitstream error */
			return 32;
		}
		if (ics1->ms_mask_present == 1)
		{
			uint8_t g, sfb;
			for (g = 0; g < ics1->num_window_groups; g++)
			{
				for (sfb = 0; sfb < ics1->max_sfb; sfb++)
				{
					ics1->ms_used[g][sfb] = (uint8_t)bs.GetBits(1);
				}
			}
		}

#ifdef ERROR_RESILIENCE
		if ((hDecoder->object_type >= ER_OBJECT_START) && (ics1->predictor_data_present))
		{
			if ((
#ifdef LTP_DEC
				ics1->ltp.data_present =
#endif
				faad_get1bit(ld DEBUGVAR(1, 50, "channel_pair_element(): ltp.data_present"))) & 1)
			{
#ifdef LTP_DEC
				if ((result = ltp_data(hDecoder, ics1, &(ics1->ltp), ld)) > 0)
				{
					return result;
				}
#else
				return 26;
#endif
			}
		}
#endif

		memcpy(ics2, ics1, sizeof(ic_stream));
	}
	else {
		ics1->ms_mask_present = 0;
	}

	if ((result = individual_channel_stream(hDecoder, &cpe, bs, ics1, 0, spec_data1)) > 0)
	{
		return result;
	}

#ifdef ERROR_RESILIENCE
	if (cpe.common_window && (hDecoder->object_type >= ER_OBJECT_START) &&
		(ics1->predictor_data_present))
	{
		if ((
#ifdef LTP_DEC
			ics1->ltp2.data_present =
#endif
			faad_get1bit(ld DEBUGVAR(1, 50, "channel_pair_element(): ltp.data_present"))) & 1)
		{
#ifdef LTP_DEC
			if ((result = ltp_data(hDecoder, ics1, &(ics1->ltp2), ld)) > 0)
			{
				return result;
			}
#else
			return 26;
#endif
		}
	}
#endif

	if ((result = individual_channel_stream(hDecoder, &cpe, bs, ics2, 0, spec_data2)) > 0)
	{
		return result;
	}

#ifdef SBR_DEC
	/* check if next bitstream element is a fill element */
	/* if so, read it now so SBR decoding can be done in case of a file with SBR */
	if (faad_showbits(ld, LEN_SE_ID) == ID_FIL)
	{
		faad_flushbits(ld, LEN_SE_ID);

		/* one sbr_info describes a channel_element not a channel! */
		if ((result = fill_element(hDecoder, ld, hDecoder->drc, hDecoder->fr_ch_ele)) > 0)
		{
			return result;
		}
	}
#endif

	/* noiseless coding is done, spectral reconstruction is done now */
	//if ((result = reconstruct_channel_pair(hDecoder, ics1, ics2, &cpe,
	//	spec_data1, spec_data2)) > 0)
	//{
	//	return result;
	//}

	return 0;
}

/* Table 4.4.2 */
/* An MPEG-4 Audio decoder is only required to follow the Program
Configuration Element in GASpecificConfig(). The decoder shall ignore
any Program Configuration Elements that may occur in raw data blocks.
PCEs transmitted in raw data blocks cannot be used to convey decoder
configuration information.
*/
static uint8_t program_config_element(program_config *pce, CBitstream &bs)
{
	uint8_t i;

	memset(pce, 0, sizeof(program_config));

	pce->channels = 0;

	pce->element_instance_tag = (uint8_t)bs.GetBits(4);

	pce->object_type = (uint8_t)bs.GetBits(2);
	pce->sf_index = (uint8_t)bs.GetBits(4);
	pce->num_front_channel_elements = (uint8_t)bs.GetBits(4);
	pce->num_side_channel_elements = (uint8_t)bs.GetBits(4);
	pce->num_back_channel_elements = (uint8_t)bs.GetBits(4);
	pce->num_lfe_channel_elements = (uint8_t)bs.GetBits(2);
	pce->num_assoc_data_elements = (uint8_t)bs.GetBits(3);
	pce->num_valid_cc_elements = (uint8_t)bs.GetBits(4);

	pce->mono_mixdown_present = (uint8_t)bs.GetBits(1);
	if (pce->mono_mixdown_present == 1)
	{
		pce->mono_mixdown_element_number = (uint8_t)bs.GetBits(4);
	}

	pce->stereo_mixdown_present = (uint8_t)bs.GetBits(1);
	if (pce->stereo_mixdown_present == 1)
	{
		pce->stereo_mixdown_element_number = (uint8_t)bs.GetBits(4);
	}

	pce->matrix_mixdown_idx_present = (uint8_t)bs.GetBits(1);
	if (pce->matrix_mixdown_idx_present == 1)
	{
		pce->matrix_mixdown_idx = (uint8_t)bs.GetBits(2);
		pce->pseudo_surround_enable = (uint8_t)bs.GetBits(1);
	}

	for (i = 0; i < pce->num_front_channel_elements; i++)
	{
		pce->front_element_is_cpe[i] = (uint8_t)bs.GetBits(1);
		pce->front_element_tag_select[i] = (uint8_t)bs.GetBits(4);

		if (pce->front_element_is_cpe[i] & 1)
		{
			pce->cpe_channel[pce->front_element_tag_select[i]] = pce->channels;
			pce->num_front_channels += 2;
			pce->channels += 2;
		}
		else {
			pce->sce_channel[pce->front_element_tag_select[i]] = pce->channels;
			pce->num_front_channels++;
			pce->channels++;
		}
	}

	for (i = 0; i < pce->num_side_channel_elements; i++)
	{
		pce->side_element_is_cpe[i] = (uint8_t)bs.GetBits(1);
		pce->side_element_tag_select[i] = (uint8_t)bs.GetBits(4);

		if (pce->side_element_is_cpe[i] & 1)
		{
			pce->cpe_channel[pce->side_element_tag_select[i]] = pce->channels;
			pce->num_side_channels += 2;
			pce->channels += 2;
		}
		else {
			pce->sce_channel[pce->side_element_tag_select[i]] = pce->channels;
			pce->num_side_channels++;
			pce->channels++;
		}
	}

	for (i = 0; i < pce->num_back_channel_elements; i++)
	{
		pce->back_element_is_cpe[i] = (uint8_t)bs.GetBits(1);
		pce->back_element_tag_select[i] = (uint8_t)bs.GetBits(4);

		if (pce->back_element_is_cpe[i] & 1)
		{
			pce->cpe_channel[pce->back_element_tag_select[i]] = pce->channels;
			pce->channels += 2;
			pce->num_back_channels += 2;
		}
		else {
			pce->sce_channel[pce->back_element_tag_select[i]] = pce->channels;
			pce->num_back_channels++;
			pce->channels++;
		}
	}

	for (i = 0; i < pce->num_lfe_channel_elements; i++)
	{
		pce->lfe_element_tag_select[i] = (uint8_t)bs.GetBits(4);

		pce->sce_channel[pce->lfe_element_tag_select[i]] = pce->channels;
		pce->num_lfe_channels++;
		pce->channels++;
	}

	for (i = 0; i < pce->num_assoc_data_elements; i++)
		pce->assoc_data_element_tag_select[i] = (uint8_t)bs.GetBits(4);

	for (i = 0; i < pce->num_valid_cc_elements; i++)
	{
		pce->cc_element_is_ind_sw[i] = (uint8_t)bs.GetBits(1);
		pce->valid_cc_element_tag_select[i] = (uint8_t)bs.GetBits(4);
	}

	uint64_t left_bits;
	bs.Tell(&left_bits);
	bs.SkipBits(left_bits % 8);

	pce->comment_field_bytes = (uint8_t)bs.GetBits(8);

	for (i = 0; i < pce->comment_field_bytes; i++)
	{
		pce->comment_field_data[i] = (uint8_t)bs.GetBits(8);
	}
	pce->comment_field_data[i] = 0;

	if (pce->channels > MAX_CHANNELS)
		return 22;

	return 0;
}

static void decode_sce_lfe(NeAACDecStruct *hDecoder, NeAACDecFrameInfo *hInfo, CBitstream& bs, uint8_t id_syn_ele)
{
	uint8_t channels = hDecoder->fr_channels;
	uint8_t tag = 0;

	if (channels + 1 > MAX_CHANNELS)
	{
		hInfo->error = 12;
		return;
	}
	if (hDecoder->fr_ch_ele + 1 > MAX_SYNTAX_ELEMENTS)
	{
		hInfo->error = 13;
		return;
	}

	/* for SCE hDecoder->element_output_channels[] is not set here because this
	can become 2 when some form of Parametric Stereo coding is used
	*/

	/* save the syntax element id */
	hDecoder->element_id[hDecoder->fr_ch_ele] = id_syn_ele;

	/* decode the element */
	hInfo->error = single_lfe_channel_element(hDecoder, bs, channels, &tag);

	/* map output channels position to internal data channels */
	if (hDecoder->element_output_channels[hDecoder->fr_ch_ele] == 2)
	{
		/* this might be faulty when pce_set is true */
		hDecoder->internal_channel[channels] = channels;
		hDecoder->internal_channel[channels + 1] = channels + 1;
	}
	else {
		if (hDecoder->pce_set)
			hDecoder->internal_channel[hDecoder->pce.sce_channel[tag]] = channels;
		else
			hDecoder->internal_channel[channels] = channels;
	}

	hDecoder->fr_channels += hDecoder->element_output_channels[hDecoder->fr_ch_ele];
	hDecoder->fr_ch_ele++;
}

static void decode_cpe(NeAACDecStruct *hDecoder, NeAACDecFrameInfo *hInfo, CBitstream& bs, uint8_t id_syn_ele)
{
	uint8_t channels = hDecoder->fr_channels;
	uint8_t tag = 0;

	if (channels + 2 > MAX_CHANNELS)
	{
		hInfo->error = 12;
		return;
	}
	if (hDecoder->fr_ch_ele + 1 > MAX_SYNTAX_ELEMENTS)
	{
		hInfo->error = 13;
		return;
	}

	/* for CPE the number of output channels is always 2 */
	if (hDecoder->element_output_channels[hDecoder->fr_ch_ele] == 0)
	{
		/* element_output_channels not set yet */
		hDecoder->element_output_channels[hDecoder->fr_ch_ele] = 2;
	}
	else if (hDecoder->element_output_channels[hDecoder->fr_ch_ele] != 2) {
		/* element inconsistency */
		hInfo->error = 21;
		return;
	}

	/* save the syntax element id */
	hDecoder->element_id[hDecoder->fr_ch_ele] = id_syn_ele;

	/* decode the element */
	hInfo->error = channel_pair_element(hDecoder, bs, channels, &tag);

	/* map output channel position to internal data channels */
	if (hDecoder->pce_set)
	{
		hDecoder->internal_channel[hDecoder->pce.cpe_channel[tag]] = channels;
		hDecoder->internal_channel[hDecoder->pce.cpe_channel[tag] + 1] = channels + 1;
	}
	else {
		hDecoder->internal_channel[channels] = channels;
		hDecoder->internal_channel[channels + 1] = channels + 1;
	}

	hDecoder->fr_channels += 2;
	hDecoder->fr_ch_ele++;
}

/* Table 4.4.10 */
static uint16_t data_stream_element(NeAACDecStruct *hDecoder, CBitstream& bs)
{
	uint8_t byte_aligned;
	uint16_t i, count;

	/* element_instance_tag = */ bs.GetBits(LEN_TAG);
	byte_aligned = (uint8_t)bs.GetBits(1);
	count = (uint16_t)bs.GetBits(8);
	if (count == 255)
	{
		count += (uint16_t)bs.GetBits(8);
	}
	if (byte_aligned)
	{
		uint64_t left_bits = 0;
		bs.Tell(&left_bits);
		bs.SkipBits(left_bits % 8);
	}

	for (i = 0; i < count; i++)
	{
		bs.GetBits(LEN_BYTE);
	}

	return count;
}

/* Table 4.4.32 */
static uint8_t excluded_channels(CBitstream& bs, drc_info *drc)
{
	uint8_t i, n = 0;
	uint8_t num_excl_chan = 7;

	for (i = 0; i < 7; i++)
	{
		drc->exclude_mask[i] = (uint8_t)bs.GetBits(1);
	}
	n++;

	while ((drc->additional_excluded_chns[n - 1] = (uint8_t)bs.GetBits(1)) == 1)
	{
		for (i = num_excl_chan; i < num_excl_chan + 7; i++)
		{
			drc->exclude_mask[i] = (uint8_t)bs.GetBits(1);
		}
		n++;
		num_excl_chan += 7;
		if (num_excl_chan + 7 > MAX_CHANNELS)
			throw std::exception();
	}

	return n;
}

/* Table 4.4.31 */
static uint8_t dynamic_range_info(CBitstream& bs, drc_info *drc)
{
	uint8_t i, n = 1;
	uint8_t band_incr;

	drc->num_bands = 1;

	if (bs.GetBits(1))
	{
		drc->pce_instance_tag = (uint8_t)bs.GetBits(4);
		/* drc->drc_tag_reserved_bits = */ bs.GetBits(4);
		n++;
	}

	drc->excluded_chns_present = (uint8_t)bs.GetBits(1);
	if (drc->excluded_chns_present == 1)
	{
		n += excluded_channels(bs, drc);
	}

	if (bs.GetBits(1))
	{
		band_incr = (uint8_t)bs.GetBits(4);
		/* drc->drc_bands_reserved_bits = */ bs.GetBits(4);
		n++;
		drc->num_bands += band_incr;

		for (i = 0; i < drc->num_bands; i++)
		{
			drc->band_top[i] = (uint8_t)bs.GetBits(8);
			n++;
		}
	}

	if (bs.GetBits(1))
	{
		drc->prog_ref_level = (uint8_t)bs.GetBits(7);
		/* drc->prog_ref_level_reserved_bits = */ bs.GetBits(1);
		n++;
	}

	for (i = 0; i < drc->num_bands; i++)
	{
		drc->dyn_rng_sgn[i] = (uint8_t)bs.GetBits(1);
		drc->dyn_rng_ctl[i] = (uint8_t)bs.GetBits(7);
		n++;
	}

	return n;
}

/* Table 4.4.30 */
static uint16_t extension_payload(CBitstream& bs, drc_info *drc, uint16_t count)
{
	uint16_t i, n, dataElementLength;
	uint8_t dataElementLengthPart;
	uint8_t align = 4, data_element_version, loopCounter;

	uint8_t extension_type = (uint8_t)bs.GetBits(4);

	switch (extension_type)
	{
	case EXT_DYNAMIC_RANGE:
		drc->present = 1;
		n = dynamic_range_info(bs, drc);
		return n;
	case EXT_FILL_DATA:
		/* fill_nibble = */ bs.GetBits(4); /* must be ‘0000’ */
		for (i = 0; i < count - 1; i++)
		{
			/* fill_byte[i] = */ bs.GetBits(8); /* must be ‘10100101’ */
		}
		return count;
	case EXT_DATA_ELEMENT:
		data_element_version = (uint8_t)bs.GetBits(4);
		switch (data_element_version)
		{
		case ANC_DATA:
			loopCounter = 0;
			dataElementLength = 0;
			do {
				dataElementLengthPart = (uint8_t)bs.GetBits(8);
				dataElementLength += dataElementLengthPart;
				loopCounter++;
			} while (dataElementLengthPart == 255);

			for (i = 0; i < dataElementLength; i++)
			{
				/* data_element_byte[i] = */ bs.GetBits(8);
				return (dataElementLength + loopCounter + 1);
			}
		default:
			align = 0;
		}
	case EXT_FIL:
	default:
		bs.GetBits(align);
		for (i = 0; i < count - 1; i++)
		{
			/* other_bits[i] = */ bs.GetBits(8);
		}
		return count;
	}
}

/* Table 4.4.11 */
static uint8_t fill_element(NeAACDecStruct *hDecoder, CBitstream& bs, drc_info *drc
#ifdef SBR_DEC
	, uint8_t sbr_ele
#endif
)
{
	uint16_t count;
#ifdef SBR_DEC
	uint8_t bs_extension_type;
#endif

	count = (uint16_t)bs.GetBits(4);
	if (count == 15)
	{
		count += (uint16_t)bs.GetBits(8) - 1;
	}

	if (count > 0)
	{
#ifdef SBR_DEC
		bs_extension_type = (uint8_t)faad_showbits(ld, 4);

		if ((bs_extension_type == EXT_SBR_DATA) ||
			(bs_extension_type == EXT_SBR_DATA_CRC))
		{
			if (sbr_ele == INVALID_SBR_ELEMENT)
				return 24;

			if (!hDecoder->sbr[sbr_ele])
			{
				hDecoder->sbr[sbr_ele] = sbrDecodeInit(hDecoder->frameLength,
					hDecoder->element_id[sbr_ele], 2 * get_sample_rate(hDecoder->sf_index),
					hDecoder->downSampledSBR
#ifdef DRM
					, 0
#endif
				);
			}

			hDecoder->sbr_present_flag = 1;

			/* parse the SBR data */
			hDecoder->sbr[sbr_ele]->ret = sbr_extension_data(ld, hDecoder->sbr[sbr_ele], count,
				hDecoder->postSeekResetFlag);

#if 0
			if (hDecoder->sbr[sbr_ele]->ret > 0)
			{
				printf("%s\n", NeAACDecGetErrorMessage(hDecoder->sbr[sbr_ele]->ret));
			}
#endif

#if (defined(PS_DEC) || defined(DRM_PS))
			if (hDecoder->sbr[sbr_ele]->ps_used)
			{
				hDecoder->ps_used[sbr_ele] = 1;

				/* set element independent flag to 1 as well */
				hDecoder->ps_used_global = 1;
			}
#endif
		}
		else {
#endif
#ifndef DRM
			while (count > 0)
			{
				count -= extension_payload(bs, drc, count);
			}
#else
			return 30;
#endif
#ifdef SBR_DEC
		}
#endif
	}

	return 0;
}

#ifdef COUPLING_DEC
/* Table 4.4.8: Currently just for skipping the bits... */
static uint8_t coupling_channel_element(NeAACDecStruct *hDecoder, CBitstream& bs)
{
	uint8_t c, result = 0;
	uint8_t ind_sw_cce_flag = 0;
	uint8_t num_gain_element_lists = 0;
	uint8_t num_coupled_elements = 0;

	element el_empty = { 0 };
	ic_stream ics_empty = { 0 };
	int16_t sh_data[1024];

	c = (uint8_t)bs.GetBits(LEN_TAG);

	ind_sw_cce_flag = (uint8_t)bs.GetBits(1);
	num_coupled_elements = (uint8_t)bs.GetBits(3);

	for (c = 0; c < num_coupled_elements + 1; c++)
	{
		uint8_t cc_target_is_cpe, cc_target_tag_select;

		num_gain_element_lists++;

		cc_target_is_cpe = (uint8_t)bs.GetBits(1);
		cc_target_tag_select = (uint8_t)bs.GetBits(4);

		if (cc_target_is_cpe)
		{
			uint8_t cc_l = (uint8_t)bs.GetBits(1);
			uint8_t cc_r = (uint8_t)bs.GetBits(1);

			if (cc_l && cc_r)
				num_gain_element_lists++;
		}
	}

	(uint8_t)bs.GetBits(1);
	(uint8_t)bs.GetBits(1);
	(uint8_t)bs.GetBits(2);

	if ((result = individual_channel_stream(hDecoder, &el_empty, bs, &ics_empty, 0, sh_data)) > 0)
	{
		return result;
	}

	/* IS not allowed in single channel */
	if (ics_empty.is_used)
		return 32;

	for (c = 1; c < num_gain_element_lists; c++)
	{
		uint8_t cge;

		if (ind_sw_cce_flag)
		{
			cge = 1;
		}
		else {
			cge = (uint8_t)bs.GetBits(1);
		}

		if (cge)
		{
			huffman_scale_factor(bs);
		}
		else {
			uint8_t g, sfb;

			for (g = 0; g < ics_empty.num_window_groups; g++)
			{
				for (sfb = 0; sfb < ics_empty.max_sfb; sfb++)
				{
					if (ics_empty.sfb_cb[g][sfb] != ZERO_HCB)
						huffman_scale_factor(bs);
				}
			}
		}
	}

	return 0;
}
#endif

void raw_data_block(NeAACDecStruct *hDecoder, NeAACDecFrameInfo *hInfo, CBitstream& bs, program_config *pce, drc_info *drc)
{
	uint8_t id_syn_ele;
	uint8_t ele_this_frame = 0;

	hDecoder->fr_channels = 0;
	hDecoder->fr_ch_ele = 0;
	hDecoder->first_syn_ele = 25;
	hDecoder->has_lfe = 0;

#ifdef ERROR_RESILIENCE
	if (hDecoder->object_type < ER_OBJECT_START)
	{
#endif
		/* Table 4.4.3: raw_data_block() */
		while ((id_syn_ele = (uint8_t)bs.GetBits(LEN_SE_ID)) != ID_END)
		{
			switch (id_syn_ele) {
			case ID_SCE:
				ele_this_frame++;
				if (hDecoder->first_syn_ele == 25) hDecoder->first_syn_ele = id_syn_ele;
				decode_sce_lfe(hDecoder, hInfo, bs, id_syn_ele);
				if (hInfo->error > 0)
					return;
				break;
			case ID_CPE:
				ele_this_frame++;
				if (hDecoder->first_syn_ele == 25) hDecoder->first_syn_ele = id_syn_ele;
				decode_cpe(hDecoder, hInfo, bs, id_syn_ele);
				if (hInfo->error > 0)
					return;
				break;
			case ID_LFE:
#ifdef DRM
				hInfo->error = 32;
#else
				ele_this_frame++;
				hDecoder->has_lfe++;
				decode_sce_lfe(hDecoder, hInfo, bs, id_syn_ele);
#endif
				if (hInfo->error > 0)
					return;
				break;
			case ID_CCE: /* not implemented yet, but skip the bits */
#ifdef DRM
				hInfo->error = 32;
#else
				ele_this_frame++;
#ifdef COUPLING_DEC
				hInfo->error = coupling_channel_element(hDecoder, bs);
#else
				hInfo->error = 6;
#endif
#endif
				if (hInfo->error > 0)
					return;
				break;
			case ID_DSE:
				ele_this_frame++;
				data_stream_element(hDecoder, bs);
				break;
			case ID_PCE:
				if (ele_this_frame != 0)
				{
					hInfo->error = 31;
					return;
				}
				ele_this_frame++;
				/* 14496-4: 5.6.4.1.2.1.3: */
				/* program_configuration_element()'s in access units shall be ignored */
				program_config_element(pce, bs);
				//if ((hInfo->error = program_config_element(pce, ld)) > 0)
				//    return;
				//hDecoder->pce_set = 1;
				break;
			case ID_FIL:
				ele_this_frame++;
				/* one sbr_info describes a channel_element not a channel! */
				/* if we encounter SBR data here: error */
				/* SBR data will be read directly in the SCE/LFE/CPE element */
				if ((hInfo->error = fill_element(hDecoder, bs, drc
#ifdef SBR_DEC
					, INVALID_SBR_ELEMENT
#endif
				)) > 0)
					return;
				break;
			}
		}
#ifdef ERROR_RESILIENCE
	}
	else {
		/* Table 262: er_raw_data_block() */
		switch (hDecoder->channelConfiguration)
		{
		case 1:
			decode_sce_lfe(hDecoder, hInfo, ld, ID_SCE);
			if (hInfo->error > 0)
				return;
			break;
		case 2:
			decode_cpe(hDecoder, hInfo, ld, ID_CPE);
			if (hInfo->error > 0)
				return;
			break;
		case 3:
			decode_sce_lfe(hDecoder, hInfo, ld, ID_SCE);
			decode_cpe(hDecoder, hInfo, ld, ID_CPE);
			if (hInfo->error > 0)
				return;
			break;
		case 4:
			decode_sce_lfe(hDecoder, hInfo, ld, ID_SCE);
			decode_cpe(hDecoder, hInfo, ld, ID_CPE);
			decode_sce_lfe(hDecoder, hInfo, ld, ID_SCE);
			if (hInfo->error > 0)
				return;
			break;
		case 5:
			decode_sce_lfe(hDecoder, hInfo, ld, ID_SCE);
			decode_cpe(hDecoder, hInfo, ld, ID_CPE);
			decode_cpe(hDecoder, hInfo, ld, ID_CPE);
			if (hInfo->error > 0)
				return;
			break;
		case 6:
			decode_sce_lfe(hDecoder, hInfo, ld, ID_SCE);
			decode_cpe(hDecoder, hInfo, ld, ID_CPE);
			decode_cpe(hDecoder, hInfo, ld, ID_CPE);
			decode_sce_lfe(hDecoder, hInfo, ld, ID_LFE);
			if (hInfo->error > 0)
				return;
			break;
		case 7: /* 8 channels */
			decode_sce_lfe(hDecoder, hInfo, ld, ID_SCE);
			decode_cpe(hDecoder, hInfo, ld, ID_CPE);
			decode_cpe(hDecoder, hInfo, ld, ID_CPE);
			decode_cpe(hDecoder, hInfo, ld, ID_CPE);
			decode_sce_lfe(hDecoder, hInfo, ld, ID_LFE);
			if (hInfo->error > 0)
				return;
			break;
		default:
			hInfo->error = 7;
			return;
		}
#if 0
		cnt = bits_to_decode() / 8;
		while (cnt >= 1)
		{
			cnt -= extension_payload(cnt);
		}
#endif
	}
#endif

	/* new in corrigendum 14496-3:2002 */
#ifdef DRM
	if (hDecoder->object_type != DRM_ER_LC
#if 0
		&& !hDecoder->latm_header_present
#endif
		)
#endif
	{
		uint64_t left_bits = 0;
		uint64_t bit_pos = bs.Tell(&left_bits);
		bs.SkipBits(left_bits % 8);
	}

	return;
}