/**
 * \file TextureCookISPCKernel.ispc
 * \brief Implementation of image manipulation and compression using Intel ISPC.
 * \sa https://github.com/GameTechDev/ISPCTextureCompressor
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

// TODO: Improve task scheduling - we want to target a number of
// blocks per task, independent of the width of an input image.

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////
/** Quality levels - beyond this value, quality is clamped. */
static uniform const int kiMaxQualityETC1 = 6;

/**
 * Element counts of the pixel source of a block, depending on whether we need
 * RGB or RGBA data.
 */
static uniform const int kiPixelBlockCountRGB = 48;
static uniform const int kiPixelBlockCountRGBA = 64;

/** Convenience constant - maximum positive value of a 32-bit float. */
static uniform const float kfMaxFloat = 3.402823466e38f;

/**
 * Modifier table values for ETC1 compression.
 *
 * \sa https://www.khronos.org/registry/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
 */
static uniform const float kafModifierTableETC[8][4] =
{
	{ -8, -2, 2, 8 },
	{ -17, -5, 5, 17 },
	{ -29, -9, 9, 29 },
	{ -42, -13, 13, 42 },
	{ -60, -18, 18, 60 },
	{ -80, -24, 24, 80 },
	{ -106, -33, 33, 106 },
	{ -183, -47, 47, 183 },
};

/**
 * ETC1 modifier table index mapping
 *
 * \sa https://www.khronos.org/registry/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
 */
static uniform const int32 kaiModifierRemapETC[] = { 2, 3, 1, 0 };

///////////////////////////////////////////////////////////////////////////////
// Types
///////////////////////////////////////////////////////////////////////////////
struct ETC1_State
{
	float m_afBlock[kiPixelBlockCountRGB];
	int32 m_iPrevCenterQ[3];
	float m_fBestError;
	uint32 m_auBestData[2];
	uniform bool m_bDiffBit;

	// Read-only setting, does not change during compression.
	uniform int32 m_iQuality;
};

struct Image
{
	uint8* m_pData;
	int32 m_iWidth;
	int32 m_iHeight;
	int32 m_iPitchInBytes;
};

///////////////////////////////////////////////////////////////////////////////
// Common functions
///////////////////////////////////////////////////////////////////////////////
/** Wrapper around clamp(). */
inline float ClampFloat(float v, float a, float b)
{
	return clamp(v, a, b);
}

/** Wrapper around clamp() for three integer values. */
inline int32 ClampInt(int32 v, int32 a, int32 b)
{
	return (int32)clamp((float)v, (float)a, (float)b);
}

/**
 * Given an RGB color stored in 565 format in iRGB565, write an
 * 8-bit per channel RGB color into c.
 */
inline void DecodeRGB565(float c[3], int32 iRGB565)
{
	int32 c2 = (iRGB565 >> 0) & 31;
	int32 c1 = (iRGB565 >> 5) & 63;
	int32 c0 = (iRGB565 >> 11) & 31;

	c[0] = (c0 << 3) + (c0 >> 2);
	c[1] = (c1 << 2) + (c1 >> 4);
	c[2] = (c2 << 3) + (c2 >> 2);
}

/** \sa https://github.com/nothings/stb/blob/master/stb_dxt.h stb__Mul8Bit */
inline int32 Multiply8Bit(int32 a, int32 b)
{
	int32 t = a * b + 128;
	return (t + (t >> 8)) >> 8;
}

/**
 * Given an RGB 8-bit color with each component on [0, 255],
 * compact the values into a 16-bit 565 color value.
 */
inline uint16 EncodeRGB565(float c[3])
{
	int32 r = (int32)c[0];
	int32 g = (int32)c[1];
	int32 b = (int32)c[2];

	return
		(Multiply8Bit(r, 31) << 11) +
		(Multiply8Bit(g, 63) << 5) +
		(Multiply8Bit(b, 31));
}

/** 32-bit byte swap, convert little endian to big endian and vice versa. */
inline uint32 EndianSwap32(uint32 v)
{
	uint32 r = 0;
	r += ((v >> 24) & 255) << 0;
	r += ((v >> 16) & 255) << 8;
	r += ((v >> 8) & 255) << 16;
	r += ((v >> 0) & 255) << 24;
	return r;
}

/** Power specialized on power of 2s, performant. */
inline int32 Pow2(int32 x)
{
	return (1 << x);
}

/** Convenience, (v * v). */
inline float Square(float v)
{
	return (v * v);
}

/**
 * Generates a partial sort of aiValues. The first iSubLength elements
 * will contain sorted values, in ascending order, sampled from the entire
 * list.
 */
inline void PartialInsertionSort(
	int32 aiValues[],
	uniform int32 const iLength,
	uniform int32 const iSubLength)
{
	for (uniform int32 k = 0; k < iSubLength; k++)
	{
		// Insertion sort.
		int32 iBestIdx = k;
		int32 iBestValue = aiValues[k];
		for (uniform int32 i = k + 1; i < iLength; i++)
		{
			if (iBestValue > aiValues[i])
			{
				iBestValue = aiValues[i];
				iBestIdx = i;
			}
		}

		// Swap the best index and k.
		aiValues[iBestIdx] = aiValues[k]; // (perf. warning)
		aiValues[k] = iBestValue;
	}
}

/** Fully sorts a list of exactly 8 elements. */
inline void Sort8(int32 aiValues[])
{
	PartialInsertionSort(aiValues, 8, 8);
}

/** Swap function for two int32 values. */
inline void SwapInt(varying int32* uniform piA, varying int32* uniform piB)
{
	int32 iT = *piA;
	*piA = *piB;
	*piB = iT;
}

/** Stream writer, bit by bit. */
inline void WriteBits(
	uint32 auData[],
	uniform int32* uniform piPosition,
	uniform int32 const iBits,
	varying int32 const v)
{
	assert(v < Pow2(iBits));
	auData[*piPosition / 32] |= ((uint32)v) << (*piPosition % 32);
	if (*piPosition % 32 + iBits > 32)
	{
		auData[(*piPosition / 32) + 1] |= (v >> (32 - *piPosition % 32));
	}
	*piPosition += iBits;
}

/**
 * Load a 4x4 block of pixels into afBlock for further processing. Data
 * is loaded such that each channel is contiguous in the block (first is
 * the 4x4 block of red pixels, than green, than blue). In this way,
 * the same structure can be used for RGB and RGBA blocks.
 *
 * This function loads only 3 channels and does not include alpha.
 */
inline void LoadBlockRGB(
	varying float afBlock[kiPixelBlockCountRGB],
	uniform Image const* uniform pImage,
	varying int32 iBlockX,
	varying int32 iBlockY)
{
	for (uniform int32 y = 0; y < 4; y++)
	{
		for (uniform int32 x = 0; x < 4; x++)
		{
			varying int32 iIndex = (iBlockY * 4 + y) * pImage->m_iPitchInBytes;
			uniform uint32* varying pSource = (uint32*)&pImage->m_pData[iIndex];
			uint32 rgba = pSource[iBlockX * 4 + x]; // (perf. warning)

			afBlock[16 * 0 + y * 4 + x] = (int32)((rgba >> 0)  & 255);
			afBlock[16 * 1 + y * 4 + x] = (int32)((rgba >> 8)  & 255);
			afBlock[16 * 2 + y * 4 + x] = (int32)((rgba >> 16) & 255);
		}
	}
}

/**
 * Load a 4x4 block of pixels into afBlock for further processing. Data
 * is loaded such that each channel is contiguous in the block (first is
 * the 4x4 block of red pixels, than green, than blue). In this way,
 * the same structure can be used for RGB and RGBA blocks.
 *
 * This function loads all 4 channels, including alpha.
 */
inline void LoadBlockRGBA(
	float afBlock[kiPixelBlockCountRGBA],
	uniform Image const* uniform pImage,
	varying int32 iBlockX,
	varying int32 iBlockY)
{
	for (uniform int32 y = 0; y < 4; y++)
	{
		for (uniform int32 x = 0; x < 4; x++)
		{
			varying int32 iIndex = (iBlockY * 4 + y)*pImage->m_iPitchInBytes;
			uniform uint32* varying pSource = (uint32*)&pImage->m_pData[iIndex];
			uint32 rgba = pSource[iBlockX * 4 + x]; // (perf. warning)

			afBlock[16 * 0 + y * 4 + x] = (int32)((rgba >> 0)  & 255);
			afBlock[16 * 1 + y * 4 + x] = (int32)((rgba >> 8)  & 255);
			afBlock[16 * 2 + y * 4 + x] = (int32)((rgba >> 16) & 255);
			afBlock[16 * 3 + y * 4 + x] = (int32)((rgba >> 24) & 255);
		}
	}
}

/** Given n 32-bit words, writes the output as a block entry. */
inline void StoreBlock(
	uniform uint8 auOutput[],
	uniform int32 const iWidth,
	varying int32 const iBlockX,
	varying int32 const iBlockY,
	uint32 const auData[],
	int32 const iDataSize)
{
	for (uniform int32 k = 0; k < iDataSize; k++)
	{
		varying int32 iIndex = (iBlockY * iWidth * iDataSize);
		uniform uint32* varying pTarget = (uint32*)&auOutput[iIndex];
		pTarget[iBlockX * iDataSize + k] = auData[k]; // (perf. warning)
	}
}

///////////////////////////////////////////////////////////////////////////////
// DXT1
///////////////////////////////////////////////////////////////////////////////
inline void DXT1_SolveSymmetricVariance(
	float a[3],
	float const afCovar[6],
	float const b[3])
{
	a[0] = (afCovar[0] * b[0]) + (afCovar[1] * b[1]) + (afCovar[2] * b[2]);
	a[1] = (afCovar[1] * b[0]) + (afCovar[3] * b[1]) + (afCovar[4] * b[2]);
	a[2] = (afCovar[2] * b[0]) + (afCovar[4] * b[1]) + (afCovar[5] * b[2]);
}

/**
 * Find the principal axis through the block of colors,
 * as defined by the already computed covariance matrix.
 */
inline void DXT1_ComputeAxis3(
	float afAxis[3],
	float const afCovar[6],
	uniform const int32 iPowerIterations)
{
	float afScratch[3] = { 1, 1, 1 };
	for (uniform int32 i = 0; i < iPowerIterations; i++)
	{
		DXT1_SolveSymmetricVariance(afAxis, afCovar, afScratch);
		for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
		{
			afScratch[iComponent] = afAxis[iComponent];
		}

		if (i % 2 == 1) // renormalize every other iteration
		{
			float fNormSq = 0.0f;
			for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
			{
				fNormSq += Square(afAxis[iComponent]);
			}

			float fRcpNorm = rsqrt(fNormSq);
			for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
			{
				afScratch[iComponent] *= fRcpNorm;
			}
		}
	}

	for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
	{
		afAxis[iComponent] = afScratch[iComponent];
	}
}

/**
 * Compute the covariance matrix for the block of colors.
 * and mean color. Used to measure the deviation in
 * the block and build a PCA to find a best fit axis
 * through the block of colors.
 */
inline void DXT1_ComputeCovarianceAndMean(
	float afCovar[6],
	float afMeanColor[3],
	float const afBlock[kiPixelBlockCountRGB])
{
	// Mean computation.
	for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
	{
		float fSum = 0;
		for (uniform int32 k = 0; k < 16; k++)
		{
			fSum += afBlock[k + iComponent * 16];
		}
		afMeanColor[iComponent] = (fSum / 16.0f);
	}

	// Covariance matrix.
	float fCovar0 = 0.0f;
	float fCovar1 = 0.0f;
	float fCovar2 = 0.0f;
	float fCovar3 = 0.0f;
	float fCovar4 = 0.0f;
	float fCovar5 = 0.0f;

	for (uniform int32 k = 0; k < 16; k++)
	{
		float rgb0 = afBlock[k + 0 * 16] - afMeanColor[0];
		float rgb1 = afBlock[k + 1 * 16] - afMeanColor[1];
		float rgb2 = afBlock[k + 2 * 16] - afMeanColor[2];

		fCovar0 += (rgb0 * rgb0);
		fCovar1 += (rgb0 * rgb1);
		fCovar2 += (rgb0 * rgb2);
		fCovar3 += (rgb1 * rgb1);
		fCovar4 += (rgb1 * rgb2);
		fCovar5 += (rgb2 * rgb2);
	}

	afCovar[0] = fCovar0;
	afCovar[1] = fCovar1;
	afCovar[2] = fCovar2;
	afCovar[3] = fCovar3;
	afCovar[4] = fCovar4;
	afCovar[5] = fCovar5;
}

inline uint32 DXT1_FixQBits(uint32 const uQBits)
{
	static uniform const uint32 kuMask01 = 0x55555555;
	static uniform const uint32 kuMask10 = 0xAAAAAAAA;

	uint32 uQBits0 = (uQBits & kuMask01);
	uint32 uQBits1 = (uQBits & kuMask10);

	return (uQBits1 >> 1) + (uQBits1 ^ (uQBits0 << 1));
}

/**
 * Using the PCA axis and mean, find the two endpoints
 * from the set of colors in the original image's block
 * that form a best fit.
 */
inline void DXT1_PickEndpoints(
	float afC0[3],
	float afC1[3],
	float const afBlock[kiPixelBlockCountRGB],
	float const afAxis[3],
	float const afMeanColor[3])
{
	float fMinDot = (256.0f * 256.0f);
	float fMaxDot = 0.0f;
	for (uniform int32 y = 0; y < 4; y++)
	{
		for (uniform int32 x = 0; x < 4; x++)
		{
			float fDot = 0;
			for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
			{
				uniform int32 iIndex = iComponent * 16 + y * 4 + x;
				fDot += (afBlock[iIndex] - afMeanColor[iComponent])
					* afAxis[iComponent];
			}

			fMinDot = min(fMinDot, fDot);
			fMaxDot = max(fMaxDot, fDot);
		}
	}

	if (fMaxDot - fMinDot < 1.0f)
	{
		fMinDot -= 0.5f;
		fMaxDot += 0.5f;
	}

	float fNormSq = 0;
	for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
	{
		fNormSq += Square(afAxis[iComponent]);
	}

	float fRcpNormSq = rcp(fNormSq);
	for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
	{
		afC0[iComponent] = ClampFloat(
			afMeanColor[iComponent] + fMinDot * fRcpNormSq * afAxis[iComponent],
			0.0f,
			255.0f);
		afC1[iComponent] = ClampFloat(
			afMeanColor[iComponent] + fMaxDot * fRcpNormSq * afAxis[iComponent],
			0.0f,
			255.0f);
	}
}

/**
 * Refinement step. Iterates to solve a system
 * of linear equations to improve the initial
 * guess and minimize error.
 */
inline void DXT1_Refine(
	int32 aiEndpointsQ[2],
	float const afBlock[kiPixelBlockCountRGB],
	uint32 const uBits,
	float const afMeanColor[3])
{
	float afC0[3];
	float afC1[3];

	// Block is best approximated with a single color,
	// so just use the mean value for both endpoints.
	if ((uBits ^ (uBits * 4)) < 4)
	{
		for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
		{
			afC0[iComponent] = afMeanColor[iComponent];
			afC1[iComponent] = afMeanColor[iComponent];
		}
	}
	// Otherwise, refine with a linear solve.
	else
	{
		float Atb1[3] = { 0, 0, 0 };
		float fSumQ = 0;
		float fSumQQ = 0;
		uint32 uShiftedBits = uBits;

		for (uniform int32 k = 0; k < 16; k++)
		{
			float q = (int32)(uShiftedBits & 3);
			uShiftedBits >>= 2;

			float x = 3-q;
			float y = q;

			fSumQ += q;
			fSumQQ += Square(q);

			for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
			{
				Atb1[iComponent] += x * afBlock[k + iComponent * 16];
			}
		}

		float afSum[3];
		float Atb2[3];

		for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
		{
			afSum[iComponent] = (afMeanColor[iComponent] * 16.0f);
		    Atb2[iComponent] = 3.0f * afSum[iComponent] - Atb1[iComponent];
		}

	    float Cxx = (16 * Square(3.0f)) - (2.0f * 3.0f * fSumQ) + fSumQQ;
	    float Cyy = fSumQQ;
		float Cxy = (3.0f * fSumQ) - fSumQQ;
		float fScale = 3.0f * rcp(Cxx * Cyy - Cxy * Cxy);

		for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
		{
			afC0[iComponent] =
				(Atb1[iComponent] * Cyy - Atb2[iComponent] * Cxy) * fScale;
			afC1[iComponent] =
				(Atb2[iComponent] * Cxx - Atb1[iComponent] * Cxy) * fScale;

			afC0[iComponent] = ClampFloat(afC0[iComponent], 0, 255);
			afC1[iComponent] = ClampFloat(afC1[iComponent], 0, 255);
		}
	}

	// Commit outputs.
	aiEndpointsQ[0] = EncodeRGB565(afC0);
	aiEndpointsQ[1] = EncodeRGB565(afC1);
}

/**
 * Given two endpoints colors in 565 format,
 * encode a DXT1 endpoint word (32-bits) that
 * fits the pixel colors of the source block.
 */
inline uint32 DXT1_Quantize(
	float const afBlock[kiPixelBlockCountRGB],
	int32 const iEndpointQ0,
	int32 const iEndpointQ1)
{
	float afC0[3];
	DecodeRGB565(afC0, iEndpointQ0);

	float afC1[3];
	DecodeRGB565(afC1, iEndpointQ1);

	float afDir[3];
	for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
	{
		afDir[iComponent] = (afC1[iComponent] - afC0[iComponent]);
	}

	float fNormSq = 0;
	for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
	{
		fNormSq += Square(afDir[iComponent]);
	}

	float fRcpNormSq = rcp(fNormSq);
	for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
	{
		afDir[iComponent] *= fRcpNormSq * 3.0f;
	}

	float fBias = 0.5;
	for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
	{
		fBias -= (afC0[iComponent] * afDir[iComponent]);
	}

	// Now encode the endpoints, using the compute bias
	// to push the value towards its nearest quantize
	// value in either direction.
	uint32 uBits = 0;
	uint32 uScaler = 1;
	for (uniform int32 k = 0; k < 16; k++)
	{
		float fDot = 0;
		for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
		{
			fDot += afBlock[k + iComponent * 16] * afDir[iComponent];
		}

		int32 q = ClampInt((int32)(fDot + fBias), 0, 3);

		uBits += (q * uScaler);
		uScaler *= 4;
	}

	return uBits;
}

/**
 * Meat of the DXT1 compressor. Roughly based on the
 * algorithm described in: http://developer.download.nvidia.com/whitepapers/2007/Real-Time-YCoCg-DXT-Compression/Real-Time%20YCoCg-DXT%20Compression.pdf
 *
 * In short, computes a covariance matrix and mean to then compute
 * a PCA to find a best fit line through the space formed by
 * the 4x4 block of colors in a DXT1 block. This is the initial
 * guess for the 2 endpoints of the compressed block.
 *
 * Refinement is then done by solving a simple system of linear
 * equations.
 */
inline void DXT1_CompressBlockColor(
	float const afBlock[kiPixelBlockCountRGB],
	uint32 auData[2])
{
	static uniform const float kfZeroEpsilon = 0.001f;
	static uniform const int32 kiPowerIterations = 4;
	static uniform const int32 kiRefineIterations = 1;

	float afCovar[6];
	float afMeanColor[3];
	DXT1_ComputeCovarianceAndMean(afCovar, afMeanColor, afBlock);
	afCovar[0] += kfZeroEpsilon;
	afCovar[3] += kfZeroEpsilon;
	afCovar[5] += kfZeroEpsilon;

	float afAxis[3];
	DXT1_ComputeAxis3(afAxis, afCovar, kiPowerIterations);

	float afC0[3];
	float afC1[3];
	DXT1_PickEndpoints(afC0, afC1, afBlock, afAxis, afMeanColor);

	int32 aiEndpointQ[2];
	aiEndpointQ[0] = EncodeRGB565(afC0);
	aiEndpointQ[1] = EncodeRGB565(afC1);
	if (aiEndpointQ[0] < aiEndpointQ[1])
	{
		SwapInt(&aiEndpointQ[0], &aiEndpointQ[1]);
	}

	// Initial guess.
	auData[0] = (1 << 16) * aiEndpointQ[1] + aiEndpointQ[0];
	auData[1] = DXT1_Quantize(afBlock, aiEndpointQ[0], aiEndpointQ[1]);

	// Refinement.
	for (uniform int32 i = 0; i < kiRefineIterations; i++)
	{
		DXT1_Refine(aiEndpointQ, afBlock, auData[1], afMeanColor);
		if (aiEndpointQ[0] < aiEndpointQ[1])
		{
			SwapInt(&aiEndpointQ[0], &aiEndpointQ[1]);
		}
		auData[0] = (1 << 16) * aiEndpointQ[1] + aiEndpointQ[0];
		auData[1] = DXT1_Quantize(afBlock, aiEndpointQ[0], aiEndpointQ[1]);
	}

	// Final.
	auData[1] = DXT1_FixQBits(auData[1]);
}

/** Compress a single 4x4 pixel block into a single compressed block. */
inline void DXT1_CompressBlock(
	uniform Image const* uniform pImage,
	int32 const iBlockX,
	int32 const iBlockY,
	uniform uint8 auOutput[])
{
	float afBlock[kiPixelBlockCountRGB];
	uint32 auData[2];
	LoadBlockRGB(afBlock, pImage, iBlockX, iBlockY);
	DXT1_CompressBlockColor(afBlock, auData);
	StoreBlock(auOutput, pImage->m_iWidth, iBlockX, iBlockY, auData, 2);
}

/** Parallelizable task - processes one row of compressed blocks. */
task void DXT1_CompressRow(
	uniform Image const* uniform pImage,
	uniform uint8 auOutput[])
{
	uniform int32 iBlockY = taskIndex0;
	foreach (iBlockX = 0 ... pImage->m_iWidth / 4)
	{
		DXT1_CompressBlock(pImage, iBlockX, iBlockY, auOutput);
	}
}

///////////////////////////////////////////////////////////////////////////////
// DXT5
///////////////////////////////////////////////////////////////////////////////
/**
 * Additional step required for DXT5 blocks, generate the portion of the
 * compressed block that encodes variable alpha.
 *
 * \note afBlock is expected to already have been adjusted to point
 * at the start of the alpha values.
 */
inline void DXT5_CompressBlockAlpha(float const afBlock[16], uint32 auData[2])
{
	float afEndpoints[2] = { 255, 0 };

	// Compute the extends of the alpha space.
	for (uniform int32 k = 0; k < 16; k++)
	{
		afEndpoints[0] = min(afEndpoints[0], afBlock[k]);
		afEndpoints[1] = max(afEndpoints[1], afBlock[k]);
	}

	// Tweak the alpha values if they're
	// equal so we still have a space.
	if (afEndpoints[0] == afEndpoints[1])
	{
		afEndpoints[1] = afEndpoints[0] + 0.1f;
	}

	// Now find the best values based on how they
	// will be quantized.
	uint32 auBlockQ[2] = { 0, 0 };
	float fScale = 7.0f / (afEndpoints[1] - afEndpoints[0]);
	for (uniform int32 k = 0; k < 16; k++)
	{
		float v = afBlock[k];
		float proj = (v - afEndpoints[0]) * fScale + 0.5f;

		int32 q = ClampInt((int32)proj, 0, 7);

		q = (7 - q);

		if (q > 0)
		{
			q++;
		}

		if (q == 8)
		{
			q = 1;
		}

		auBlockQ[k / 8] |= (q << ((k % 8) * 3));
	}

	// TODO: Refine initial guess.

	// Commit endpoints to the alpha block data.
	int32 iEndpoint0 = (int32)afEndpoints[0];
	int32 iEndpoint1 = (int32)afEndpoints[1];

	auData[0] = ClampInt(iEndpoint0, 0, 255) * 256 + ClampInt(iEndpoint1, 0, 255);
	auData[0] |= (auBlockQ[0] << 16);
	auData[1] = (auBlockQ[0] >> 16);
	auData[1] |= (auBlockQ[1] << 8);
}

/** Compress a single 4x4 pixel block into a single compressed block. */
inline void DXT5_CompressBlock(
	uniform Image const* uniform pImage,
	int32 const iBlockX,
	int32 const iBlockY,
	uniform uint8 auOutput[])
{
	float afBlock[kiPixelBlockCountRGBA];
	uint32 auData[4];

	LoadBlockRGBA(afBlock, pImage, iBlockX, iBlockY);
	DXT5_CompressBlockAlpha(&afBlock[kiPixelBlockCountRGB], &auData[0]);
	DXT1_CompressBlockColor(afBlock, &auData[2]);
	StoreBlock(auOutput, pImage->m_iWidth, iBlockX, iBlockY, auData, 4);
}

/** Parallelizable task - processes one row of compressed blocks. */
task void DXT5_CompressRow(
	uniform Image const* uniform pImage,
	uniform uint8 auOutput[])
{
	uniform int32 iBlockY = taskIndex0;
	foreach (iBlockX = 0 ... pImage->m_iWidth / 4)
	{
		DXT5_CompressBlock(pImage, iBlockX, iBlockY, auOutput);
	}
}

///////////////////////////////////////////////////////////////////////////////
// ETC1
///////////////////////////////////////////////////////////////////////////////
/**
 * Given a 4-bit color value on [0, 15], expand to an 8-bit value on [0, 255].
 */
inline int32 ETC1_Extend4To8Bits(int32 iValue)
{
	return (iValue << 4) | iValue;
}

/**
 * Given a 4-bit color value on [0, 31], expand to an 8-bit value on [0, 255].
 */
inline int32 ETC1_Extend5To8Bits(int32 iValue)
{
	return (iValue << 3) | (iValue >> 2);
}

/** Return a selector factor at q for the given selector table index. */
inline uniform float ETC1_GetModifier(uniform int32 iSelectors, uniform int32 q)
{
	return kafModifierTableETC[iSelectors][q];
}

/** Round q-bit to range - see ETC1_CompressBlockHalfInner. */
inline int32 ETC1_RemapQBit(int32 x)
{
	x -= 2;
	if (x < 0)
	{
		x = (1 - x);
	}
	return x;
}

/** Given an 8-bit color value on [0, 255], quantize as a 4 bit value. */
inline int32 ETC1_QuantizeTo4Bits(float fValue)
{
	return (int32)ClampFloat((fValue / 255.0f) * 15.0f + 0.5, 0.0f, 15.0f);
}

/** Given an 8-bit color value on [0, 255], quantize as a 5 bit value. */
inline int32 ETC1_QuantizeTo5Bits(float fValue)
{
	return (int32)ClampFloat((fValue / 255.0f) * 31.0f + 0.5, 0.0f, 31.0f);
}

/**
 * Given new center value (in full precision, 888), generate quantized
 * color values (based on diff bit, either a 444 color, or a
 * 555 color/333 delta), and then update the full precision center to reflect
 * the quantized value.
 */
inline void ETC1_UpdateCenters(
	int32 aiOutCenterQ[3],
	float afInOutCenter[3],
	uniform bool const bDiffBit,
	int32 const aiPrevCenterQ[3])
{
	// When the diff bit is true, endpoints are made up of
	// a base 555 color value and a delta 333 color delta
	// value (24 bits total).
	if (bDiffBit)
	{
		for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
		{
			// Quantize.
			aiOutCenterQ[iComponent] =
				ETC1_QuantizeTo5Bits(afInOutCenter[iComponent]);

			// If we're the second half block of the 2 half block
			// pair in this encoding style, then our value is the
			// 333 color delta, so we compute a delta against
			// the previous value.
			if (aiPrevCenterQ[0] >= 0)
			{
				if (aiOutCenterQ[iComponent] - aiPrevCenterQ[iComponent] > 3)
				{
					aiOutCenterQ[iComponent] = aiPrevCenterQ[iComponent] + 3;
				}

				if (aiOutCenterQ[iComponent] - aiPrevCenterQ[iComponent] < -4)
				{
					aiOutCenterQ[iComponent] = aiPrevCenterQ[iComponent] - 4;
				}
			}

			// Expand so the full precision reflects the
			// quantized value.
			afInOutCenter[iComponent] =
				ETC1_Extend5To8Bits(aiOutCenterQ[iComponent]);
		}
	}
	// When the diff bit is false, endpoints are made up of
	// 2 444 color endpoints (24 bits total).
	else
	{
		for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
		{
			// Encode the quantized 444 endpoint and then
			// re-expand to full precision 8-bits to
			// reflect the quanitzed value.
			aiOutCenterQ[iComponent] =
				ETC1_QuantizeTo4Bits(afInOutCenter[iComponent]);
			afInOutCenter[iComponent] =
				ETC1_Extend4To8Bits(aiOutCenterQ[iComponent]);
		}
	}
}

/**
 * Given a set of colors present in the block,
 * find the best endpoint value for the given selector
 * set.
 */
inline float ETC1_GetOptimalCenter(
	float const aafColors[4][10],
	uniform int32 const iComponent,
	uniform int32 const iSelectors)
{
	float fBestCenter = 0.0f;
	for (uniform int32 q = 0; q < 4; q++)
	{
		fBestCenter += (aafColors[q][7 + iComponent] - ETC1_GetModifier(iSelectors, q)) * aafColors[q][3];
	}
	fBestCenter /= 8.0f;

	float fBestError = 0.0f;
	for (uniform int32 q = 0; q < 4; q++)
	{
		float fModifier = ETC1_GetModifier(iSelectors, q);
		fBestError += Square(ClampFloat(fBestCenter + fModifier, 0.0f, 255.0f) - aafColors[q][7 + iComponent]) * aafColors[q][3];
	}

	for (uniform int32 iBranch = 0; iBranch < 4; iBranch++)
	{
		float fNewCenter = 0.0f;
		float fSum = 0.0f;
		for (uniform int32 q = 0; q < 4; q++)
		{
			if (iBranch <= 1 && q <= iBranch)
			{
				continue;
			}

			if (iBranch >= 2 && q >= iBranch)
			{
				continue;
			}

			fNewCenter += (aafColors[q][7 + iComponent] - ETC1_GetModifier(iSelectors, q)) * aafColors[q][3];
			fSum += aafColors[q][3];
		}

		fNewCenter /= fSum;

		float fError = 0;
		for (uniform int32 q = 0; q < 4; q++)
		{
			float fModifier = ETC1_GetModifier(iSelectors, q);
			fError += Square(ClampFloat(fNewCenter + fModifier, 0.0f, 255.0f) - aafColors[q][7 + iComponent]) * aafColors[q][3];
		}

		if (fError < fBestError)
		{
			fBestError = fError;
			fBestCenter = fNewCenter;
		}
	}

	return fBestCenter;
}

/**
 * Meat of the compression loop - finds color endpoints
 * and selector bits that minimize error for half
 * an ETC1 block (4x2).
 *
 * pState is expected to store current settings (diff bit and flip bit),
 * as well as the found solution for the previous half block (if this
 * is the second half block), used to tweak color selection for both
 * blocks.
 *
 * \sa https://github.com/GameTechDev/ISPCTextureCompressor/blob/master/ISPC%20Texture%20Compressor/ISPC%20Texture%20Compressor.pdf
 */
inline float ETC1_CompressBlockHalfInner(
	varying uint32* uniform puOutQBits,
	varying int32* uniform piOutSelectors,
	int32 aiOutCenterQ[3],
	float const afHalfPixels[],
	varying ETC1_State* uniform pState)
{
	int32 aiPackedValues[165];
	int32 aiSortedValueInv[8];
	float afSortedValue[8];

	// Step one is to encode all 8 pixels in the input half block
	// into a sortable value. This is done by summing all three
	// components of a color (RGB), then shifting the value to
	// add a disamgiguator to the lowest 4 bits.
	{
		// Generate and sort the main values.
		int32 aiSortedIdx[8];
		for (uniform int32 k = 0; k < 8; k++)
		{
			float fValue = 0;
			for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
			{
				fValue += afHalfPixels[k + iComponent * 16];
			}

			aiSortedIdx[k] = (((int32)fValue) << 4) + k;
		}
		Sort8(aiSortedIdx);

		// Inverted values use the disambiguator only.
		for (uniform int32 k = 0; k < 8; k++)
		{
			aiSortedValueInv[k] = ((aiSortedIdx[k] & 0xF) << 4) + k;
		}
		Sort8(aiSortedValueInv);

		// Now produce the rescaled sorted value.
		for (uniform int32 k = 0; k < 8; k++)
		{
			afSortedValue[k] = (aiSortedIdx[k] >> 4) / 3.0f;
		}
	}

	// Step two is to build a large space of sortable samples.
	// These comprise the tetrahedron of the symmetric
	// portion of the 3D space of possible color values samples
	// taken from the input block. The encoded value is the error
	// of the sample (in the MSB) and the indices of the sample
	// (in the LSB) for reconstruction afterwards.
	uniform int32 iPackedValueIndex = -1;
	for (uniform int32 iLevel1 = 0; iLevel1 <= 8; iLevel1++)
	{
		for (uniform int32 iLevel2 = iLevel1; iLevel2 <= 8; iLevel2++)
		{
			for (uniform int32 iLevel3 = iLevel2; iLevel3 <= 8; iLevel3++)
			{
				iPackedValueIndex++;
				assert(iPackedValueIndex < 165);

				// Accumulators for measuring error.
				float afSum[4];
				float afSumSq[4];
				float afCount[4];
				float afInvCount[4];
				for (uniform int32 q = 0; q < 4; q++)
				{
					afSum[q] = 0.0f;
					afSumSq[q] = 0.0f;
					afCount[q] = 0.0f;
					afInvCount[q] = 0.0f;
				}

				// Accumulate error of the 8 possibilities
				// at the current variation.
				for (uniform int32 k = 0; k < 8; k++)
				{
					uniform int32 q;
					if (k >= iLevel3) { q = 3; }
					else if (k >= iLevel2) { q = 2; }
					else if (k >= iLevel1) { q = 1; }
					else { q = 0; }

					afSum[q] += afSortedValue[k];
					afSumSq[q] += Square(afSortedValue[k]);
					afCount[q] += 1.0f;
				}

				// Compute inverse count.
				for (uniform int32 q = 0; q < 4; q++)
				{
					if (afCount[q] > 0.0f)
					{
						afInvCount[q] = 1.0f / afCount[q];
					}
				}

				// Error values from mean.
				float fBaseError = 0.0f;
				for (uniform int32 q = 0; q < 4; q++)
				{
					fBaseError += afSumSq[q] - Square(afSum[q]) * afInvCount[q];
				}

				// Now compute the error at all the possible selector levels.
				// The best error of these results is the best possible error
				// of this sample.
				float fTempError = kfMaxFloat;
				for (uniform int32 iSelectors = 0; iSelectors < 8; iSelectors++)
				{
					float fCenter = 0.0f;
					for (uniform int32 q = 0; q < 4; q++)
					{
						fCenter += afSum[q] - ETC1_GetModifier(iSelectors, q) * afCount[q];
					}
					fCenter /= 8.0f;

					float fError = fBaseError;
					for (uniform int32 q = 0; q < 4; q++)
					{
						fError += Square(fCenter + ETC1_GetModifier(iSelectors, q) - afSum[q] * afInvCount[q]) * afCount[q];
					}

					fTempError = min(fTempError, fError);
				}

				int32 iPacked = (iLevel1 * 16 + iLevel2) * 16 + iLevel3;

				aiPackedValues[iPackedValueIndex] = (((int32)fTempError) << 12) + iPacked;
			}
		}
	}

	// This selects the m_iQuality number of solutions
	// with the lowest error values. We then evaluate
	// each to find the best solution.
	PartialInsertionSort(aiPackedValues, 165, pState->m_iQuality);

	// Now iterate over the twop pState->m_iQuality samples,
	// test them, refine them, and pick the best as our
	// solution.
	float fBestError = (Square(255.0f) * 3.0f * 8.0f);
	int32 iBestSelectors = -1;
	int32 aiBestCenterQ[3];
	uint32 uBestQBits;
	for (uniform int32 i = 0; i < pState->m_iQuality; i++)
	{
		// Unpack the indices of the sample we're about to test.
		int32 iPacked = aiPackedValues[i] & 0xFFF;
		int32 iLevel1 = (iPacked >> 8) & 0xF;
		int32 iLevel2 = (iPacked >> 4) & 0xF;
		int32 iLevel3 = (iPacked >> 0) & 0xF;

		// Initialize our color table for solving.
		float aafColors[4][10];
		for (uniform int32 p = 0; p < 10; p++)
		{
			for (uniform int32 q = 0; q < 4; q++)
			{
				aafColors[q][p] = 0;
			}
		}

		// Fill out quantized values and color
		// space at the current sample.
		uint32 uQBits = 0;
		for (uniform int32 kk = 0; kk < 8; kk++)
		{
			// Extract the lookup index from the lowest 4 bits.
			int32 k = aiSortedValueInv[kk] & 0xF;

			// Extract the quality bucket.
			int32 qq;
			if (k >= iLevel3) { qq = 3; }
			else if (k >= iLevel2) { qq = 2; }
			else if (k >= iLevel1) { qq = 1; }
			else { qq = 0; }

			// Get the x/y coordinate in the block.
			uniform int32 iBlockX = (kk & 3);
			uniform int32 iBlockY = (kk >> 2);

			int32 qqq = ETC1_RemapQBit(qq);
			uQBits |= (qqq & 1) << (iBlockY + iBlockX * 4);
			uQBits |= (qqq >> 1) << (16 + iBlockY + iBlockX * 4);

			float afVecQ[4];
			for (uniform int32 q = 0; q < 4; q++)
			{
				afVecQ[q] = (q == qq ? 1.0f : 0.0f);
				aafColors[q][3] += afVecQ[q];
			}

			for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
			{
				float fValue = afHalfPixels[16 * iComponent + kk];
				for (uniform int32 q = 0; q < 4; q++)
				{
					aafColors[q][iComponent] += fValue * afVecQ[q];
					aafColors[q][4 + iComponent] += Square(fValue) * afVecQ[q];
				}
			}
		}

		// Now measure the base error from mean.
		float fBaseError = 0;
		for (uniform int32 q = 0; q < 4; q++)
		{
			if (aafColors[q][3] > 0)
			{
				for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
				{
					aafColors[q][7 + iComponent] = aafColors[q][iComponent] / aafColors[q][3];
					fBaseError += aafColors[q][4 + iComponent] - Square(aafColors[q][7 + iComponent]) * aafColors[q][3];
				}
			}
		}

		// Now try all 8 selector possibilities, find the best color endpoint
		// at that selector, and use it if the error is less than our
		// running value.
		for (uniform int32 iSelectors = 0; iSelectors < 8; iSelectors++)
		{
			float afCenter[3];
			int32 aiCenterQ[3];

			// Get the optimal center at the selector.
			for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
			{
				afCenter[iComponent] =
					ETC1_GetOptimalCenter(aafColors, iComponent, iSelectors);
			}

			// Refine/compute center values - this will
			// quantize center then update afCenter from
			// the quantized value, so we can measure
			// error against the real value.
			ETC1_UpdateCenters(
				aiCenterQ,
				afCenter,
				pState->m_bDiffBit,
				pState->m_iPrevCenterQ);

			// Now measure the error against the selected endpoint.
			float fError = fBaseError;
			for (uniform int32 q = 0; q < 4; q++)
			{
				float fModifier = ETC1_GetModifier(iSelectors, q);
				for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
				{
					fError += Square(ClampFloat(afCenter[iComponent] + fModifier, 0.0f, 255.0f) - aafColors[q][7 + iComponent]) * aafColors[q][3];
				}
			}

			// If the error was better, this is our new best guess.
			if (fError < fBestError)
			{
				fBestError = fError;
				iBestSelectors = iSelectors;
				uBestQBits = uQBits;
				for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
				{
					aiBestCenterQ[iComponent] = aiCenterQ[iComponent];
				}
			}
		}
	}

	// Assign outputs.
	*piOutSelectors = iBestSelectors;
	*puOutQBits = uBestQBits;
	for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
	{
		aiOutCenterQ[iComponent] = aiBestCenterQ[iComponent];
	}

	return fBestError;
}

/**
 * Wrapper around ETC1_CompressBlockHalfInner, which does the meat
 * of ETC1 compression, compressing a single 4x2 half block of a
 * compressed ETC1 block.
 */
inline float ETC1_CompressBlockHalf(
	varying uint32* uniform puOutQBits,
	varying int32* uniform piOutSelectors,
	int32 aiOutCenterQ[3],
	float const afHalfPixels[],
	varying ETC1_State* uniform pState)
{
	float fError = ETC1_CompressBlockHalfInner(
		puOutQBits,
		piOutSelectors,
		aiOutCenterQ,
		afHalfPixels,
		pState);

	// Capture the current quantized color centers
	// for calculations of the other half block.
	for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
	{
		pState->m_iPrevCenterQ[iComponent] = aiOutCenterQ[iComponent];
	}

	return fError;
}

/**
 * Given all parts (selector bits, endpoint data, and config bits),
 * generate 2 32-bit ETC1 words for a block, written to auData.
 */
inline void ETC1_WriteBlock(
	uint32 auOutData[2],
	uint32 const auQBits[2],
	int32 const aiSelectors[2],
	int32 const aaiCentersQ[2][3],
	uniform int32 const iDiffBit,
	uniform int32 const iFlipBit)
{
	// Initial default output data - an ETC1 block is
	// two 32-bit words.
	auOutData[0] = 0;
	auOutData[1] = 0;

	// Encode the two color end points - encoding
	// format is dependent on the diff bit.
	uniform int32 iPosition = 0;
	if (iDiffBit == 0)
	{
		WriteBits(auOutData, &iPosition, 4, aaiCentersQ[1][0]);
		WriteBits(auOutData, &iPosition, 4, aaiCentersQ[0][0]);

		WriteBits(auOutData, &iPosition, 4, aaiCentersQ[1][1]);
		WriteBits(auOutData, &iPosition, 4, aaiCentersQ[0][1]);

		WriteBits(auOutData, &iPosition, 4, aaiCentersQ[1][2]);
		WriteBits(auOutData, &iPosition, 4, aaiCentersQ[0][2]);
	}
	else
	{
		WriteBits(auOutData, &iPosition, 3, (aaiCentersQ[1][0] - aaiCentersQ[0][0]) & 7);
		WriteBits(auOutData, &iPosition, 5, aaiCentersQ[0][0]);

		WriteBits(auOutData, &iPosition, 3, (aaiCentersQ[1][1] - aaiCentersQ[0][1]) & 7);
		WriteBits(auOutData, &iPosition, 5, aaiCentersQ[0][1]);

		WriteBits(auOutData, &iPosition, 3, (aaiCentersQ[1][2] - aaiCentersQ[0][2]) & 7);
		WriteBits(auOutData, &iPosition, 5, aaiCentersQ[0][2]);
	}

	// Now encode the flip and diff bits, and finally
	// the selector words.
	WriteBits(auOutData, &iPosition, 1, iFlipBit);
	WriteBits(auOutData, &iPosition, 1, iDiffBit);
	WriteBits(auOutData, &iPosition, 3, aiSelectors[1]);
	WriteBits(auOutData, &iPosition, 3, aiSelectors[0]);

	// Now fixup the selector bits based on the flipped bit.
	uint32 uAllQBitsFlipped = (auQBits[1] << 2) | auQBits[0];
	uint32 uAllQBits = 0;
	if (iFlipBit != 0)
	{
		uAllQBits = uAllQBitsFlipped;
	}
	else
	{
		for (uniform int32 k = 0; k < 2; k++)
		{
			for (uniform int32 y = 0; y < 4; y++)
			{
				for (uniform int32 x = 0; x < 4; x++)
				{
					int32 iBit = (uAllQBitsFlipped >> (k * 16 + x * 4 + y)) & 1;
					uAllQBits += iBit << (k * 16 + y * 4 + x);
				}
			}
		}
	}

	// Finally, correct - ETC1 is big endian.
	auOutData[1] = EndianSwap32(uAllQBits);
}

/**
 * Transposes the 4x4 block of pixels defined by afBlock to
 * capture the meaning of the flip bit in an ETC1 block.
 */
inline void ETC1_ComputeFlippedBlock(
	float afFlipped[kiPixelBlockCountRGB],
	float const afBlock[kiPixelBlockCountRGB])
{
	for (uniform int32 y = 0; y < 4; y++)
	{
		for (uniform int32 x = 0; x < 4; x++)
		{
			for (uniform int32 iComponent = 0; iComponent < 3; iComponent++)
			{
				uniform int32 iTo = 16 * iComponent + x * 4 + y;
				uniform int32 iFrom = 16 * iComponent + y * 4 + x;

				afFlipped[iTo] = afBlock[iFrom];
			}
		}
	}
}

/**
 * Root block compression loop - tries solutions for all
 * combinations (flipped or not, diff delta bit or not), and
 * chooses the combination with the lowest error.
 *
 * \sa https://www.khronos.org/registry/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
 *     Section 3.17.1 for meaning of the diff bit.
 *     Section 3.9.2 for flipbit=0
 *     Section 3.9.3 for flipbit=1
 */
inline void ETC1_CompressBlockColor(varying ETC1_State* uniform pState)
{
	float afFlippedBlock[kiPixelBlockCountRGB];
	ETC1_ComputeFlippedBlock(afFlippedBlock, pState->m_afBlock);

	uint32 auQBits[2];
	int32 aiSelectors[2];
	int32 aaiCentersQ[2][3];
	for (uniform int32 iFlipBit = 0; iFlipBit < 2; iFlipBit++)
	{
		varying float* uniform pixels = (0 == iFlipBit
			? afFlippedBlock
			: pState->m_afBlock);

		for (uniform int32 iDiffBit = 1; iDiffBit >= 0; iDiffBit--)
		{
			pState->m_bDiffBit = (iDiffBit == 1);
			pState->m_iPrevCenterQ[0] = -1;

			// ETC1 blocks are split into 2 (2x4) sub blocks,
			// so compute the best fit for each half, and total
			// up the error.
			float fError = 0.0f;
			fError += ETC1_CompressBlockHalf(
				&auQBits[0],
				&aiSelectors[0],
				aaiCentersQ[0],
				&pixels[0],
				pState);
			fError += ETC1_CompressBlockHalf(
				&auQBits[1],
				&aiSelectors[1],
				aaiCentersQ[1],
				&pixels[8],
				pState);

			// If the total error of the two halves is less
			// than the best fit so far, use this new packing.
			if (fError < pState->m_fBestError)
			{
				pState->m_fBestError = fError;
				ETC1_WriteBlock(
					pState->m_auBestData,
					auQBits,
					aiSelectors,
					aaiCentersQ,
					iDiffBit,
					iFlipBit);
			}
		}
	}
}

/** Compress a single 4x4 pixel block into a single compressed block. */
inline void ETC1_CompressBlock(
	uniform uint8 auOutput[],
	uniform Image const* uniform pImage,
	int32 const iBlockX,
	int32 const iBlockY,
	uniform int32 const iQuality)
{
	ETC1_State state;
	state.m_iQuality = iQuality;
	state.m_fBestError = kfMaxFloat;

	LoadBlockRGB(state.m_afBlock, pImage, iBlockX, iBlockY);
	ETC1_CompressBlockColor(&state);
	StoreBlock(auOutput, pImage->m_iWidth, iBlockX, iBlockY, state.m_auBestData, 2);
}

/** Parallelizable task - processes one row of compressed blocks. */
task void ETC1_CompressRow(
	uniform Image const* uniform pImage,
	uniform uint8 auOutput[],
	uniform int32 const iQuality)
{
	uniform int32 iBlockY = taskIndex0;
	foreach (iBlockX = 0 ... pImage->m_iWidth / 4)
	{
		ETC1_CompressBlock(auOutput, pImage, iBlockX, iBlockY, iQuality);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Exports
///////////////////////////////////////////////////////////////////////////////
export void DXT1_Compress(
	uniform Image const* uniform pImage,
	uniform uint8 auOutput[])
{
	launch [pImage->m_iHeight / 4]
	DXT1_CompressRow(pImage, auOutput);
}

export void DXT5_Compress(
	uniform Image const* uniform pImage,
	uniform uint8 auOutput[])
{
	launch [pImage->m_iHeight / 4]
	DXT5_CompressRow(pImage, auOutput);
}

export void ETC1_Compress(
	uniform Image const* uniform pImage,
	uniform uint8 auOutput[],
	uniform int32 iQuality)
{
	iQuality = min(iQuality, kiMaxQualityETC1);

	launch [pImage->m_iHeight / 4]
	ETC1_CompressRow(pImage, auOutput, iQuality);
}
