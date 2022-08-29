/**
 * \file PCCookManager.h
 * \brief Specialization of CookManager on the PC - handles cooking by
 * delegating cooking tasks to cooker applications in SeoulEngine's Tools
 * folder.
 *
 * \warning PCCookManager does not handle disabling/enabling cooking
 * for ship builds or other cases - if you want to completely disable
 * cooking, do not create a PCCookManager, conditionally create
 * a NullCookManager in these cases.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PC_COOK_MANAGER_H
#define PC_COOK_MANAGER_H

#include "CookManager.h"
#include "HashSet.h"
#include "Mutex.h"
namespace Seoul { class CookDatabase; }

namespace Seoul
{

class PCCookManager SEOUL_SEALED : public CookManager
{
public:
	SEOUL_DELEGATE_TARGET(PCCookManager);

	static CheckedPtr<PCCookManager> Get()
	{
		return (PCCookManager*)CookManager::Get().Get();
	}

	PCCookManager();
	virtual ~PCCookManager();

	virtual FilePath GetCurrent() const SEOUL_OVERRIDE
	{
		return m_CurrentlyCooking;
	}

	virtual void GetDependents(FilePath filePath, Dependents& rv) const SEOUL_OVERRIDE;

private:
	virtual CookResult DoCook(FilePath filePath, Bool bOnlyIfNeeded) SEOUL_OVERRIDE;

	CookResult GenericCook(
		FilePath filePath,
		Bool bCheckTimestamp,
		Byte const* sCookerExeFilename);

	void ClearCurrentlyCooking()
	{
		m_CurrentlyCooking = FilePath();
	}

	void SynchronizedCurrentlyCookingSet(FilePath newFilePath);

	Mutex m_Mutex;
	ScopedPtr<CookDatabase> m_pCookDatabase;
	FilePath m_CurrentlyCooking;
	Bool m_bIssuedMissingCookerExecutableLog;

	SEOUL_DISABLE_COPY(PCCookManager);
}; // class PCCookManager

} // namespace Seoul

#endif // include guard
