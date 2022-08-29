/**
 * \file AtomicHandle.cpp
 * \brief Thread-safe equivalent to Seoul::Handle. Likely and eventually,
 * will be fully merged with Handle.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AtomicHandle.h"

namespace Seoul
{

AtomicHandleTableCommon::Data::Data()
	: m_Pool(Entry())
	, m_PoolIndirect((Entry*)nullptr)
	, m_AllocatedCount(0)
{
}

} // namespace Seoul
