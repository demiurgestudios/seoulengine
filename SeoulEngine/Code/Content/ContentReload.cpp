/**
 * \file ContentReload.cpp
 * \brief Utility class used by ContentLoadManager. Passed with
 * a list of content reload requests, filled and returned with a
 * list of actively reloading content.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentEntry.h"
#include "ContentReload.h"

namespace Seoul::Content
{

/** @return The total number of entries marked for reload, and the total number that have been reloaded. */
void Reload::GetProgress(UInt32& ruToReload, UInt32& ruReloaded) const
{
	ruToReload = m_vReloaded.GetSize();
	ruReloaded = 0u;
	for (UInt32 i = 0u; i < ruToReload; ++i)
	{
		ruReloaded += (m_vReloaded[i]->IsLoading() ? 0u : 1u);
	}
}

} // namespace Seoul::Content
