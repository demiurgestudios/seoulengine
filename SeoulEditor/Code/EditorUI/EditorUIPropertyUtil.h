/**
 * \file EditorUIPropertyUtil.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_PROPERTY_UTIL_H
#define EDITOR_UI_PROPERTY_UTIL_H

#include "Prereqs.h"
#include "SeoulHString.h"
#include "Vector.h"

namespace Seoul::EditorUI
{

namespace PropertyUtil
{

struct NumberOrHString SEOUL_SEALED
{
	NumberOrHString()
		: m_uId(0u)
		, m_Id()
	{
	}

	NumberOrHString(UInt32 uId)
		: m_uId(uId)
		, m_Id()
	{
	}

	NumberOrHString(HString id)
		: m_uId(0u)
		, m_Id(id)
	{
	}

	Bool operator==(const NumberOrHString& b) const
	{
		return
			(m_uId == b.m_uId) &&
			(m_Id == b.m_Id);
	}

	Bool operator!=(const NumberOrHString& b) const
	{
		return !(*this == b);
	}

	UInt32 m_uId;
	HString m_Id;
}; // struct NumberOrHString
typedef Vector<NumberOrHString, MemoryBudgets::Editor> Path;

} // namespace PropertyUtil

} // namespace Seoul::EditorUI

#endif // include guard
