/**
 * \file EditorUIMainFormScene.cpp
 * \brief Main form for modifying a root scene prefab.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIImGui.h"
#include "EditorUIControllerScene.h"
#include "EditorUIViewCommandHistory.h"
#include "EditorUIViewFileBrowser.h"
#include "EditorUIViewLog.h"
#include "EditorUIViewSceneInspector.h"
#include "EditorUIViewSceneObjects.h"
#include "EditorUIViewSceneViewport.h"
#include "EditorUIMainFormScene.h"
#include "GamePaths.h"
#include "ReflectionDefine.h"
#include "SceneFxComponent.h"
#include "SceneMeshDrawComponent.h"
#include "SceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

static DevUI::MainForm::ViewEntry ToEntry(DevUI::View* pView)
{
	auto const name = pView->GetId();

	DevUI::MainForm::ViewEntry entry;
	entry.m_sPrunedName = String(name).ReplaceAll("&", String());
	entry.m_Name = name;
	entry.m_pView = pView;

	// TODO: Need to sort out the desired behavior and how to generalize
	// this, otherwise this will be needed in every editor main form.
	entry.m_pView->SetOpen(true);

	return entry;
}

static DevUI::MainForm::Views CreateViews(const SharedPtr<ControllerScene>& pController)
{
	DevUI::MainForm::Views vViews;

	ViewSceneViewport* pViewport(
		SEOUL_NEW(MemoryBudgets::Editor) ViewSceneViewport(pController->GetSettings()));
	ViewSceneObjects* pOutline(
		SEOUL_NEW(MemoryBudgets::Editor) ViewSceneObjects);
	ViewSceneInspector* pInspector(
		SEOUL_NEW(MemoryBudgets::Editor) ViewSceneInspector);
	ViewCommandHistory* pCommandHistory(
		SEOUL_NEW(MemoryBudgets::Editor) ViewCommandHistory);

	// TODO: Hate hard coding these things - at the very least, should break this out into one place.
	FilePath dirPath;
	dirPath.SetDirectory(GameDirectory::kContent);
	dirPath.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename("Authored"));

	ViewFileBrowser* pFileBrowser(
		SEOUL_NEW(MemoryBudgets::Editor) ViewFileBrowser(dirPath));
	ViewLog* pLog(
		SEOUL_NEW(MemoryBudgets::Editor) ViewLog);

	vViews.PushBack(ToEntry(pOutline));
	vViews.PushBack(ToEntry(pInspector));
	vViews.PushBack(ToEntry(pViewport));
	vViews.PushBack(ToEntry(pCommandHistory));
	vViews.PushBack(ToEntry(pFileBrowser));
	vViews.PushBack(ToEntry(pLog));

	return vViews;
}

MainFormScene::MainFormScene(const SharedPtr<ControllerScene>& pController)
	: DevUI::MainForm(CreateViews(pController))
	, m_pController(pController)
{
	// Sanity check, this is enforced.
	SEOUL_ASSERT(m_pController.IsValid());
}

MainFormScene::~MainFormScene()
{
}

DevUI::Controller& MainFormScene::GetController()
{
	return *m_pController;
}

// TODO: Eliminate redundancy like this between multiple views that share this kind of menu.
void MainFormScene::PrePoseMainMenu()
{
	using namespace ImGui;

	Bool const bVisible = BeginMenu("&Edit");
	{
		Bool const bCanRedo = m_pController->CanRedo();
		Bool const bCanUndo = m_pController->CanUndo();
		Bool const bCanCut = m_pController->CanCut();
		Bool const bCanCopy = m_pController->CanCopy();
		Bool const bCanPaste = m_pController->CanPaste();
		Bool const bCanDelete = m_pController->CanDelete();

		if (MenuItemEx(bVisible, "&Undo", "Ctrl+Z", false, bCanUndo)) { m_pController->Undo(); }
		if (MenuItemEx(bVisible, "&Redo", "Ctrl+Y", false, bCanRedo)) { m_pController->Redo(); }
		SeparatorEx(bVisible);
		if (MenuItemEx(bVisible, "Cu&t", "Ctrl+X", false, bCanCut)) { m_pController->Cut(); }
		if (MenuItemEx(bVisible, "&Copy", "Ctrl+C", false, bCanCopy)) { m_pController->Copy(); }
		if (MenuItemEx(bVisible, "&Paste", "Ctrl+V", false, bCanPaste)) { m_pController->Paste(); }
		if (MenuItemEx(bVisible, "&Delete", "Shift+Del", false, bCanDelete)) { m_pController->Delete(); }

		if (bVisible)
		{
			EndMenu();
		}
	}
}

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE
