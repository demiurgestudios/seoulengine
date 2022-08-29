/**
 * \file SeoulMD5.h
 * \brief Implementation of an MD5 hash.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

// Based on: http://www.fourmilab.ch/md5/
/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.	This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */
/* Brutally hacked by John Walker back from ANSI C to K&R (no prototypes) to maintain the tradition that Netfone will compile with Sun's original "cc". */

#pragma once
#ifndef SEOUL_MD5_H
#define SEOUL_MD5_H

#include "FixedArray.h"
#include "Prereqs.h"
#include "SeoulHString.h"
#include "SeoulString.h"

namespace Seoul
{

/**
 * Build up an MD5 hash, result is written to the String passed by
 * reference to MD5() at destruction.
 */
class MD5 SEOUL_SEALED
{
public:
	static const UInt32 kResultSize = 16u;
	static const UInt32 kBlockSize = 64u;

	MD5(FixedArray<UInt8, kResultSize>& rOutput);
	~MD5();

	void AppendData(UInt8 const* pData, UInt32 zDataSizeInBytes);

	void AppendData(void const* pData, UInt32 zDataSizeInBytes)
	{
		AppendData((UInt8 const*)pData, zDataSizeInBytes);
	}

	void AppendData(Byte const* s)
	{
		AppendData(s, StrLen(s));
	}

	void AppendData(HString h)
	{
		AppendData(h.CStr(), h.GetSizeInBytes());
	}

	void AppendData(const String& s)
	{
		AppendData(s.CStr(), s.GetSize());
	}

	template <typename T>
	void AppendPOD(const T& v)
	{
		AppendData(&v, sizeof(v));
	}

private:
	FixedArray<UInt8, kResultSize>& m_rOutput;
	FixedArray<UInt32, 4> m_aState;
	FixedArray<UInt8, kBlockSize> m_aBuffer;
	FixedArray<UInt32, 2u> m_aBits;
}; // class MD5

} // namespace Seoul

#endif // include guard
