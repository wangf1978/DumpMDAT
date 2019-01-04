#ifndef __AAC_SYNTAX_H__
#define __AAC_SYNTAX_H__

#include <stdint.h>
#include "AACStruct.h"
#include "Bitstream.h"

void raw_data_block(NeAACDecStruct *hDecoder, NeAACDecFrameInfo *hInfo,	CBitstream& bs, program_config *pce, drc_info *drc);

#endif
