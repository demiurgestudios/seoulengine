/**
 * \file FalconTypes.cpp
 * \brief Dumping ground for lots of simple types used
 * by the Falcon project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

// TODO: Falcon was originally a separate project from Seoul,
// meant to be integrated into arbitrary third party engines without
// a Seoul dependency. After the Seoul merge, some types
// were left redundant and separate (RGBA, Rectangle) and still need
// to be replaced with their Seoul counterparts.
//
// Further, this header is likely too heavy in general. Types
// should be refactored out into smaller header files.

#include "Compress.h"
#include "FalconStbTrueType.h"
#include "FalconTypes.h"
#include "Logger.h"
#include "Platform.h"
#include "ReflectionDefine.h"
#include "StreamBuffer.h"

// Cooking functions must be updated for big endian platforms.
SEOUL_STATIC_ASSERT(SEOUL_LITTLE_ENDIAN);

namespace Seoul
{

SEOUL_BEGIN_TYPE(Falcon::ColorTransform)
	SEOUL_PROPERTY_N("MulR", m_fMulR)
	SEOUL_PROPERTY_N("MulG", m_fMulG)
	SEOUL_PROPERTY_N("MulB", m_fMulB)
	SEOUL_PROPERTY_N("AddR", m_AddR)
	SEOUL_PROPERTY_N("AddG", m_AddG)
	SEOUL_PROPERTY_N("AddB", m_AddB)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Falcon::ColorTransformWithAlpha)
	SEOUL_PROPERTY_N("MulR", m_fMulR)
	SEOUL_PROPERTY_N("MulG", m_fMulG)
	SEOUL_PROPERTY_N("MulB", m_fMulB)
	SEOUL_PROPERTY_N("MulA", m_fMulA)
	SEOUL_PROPERTY_N("AddR", m_AddR)
	SEOUL_PROPERTY_N("AddG", m_AddG)
	SEOUL_PROPERTY_N("AddB", m_AddB)
	SEOUL_PROPERTY_N("BlendingFactor", m_uBlendingFactor)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Falcon::Rectangle)
	SEOUL_PROPERTY_N("Left", m_fLeft)
	SEOUL_PROPERTY_N("Right", m_fRight)
	SEOUL_PROPERTY_N("Top", m_fTop)
	SEOUL_PROPERTY_N("Bottom", m_fBottom)
SEOUL_END_TYPE()

namespace Falcon
{

static const UInt32 kuCookedDataSignature = 0xB89FB3E9;
static const UInt32 kuCookedDataVersion = 3u;

CookedTrueTypeFontData::CookedTrueTypeFontData(
	const HString& sUniqueIdentifier,
	void* pData,
	UInt32 uSizeInBytes)
	: m_Data(MemoryBudgets::FalconFont)
	, m_tGlyphs()
	, m_iAscent(0)
	, m_iDescent(0)
	, m_iLineGap(0)
	, m_sUniqueIdentifier(sUniqueIdentifier)
	, m_pInfo(SEOUL_NEW(MemoryBudgets::Falcon) stbtt_fontinfo)
	, m_fGlyphScaleSDF(0.0f)
	, m_bHasValidData(false)
{
	m_Data.TakeOwnership((Byte*)pData, uSizeInBytes);
	InitData();
}

CookedTrueTypeFontData::~CookedTrueTypeFontData()
{
}

Float CookedTrueTypeFontData::GetGlyphAdvance(UniChar uCodePoint) const
{
	CookedGlyphEntry const* p = nullptr;
	if (!m_tGlyphs.GetValue(uCodePoint, p))
	{
		return 0;
	}

	return (Float)p->m_iAdvanceInPixels;
}

Float CookedTrueTypeFontData::GetGlyphAdvance(UniChar uCodePoint, Float fGlyphHeight) const
{
	CookedGlyphEntry const* p = nullptr;
	if (!m_tGlyphs.GetValue(uCodePoint, p))
	{
		return 0.0f;
	}

	return (Float)p->m_iAdvanceInPixels * (fGlyphHeight / kfGlyphHeightSDF);
}

Bool CookedTrueTypeFontData::GetGlyphBitmapDataSDF(
	UniChar uCodePoint,
	void*& rpData,
	Int32& riWidth,
	Int32& riHeight) const
{
	CookedGlyphEntry const* pGlyph = nullptr;
	if (!m_tGlyphs.GetValue(uCodePoint, pGlyph))
	{
		return false;
	}

	// Compute dimensions.
	Int32 const iGlyphWidth = (pGlyph->m_iX1 - pGlyph->m_iX0) + 1;
	Int32 const iGlyphHeight = (pGlyph->m_iY1 - pGlyph->m_iY0) + 1;
	Int32 const iFullWidth = (iGlyphWidth + kiDiameterSDF);
	Int32 const iFullHeight = (iGlyphHeight + kiDiameterSDF);

	// Generate the data.
	void* p = MemoryManager::Allocate((UInt32)iFullWidth * (UInt32)iFullHeight, MemoryBudgets::UIRuntime);
	memset(p, 0, (UInt32)iFullWidth * (UInt32)iFullHeight);
	MakeGlyphBitmapSDF(
		m_pInfo.Get(),
		(UInt8*)p,
		iFullWidth,
		iFullHeight,
		iFullWidth,
		m_fGlyphScaleSDF,
		m_fGlyphScaleSDF,
		pGlyph->m_iGlyphIndex);

	rpData = p;
	riWidth = iFullWidth;
	riHeight = iFullHeight;
	return true;
}

/** Assumes a single line of basic characters. */
Bool CookedTrueTypeFontData::Measure(
	String::Iterator iStringBegin,
	String::Iterator iStringEnd,
	const FontOverrides& overrides,
	Float fPixelHeight,
	Float& rfX0,
	Float& rfY0,
	Float& rfWidth,
	Float& rfHeight,
	Bool bIncludeTrailingWhitespace /*= false*/) const
{
	if (iStringBegin == iStringEnd)
	{
		return false;
	}

	// Progress.
	auto i = iStringBegin;
	auto const iEnd = --iStringEnd;

	// First glyph.
	CookedGlyphEntry const* p = nullptr;
	if (!m_tGlyphs.GetValue(*i, p))
	{
		return false;
	}

	Int32 iX0 = p->m_iX0;
	Int32 iY0 = p->m_iY0;
	Int32 iY1 = p->m_iY1;
	Int32 iX1 = 0;
	while (i != iEnd)
	{
		// Accum advance from previous glyph.
		iX1 += p->m_iAdvanceInPixels;

		// Get next glyph, will be used on loop or fall through.
		++i;
		if (!m_tGlyphs.GetValue(*i, p))
		{
			return false;
		}

		iY0 = Min(iY0, p->m_iY0);
		iY1 = Max(iY1, p->m_iY1);
	}

	// Add in right side of last glyph.
	if (bIncludeTrailingWhitespace && 0 == (p->m_iX1 - p->m_iX0))
	{
		iX1 += p->m_iAdvanceInPixels;
	}
	else
	{
		iX1 += p->m_iX1;
	}

	// Rescale values.
	auto const fScale = GetScaleForPixelHeight(fPixelHeight);
	auto const fAscent = GetAscent(overrides);
	auto const fOnePixel = GetOneEmForPixelHeight(fPixelHeight);

	// Compute.
	rfX0 = Floor(iX0 * fScale);
	rfY0 = Floor(((Float)iY0 + fAscent) * fScale);
	rfWidth = Ceil(((Float)iX1 * fScale) + fOnePixel - rfX0); // Plus one to include last pixel in width.
	rfHeight = Ceil((((Float)iY1 + fAscent) * fScale) + fOnePixel - rfY0); // Plus one to include last pixel in height.
	return true;
}

void CookedTrueTypeFontData::InitData()
{
	// Data is initially invalid.
	m_bHasValidData = false;

	// Reset the stream.
	m_Data.SeekToOffset(0);

	// Read header data.
	UInt32 uSignature = 0u;
	if (!m_Data.Read(uSignature) || kuCookedDataSignature != uSignature)
	{
		SEOUL_WARN("%s: failed reading font data, could not read signature or invalid signature (%u).\n",
			m_sUniqueIdentifier.CStr(),
			uSignature);
		return;
	}

	UInt32 uVersion = 0u;
	if (!m_Data.Read(uVersion) || kuCookedDataVersion != uVersion)
	{
		SEOUL_WARN("%s: failed reading font data, could not read version or invalid version (%u).\n",
			m_sUniqueIdentifier.CStr(),
			uVersion);
		return;
	}

	// Read font metrics.
	if (!m_Data.Read(m_iAscent))
	{
		SEOUL_WARN("%s: failed reading font data, could not read ascent.\n",
			m_sUniqueIdentifier.CStr());
		return;
	}
	if (!m_Data.Read(m_iDescent))
	{
		SEOUL_WARN("%s: failed reading font data, could not read descent.\n",
			m_sUniqueIdentifier.CStr());
		return;
	}
	if (!m_Data.Read(m_iLineGap))
	{
		SEOUL_WARN("%s: failed reading font data, could not read line gap.\n",
			m_sUniqueIdentifier.CStr());
		return;
	}

	// Get the number of glyphs entries.
	UInt32 uEntries = 0u;
	if (!m_Data.Read(uEntries))
	{
		SEOUL_WARN("%s: failed reading font data, could not read glyph entry count.\n",
			m_sUniqueIdentifier.CStr());
		return;
	}

	// Now read glyph entries.
	UInt32 const uGlyphEntriesOffset = m_Data.GetOffset();
	Glyphs tGlyphs;
	tGlyphs.Reserve(uEntries);
	for (UInt32 i = 0u; i < uEntries; ++i)
	{
		// Cache the pointer.
		CookedGlyphEntry const* p = (CookedGlyphEntry const*)(m_Data.GetBuffer() + uGlyphEntriesOffset + (i * sizeof(CookedGlyphEntry)));

		// Sanity check that we handled alignment correctly.
		SEOUL_ASSERT((size_t)p % SEOUL_ALIGN_OF(Int32) == 0);

		// Insert the entry.
		if (!tGlyphs.Insert(p->m_uCodePoint, p).Second)
		{
			SEOUL_WARN("%s: failed reading font data, invalid duplicate glyph entry '%u'\n",
				m_sUniqueIdentifier.CStr(),
				(UInt32)p->m_uCodePoint);
			return;
		}
	}

	// Attempt to initialize the TTF data.
	memset(m_pInfo.Get(), 0, sizeof(*m_pInfo));
	auto pTtfData = (UInt8*)(m_Data.GetBuffer() + uGlyphEntriesOffset + (uEntries * sizeof(CookedGlyphEntry)));
	if (0 == stbtt_InitFont(m_pInfo.Get(), (UInt8*)pTtfData, 0))
	{
		return;
	}

	// Cache glyph font scale.
	m_fGlyphScaleSDF = stbtt_ScaleForMappingEmToPixels(m_pInfo.Get(), kfGlyphHeightSDF);

	// Done, success.
	m_tGlyphs.Swap(tGlyphs);
	m_bHasValidData = true;
}

TrueTypeFontData::TrueTypeFontData(
	const HString& sUniqueIdentifier,
	void* pData,
	UInt32 uData)
	: m_sUniqueIdentifier(sUniqueIdentifier)
	, m_iAscent(0)
	, m_iDescent(0)
	, m_iLineGap(0)
	, m_pTtfData(pData)
	, m_uTtfData(uData)
	, m_pInfo(SEOUL_NEW(MemoryBudgets::Falcon) stbtt_fontinfo)
	, m_bHasValidData(false)
{
	memset(m_pInfo.Get(), 0, sizeof(*m_pInfo));

	if (!m_pTtfData)
	{
		ReleaseData();
		return;
	}

	if (0 == stbtt_InitFont(m_pInfo.Get(), (UInt8*)m_pTtfData, 0))
	{
		ReleaseData();
		return;
	}

	m_iAscent = 0;
	m_iDescent = 0;
	m_iLineGap = 0;

	// To match our old Flash runtime, we use the usWinAscent
	// and usWinDescent values, if available (and if available,
	// line gap will always be 0).
	//
	// See also: https://docs.microsoft.com/en-us/typography/opentype/spec/os2
	if (0 == stbtt_GetFontVMetricsWin(m_pInfo.Get(), &m_iAscent, &m_iDescent, &m_iLineGap))
	{
		stbtt_GetFontVMetrics(m_pInfo.Get(), &m_iAscent, &m_iDescent, &m_iLineGap);
	}

	m_bHasValidData = true;
}

TrueTypeFontData::~TrueTypeFontData()
{
	ReleaseData();
}

Bool TrueTypeFontData::Cook(StreamBuffer& r) const
{
	// Early out if no data.
	if (!m_bHasValidData)
	{
		return false;
	}

	// Get our lookup table for resolving UniChars.
	UniCharToIndex t;
	GetUniCharToIndexTable (m_pInfo.Get(), t);

	StreamBuffer buffer;

	// Cache the scale used for the rest of this function.
	Float const fScale = stbtt_ScaleForMappingEmToPixels(m_pInfo.Get(), kfGlyphHeightSDF);

	// Signature and versioning.
	buffer.Write(kuCookedDataSignature);
	buffer.Write(kuCookedDataVersion);

	// Per-font data.
	buffer.Write((Int32)Ceil((Float32)m_iAscent * fScale));
	buffer.Write((Int32)Ceil((Float32)m_iDescent * fScale));
	buffer.Write((Int32)Ceil((Float32)m_iLineGap * fScale));

	// Glyph count.
	buffer.Write((UInt32)t.GetSize());

	// Write glyph entries - offset is zero initially,
	// we're rewind the stream and fix them up after
	// we know the starting position.
	auto const iBegin = t.Begin();
	auto const iEnd = t.End();
	for (auto i = iBegin; iEnd != i; ++i)
	{
		CookedGlyphEntry entry;
		entry.m_uCodePoint = i->First;

		// Box.
		stbtt_GetGlyphBitmapBox(
			m_pInfo.Get(),
			i->Second,
			fScale,
			fScale,
			&entry.m_iX0,
			&entry.m_iY0,
			&entry.m_iX1,
			&entry.m_iY1);

		// Metrics.
		Int32 iAdvance = 0;
		Int32 iLeftSideBearing = 0;
		stbtt_GetGlyphHMetrics(
			m_pInfo.Get(),
			i->Second,
			&iAdvance,
			&iLeftSideBearing);
		entry.m_iAdvanceInPixels = (Int32)Ceil((Float32)iAdvance * fScale);
		entry.m_iLeftSideBearingInPixels = (Int32)Ceil((Float32)iLeftSideBearing * fScale);
		entry.m_iGlyphIndex = i->Second;

		// Write out the value.
		buffer.Write(entry);
	}

	// Append TTF data.
	buffer.Write(m_pTtfData, m_uTtfData);

	// Compress.
	void* pC = nullptr;
	UInt32 uC = 0u;
	if (!ZSTDCompress(
		buffer.GetBuffer(),
		buffer.GetTotalDataSizeInBytes(),
		pC,
		uC))
	{
		return false;
	}

	// Swap,
	buffer.TakeOwnership((Byte*)pC, uC);
	pC = nullptr;
	uC = 0u;


	// Done.
	r.Swap(buffer);
	return true;
}

Float TrueTypeFontData::GetGlyphAdvance(
	UniChar uCodePoint,
	Float fGlyphHeight) const
{
	Float const fScaleForPixelHeight = stbtt_ScaleForMappingEmToPixels(m_pInfo.Get(), fGlyphHeight);
	Int32 iAdvanceInFontUnits = 0;

	// TODO: FindGlyphIndex can be slow.
	Int32 const iGlyphIndex = stbtt_FindGlyphIndex(m_pInfo.Get(), uCodePoint);
	stbtt_GetGlyphHMetrics(m_pInfo.Get(), iGlyphIndex, &iAdvanceInFontUnits, nullptr);

	return(Float)iAdvanceInFontUnits * fScaleForPixelHeight;
}

Bool TrueTypeFontData::GetGlyphBitmapBox(
	UniChar uCodePoint,
	Float fFontScale,
	Int32& riX0,
	Int32& riY0,
	Int32& riX1,
	Int32& riY1) const
{
	if (!m_bHasValidData)
	{
		return false;
	}

	// TODO: FindGlyphIndex can be slow.
	Int32 const iGlyphIndex = stbtt_FindGlyphIndex(m_pInfo.Get(), uCodePoint);
	stbtt_GetGlyphBitmapBox(m_pInfo.Get(), iGlyphIndex, fFontScale, fFontScale, &riX0, &riY0, &riX1, &riY1);
	return true;
}

Float TrueTypeFontData::GetScaleForPixelHeight(Float fPixelHeight) const
{
	if (m_bHasValidData)
	{
		return stbtt_ScaleForMappingEmToPixels(m_pInfo.Get(), fPixelHeight);
	}
	else
	{
		return 0.0f;
	}
}

Bool TrueTypeFontData::WriteGlyphBitmap(
	UniChar uCodePoint,
	UInt8* pOut,
	Int32 iGlyphWidth,
	Int32 iGlyphHeight,
	Int32 iPitch,
	Float fFontScale,
	Bool bSDF)
{
	if (!m_bHasValidData)
	{
		return false;
	}

	// TODO: FindGlyphIndex can be slow.
	Int32 const iGlyphIndex = stbtt_FindGlyphIndex(m_pInfo.Get(), uCodePoint);

	// TODO: If we're never going to use standard glyph
	// generation, best to remove this conditional and the corresponding
	// paths.
	if (bSDF)
	{
		MakeGlyphBitmapSDF(
			m_pInfo.Get(),
			pOut,
			iGlyphWidth,
			iGlyphHeight,
			iPitch,
			fFontScale,
			fFontScale,
			iGlyphIndex);
	}
	else
	{
		stbtt_MakeGlyphBitmap(
			m_pInfo.Get(),
			pOut,
			iGlyphWidth,
			iGlyphHeight,
			iPitch,
			fFontScale,
			fFontScale,
			iGlyphIndex);
	}
	return true;
}

void TrueTypeFontData::ReleaseData()
{
	MemoryManager::Deallocate(m_pTtfData);
	m_pTtfData = nullptr;
	m_uTtfData = 0u;
	m_pInfo.Reset();
	m_bHasValidData = false;
}

} // namespace Falcon

} // namespace Seoul
