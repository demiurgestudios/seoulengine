/**
 * \file PatchablePackageFileSystem.cpp
 * \brief Wrapper around DownloadablePackageFileSystem that adds
 * handling for updating the URL that drives the system, as well
 * as specifying a read-only fallback that is always locally available.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DownloadablePackageFileSystem.h"
#include "JobsManager.h"
#include "PatchablePackageFileSystem.h"

namespace Seoul
{

/**
 * Public access to the customized settings that a patchable
 * system uses to apply to its underlying downloadable systems.
 */
void PatchablePackageFileSystem::AdjustSettings(DownloadablePackageFileSystemSettings& r)
{
	// Patches are either mostly or entirely downloaded in full, so we want
	// their worker threads progressing aggressively.
	r.m_bNormalPriority = true;
	// We also want to retune download operations - less responsive but faster
	// overall (fewer, larger requests). This essentially doubles the default
	// configuration of a downloader.
	r.m_uMaxRedownloadSizeThresholdInBytes = 16 * 1024;
	r.m_uLowerBoundMaxSizePerDownloadInBytes = 64 * 1024;
	r.m_uUpperBoundMaxSizePerDownloadInBytes = 512 * 1024;
	r.m_fTargetPerDownloadTimeInSeconds = 1.0;
}

PatchablePackageFileSystem::PatchablePackageFileSystem(
	const String& sReadOnlyFallbackAbsoluteFilename,
	const String& sPackageAbsoluteFilename)
	: m_sReadOnlyFallbackAbsoluteFilename(sReadOnlyFallbackAbsoluteFilename)
	, m_sAbsoluteFilename(sPackageAbsoluteFilename)
	, m_pFallback(SEOUL_NEW(MemoryBudgets::Io) PackageFileSystem(sReadOnlyFallbackAbsoluteFilename))
	, m_pDownloadable()
	, m_pActive(m_pFallback.Get())
	, m_CounterMutex()
	, m_Counter(0)
{
}

PatchablePackageFileSystem::~PatchablePackageFileSystem()
{
}

/**
 * Retrieve stats from the internal downloadable file system, if
 * present.
 */
Bool PatchablePackageFileSystem::GetStats(DownloadablePackageFileSystemStats& rStats) const
{
	CounterLock lock(*this);
	if (m_pDownloadable.IsValid())
	{
		m_pDownloadable->GetStats(rStats);
		return true;
	}

	return false;
}

/** @return the URL set to the downloadable file system, if defined. */
String PatchablePackageFileSystem::GetURL() const
{
	CounterLock lock(*this);
	return (m_pDownloadable.IsValid() ? m_pDownloadable->GetURL() : String());
}

/** @return if the downloadable file system has/is experiencing a write failure. */
Bool PatchablePackageFileSystem::HasExperiencedWriteFailure() const
{
	CounterLock lock(*this);
	return (m_pDownloadable.IsValid() ? m_pDownloadable->HasExperiencedWriteFailure() : false);
}

/** @return true if the downloadable file system has work to do. */
Bool PatchablePackageFileSystem::HasWork() const
{
	CounterLock lock(*this);

	return (m_pDownloadable.IsValid() ? m_pDownloadable->HasWork() : false);
}

/** Issue prefetch in the downloadable system, if defined. */
Bool PatchablePackageFileSystem::Fetch(
	const Files& vInFilesToPrefetch,
	ProgressCallback progressCallback /*= ProgressCallback()*/,
	NetworkFetchPriority ePriority /*= NetworkFetchPriority::kDefault*/)
{
	CounterLock lock(*this);

	if (!m_pDownloadable.IsValid())
	{
		return true;
	}

	return m_pDownloadable->Fetch(vInFilesToPrefetch, progressCallback, ePriority);
}

/**
 * Blocking operation - can be expensive. Update the active URL of the
 * downloadable file system in this patchable file system. If empty, reverts
 * to the builtin file system.
 */
void PatchablePackageFileSystem::SetURL(const String& sURL)
{
	// Lock this scope in a JobAwareLock - the JobAwareLock locks
	// the counter mutex, once the counter reaches 0 - this makes sure
	// that no open operations into m_pPackageFileSystem exist before
	// we swap it out, but does so in a way that prevents the following
	// "bad things" from happening:
	// - current thread locks a mutex around code that can take an arbitrarily long time
	// - current thread locks a mutex around a call to Jobs::Manager::Get()->YieldThreadTime().
	JobAwareLock lock(*this);

	// If URL is equal to the downloadable, early out.
	if (m_pDownloadable.IsValid() && m_pDownloadable->GetURL() == sURL)
	{
		return;
	}

	// At this point, in all cases, we're destroying our existing system.
	m_pDownloadable.Reset();

	// If empty, use the fallback system.
	if (sURL.IsEmpty())
	{
		m_pActive = m_pFallback.Get();
	}
	// Otherwise, create a new downloadable.
	else
	{
		// Configure downloader with default settings.
		DownloadablePackageFileSystemSettings settings;
		settings.m_sAbsolutePackageFilename = m_sAbsoluteFilename;
		settings.m_sInitialURL = sURL;

		// Use the fallback as well.
		if (m_pFallback.IsValid() && m_pFallback->IsOk())
		{
			settings.m_vPopulatePackages.PushBack(m_sReadOnlyFallbackAbsoluteFilename);
		}

		AdjustSettings(settings);

		m_pDownloadable.Reset(SEOUL_NEW(MemoryBudgets::Io) DownloadablePackageFileSystem(settings));
		m_pActive = m_pDownloadable.Get();
	}
}

PatchablePackageFileSystem::JobAwareLock::JobAwareLock(const PatchablePackageFileSystem& r)
	: m_r(r)
{
	while (true)
	{
		if (m_r.m_CounterMutex.TryLock())
		{
			if (0 == m_r.m_Counter)
			{
				break;
			}

			m_r.m_CounterMutex.Unlock();
		}

		if (Jobs::Manager::Get())
		{
			Jobs::Manager::Get()->YieldThreadTime();
		}
	}
}

PatchablePackageFileSystem::JobAwareLock::~JobAwareLock()
{
	m_r.m_CounterMutex.Unlock();
}

} // namespace Seoul
