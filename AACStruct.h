#ifndef __AAC_STRUCT_H__
#define __AAC_STRUCT_H__

#include <stdint.h>

#if 0 //defined(_WIN32) && !defined(_WIN32_WCE)
#define ALIGN __declspec(align(16))
#else
#define ALIGN
#endif

#if 1
#define INLINE __inline
#else
#define INLINE inline
#endif

#define MAIN_DEC
#define LTP_DEC
#define SSR_DEC
#define COUPLING_DEC

#define MAX_CHANNELS        64
#define MAX_SYNTAX_ELEMENTS 48
#define MAX_WINDOW_GROUPS    8
#define MAX_SFB             51
#define MAX_LTP_SFB         40
#define MAX_LTP_SFB_S        8

/* library output formats */
#define FAAD_FMT_16BIT  1
#define FAAD_FMT_24BIT  2
#define FAAD_FMT_32BIT  3
#define FAAD_FMT_FLOAT  4
#define FAAD_FMT_FIXED  FAAD_FMT_FLOAT
#define FAAD_FMT_DOUBLE 5

#define MAIN       1
#define LC         2
#define SSR        3
#define LTP        4
#define HE_AAC     5
#define LD        23
#define ER_LC     17
#define ER_LTP    19
#define DRM_ER_LC 27 /* special object type for DRM */

#define ONLY_LONG_SEQUENCE   0x0
#define LONG_START_SEQUENCE  0x1
#define EIGHT_SHORT_SEQUENCE 0x2
#define LONG_STOP_SEQUENCE   0x3

#define ZERO_HCB       0
#define FIRST_PAIR_HCB 5
#define ESC_HCB        11
#define QUAD_LEN       4
#define PAIR_LEN       2
#define NOISE_HCB      13
#define INTENSITY_HCB2 14
#define INTENSITY_HCB  15

#define INVALID_SBR_ELEMENT 255

typedef float real_t;
#define MUL_R(A,B) ((A)*(B))
#define MUL_C(A,B) ((A)*(B))
#define MUL_F(A,B) ((A)*(B))

#define REAL_CONST(A) ((real_t)(A))
#define COEF_CONST(A) ((real_t)(A))
#define Q2_CONST(A) ((real_t)(A))
#define FRAC_CONST(A) ((real_t)(A)) /* pure fractional part */

typedef real_t complex_t[2];
#define RE(A) A[0]
#define IM(A) A[1]

/* used to save the prediction state */
typedef struct {
	int16_t r[2];
	int16_t COR[2];
	int16_t VAR[2];
} pred_state;

typedef struct
{
	uint16_t n;
	uint16_t ifac[15];
	complex_t *work;
	complex_t *tab;
} cfft_info;

typedef struct {
	uint16_t N;
	cfft_info *cfft;
	complex_t *sincos;
#ifdef PROFILE
	int64_t cycles;
	int64_t fft_cycles;
#endif
} mdct_info;

typedef struct
{
	const real_t *long_window[2];
	const real_t *short_window[2];
#ifdef LD_DEC
	const real_t *ld_window[2];
#endif

	mdct_info *mdct256;
#ifdef LD_DEC
	mdct_info *mdct1024;
#endif
	mdct_info *mdct2048;
#ifdef PROFILE
	int64_t cycles;
#endif
} fb_info;

typedef struct
{
	uint8_t present;

	uint8_t num_bands;
	uint8_t pce_instance_tag;
	uint8_t excluded_chns_present;
	uint8_t band_top[17];
	uint8_t prog_ref_level;
	uint8_t dyn_rng_sgn[17];
	uint8_t dyn_rng_ctl[17];
	uint8_t exclude_mask[MAX_CHANNELS];
	uint8_t additional_excluded_chns[MAX_CHANNELS];

	real_t ctrl1;
	real_t ctrl2;
} drc_info;

typedef struct
{
	uint8_t element_instance_tag;
	uint8_t object_type;
	uint8_t sf_index;
	uint8_t num_front_channel_elements;
	uint8_t num_side_channel_elements;
	uint8_t num_back_channel_elements;
	uint8_t num_lfe_channel_elements;
	uint8_t num_assoc_data_elements;
	uint8_t num_valid_cc_elements;
	uint8_t mono_mixdown_present;
	uint8_t mono_mixdown_element_number;
	uint8_t stereo_mixdown_present;
	uint8_t stereo_mixdown_element_number;
	uint8_t matrix_mixdown_idx_present;
	uint8_t pseudo_surround_enable;
	uint8_t matrix_mixdown_idx;
	uint8_t front_element_is_cpe[16];
	uint8_t front_element_tag_select[16];
	uint8_t side_element_is_cpe[16];
	uint8_t side_element_tag_select[16];
	uint8_t back_element_is_cpe[16];
	uint8_t back_element_tag_select[16];
	uint8_t lfe_element_tag_select[16];
	uint8_t assoc_data_element_tag_select[16];
	uint8_t cc_element_is_ind_sw[16];
	uint8_t valid_cc_element_tag_select[16];

	uint8_t channels;

	uint8_t comment_field_bytes;
	uint8_t comment_field_data[257];

	/* extra added values */
	uint8_t num_front_channels;
	uint8_t num_side_channels;
	uint8_t num_back_channels;
	uint8_t num_lfe_channels;
	uint8_t sce_channel[16];
	uint8_t cpe_channel[16];
} program_config;

#ifdef LTP_DEC
typedef struct
{
	uint8_t last_band;
	uint8_t data_present;
	uint16_t lag;
	uint8_t lag_update;
	uint8_t coef;
	uint8_t long_used[MAX_SFB];
	uint8_t short_used[8];
	uint8_t short_lag_present[8];
	uint8_t short_lag[8];
} ltp_info;
#endif

#ifdef MAIN_DEC
typedef struct
{
	uint8_t limit;
	uint8_t predictor_reset;
	uint8_t predictor_reset_group_number;
	uint8_t prediction_used[MAX_SFB];
} pred_info;
#endif

typedef struct
{
	uint8_t number_pulse;
	uint8_t pulse_start_sfb;
	uint8_t pulse_offset[4];
	uint8_t pulse_amp[4];
} pulse_info;

typedef struct
{
	uint8_t n_filt[8];
	uint8_t coef_res[8];
	uint8_t length[8][4];
	uint8_t order[8][4];
	uint8_t direction[8][4];
	uint8_t coef_compress[8][4];
	uint8_t coef[8][4][32];
} tns_info;

#ifdef SSR_DEC
typedef struct
{
	uint8_t max_band;

	uint8_t adjust_num[4][8];
	uint8_t alevcode[4][8][8];
	uint8_t aloccode[4][8][8];
} ssr_info;
#endif

typedef struct
{
	uint8_t max_sfb;

	uint8_t num_swb;
	uint8_t num_window_groups;
	uint8_t num_windows;
	uint8_t window_sequence;
	uint8_t window_group_length[8];
	uint8_t window_shape;
	uint8_t scale_factor_grouping;
	uint16_t sect_sfb_offset[8][15 * 8];
	uint16_t swb_offset[52];
	uint16_t swb_offset_max;

	uint8_t sect_cb[8][15 * 8];
	uint16_t sect_start[8][15 * 8];
	uint16_t sect_end[8][15 * 8];
	uint8_t sfb_cb[8][8 * 15];
	uint8_t num_sec[8]; /* number of sections in a group */

	uint8_t global_gain;
	int16_t scale_factors[8][51]; /* [0..255] */

	uint8_t ms_mask_present;
	uint8_t ms_used[MAX_WINDOW_GROUPS][MAX_SFB];

	uint8_t noise_used;
	uint8_t is_used;

	uint8_t pulse_data_present;
	uint8_t tns_data_present;
	uint8_t gain_control_data_present;
	uint8_t predictor_data_present;

	pulse_info pul;
	tns_info tns;
#ifdef MAIN_DEC
	pred_info pred;
#endif
#ifdef LTP_DEC
	ltp_info ltp;
	ltp_info ltp2;
#endif
#ifdef SSR_DEC
	ssr_info ssr;
#endif


} ic_stream; /* individual channel stream */

typedef struct
{
	uint8_t channel;
	int16_t paired_channel;

	uint8_t element_instance_tag;
	uint8_t common_window;

	ic_stream ics1;
	ic_stream ics2;
} element; /* syntax element (SCE, CPE, LFE) */

#define MAX_ASC_BYTES 64
typedef struct {
	int inited;
	int version, versionA;
	int framelen_type;
	int useSameStreamMux;
	int allStreamsSameTimeFraming;
	int numSubFrames;
	int numPrograms;
	int numLayers;
	int otherDataPresent;
	uint32_t otherDataLenBits;
	uint32_t frameLength;
	uint8_t ASC[MAX_ASC_BYTES];
	uint32_t ASCbits;
} latm_header;

typedef struct NeAACDecConfiguration
{
	unsigned char defObjectType;
	unsigned long defSampleRate;
	unsigned char outputFormat;
	unsigned char downMatrix;
	unsigned char useOldADTSFormat;
	unsigned char dontUpSampleImplicitSBR;
} NeAACDecConfiguration, *NeAACDecConfigurationPtr;

struct NeAACDecStruct
{
	uint8_t adts_header_present;
	uint8_t adif_header_present;
	uint8_t latm_header_present;
	uint8_t sf_index;
	uint8_t object_type;
	uint8_t channelConfiguration;
#ifdef ERROR_RESILIENCE
	uint8_t aacSectionDataResilienceFlag;
	uint8_t aacScalefactorDataResilienceFlag;
	uint8_t aacSpectralDataResilienceFlag;
#endif
	uint16_t frameLength;
	uint8_t postSeekResetFlag;

	uint32_t frame;

	uint8_t downMatrix;
	uint8_t upMatrix;
	uint8_t first_syn_ele;
	uint8_t has_lfe;
	/* number of channels in current frame */
	uint8_t fr_channels;
	/* number of elements in current frame */
	uint8_t fr_ch_ele;

	/* element_output_channels:
	determines the number of channels the element will output
	*/
	uint8_t element_output_channels[MAX_SYNTAX_ELEMENTS];
	/* element_alloced:
	determines whether the data needed for the element is allocated or not
	*/
	uint8_t element_alloced[MAX_SYNTAX_ELEMENTS];
	/* alloced_channels:
	determines the number of channels where output data is allocated for
	*/
	uint8_t alloced_channels;

	/* output data buffer */
	void *sample_buffer;

	uint8_t window_shape_prev[MAX_CHANNELS];
#ifdef LTP_DEC
	uint16_t ltp_lag[MAX_CHANNELS];
#endif
	fb_info *fb;
	drc_info *drc;

	real_t *time_out[MAX_CHANNELS];
	real_t *fb_intermed[MAX_CHANNELS];

#ifdef SBR_DEC
	int8_t sbr_present_flag;
	int8_t forceUpSampling;
	int8_t downSampledSBR;
	/* determines whether SBR data is allocated for the gives element */
	uint8_t sbr_alloced[MAX_SYNTAX_ELEMENTS];

	sbr_info *sbr[MAX_SYNTAX_ELEMENTS];
#endif
#if (defined(PS_DEC) || defined(DRM_PS))
	uint8_t ps_used[MAX_SYNTAX_ELEMENTS];
	uint8_t ps_used_global;
#endif

#ifdef SSR_DEC
	real_t *ssr_overlap[MAX_CHANNELS];
	real_t *prev_fmd[MAX_CHANNELS];
	real_t ipqf_buffer[MAX_CHANNELS][4][96 / 4];
#endif

#ifdef MAIN_DEC
	pred_state *pred_stat[MAX_CHANNELS];
#endif
#ifdef LTP_DEC
	int16_t *lt_pred_stat[MAX_CHANNELS];
#endif

#ifdef DRM
	uint8_t error_state;
#endif

	/* RNG states */
	uint32_t __r1;
	uint32_t __r2;

	/* Program Config Element */
	uint8_t pce_set;
	program_config pce;
	uint8_t element_id[MAX_CHANNELS];
	uint8_t internal_channel[MAX_CHANNELS];

	/* Configuration data */
	NeAACDecConfiguration config;

#ifdef PROFILE
	int64_t cycles;
	int64_t spectral_cycles;
	int64_t output_cycles;
	int64_t scalefac_cycles;
	int64_t requant_cycles;
#endif
	latm_header latm_config;
	const unsigned char *cmes;
};

typedef struct NeAACDecFrameInfo
{
	unsigned long bytesconsumed;
	unsigned long samples;
	unsigned char channels;
	unsigned char error;
	unsigned long samplerate;

	/* SBR: 0: off, 1: on; upsample, 2: on; downsampled, 3: off; upsampled */
	unsigned char sbr;

	/* MPEG-4 ObjectType */
	unsigned char object_type;

	/* AAC header type; MP4 will be signalled as RAW also */
	unsigned char header_type;

	/* multichannel configuration */
	unsigned char num_front_channels;
	unsigned char num_side_channels;
	unsigned char num_back_channels;
	unsigned char num_lfe_channels;
	unsigned char channel_position[64];

	/* PS: 0: off, 1: on */
	unsigned char ps;
} NeAACDecFrameInfo;

#endif
