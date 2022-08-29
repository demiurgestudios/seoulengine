/**
 * \file FalconEditTextLink.h
 * \brief Encapsulation of a hyperlink in a Falcon::EditTextInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_EDIT_TEXT_LINK_H
#define FALCON_EDIT_TEXT_LINK_H

#include "FalconTypes.h"
#include "SeoulString.h"
#include "Vector.h"

namespace Seoul::Falcon
{

class EditTextLink SEOUL_SEALED
{
public:
	EditTextLink()
		: m_sLinkString()
		, m_sType()
		, m_vBounds()
	{
	}

	String m_sLinkString;
	String m_sType;

	Vector<Rectangle, MemoryBudgets::Falcon> m_vBounds;

private:
	SEOUL_REFERENCE_COUNTED(EditTextLink);
}; // struct EditTextLink

} // namespace Seoul::Falcon

#endif // include guard
