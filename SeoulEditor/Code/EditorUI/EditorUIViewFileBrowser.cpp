/**
 * \file EditorUIViewFileBrowser.cpp
 * \brief View that displays a tree
 * hierarchy of files on disk.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIImGui.h"
#include "DevUIImGuiRenderer.h"
#include "Directory.h"
#include "EditorUIRoot.h"
#include "EditorUIDragSourceFilePath.h"
#include "EditorUIIcons.h"
#include "EditorUIUtil.h"
#include "EditorUIViewFileBrowser.h"
#include "Engine.h"
#include "FileChangeNotifier.h"
#include "JobsFunction.h"
#include "Path.h"
#include "Texture.h"
#include "TextureManager.h"

namespace Seoul::EditorUI
{

ViewFileBrowser::ViewFileBrowser(FilePath filePath)
	: m_pFileChangeNotifier()
	, m_pRootEntry(SEOUL_NEW(MemoryBudgets::Editor) Entry)
	, m_PendingChangesMutex()
	, m_PendingChanges()
	, m_SelectedDir()
	, m_SelectedFile()
{
	String const sPath(filePath.GetAbsoluteFilenameInSource());
	m_pRootEntry->m_bIsDirectory = true;
	m_pRootEntry->m_FilePath = filePath;
	m_pRootEntry->m_sName = Path::GetFileName(sPath);
	SEOUL_VERIFY(m_tLookup.Insert(filePath, m_pRootEntry).Second);
	InternalExpand(*m_pRootEntry);

	m_pFileChangeNotifier.Reset(SEOUL_NEW(MemoryBudgets::Editor) FileChangeNotifier(
		sPath,
		SEOUL_BIND_DELEGATE(&ViewFileBrowser::InternalOnFileChange, this)));
}

ViewFileBrowser::~ViewFileBrowser()
{
	m_pFileChangeNotifier.Reset();
	InternalDestroy(m_pRootEntry);

	// Sanity check.
	SEOUL_ASSERT(m_tLookup.IsEmpty());
}

void ViewFileBrowser::DoPrePose(DevUI::Controller& rController, RenderPass& rPass)
{
	// Apply changes.
	{
		PendingChanges changes;
		{
			Lock lock(m_PendingChangesMutex);
			changes = m_PendingChanges;
			m_PendingChanges.Clear();
		}

		{
			auto const iBegin = changes.Begin();
			auto const iEnd = changes.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				InternalRefresh(*i);
			}
		}
	}

	auto const eFlags = (
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings);

	using namespace ImGui;

	Columns(2);

	(void)BeginChild("Directory Tree", ImVec2(0, 0), false, eFlags);
	InternalPrePoseDirTree(*m_pRootEntry->m_pvDirChildren);
	(void)EndChild();

	NextColumn();

	(void)BeginChild("File View", ImVec2(0, 0), false, eFlags);
	InternalPrePoseFileView();
	(void)EndChild();

	Columns(1);
}

void ViewFileBrowser::InternalOnFileChange(
	const String& sOldPath,
	const String& sNewPath,
	FileChangeNotifier::FileEvent eEvent)
{
	Lock lock(m_PendingChangesMutex);
	(void)m_PendingChanges.Insert(Path::GetDirectoryName(sOldPath));
	(void)m_PendingChanges.Insert(Path::GetDirectoryName(sNewPath));
}

void ViewFileBrowser::InternalDestroy(Entry*& rp)
{
	// Early out.
	if (nullptr == rp)
	{
		return;
	}

	InternalDestroy(rp->m_pvFileChildren);
	InternalDestroy(rp->m_pvDirChildren);
	(void)m_tLookup.Erase(rp->m_FilePath);
	(void)m_SelectedDir.Erase(rp);
	(void)m_SelectedFile.Erase(rp);
	SafeDelete(rp);
}

void ViewFileBrowser::InternalDestroy(Entries*& rpvEntries)
{
	// Early out.
	if (nullptr == rpvEntries)
	{
		return;
	}

	Int32 const iCount = (Int32)rpvEntries->GetSize();
	for (Int32 i = iCount - 1; i >= 0; --i)
	{
		Entry* pEntry = (*rpvEntries)[i];
		InternalDestroy(pEntry);
	}

	SafeDelete(rpvEntries);
}

namespace
{

struct Sorter SEOUL_SEALED
{
	Bool operator()(ViewFileBrowser::Entry const* pA, ViewFileBrowser::Entry const* pB) const
	{
		return (pA->m_sName < pB->m_sName);
	}
};

} // namespace anonymous

void ViewFileBrowser::InternalExpand(Entry& rEntry)
{
	typedef HashSet<FilePath, MemoryBudgets::Editor> SetType;
	typedef Vector<String> VectorT;

	VectorT vsResults;
	if (!Directory::GetDirectoryListing(
		rEntry.m_FilePath.GetAbsoluteFilenameInSource(),
		vsResults,
		true,
		false))
	{
		// Simple case, just prune all entries.
		InternalDestroy(rEntry.m_pvFileChildren);
		InternalDestroy(rEntry.m_pvDirChildren);
		return;
	}

	// Assemble a set
	SetType set;
	for (auto const& s : vsResults)
	{
		auto const filePath = FilePath::CreateContentFilePath(s);
		if (filePath.IsValid())
		{
			SEOUL_VERIFY(set.Insert(filePath).Second);
		}
	}

	// Reduce the set to those entries that need to be added - this will
	// also remove any entries that no longer exist.
	InternalPrune(set, rEntry.m_pvDirChildren);
	InternalPrune(set, rEntry.m_pvFileChildren);

	// Now add new entries.
	{
		if (nullptr == rEntry.m_pvDirChildren)
		{
			rEntry.m_pvDirChildren = SEOUL_NEW(MemoryBudgets::Editor) Entries;
		}

		if (nullptr == rEntry.m_pvFileChildren)
		{
			rEntry.m_pvFileChildren = SEOUL_NEW(MemoryBudgets::Editor) Entries;
		}

		auto const iBegin = set.Begin();
		auto const iEnd = set.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			auto const bDirectory = Directory::DirectoryExists(i->GetAbsoluteFilenameInSource());
			auto pv = (bDirectory ? rEntry.m_pvDirChildren : rEntry.m_pvFileChildren);

			Entry* pEntry = SEOUL_NEW(MemoryBudgets::Editor) Entry;
			pEntry->m_pParent = &rEntry;
			pEntry->m_bIsDirectory = bDirectory;
			pEntry->m_FilePath = *i;
			pEntry->m_sName = Path::GetFileName(String(i->GetRelativeFilenameInSource()));
			SEOUL_VERIFY(m_tLookup.Insert(pEntry->m_FilePath, pEntry).Second);
			pv->PushBack(pEntry);
		}
	}

	// Finally, sort the entries lexographically.
	Sorter sorter;
	QuickSort(rEntry.m_pvDirChildren->Begin(), rEntry.m_pvDirChildren->End(), sorter);
	QuickSort(rEntry.m_pvFileChildren->Begin(), rEntry.m_pvFileChildren->End(), sorter);
}

static void OpenExternal(FilePath filePath)
{
	auto const sURL("file:///" +
		filePath.GetAbsoluteFilenameInSource().ReplaceAll("\\", "/"));
	Engine::Get()->OpenURL(sURL);
}

void ViewFileBrowser::InternalOpen(FilePath filePath)
{
	switch (filePath.GetType())
	{
		// Internal type.
	case FileType::kScenePrefab: Root::Get()->OpenScenePrefab(filePath); return;
	default:
		break;
	}

	// External open.
	Jobs::AsyncFunction(&OpenExternal, filePath);
}

void ViewFileBrowser::InternalPrePoseDirTree(Entries& rvEntries)
{
	using namespace ImGui;

	auto const& icons = Root::Get()->GetIcons();
	auto const closed = Root::Get()->GetRenderer().ResolveTexture(icons.m_FolderClosed);
	auto const open = Root::Get()->GetRenderer().ResolveTexture(icons.m_FolderOpen);

	for (auto p : rvEntries)
	{
		// Make sure we have info on this directories items.
		if (nullptr == p->m_pvDirChildren)
		{
			InternalExpand(*p);
		}

		// Compute selection state and draw settings.
		auto bSelected = m_SelectedDir.HasKey(p);
		ImGuiTreeNodeFlags eFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
		eFlags |= (p->m_bIsDirectory && !p->m_pvDirChildren->IsEmpty() ? ImGuiTreeNodeFlags_OpenOnDoubleClick : ImGuiTreeNodeFlags_Leaf);
		eFlags |= (bSelected ? ImGuiTreeNodeFlags_Selected : 0);
		eFlags |= ((eFlags & ImGuiTreeNodeFlags_Leaf) ? ImGuiTreeNodeFlags_Bullet : 0);

		// Expand if needed.
		if (p->m_bPendingExpand)
		{
			if (!IsTreeNodeOpen(p->m_sName.CStr(), eFlags))
			{
				SetNextItemOpen(true);
			}
			p->m_bPendingExpand = false;
		}

		// Draw the tree node - return true means it's been expanded.
		Bool bDoSelect = false;
		if (TreeNodeImage(closed, open, p->m_sName.CStr(), eFlags))
		{
			bDoSelect = IsItemClicked();

			InternalPrePoseDirTree(*p->m_pvDirChildren);

			TreePop();
		}
		else
		{
			bDoSelect = IsItemClicked();
		}

		if (bDoSelect)
		{
			m_SelectedDir.Clear();
			m_SelectedDir.Insert(p);
			bSelected = true;
		}
	}
}

Bool ViewFileBrowser::InternalPrePoseFileEntry(
	Entry* pEntry,
	Entry*& rpDirChange)
{
	using namespace ImGui;

	static const Float kfSize = 44.0f; // TODO:

	auto const& icons = Root::Get()->GetIcons();
	auto const entryFilePath = pEntry->m_FilePath;
	auto const bSelected = m_SelectedFile.HasKey(pEntry);
	FilePath iconFilePath;

	if (IsTextureFileType(entryFilePath.GetType()))
	{
		iconFilePath = entryFilePath;
	}
	else if (pEntry->m_bIsDirectory)
	{
		iconFilePath = icons.m_FolderClosed;
	}
	else if (FileType::kUnknown != entryFilePath.GetType())
	{
		switch (entryFilePath.GetType())
		{
		case FileType::kCs:
			iconFilePath = icons.m_CSharp;
			break;
		case FileType::kEffect: // fall-through
		case FileType::kEffectHeader:
			iconFilePath = icons.m_Brush;
			break;
		case FileType::kFont:
			iconFilePath = icons.m_Font;
			break;
		case FileType::kFxBank:
			iconFilePath = icons.m_Fire;
			break;
		case FileType::kSceneAsset:
			iconFilePath = icons.m_Mesh;
			break;
		case FileType::kScenePrefab:
			iconFilePath = icons.m_Prefab;
			break;
		case FileType::kScript:
			iconFilePath = icons.m_Lua;
			break;
		case FileType::kSoundProject:
			iconFilePath = icons.m_Audio;
			break;
		case FileType::kJson: // fall-through
		case FileType::kText:
			iconFilePath = icons.m_DocumentText;
			break;
		case FileType::kUIMovie:
			iconFilePath = icons.m_Flash;
			break;
		default:
			iconFilePath = icons.m_Document;
			break;
		};
	}
	else
	{
		// Don't include this file entry, type not used by the engine or
		// editor.
		return false;
	}

	auto pData = Root::Get()->GetRenderer().ResolveTexture(iconFilePath);

	// Wrap when out of horizontal width.
	if (GetContentRegionAvail().x < kfSize)
	{
		NewLine();
	}

	auto const eAction = ImageButtonWithLabel(
		(ImTextureID)pData,
		ImVec2(kfSize, kfSize),
		pEntry->m_sName.CStr(),
		bSelected);
	SetTooltipEx(pEntry->m_sName.CStr());

	Bool bReturn = false;

	// Drag and drop handling - start dragging.
	if (ImageButtonAction::kDragging == eAction)
	{
		Root::Get()->StartDragging(
			iconFilePath,
			DragSourceFilePath{ pEntry->m_FilePath });
	}
	// Selection - make this the active file.
	else if (ImageButtonAction::kSelected == eAction)
	{
		m_SelectedFile.Clear();
		m_SelectedFile.Insert(pEntry);
	}
	// Double click - open directory or open file
	// in external utility.
	else if (ImageButtonAction::kDoubleClicked == eAction)
	{
		if (pEntry->m_bIsDirectory)
		{
			rpDirChange = pEntry;
			bReturn = true;
		}
		else
		{
			InternalOpen(pEntry->m_FilePath);
		}
	}
	SameLine();
	return bReturn;
}

static inline void SetPendingExpand(ViewFileBrowser::Entry& rEntry)
{
	rEntry.m_bPendingExpand = true;
	if (nullptr != rEntry.m_pParent)
	{
		SetPendingExpand(*rEntry.m_pParent);
	}
}

void ViewFileBrowser::InternalPrePoseFileView()
{
	Entry* pDirChange = nullptr;
	for (auto pRootEntry : m_SelectedDir)
	{
		for (auto pEntry : *pRootEntry->m_pvDirChildren)
		{
			if (InternalPrePoseFileEntry(pEntry, pDirChange))
			{
				SetPendingExpand(*pEntry);
			}
		}

		for (auto pEntry : *pRootEntry->m_pvFileChildren)
		{
			InternalPrePoseFileEntry(pEntry, pDirChange);
		}
	}

	// Apply the directory change now.
	if (nullptr != pDirChange)
	{
		// Make sure we have info on this directories items.
		if (nullptr == pDirChange->m_pvDirChildren)
		{
			InternalExpand(*pDirChange);
		}

		m_SelectedDir.Clear();
		m_SelectedDir.Insert(pDirChange);
	}
}

void ViewFileBrowser::InternalPrune(SetType& set, Entries* pvEntries)
{
	// Early out.
	if (nullptr == pvEntries)
	{
		return;
	}

	// Process.
	for (auto i = pvEntries->Begin(); pvEntries->End() != i; )
	{
		// Remove entries not in the set.
		if (!set.HasKey((*i)->m_FilePath))
		{
			InternalDestroy(*i);
			i = pvEntries->Erase(i);
		}
		// Erase entries already present from the set.
		else
		{
			auto const filePath = (*i)->m_FilePath;
			SEOUL_VERIFY(set.Erase(filePath));
			++i;
		}
	}
}

void ViewFileBrowser::InternalRefresh(const String& sPath)
{
	auto const filePath = FilePath::CreateContentFilePath(sPath);

	Entry* p = nullptr;
	if (m_tLookup.GetValue(filePath, p))
	{
		InternalExpand(*p);
	}
}

} // namespace Seoul::EditorUI
