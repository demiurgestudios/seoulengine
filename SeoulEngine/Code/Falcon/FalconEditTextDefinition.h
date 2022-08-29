/**
 * \file FalconEditTextDefinition.h
 * \brief A definition of a Flash text box.
 *
 * Falcon implements both immutable and mutable text boxes
 * with an EditTextDefinition and its corresponding
 * EditTextInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_EDIT_TEXT_DEFINITION_H
#define FALCON_EDIT_TEXT_DEFINITION_H

#include "FalconDefinition.h"
#include "FalconTypes.h"
#include "SeoulString.h"

namespace Seoul::Falcon
{

// forward declarations
class FontDefinition;
class SwfReader;

class EditTextDefinition SEOUL_SEALED : public Definition
{
public:
	EditTextDefinition(UInt16 uDefinitionID)
		: Definition(DefinitionType::kEditText, uDefinitionID)
		, m_pFontDefinition()
		, m_eUseFlashType(UseFlashType::kNormalRenderer)
		, m_eGridFit(GridFit::kDoNotUseGridFitting)
		, m_fThickness(0.0f)
		, m_fSharpness(0.0f)
		, m_Bounds()
		, m_bHasText(false)
		, m_sVariableName()
		, m_sInitialText()
		, m_bWordWrap(false)
		, m_bMultiline(false)
		, m_bPassword(false)
		, m_bReadOnly(false)
		, m_bHasTextColor(false)
		, m_TextColor(RGBA::Black())
		, m_SecondaryTextColor(RGBA::Black())
		, m_bHasMaxLength(false)
		, m_uMaxLength(0)
		, m_bHasFontDefinition(false)
		, m_fFontHeight(0.0f)
		, m_uFontDefinitionID(0)
		, m_bHasFontClass(false)
		, m_sFontClass()
		, m_bAutoSize(false)
		, m_bHasLayout(false)
		, m_eAlign(HtmlAlign::kLeft)
		, m_fLeftMargin(0.0f)
		, m_fRightMargin(0.0f)
		, m_fWordWrapMargin(0.0f)
		, m_fTopMargin(0.0f)
		, m_fIndent(0.0f)
		, m_fLeading(0.0f)
		, m_bNoSelect(false)
		, m_bBorder(false)
		, m_bWasStatic(false)
		, m_bHtml(false)
		, m_bUseOutlines(false)
	{
	}

	Bool Read(FCNFile& rFile, SwfReader& rBuffer);

	HtmlAlign GetAlignment() const
	{
		return m_eAlign;
	}

	const Rectangle& GetBounds() const
	{
		return m_Bounds;
	}

	const SharedPtr<FontDefinition>& GetFontDefinition() const
	{
		return m_pFontDefinition;
	}

	UInt16 GetFontDefinitionID() const
	{
		return m_uFontDefinitionID;
	}

	const HString& GetFontDefinitionName() const
	{
		return m_sFontClass;
	}

	Float32 GetFontHeight() const
	{
		return m_fFontHeight;
	}

	Float32 GetIndent() const
	{
		return m_fIndent;
	}

	Float32 GetLeading() const
	{
		return m_fLeading;
	}

	Float32 GetLeftMargin() const
	{
		return m_fLeftMargin;
	}

	Float32 GetRightMargin() const
	{
		return m_fRightMargin;
	}

	Float32 GetTopMargin() const
	{
		return m_fTopMargin;
	}

	Float32 GetWordWrapMargin() const
	{
		return m_fWordWrapMargin;
	}

	Bool HasWordWrap() const
	{
		return m_bWordWrap;
	}

	Bool IsMultiline() const
	{
		return m_bMultiline;
	}

	const String& GetInitialText() const
	{
		return m_sInitialText;
	}

	RGBA GetTextColor() const
	{
		return m_TextColor;
	}

	RGBA GetSecondaryTextColor() const
	{
		return m_SecondaryTextColor;
	}

	Bool HasTextColor() const
	{
		return m_bHasTextColor;
	}

	HString GetFCNFileURL() const
	{
		return m_sFCNFileURL;
	}

	Bool Html() const 
	{
		return m_bHtml;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(EditTextDefinition);

	virtual void DoCreateInstance(SharedPtr<Instance>& rp) const SEOUL_OVERRIDE;

	friend class FCNFile;
	HString m_sFCNFileURL;
	SharedPtr<FontDefinition> m_pFontDefinition;
	UseFlashType m_eUseFlashType;
	GridFit m_eGridFit;
	Float m_fThickness;
	Float m_fSharpness;
	Rectangle m_Bounds;
	Bool m_bHasText;
	HString m_sVariableName;
	String m_sInitialText;
	Bool m_bWordWrap;
	Bool m_bMultiline;
	Bool m_bPassword;
	Bool m_bReadOnly;
	Bool m_bHasTextColor;
	RGBA m_TextColor;
	RGBA m_SecondaryTextColor;
	Bool m_bHasMaxLength;
	UInt16 m_uMaxLength;
	Bool m_bHasFontDefinition;
	Float32 m_fFontHeight;
	UInt16 m_uFontDefinitionID;
	Bool m_bHasFontClass;
	HString m_sFontClass;
	Bool m_bAutoSize;
	Bool m_bHasLayout;
	HtmlAlign m_eAlign;
	Float32 m_fLeftMargin;
	Float32 m_fRightMargin;
	Float32 m_fWordWrapMargin;
	Float32 m_fTopMargin;
	Float32 m_fIndent;
	Float32 m_fLeading;
	Bool m_bNoSelect;
	Bool m_bBorder;
	Bool m_bWasStatic;
	Bool m_bHtml;
	Bool m_bUseOutlines;

	SEOUL_DISABLE_COPY(EditTextDefinition);
}; // class EditTextDefinition
template <> struct DefinitionTypeOf<EditTextDefinition> { static const DefinitionType Value = DefinitionType::kEditText; };

} // namespace Seoul::Falcon

#endif // include guard
