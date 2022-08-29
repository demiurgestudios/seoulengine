/**
 * \file GameDevUIViewUIExplorer.cpp
 * \brief A developer UI view that displays current UI
 * graph state.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "DevUIRoot.h"
#include "DevUIImGui.h"
#include "DevUIUtil.h"
#include "Engine.h"
#include "FalconMovieClipInstance.h"
#include "FalconStage3DSettings.h"
#include "FalconTextChunk.h"
#include "FxManager.h"
#include "GameDevUIViewGameUI.h"
#include "GameDevUIViewUIExplorer.h"
#include "LocManager.h"
#include "Prereqs.h"
#include "ReflectionDefine.h"
#include "RenderDevice.h"
#include "ScopedAction.h"
#include "UIManager.h"
#include "UIMovie.h"
#include "UIRenderer.h"
#include "Viewport.h"

#if (SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)

namespace Seoul
{

SEOUL_BEGIN_TYPE(Game::DevUIViewUIExplorer, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(DisplayName, "UI Explorer")
	SEOUL_PARENT(DevUI::View)
SEOUL_END_TYPE()

namespace Game
{

DevUIViewUIExplorer::DevUIViewUIExplorer()
	: m_bNeedScroll(false)
	, m_vExpansion()
	, m_tLastConditions()
	, m_vSortedConditions()
	, m_vTriggerHistory()
	, m_bStage3DSettingsDirty(false)
	, m_bTextEffectSettingsDirty(false)
{
}

DevUIViewUIExplorer::~DevUIViewUIExplorer()
{
}

static Bool PoseEnumGetter(void* pData, Int iIndex, Byte const** psOut)
{
	auto const& e = *((Reflection::Enum const*)pData);
	auto const& vNames = e.GetNames();

	if (iIndex >= 0 && (UInt32)iIndex < vNames.GetSize())
	{
		*psOut = vNames[iIndex].CStr();
		return true;
	}

	return false;
}

template <typename T>
static Bool SelectEnum(T& reEnum)
{
	using namespace ImGui;

	auto const& e = EnumOf<T>();
	auto const& vValues = e.GetValues();

	// Find the offset into the values array for the value.
	Int iCurrent = 0;
	{
		auto i = vValues.Find((Int)reEnum);
		if (vValues.End() != i)
		{
			iCurrent = (Int)(i - vValues.Begin());
		}
	}

	// Present and update.
	Bool bReturn = false;
	if (Combo("", &iCurrent, PoseEnumGetter, (void*)&e, (Int)e.GetNames().GetSize(), (Int)e.GetNames().GetSize()))
	{
		if (iCurrent >= 0 && iCurrent < (Int32)vValues.GetSize())
		{
			reEnum = (T)vValues[iCurrent];
			bReturn = true;
		}
	}
	return bReturn;
}

static Bool SelectFilePath(
	GameDirectory eDirectory,
	FileType eFileType,
	FilePath& rFilePath)
{
	auto const oldFilePath = rFilePath;

	auto const sDisplayName(oldFilePath.IsValid()
		? Path::GetFileName(oldFilePath.GetRelativeFilenameInSource())
		: String("..."));

	// Clicking the button opens a file selection browser.
	if (ImGui::Button(sDisplayName.CStr(), ImVec2(0, 0)))
	{
		FilePath newFilePath = oldFilePath;
		(void)Engine::Get()->DisplayFileDialogSingleSelection(
			newFilePath,
			Engine::FileDialogOp::kOpen,
			eFileType,
			eDirectory);

		if (newFilePath != oldFilePath)
		{
			rFilePath = newFilePath;
			return true;
		}
	}

	return false;
}

template <typename T>
static Bool Pose(HString name, Reflection::Any& rAny, Bool bCanSet, Bool& rbSet);

template <>
Bool Pose<Bool>(HString name, Reflection::Any& rAny, Bool bCanSet, Bool& rbSet)
{
	Bool b;

	// Need to be pickier about booleans - most things are castable to Bool.
	if ((rAny.IsOfType<Bool>() || rAny.IsOfType<Bool const>()) && Reflection::SimpleCast(rAny, b))
	{
		BeginValue(name);
		if (ImGui::Checkbox("", &b) && bCanSet)
		{
			rAny = b;
			rbSet = true;
		}
		EndValue();
		return true;
	}

	return false;
}

template <>
Bool Pose<Float32>(HString name, Reflection::Any& rAny, Bool bCanSet, Bool& rbSet)
{
	Float32 f;
	if (Reflection::SimpleCast(rAny, f))
	{
		BeginValue(name);
		if (bCanSet)
		{
			if (ImGui::InputFloat("", &f))
			{
				rAny = f;
				rbSet = true;
			}
		}
		else
		{
			ImGui::Text("%f", f);
		}
		EndValue();
		return true;
	}

	return false;
}

template <>
Bool Pose<Int32>(HString name, Reflection::Any& rAny, Bool bCanSet, Bool& rbSet)
{
	// Check if the value is a reasonable Float32 first with some precision.
	if (rAny.IsOfType<Float32>() || rAny.IsOfType<Float64>())
	{
		return false;
	}

	Int32 i;
	if (Reflection::SimpleCast(rAny, i))
	{
		BeginValue(name);
		if (bCanSet)
		{
			if (ImGui::InputInt("", &i))
			{
				rAny = i;
				rbSet = true;
			}
		}
		else
		{
			ImGui::Text("%d", i);
		}
		EndValue();
		return true;
	}

	return false;
}

template <>
Bool Pose<String>(HString name, Reflection::Any& rAny, Bool bCanSet, Bool& rbSet)
{
	static const UInt32 kuExpansionPadding = 64u;

	using namespace Reflection;

	String s;
	if (TypeConstruct(rAny.GetWeakAny(), s))
	{
		BeginValue(name);
		if (bCanSet)
		{
			Vector<Byte, MemoryBudgets::DevUI> vBuffer(s.GetSize() + kuExpansionPadding);
			memcpy(vBuffer.Data(), s.CStr(), s.GetSize());
			vBuffer[s.GetSize()] = '\0';

			if (ImGui::InputTextMultiline("", vBuffer.Data(), vBuffer.GetSize()))
			{
				s.Assign(vBuffer.Data());
				rAny = s;
				rbSet = true;
			}
		}
		else
		{
			ImGui::Text("%s", s.CStr());
		}
		EndValue();
		return true;
	}

	return false;
}

template <>
Bool Pose<Vector2D>(HString name, Reflection::Any& rAny, Bool bCanSet, Bool& rbSet)
{
	using namespace Reflection;

	Vector2D v;
	if (TypeConstruct(rAny.GetWeakAny(), v))
	{
		BeginValue(name);
		if (ImGui::InputFloat2("", v.GetData(), "%.3f", (bCanSet ? 0 : ImGuiInputTextFlags_ReadOnly)))
		{
			rAny = v;
			rbSet = true;
		}
		EndValue();
		return true;
	}

	return false;
}

template <>
Bool Pose<Vector3D>(HString name, Reflection::Any& rAny, Bool bCanSet, Bool& rbSet)
{
	using namespace Reflection;

	Vector3D v;
	if (TypeConstruct(rAny.GetWeakAny(), v))
	{
		BeginValue(name);
		if (ImGui::InputFloat3("", v.GetData(), "%.3f", (bCanSet ? 0 : ImGuiInputTextFlags_ReadOnly)))
		{
			rAny = v;
			rbSet = true;
		}
		EndValue();
		return true;
	}

	return false;
}

template <>
Bool Pose<Vector4D>(HString name, Reflection::Any& rAny, Bool bCanSet, Bool& rbSet)
{
	using namespace Reflection;

	Vector4D v;
	if (TypeConstruct(rAny.GetWeakAny(), v))
	{
		BeginValue(name);
		if (ImGui::InputFloat4("", v.GetData(), "%.3f", (bCanSet ? 0 : ImGuiInputTextFlags_ReadOnly)))
		{
			rAny = v;
			rbSet = true;
		}
		EndValue();
		return true;
	}

	return false;
}

static Bool PoseEnum(HString name, Reflection::Any& rAny, Bool bCanSet, Bool& rbSet)
{
	using namespace Reflection;
	Enum const* pEnum = rAny.GetType().TryGetEnum();
	if (nullptr != pEnum)
	{
		HString value;
		if (pEnum->TryGetName(rAny.GetWeakAny(), value))
		{
			// TODO: Selection entry.

			String sValue(value);
			Any any(sValue);
			return Pose<String>(name, any, false, rbSet);
		}
	}

	return false;
}

static Bool PoseAny(HString name, Reflection::Any& rAny, Bool bCanSet)
{
	Bool bDone = false;
	Bool bSet = false;

	// Order here matters - e.g. an Int32 can usually be converted
	// to a Float32, so we need to try the Int32 conversion first.
	bDone = bDone || Pose<Vector4D>(name, rAny, bCanSet, bSet);
	bDone = bDone || Pose<Vector3D>(name, rAny, bCanSet, bSet);
	bDone = bDone || Pose<Vector2D>(name, rAny, bCanSet, bSet);
	bDone = bDone || PoseEnum(name, rAny, bCanSet, bSet);
	bDone = bDone || Pose<String>(name, rAny, bCanSet, bSet);
	bDone = bDone || Pose<Bool>(name, rAny, bCanSet, bSet);
	bDone = bDone || Pose<Int32>(name, rAny, bCanSet, bSet);
	bDone = bDone || Pose<Float32>(name, rAny, bCanSet, bSet);

	return bSet;
}

static void PoseProperties(const Reflection::WeakAny& weakAny, const Reflection::Type& type)
{
	// Parents.
	UInt32 const uParents = type.GetParentCount();
	for (UInt32 i = 0u; i < uParents; ++i)
	{
		auto pParent = type.GetParent(i);
		PoseProperties(weakAny, *pParent);
	}

	// Self.
	UInt32 const uProperties = type.GetPropertyCount();
	for (UInt32 i = 0u; i < uProperties; ++i)
	{
		auto pProperty = type.GetProperty(i);
		Reflection::Any any;
		if (pProperty->TryGet(weakAny, any))
		{
			if (PoseAny(pProperty->GetName(), any, pProperty->CanSet()))
			{
				(void)pProperty->TrySet(weakAny, any);
			}
		}
	}
}

template <typename T>
static void PoseProperties(T& v)
{
	ImGui::Columns(2);

	auto weakAny = v.GetReflectionThis();
	auto const& type = weakAny.GetType();

	// Perform.
	PoseProperties(weakAny, type);

	ImGui::Columns(1);
}

void DevUIViewUIExplorer::DoPrePose(DevUI::Controller& rController, RenderPass& rPass)
{
	PoseExplorer();
	PoseState();
	PoseStage3DSettings();
	PoseTextEffectSettings();
}

void DevUIViewUIExplorer::DoPrePoseAlways(DevUI::Controller& rController, RenderPass& rPass, Bool bVisible)
{
	ImGuiIO& io = ImGui::GetIO();

	// TODO: Eliminate this coupling.
	FxPreviewModeState state;
	if (FxManager::Get()->GetFxPreviewModeState(state) && state.m_bActive)
	{
		DevUIViewGameUI::Get()->HighlightSelect();
		return;
	}

	// Execute a pick - on success, make sure we
	// and the main menu are visible.
	if (Pick())
	{
		DevUI::Root::Get()->SetMainMenuVisible(true);
		SetOpen(true);
	}
	// Escape is used to deselect.
	else if (
		DevUIViewGameUI::Get()->GetSelectedInstance().IsValid() &&
		!io.WantTextInput &&
		ImGui::IsKeyPressed((int)InputButton::KeyEscape))
	{
		DevUI::Root::Get()->CaptureKey(InputButton::KeyEscape);
		DevUIViewGameUI::Get()->HighlightSelect();
	}
}

UInt32 DevUIViewUIExplorer::GetFlags() const
{
	return ImGuiWindowFlags_HorizontalScrollbar;
}

/** Used when clicks require the view tree to be expanded. */
Bool DevUIViewUIExplorer::CheckExpansion(void* p)
{
	if (m_vExpansion.IsEmpty())
	{
		return false;
	}

	if (m_vExpansion.Back() == p)
	{
		m_vExpansion.PopBack();
		ImGui::SetNextItemOpen(true);
		return true;
	}

	return false;
}

namespace
{

struct PickEntry
{
	CheckedPtr<UI::State> m_pState;
	UI::State::PickEntry m_Entry;

	Bool operator==(const SharedPtr<Falcon::Instance>& p) const
	{
		return m_Entry.m_pHitInstance == p;
	}
};

static void FindBest(
	const SharedPtr<Falcon::Instance>& pSelected,
	const Vector<PickEntry, MemoryBudgets::DevUI>& v,
	PickEntry& rBest)
{
	// First if not selected.
	if (!pSelected.IsValid())
	{
		rBest = v.Front();
		return;
	}

	// Try to find selected.
	auto const iSelected = v.Find(pSelected);
	// None, use first again.
	if (v.End() == iSelected)
	{
		rBest = v.Front();
		return;
	}
	// Last, use first again.
	if (v.End() == (iSelected + 1))
	{
		rBest = v.Front();
		return;
	}
	// Finally, use the next one.
	rBest = *(iSelected + 1);
}

} // namespace anonymous

/* Handle picking, triggered by right-click. */
Bool DevUIViewUIExplorer::Pick()
{
	auto const& io = ImGui::GetIO();

	// Early out if no right-click.
	// Opposite - game view must have captured the input in the client area
	// is virtualized.
	if (!io.WantCaptureMouse ||
		!ImGui::IsMouseClicked(1) ||
		!DevUIViewGameUI::Get()->HoverGameView())
	{
		return false;
	}

	auto const mousePosition = InputManager::Get()->GetMousePosition();
	auto const& vStack = UI::Manager::Get()->GetStack();

	Vector<PickEntry, MemoryBudgets::DevUI> v;
	for (auto const& stackEntry : vStack)
	{
		auto pState = stackEntry.m_pMachine->GetActiveState();
		if (!pState.IsValid())
		{
			continue;
		}

		Vector<UI::State::PickEntry, MemoryBudgets::UIRuntime> vEntries;
		pState->Pick(mousePosition, vEntries);

		for (auto const& e : vEntries)
		{
			PickEntry pick;
			pick.m_Entry = e;
			pick.m_pState = pState;
			v.PushBack(pick);
		}
	}

	// If not empty, find the "best" entry. This is the
	// first entry unless we have a current selection,
	// in which case it is the one after the current selection
	// (or the first entry, if the current selection is
	// the last entry).
	if (!v.IsEmpty())
	{
		PickEntry best;
		FindBest(DevUIViewGameUI::Get()->GetSelectedInstance(), v, best);

		m_bNeedScroll = true;
		UpdateSelected(best.m_Entry.m_pHitMovie, best.m_Entry.m_pHitInstance);
		m_vExpansion.Clear();

		auto pInstance(best.m_Entry.m_pHitInstance);
		while (pInstance.IsValid())
		{
			m_vExpansion.PushBack(pInstance.GetPtr());
			pInstance.Reset(pInstance->GetParent());
		}
		m_vExpansion.PushBack(best.m_Entry.m_pHitMovie);
		m_vExpansion.PushBack(best.m_pState.Get());

		return true;
	}
	else
	{
		// If we get here, no hit occurred,
		// so we need to reset the selection and return failure.
		DevUIViewGameUI::Get()->HighlightSelect();
		return false;
	}
}

void DevUIViewUIExplorer::PoseExplorer()
{
	using namespace ImGui;

	if (CollapsingHeader("Explorer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto const& vStack = UI::Manager::Get()->GetStack();
		for (auto const& stackEntry : vStack)
		{
			auto pStateMachine = stackEntry.m_pMachine;
			auto pState = pStateMachine->GetActiveState();
			if (!pState)
			{
				continue;
			}

			CheckExpansion(pState);
			if (TreeNodeEx(pState, 0, "%s (%s)", pStateMachine->GetName().CStr(), pState->GetStateIdentifier().CStr()))
			{
				for (auto pMovie = pState->GetMovieStackHead(); pMovie.IsValid(); pMovie = pMovie->GetNextMovie())
				{
					CheckExpansion(pMovie);
					if (TreeNodeEx(pMovie, 0, "%s", pMovie->GetMovieTypeName().CStr()))
					{
						// Properties first.
						if (TreeNode("Properties"))
						{
							PoseProperties(*pMovie);
							TreePop();
						}

						// Now the MovieClip hierarchy.
						SharedPtr<Falcon::MovieClipInstance> pRoot;
						if (pMovie->GetRootMovieClip(pRoot))
						{
							PoseMovieClip(pMovie, pRoot);
						}
						TreePop();
					}
				}
				TreePop();
			}
		}
	}
}

void DevUIViewUIExplorer::PoseInstance(
	UI::Movie const* pMovie,
	const SharedPtr<Falcon::Instance>& pInstance)
{
	using namespace ImGui;

	String sName(pInstance->GetName());
#if !SEOUL_SHIP
	if (sName.IsEmpty())
	{
		sName = pInstance->GetDebugName();
	}
#endif // /#if !SEOUL_SHIP

	if (sName.IsEmpty())
	{
		if (pInstance->GetType() == Falcon::InstanceType::kMovieClip)
		{
			auto pMovieClipInstance = (Falcon::MovieClipInstance const*)pInstance.GetPtr();
			sName = String(pMovieClipInstance->GetMovieClipDefinition()->GetClassName());
		}
	}

	if (sName.IsEmpty())
	{
		HString name;
		EnumOf<Falcon::InstanceType>().TryGetName(pInstance->GetType(), name);
		sName = String(name);
	}

	auto const bSelected = (DevUIViewGameUI::Get()->GetSelectedInstance() == pInstance.GetPtr());

	// Handle scroll to target.
	if (bSelected && m_bNeedScroll)
	{
		SetScrollHereY();
		SetScrollX(GetScrollMaxX());
		m_bNeedScroll = false;
	}

	ImGuiTreeNodeFlags eFlags = (bSelected ? ImGuiTreeNodeFlags_Selected : 0);
	if (TreeNodeEx(pInstance.GetPtr(), eFlags, "%s", sName.CStr()))
	{
		PoseProperties(*pInstance);
		TreePop();
	}

	if (IsMouseClicked(0) && IsSpecificItemHovered(GetID(pInstance.GetPtr())))
	{
		UpdateSelected(pMovie, pInstance);
	}
}

void DevUIViewUIExplorer::PoseMovieClip(
	UI::Movie const* pMovie,
	const SharedPtr<Falcon::MovieClipInstance>& pMovieClip)
{
	using namespace ImGui;

	String sName(pMovieClip->GetName());
#if !SEOUL_SHIP
	if (sName.IsEmpty())
	{
		sName = pMovieClip->GetDebugName();
	}
#endif // /#if !SEOUL_SHIP

	if (sName.IsEmpty())
	{
		HString name;
		EnumOf<Falcon::InstanceType>().TryGetName(pMovieClip->GetType(), name);
		sName = String(name);
	}

	SharedPtr<Falcon::Instance> pChild;
	auto const bExpanded = CheckExpansion(pMovieClip.GetPtr());
	auto const bSelected = (DevUIViewGameUI::Get()->GetSelectedInstance() == pMovieClip.GetPtr());

	// Handle scroll to target.
	if (bSelected && m_bNeedScroll)
	{
		SetScrollHereY();
		SetScrollX(GetScrollMaxX());
		m_bNeedScroll = false;
	}

	ImGuiTreeNodeFlags eFlags = (bSelected ? ImGuiTreeNodeFlags_Selected : 0);
	if (TreeNodeEx(pMovieClip.GetPtr(), eFlags, "%s", sName.CStr()))
	{
		// Properties first.
		if (TreeNode("Properties"))
		{
			PoseProperties(*pMovieClip);
			TreePop();
		}

		// Now children.
		UInt32 const uChildren = pMovieClip->GetChildCount();
		if (uChildren > 0u && bExpanded) { SetNextItemOpen(true); }
		if (uChildren > 0u && TreeNode("Children"))
		{
			for (UInt32 i = 0u; i < uChildren; ++i)
			{
				SEOUL_VERIFY(pMovieClip->GetChildAt((Int)i, pChild));
				if (pChild->GetType() == Falcon::InstanceType::kMovieClip)
				{
					SharedPtr<Falcon::MovieClipInstance> pt((Falcon::MovieClipInstance*)pChild.GetPtr());
					PoseMovieClip(pMovie, pt);
				}
				else
				{
					PoseInstance(pMovie, pChild);
				}
			}
			TreePop();
		}

		TreePop();
	}
	if (IsMouseClicked(0) && IsSpecificItemHovered(GetID(pMovieClip.GetPtr())))
	{
		UpdateSelected(pMovie, pMovieClip);
	}
}

void DevUIViewUIExplorer::PoseState()
{
	using namespace ImGui;

	if (CollapsingHeader("State"))
	{
		// Active states.
		if (TreeNodeEx("Machines"))
		{
			ImGui::Columns(2);

			auto const& vStack = UI::Manager::Get()->GetStack();
			for (auto i = vStack.Begin(); vStack.End() != i; ++i)
			{
				auto pStateMachine = i->m_pMachine;
				auto pActive = pStateMachine->GetActiveState();
				BeginValue(pStateMachine->GetName());
				Text("%s", pActive.IsValid() ? pActive->GetStateIdentifier().CStr() : "<null>");
				EndValue();
			}
			ImGui::Columns();

			TreePop();
		}

		// Conditions.
		if (TreeNodeEx("Conditions"))
		{
			// Update our cache of the condition state.
			{
				UI::Manager::Get()->GetConditions(m_tLastConditions);
				m_vSortedConditions.Clear();
				m_vSortedConditions.Reserve(m_tLastConditions.GetSize());
				auto const iBegin = m_tLastConditions.Begin();
				auto const iEnd = m_tLastConditions.End();
				for (auto i = iBegin; iEnd != i; ++i)
				{
					ConditionEntry entry;
					entry.m_bValue = i->Second;
					entry.m_Name = i->First;
					m_vSortedConditions.PushBack(entry);
				}
				QuickSort(m_vSortedConditions.Begin(), m_vSortedConditions.End());
			}

			// Draw.
			ImGui::Columns(2);
			for (auto i = m_vSortedConditions.Begin(); m_vSortedConditions.End() != i; ++i)
			{
				BeginValue(i->m_Name);
				auto bValue = i->m_bValue;
				if (Checkbox("", &bValue))
				{
					i->m_bValue = bValue;
					UI::Manager::Get()->SetCondition(i->m_Name, bValue);
				}
				EndValue();
			}
			ImGui::Columns();

			TreePop();
		}

		// Trigger history
		if (TreeNode("Trigger History"))
		{
			// Update our cache of trigger history.
			UI::Manager::Get()->GetTriggerHistory(m_vTriggerHistory);

			// Draw.
			ImGui::Columns(2);
			for (Int32 i = (Int32)m_vTriggerHistory.GetSize() - 1; i >= 0; --i)
			{
				auto const& entry = m_vTriggerHistory[i];
				BeginValue(entry.m_TriggerName);
				if (entry.m_StateMachine.IsEmpty())
				{
					Text("<no transitions>");
				}
				else
				{
					Text("%s: %s -> %s",
						entry.m_StateMachine.CStr(),
						entry.m_FromState.CStr(),
						entry.m_ToState.CStr());
				}
				EndValue();
			}
			ImGui::Columns();

			TreePop();
		}
	}
}

namespace
{

struct ShadowEntry SEOUL_SEALED
{
	ShadowEntry(HString name = HString(), CheckedPtr<Falcon::Stage3DSettings> pSettings = CheckedPtr<Falcon::Stage3DSettings>())
		: m_pSettings(pSettings)
		, m_Name(name)
	{
	}

	CheckedPtr<Falcon::Stage3DSettings> m_pSettings;
	HString m_Name;

	Bool operator<(const ShadowEntry& b) const
	{
		return (strcmp(m_Name.CStr(), b.m_Name.CStr()) < 0);
	}
}; // struct ShadowEntry

struct TextEffectEntry SEOUL_SEALED
{
	TextEffectEntry(HString name = HString(), CheckedPtr<Falcon::TextEffectSettings> pSettings = CheckedPtr<Falcon::TextEffectSettings>())
		: m_pSettings(pSettings)
		, m_Name(name)
	{
	}

	CheckedPtr<Falcon::TextEffectSettings> m_pSettings;
	HString m_Name;

	Bool operator<(const TextEffectEntry& b) const
	{
		return (strcmp(m_Name.CStr(), b.m_Name.CStr()) < 0);
	}
}; // struct TextEffectEntry

} // namespace anonymous

void DevUIViewUIExplorer::PoseStage3DSettings()
{
	using namespace ImGui;

	if (CollapsingHeader("3D Settings"))
	{
		auto const& tSettings = UI::Manager::Get()->GetStage3DSettingsTable();

		// Save handling.
		if (Button("Save", ImVec2(0, 0), m_bStage3DSettingsDirty))
		{
			FilePath const filePath(UI::Manager::Get()->GetStage3DSettingsFilePath());

			Content::LoadManager::Get()->TempSuppressSpecificHotLoad(filePath);
			if (!Reflection::SaveObject(&tSettings, filePath))
			{
				SEOUL_WARN("Failed saving text effect settings. Check that \"%s\" is not read-only "
					"(checked out from source control).",
					filePath.GetAbsoluteFilenameInSource().CStr());
			}
			else
			{
				m_bStage3DSettingsDirty = false;
			}
		}

		auto const iBegin = tSettings.Begin();
		auto const iEnd = tSettings.End();

		// Build a list for sorting.
		Vector<ShadowEntry> vEntries;
		vEntries.Reserve(tSettings.GetSize());
		for (auto i = iBegin; iEnd != i; ++i)
		{
			vEntries.PushBack(ShadowEntry(i->First, i->Second));
		}
		QuickSort(vEntries.Begin(), vEntries.End());

		// Now display all settings for editing.
		for (auto i = vEntries.Begin(); vEntries.End() != i; ++i)
		{
			if (TreeNode(i->m_Name.CStr()))
			{
				if (TreeNode("Lighting"))
				{
					if (TreeNode("Props"))
					{
						auto& e = i->m_pSettings->m_Lighting.m_Props;

						{
							BeginValue(HString("Color"));
							if (ColorEdit3("", &e.m_vColor.X))
							{
								m_bStage3DSettingsDirty = true;
							}
							EndValue();
						}

						TreePop();
					}

					TreePop();
				}

				if (TreeNode("Perspective"))
				{
					auto& e = i->m_pSettings->m_Perspective;

					if (TreeNode("Debug"))
					{
						Columns(2);

						{
							BeginValue(HString("Show Grid Texture"));
							if (Checkbox("", &e.m_bDebugShowGridTexture))
							{
								m_bStage3DSettingsDirty = true;
							}
							EndValue();
						}

						Columns();
						TreePop();
					}

					{
						BeginValue(HString("Factor"));
						if (DragFloat("", &e.m_fFactor, 0.01f, 0.0f, 0.99f))
						{
							m_bStage3DSettingsDirty = true;
						}
						EndValue();
					}

					{
						BeginValue(HString("Horizon"));
						if (DragFloat("", &e.m_fHorizon, 0.01f, 0.0f, 1.0f))
						{
							m_bStage3DSettingsDirty = true;
						}
						EndValue();
					}

					TreePop();
				}

				if (TreeNode("Shadow"))
				{
					auto& e = i->m_pSettings->m_Shadow;

					if (TreeNode("Debug"))
					{
						Columns(2);

						{
							BeginValue(HString("Enabled"));
							Bool b = e.GetEnabled();
							if (Checkbox("", &b))
							{
								e.SetEnabled(b);
								m_bStage3DSettingsDirty = true;
							}
							EndValue();
						}
						{
							BeginValue(HString("Force One Pass"));
							Bool b = e.GetDebugForceOnePassRendering();
							if (Checkbox("", &b))
							{
								e.SetDebugForceOnePassRendering(b);
								m_bStage3DSettingsDirty = true;
							}
							EndValue();
						}

						Columns();

						TreePop();
					}

					if (TreeNode("Light"))
					{
						Columns(2);

						{
							BeginValue(HString("Pitch"));
							Float f = e.GetLightPitchInDegrees();
							if (DragFloat("", &f, 1.0f, 0.0f, 89.0f))
							{
								e.SetLightPitchInDegrees(f);
								m_bStage3DSettingsDirty = true;
							}
							EndValue();
						}
						{
							BeginValue(HString("Yaw"));
							Float f = e.GetLightYawInDegrees();
							if (DragFloat("", &f, 1.0f, -180.0f, 180.0f))
							{
								e.SetLightYawInDegrees(f);
								m_bStage3DSettingsDirty = true;
							}
							EndValue();
						}

						Columns();

						TreePop();
					}

					if (TreeNode("Plane"))
					{
						Columns(2);

						{
							BeginValue(HString("Pitch"));
							Float f = e.GetPlanePitchInDegrees();
							if (DragFloat("", &f, 1.0f, 0.0f, 89.0f))
							{
								e.SetPlanePitchInDegrees(f);
								m_bStage3DSettingsDirty = true;
							}
							EndValue();
						}

						Columns();

						TreePop();
					}

					if (TreeNode("Quality"))
					{
						Columns(2);

						{
							BeginValue(HString("Alpha"));
							Int i = (Int)(e.GetAlpha() * 255.0f + 0.5f);
							if (DragInt("", &i, 1.0f, 0, 255))
							{
								e.SetAlpha((Float)i / 255.0f);
								m_bStage3DSettingsDirty = true;
							}
							EndValue();
						}
						{
							BeginValue(HString("Resolution"));
							Float f = e.GetResolutionScale();
							if (DragFloat("", &f, 0.01f, 0.0f, 1.0f))
							{
								e.SetResolutionScale(f);
								m_bStage3DSettingsDirty = true;
							}
							EndValue();
						}

						Columns();

						TreePop();
					}

					TreePop();
				}

				TreePop();
			}
		}
	}
}

void DevUIViewUIExplorer::PoseTextEffectSettings()
{
	using namespace ImGui;

	if (CollapsingHeader("Text Effect Settings"))
	{
		auto const& tSettings = UI::Manager::Get()->GetTextEffectSettingsTable();

		// Save handling.
		if (Button("Save", ImVec2(0, 0), m_bTextEffectSettingsDirty))
		{
			FilePath const filePath(UI::Manager::Get()->GetTextEffectSettingsFilePath());

			Content::LoadManager::Get()->TempSuppressSpecificHotLoad(filePath);
			if (!Reflection::SaveObject(&tSettings, filePath))
			{
				SEOUL_WARN("Failed saving text effect settings. Check that \"%s\" is not read-only "
					"(checked out from source control).",
					filePath.GetAbsoluteFilenameInSource().CStr());
			}
			else
			{
				m_bTextEffectSettingsDirty = false;
			}
		}

		auto const iBegin = tSettings.Begin();
		auto const iEnd = tSettings.End();

		// Build a list for sorting.
		Vector<TextEffectEntry> vEntries;
		vEntries.Reserve(tSettings.GetSize());
		for (auto i = iBegin; iEnd != i; ++i)
		{
			vEntries.PushBack(TextEffectEntry(i->First, i->Second));
		}
		QuickSort(vEntries.Begin(), vEntries.End());

		// Now display all settings for editing.
		for (auto i = vEntries.Begin(); vEntries.End() != i; ++i)
		{
#if SEOUL_ENABLE_CHEATS
			UInt32 uUseCount = 0u;
			(void)LocManager::Get()->DebugGetFontEffectUseCount(i->m_Name, uUseCount);
			Byte const* sSuffix = (1u == uUseCount ? "use" : "uses");
			if (TreeNode(i->m_Name.CStr(), "%s (%u %s)", i->m_Name.CStr(), uUseCount, sSuffix))
#else
			if (TreeNode(i->m_Name.CStr()))
#endif
			{
				if (TreeNode("Color Defaults"))
				{
					Columns(2);

					auto& e = *i->m_pSettings;
					{
						BeginValue(HString("Color"));
						auto bEnableColor = (nullptr == e.m_pTextColorBottom && nullptr == e.m_pTextColorTop);
						auto b = bEnableColor && (nullptr != e.m_pTextColor);
						if (Checkbox("", &b, bEnableColor))
						{
							SafeDelete(e.m_pTextColor);
							if (b) { e.m_pTextColor = SEOUL_NEW(MemoryBudgets::Falcon) ColorARGBu8(ColorARGBu8::Black()); }
							m_bTextEffectSettingsDirty = true;
						}
						SameLine();
						if (b)
						{
							Color4 c(*e.m_pTextColor);
							if (ColorEdit4("", c.GetData(), true))
							{
								*e.m_pTextColor = c.ToColorARGBu8();
								m_bTextEffectSettingsDirty = true;
							}
						}
						EndValue();
					}

					{
						BeginValue(HString("ColorTop"));
						auto b = (nullptr != e.m_pTextColorTop);
						if (Checkbox("", &b))
						{
							SafeDelete(e.m_pTextColorTop);
							if (b) { e.m_pTextColorTop = SEOUL_NEW(MemoryBudgets::Falcon) ColorARGBu8(ColorARGBu8::Black()); }
							m_bTextEffectSettingsDirty = true;
						}
						SameLine();
						if (b)
						{
							Color4 c(*e.m_pTextColorTop);
							if (ColorEdit4("", c.GetData(), true))
							{
								*e.m_pTextColorTop = c.ToColorARGBu8();
								m_bTextEffectSettingsDirty = true;
							}
						}
						EndValue();
					}

					{
						BeginValue(HString("ColorBottom"));
						auto b = (nullptr != e.m_pTextColorBottom);
						if (Checkbox("", &b))
						{
							SafeDelete(e.m_pTextColorBottom);
							if (b) { e.m_pTextColorBottom = SEOUL_NEW(MemoryBudgets::Falcon) ColorARGBu8(ColorARGBu8::Black()); }
							m_bTextEffectSettingsDirty = true;
						}
						SameLine();
						if (b)
						{
							Color4 c(*e.m_pTextColorBottom);
							if (ColorEdit4("", c.GetData(), true))
							{
								*e.m_pTextColorBottom = c.ToColorARGBu8();
								m_bTextEffectSettingsDirty = true;
							}
						}
						EndValue();
					}

					Columns();

					TreePop();
				}

				if (TreeNode("Shadow"))
				{
					Columns(2);

					auto& e = *i->m_pSettings;
					BeginValue(HString("Enable"));
					if (Checkbox("", &e.m_bShadowEnable))
					{
						m_bTextEffectSettingsDirty = true;
					}
					EndValue();

					{
						BeginValue(HString("Blur"));
						Int iValue = (Int)e.m_uShadowBlur;
						if (DragInt("", &iValue, 1.0f, 0, 255))
						{
							e.m_uShadowBlur = (UInt8)iValue;
							m_bTextEffectSettingsDirty = true;
						}
						EndValue();
					}

					{
						BeginValue(HString("Color"));
						Color4 c(e.m_ShadowColor);
						if (ColorEdit4("", c.GetData(), true))
						{
							e.m_ShadowColor = c.ToColorARGBu8();
							m_bTextEffectSettingsDirty = true;
						}
						EndValue();
					}

					{
						BeginValue(HString("Offset"));
						if (DragFloat2("", e.m_vShadowOffset.GetData(), 1.0f, 0.0f, 128.0f))
						{
							m_bTextEffectSettingsDirty = true;
						}
						EndValue();
					}

					{
						BeginValue(HString("Outline Width"));
						Int iValue = (Int)e.m_uShadowOutlineWidth;
						if (DragInt("", &iValue, 1.0f, 0, 128))
						{
							e.m_uShadowOutlineWidth = (UInt8)iValue;
							m_bTextEffectSettingsDirty = true;
						}
						EndValue();
					}

					Columns();

					TreePop();
				}

				if (TreeNode("Extra Outline"))
				{
					Columns(2);

					auto& e = *i->m_pSettings;
					BeginValue(HString("Enable"));
					if (Checkbox("", &e.m_bExtraOutlineEnable))
					{
						m_bTextEffectSettingsDirty = true;
					}
					EndValue();

					{
						BeginValue(HString("Blur"));
						Int iValue = (Int)e.m_uExtraOutlineBlur;
						if (DragInt("", &iValue, 1.0f, 0, 255))
						{
							e.m_uExtraOutlineBlur = (UInt8)iValue;
							m_bTextEffectSettingsDirty = true;
						}
						EndValue();
					}

					{
						BeginValue(HString("Color"));
						Color4 c(e.m_ExtraOutlineColor);
						if (ColorEdit4("", c.GetData(), true))
						{
							e.m_ExtraOutlineColor = c.ToColorARGBu8();
							m_bTextEffectSettingsDirty = true;
						}
						EndValue();
					}

					{
						BeginValue(HString("Offset"));
						if (DragFloat2("", e.m_vExtraOutlineOffset.GetData(), 1.0f, 0.0f, 128.0f))
						{
							m_bTextEffectSettingsDirty = true;
						}
						EndValue();
					}

					{
						BeginValue(HString("Outline Width"));
						Int iValue = (Int)e.m_uExtraOutlineWidth;
						if (DragInt("", &iValue, 1.0f, 0, 128))
						{
							e.m_uExtraOutlineWidth = (UInt8)iValue;
							m_bTextEffectSettingsDirty = true;
						}
						EndValue();
					}

					Columns();

					TreePop();
				}

				if (TreeNode("Detail"))
				{
					Columns(2);

					auto& e = *i->m_pSettings;

					{
						BeginValue(HString("Enable"));
						if (Checkbox("", &e.m_bDetail))
						{
							m_bTextEffectSettingsDirty = true;
						}
						EndValue();
					}

					{
						BeginValue(HString("Mode"));
						if (SelectEnum(e.m_eDetailMode))
						{
							m_bTextEffectSettingsDirty = true;
						}
						EndValue();
					}

					{
						BeginValue(HString("Stretch Mode"));
						if (SelectEnum(e.m_eDetailStretchMode))
						{
							m_bTextEffectSettingsDirty = true;
						}
						EndValue();
					}

					{
						BeginValue(HString("Offset"));
						if (DragFloat2("", e.m_vDetailOffset.GetData(), 0.01f, -1.0f, 1.0f))
						{
							m_bTextEffectSettingsDirty = true;
						}
						EndValue();
					}

					{
						BeginValue(HString("Speed"));
						if (DragFloat2("", e.m_vDetailSpeed.GetData(), 1.0f, -1000.0f, 1000.0f))
						{
							m_bTextEffectSettingsDirty = true;
						}
						EndValue();
					}

					{
						BeginValue(HString("FilePath"));
						if (SelectFilePath(GameDirectory::kContent, FileType::kTexture0, e.m_DetailFilePath))
						{
							m_bTextEffectSettingsDirty = true;
						}
						EndValue();
					}

					Columns();

					TreePop();
				}

				TreePop();
			}
		}
	}
}

/** Set a selection and capture view projection properties for drawing its selection rectangle. */
void DevUIViewUIExplorer::UpdateSelected(
	UI::Movie const* pMovie,
	const SharedPtr<Falcon::Instance>& pInstance)
{
	DevUIViewGameUI::Get()->HighlightSelect(pMovie->GetHandle(), pInstance);
}

} // namespace Game

} // namespace Seoul

#endif // /(SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)
