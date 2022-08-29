/**
 * \file FalconFont.h
 * \brief Encapsulates TTF data, used primarily by Falcon::EditTextInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_FONT_H
#define FALCON_FONT_H

#include "FalconDefinition.h"
#include "FalconTypes.h"
#include "SeoulHString.h"
#include "Vector.h"

namespace Seoul::Falcon
{

struct Font SEOUL_SEALED
{
	Font()
		: m_pData()
		, m_sName()
		, m_Overrides()
		, m_bBold(false)
		, m_bItalic(false)
	{
	}

	SharedPtr<CookedTrueTypeFontData> m_pData;
	HString m_sName;
	FontOverrides m_Overrides;
	Bool m_bBold;
	Bool m_bItalic;
}; // struct Font

class FontDefinition SEOUL_SEALED : public Definition
{
public:
	FontDefinition(const Font& font, UInt16 uDefinitionID);
	~FontDefinition();

	const Font& GetFont() const
	{
		return m_Font;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(FontDefinition);

	Font m_Font;

	SEOUL_DISABLE_COPY(FontDefinition);
}; // class FontDefinition
template <> struct DefinitionTypeOf<FontDefinition> { static const DefinitionType Value = DefinitionType::kFont; };

} // namespace Seoul::Falcon

#endif // include guard
