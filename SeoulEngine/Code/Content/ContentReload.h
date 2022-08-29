/**
 * \file ContentReload.h
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

#pragma once
#ifndef CONTENT_RELOAD_H
#define CONTENT_RELOAD_H

#include "HashSet.h"
#include "FilePath.h"
#include "SharedPtr.h"
#include "Vector.h"
namespace Seoul { namespace Content { class EntryBase; } }

namespace Seoul::Content
{

/** Utility structure, used by the Content::LoadManager::Reload() method. */
struct Reload SEOUL_SEALED
{
	typedef Vector< SharedPtr<EntryBase>, MemoryBudgets::Content> Reloaded;

	void Clear()
	{
		m_vReloaded.Clear();
	}

	// Return overall progress of content reload operations.
	void GetProgress(UInt32& ruToReload, UInt32& ruReloaded) const;
	
	/** @return true if any reload ops are still pending, false otherwise. */
	Bool IsLoading() const
	{
		UInt32 uToReload = 0u;
		UInt32 uReloaded = 0u;
		GetProgress(uToReload, uReloaded);
		return (uReloaded < uToReload);
	}

	void Swap(Reload& r)
	{
		m_vReloaded.Swap(r.m_vReloaded);
	}

	Reloaded m_vReloaded;
}; // struct Content::Reload

} // namespace Seoul::Content

#endif // include guard
