/**
 * \file SeoulMD5.cpp
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
 *  except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */
/* Brutally hacked by John Walker back from ANSI C to K&R (no prototypes) to maintain the tradition that Netfone will compile with Sun's original "cc". */

#include "SeoulMD5.h"

namespace Seoul
{

static const UInt32 S11 = 7;
static const UInt32 S12 = 12;
static const UInt32 S13 = 17;
static const UInt32 S14 = 22;
static const UInt32 S21 = 5;
static const UInt32 S22 = 9;
static const UInt32 S23 = 14;
static const UInt32 S24 = 20;
static const UInt32 S31 = 4;
static const UInt32 S32 = 11;
static const UInt32 S33 = 16;
static const UInt32 S34 = 23;
static const UInt32 S41 = 6;
static const UInt32 S42 = 10;
static const UInt32 S43 = 15;
static const UInt32 S44 = 21;

// F, G, H and I are basic MD5 functions.
inline static UInt32 F(UInt32 x, UInt32 y, UInt32 z)
{
	return (x & y) | (~x & z);
}

inline static UInt32 G(UInt32 x, UInt32 y, UInt32 z)
{
	return (x & z) | (y & ~z);
}

inline static UInt32 H(UInt32 x, UInt32 y, UInt32 z)
{
	return x ^ y ^ z;
}

inline static UInt32 I(UInt32 x, UInt32 y, UInt32 z)
{
	return y ^ (x | ~z);
}

// rotate_left rotates x left n bits.
inline static UInt32 RotateLeft(UInt32 x, Int n)
{
	return (x << n) | (x >> (32 - n));
}

// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
// Rotation is separate from addition to prevent recomputation.
inline static void FF(UInt32 &a, UInt32 b, UInt32 c, UInt32 d, UInt32 x, UInt32 s, UInt32 ac)
{
	a = RotateLeft(a+ F(b, c, d) + x + ac, s) + b;
}

inline static void GG(UInt32 &a, UInt32 b, UInt32 c, UInt32 d, UInt32 x, UInt32 s, UInt32 ac)
{
	a = RotateLeft(a + G(b, c, d) + x + ac, s) + b;
}

inline static void HH(UInt32 &a, UInt32 b, UInt32 c, UInt32 d, UInt32 x, UInt32 s, UInt32 ac)
{
	a = RotateLeft(a + H(b, c, d) + x + ac, s) + b;
}

inline static void II(UInt32 &a, UInt32 b, UInt32 c, UInt32 d, UInt32 x, UInt32 s, UInt32 ac)
{
	a = RotateLeft(a + I(b, c, d) + x + ac, s) + b;
}

inline static void Decode(UInt8 const* pInput, UInt32* pOutput, UInt32 nCount)
{
	for (UInt32 i = 0u, j = 0u; j < nCount; i++, j += 4u)
	{
		pOutput[i] =
			(((UInt32)pInput[j+0]) << 0)  |
			(((UInt32)pInput[j+1]) << 8)  |
			(((UInt32)pInput[j+2]) << 16) |
			(((UInt32)pInput[j+3]) << 24);
	}
}

inline static void Transform(UInt8 const* pBlock, UInt32* pState)
{
	UInt32 a = pState[0];
	UInt32 b = pState[1];
	UInt32 c = pState[2];
	UInt32 d = pState[3];

	UInt32 x[16u];
	Decode(pBlock, x, MD5::kBlockSize);

	/* Round 1 */
	FF(a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
	FF(d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
	FF(c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
	FF(b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
	FF(a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
	FF(d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
	FF(c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
	FF(b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
	FF(a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
	FF(d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
	FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
	FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
	FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
	FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
	FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
	FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

	/* Round 2 */
	GG(a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
	GG(d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
	GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
	GG(b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
	GG(a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
	GG(d, a, b, c, x[10], S22,  0x2441453); /* 22 */
	GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
	GG(b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
	GG(a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
	GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
	GG(c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
	GG(b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
	GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
	GG(d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
	GG(c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
	GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

	/* Round 3 */
	HH(a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
	HH(d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
	HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
	HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
	HH(a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
	HH(d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
	HH(c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
	HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
	HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
	HH(d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
	HH(c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
	HH(b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
	HH(a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
	HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
	HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
	HH(b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

	/* Round 4 */
	II(a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
	II(d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
	II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
	II(b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
	II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
	II(d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
	II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
	II(b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
	II(a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
	II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
	II(c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
	II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
	II(a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
	II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
	II(c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
	II(b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

	pState[0] += a;
	pState[1] += b;
	pState[2] += c;
	pState[3] += d;
}

#if SEOUL_LITTLE_ENDIAN
#define ByteReverse(buf, len)	// Nothing
#else
template <typename T>
inline static void ByteReverse(T* pBuffer, UInt32 n)
{
	UInt32* p = (UInt32*)pBuffer;
	for (UInt32 i = 0u; i < n; ++i)
	{
		p[i] = EndianSwap32(p[i]);
	}
}
#endif

MD5::MD5(FixedArray<UInt8, kResultSize>& rOutput)
	: m_rOutput(rOutput)
	, m_aState()
	, m_aBuffer()
	, m_aBits(0u)
{
	m_aState[0] = 0x67452301;
	m_aState[1] = 0xefcdab89;
	m_aState[2] = 0x98badcfe;
	m_aState[3] = 0x10325476;
}

MD5::~MD5()
{
	// Compute number of bytes mod 64
	UInt32 nCount = (m_aBits[0] >> 3u) & 0x3F;

	// Set the first char of padding to 0x80.  This is safe since there is always at least one byte free
	UInt8* p = m_aBuffer.Get(nCount);
	*p++ = 0x80;

	// Bytes of padding needed to make 64 bytes
	nCount = (kBlockSize - 1u - nCount);

	// Pad out to 56 mod 64
	if (nCount < 8)
	{
		// Two lots of padding:  Pad the first block to 64 bytes
		memset(p, 0, nCount);
		ByteReverse(m_aBuffer.Get(0u), 16u);
		Transform(m_aBuffer.Get(0u), m_aState.Get(0u));

		// Now fill the next block with 56 bytes
		memset(m_aBuffer.Get(0u), 0, 56);
	}
	else
	{
		// Pad block to 56 bytes
		memset(p, 0, nCount - 8u);
	}
	ByteReverse(m_aBuffer.Get(0u), 14);

	// Append length in bits and transform
	((UInt32*)m_aBuffer.Get(0u))[14] = m_aBits[0];
	((UInt32*)m_aBuffer.Get(0u))[15] = m_aBits[1];
	Transform(m_aBuffer.Get(0u), m_aState.Get(0u));

	// Return the hex representation of the state.
	FixedArray<UInt8, kResultSize> aDigest;
	ByteReverse((unsigned char *) m_aState.Get(0u), 4);
	memcpy(aDigest.Get(0u), m_aState.Get(0u), 16);

	// Write the result.
	m_rOutput.Swap(aDigest);
}

void MD5::AppendData(UInt8 const* pData, UInt32 zDataSizeInBytes)
{
	/* Update bitcount */
	UInt32 t = m_aBits[0];
	m_aBits[0] = t + ((UInt32)zDataSizeInBytes << 3u);
	if (m_aBits[0] < t)
	{
		/* Carry from low to high */
		m_aBits[1]++;
	}
	m_aBits[1] += (zDataSizeInBytes >> 29u);

	/* Bytes already in shsInfo->data */
	t = (t >> 3u) & 0x3F;

	/* Handle any leading odd-sized chunks */
	if (t)
	{
		UInt8* p = m_aBuffer.Get(t);

		t = (kBlockSize - t);
		if (zDataSizeInBytes < t)
		{
			memcpy(p, pData, zDataSizeInBytes);
			return;
		}

		memcpy(p, pData, t);
		ByteReverse(m_aBuffer.Get(0u), 16);
		Transform(m_aBuffer.Get(0u), m_aState.Get(0u));
		pData += t;
		zDataSizeInBytes -= t;
	}

	/* Process data in 64-byte chunks */
	while (zDataSizeInBytes >= kBlockSize)
	{
		memcpy(m_aBuffer.Get(0u), pData, kBlockSize);
		ByteReverse(m_aBuffer.Get(0u), 16);
		Transform(m_aBuffer.Get(0u), m_aState.Get(0u));
		pData += kBlockSize;
		zDataSizeInBytes -= kBlockSize;
	}

	/* Handle any remaining bytes of data. */
	memcpy(m_aBuffer.Get(0u), pData, zDataSizeInBytes);
}

} // namespace Seoul
