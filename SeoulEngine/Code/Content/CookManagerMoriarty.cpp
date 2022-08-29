/**
 * \file CookManagerMoriarty.cpp
 * \brief CookManager implementation for cooking remote files through the
 * Moriarty server.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "MoriartyClient.h"
#include "CookManagerMoriarty.h"

#if SEOUL_WITH_MORIARTY

namespace Seoul
{

/**
 * Cooks one remote file, optionally only if it's out-of-date
 */
CookManager::CookResult CookManagerMoriarty::DoCook(FilePath filePath, Bool bCheckTimestamp)
{
	if (!MoriartyClient::Get())
	{
		return kErrorCookingFailed;
	}

	Int32 eResult;
	if (MoriartyClient::Get()->CookFile(filePath, bCheckTimestamp, &eResult))
	{
		return (CookResult)eResult;
	}
	else
	{
		return kErrorCookingFailed;
	}
}

} // namespace Seoul

#endif  // SEOUL_WITH_MORIARTY
