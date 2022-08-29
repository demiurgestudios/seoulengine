/**
 * \file Cooker.h
 * \brief Root instance to create to access SeoulEngine content
 * cooking facilities.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COOKER_H
#define COOKER_H

#include "CookerSettings.h"
#include "FilePath.h"
#include "HashTable.h"
#include "Prereqs.h"
#include "SccPerforceClientParameters.h"
#include "ScopedPtr.h"
#include "SeoulTime.h"
#include "SharedPtr.h"
#include "Vector.h"
namespace Seoul { class CookDatabase; }
namespace Seoul { class SyncFile; }
namespace Seoul { namespace Cooking { class CookerConstructJob; } }
namespace Seoul { namespace Cooking { struct CookerState; } }

namespace Seoul::Cooking
{

class Cooker SEOUL_SEALED
{
public:
	Cooker(const CookerSettings& settings);
	~Cooker();

	Bool CookAllOutOfDateContent();
	Bool CookSingle();

private:
	SEOUL_DISABLE_COPY(Cooker);

	Bool FinishConstruct();
	Bool ValidateContentDir() const;
	Bool ValidateContentEnvironment();

	CookerSettings const m_Settings;
	SharedPtr<CookerConstructJob> m_pConstructJob;
	ScopedPtr<CookerState> m_pState;
}; // class Cooker

} // namespace Seoul::Cooking

#endif // include guard
