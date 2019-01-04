#include "StdAfx.h"
#include "Bitstream.h"
#include <exception>
#include <assert.h>
#include <iostream>

CBitstream::CBitstream()
	: CBitstream(NULL, 0)
{}

CBitstream::CBitstream(uint8_t* pBuf, size_t cbitSize)
{
	size_t cbSize = (cbitSize + 7) >> 3;

	cursor.start_offset = ((intptr_t)pBuf & 3);
	cursor.p = cursor.p_start = pBuf?((uint8_t*)pBuf - cursor.start_offset):nullptr;
	cursor.p_end = pBuf?(pBuf + cbSize):nullptr;
	cursor.bits_left = (sizeof(CURBITS_TYPE) - cursor.start_offset) * 8;

	cursor.curbits = pBuf?CURBITS_VALUE(cursor.p):0;

	if ((size_t)cursor.bits_left >= cbitSize)
	{
		/*
		|--------|--------|--------|--------|
		p          r
		           ^++++++ bits_left +++++++^
		           ^+++++nBits+++++++++^
		*/
		cursor.exclude_bits = (uint8_t)(cursor.bits_left - cbitSize);
	}
	else
	{
		/*
		|--------|--------|--------|--------|.......|--------|--------|--------|--------|
		p          r
		           ^++++++ bits_left +++++++^
		           ^+++++nBits++++++++++++++++++++++++++++++^
		*/
		size_t nLeftBits = cbitSize - cursor.bits_left;
		cursor.exclude_bits = (uint8_t)(((nLeftBits + 7) & 0xFFFFFFF8) - nLeftBits);
	}

	memset(&save_point, 0, sizeof(save_point));
}

int CBitstream::GetAllLeftBits()
{
	int bits_left = cursor.p_end <= cursor.p ? 0 : (((int)(cursor.p_end - cursor.p)) << 3);
	return bits_left <= cursor.exclude_bits ? 0
		: (bits_left < (int)(sizeof(CURBITS_TYPE) << 3) ? (cursor.bits_left - cursor.exclude_bits)
			: (bits_left - (int)(sizeof(CURBITS_TYPE) << 3) + cursor.bits_left - cursor.exclude_bits));
}

void CBitstream::_UpdateCurBits(bool bEos)
{
	if (cursor.p + sizeof(CURBITS_TYPE) <= cursor.p_end)
	{
		cursor.curbits = CURBITS_VALUE(cursor.p);
		cursor.bits_left = sizeof(CURBITS_TYPE) * 8;
	}
	else
	{
		cursor.curbits = 0;
		for (uint8_t* pbyte = cursor.p; pbyte < cursor.p_end; pbyte++) {
			cursor.curbits <<= 8;
			cursor.curbits |= *(uint8_t*)pbyte;
			cursor.bits_left += 8;
		}
	}
}

void CBitstream::_Advance_InCacheBits(int n)
{
	cursor.bits_left -= n;
}

void CBitstream::_FillCurrentBits(bool bPeek)
{
	// Fill the curbits
	// It may be also external buffer or inner buffer type except the read-out ring-chunk buffer type.
	if (cursor.p < cursor.p_end)
	{
		assert(cursor.bits_left == 0);
		if (cursor.p_end <= cursor.p + sizeof(CURBITS_TYPE))
			cursor.p = cursor.p_end;
		else
			cursor.p += sizeof(CURBITS_TYPE);

		_UpdateCurBits();
	}
}

uint64_t CBitstream::_GetBits(int n, bool bPeek, bool bThrowExceptionHitStartCode)
{
	UNREFERENCED_PARAMETER(bThrowExceptionHitStartCode);

	uint64_t nRet = 0;

	if (bPeek)
	{
		// Make sure there is no active data in save point of bitstream
		assert(save_point.p == NULL);
	}

	if (cursor.bits_left == 0)
		_UpdateCurBits();

	// Activate a save_point for the current bit-stream cursor
	if (bPeek)
		save_point = cursor;

	while (n > 0 && cursor.bits_left > 0)
	{
		int nProcessed = std::min(n, cursor.bits_left);
		CURBITS_TYPE nMask = CURBITS_MASK(cursor.bits_left);
		nRet <<= nProcessed;
		nRet |= (cursor.curbits&nMask) >> (cursor.bits_left - nProcessed);
		_Advance_InCacheBits(nProcessed);

		if (cursor.bits_left == 0)
		{
			_FillCurrentBits(bPeek);
		}

		n -= nProcessed;
	}

	// Restore the save point
	if (save_point.p != NULL && bPeek == true)
	{
		cursor = save_point;
	}

	if (n != 0)
		throw std::out_of_range("invalid parameter, no enough data");

	return nRet;
}

int64_t CBitstream::SkipBits(int64_t skip_bits)
{
	int64_t ret_skip_bits = skip_bits;
	while (skip_bits > 0 && cursor.p < cursor.p_end)
	{
		int nProcessed = (int)std::min(skip_bits, (int64_t)cursor.bits_left);
		_Advance_InCacheBits(nProcessed);

		if (cursor.bits_left == 0)
		{
			_FillCurrentBits();
		}

		skip_bits -= nProcessed;
	}

	ret_skip_bits -= skip_bits;

	return ret_skip_bits;
}

uint64_t CBitstream::GetBits(int n)
{
	if (n > (int)sizeof(uint64_t) * 8)
		throw std::invalid_argument("invalid parameter");

	return _GetBits(n);
}

uint64_t CBitstream::PeekBits(int n)
{
	if (n > (int)sizeof(uint64_t) * 8)
		throw std::invalid_argument("invalid parameter");

	if (n == 0)
		return 0;

	// Clean up the previous save point
	CleanSavePoint();

	return _GetBits(n, true);
}

uint64_t CBitstream::Tell(uint64_t* left_bits_in_bst)
{
	if (cursor.p_start == NULL || cursor.p == NULL)
	{
		if (left_bits_in_bst)
			*left_bits_in_bst = 0;
		return 0;
	}

	int nAllLeftBits = GetAllLeftBits();
	if (left_bits_in_bst != NULL)
		*left_bits_in_bst = nAllLeftBits;

	return (uint64_t)(8 * (cursor.p_end - cursor.p_start - cursor.start_offset) - nAllLeftBits);
}

int CBitstream::Seek(uint64_t bit_pos)
{
	if (bit_pos > (uint64_t)(cursor.p_end - cursor.p_start - cursor.start_offset) * 8)
		return -1;

	if (bit_pos == (uint64_t)-1LL)
		bit_pos = (uint64_t)(cursor.p_end - cursor.p_start - cursor.start_offset) * 8;

	uint8_t* ptr_dest = cursor.p_start + (bit_pos + cursor.start_offset * 8) / (sizeof(CURBITS_TYPE) * 8) * sizeof(CURBITS_TYPE);
	size_t bytes_left = (size_t)(cursor.p_end - cursor.p);
	size_t bits_left = std::min(bytes_left, sizeof(CURBITS_TYPE)) * 8 - (bit_pos + cursor.start_offset * 8) % (sizeof(CURBITS_TYPE) * 8);

	cursor.p = ptr_dest;
	_UpdateCurBits();
	cursor.bits_left = (int)bits_left;

	return 0;
}

int CBitstream::Read(uint8_t* buffer, int cbSize)
{
	if (cbSize < 0)
		return RET_CODE_INVALID_PARAMETER;

	if (cursor.bits_left % 8 != 0)
		return RET_CODE_NEEDBYTEALIGN;

	// Wrong pointer
	if (cursor.p_end < cursor.p || cursor.p_end > cursor.p + INT32_MAX)
		return RET_CODE_ERROR;

	int orig_size = cbSize;
	
	do
	{
		uint8_t* p_start = cursor.p + AMP_MIN((int)(cursor.p_end - cursor.p), (int)sizeof(CURBITS_TYPE)) - cursor.bits_left / 8;

		int left_bytes = cursor.p_end >= cursor.p + sizeof(CURBITS_TYPE) ?
			((int)(cursor.p_end - cursor.p - sizeof(CURBITS_TYPE)) + cursor.bits_left / 8) : cursor.bits_left / 8;

		int skip_bytes = AMP_MIN(left_bytes, cbSize);
		if (cursor.p_end > p_start)
		{
			memcpy(buffer, p_start, skip_bytes);
			buffer += skip_bytes;
			cbSize -= skip_bytes;
		}

		if (SkipBits((int64_t)skip_bytes << 3) <= 0)
			break;

	} while (cbSize > 0);

	return orig_size - cbSize;
}

CBitstream::~CBitstream()
{
}
