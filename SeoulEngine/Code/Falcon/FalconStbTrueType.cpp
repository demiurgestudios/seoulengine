/**
 * \file FalconTrueType.cpp
 * \brief Implementation of stb_truetype into the Falcon project,
 * as well as some custom Falcon extensions (SDF glyph generation).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#define STB_TRUETYPE_IMPLEMENTATION
#include "FalconConstants.h"
#include "FalconStbTrueType.h"
#include "MemoryManager.h"
#include "StackOrHeapArray.h"
#include "StringUtil.h"
#include "Vector.h"
#include "Vector2D.h"

namespace Seoul::Falcon
{

namespace
{

typedef StackOrHeapArray<Float, 1024, MemoryBudgets::Falcon> SquareDistances;
typedef Vector< Float > LineList;
typedef Vector< LineList, MemoryBudgets::Falcon > LineLists;

class ContoursPWL SEOUL_SEALED
{
public:
	ContoursPWL(void* pUserData)
		: m_iContours(0)
		, m_piPointCounts(nullptr)
		, m_pPoints(nullptr)
		, m_pUserData(pUserData)
	{
	}

	~ContoursPWL()
	{
		// Release points.
		STBTT_free(m_piPointCounts, m_pUserData);
		m_piPointCounts = nullptr;
		STBTT_free(m_pPoints, m_pUserData);
		m_pPoints = nullptr;
	}

	Bool HasData() const
	{
		return (nullptr != m_pPoints);
	}

	void Swap(ContoursPWL& r)
	{
		Seoul::Swap(m_iContours, r.m_iContours);
		Seoul::Swap(m_piPointCounts, r.m_piPointCounts);
		Seoul::Swap(m_pPoints, r.m_pPoints);
		Seoul::Swap(m_pUserData, r.m_pUserData);
	}

	Int32 m_iContours;
	Int32* m_piPointCounts;
	stbtt__point* m_pPoints;

private:
	void* m_pUserData;

	SEOUL_DISABLE_COPY(ContoursPWL);
}; // struct ContoursPWL

} // anonymous namespace

/**
 * Build a list per row of the line segments that overlap that row,
 * outputing a sorted list of dot coordinates (see comment below).
 *
 * Used to determine inside vs. outside per line.
 */
static void GetLineLists(
	Int32 iWidth,
	Int32 iHeight,
	const ContoursPWL& pwl,
	LineLists& rv)
{
	rv.Resize(iHeight);

	Int iPointBegin = 0;
	for (Int32 iContour = 0; iContour < pwl.m_iContours; ++iContour)
	{
		Int const iPointEnd = (iPointBegin + pwl.m_piPointCounts[iContour]);
		Int32 iPrevPoint = (iPointEnd - 1);
		for (Int32 iPoint = iPointBegin; iPoint < iPointEnd; ++iPoint)
		{
			auto const& e0(pwl.m_pPoints[iPrevPoint]);
			auto const& e1(pwl.m_pPoints[iPoint]);

			// Compute the vertical range that this line affects.
			Int32 i0((Int32)(e0.y));
			Int32 i1((Int32)(e1.y));
			if (i1 < i0) { Swap(i0, i1); }
			i0 -= 1;
			i1 += 1;
			i0 = Max(0, i0);
			i1 = Min(iHeight - 1, i1);

			// Skip the line if it's ended up entirely horizontal.
			if (i0 == i1)
			{
				continue;
			}

			// Now add the line terms to the various line lists.
			for (Int y = i0; y <= i1; ++y)
			{
				Float const fY((Float)y);

				// If the line's Y is completely above or below
				// the Y of this line, it doesn't influence the line,
				// so skipt it.
				if ((e0.y > fY) == (e1.y > fY))
				{
					continue;
				}

				// Otherwise - compute the term. This is projection along
				// X. It is the dot coordinate of the line at row Y along
				// X. It can be compared to a position's X value later
				// for determining when the line is crossed.
				Float const fDotCoordinateX =
					(((e1.x - e0.x) * (fY - e0.y)) / (e1.y - e0.y)) + e0.x;

				// Done, add the term.
				rv[y].PushBack(fDotCoordinateX);
			}

			iPrevPoint = iPoint;
		}
		iPointBegin = iPointEnd;
	}

	// Sort the line lists.
	for (auto i = rv.Begin(); rv.End() != i; ++i)
	{
		QuickSort(i->Begin(), i->End());
	}
}

/**
 * @return A point set that forms a piecewise-linear approximation
 * to the glyph's contours defined by pInfo->iGlyph.
 *
 * The returned PWL will be rescaled based on fScaleX and
 * fScaleY.
 */
static void GetContoursPWL(
	stbtt_fontinfo const* pInfo,
	Float fOriginX,
	Float fOriginY,
	Float fScaleX,
	Float fScaleY,
	Int32 iGlyph,
	ContoursPWL& r)
{
	// Hot constant - changing this value can significantly alter
	// the runtime cost of computing glyphs as well as the visual quality.
	//
	// Alter with care.
	static const Float kfFlatnessInPixels = 0.15f;

	ContoursPWL pwl(pInfo->userdata);

	// Flatten glyph curves into a list of line segment vertices and winding
	// order.
	{
		stbtt_vertex* pVertices = nullptr;
		Int32 iVertices = stbtt_GetGlyphShape(pInfo, iGlyph, &pVertices);

		Float32 const fScale = (fScaleX > fScaleY ? fScaleY : fScaleX);

		pwl.m_pPoints = stbtt_FlattenCurves(
			pVertices,
			iVertices,
			(kfFlatnessInPixels / fScale),
			&pwl.m_piPointCounts,
			&pwl.m_iContours,
			pInfo->userdata);

		stbtt_FreeShape(pInfo, pVertices);
	}

	// If we have contours, scale and offset them into the proper space.
	if (nullptr != pwl.m_pPoints)
	{
		Int iPointBegin = 0;
		for (Int32 iContour = 0; iContour < pwl.m_iContours; ++iContour)
		{
			Int const iPointEnd = (iPointBegin + pwl.m_piPointCounts[iContour]);
			for (Int32 iPoint = iPointBegin; iPoint < iPointEnd; ++iPoint)
			{
				auto& r = pwl.m_pPoints[iPoint];
				r.x = r.x * fScaleX - fOriginX;
				r.y = r.y * fScaleY - fOriginY;
			}
			iPointBegin = iPointEnd;
		}
	}

	pwl.Swap(r);
}

/**
 * Equivalent to stbtt_MakeGlyphBitmap, except the generated
 * bitmap contains signed distance field data.
 *
 * iWidth and iHeight are expected to have been expanded
 * to contain kiDiameterSDF additional pixels beyond the glyph
 * dimensions.
 *
 * fScaleX and fScaleY are expected to have been computed based
 * on the base desired glyph size, which will be (iWidth - kiDiameterSDF) x
 * (iHeight - kiDiameterSDF) pixels.
 */
void MakeGlyphBitmapSDF(
	stbtt_fontinfo const* pInfo,
	UInt8* pOutput,
	Int32 iWidth,
	Int32 iHeight,
	Int32 iStride,
	Float fScaleX,
	Float fScaleY,
	Int32 iGlyph)
{
	// Nop if no size to populate.
	if (iWidth == 0 || iHeight == 0)
	{
		return;
	}

	ContoursPWL pwl(pInfo->userdata);

	// Compute the origin offset, used to translate pixel coordinates
	// placed into the output byte buffer into the curve's space.
	//
	// This is the glyph bounding box adjusted to the SDF margin,
	// clamped to pixel coordinates.
	{
		Float fOriginX = -(Float)kiRadiusSDF;
		Float fOriginY = -(Float)kiRadiusSDF;

		Int32 iX = 0;
		Int32 iY = 0;
		stbtt_GetGlyphBox(pInfo, iGlyph, &iX, &iY, nullptr, nullptr);

		fOriginX += (Float)iX * fScaleX;
		fOriginY += (Float)iY * fScaleY;

		fOriginX = Floor(fOriginX);
		fOriginY = Floor(fOriginY);

		// Now compute the PWL approximation to the contours.
		GetContoursPWL(
			pInfo,
			fOriginX,
			fOriginY,
			fScaleX,
			fScaleY,
			iGlyph,
			pwl);
	}

	// Nothing more to do if we have no data.
	if (!pwl.HasData())
	{
		return;
	}

	// Get list acceleration structure for inside query.
	LineLists vLineLists;
	GetLineLists(
		iWidth,
		iHeight,
		pwl,
		vLineLists);

	SquareDistances aSquareDistances(iWidth * iHeight);
	aSquareDistances.Fill(FloatMax);

	// Hot portion of a hot function - this loop is perf. critical (minor modifications
	// can change performance by whole milliseconds per glyph), take care when modifying.

	// First pass, iterate over contours, then compute the distance to all pixels
	// within the kiRadiusSDF bounding box around each line segment of the contours.
	Int iPointBegin = 0;
	for (Int32 iContour = 0; iContour < pwl.m_iContours; ++iContour)
	{
		Int const iPointEnd = (iPointBegin + pwl.m_piPointCounts[iContour]);
		Int32 iPrevPoint = (iPointEnd - 1);
		for (Int32 iPoint = iPointBegin; iPoint < iPointEnd; ++iPoint)
		{
			// Get the endpoints of the segment.
			Vector2D const e0(pwl.m_pPoints[iPrevPoint].x, pwl.m_pPoints[iPrevPoint].y);
			Vector2D const e1(pwl.m_pPoints[iPoint].x, pwl.m_pPoints[iPoint].y);

			// Move onto the next point.
			iPrevPoint = iPoint;

			// Compute common values.
			Vector2D const d(e1 - e0);
			Float32 const fSeparationSquared = d.LengthSquared();

			// Skip single points - all contours are closed, so this kind of line segment
			// doesn't provide any additional information (the point is also the end point or
			// start point of another line).
			if (IsZero(fSeparationSquared))
			{
				continue;
			}

			// Convenience/perf.
			Float32 const fInvSeparationSquared = 1.0f / fSeparationSquared;

			// Get min/max and expand it by the radius.
			Vector2D vMin(Vector2D::Min(e0, e1));
			Vector2D vMax(Vector2D::Max(e0, e1));

			// TODO: -1 here makes sense to me (the depth will be quantized to
			// the max value) but -2 is surprising.
			// This means that the outer 2 pixels of the expanded SDF box are currently wasted.

			// Shrink the area that we're computing against for perf. The outer 2 pixels will always
			// be at the quantized max distance anyway, so we're not contributing anything.
			vMax += Vector2D((Float)kiRadiusSDF - 2);
			vMin -= Vector2D((Float)kiRadiusSDF - 2);

			// Compute the min/max in integers.
			Int32 const iX0 = Max((Int32)Floor(vMin.X), 0);
			Int32 const iY0 = Max((Int32)Floor(vMin.Y), 0);
			Int32 const iX1 = Min((Int32)Ceil(vMax.X), iWidth - 1);
			Int32 const iY1 = Min((Int32)Ceil(vMax.Y), iHeight - 1);

			// Now compute and mixin the distances in the range.
			for (Int32 iY = iY0; iY <= iY1; ++iY)
			{
				for (Int32 iX = iX0; iX <= iX1; ++iX)
				{
					// Get the pixel coordinates as a Vector2D.
					Vector2D const v((Float)iX, (Float)iY);

					// Line point computation. Project the point onto the line
					// (treat the line as a plane), then compute the distance
					// squared between that projection and the point.
					//
					// This has been unfolded a bit for perf. reasons.
					Vector2D const vMinusE0(v - e0);
					Float32 const fT = (Vector2D::Dot(vMinusE0, d) * fInvSeparationSquared);

					Float fDistSquared;
					if (fT <= 0.0f)
					{
						fDistSquared = vMinusE0.LengthSquared();
					}
					else if (fT >= 1.0f)
					{
						fDistSquared = (e1 - v).LengthSquared();
					}
					else
					{
						Vector2D const vProjection = e0 + fT * d;
						fDistSquared = (vProjection - v).LengthSquared();
					}

					// Merge the value.
					Int32 const iIndex = (iY * iWidth + iX);
					aSquareDistances[iIndex] = Min(fDistSquared, aSquareDistances[iIndex]);
				}
			}
		}

		iPointBegin = iPointEnd;
	}

	// Final pass, determine inside/outside and fill in the final bitmap.
	UInt8* pOut = (pOutput + (iHeight * iStride));
	for (Int32 iY = 0; iY < iHeight; ++iY)
	{
		// Start the current row.
		pOut -= iStride;
		auto const& vLines = vLineLists[iY];

		// Special variation when vLines is empty, no pixels are inside on this line.
		if (vLines.IsEmpty())
		{
			for (Int32 iX = 0; iX < iWidth; ++iX)
			{
				// For lines with no affectors, just the unsigned distance.
				Float const fDistanceSquared = aSquareDistances[iY * iWidth + iX];
				Float const fDistance = Sqrt(fDistanceSquared);

				// Compute the final value.
				Float const fScaledDistance = fDistance * (255.0f / (Float)kiRadiusSDF);
				pOut[iX] = (UInt8)Clamp((255.0f - (fScaledDistance + kfNegativeQuantizeSDF)) + 0.5f, 0.0f, 255.0f);
			}
		}
		// Otherwise, standard processing.
		else
		{
			UInt32 uLineSample = 0u;
			Float fLineSample = vLines.Front();
			UInt32 const uLines = vLines.GetSize();

			Bool bInside = false;
			for (Int32 iX = 0; iX < iWidth; ++iX)
			{
				// Check inside/outside - this applies the enter/leave rule. Font
				// curves (and many polygon based rasterizers) setup lines so that counting
				// the number of line intersections determines whether a pixel is inside or
				// outside the shape (whether it should be filled or not).
				while ((Float)iX >= fLineSample)
				{
					bInside = !bInside;
					++uLineSample;
					fLineSample = (uLineSample < uLines ? vLines[uLineSample] : FloatMax);
				}

				// Signed distance - negative if inside.
				Float const fDistanceSquared = aSquareDistances[iY * iWidth + iX];
				Float const fDistance = Sqrt(fDistanceSquared);
				Float const fSignedDistance = (bInside ? -fDistance : fDistance);

				// Compute the final value.
				Float const fScaledDistance = fSignedDistance * (255.0f / (Float)kiRadiusSDF);
				pOut[iX] = (UInt8)Clamp((255.0f - (fScaledDistance + kfNegativeQuantizeSDF)) + 0.5f, 0.0f, 255.0f);
			}
		}
	}
}

/**
 * Utility, builds a UniChar -> index mapping table for
 * all valid UniChars in the font.
 */
void GetUniCharToIndexTable(stbtt_fontinfo const* pInfo, UniCharToIndex& rt)
{
	// Cache inputs.
	auto const pData = pInfo->data;
	UInt32 const uIndexMap = pInfo->index_map;

	// Table we're populating.
	UniCharToIndex t;

	// Varous TTF data, see stbtt_FindGlyphIndex().
	auto const eFormat = ttUSHORT(pData + uIndexMap + 0);
	switch (eFormat)
	{
		// Format 0: Byte encoding table: https://www.microsoft.com/typography/otspec/cmap.htm
	case 0:
		{
			UInt32 const uSizeInBytes = ttUSHORT(pData + uIndexMap + 2);
			if (uSizeInBytes > 6u)
			{
				for (UInt32 i = 0u; i < uSizeInBytes - 6u; ++i)
				{
					Int32 const iIndex = (Int32)ttBYTE(pData + uIndexMap + 6u + i);
					UniChar const ch = (UniChar)i;

					// Sanity check.
					SEOUL_ASSERT(stbtt_FindGlyphIndex(pInfo, ch) == iIndex);

					if (IsValidUnicodeChar(ch))
					{
						SEOUL_VERIFY(t.Insert(ch, iIndex).Second);
					}
				}
			}
		}
		break;

		// Format 2: High-byte mapping through table: https://www.microsoft.com/typography/otspec/cmap.htm
	case 2:
		{
			SEOUL_FAIL("Not implemented.");
		}
		break;

		// Format 4: Segment mapping to delta values: https://www.microsoft.com/typography/otspec/cmap.htm
	case 4:
		{
			// Segcount is stored in the file format as Segcount * 2.
			UInt32 const uSegCountX2 = ttUSHORT(pData + uIndexMap + 6u);
			UInt32 const uSegCount = (uSegCountX2 / 2);

			UInt8 const* pEndCounts = (pData + uIndexMap + 14);
			UInt8 const* pStartCounts = (pEndCounts + uSegCountX2 + 2);
			UInt8 const* pIdDeltas = (pStartCounts + uSegCountX2);
			UInt8 const* pIdRangeOffsets = (pIdDeltas + uSegCountX2);

			for (UInt32 i = 0u; i < uSegCount; ++i)
			{
				UInt16 const uStartCode = (ttUSHORT(pStartCounts + (2 * i)));
				UInt16 const uEndCode = (ttUSHORT(pEndCounts + (2 * i)));
				Int16 const iDelta = (ttSHORT(pIdDeltas + (2 * i)));
				UInt16 const uRangeOffset = (ttUSHORT(pIdRangeOffsets + (2 * i)));

				for (UInt32 uChar = uStartCode; uChar <= uEndCode; ++uChar)
				{
					// This bit is ugly, to quote from the link above:
					//
					// "
					// If the idRangeOffset value for the segment is not 0, the mapping
					// of character codes relies on glyphIdArray. The character code offset
					// from startCode is added to the idRangeOffset value. This sum is used
					// as an offset from the current location within idRangeOffset itself to
					// index out the correct glyphIdArray value. This obscure indexing trick
					// works because glyphIdArray immediately follows idRangeOffset in the font
					// file. The C expression that yields the glyph index is:
					//   *(idRangeOffset[i] / 2 + (c - startCount[i]) + &idRangeOffset[i])
					// "
					if (uRangeOffset != 0)
					{
						Int32 const iIndex = (Int32)ttUSHORT(pIdRangeOffsets + (2 * i) + ((uChar - uStartCode) * 2) + uRangeOffset);
						UniChar const ch = (UniChar)uChar;

						// Sanity check.
						SEOUL_ASSERT(stbtt_FindGlyphIndex(pInfo, ch) == iIndex);

						if (IsValidUnicodeChar(ch))
						{
							SEOUL_VERIFY(t.Insert(ch, iIndex).Second);
						}
					}
					else
					{
						// UInt16 cast here is necessary, from the link above:
						// "
						// If the idRangeOffset is 0, the idDelta value is added directly
						// to the character code offset (i.e. idDelta[i] + c) to get the
						// corresponding glyph index. Again, the idDelta arithmetic is modulo 65536.
						// "
						Int32 const iIndex = (Int32)((UInt16)((Int32)uChar + (Int32)iDelta));
						UniChar const ch = (UniChar)uChar;

						// Sanity check.
						SEOUL_ASSERT(stbtt_FindGlyphIndex(pInfo, ch) == iIndex);

						if (IsValidUnicodeChar(ch))
						{
							SEOUL_VERIFY(t.Insert(ch, iIndex).Second);
						}
					}
				}
			}
		}
		break;

		// Format 6: Trimmed table mapping: https://www.microsoft.com/typography/otspec/cmap.htm
	case 6:
		{
			UInt16 const uFirst = ttUSHORT(pData + uIndexMap + 6u);
			UInt16 const uCount = ttUSHORT(pData + uIndexMap + 8u);

			for (UInt32 i = 0u; i < uCount; ++i)
			{
				Int32 const iIndex = (Int32)ttUSHORT(pData + uIndexMap + 10 + (i * 2u));
				UniChar const ch = (UniChar)(i + uFirst);

				// Sanity check.
				SEOUL_ASSERT(stbtt_FindGlyphIndex(pInfo, ch) == iIndex);

				if (IsValidUnicodeChar(ch))
				{
					SEOUL_VERIFY(t.Insert(ch, iIndex).Second);
				}
			}
		}
		break;

		// Format 12: Segmented coverage: https://www.microsoft.com/typography/otspec/cmap.htm
	case 12:
		{
			UInt8 const* pGroups = (pData + uIndexMap + 16);
			UInt32 const uGroups = ttULONG(pData + uIndexMap + 12);
			for (UInt32 i = 0u; i < uGroups; ++i)
			{
				UInt32 const uStartCharCode = ttULONG(pGroups + (i * 12) + 0u);
				UInt32 const uEndCharCode = ttULONG(pGroups + (i * 12) + 4u);
				UInt32 const uStartGlyphID = ttULONG(pGroups + (i * 12) + 8u);

				for (UInt32 uChar = uStartCharCode; uChar <= uEndCharCode; ++uChar)
				{
					Int32 const iIndex = (Int32)(uStartGlyphID + (uChar - uStartCharCode));
					UniChar ch = (UniChar)uChar;

					// Sanity check.
					SEOUL_ASSERT(stbtt_FindGlyphIndex(pInfo, ch) == iIndex);

					if (IsValidUnicodeChar(ch))
					{
						SEOUL_VERIFY(t.Insert(ch, iIndex).Second);
					}
				}
			}
		}
		break;

		// Format 13: Many-to-one range mappings: https://www.microsoft.com/typography/otspec/cmap.htm
		// 13 is identical to 12, except that uGlyphID is just the idea for all characters
		// in the range, instead of a delta.
	case 13:
		{
			UInt8 const* pGroups = (pData + uIndexMap + 16);
			UInt32 const uGroups = ttULONG(pData + uIndexMap + 12);
			for (UInt32 i = 0u; i < uGroups; ++i)
			{
				UInt32 const uStartCharCode = ttULONG(pGroups + (i * 12) + 0u);
				UInt32 const uEndCharCode = ttULONG(pGroups + (i * 12) + 4u);
				UInt32 const uGlyphID = ttULONG(pGroups + (i * 12) + 8u);

				for (UInt32 uChar = uStartCharCode; uChar <= uEndCharCode; ++uChar)
				{
					Int32 const iIndex = (Int32)uGlyphID;
					UniChar ch = (UniChar)uChar;

					// Sanity check.
					SEOUL_ASSERT(stbtt_FindGlyphIndex(pInfo, ch) == iIndex);

					if (IsValidUnicodeChar(ch))
					{
						SEOUL_VERIFY(t.Insert(ch, iIndex).Second);
					}
				}
			}
		}
		break;

	default:
		SEOUL_FAIL("Not implemented.");
		break;
	};

	rt.Swap(t);
}

} // namespace Seoul::Falcon
