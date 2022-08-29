/**
 * \file FalconEditTextInstance.h
 * \brief The instance of an EditTextDefinition
 * in the Falcon scene graph.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_EDIT_TEXT_INSTANCE_H
#define FALCON_EDIT_TEXT_INSTANCE_H

#include "Delegate.h"
#include "FalconEditTextLink.h"
#include "FalconInstance.h"
#include "FalconTextChunk.h"
#include "FalconTypes.h"
#include "HashTable.h"
#include "Lexer.h"
#include "ReflectionDeclare.h"
#include "SeoulHString.h"
#include "SeoulString.h"
#include "Vector.h"
namespace Seoul { class HtmlReader; }

namespace Seoul::Falcon
{

// forward declarations
class BitmapDefinition;
class EditTextDefinition;

class EditTextInstance SEOUL_SEALED : public Instance
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(EditTextInstance);

	EditTextInstance(const SharedPtr<EditTextDefinition const>& pEditTextDefinition);
	~EditTextInstance();

	virtual void Advance(
		AdvanceInterface& rInterface) SEOUL_OVERRIDE;

	virtual Instance* Clone(AddInterface& rInterface) const SEOUL_OVERRIDE
	{
		EditTextInstance* pReturn = SEOUL_NEW(MemoryBudgets::Falcon) EditTextInstance(m_pEditTextDefinition);
		CloneTo(rInterface, *pReturn);
		return pReturn;
	}

	virtual Bool ComputeLocalBounds(Rectangle& rBounds) SEOUL_OVERRIDE;

	void CommitFormatting()
	{
		CheckFormatting();
	}

	virtual void Pose(
		Render::Poser& rPoser,
		const Matrix2x3& mParent,
		const ColorTransformWithAlpha& cxParent) SEOUL_OVERRIDE;

	// Developer only feature, traversal for rendering hit testable areas.
#if SEOUL_ENABLE_CHEATS
	virtual void PoseInputVisualization(
		Render::Poser& rPoser,
		const Matrix2x3& mParent,
		RGBA color) SEOUL_OVERRIDE;
#endif

	virtual void Draw(
		Render::Drawer& rDrawer,
		const Rectangle& worldBoundsPreClip,
		const Matrix2x3& mWorld,
		const ColorTransformWithAlpha& cxWorld,
		const TextureReference& textureReference,
		Int32 iSubInstanceId) SEOUL_OVERRIDE;

	Bool GetAutoSizeBottom() const
	{
		return m_bAutoSizeBottom;
	}

	Bool GetAutoSizeContents() const
	{
		return m_bAutoSizeContents;
	}

	Bool GetAutoSizeHorizontal() const
	{
		return m_bAutoSizeHorizontal;
	}

	RGBA GetCursorColor() const
	{
		return m_CursorColor;
	}

	const SharedPtr<EditTextDefinition const>& GetDefinition() const
	{
		return m_pEditTextDefinition;
	}

	Bool GetHasTextEditFocus() const
	{
		return m_bHasTextEditFocus;
	}

	Rectangle GetLocalBounds() const;

	Bool GetTextBounds(Rectangle& bounds) const;
	Bool GetLocalTextBounds(Rectangle& bounds) const;
	Bool GetWorldTextBounds(Rectangle& bounds) const;

	Int32 GetNumLines() const
	{
		Int32 const iNumLines = Seoul::Max(
			m_vTextChunks.IsEmpty() ? 0 : (m_vTextChunks.Back().m_iLine + 1),
			m_vImages.IsEmpty() ? 0 : (m_vImages.Back().m_iStartingTextLine + 1));

		return iNumLines;
	}

	const String& GetPlainText() const
	{
		return m_sText;
	}

	const String& GetText() const
	{
		return m_sText;
	}

	const String& GetXhtmlText() const
	{
		return m_sMarkupText;
	}

	virtual InstanceType GetType() const SEOUL_OVERRIDE
	{
		return InstanceType::kEditText;
	}

	Bool GetVerticalCenter() const
	{
		return m_bVerticalCenter;
	}

	Bool GetXhtmlParsing() const
	{
		return m_bXhtmlParsing;
	}

	virtual Bool HitTest(
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY,
		Bool bIgnoreVisibility = false) const SEOUL_OVERRIDE;

	Bool LinkHitTest(
		SharedPtr<EditTextLink>& rpLinkIndex,
		Float32 fWorldX,
		Float32 fWorldY);

	/**
	 * Enable/disable bottom auto-sizing.
	 *
	 * When true, the bottom border of the text box will be expanded
	 * or shrunk to fit the actual contents size.
	 *
	 * This parameter is mutually exclusive from content auto-sizing
	 * when the text box is multiline and uses word wrapping. When true,
	 * content auto-sizing is effectively disabled for multi-line, word
	 * wrapped text boxes.
	 */
	void SetAutoSizeBottom(Bool bAutoSizeBottom)
	{
		m_bAutoSizeBottom = bAutoSizeBottom;
		m_bNeedsFormatting = true;
	}

	/**
	 * Enable/disable horizontal auto-sizing.
	 *
	 * When true, the left or right border will be
	 * expanded to fit the contents, depending on the
	 * alignment mode of a line of text.
	 */
	void SetAutoSizeHorizontal(Bool bAutoSizeHorizontal)
	{
		m_bAutoSizeHorizontal = bAutoSizeHorizontal;
		m_bNeedsFormatting = true;
	}

	/**
	 * Enable/disable contents auto-sizing.
	 *
	 * When true (the default), contents that have clipped against
	 * the bounds of the text box will be resized to fit (within a
	 * max threshold).
	 */
	void SetAutoSizeContents(Bool b)
	{
		m_bAutoSizeContents = b;
		m_bNeedsFormatting = true;
	}

	void SetCursorColor(RGBA cursorColor)
	{
		m_CursorColor = cursorColor;
	}

	void SetHasTextEditFocus(Bool bHasTextEditFocus)
	{
		m_bHasTextEditFocus = bHasTextEditFocus;
		if (!m_bHasTextEditFocus)
		{
			m_fCursorBlinkTimer = 0.0f;
		}
	}

	/** Explicit set text without XHTML formatting. Sets text and disables XHTML mode. */
	void SetPlainText(const String& sText)
	{
		m_bXhtmlParsing = false;
		SetText(sText);
	}

	/** Explicitly set XHTML text. Sets text and enables XHTML mode. */
	void SetXhtmlText(const String& sText)
	{
		m_bXhtmlParsing = true;
		SetText(sText);
	}

	void SetText(const String& sText)
	{
		// Once text has been set explicitly, we don't want to apply the
		// initial/default text anymore.
		m_bUseInitialText = false;
		m_sText = sText;
		m_sMarkupText = sText;
		m_bNeedsFormatting = true;
	}

	void SetVerticalCenter(Bool bVerticalCenter)
	{
		m_bVerticalCenter = bVerticalCenter;
		m_bNeedsFormatting = true;
	}

	void SetXhtmlParsing(Bool bXhtmlParsing)
	{
		m_bXhtmlParsing = bXhtmlParsing;
		m_bNeedsFormatting = true;
	}

	UInt32 GetLinkCount()
	{
		return m_vLinks.GetSize();
	}

	const Vector< SharedPtr<EditTextLink>, MemoryBudgets::Falcon >& GetLinks()
	{
		return m_vLinks;
	}

	// TODO: This functionality does not consider inline images.

	/** @return Max number of text characters that will be rendered. */
	UInt32 GetVisibleCharacters() const { return m_VisibleCharacters.m_uVisibleCount; }
	/**
	 * Set the max number of characters that will be rendered.
	 *
	 * Can be used to animate text. Text is formatted once and then the visible element
	 * field can be used to progressively display the characters.
	 */
	void SetVisibleCharacters(UInt32 uVisibleCount)
	{
		m_VisibleCharacters.m_uVisibleCount = uVisibleCount;
		InternalRefreshVisibleCharacters();
	}

	HtmlAlign GetAlignment() const;

	struct ImageEntry
	{
		ImageEntry();
		~ImageEntry();

		Float ComputeCenterY() const;
		Float GetHeight() const;
		Float GetRightBorder() const;
		Float GetWidth() const;
		Bool IsValid() const;

		SharedPtr<BitmapDefinition> m_pBitmap;
		Vector4D m_vTextureCoordinates;
		Float m_fXOffset;
		Float m_fYOffset;
		Float m_fXMargin;
		Float m_fYMargin;
		Int16 m_iLinkIndex;
		int m_iStartingTextLine;
		HtmlAlign m_eAlignment;
		HtmlImageAlign m_eImageAlignment;
		Float m_fRescale;
	};

	typedef Vector<ImageEntry, MemoryBudgets::Falcon> Images;
	typedef Vector<SharedPtr<EditTextLink>, MemoryBudgets::Falcon > Links;
	typedef Vector<TextChunk, MemoryBudgets::Falcon> TextChunks;

#if SEOUL_UNIT_TESTS
	const Images& UnitTesting_GetImages() const
	{
		return m_vImages;
	}

	const Links& UnitTesting_GetLinks() const
	{
		return m_vLinks;
	}

	const TextChunks& UnitTesting_GetTextChunks() const
	{
		return m_vTextChunks;
	}
#endif

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(EditTextInstance);

	class PlainAutoSizeUtil;
	class XhtmlAutoSizeUtil;

	struct LineBreakRecord SEOUL_SEALED
	{
		UInt32 m_uTextChunk{};
		UInt32 m_uNumberOfCharacters{};
		UInt32 m_uOffset{};
		Float m_f{};

		Bool IsValid() const
		{
			// We never set a break option at the start of the entire
			// string, so 0 value is an effective invalid identifier.
			return (0u != m_uOffset);
		}

		void Reset()
		{
			m_uTextChunk = 0u;
			m_uNumberOfCharacters = 0u;
			m_uOffset = 0u;
			m_f = 0.0f;
		}
	};

	Float AdvanceLine(Float fCurrentY, TextChunk& rTextChunk);
	void ComputeGlyphBounds();
	void ApplyAlignmentAndCentering();
	void ApplyImageAlignmentAndFixupBaseline(Int32 iLine);
	void AutoSizeBottom();
	void AutoSizeHorizontal();
	void CheckFormatting(AdvanceInterface& rInterface);
	void CheckFormatting();
	void CloneTo(AddInterface& rInterface, EditTextInstance& rClone) const;
	Float ComputeContentsBottom() const;
	Float ComputeContentsTop() const;
	Float ComputeContentsLeft() const;
	Float ComputeContentsRight() const;
	void DrawOutline(Render::Drawer& rDrawer, const Rectangle& localBounds, const TextChunk& rTextChunk, const Matrix2x3& mWorld, const ColorTransformWithAlpha& cxWorld, const Vector2D& outlineOffset, const ColorARGBu8& outlineColor, const UInt8 outlineWidth, const UInt8 outlineBlur);
	void FormatNode(
		HtmlReader& rReader,
		LineBreakRecord& rLastLineBreakOption,
		HtmlTag eTag,
		TextChunk& rTextChunk,
		Float fAutoSizeRescale);
	void FormatPlainText();
	void FormatPlainTextInner(Float fAutoSizeRescale);
	void FormatText();
	void FormatTextChunk(
		LineBreakRecord& rLastLineBreakOption,
		TextChunk& rInOutTextChunk,
		Bool bAllowReflow = true);
	void FormatXhtmlText();
	void FormatXhtmlTextInner(Float fAutoSizeRescale);
	void FormatWithAutoContentSizing(const Delegate<void(Float fAutoSizeRescale)>& formatter);
	Bool GetCanWordWrap() const;
	Bool GetDetailTexture(
		const Matrix2x3& mWorld,
		Render::Poser& rPoser,
		const TextChunk& chunk,
		TextureReference& detail);
	Bool GetGlyphX0(Float fX, UniChar c, const TextChunk& textChunk, Float& rfX0) const;
	Bool GetGlyphX1(Float fX, UniChar c, const TextChunk& textChunk, Float& rfX1) const;
	Bool GetGlyphY0Y1(Float fY, UniChar c, const TextChunk& textChunk, Float& rfY0, Float& rfY1) const;
	Bool GetGlyphY0(Float fY, UniChar c, const TextChunk& textChunk, Float& rfY0) const;
	Bool GetGlyphY1(Float fY, UniChar c, const TextChunk& textChunk, Float& rfY1) const;
	Bool GetInitialTextChunk(TextChunk& rTextChunk, Float fAutoSizeRescale) const;
	Float GetInitialY() const;
	Float GetLineStart(Bool bNewParagraph) const;
	Float GetLineRightBorder(Int iLine, TextChunks::SizeType& ruTextChunk, Images::SizeType& ruImage) const;
	Float GetRightMargin() const;
	Float GetWordWrapX() const;
	void PostFormat(
		LineBreakRecord& rLastLineBreakOption,
		HtmlTag eTag,
		TextChunk& rTextChunk);
	void PreFormat(
		HtmlReader& rReader,
		HtmlTag eTag,
		TextChunk& rTextChunk,
		Float fAutoSizeRescale,
		HtmlTagStyle& reStyle);
	void ResetFormattedState();
	void Reflow(
		const LineBreakRecord& option,
		Float& rfX,
		Float& rfY,
		TextChunk& rTextChunk);

	Float ComputeContentsTopFromGlyphBounds() const;
	Float ComputeContentsBottomFromGlyphBounds() const;

	SharedPtr<EditTextDefinition const> m_pEditTextDefinition;
	Images m_vImages;
	Links m_vLinks;
	TextChunks m_vTextChunks;
	String m_sText;
	String m_sMarkupText;
	Float m_fCursorBlinkTimer;
	RGBA m_CursorColor;
	Float32 m_fBottom;
	Float32 m_fLeft;
	Float32 m_fRight;
	// TODO: Remove/reduce. Visible count is a very
	// special case (currently used to implement progressive
	// text display for e.g. NPC dialogue) and ideally
	// shouldn't contribute to the fat of EditTextInstance
	// generally.
	struct
	{
		UInt32 m_uVisibleCount = UIntMax;
		UInt32 m_uPartiallyVisibleTextChunk = UIntMax;
		UInt32 m_uPartiallyVisibleCharacterCount = 0u;
	} m_VisibleCharacters;
	void InternalRefreshVisibleCharacters();
	void InternalApplyVisibleToChunk(TextChunk& rChunk) const;

	// 32-bits of other members.
	union
	{
		UInt32 m_uOptions;
		struct
		{
			UInt32 m_bNeedsFormatting : 1;
			UInt32 m_bUseInitialText : 1;
			UInt32 m_bVerticalCenter : 1;
			UInt32 m_bAutoSizeBottom : 1;
			UInt32 m_bXhtmlParsing : 1;
			UInt32 m_bHasTextEditFocus : 1;
			UInt32 m_bXhtmlVerticalCenter : 1;
			UInt32 m_bAutoSizeContents : 1;
			UInt32 m_bAutoSizeHorizontal : 1;
			UInt32 m_uUnusedReserved : 23;
		};
	};

	SEOUL_DISABLE_COPY(EditTextInstance);
}; // class EditTextInstance
template <> struct InstanceTypeOf<EditTextInstance> { static const InstanceType Value = InstanceType::kEditText; };

} // namespace Seoul::Falcon

#endif // include guard
