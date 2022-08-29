/**
 * \file EditorUIRoot.cpp
 * \brief Specialization of DevUIRoot for Seoul Editor.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "CookManager.h"
#include "DevUIController.h"
#include "DevUIImGui.h"
#include "DevUIImGuiRenderer.h"
#include "DevUIMainForm.h"
#include "EditorUIControllerScene.h"
#include "EditorUIIcons.h"
#include "EditorUILogBuffer.h"
#include "EditorUIMainFormScene.h"
#include "EditorUIRecentDocuments.h"
#include "EditorUIRoot.h"
#include "Engine.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "InputManager.h"
#include "Renderer.h"
#include "RenderPass.h"
#include "ScopedAction.h"

namespace Seoul::EditorUI
{

// TODO: Implement in a way that is specific to classes of root main forms.
static Byte const* ksDefaultSettings = R"(
[Window][Debug##Default]
Pos=60,60
Size=400,400
Collapsed=0

[Window][Inspector]
Pos=1610,52
Size=302,514
Collapsed=0
DockId=0x00000009,0

[Window][Scene]
Pos=238,52
Size=1370,514
Collapsed=0
DockId=0x00000003,0

[Window][History]
Pos=1610,568
Size=302,464
Collapsed=0
DockId=0x00000006,0

[Window][Files]
Pos=238,568
Size=1370,464
Collapsed=0
DockId=0x00000005,0

[Window][Log]
Pos=238,568
Size=1370,464
Collapsed=0
DockId=0x00000005,1

[Window][Objects]
Pos=8,52
Size=228,980
Collapsed=0
DockId=0x00000001,0
0xFa718776
[Docking][Data]
DockSpace       ID=0xFA718776 Window=0xFA718776 Pos=8,52 Size=1904,980 Split=X
  DockNode      ID=0x00000001 Parent=0xFA718776 SizeRef=217,782 Selected=0x7DA7F56F
  DockNode      ID=0x00000004 Parent=0xFA718776 SizeRef=1596,782 Split=Y
    DockNode    ID=0x00000002 Parent=0x00000004 SizeRef=1815,410 Split=X Selected=0x18B8C0DE
      DockNode  ID=0x00000003 Parent=0x00000002 SizeRef=1370,345 Selected=0x18B8C0DE
      DockNode  ID=0x00000009 Parent=0x00000002 SizeRef=302,345 Selected=0xF02CD328
    DockNode    ID=0x00000008 Parent=0x00000004 SizeRef=1815,370 Split=X
      DockNode  ID=0x00000005 Parent=0x00000008 SizeRef=1511,782 CentralNode=1 Selected=0xC7F46F5D
      DockNode  ID=0x00000006 Parent=0x00000008 SizeRef=302,782 Selected=0xE80749D7
)";

// TODO: Should bolt Index to new main forms
// so that numbering is consistent in all cases.

/**
 * Convenience utility for generating a human
 * readable name of a main form.
 */
static String GetName(CheckedPtr<DevUI::MainForm> pMainForm, Int* piIndex = nullptr)
{
	auto const filePath = pMainForm->GetController().GetSaveFilePath();
	String sReturn(filePath.IsValid() ? filePath.GetRelativeFilenameInSource() : "New");
	if (!filePath.IsValid() && nullptr != piIndex)
	{
		++(*piIndex);
		sReturn.Append(String::Printf(" %d", *piIndex));
	}
	if (pMainForm->GetController().NeedsSave())
	{
		sReturn.Append("*");
	}
	return sReturn;
}

/**
 * @return A root poseable that can be used to pose and render the Editor UI.
 */
static IPoseable* SpawnRoot(
	const DataStoreTableUtil& /*configSettings*/,
	Bool& rbRenderPassOwnsPoseableObject)
{
	rbRenderPassOwnsPoseableObject = false;

	return Root::Get();
}

/** \brief HString constant used to uniquely identify the root poseable. */
static const HString kPoseableSpawnType("EditorUI");

Root::Root(const Settings& settings)
	: DevUI::Root(DevUI::Type::Editor, nullptr)
	, m_Settings(settings)
	, m_vPendingOpen()
#if SEOUL_LOGGING_ENABLED
	, m_pLogBuffer(SEOUL_NEW(MemoryBudgets::Editor) LogBuffer)
#endif // /#if SEOUL_LOGGING_ENABLED
	, m_pIcons(SEOUL_NEW(MemoryBudgets::Editor) Icons)
	, m_pRecentDocuments(SEOUL_NEW(MemoryBudgets::Editor) RecentDocuments(GameDirectory::kContent, FileType::kScenePrefab))
	, m_vControllers()
	, m_vbExitSaveState()
	, m_ePendingClose(PendingClose::kNone)
	, m_eMainFormCleanupAction(kMfcaNone)
{
	SEOUL_ASSERT(IsMainThread());

	// TODO: Apply in base?
	ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
	// TODO: Temporary, eliminate.
	ImGui::LoadIniSettingsFromMemory(ksDefaultSettings, strlen(ksDefaultSettings));

	// Register the root poseable hook for rendering UI screens.
	RenderPass::RegisterPoseableSpawnDelegate(
		kPoseableSpawnType,
		SpawnRoot);
}

Root::~Root()
{
	SEOUL_ASSERT(IsMainThread());

	// Unregister handling of the Root root poseable.
	RenderPass::UnregisterPoseableSpawnDelegate(
		kPoseableSpawnType);

	// Cleanup controllers.
	m_vControllers.Clear();
	// Cleanup recent documents.
	m_pRecentDocuments.Reset();
}

void Root::OpenScenePrefab(FilePath filePath)
{
	m_vPendingOpen.PushBack(filePath);
}

/**
 * @return true if an item is being dragged
 * and the current item is a potential
 * target (based on item rectangle).
 *
 * Note: the item should check for mouse
 * down to decide between a drop event
 * vs. a drag over.
 */
Bool Root::IsItemDragAndDropTarget() const
{
	using namespace ImGui;

	// Early out if not dragging or if cancelled.
	if (!m_DragData.m_bActive || m_DragData.m_bCancelled)
	{
		return false;
	}

	// Mouse released, finish the drag and drop.
	if (IsItemHovered(ImGuiHoveredFlags_RectOnly))
	{
		return true;
	}

	return false;
}

/**
* @return true if an item is being dragged
* and the current window is a potential
* target (based on window rectangle).
*
* Note: the item should check for mouse
* down to decide between a drop event
* vs. a drag over.
*/
Bool Root::IsWindowDragAndDropTarget() const
{
	using namespace ImGui;

	// Early out if not dragging or if cancelled.
	if (!m_DragData.m_bActive || m_DragData.m_bCancelled)
	{
		return false;
	}

	// Mouse released, finish the drag and drop.
	if (IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
	{
		return true;
	}

	return false;
}

/**
 * Begin a drag and drop operation - nop if
 * a drag is already active.
 */
void Root::StartDragging(
	FilePath iconFilePath,
	const Reflection::Any& data)
{
	// Early out if we already have a drag target.
	if (m_DragData.m_bActive)
	{
		return;
	}

	m_DragData.m_bCanDrop = false;
	m_DragData.m_bActive = true;
	m_DragData.m_Data = data;
	m_DragData.m_IconFilePath = iconFilePath;
}

Bool Root::AnyMainFormNeedsSave() const
{
	for (auto const& pMainForm : m_vMainForms)
	{
		if (pMainForm->GetController().NeedsSave())
		{
			return true;
		}
	}

	return false;
}

void Root::InternalDoSave(DevUI::Controller& rController)
{
	if (!rController.NeedsSave())
	{
		return;
	}

	// If we need to save but haven't yet associated a filepath with
	// the active main form, perform a SaveAs().
	if (!rController.HasSaveFilePath())
	{
		OnSaveAs();
		return;
	}

	// Otherwise, save.
	if (!rController.Save())
	{
		// TODO: Report error information with the failure.
		// TODO: Report error information.
	}
}

void Root::InternalDoSaveAs(DevUI::Controller& rController, const String& sFilename)
{
	FilePath filePath;
	if (sFilename.IsEmpty())
	{
		if (!Engine::Get()->DisplayFileDialogSingleSelection(
			filePath,
			Engine::FileDialogOp::kSave,
			FileType::kScenePrefab, // TODO:
			GameDirectory::kContent))
		{
			return;
		}
	}
	else
	{
		filePath = FilePath::CreateContentFilePath(sFilename);
	}

	if (!filePath.IsValid())
	{
		// TODO: Report.
		return;
	}

	// Set the save file path.
	rController.SetSaveFilePath(filePath);

	/// Save.
	if (!rController.Save())
	{
		// TODO: Report error information with the failure.
		// TODO: Report error information.
	}
}

Bool Root::InternalAddMainForm(FilePath filePath)
{
	// Special handling if this is an open.
	if (filePath.IsValid())
	{
		// Make sure the file exists.
		if (!FileManager::Get()->Exists(filePath.GetAbsoluteFilenameInSource()))
		{
			// TODO: Warn.
			return false;
		}

		// Check for already open.
		for (auto pMainForm : m_vMainForms)
		{
			if (pMainForm->GetController().GetSaveFilePath() == filePath)
			{
				// Found already, just switch to it.
				m_pActiveMainForm = pMainForm;
				return true;
			}
		}
	}

	// Handle an actual create.
	switch (filePath.GetType())
	{
	case FileType::kScenePrefab: // fall-through
	default: // TODO:
#if SEOUL_WITH_SCENE
		{
			SharedPtr<ControllerScene> pController(SEOUL_NEW(MemoryBudgets::Editor) ControllerScene(
				m_Settings, filePath));
			m_vControllers.PushBack(pController);

			CheckedPtr<MainFormScene> pMainForm(SEOUL_NEW(MemoryBudgets::Editor) MainFormScene(pController));
			m_vMainForms.PushBack(pMainForm);

			m_pActiveMainForm = pMainForm;
		}
#endif // /#if SEOUL_WITH_SCENE
		break;
	};

	return m_pActiveMainForm.IsValid();
}

void Root::InternalDeleteMainForm(DevUI::MainForm* pMainForm)
{
	auto i = m_vMainForms.Find(pMainForm);
	if (m_vMainForms.End() != i)
	{
		m_vMainForms.Erase(i);
	}

	if (m_pActiveMainForm == pMainForm)
	{
		m_pActiveMainForm.Reset();
		if (!m_vMainForms.IsEmpty())
		{
			m_pActiveMainForm = m_vMainForms.Back();
		}
	}

	SafeDelete(pMainForm);
}

/**
 * Management of drag and drop.
 */
void Root::InternalHandleDragAndDrop()
{
	using namespace ImGui;

	// Early out if not dragging.
	if (!m_DragData.m_bActive)
	{
		return;
	}

	// Escape pressed, cancel the drag and drop.
	if (IsShortcutPressed("Escape"))
	{
		m_DragData.m_bCancelled = true;
	}

	// Mouse released, finish the drag and drop.
	if (IsMouseReleased(0))
	{
		m_DragData.Reset();
		return;
	}

	// No further processing if cancelled.
	if (m_DragData.m_bCancelled)
	{
		return;
	}

	// Drag the drag info. Setup flags for
	// a full screen draw area.
	auto const eFlags = (
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_Tooltip);

	// Begin the draw area.
	SetNextWindowBgAlpha(0.0f);
	Begin("##DragAndDrop", nullptr, eFlags);
	auto pDrawList = GetWindowDrawList();
	pDrawList->PushClipRectFullScreen();

	// Get positiona and color of the drag icon.
	auto const& style = GetStyle();
	auto const size = ImVec2(GetFontSize() * 2.0f, GetFontSize() * 2.0f);
	auto const pos = GetMousePos() - ImVec2(size.x * 0.5f, size.y * 0.75f);
	auto const tint = (m_DragData.m_bCanDrop ? ImColor(0.0f, 1.0f, 0.0f, 0.8f) : ImColor(1.0f, 0.0f, 0.0f, 0.8f)); // TODO: Don't hard code/account for color scheme.

	// Resolve the drag icon.
	auto pTexture = GetRenderer().ResolveTexture(m_DragData.m_IconFilePath);

	// Drag icon.
	pDrawList->AddImage(pTexture, pos, pos + size, ImVec2(0, 0), ImVec2(1, 1), tint);

	// Finish up the draw area.
	pDrawList->PopClipRect();
	End();

	// Done for this frame - if we're still hovering
	// a valid drag target, it is expected to
	// set this to true again next frame.
	m_DragData.m_bCanDrop = false;
}

Bool Root::InternalBeginMainMenuPrePose(Bool bRootMainMenu)
{
	auto fOriginal = ImGui::GetStyle().Alpha;
	ImGui::GetStyle().Alpha = 0.5f;
	Bool bBegin = false;
	if (bRootMainMenu)
	{
		bBegin = ImGui::BeginMainMenuBar();
	}
	else
	{
		bBegin = ImGui::BeginMenuBar();
	}
	ImGui::GetStyle().Alpha = fOriginal;

	if (bBegin)
	{
		// Upper left corner logo if not a root (os window) main menu.
		if (!bRootMainMenu)
		{
			auto const logo = GetRenderer().ResolveTexture(m_pIcons->m_Logo);
			ImGui::MenuBarImage(logo, ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize()));
		}

		InternalPrePoseLeftMenus();
		return true;
	}

	return false;
}

void Root::InternalEndMainMenuPrePose(Bool bRootMainMenu)
{
	if (m_pActiveMainForm)
	{
		m_pActiveMainForm->PrePoseMainMenu();
	}
	InternalPrePoseRightMenus();
	InternalMainMenuAsTitleBarControls(bRootMainMenu);
	if (bRootMainMenu)
	{
		ImGui::EndMainMenuBar();
	}
	else
	{
		ImGui::EndMenuBar();
	}
}

void Root::InternalPrePoseLeftMenus()
{
	using namespace ImGui;

	Bool const bVisible = BeginMenu("&File");

	// Rules:
	// - New is always available.
	// - Open is always available.
	// - Close is only available if m_pActiveMainForm is valid.
	// - Save is only valid if m_pActiveMainForm is valid and its controller needs saving.
	// - SaveAs is only available if m_pActiveMainForm is valid.
	Bool const bCanClose = (m_pActiveMainForm.IsValid());
	Bool const bCanSave = (
		m_pActiveMainForm.IsValid() &&
		m_pActiveMainForm->GetController().NeedsSave());
	Bool const bCanSaveAs = (m_pActiveMainForm.IsValid());

	if (MenuItemEx(bVisible, "&New", "Ctrl+N")) { OnNew(); }
	if (MenuItemEx(bVisible, "&Open", "Ctrl+O")) { OnOpen(); }
	SeparatorEx(bVisible);
	if (MenuItemEx(bVisible, "&Close", nullptr, false, bCanClose)) { OnClose(); }
	SeparatorEx(bVisible);
	if (MenuItemEx(bVisible, "&Save", "Ctrl+S", false, bCanSave)) { OnSave(); }
	if (MenuItemEx(bVisible, "Save &As", nullptr, false, bCanSaveAs)) { OnSaveAs(); }
	SeparatorEx(bVisible);
	if (bVisible)
	{
		RecentDocumentsLock lock(*m_pRecentDocuments);
		auto const& vDoc = lock.GetDocuments();
		if (BeginMenu("Recent &Files", !vDoc.IsEmpty()))
		{
			for (auto const& filePath : vDoc)
			{
				if (MenuItem(filePath.GetAbsoluteFilenameInSource().CStr())) { InternalAddMainForm(filePath); }
			}
			EndMenu();
		}
	}
	SeparatorEx(bVisible);

	// TODO: Need to catch all exit events
	// and ask for saving prior to doing so.
	if (MenuItemEx(bVisible, "E&xit", "Alt+F4")) { OnExit(); }

	if (bVisible)
	{
		EndMenu();
	}
}

void Root::InternalPrePoseRightMenus()
{
	using namespace ImGui;

	if (BeginMenu("&Window"))
	{
		Int32 iIndex = 0;
		for (auto const& p : m_vMainForms)
		{
			String sName(GetName(p, &iIndex));
			if (MenuItem(sName.CStr(), nullptr, (m_pActiveMainForm == p)))
			{
				m_pActiveMainForm = p;
			}
		}

		// TODO: Separator();
		// TODO:
		// TODO: if (MenuItem("&Reset Window Layout", nullptr, false, !m_pSave->IsDefaultLayout())) { m_pSave->ApplyDefault(); }

		// Give the active main form a chance to add entries to the Windows menu.
		if (m_pActiveMainForm)
		{
			m_pActiveMainForm->PrePoseWindowsMenu();
		}

		EndMenu();
	}

	Bool bShowAboutSeoulEditor = false;
	if (BeginMenu("&Help"))
	{
		if (MenuItem("&About Seoul Editor"))
		{
			bShowAboutSeoulEditor = true;
		}

		EndMenu();
	}

	if (bShowAboutSeoulEditor)
	{
		OpenPopup("About Seoul Editor");
	}
	if (BeginPopupModalEx("About Seoul Editor", GetWindowCenter(), nullptr, ImGuiWindowFlags_NoResize))
	{
		Text("Seoul Editor\n\nCopyright (C) Demiurge Studios 2017-2022.");
		if (Button("OK", ImVec2(120, 0)))
		{
			CloseCurrentPopup();
		}

		EndPopup();
	}

	// TODO: Unfortunate way to handle modal popups.
	// Handle close popups.
	switch (m_ePendingClose)
	{
	case PendingClose::kOpeningCloseCurrent:
		OpenPopup("Save?##OnClose");
		m_ePendingClose = PendingClose::kCloseCurrent;
		break;
	case PendingClose::kOpeningExit:
		OpenPopup("Save?##OnExit");
		m_ePendingClose = PendingClose::kExit;
		m_vbExitSaveState.Clear();
		break;
	default:
		break;
	};

	switch (m_ePendingClose)
	{
	case PendingClose::kCloseCurrent:
		if (BeginPopupModalEx("Save?##OnClose", GetWindowCenter(), nullptr, ImGuiWindowFlags_NoResize))
		{
			Text("Save changes?");
			if (Button("Yes"))
			{
				OnSave();
				m_ePendingClose = PendingClose::kNone;
				CloseCurrentPopup();
				m_eMainFormCleanupAction = kMfcaClose;
			}
			SameLine();
			if (Button("No"))
			{
				m_ePendingClose = PendingClose::kNone;
				CloseCurrentPopup();
				m_eMainFormCleanupAction = kMfcaClose;
			}
			SameLine();
			if (Button("Cancel"))
			{
				m_ePendingClose = PendingClose::kNone;
				CloseCurrentPopup();
			}

			EndPopup();
		}
		break;
	case PendingClose::kExit:
		if (BeginPopupModalEx("Save?##OnExit", GetWindowCenter(), nullptr, ImGuiWindowFlags_NoResize))
		{
			UInt32 const uSize = m_vControllers.GetSize();
			m_vbExitSaveState.Resize(uSize, true);
			for (UInt32 i = 0u; i < uSize; ++i)
			{
				if (!m_vControllers[i]->NeedsSave())
				{
					continue;
				}

				FilePath const filePath(m_vControllers[i]->GetSaveFilePath());
				if (filePath.IsValid())
				{
					ImGui::Checkbox(
						filePath.GetAbsoluteFilenameInSource().CStr(),
						m_vbExitSaveState.Get(i));
				}
				else
				{
					ImGui::Checkbox(
						"Unname",
						m_vbExitSaveState.Get(i));
				}
			}

			if (Button("Save Selected"))
			{
				for (UInt32 i = 0u; i < m_vControllers.GetSize(); ++i)
				{
					if (!m_vControllers[i]->NeedsSave())
					{
						continue;
					}

					if (!m_vbExitSaveState[i])
					{
						continue;
					}

					if (m_vControllers[i]->HasSaveFilePath())
					{
						InternalDoSave(*m_vControllers[i]);
					}
					else
					{
						InternalDoSaveAs(*m_vControllers[i], String());
					}
				}

				m_ePendingClose = PendingClose::kNone;
				CloseCurrentPopup();

				(void)Engine::Get()->PostNativeQuitMessage();
			}
			SameLine();
			if (Button("Don't Save"))
			{
				m_ePendingClose = PendingClose::kNone;
				CloseCurrentPopup();

				(void)Engine::Get()->PostNativeQuitMessage();
			}
			SameLine();
			if (Button("Cancel"))
			{
				m_ePendingClose = PendingClose::kNone;
				CloseCurrentPopup();
			}

			EndPopup();
		}
		break;
	default:
		// Nop
		break;
	};
}

/**
 * Utility that handles checking if the active main form is out-of-date
 * (the file has changed on disk). If so, prompts the user to take
 * a corrective action.
 */
void Root::InternalOutOfDateMainFormCheck()
{
	using namespace ImGui;

	// Nothing to check if no main form.
	if (!m_pActiveMainForm.IsValid())
	{
		return;
	}

	// Check if the popup is already showing.
	if (BeginPopupModalEx("File out of date", GetWindowCenter(), nullptr, ImGuiWindowFlags_NoResize))
	{
		// Already open, so handle displaying
		Text("%s", m_pActiveMainForm->GetController().GetSaveFilePath().GetAbsoluteFilenameInSource().CStr());

		// Different message depending on whether the controller has changes or not.
		if (m_pActiveMainForm->GetController().NeedsSave())
		{
			Text("The file has unsaved changes inside the editor and has been changed externally.\nDo you want to reload it and lose the changes made inside the editor?");
		}
		else
		{
			Text("The file has been changed externally, and has no unsaved changes inside the editor.\nDo you want to reload it?");
		}

		// Hit "Yes", we want to reload the main form.
		if (Button("Yes"))
		{
			m_eMainFormCleanupAction = kMfcaReload;
			CloseCurrentPopup();
		}

		SameLine();

		// Hit "No", just mark the main form up-to-date.
		if (Button("No"))
		{
			m_pActiveMainForm->GetController().MarkUpToDate();
			CloseCurrentPopup();
		}

		EndPopup();
		return;
	}

	// Not out of date, nothing to do.
	if (!m_pActiveMainForm->GetController().IsOutOfDate())
	{
		return;
	}

	// Open the "File out of date" popup.
	OpenPopup("File out of date");
}

void Root::InternalDoTickBegin(RenderPass& rPass, Float fDeltaTimeInSeconds, IPoseable* pParent)
{
	SEOUL_ASSERT(IsMainThread());

	InternalPruneControllers();
	InternalProcessPending();

	// Cleanup active main form if requested.
	if (kMfcaNone != m_eMainFormCleanupAction)
	{
		if (m_pActiveMainForm.IsValid())
		{
			FilePath const filePath = m_pActiveMainForm->GetController().GetSaveFilePath();
			InternalDeleteMainForm(m_pActiveMainForm);

			// One more step if this was a reload, not a close.
			if (kMfcaReload == m_eMainFormCleanupAction)
			{
				InternalAddMainForm(filePath);
			}
		}
		m_eMainFormCleanupAction = kMfcaNone;
	}

	// Make sure we always have an active main form.
	if (!m_pActiveMainForm.IsValid())
	{
		if (m_vMainForms.IsEmpty())
		{
			if (InternalAddMainForm(FilePath()))
			{
				m_pActiveMainForm = m_vMainForms.Front();
			}
		}
	}
}

void Root::InternalPrePoseImGuiFrameEnd(RenderPass& rPass, Float fDeltaTimeInSeconds)
{
	// Drag and drop handling.
	InternalHandleDragAndDrop();

	// Now check if the active main form is out of date,
	// to display appropriate messaging.
	InternalOutOfDateMainFormCheck();
}

Bool Root::InternalDrawStatusBar(Bool bRootStatusBar)
{
	using namespace ImGui;

	if (BeginStatusBar())
	{
		auto const contentLoads = Content::LoaderBase::GetActiveLoaderCount();
		if (contentLoads > 0)
		{
			Text("Loading (%d)...", (Int)contentLoads);

			if (CookManager::Get()->GetCurrent().IsValid())
			{
				Text("(cooking on the fly...)");
			}
		}
		else if (CookManager::Get()->GetCurrent().IsValid())
		{
			Text("Cooking on the fly...");
		}

		EndStatusBar();
		return true;
	}

	return false;
}

void Root::InternalProcessPending()
{
	// Handle open requests.
	for (auto const& filePath : m_vPendingOpen)
	{
		InternalAddMainForm(filePath);
	}
	m_vPendingOpen.Clear();
}

void Root::InternalPruneControllers()
{
	Int32 iCount = (Int32)m_vControllers.GetSize();
	for (Int32 i = 0; i < iCount; ++i)
	{
		if (m_vControllers[i].IsUnique())
		{
			Swap(m_vControllers[i], m_vControllers[iCount - 1]);
			--i;
			--iCount;
		}
	}

	SEOUL_ASSERT(iCount >= 0);
	m_vControllers.Resize((UInt32)iCount);
}

void Root::OnClose()
{
	if (m_pActiveMainForm.IsValid() && m_pActiveMainForm->GetController().NeedsSave())
	{
		m_ePendingClose = PendingClose::kOpeningCloseCurrent;
		return;
	}
	else
	{
		m_eMainFormCleanupAction = kMfcaClose;
	}
}

void Root::OnExit()
{
	if (AnyMainFormNeedsSave())
	{
		m_ePendingClose = PendingClose::kOpeningExit;
	}
	else
	{
		(void)Engine::Get()->PostNativeQuitMessage();
	}
}

void Root::OnNew()
{
	InternalAddMainForm(FilePath());
}

void Root::OnOpen()
{
	FilePath filePath;
	if (Engine::Get()->DisplayFileDialogSingleSelection(
		filePath,
		Engine::FileDialogOp::kOpen,
		FileType::kScenePrefab, // TODO:
		GameDirectory::kContent))
	{
		InternalAddMainForm(filePath);
	}
}

void Root::OnSave()
{
	// Nothing to save if no active main form.
	if (!m_pActiveMainForm.IsValid())
	{
		return;
	}

	// Nothing to save if controller says state doesn't need saving.
	DevUI::Controller& rController = m_pActiveMainForm->GetController();
	InternalDoSave(rController);
}

void Root::OnSaveAs()
{
	// Nothing to save if no active main form.
	if (!m_pActiveMainForm.IsValid())
	{
		return;
	}

	DevUI::Controller& rController = m_pActiveMainForm->GetController();
	InternalDoSaveAs(rController, String());
}

void Root::OnSaveAs(const String& sFileName)
{
	// Nothing to save if no active main form.
	if (!m_pActiveMainForm.IsValid())
	{
		return;
	}

	DevUI::Controller& rController = m_pActiveMainForm->GetController();
	InternalDoSaveAs(rController, sFileName);
}

} // namespace Seoul::EditorUI
