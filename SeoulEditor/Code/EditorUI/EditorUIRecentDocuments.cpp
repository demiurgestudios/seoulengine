/**
 * \file EditorUIRecentDocuments.cpp
 * \brief Editor utility to maintain a list of recent documents.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorUIRecentDocuments.h"
#include "Engine.h"
#include "JobsFunction.h"

namespace Seoul::EditorUI
{

RecentDocuments::RecentDocuments(
	GameDirectory eDirectory,
	FileType eType)
	: m_eDirectory(eDirectory)
	, m_eType(eType)
	, m_pNotifier()
	, m_pJob()
{
	// Initial population.
	m_pJob = Jobs::MakeFunction(SEOUL_BIND_DELEGATE(&RecentDocuments::InternalRefresh, this));
	m_pJob->StartJob();

	// Register for refresh events, if supported.
	auto const sPath(Engine::Get()->GetRecentDocumentPath());
	if (!sPath.IsEmpty())
	{
		m_pNotifier.Reset(SEOUL_NEW(MemoryBudgets::Editor) FileChangeNotifier(
			sPath,
			SEOUL_BIND_DELEGATE(&RecentDocuments::OnFileChange, this),
			FileChangeNotifier::kAll));
	}
}

RecentDocuments::~RecentDocuments()
{
	// Kill the notifier.
	m_pNotifier.Reset();

	// Kill the job, if active.
	m_pJob->WaitUntilJobIsNotRunning();
	m_pJob.Reset();
}

namespace
{

struct Sorter SEOUL_SEALED
{
	Bool operator()(const FilePath& a, const FilePath& b) const
	{
		return strcmp(a.CStr(), b.CStr()) < 0;
	}
};

} // namespace anonymous

void RecentDocuments::InternalRefresh()
{
	// Early out if can't retrieve recent documents.
	Engine::RecentDocuments v;
	if (!Engine::Get()->GetRecentDocuments(
		m_eDirectory,
		m_eType,
		v))
	{
		return;
	}

	// Sort and replace.
	Sorter sorter;
	QuickSort(v.Begin(), v.End(), sorter);

	Lock lock(m_Mutex);
	m_vDocuments.Swap(v);
}

void RecentDocuments::OnFileChange(
	const String& sOldPath,
	const String& sNewPath,
	FileChangeNotifier::FileEvent /*eEvent*/)
{
	// Trigger a refresh, unless one is already running.
	{
		Lock lock(m_Mutex);
		if (!m_pJob.IsValid() || !m_pJob->IsJobRunning())
		{
			m_pJob = Jobs::MakeFunction(SEOUL_BIND_DELEGATE(&RecentDocuments::InternalRefresh, this));
			m_pJob->StartJob();
		}
	}
}

} // namespace Seoul::EditorUI
