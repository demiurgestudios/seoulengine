/**
 * \file CookManagerMoriarty.h
 * \brief CookManager implementation for cooking remote files through the
 * Moriarty server.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COOK_MANAGER_MORIARTY_H
#define COOK_MANAGER_MORIARTY_H

#include "StandardPlatformIncludes.h"

#if SEOUL_WITH_MORIARTY

#include "CookManager.h"

namespace Seoul
{

class CookManagerMoriarty : public CookManager
{
protected:
	// Cooks one remote file, optionally only if it's out-of-date
	virtual CookResult DoCook(FilePath filePath, Bool bCheckTimestamp) SEOUL_OVERRIDE;
};

} // namespace Seoul

#endif  // SEOUL_WITH_MORIARTY

#endif // include guard
