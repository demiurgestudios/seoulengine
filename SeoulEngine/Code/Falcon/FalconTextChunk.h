/**
 * \file FalconTextChunk.h
 * \brief The smallest unit of text data that can be submitted
 * to the Falcon render backend.
 *
 * Each text chunk has size, style, and character data that fully
 * defines its contents. A single text chunk can have only one style
 * and one size. Style and size changes require the generation of
 * a new text chunk.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_TEXT_CHUNK_H
#define FALCON_TEXT_CHUNK_H

#include "FalconFont.h"
#include "FilePath.h"
#include "SeoulString.h"

namespace Seoul::Falcon
{

/**
 * Text effects support a "face" texture, which is layered
 * across the text. This setting describes the framing
 * of that face texture.
 */
enum class TextEffectDetailMode
{
	/* Texture will be layered across the entire chunk. e.g. the
	 * left edge of the texture aligns with the left edge of the chunk,
	 * and the right edge of the texture aligns with the right edge
	 * of the chunk. */
	Word,
	
	/* Texture will be layered across individual characters. e.g. the
	 * left edge of the texture aligns with the left edge of each
	 * individual character, and the right edge of the texture aligns
	 * with the right edge of each individual character. */
	Character,
};

/**
 * Text effects support a "face" texture, which is layered
 * across the text. This setting describes the aspect ratio
 * of that face texture.
 */
enum class TextEffectDetailStretchMode
{
	/* Aspect ratio is not respected and the texture is stretched to the area
	 * as defined by the TextEffectDetailMode. */
	Stretch,
	
	/* The texture is stretched to the width as defined by the TextEffectDetailMode,
	 * then the height is scaled to maintain the aspect ratio of the face texture. */
	FitWidth,
	
	/* The texture is stretched to the height as defined by the TextEffectDetailMode,
	 * then the width is scaled to maintain the aspect ratio of the face texture. */
	FitHeight,
};

/**
 * Describe advanced text effects that can be applied
 * via markup. The settings are stored in a global table
 * and referenced via the (SeoulEngine specific) <font effect=>
 * attribute.
 */
struct TextEffectSettings SEOUL_SEALED
{
	TextEffectSettings()
		: m_vShadowOffset(0, 0)
		, m_ShadowColor(ColorARGBu8::TransparentBlack())
		, m_pTextColor(nullptr)
		, m_pTextColorTop(nullptr)
		, m_pTextColorBottom(nullptr)
		, m_uShadowBlur(0u)
		, m_uShadowOutlineWidth(0u)
		, m_vExtraOutlineOffset(0, 0)
		, m_ExtraOutlineColor(ColorARGBu8::TransparentBlack())
		, m_uExtraOutlineBlur(0u)
		, m_uExtraOutlineWidth(0u)
		, m_bShadowEnable(false)
		, m_bExtraOutlineEnable(false)
		, m_eDetailMode(TextEffectDetailMode::Word)
		, m_eDetailStretchMode(TextEffectDetailStretchMode::Stretch)
		, m_vDetailOffset(0.0f, 0.0f)
		, m_vDetailSpeed(0.0f, 0.0f)
		, m_DetailFilePath()
		, m_bDetail(false)
		, m_vDetailAnimOffsetInWorld(0.0f, 0.0f)
	{
	}

	~TextEffectSettings()
	{
		SafeDelete(m_pTextColorBottom);
		SafeDelete(m_pTextColorTop);
		SafeDelete(m_pTextColor);
	}

	Vector2D m_vShadowOffset;
	// TODO: ColorARGBu8 here for serialization purposes, need to unify RGBA and this.
	ColorARGBu8 m_ShadowColor;
	CheckedPtr<ColorARGBu8> m_pTextColor;
	CheckedPtr<ColorARGBu8> m_pTextColorTop;
	CheckedPtr<ColorARGBu8> m_pTextColorBottom;
	UInt8 m_uShadowBlur;
	UInt8 m_uShadowOutlineWidth;

	Vector2D m_vExtraOutlineOffset;
	ColorARGBu8 m_ExtraOutlineColor;
	UInt8 m_uExtraOutlineBlur;
	UInt8 m_uExtraOutlineWidth;

	Bool m_bShadowEnable;
	Bool m_bExtraOutlineEnable;

	TextEffectDetailMode m_eDetailMode;
	TextEffectDetailStretchMode m_eDetailStretchMode;
	Vector2D m_vDetailOffset;
	Vector2D m_vDetailSpeed;
	FilePath m_DetailFilePath;
	Bool m_bDetail;

	// Not serialized - used at runtime to accumulate offset
	// from m_vDetailSpeed.
	Vector2D m_vDetailAnimOffsetInWorld;

private:
	SEOUL_DISABLE_COPY(TextEffectSettings);
}; // struct TextEffectSettings

struct TextChunk SEOUL_SEALED
{
	TextChunk()
		: m_Format()
		, m_fXOffset(0)
		, m_fYOffset(0)
		, m_fLeftGlyphBorder(0)
		, m_fRightGlyphBorder(0)
		, m_fTopGlyphBorder(0)
		, m_fBottomGlyphBorder(0)
		, m_iBegin()
		, m_iEnd()
		, m_uNumberOfCharacters(0)
		, m_iLine(0)
	{
	}

	struct Formatting
	{
		Formatting()
			: m_Font()
			, m_TextColor(RGBA::Black())
			, m_SecondaryTextColor(RGBA::Black())
			, m_TextEffectSettings()
			, m_iAlignment((Int16)HtmlAlign::kLeft)
			, m_iLinkIndex(-1)
			, m_fTextHeight(0)
			, m_fLetterSpacing(0.0f)
		{
		}

		Font m_Font;
		RGBA m_TextColor;
		RGBA m_SecondaryTextColor;
		HString m_TextEffectSettings;
		// This is an Int16 instead of the Enum to save space.
		Int16 m_iAlignment;
		Int16 m_iLinkIndex;

		Float GetLetterSpacing() const
		{
			return m_Font.m_Overrides.m_fRescale * m_fLetterSpacing;
		}

		Float GetLineGap() const
		{
			if (m_Font.m_pData.IsValid())
			{
				Float const fTextHeight = GetTextHeight();
				return m_Font.m_pData->GetLineGap(m_Font.m_Overrides) * m_Font.m_pData->GetScaleForPixelHeight(fTextHeight);
			}
			else
			{
				return 0.0;
			}
		}

		Float GetLineHeight() const
		{
			Float const fTextHeight = GetTextHeight();
			if (m_Font.m_pData.IsValid())
			{
				return m_Font.m_pData->ComputeLineHeightFromTextHeight(m_Font.m_Overrides, fTextHeight);
			}
			else
			{
				return fTextHeight;
			}
		}
		
		Float GetTextHeight() const
		{
			return m_Font.m_Overrides.m_fRescale * m_fTextHeight;
		}

		Float GetUnscaledLetterSpacing() const
		{
			return m_fLetterSpacing;
		}

		Float GetUnscaledTextHeight() const
		{
			return m_fTextHeight;
		}

		HtmlAlign GetAlignmentEnum() const
		{
			return (HtmlAlign)m_iAlignment;
		}

		void SetAlignmentEnum(HtmlAlign eAlignment)
		{
			m_iAlignment = (Int16)eAlignment;
		}

		void SetUnscaledLetterSpacing(Float fLetterSpacing)
		{
			m_fLetterSpacing = fLetterSpacing;
		}

		void SetUnscaledTextHeight(Float fTextHeight)
		{
			m_fTextHeight = fTextHeight;
		}

	private:
		Float m_fTextHeight;
		Float m_fLetterSpacing;
	} m_Format;

	/**
	 * Tight fitting bounding box - excludes any oversize
	 * for outline, etc. Note intended for render bounds.
	 */
	Rectangle ComputeGlyphBounds() const
	{
		Rectangle ret;
		ret.m_fLeft = m_fLeftGlyphBorder;
		ret.m_fRight = m_fRightGlyphBorder;
		ret.m_fTop = m_fTopGlyphBorder;
		ret.m_fBottom = m_fBottomGlyphBorder;
		return ret;
	}

	/**
	 * Vertex bounds when rendering - use for rendering bounding
	 * box.
	 */
	Rectangle ComputeRenderBounds() const;

	Float ComputeCenterY() const
	{
		return (m_fYOffset + m_Format.GetLineHeight() * 0.5f);
	}

	Float m_fXOffset;
	Float m_fYOffset;
	Float m_fLeftGlyphBorder;
	Float m_fRightGlyphBorder;
	Float m_fTopGlyphBorder;
	Float m_fBottomGlyphBorder;
	String::Iterator m_iBegin;
	String::Iterator m_iEnd;
	UInt32 m_uNumberOfCharacters;
	Int32 m_iLine;
}; // struct TextChunk

} // namespace Seoul::Falcon

#endif // include guard
