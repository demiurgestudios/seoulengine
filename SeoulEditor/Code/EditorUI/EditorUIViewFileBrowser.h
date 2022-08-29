/**
 * \file EditorUIViewFileBrowser.h
 * \brief View that displays a tree
 * hierarchy of files on disk.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_VIEW_FILE_BROWSER_H
#define EDITOR_UI_VIEW_FILE_BROWSER_H

#include "DevUIView.h"
#include "FileChangeNotifier.h"
#include "HashSet.h"
#include "Mutex.h"
#include "SeoulString.h"
#include "Vector.h"
namespace Seoul { class FileChangeNotifier; }

namespace Seoul::EditorUI
{

class ViewFileBrowser SEOUL_SEALED : public DevUI::View
{
public:
	SEOUL_DELEGATE_TARGET(ViewFileBrowser);

	ViewFileBrowser(FilePath filePath);
	~ViewFileBrowser();

	// DevUI::View overrides
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		static const HString kId("Files");
		return kId;
	}
	// /DevUI::View overrides

	struct Entry SEOUL_SEALED
	{
		Entry* m_pParent{};
		FilePath m_FilePath{};
		String m_sName{};
		Vector<Entry*, MemoryBudgets::Editor>* m_pvDirChildren{};
		Vector<Entry*, MemoryBudgets::Editor>* m_pvFileChildren{};
		Bool m_bIsDirectory{};
		Bool m_bPendingExpand{};
	}; // struct Entry
	typedef Vector<Entry*, MemoryBudgets::Editor> Entries;

private:
	ScopedPtr<FileChangeNotifier> m_pFileChangeNotifier;

	Entry* m_pRootEntry;

	Mutex m_PendingChangesMutex;
	typedef HashSet<String, MemoryBudgets::Editor> PendingChanges;
	PendingChanges m_PendingChanges;

	typedef HashTable<FilePath, Entry*, MemoryBudgets::Editor> Lookup;
	Lookup m_tLookup;

	typedef HashSet<Entry*, MemoryBudgets::Editor> Selected;
	Selected m_SelectedDir;
	Selected m_SelectedFile;

	typedef HashSet<FilePath, MemoryBudgets::Editor> SetType;

	// DevUI::View overrides
	virtual void DoPrePose(DevUI::Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	// /DevUI::View overrides

	void InternalDestroy(Entry*& rp);
	void InternalDestroy(Entries*& rpvEntries);
	void InternalExpand(Entry& rEntry);
	void InternalOnFileChange(const String& sOldPath, const String& sNewPath, FileChangeNotifier::FileEvent eEvent);
	void InternalOpen(FilePath filePath);
	void InternalPrePoseDirTree(Entries& rvEntries);
	Bool InternalPrePoseFileEntry(Entry* pEntry, Entry*& rpDirChange);
	void InternalPrePoseFileView();
	void InternalPrune(SetType& set, Entries* pvEntries);
	void InternalRefresh(const String& sPath);

	SEOUL_DISABLE_COPY(ViewFileBrowser);
}; // class ViewFileBrowser

} // namespace Seoul::EditorUI

#endif // include guard
