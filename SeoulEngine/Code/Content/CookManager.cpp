/**
 * \file CookManager.cpp
 * \brief CookManager handles converting raw asset files into cooked files that
 * SeoulEngine can load. Whether cooking is available or not is platform
 * dependent. The NullCookManager can be used to disable cooking, for example,
 * in ship builds or on platforms that do not support cooking.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CookManager.h"

namespace Seoul
{

/**
 * When specialized by a CookManager, attempt a cook with the following rules and return the result:
 * - if bCheckTimestamp is true, only cook if a comparison of source file/destination file time stamps
 *   indicates that the cooked file is out of sync with the source file.
 * - if bCheckTimestamp is false, always attempt to cook.
 * - return a result code of type CookResult, see description of CookResult members for guidance
 *   of when different codes should be returned.
 *
 * WARNING: Subclasses of Cook must always support the following:
 * - Cook must be thread safe
 * - While cooking a file, the CookManager subclass must guard against another Cook being queued
 *   for the same file
 */
CookManager::CookResult CookManager::Cook(FilePath filePath, Bool bCheckTimestamp /*= true*/)
{
	if (!IsCookingEnabled())
	{
		return kErrorCookingDisabled;
	}

	return DoCook(filePath, bCheckTimestamp);
}

/**
 * When specialized by a CookManager subclass, should return true if a file of
 * type eType can be cooked. Note that this function should return true if
 * eType is the type of both the "cooked" and "source" versions of a file.
 */
Bool CookManager::SupportsCooking(FileType eType) const
{
	return FileTypeNeedsCooking(eType);
}

} // namespace Seoul
