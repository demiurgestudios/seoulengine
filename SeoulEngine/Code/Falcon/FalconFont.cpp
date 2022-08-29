/**
 * \file FalconFont.cpp
 * \brief Encapsulates TTF data, used primarily by Falcon::EditTextInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconFont.h"
#include "FalconTypes.h"

namespace Seoul::Falcon
{

FontDefinition::FontDefinition(
	const Font& font,
	UInt16 uDefinitionID)
	: Definition(DefinitionType::kFont, uDefinitionID)
	, m_Font(font)
{
}

FontDefinition::~FontDefinition()
{
}

} // namespace Seoul::Falcon
