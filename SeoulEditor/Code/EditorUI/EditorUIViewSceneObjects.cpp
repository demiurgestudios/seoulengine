/**
* \file EditorUIViewSceneObjects.cpp
* \brief An editor view that displays
* the list of objects in the view's root object
* group.
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#include "DevUIController.h"
#include "DevUIImGui.h"
#include "DevUIImGuiRenderer.h"
#include "EditorSceneContainer.h"
#include "EditorUIIcons.h"
#include "EditorUIRoot.h"
#include "EditorUIControllerScene.h"
#include "EditorUIIControllerSceneRoot.h"
#include "EditorUIUtil.h"
#include "EditorUIViewSceneObjects.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "ReflectionType.h"
#include "SceneFreeTransformComponent.h"
#include "SceneObject.h"
#include "ScenePrefab.h"
#include "ScenePrefabComponent.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_ENUM(EditorUI::ViewSceneObjectsFilterMode)
	SEOUL_ENUM_N("Id", EditorUI::ViewSceneObjectsFilterMode::kId)
	SEOUL_ENUM_N("Type", EditorUI::ViewSceneObjectsFilterMode::kType)
SEOUL_END_ENUM()

namespace EditorUI
{

// TODO: Break this out.
static const UInt32 kuInputTextOversize = 64u;

// TODO: Break out into a utility file.
static const HString kDefaultObjectCategory("Objects");
static const HString kDefaultPrefabCategory("Prefabs");

ViewSceneObjects::ViewSceneObjects()
	: m_vComponentTypes(SceneComponentUtil::PopulateComponentTypes(true, false))
	, m_bMouseLeftLock(false)
	, m_eFilterMode(ViewSceneObjectsFilterMode::kId)
	, m_sFilterId()
	, m_iFilterType(-1)
	, m_vFilteredObjects()
	, m_pRenaming()
	, m_vRenameBuffer()
	, m_bRenameFirst(false)
{
}

ViewSceneObjects::~ViewSceneObjects()
{
}

void ViewSceneObjects::DoPrePose(DevUI::Controller& rController, RenderPass& rPass)
{
	using namespace ImGui;

	auto pRoot = DynamicCast<ControllerScene*>(&rController);
	if (nullptr == pRoot)
	{
		return;
	}

	auto const& scene = pRoot->GetScene();
	if (scene.IsLoading())
	{
		return;
	}

	// Pose the object list.
	InternalPoseObjects(*pRoot);

	// No longer left click locked if released.
	if (IsMouseReleased(0)) { m_bMouseLeftLock = false; }
}

void ViewSceneObjects::InternalAddObject(ControllerScene& rController)
{
	// TODO: More basic object templates?
	// TODO: Don't copy and paste this code.
	String const sId("Object");
	SharedPtr<Scene::Object> pObject(SEOUL_NEW(MemoryBudgets::SceneObject) Scene::Object(sId));
	pObject->SetEditorCategory(kDefaultObjectCategory);
	pObject->AddComponent(SharedPtr<Scene::Component>(SEOUL_NEW(MemoryBudgets::SceneComponent) Scene::FreeTransformComponent));

	rController.AddObject(pObject);
}

void ViewSceneObjects::InternalAddPrefab(ControllerScene& rController)
{
	// TODO: More basic object templates?
	// TODO: Don't copy and paste this code.
	String const sId("Prefab");
	SharedPtr<Scene::Object> pObject(SEOUL_NEW(MemoryBudgets::SceneObject) Scene::Object(sId));
	pObject->SetEditorCategory(kDefaultPrefabCategory);

	// Give the object a FreeTransform Component.
	pObject->AddComponent(SharedPtr<Scene::Component>(SEOUL_NEW(MemoryBudgets::SceneComponent) Scene::FreeTransformComponent));

	// Now a prefab.
	pObject->AddComponent(SharedPtr<Scene::Component>(SEOUL_NEW(MemoryBudgets::SceneComponent) Scene::PrefabComponent));

	rController.AddObject(pObject);
}

Bool ViewSceneObjects::InternalGetComponentName(void* pData, Int iIndex, Byte const** psOut)
{
	const ViewSceneObjects& r = *((ViewSceneObjects*)pData);
	if (iIndex < 0 || iIndex >= (Int32)r.m_vComponentTypes.GetSize())
	{
		return false;
	}

	*psOut = r.m_vComponentTypes[iIndex].m_DisplayName.CStr();
	return true;
}

void ViewSceneObjects::InternalPoseContextMenu(ControllerScene& rController)
{
	auto const& tSelected = rController.GetSelectedObjects();

	Bool const bSelected = !tSelected.IsEmpty();
	Bool const bCanCopy = (bSelected && rController.CanCopy());
	Bool const bCanPaste = (bSelected && rController.CanPaste());
	Bool const bCanDelete = (bSelected && rController.CanDelete());
	Bool const bCanRename = (bSelected && tSelected.GetSize() == 1u);

	using namespace ImGui;
	if (MenuItem("&Copy", "Ctrl+C", false, bCanCopy)) { rController.Copy(); }
	if (MenuItem("&Paste", "Ctrl+V", false, bCanPaste)) { rController.Paste(); }
	if (MenuItem("&Delete", "Shift+Del", false, bCanDelete)) { rController.Delete(); }

	Separator();

	if (MenuItem("&Rename", nullptr, false, bCanRename)) { InternalStartRenamObject(*tSelected.Begin()); }

	Separator();

	if (MenuItem("&New Object", nullptr, false, true)) { InternalAddObject(rController); }
	if (MenuItem("New Pre&fab", nullptr, false, true)) { InternalAddPrefab(rController); }
}

static void GetTreeViewTextures(
	const SharedPtr<Scene::Object>& pObject,
	FilePath& rClosed,
	ImTextureID& rClosedTexture,
	FilePath& rOpen,
	ImTextureID& rOpenTexture)
{
	auto pUI = Root::Get();
	auto& renderer = pUI->GetRenderer();
	auto const& icons = pUI->GetIcons();

	// Prefabs get a special icon.
	rClosed = icons.m_Object;
	if (pObject->GetComponent<Scene::PrefabComponent>().IsValid())
	{
		rClosed = icons.m_Prefab;
	}

	rClosedTexture = renderer.ResolveTexture(rClosed);
	rOpen = rClosed;
	rOpenTexture = rClosedTexture;
}

/**
 * @return Whether a category is considered visible or not.
 *
 * A category is visible if at least one object contained
 * within it is visible.
 */
static inline Bool IsCategoryVisible(
	HString name,
	EditorScene::Container::Objects::ConstIterator i,
	EditorScene::Container::Objects::ConstIterator const iEnd)
{
	while (i < iEnd)
	{
		// If we hit a new category before
		// finding a visible object, we're done.
		if ((*i)->GetEditorCategory() != name)
		{
			return false;
		}

		// Found a visible object, category is visible.
		if ((*i)->GetVisibleInEditor())
		{
			return true;
		}

		++i;
	}

	// No visible object, category is not visible.
	return false;
}

/** Set the visibility of all objects in a category to the specified value. */
static inline void SetCategoryVisible(
	ControllerScene& rController,
	HString name,
	EditorScene::Container::Objects::ConstIterator i,
	EditorScene::Container::Objects::ConstIterator const iEnd,
	Bool bVisible)
{
	ControllerScene::SelectedObjects tObjects;
	while (i < iEnd)
	{
		// Hit the next category, done.
		if ((*i)->GetEditorCategory() != name)
		{
			break;
		}

		// Add the object.
		SEOUL_VERIFY(tObjects.Insert(*i).Second);

		++i;
	}

	// Sanity check, but expected to always be true.
	if (!tObjects.IsEmpty())
	{
		rController.SetObjectVisibility(tObjects, bVisible);
	}
}

void ViewSceneObjects::InternalPoseFilter(ControllerScene& rController)
{
	using namespace ImGui;

	auto pUI = Root::Get();
	auto& renderer = pUI->GetRenderer();
	auto const& icons = pUI->GetIcons();
	auto pDelete = renderer.ResolveTexture(icons.m_Delete);
	auto pSearch = renderer.ResolveTexture(icons.m_Search);

	Int iCurrent = (Int)m_eFilterMode;
	if (ImGui::ImageButtonCombo(
		pSearch,
		ImVec2(GetFontSize(), GetFontSize()),
		&iCurrent,
		ImGuiEnumNameUtil<ViewSceneObjectsFilterMode>,
		nullptr,
		(Int)ViewSceneObjectsFilterMode::COUNT))
	{
		m_eFilterMode = (ViewSceneObjectsFilterMode)iCurrent;
	}

	SameLine();

	PushItemWidth(GetContentRegionAvail().x - GetFontSize() - 2.0f * GetStyle().FramePadding.x);
	switch (m_eFilterMode)
	{
	case ViewSceneObjectsFilterMode::kId:
	{
		(void)InputText("##FilterString", m_sFilterId);
	}
	break;
	case ViewSceneObjectsFilterMode::kType:
	{
		(void)ImGui::Combo(
			"##FilterType",
			&m_iFilterType,
			InternalGetComponentName,
			this,
			m_vComponentTypes.GetSize());
	}
	break;
	default:
		break;
	};
	PopItemWidth();

	SameLine();

	if (ImageButton(
		pDelete,
		ImVec2(GetFontSize(), GetFontSize()),
		ImVec2(0, 0),
		ImVec2(1, 1),
		-1,
		ImVec4(0, 0, 0, 0),
		ImVec4(1, 1, 1, 1),
		IsFiltering()))
	{
		switch (m_eFilterMode)
		{
		case ViewSceneObjectsFilterMode::kId: m_sFilterId.Clear(); break;
		case ViewSceneObjectsFilterMode::kType: m_iFilterType = -1; break;
		default:
			break;
		};
	}
}

void ViewSceneObjects::InternalPoseObjects(ControllerScene& rController)
{
	using namespace ImGui;

	// Filter handling - must happen first,
	// as it can update the results from InternalResolveObjects().
	InternalPoseFilter(rController);

	// Resolve object sets.
	auto const& tSelected = rController.GetSelectedObjects();
	auto const& vObjects = InternalResolveObjects(rController);

	Int32 iLastSelectionIndex = -1;
	{
		auto iLastSelection = vObjects.Find(rController.GetLastSelection());
		iLastSelectionIndex = (iLastSelection != vObjects.End() ? (Int32)(iLastSelection - vObjects.Begin()) : -1);
	}

	// Get textures for visibility.
	auto pUI = Root::Get();
	auto& renderer = pUI->GetRenderer();
	auto const& icons = pUI->GetIcons();
	auto pVisible = renderer.ResolveTexture(icons.m_EyeOpen);
	auto pHidden = renderer.ResolveTexture(icons.m_EyeClosed);

	// Various captures of this loop - otherwise,
	// loop over objects and handle various input events.
	SharedPtr<Scene::Object> pRightClicked;
	Bool bRightClickSelected = false;
	SharedPtr<Scene::Object> pVisibilityClicked;
	Bool bClickedVisibility = false;
	Bool bClickedVisibilityOnSelected = false;
	HString categoryName;
	Bool bCategoryOpen = true;
	HString setCategoryName;
	BeginChild("##Objects");
	for (auto i = vObjects.Begin(); vObjects.End() != i; ++i)
	{
		auto const& pObject = *i;

		// Category handling.
		if (!IsFiltering())
		{
			// Get the object's category.
			auto objectCategory = pObject->GetEditorCategory();

			// On a category change, need to add a collapsable
			// header for the category.
			if (objectCategory != categoryName)
			{
				Bool bVisibilityToggle = false;

				// Compute whether the current category.
				// has any visible objects within it.
				Bool const bCategoryVisible = IsCategoryVisible(objectCategory, i, vObjects.End());

				// TODO: Wrap this styling in a utility function.

				// Display the category header - colored differently
				// to separate it from the tree items that represent
				// objects.
				auto const& style = GetStyle();
				bCategoryOpen = CollapsingHeaderEx(
					objectCategory.CStr(),
					(bCategoryVisible ? pVisible : pHidden),
					&bVisibilityToggle,
					ImGuiTreeNodeFlags_DefaultOpen);

				// Now in the new category.
				categoryName = objectCategory;

				// Drag and drop handling.
				if (Root::Get()->IsItemDragAndDropTarget() &&
					Root::Get()->GetDragData().m_Data.IsOfType<DragSourceSelectedSceneObjects>())
				{
					// On release, mark that we
					// have a category update to apply.
					if (IsMouseReleased(0))
					{
						setCategoryName = categoryName;
					}
					// Otherwise, just mark that
					// we're a valid drop target.
					else
					{
						Root::Get()->MarkCanDrop();
					}
				}

				// Visibility button - if clicked,
				// need to set visibility on current selection.
				if (bVisibilityToggle)
				{
					SetCategoryVisible(
						rController,
						objectCategory,
						i,
						vObjects.End(),
						!bCategoryVisible);
				}
			}

			// Can skip the object if its category is collapsed.
			if (!bCategoryOpen)
			{
				continue;
			}
		}

		Int32 const iObject = (Int32)(i - vObjects.Begin());

		// Get images we'll need for the tree view.
		FilePath closedFilePath;
		ImTextureID closedTexture;
		FilePath openFilePath;
		ImTextureID openTexture;
		GetTreeViewTextures(pObject, closedFilePath, closedTexture, openFilePath, openTexture);

		// Track if the object is selected or not.
		Bool bSelected = tSelected.HasKey(pObject);

		// All objets are leaf nodes. Also, prefix with
		// a bullet.
		ImGuiTreeNodeFlags eFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth;

		// Highlight if selected.
		eFlags |= (bSelected ? ImGuiTreeNodeFlags_Selected : 0);

		// Check if the visibility button is hovered.
		Float const fVizButtonRight = (GetCursorPosX() + GetContentRegionAvail().x);
		Float const fVizButtonLeft = (fVizButtonRight - GetFontSize());
		Bool const bVisibilityHovered = IsMouseHoveringCursorRelative(
			ImVec2(fVizButtonLeft, GetCursorPosY()),
			ImVec2(GetFontSize(), GetFontSize()));

		// Check if the visibility bubble was clicked - this
		// overrides other actions.
		if (bVisibilityHovered && !bClickedVisibility)
		{
			// If we clicked the visibility, track whether we toggled visibility
			// on a selected item.
			if (IsMouseClicked(0))
			{
				bClickedVisibility = true;
				pVisibilityClicked = pObject;
				bClickedVisibilityOnSelected = bSelected;
			}
		}

		// On visibility click, cancel renaming.
		if (bClickedVisibility)
		{
			m_pRenaming.Reset();
		}

		// Visibility bubble.
		Bool const bVisible = pObject->GetVisibleInEditor();
		Bool bVizToggled = false;

		// Special handling for a renaming event.
		if (IsRenaming(pObject))
		{
			InternalHandleRenamObject(rController, (IsTreeNodeOpen(pObject->GetId().CStr(), eFlags) ? openTexture : closedTexture));
		}
		// In this case, always pose the tree node image,
		// but only perform additional processing
		// if the visibility bubble was not clicked.
		else if (TreeNodeImageEx(closedTexture, openTexture, pObject->GetId().CStr(), (bVisible ? pVisible : pHidden), &bVizToggled, eFlags))
		{
			// None of these internal actions are possible
			// if we're hovering the visibility bubble.
			if (!bVisibilityHovered)
			{
				// Check for dragging - StartDragging()
				// automatically filters if it's already dragging.
				if (IsItemActive() && IsMouseDragging(0))
				{
					Root::Get()->StartDragging(
						closedFilePath,
						DragSourceSelectedSceneObjects());
				}

				// Check for clicks and toggle selected space when they occur.
				if (IsItemClicked())
				{
					// Stop renaming on any other item click.
					m_pRenaming.Reset();

					// Shift held engages multi-selection behavior, and combines
					// in unique ways with control.
					if (GetIO().KeyShift)
					{
						// If last selection is not set, then this an
						// exception where we set the last item selected index
						// while Shift is held. NOTE:
						// This differs from Windows behavior. In Windows,
						// this would just set the "active" index, which is
						// the dotted outline that indicates the target control
						// for keyboard use (which our ImGui backend currently
						// does not support).
						if (iLastSelectionIndex < 0 || (UInt32)iLastSelectionIndex >= vObjects.GetSize())
						{
							// Update selected state of the object.
							rController.UniqueSetObjectSelected(pObject, !bSelected);
							bSelected = !bSelected;

							// Single click sets the last selected index.
							iLastSelectionIndex = iObject;
							m_bMouseLeftLock = true;
						}
						else
						{
							// The new selection set is all objects starting at
							// m_iLastItemSelected up to an including the current.
							auto iFirst = iLastSelectionIndex;
							auto iLast = iObject;
							if (iFirst > iLast) { Swap(iFirst, iLast); }

							ControllerScene::SelectedObjects tNewSelected;
							for (Int32 iInner = iFirst; iInner <= iLast; ++iInner)
							{
								tNewSelected.Insert(vObjects[iInner]);
							}

							// If Control is held, we include the existing
							// selected set as well as the new selection. We
							// also set the last item selected index.
							auto pLastSelection(rController.GetLastSelection());
							if (GetIO().KeyCtrl)
							{
								auto const iBeginSelected = tSelected.Begin();
								auto const iEndSelected = tSelected.End();
								for (auto iInner = iBeginSelected; iEndSelected != iInner; ++iInner)
								{
									tNewSelected.Insert(*iInner);
								}

								iLastSelectionIndex = iObject;
								pLastSelection = pObject;
							}

							// Update.
							rController.SetSelectedObjects(pLastSelection, tNewSelected);
							m_bMouseLeftLock = true;
						}
					}
					// If there is no existing selection, then control selection
					// is identical to single click selection.
					else if (GetIO().KeyCtrl && !tSelected.IsEmpty())
					{
						// If there is a single selection and it is the currently
						// selected element, then this because a unique toggle off.
						if (tSelected.GetSize() == 1 && bSelected)
						{
							// Update selected state of the object.
							rController.UniqueSetObjectSelected(pObject, false);
							bSelected = false;
							m_bMouseLeftLock = true;
						}
						else
						{
							// If Control is held and Shift is not, then this is just a toggle.
							// The new selection state is the previous selection state with the
							// current item toggled.
							auto tNewSelected = tSelected;

							// Toggle.
							if (bSelected) { tNewSelected.Erase(pObject); }
							else { tNewSelected.Insert(pObject); }
							bSelected = !bSelected;

							// Update.
							rController.SetSelectedObjects(pObject, tNewSelected);
							m_bMouseLeftLock = true;
						}

						// Control click also updates the last selected index.
						iLastSelectionIndex = iObject;
					}
					// Single selection behavior.
					else
					{
						if (!bSelected)
						{
							// Update selected state of the object.
							rController.UniqueSetObjectSelected(pObject, true);
							bSelected = true;
							m_bMouseLeftLock = true;
						}

						// Single click sets the last selected index.
						iLastSelectionIndex = iObject;
					}
				}

				// Rename or single selection handling - requires a release.
				if (IsItemHovered() && !m_bMouseLeftLock)
				{
					if (IsMouseReleased(0) && bSelected)
					{
						// If this is one of many selections, it becomes a unique
						// selection on release.
						if (tSelected.GetSize() > 1u)
						{
							rController.UniqueSetObjectSelected(pObject, true);
						}
						// Otherwise, we trigger rename handling.
						else
						{
							InternalStartRenamObject(pObject);
						}
					}
				}

				// Handle context menu click (right-click).
				if (IsItemHovered() && IsMouseClicked(1))
				{
					pRightClicked = pObject;
					bRightClickSelected = bSelected;
				}
			}

			TreePop();
		}
	}

	// Handle visibility toggled, if clicked.
	if (bClickedVisibility)
	{
		Bool const bNewVisibility = !pVisibilityClicked->GetVisibleInEditor();

		// If the visibility bubble was clicked on a selected item,
		// we update the visibility on all selected items. The
		// resulting visibility is still the opposite of the visibility
		// of the clicked item.
		if (bClickedVisibilityOnSelected)
		{
			rController.SetObjectVisibility(tSelected, bNewVisibility);
		}
		// Otherwise, we only toggle visibility on the single item.
		else
		{
			IControllerSceneRoot::SelectedObjects t;
			SEOUL_VERIFY(t.Insert(pVisibilityClicked).Second);
			rController.SetObjectVisibility(t, bNewVisibility);
		}
	}

	// Handle the right clicked item, if defined.
	if (pRightClicked.IsValid())
	{
		// Trigger the context menu.
		OpenPopup("Object Context Menu");

		// Stop renaming on any other item click.
		m_pRenaming.Reset();

		// If not selected, uniquely select the right-clicked item.
		if (!bRightClickSelected)
		{
			rController.UniqueSetObjectSelected(pRightClicked, true);
		}
	}

	// Now handle the actual context menu - this call will either:
	// - open because the block above opened it.
	// - open because a right-click occured in the window with no item.
	if (BeginPopupContextWindow("Object Context Menu", 1 | ImGuiPopupFlags_NoOpenOverItems))
	{
		// Stop renaming on any other item click.
		m_pRenaming.Reset();

		InternalPoseContextMenu(rController);
		ImGui::EndPopup();
	}

	// Handle click outside the list of items (click the window background, which is
	// equivalent to a "select none".
	if (IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
		IsMouseClicked(0) &&
		!IsAnyItemHovered())
	{
		rController.UniqueSetObjectSelected(SharedPtr<Scene::Object>(), false);
	}

	// Handle category set if specified.
	if (!setCategoryName.IsEmpty())
	{
		// Get the current selection.
		auto const& tSelected = rController.GetSelectedObjects();

		// TODO: Update CommitPropertyEdit() API
		// for this case - I think it can *always* just
		// capture the old properties if we pass in the path
		// and new value.

		// Gather old values for commit.
		ControllerScene::PropertyValues vOldValues;
		vOldValues.Reserve(tSelected.GetSize());
		for (auto const& pObject : tSelected)
		{
			vOldValues.PushBack(pObject->GetEditorCategory());
		}

		// Commit the category change.
		PropertyUtil::Path vPath;
		vPath.PushBack(PropertyUtil::NumberOrHString(HString("Category")));
		rController.CommitPropertyEdit(
			vPath,
			vOldValues,
			IControllerPropertyEditor::PropertyValues(1u, setCategoryName));
	}

	EndChild();
}

void ViewSceneObjects::InternalHandleRenamObject(
	ControllerScene& rController,
	void* pTexture)
{
	using namespace ImGui;

	// Make sure the rename buffer has some extra space.
	UInt32 const uSize = (m_vRenameBuffer.IsEmpty() ? 0u : StrLen(m_vRenameBuffer.Data()));
	m_vRenameBuffer.Resize(uSize + 1u + kuInputTextOversize);
	m_vRenameBuffer[uSize] = '\0';

	SetCursorPosX(GetCursorPosX() + GetStyle().FramePadding.x);
	Image(pTexture, ImVec2(GetFontSize(), GetFontSize()));
	SameLine();

	// Make sure the input text has focus on first display.
	ImGuiInputTextFlags eFlags = ImGuiInputTextFlags_EnterReturnsTrue;
	if (m_bRenameFirst)
	{
		eFlags |= ImGuiInputTextFlags_SetFocus | ImGuiInputTextFlags_AutoSelectAll;
		m_bRenameFirst = false;
	}

	PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
	if (InputTextEx(
		"##Object Rename",
		NULL,
		m_vRenameBuffer.Data(),
		m_vRenameBuffer.GetSize(),
		ImVec2(0, GetFontSize()),
		eFlags))
	{
		PropertyUtil::Path vPath;
		vPath.PushBack(HString("Id"));
		rController.CommitPropertyEdit(
			vPath,
			IControllerPropertyEditor::PropertyValues(1u, m_pRenaming->GetId()),
			IControllerPropertyEditor::PropertyValues(1u, String(m_vRenameBuffer.Data())));

		m_pRenaming.Reset();
	}
	// If we click anywhere else, or if lost focus otherwise,
	// stop renaming.
	else if (
		!IsItemActive() ||
		(IsMouseClicked(0) && !IsItemClicked(0)) ||
		(IsMouseClicked(1) && !IsItemClicked(1)))
	{
		m_pRenaming.Reset();
	}
	PopStyleVar();
}

namespace
{

/**
 * Specialized ObjectSorter - unlike the standard, does *not* include category,
 * since it's only included in filtered lists that don't display the category.
 */
struct ObjectSorter
{
	Bool operator()(const SharedPtr<Scene::Object>& pA, const SharedPtr<Scene::Object>& pB) const
	{
		return (pA->GetId() < pB->GetId());
	}
};

} // namespace anonymous

const ViewSceneObjects::Objects& ViewSceneObjects::InternalResolveObjects(
	ControllerScene& rController)
{
	auto const& vAllObjects = rController.GetScene().GetObjects();
	if (IsFiltering())
	{
		// TODO: Cache this so we're not refreshing it every frame.
		m_vFilteredObjects.Clear();
		m_vFilteredObjects.Reserve(vAllObjects.GetSize());

		auto const sFilter(m_sFilterId.ToLowerASCII());
		auto const& type(m_iFilterType >= 0
			? *m_vComponentTypes[m_iFilterType].m_pType
			: TypeOf<void>());
		for (auto const& pObject : vAllObjects)
		{
			switch (m_eFilterMode)
			{
			case ViewSceneObjectsFilterMode::kId:
				if (pObject->GetId().ToLowerASCII().Find(sFilter) != String::NPos)
				{
					m_vFilteredObjects.PushBack(pObject);
				}
				break;
			case ViewSceneObjectsFilterMode::kType:
				if (pObject->GetComponent(type).IsValid())
				{
					m_vFilteredObjects.PushBack(pObject);
				}
				break;
			default:
				break;
			};
		}

		ObjectSorter sorter;
		QuickSort(m_vFilteredObjects.Begin(), m_vFilteredObjects.End(), sorter);

		return m_vFilteredObjects;
	}
	else
	{
		return vAllObjects;
	}
}

void ViewSceneObjects::InternalStartRenamObject(
	const SharedPtr<Scene::Object>& pObject)
{
	m_pRenaming = pObject;
	auto sId = m_pRenaming->GetId();
	m_vRenameBuffer.Resize(sId.GetSize() + kuInputTextOversize + 1u);
	memcpy(m_vRenameBuffer.Data(), sId.CStr(), sId.GetSize());
	m_vRenameBuffer[sId.GetSize()] = '\0';
	m_bRenameFirst = true;
}

} // namespace EditorUI

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
