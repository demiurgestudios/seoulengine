/**
 * \file DelegateMemberBindHandle.cpp
 *
 * \brief Utility used by Delegate<> when binding to a member function. Equivalent
 * to Handle<>, except thread-safe and specialized for its use case within Delegate<>
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DelegateMemberBindHandle.h"

namespace Seoul
{

DelegateMemberBindHandleTable::Data::Data()
	: m_Pool(Entry())
	, m_PoolIndirect((Entry*)nullptr)
	, m_AllocatedCount(0)
{
}

DelegateMemberBindHandleTable::Data DelegateMemberBindHandleTable::s_Data;

} // namespace Seoul
