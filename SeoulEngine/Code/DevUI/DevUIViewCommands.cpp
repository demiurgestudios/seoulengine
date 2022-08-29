/**
 * \file DevUIViewCommands.cpp
 * \brief In-game cheat/developer command support. Used
 * to run cheats or otherwise interact with the running simulation.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIImGui.h"
#include "DevUIRoot.h"
#include "DevUIUtil.h"
#include "DevUIViewCommands.h"
#include "InputManager.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "ReflectionRegistry.h"

#if (SEOUL_ENABLE_DEV_UI && SEOUL_ENABLE_CHEATS)

namespace Seoul
{

SEOUL_BEGIN_TYPE(DevUI::ViewCommands, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(DisplayName, "Commands")
	SEOUL_PARENT(DevUI::View)
SEOUL_END_TYPE()

namespace DevUI
{

static const HString kMiscellaneousCategory("Miscellaneous");

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

static inline Bool PoseEnum(const Reflection::TypeInfo& info, Reflection::Any& r)
{
	using namespace ImGui;

	auto const& e = *info.GetType().TryGetEnum();
	auto const& vValues = e.GetValues();

	// Get the current enum value.
	if (!r.IsOfType<Int>())
	{
		r = (Int)0;
	}
	Int const iEnumValue = r.Cast<Int>();

	// Find the offset into the values array for the value.
	Int iCurrent = 0;
	{
		auto i = vValues.Find(iEnumValue);
		if (vValues.End() != i)
		{
			iCurrent = (Int)(i - vValues.Begin());
		}
	}

	// Present and update.
	Bool bReturn = false;
	PushID((void*)&r);
	if (Combo("", &iCurrent, PoseEnumGetter, (void*)&e, (Int)e.GetNames().GetSize(), (Int)e.GetNames().GetSize()))
	{
		if (iCurrent >= 0 && iCurrent < (Int32)vValues.GetSize())
		{
			r = vValues[iCurrent];
			bReturn = true;
		}
	}
	PopID();
	return bReturn;
}

static Bool PoseEnumLikeGetter(void* pData, Int iIndex, Byte const** psOut)
{
	auto const& v = *((Reflection::Attributes::EnumLike::Names const*)pData);

	if (iIndex >= 0 && (UInt32)iIndex < v.GetSize())
	{
		*psOut = v[iIndex].CStr();
		return true;
	}

	return false;
}

static inline Bool PoseEnumLike(const Reflection::TypeInfo& info, const Reflection::Attributes::EnumLike& enumLike, Reflection::Any& r)
{
	using namespace ImGui;

	Reflection::Attributes::EnumLike::Names v;
	enumLike.GetNames(v);

	// Get the current name.
	HString currentName;
	enumLike.ValueToName(r, currentName);

	if (currentName.IsEmpty() && !v.IsEmpty())
	{
		currentName = v.Front();
		enumLike.NameToValue(currentName, r);
	}

	// Find the offset into the values array for the value.
	Int iCurrent = 0;
	{
		auto i = v.Find(currentName);
		if (v.End() != i)
		{
			iCurrent = (Int)(i - v.Begin());
		}
		else if (!v.IsEmpty())
		{
			currentName = v.Front();
			enumLike.NameToValue(currentName, r);
			iCurrent = 0;
		}
	}

	// Present and update.
	Bool bReturn = false;
	PushID((void*)&r);
	if (Combo("", &iCurrent, PoseEnumLikeGetter, (void*)&v, (Int)v.GetSize(), (Int)v.GetSize()))
	{
		if (iCurrent >= 0 && iCurrent < (Int32)v.GetSize())
		{
			enumLike.NameToValue(v[iCurrent], r);
			bReturn = true;
		}
	}
	PopID();
	return bReturn;
}

static inline Bool PoseBool(const Reflection::TypeInfo& info, Reflection::Any& r)
{
	using namespace ImGui;

	if (!r.IsOfType<Bool>())
	{
		r = (Bool)false;
	}
	Bool bCurrent = r.Cast<Bool>();

	Bool bReturn = false;
	PushID((void*)&r);
	if (Checkbox("", &bCurrent))
	{
		r = bCurrent;
		bReturn = true;
	}
	PopID();
	return bReturn;
}

static inline Bool PoseVector2D(const Reflection::TypeInfo& info, Reflection::Any& r)
{
	using namespace ImGui;

	if (!r.IsOfType<Vector2D>())
	{
		r = Vector2D();
	}
	Vector2D vCurrent = r.Cast<Vector2D>();

	Bool bReturn = false;
	PushID((void*)&r);
	if (InputFloat2("", vCurrent.GetData()))
	{
		r = vCurrent;
		bReturn = true;
	}
	PopID();
	return bReturn;
}

static inline Bool PoseComplex(const Reflection::TypeInfo& info, Reflection::Any& r)
{
	using namespace ImGui;

	// return value is if it was modified
	if (info == TypeId<Vector2D>())
	{
		return PoseVector2D(info, r);
	}
	else
	{
		return false;
	}
}

static inline Bool PoseFloat(const Reflection::TypeInfo& info, Reflection::Any& r, Reflection::Attributes::Range const* pRange, Float maxWidth = 0)
{
	using namespace ImGui;

	if (maxWidth > 0)
	{
		PushItemWidth(maxWidth);
	}

	if (!r.IsOfType<Float>())
	{
		r = (Float)0.0f;
	}
	Float fCurrent = r.Cast<Float>();

	Bool bReturn = false;
	PushID((void*)&r);
	if (nullptr != pRange)
	{
		Float const fMin = pRange->m_Min.Cast<Float>();
		Float const fMax = pRange->m_Max.Cast<Float>();
		Float const fStep = Abs(fMax - fMin) / 100.0f;
		if (DragFloat("", &fCurrent, fStep, fMin, fMax))
		{
			r = fCurrent;
			bReturn = true;
		}
	}
	else
	{
		if (InputFloat("", &fCurrent))
		{
			r = fCurrent;
			bReturn = true;
		}
	}
	PopID();

	if (maxWidth > 0)
	{
		PopItemWidth();
	}

	return bReturn;
}

template <typename T>
static inline Bool PoseInt(const Reflection::TypeInfo& info, Reflection::Any& r, Reflection::Attributes::Range const* pRange, Float maxWidth = 0)
{
	using namespace ImGui;

	if (maxWidth > 0)
	{
		PushItemWidth(maxWidth);
	}

	if (!r.IsOfType<T>())
	{
		r = (T)0;
	}
	Int iCurrent = (Int)r.Cast<T>();

	Bool bReturn = false;
	PushID((void*)&r);
	if (nullptr != pRange)
	{
		Int const iMin = pRange->m_Min.Cast<Int>();
		Int const iMax = pRange->m_Max.Cast<Int>();
		Float const fStep = Abs(iMax - iMin) / 100.0f;
		if (DragInt("", &iCurrent, fStep, iMin, iMax))
		{
			r = (T)iCurrent;
			bReturn = true;
		}
	}
	else
	{
		if (InputInt("", &iCurrent))
		{
			r = (T)iCurrent;
			bReturn = true;
		}
	}
	PopID();

	if (maxWidth > 0)
	{
		PopItemWidth();
	}
	return bReturn;
}

static inline Bool PoseString(const Reflection::TypeInfo& info, Reflection::Any& r)
{
	using namespace ImGui;

	static const UInt32 kuOversize = 64u;

	if (!r.IsOfType<String>())
	{
		r = String();
	}
	String sCurrent = r.Cast<String>();

	Bool bReturn = false;
	PushID((void*)&r);

	Vector<Byte, MemoryBudgets::DevUI> vBuffer(sCurrent.GetSize() + kuOversize, (Byte)0);
	memcpy(vBuffer.Data(), sCurrent.CStr(), sCurrent.GetSize());
	vBuffer[sCurrent.GetSize()] = '\0';
	if (InputText("", vBuffer.Data(), vBuffer.GetSize(), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		bReturn = true;
	}
	r = String(vBuffer.Data());
	PopID();
	return bReturn;
}

template <typename T>
static inline Bool PoseUInt(const Reflection::TypeInfo& info, Reflection::Any& r, Reflection::Attributes::Range const* pRange, Float maxWidth = 0)
{
	using namespace ImGui;

	if (maxWidth > 0)
	{
		PushItemWidth(maxWidth);
	}

	if (!r.IsOfType<T>())
	{
		r = (T)0;
	}
	UInt uCurrent = (UInt)r.Cast<T>();

	Bool bReturn = false;
	PushID((void*)&r);
	if (nullptr != pRange)
	{
		UInt const uMin = pRange->m_Min.Cast<UInt>();
		UInt const uMax = pRange->m_Max.Cast<UInt>();
		Float const fStep = Abs((Int)(uMax - uMin)) / 100.0f;
		if (DragUInt("", &uCurrent, fStep, uMin, uMax))
		{
			r = (T)uCurrent;
			bReturn = true;
		}
	}
	else
	{
		if (InputUInt("", &uCurrent))
		{
			r = (T)uCurrent;
			bReturn = true;
		}
	}
	PopID();

	if (maxWidth > 0)
	{
		PopItemWidth();
	}
	return bReturn;
}

ViewCommands::ViewCommands()
	: m_vCommands()
	, m_vInstances()
	, m_PendingBinding()
{
	// Setup commands.
	GatherCommands();
}

ViewCommands::~ViewCommands()
{
	DestroyCommands();
}

Bool ViewCommands::OnKeyPressed(InputButton eButton, UInt32 uModifiers)
{
	if (!m_PendingBinding.IsEmpty())
	{
		InputManager::Get()->OverrideButtonForBinding(
			m_PendingBinding,
			eButton,
			uModifiers);
		m_PendingBinding = HString();
		return true;
	}

	return false;
}

void ViewCommands::DoPrePose(Controller& rController, RenderPass& rPass)
{
	static const Float kfMaxNumberBoxWidth = 107.0f;
	static const Float kfMaxItemWidth = 190.0f;
#if SEOUL_PLATFORM_WINDOWS
	static const Float kfExtraWideMaxItemWidth = 490.0f;
#endif
	static const Float kfHotkeyWidth = 120.0f;

	using namespace ImGui;
	using namespace Reflection;

	// Don't display HotKey hints on mobile.
#if SEOUL_DEVUI_MOBILE
	Bool const bHotKeys = false;
#else
	Bool const bHotKeys = true;
#endif

	// Initialize the category to the first entry. Commands should
	// be sorted by category, then by name.
	String sCategory;
	if (!m_vCommands.IsEmpty())
	{
		sCategory = m_vCommands.Front().m_sCategory;
	}

	// Track whether we drawing entries in this category.
	Bool bDrawing = CollapsingHeader(sCategory.CStr());
	Int iCount = 0;
	for (auto i = m_vCommands.Begin(); m_vCommands.End() != i; ++i)
	{
		// Check if we need to switch categories.
		if (i->m_sCategory != sCategory)
		{
			sCategory = i->m_sCategory;
			bDrawing = CollapsingHeader(sCategory.CStr());
			iCount = 0;
		}

		// Check if the command is disabled - disable all actions
		// and display a different tooltip.
		auto const disabled = (i->m_IsDisabledFunc ? i->m_IsDisabledFunc() : HString());

		// If we're drawing, display the command.
		if (bDrawing)
		{
			// We draw a horizontal separated between each command.
			if (0 != iCount)
			{
				Separator();
			}
			++iCount;

			// Cache data necessary to display and activate the command.
			auto pMethod = i->m_pMethod;
			auto const& typeInfo = pMethod->GetTypeInfo();
			UInt32 const uArguments = typeInfo.m_uArgumentCount;
			auto& aArguments = i->m_aArguments;

			// Whether we need to display an activate button or not (vs. activating on value changes).
			// By default, we only display the button if a method has 0 arguments, but the button
			// activation can be opt-in for some special cases (e.g. activation is expensive and
			// it would be a bad user experience to activate on each argument value change).
			Bool const bUseButton = (
				uArguments == 0u || pMethod->GetAttributes().HasAttribute<Attributes::CommandNeedsButton>());

			Float fMaxItemWidth = kfMaxItemWidth;
#if SEOUL_PLATFORM_WINDOWS
			// On PC only, allow extra wide input
			if (pMethod->GetAttributes().HasAttribute<Attributes::ExtraWideInput>())
			{
				fMaxItemWidth = kfExtraWideMaxItemWidth;
			}
#endif
			// Help imgui format the argument + button area.
			Float const fItemWidth = Min(
				GetContentRegionAvail().x / (Float)(uArguments) + (bUseButton ? 1 : 0),
				fMaxItemWidth
			);
			// Help imgui format the argument + button area.
			Float const fIntBoxItemWidth = Min(
				GetContentRegionAvail().x / (Float)(uArguments),
				kfMaxNumberBoxWidth
			);

			if (bHotKeys)
			{
				// Two columns - column 0 is arguments+optional button, column 1 is the hotkey for the command.
				Columns(2, nullptr, false);
			}

			// arguments+optional button group.
			BeginGroup();
			PushItemWidth(fItemWidth);

			// Enumerate arguments and display each.
			Bool bClickedButton = false;

			// If no arguments, or if required, only activate on a button press.
			if (bUseButton)
			{
				bClickedButton = Button(i->m_sName.CStr(), ImVec2(0, 0), disabled.IsEmpty()); SameLine();
			}

			// Enumerate arguments and display each.
			Bool bActivate = false;

			if (!disabled.IsEmpty()) { PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]); }
			for (UInt32 uArg = 0u; uArg < uArguments; ++uArg)
			{
				auto& arg = aArguments[uArg];
				auto const& argTypeInfo = typeInfo.GetArgumentTypeInfo(uArg);

				auto pEnumLike = pMethod->GetAttributes().GetAttribute<Attributes::EnumLike>((Int)uArg);
				auto pGetCurrentValue = pMethod->GetAttributes().GetAttribute<Attributes::GetCurrentValue>((Int)uArg);
				auto pRange = pMethod->GetAttributes().GetAttribute<Attributes::Range>((Int)uArg);
				auto pEditorHide = pMethod->GetAttributes().GetAttribute<Attributes::EditorHide>((Int)uArg);

				if (nullptr != pEditorHide)
				{
					continue;
				}

				// Refresh the value if we have a getter.
				if (nullptr != pGetCurrentValue)
				{
					arg = pGetCurrentValue->Get();
				}

				if (nullptr != pEnumLike)
				{
					bActivate = PoseEnumLike(argTypeInfo, *pEnumLike, arg) || bActivate; SameLine();
				}
				else
				{
					switch (argTypeInfo.GetSimpleTypeInfo())
					{
					case SimpleTypeInfo::kBoolean: bActivate = PoseBool(argTypeInfo, arg) || bActivate; SameLine(); break;
					case SimpleTypeInfo::kEnum: bActivate = PoseEnum(argTypeInfo, arg) || bActivate; SameLine(); break;
					case SimpleTypeInfo::kFloat32: bActivate = PoseFloat(argTypeInfo, arg, pRange, fIntBoxItemWidth) || bActivate; SameLine(); break;
					case SimpleTypeInfo::kHString: bActivate = PoseString(argTypeInfo, arg) || bActivate; SameLine(); break;
					case SimpleTypeInfo::kInt8: bActivate = PoseInt<Int8>(argTypeInfo, arg, pRange, fIntBoxItemWidth) || bActivate; SameLine(); break;
					case SimpleTypeInfo::kInt16: bActivate = PoseInt<Int16>(argTypeInfo, arg, pRange, fIntBoxItemWidth) || bActivate; SameLine(); break;
					case SimpleTypeInfo::kInt32: bActivate = PoseInt<Int32>(argTypeInfo, arg, pRange, fIntBoxItemWidth) || bActivate; SameLine(); break;
					case SimpleTypeInfo::kInt64: bActivate = PoseInt<Int64>(argTypeInfo, arg, pRange, fIntBoxItemWidth) || bActivate; SameLine(); break;
					case SimpleTypeInfo::kString: bActivate = PoseString(argTypeInfo, arg) || bActivate; SameLine(); break;
					case SimpleTypeInfo::kUInt8: bActivate = PoseUInt<UInt8>(argTypeInfo, arg, pRange, fIntBoxItemWidth) || bActivate; SameLine(); break;
					case SimpleTypeInfo::kUInt16: bActivate = PoseUInt<UInt16>(argTypeInfo, arg, pRange, fIntBoxItemWidth) || bActivate; SameLine(); break;
					case SimpleTypeInfo::kUInt32: bActivate = PoseUInt<UInt32>(argTypeInfo, arg, pRange, fIntBoxItemWidth) || bActivate; SameLine(); break;
					case SimpleTypeInfo::kUInt64: bActivate = PoseUInt<UInt64>(argTypeInfo, arg, pRange, fIntBoxItemWidth) || bActivate; SameLine(); break;
					case SimpleTypeInfo::kComplex: bActivate = PoseComplex(argTypeInfo, arg) || bActivate; SameLine(); break;
					default:
						SEOUL_WARN("For command '%s', argument '%u' has unsupported type '%s'",
							pMethod->GetName().CStr(),
							uArg,
							argTypeInfo.GetType().GetName().CStr());
						break;
					};
				}
			}

			// If not using a button, we need to label the method (otherwise the label is on the button).
			if (!bUseButton)
			{
				Text("%s", i->m_sName.CStr());
			}

			if (!disabled.IsEmpty()) { PopStyleColor(); }

			// Done with the arguments+optional button section.
			PopItemWidth();
			EndGroup();

			// if we're using a button, don't let the other widgets activate the cheat.
			if (bUseButton)
			{
				bActivate = false;
			}

			// Now at the point of check, filter activation if disabled.
			if (!disabled.IsEmpty())
			{
				bActivate = false;
			}

			// Activate the method now if requested.
			if (bClickedButton || bActivate)
			{
				LogCommand(*i);
				if (!pMethod->TryInvoke(i->m_Instance, i->m_aArguments))
				{
					SEOUL_WARN("Failed command \"%s\" invocation, ping an engineer.", i->m_sName.CStr());
				}
			}

			// Tooltips only on non-mobile platforms.
			if (!Root::Get()->IsMobile())
			{
				// Tooltip for the method - use the method's description.
				if (IsItemHovered() && nullptr != i->m_pDescription)
				{
					if (!disabled.IsEmpty())
					{
						SetTooltip("%s", disabled.CStr());
					}
					else
					{
						SetTooltip("%s", i->m_pDescription->m_DescriptionText.CStr());
					}
				}
			}

			if (bHotKeys)
			{
				// Setup formatting for the hotkey portion of the command info. This is fixed with,
				// unless the overall width is less than the fixed, at which point it becomes preportional.
				if (GetColumnMaxX() - GetColumnMinX() > 2.0f * kfHotkeyWidth)
				{
					SetColumnOffset(GetColumnIndex() + 1, GetColumnMaxX() - kfHotkeyWidth);
				}

				NextColumn();

				// Hotkey handling - if a hot key is already assigned, display the
				// hotkey's name with an invisible button to enable modification of the hotkey.
				auto const sHotKey(InputManager::Get()->BindingToString(i->m_pMethod->GetName()));
				if (!sHotKey.IsEmpty())
				{
					if (Button(String::Printf("%s##%s", sHotKey.CStr(), pMethod->GetName().CStr()).CStr()))
					{
						m_PendingBinding = pMethod->GetName();
						OpenPopup("Hotkey Config");
					}
				}
				// Otherwise, display a "Set Hotkey" area to allow definition of the hotkey.
				else if (Button(String::Printf("Set Hotkey##%s", i->m_pMethod->GetName().CStr()).CStr()))
				{
					m_PendingBinding = pMethod->GetName();
					OpenPopup("Hotkey Config");
				}

				Columns();
			}
		}
	}

	// Handle the modal "you are setting a hotkey" dialogue, if enabled.
	PoseHotkeyDialogue();
}

void ViewCommands::DoTick(Controller& rController, Float fDeltaTimeInSeconds)
{
	// Check for hotkey activation of commands.
	for (auto i = m_vCommands.Begin(); m_vCommands.End() != i; ++i)
	{
		auto const name = i->m_pMethod->GetName();
		if (InputManager::Get()->WasBindingPressed(name))
		{
			LogCommand(*i);
			if (!i->m_pMethod->TryInvoke(i->m_Instance, i->m_aArguments))
			{
				SEOUL_WARN("Failed command \"%s\" invocation, ping an engineer.", i->m_sName.CStr());
			}
			// On activation, display a notification about the executed command.
			else
			{
				Root::Get()->DisplayNotification(String::Printf("Cmd: %s", i->m_sName.CStr()));
			}
		}
	}
}

UInt32 ViewCommands::GetFlags() const
{
	return 0u;
}

void ViewCommands::LogCommand(const CommandEntry& entry)
{
	String sMessage = String::Printf("Cmd: %s", entry.m_sName.CStr());
	for (UInt uIndex = 0; uIndex < entry.m_pMethod->GetTypeInfo().m_uArgumentCount; uIndex++)
	{
		sMessage += " ";

		String sArg;
		HString hArg;

		auto arg = entry.m_aArguments[uIndex];
		auto argPtr = arg.GetPointerToObject();
		auto const& argTypeInfo = entry.m_pMethod->GetTypeInfo().GetArgumentTypeInfo(uIndex);

		if (argTypeInfo.GetSimpleTypeInfo() == Reflection::SimpleTypeInfo::kEnum
			&& argTypeInfo.GetType().TryGetEnum()->TryGetName(arg, hArg))
		{
			sMessage += String(hArg);
		}
		else
		{
			SerializeToString(argPtr, sArg);
		}
		sMessage += sArg;
	}

	SEOUL_LOG("%s", sMessage.CStr());
}

void ViewCommands::DestroyCommands()
{
	// Clear commands.
	m_vCommands.Clear();

	// Destroy all CommandLine instances.
	Int32 const iCount = (Int32)m_vInstances.GetSize();
	for (Int32 i = iCount - 1; i >= 0; --i)
	{
		m_vInstances[i].GetType().Delete(m_vInstances[i]);
	}
	m_vInstances.Clear();
}

void ViewCommands::GatherCommands()
{
	// Cleanup any existing.
	DestroyCommands();

	// Create an instance of all types that have the CommandLineInstance attribute.
	using namespace Reflection;
	UInt32 const uTypes = Registry::GetRegistry().GetTypeCount();
	for (UInt32 i = 0u; i < uTypes; ++i)
	{
		const auto& type = *Registry::GetRegistry().GetType(i);
		if (type.HasAttribute<Attributes::CommandsInstance>())
		{
			WeakAny instance = type.New(MemoryBudgets::Developer);
			if (!instance.IsValid())
			{
				SEOUL_WARN("Could not instantiate an instance of %s, commands from this "
					"class will not be available on the command line.\n",
					type.GetName().CStr());

				continue;
			}

			m_vInstances.PushBack(instance);
		}
	}

	// Now assemble commands.
	for (auto i = m_vInstances.Begin(); m_vInstances.End() != i; ++i)
	{
		const auto& type = i->GetType();
		UInt32 const uMethods = type.GetMethodCount();
		for (UInt32 j = 0u; j < uMethods; ++j)
		{
			auto pMethod = type.GetMethod(j);
			auto const& typeInfo = pMethod->GetTypeInfo();
			UInt32 const uArguments = typeInfo.m_uArgumentCount;

			HString category(kMiscellaneousCategory);
			auto pCategory = pMethod->GetAttributes().GetAttribute<Attributes::Category>();
			if (nullptr != pCategory)
			{
				category = pCategory->m_CategoryName;
			}

			HString name = pMethod->GetName();
			auto pDisplayName = pMethod->GetAttributes().GetAttribute<Attributes::DisplayName>();
			if (nullptr != pDisplayName)
			{
				name = pDisplayName->m_DisplayName;
			}

			CommandEntry entry;
			entry.m_Instance = *i;
			entry.m_pMethod = pMethod;
			entry.m_pType = &type;
			entry.m_pDescription = pMethod->GetAttributes().GetAttribute<Attributes::Description>();
			entry.m_sCategory = String(category);
			entry.m_sName = String(name);
			{
				auto p = pMethod->GetAttributes().GetAttribute<Attributes::CommandIsDisabled>();
				if (p)
				{
					entry.m_IsDisabledFunc = p->m_IsDisabledFunc;
				}
			}

			Bool bAllDefault = true;
			for (UInt32 uArg = 0u; uArg < uArguments; ++uArg)
			{
				auto pDefaulValue = pMethod->GetAttributes().GetAttribute<Attributes::DefaultValue>((Int)uArg);
				if (nullptr != pDefaulValue)
				{
					entry.m_aArguments[uArg] = pDefaulValue->m_DefaultValue;
				}
				else
				{
					bAllDefault = false;
				}
			}

			if(uArguments > 0 && bAllDefault)
			{
				LogCommand(entry);
				entry.m_pMethod->TryInvoke(entry.m_Instance, entry.m_aArguments);
			}

			m_vCommands.PushBack(entry);
		}
	}

	// Sort lexographically.
	QuickSort(m_vCommands.Begin(), m_vCommands.End());
}

void ViewCommands::PoseHotkeyDialogue()
{
	using namespace ImGui;

	// If in use, this is handling for the modal "you are setting a hotkey" dialogue.
	if (BeginPopupModalEx("Hotkey Config", GetWindowCenter(), nullptr, ImGuiWindowFlags_NoResize))
	{
		// Handle the case where the popup has been closed (pending binding has been closed).
		if (m_PendingBinding.IsEmpty())
		{
			goto close_popup;
		}

		Text("Type new hotkey, or...");
		Spacing();

		// Special cases - immediately cancel handling, or clear the hotkey definition.
		if (Button("Cancel"))
		{
			goto close_popup;
		}

		SameLine();
		if (Button("Clear"))
		{
			InputManager::Get()->ClearButtonForBinding(m_PendingBinding);
			goto close_popup;
		}

		EndPopup();
		return;

close_popup:
		m_PendingBinding = HString();
		CloseCurrentPopup();
		EndPopup();
		return;
	}
}

} // namespace DevUI

} // namespace Seoul

#endif // /(SEOUL_ENABLE_DEV_UI && SEOUL_ENABLE_CHEATS)
