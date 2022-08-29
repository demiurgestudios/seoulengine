/**
 * \file EditorUIRoot.h
 * \brief Specialization of DevUIRoot for Seoul Editor.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_ROOT_H
#define EDITOR_UI_ROOT_H

#include "DevUIRoot.h"
#include "EditorUISettings.h"
#include "ReflectionAny.h"
namespace Seoul { namespace DevUI { class Controller; } }
namespace Seoul { namespace EditorUI { struct Icons; } }
namespace Seoul { namespace EditorUI { class LogBuffer; } }
namespace Seoul { namespace EditorUI { class RecentDocuments; } }

namespace Seoul::EditorUI
{

// TODO: Unfortunate way to handle popups.
enum class PendingClose
{
	kNone,
	kCloseCurrent,
	kExit,
	kOpeningCloseCurrent,
	kOpeningExit,
};

class Root SEOUL_SEALED : public DevUI::Root
{
public:
	/**
	 * @return The global singleton instance. Will be nullptr if that instance has not
	 * yet been created.
	 */
	static CheckedPtr<Root> Get()
	{
		if (DevUI::Root::Get() && DevUI::Root::Get()->GetType() == DevUI::Type::Editor)
		{
			return (Root*)DevUI::Root::Get().Get();
		}
		else
		{
			return CheckedPtr<Root>();
		}
	}

	Root(const Settings& settings);
	~Root();

	// DevUIRoot overrides
	virtual void DisplayNotification(const String& sMessage) SEOUL_OVERRIDE
	{
		// TODO:
	}
	virtual void DisplayTrackedNotification(const String& sMessage, Int32& riId) SEOUL_OVERRIDE
	{
		// TODO:
	}
	virtual void KillNotification(Int32 iId) SEOUL_OVERRIDE
	{
		// TODO:
	}
	// /DevUIRoot overrides

	const Icons& GetIcons() const
	{
		return *m_pIcons;
	}

	void OpenScenePrefab(FilePath filePath);

	/** Utility structure used to capture a drag-and-drop item. */
	struct DragData SEOUL_SEALED
	{
		Reflection::Any m_Data{};
		FilePath m_IconFilePath{};
		Bool m_bActive{};
		Bool m_bCanDrop{};
		Bool m_bCancelled{};

		/** Clear the drag-and-drop data back to its defaults. */
		void Reset()
		{
			*this = DragData{};
		}
	};

	// Drag and drop handling.
	const DragData& GetDragData() const { return m_DragData; }
	Bool IsItemDragAndDropTarget() const;
	Bool IsWindowDragAndDropTarget() const;
	void MarkCanDrop() { m_DragData.m_bCanDrop = true; }
	void StartDragging(FilePath iconFilePath, const Reflection::Any& data);

private:
	// DevUIRoot overrides.
	virtual void InternalDoTickBegin(RenderPass& rPass, Float fDeltaTimeInSeconds, IPoseable* pParent) SEOUL_OVERRIDE;
	virtual void InternalPrePoseImGuiFrameEnd(RenderPass& rPass, Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;
	virtual void InternalDrawMenuBar(Bool bRootMainMenu) SEOUL_OVERRIDE
	{
		if (InternalBeginMainMenuPrePose(bRootMainMenu))
		{
			InternalEndMainMenuPrePose(bRootMainMenu);
		}
	}
	virtual Bool InternalDrawStatusBar(Bool bRootStatusBar) SEOUL_OVERRIDE;
	// /DevUIRoot overrides.

	Settings const m_Settings;
	DragData m_DragData;
	Vector<FilePath, MemoryBudgets::Editor> m_vPendingOpen;
	ScopedPtr<LogBuffer> m_pLogBuffer;
	ScopedPtr<Icons> m_pIcons;
	ScopedPtr<RecentDocuments> m_pRecentDocuments;
	typedef Vector<SharedPtr<DevUI::Controller>, MemoryBudgets::Editor> Controllers;
	Controllers m_vControllers;
	// TODO: Break these out.
	typedef Vector<Bool, MemoryBudgets::Editor> ExitSaveState;
	ExitSaveState m_vbExitSaveState;
	PendingClose m_ePendingClose;
	// /TODO:
	enum MainFormCleanupAction
	{
		kMfcaNone,
		kMfcaClose,
		kMfcaReload,
	};
	MainFormCleanupAction m_eMainFormCleanupAction;

	Bool AnyMainFormNeedsSave() const;

	void InternalDoSave(DevUI::Controller& rController);
	void InternalDoSaveAs(DevUI::Controller& rController, const String& sFilename);

	Bool InternalAddMainForm(FilePath filePath);
	void InternalDeleteMainForm(DevUI::MainForm* pMainForm);
	void InternalHandleDragAndDrop();
	Bool InternalBeginMainMenuPrePose(Bool bRootMainMenu);
	void InternalEndMainMenuPrePose(Bool bRootMainMenu);
	void InternalPrePoseLeftMenus();
	void InternalPrePoseRightMenus();
	void InternalOutOfDateMainFormCheck();
	void InternalProcessPending();
	void InternalPruneControllers();

	// Main editor handlers.
	void OnClose();
	void OnExit();
	void OnNew();
	void OnOpen();
	void OnSave();
	void OnSaveAs();
	void OnSaveAs(const String& sFileName);
	// /Main editor handlers.

	SEOUL_DISABLE_COPY(Root);
}; // class Root

} // namespace Seoul::EditorUI

#endif // include guard
