/**
 * \file EditorUIRecentDocuments.h
 * \brief Editor utility to maintain a list of recent documents.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_RECENT_DOCUMENTS_H
#define EDITOR_UI_RECENT_DOCUMENTS_H

#include "Delegate.h"
#include "FileChangeNotifier.h"
#include "FilePath.h"
#include "Mutex.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
#include "SeoulString.h"
#include "Vector.h"
namespace Seoul { namespace Jobs { class Job; } }

namespace Seoul::EditorUI
{

class RecentDocuments SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(RecentDocuments);

	typedef Vector<FilePath> RecentDocumentsVector;

	RecentDocuments(
		GameDirectory eDirectory,
		FileType eType);
	~RecentDocuments();

private:
	GameDirectory const m_eDirectory;
	FileType const m_eType;
	ScopedPtr<FileChangeNotifier> m_pNotifier;
	SharedPtr<Jobs::Job> m_pJob;

	friend class RecentDocumentsLock;
	Mutex m_Mutex;
	RecentDocumentsVector m_vDocuments;

	void InternalRefresh();

	void OnFileChange(
		const String& sOldPath,
		const String& sNewPath,
		FileChangeNotifier::FileEvent eEvent);

	SEOUL_DISABLE_COPY(RecentDocuments);
}; // class RecentDocuments

/** Lock provides exclusive access to the documents list. */
class RecentDocumentsLock SEOUL_SEALED
{
public:
	RecentDocumentsLock(const RecentDocuments& docs)
		: m_Lock(docs.m_Mutex)
		, m_Docs(docs)
	{
	}

	~RecentDocumentsLock()
	{
	}

	const RecentDocuments::RecentDocumentsVector& GetDocuments() const { return m_Docs.m_vDocuments; }

private:
	Lock m_Lock;
	const RecentDocuments& m_Docs;

	SEOUL_DISABLE_COPY(RecentDocumentsLock);
}; // clsas RecentDocumentsLock

} // namespace Seoul::EditorUI

#endif // include guard
