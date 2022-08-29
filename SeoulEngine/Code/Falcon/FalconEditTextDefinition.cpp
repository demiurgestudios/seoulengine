/**
 * \file FalconEditTextDefinition.cpp
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

#include "FalconEditTextCommon.h"
#include "FalconEditTextDefinition.h"
#include "FalconEditTextInstance.h"
#include "FalconFCNFile.h"
#include "FalconSwfReader.h"
#include "Logger.h"

namespace Seoul::Falcon
{

Bool EditTextDefinition::Read(FCNFile& rFile, SwfReader& rBuffer)
{
	m_sFCNFileURL = rFile.GetURL();
	m_Bounds = rBuffer.ReadRectangle();

	rBuffer.Align();

	m_bHasText = rBuffer.ReadBit();
	m_bWordWrap = rBuffer.ReadBit();
	m_bMultiline = rBuffer.ReadBit();
	m_bPassword = rBuffer.ReadBit();
	m_bReadOnly = rBuffer.ReadBit();
	m_bHasTextColor = rBuffer.ReadBit();
	m_bHasMaxLength = rBuffer.ReadBit();
	m_bHasFontDefinition = rBuffer.ReadBit();
	m_bHasFontClass = rBuffer.ReadBit();
	m_bAutoSize = rBuffer.ReadBit();
	m_bHasLayout = rBuffer.ReadBit();
	m_bNoSelect = rBuffer.ReadBit();
	m_bBorder = rBuffer.ReadBit();
	m_bWasStatic = rBuffer.ReadBit();
	m_bHtml = rBuffer.ReadBit();
	m_bUseOutlines = rBuffer.ReadBit();

	// Error for both to be true
	if (m_bHasFontDefinition && m_bHasFontClass)
	{
		SEOUL_WARN("'%s' is unsupported or corrupted, EditTextDefinition "
			"has both HasFontDefinition and HasFontClass set, this should "
			"never be true",
			rFile.GetURL().CStr());
		return false;
	}

	if (m_bHasFontDefinition)
	{
		m_uFontDefinitionID = rBuffer.ReadUInt16();

		SharedPtr<Definition> p;
		rFile.GetDefinition(m_uFontDefinitionID, p);
		if (p.IsValid() && p->GetType() == DefinitionType::kFont)
		{
			m_pFontDefinition.Reset((FontDefinition*)p.GetPtr());
		}
	}

	if (m_bHasFontClass)
	{
		m_sFontClass = rBuffer.ReadHString();
		rFile.GetImportedDefinition(m_sFontClass, m_pFontDefinition);
	}

	if (m_bHasFontDefinition || m_bHasFontClass)
	{
		m_fFontHeight = TwipsToPixels(rBuffer.ReadUInt16());
	}

	if (m_bHasTextColor)
	{
		m_TextColor = rBuffer.ReadRGBA();
		m_SecondaryTextColor = m_TextColor;
	}

	if (m_bHasMaxLength)
	{
		m_uMaxLength = rBuffer.ReadUInt16();
	}

	if (m_bHasLayout)
	{
		m_eAlign = (HtmlAlign)rBuffer.ReadUInt8();
		m_fLeftMargin = TwipsToPixels(rBuffer.ReadUInt16());
		m_fRightMargin = TwipsToPixels(rBuffer.ReadUInt16());
		m_fWordWrapMargin = m_fRightMargin;
		m_fIndent = TwipsToPixels(rBuffer.ReadUInt16());
		m_fLeading = TwipsToPixels(rBuffer.ReadInt16());
	}

	m_sVariableName = rBuffer.ReadHString();

	if (m_bHasText)
	{
		m_sInitialText = rBuffer.ReadString();
	}

	// Apply adjustment factors - see constants, derived empirically with
	// Flash A/B tests. NOTE: The relative factor (that is preportional
	// to font height) does not appear to be applicable to images.
	using namespace EditTextCommon;
	m_fLeftMargin += kfTextPaddingLeft;
	m_fRightMargin += kfTextPaddingRight;
	m_fWordWrapMargin += kfTextPaddingWordWrap;
	m_fTopMargin = kfTextPaddingTopAbs;
	return true;
}

void EditTextDefinition::DoCreateInstance(SharedPtr<Instance>& rp) const
{
	rp.Reset(SEOUL_NEW(MemoryBudgets::Falcon) EditTextInstance(SharedPtr<EditTextDefinition const>(this)));
}

} // namespace Seoul::Falcon
