/**
 * \file UICommands.cpp
 * \brief Cheat commands for UI functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconRenderDrawer.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "JobsFunction.h"
#include "LocManager.h"
#include "Path.h"
#include "ReflectionDefine.h"
#include "UIRenderer.h"
#include "UIManager.h"
#include "UIUtil.h"

namespace Seoul
{

#if SEOUL_ENABLE_CHEATS

namespace Reflection
{

namespace Attributes
{

struct UICommandsHStringSort SEOUL_SEALED
{
	Bool operator()(HString a, HString b) const
	{
		return (strcmp(a.CStr(), b.CStr()) < 0);
	}
}; // struct UICommandsHStringSort

/**
 * Specialization of EnumLike, used on SetLanguage
 * to get the set of supported languages.
 */
class SetLanguageEnumLike SEOUL_SEALED : public EnumLike
{
public:
	virtual void GetNames(Names& rvNames) const SEOUL_OVERRIDE
	{
		LocManager::SupportedLanguages v;
		LocManager::Get()->GetSupportedLanguages(v);
		QuickSort(v.Begin(), v.End());

		rvNames.Clear();
		rvNames.Reserve(v.GetSize());
		for (auto i = v.Begin(); v.End() != i; ++i)
		{
			rvNames.PushBack(HString(*i));
		}
	}

	virtual void NameToValue(HString name, Reflection::Any& rValue) const SEOUL_OVERRIDE
	{
		rValue = name;
	}

	virtual void ValueToName(const Reflection::Any& value, HString& rName) const SEOUL_OVERRIDE
	{
		if (value.IsOfType<HString>())
		{
			rName = value.Cast<HString>();
		}
		else
		{
			rName = HString();
		}
	}
}; // class SetLanguageEnumLike

/**
 * Specialization of EnumLike, used on TriggerTransition
 * to get the set of trigger names.
 */
class UITriggersEnumLike SEOUL_SEALED : public EnumLike
{
public:
	virtual void GetNames(Names& rvNames) const SEOUL_OVERRIDE
	{
		HashSet<HString, MemoryBudgets::StateMachine> set;
		auto const& vStack = UI::Manager::Get()->GetStack();
		for (auto const& e : vStack)
		{
			e.m_pMachine->GetViableTriggerNames(set);
		}

		rvNames.Clear();
		{
			for (auto const& e : set)
			{
				rvNames.PushBack(e);
			}
		}

		UICommandsHStringSort sorter;
		QuickSort(rvNames.Begin(), rvNames.End(), sorter);
	}

	virtual void NameToValue(HString name, Reflection::Any& rValue) const SEOUL_OVERRIDE
	{
		rValue = name;
	}

	virtual void ValueToName(const Reflection::Any& value, HString& rName) const SEOUL_OVERRIDE
	{
		if (value.IsOfType<HString>())
		{
			rName = value.Cast<HString>();
		}
		else
		{
			rName = HString();
		}
	}
}; // class UITriggersEnumLike

} // namespace Attributes

} // namespace Reflection

#if SEOUL_PLATFORM_WINDOWS
namespace
{

/**
 * Developer only and PC only - copy .jsfl files
 * from their source control location into
 * the current Flash folder, if needed.
 */
void CopyJsflScripts()
{
	auto const sSource(Path::Combine(
		GamePaths::Get()->GetBaseDir(),
		"Tools",
		"Flash"));

	size_t zRequired = 0;
	wchar_t aBuffer[256] = L"";
	if (0 != _wgetenv_s(&zRequired, aBuffer, SEOUL_ARRAY_COUNT(aBuffer), L"USERPROFILE"))
	{
		return;
	}

	// Get files to copy.
	Vector<String> vs;
	if (!FileManager::Get()->GetDirectoryListing(sSource, vs, false, false, ".jsfl"))
	{
		return;
	}

	for (auto const& sTarget : {
		R"(AppData\Local\Adobe\Animate CC 2019\en_US\Configuration\Commands)",
		})
	{
		// Target directory.
		auto const sTargetDir(Path::Combine(WCharTToUTF8(aBuffer), sTarget));

		// Early out if no target dir.
		if (!FileManager::Get()->IsDirectory(sTargetDir))
		{
			continue;
		}

		for (auto const& s : vs)
		{
			auto const sOut(Path::Combine(sTargetDir, Path::GetFileName(s)));

			// Early out if no change.
			auto const uIn = FileManager::Get()->GetModifiedTime(s);
			if (uIn == FileManager::Get()->GetModifiedTime(sOut))
			{
				continue;
			}

			String sBody;
			if (!FileManager::Get()->ReadAll(s, sBody))
			{
				continue;
			}

			if (!FileManager::Get()->WriteAll(
				sOut,
				sBody.CStr(),
				sBody.GetSize()))
			{
				continue;
			}

			(void)FileManager::Get()->SetModifiedTime(sOut, uIn);
		}
	}
}

} // namespace anonymous
#endif // /#if SEOUL_PLATFORM_WINDOWS

namespace UI
{

/**
 * Cheat commands for UI functionality.
 */
class Commands SEOUL_SEALED
{
public:
	Commands()
	{
		// In developer builds on PC, we use the creation
		// of this commands utility as an opportunity to kick a job
		// that will check and merge some custom .jsfl scripts
		// that we've written for Flash.
#if SEOUL_PLATFORM_WINDOWS
		Jobs::AsyncFunction(&CopyJsflScripts);
#endif // /#if SEOUL_PLATFORM_WINDOWS
	}

	~Commands()
	{
	}

	void ToggleNetworkOverlay()
	{
		static const HString ksToggleNetworkOverlay("HANDLER_CheatToggleNetworkOverlay");
		Manager::Get()->BroadcastEventTo(HString(), ksToggleNetworkOverlay);
	}

	void ToggleVisualizeClickInput()
	{
		UInt8 const uMask = (Falcon::kuClickMouseInputHitTest);
		if (0u == (uMask & Manager::Get()->GetInputVisualizationMode()))
		{
			Manager::Get()->SetInputVisualizationMode(
				Manager::Get()->GetInputVisualizationMode() | uMask);
		}
		else
		{
			Manager::Get()->SetInputVisualizationMode(
				Manager::Get()->GetInputVisualizationMode() & ~uMask);
		}
	}

	void ToggleVisualizeDragInput()
	{
		UInt8 const uMask = (Falcon::kuDragMouseInputHitTest);
		if (0u == (uMask & Manager::Get()->GetInputVisualizationMode()))
		{
			Manager::Get()->SetInputVisualizationMode(
				Manager::Get()->GetInputVisualizationMode() | uMask);
		}
		else
		{
			Manager::Get()->SetInputVisualizationMode(
				Manager::Get()->GetInputVisualizationMode() & ~uMask);
		}
	}

	void TriggerTransition(const String& sTriggerName)
	{
		HString triggerName;
		if (HString::Get(triggerName, sTriggerName))
		{
			Manager::Get()->TriggerTransition(triggerName);
		}
	}

	void SetBatchOptimizer(Bool bEnable)
	{
		auto& r = Manager::Get()->GetRenderer();
		r.SetDebugEnableBatchOptimizer(bEnable);
	}

	void SetOcclusionOptimizer(Bool bEnable)
	{
		auto& r = Manager::Get()->GetRenderer();
		r.SetDebugEnableOcclusionOptimizer(bEnable);
	}

	void SetOverfillOptimizer(Bool bEnable)
	{
		auto& r = Manager::Get()->GetRenderer();
		r.SetDebugEnableOverfillOptimizer(bEnable);
	}

	void SetLanguage(const String& s)
	{
		if (LocManager::Get()->GetCurrentLanguage() != s)
		{
			LocManager::Get()->DebugSetLanguage(s);
#if SEOUL_HOT_LOADING
			Manager::Get()->HotReload();
#endif // /#if SEOUL_HOT_LOADING
		}
	}

	void SetLanguagePlatform(Platform ePlatform)
	{
		if (ePlatform != LocManager::Get()->DebugPlatform())
		{
			LocManager::Get()->DebugSetPlatform(ePlatform);
#if SEOUL_HOT_LOADING
			Manager::Get()->HotReload();
#endif // /#if SEOUL_HOT_LOADING
		}
	}

	void ToggleDontUseFallbackLanguage()
	{
		LocManager::Get()->ToggleDontUseFallbackLanguage();
	}

	void SetAspectRatioGuide(FixedAspectRatio::Enum eMode)
	{
		Manager::Get()->SetFixedAspectRatio(eMode);
	}

	void SetFalconRenderMode(Falcon::Render::Mode::Enum eMode)
	{
		Manager::Get()->GetRenderer().SetRenderMode(eMode);
	}

	void ToggleGuide9over16()
	{
		auto const eMode(Manager::Get()->GetFixedAspectRatioMode());
		if (eMode == FixedAspectRatio::k9over16)
		{
			SetAspectRatioGuide(FixedAspectRatio::kOff);
		}
		else
		{
			SetAspectRatioGuide(FixedAspectRatio::k9over16);
		}
	}

	void ToggleOnlyShowLocTokens()
	{
		LocManager::Get()->DebugSetOnlyShowTokens(
			!LocManager::Get()->DebugOnlyShowTokens());
#if SEOUL_HOT_LOADING
		Manager::Get()->HotReload();
#endif // /#if SEOUL_HOT_LOADING
	}

	void ValidateUiFiles()
	{
#if !SEOUL_SHIP
		Manager::Get()->ValidateUiFiles("UnitTests/*", false);
#endif // /!SEOUL_SHIP
	}

private:
	SEOUL_DISABLE_COPY(Commands);
}; // class Commands

static inline Reflection::Any GetBatchOptimizer()
{
	return Manager::Get()->GetRenderer().GetDebugEnableBatchOptimizer();
}

static Reflection::Any GetCurrentAspectRatioGuide()
{
	return (Int)Manager::Get()->GetFixedAspectRatioMode();
}

static inline Reflection::Any GetLanguagePlatformCurrent()
{
	return (Int)LocManager::Get()->DebugPlatform();
}

static inline Reflection::Any GetFalconRenderModeCurrent()
{
	return (Int)Manager::Get()->GetRenderer().GetRenderMode();
}

static inline Reflection::Any GetOcclusionOptimizer()
{
	return Manager::Get()->GetRenderer().GetDebugEnableOcclusionOptimizer();
}

static inline Reflection::Any GetOverfillOptimizer()
{
	return Manager::Get()->GetRenderer().GetDebugEnableOverfillOptimizer();
}

} // namespace UI

SEOUL_BEGIN_TYPE(UI::Commands, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(CommandsInstance)

	SEOUL_METHOD(ToggleNetworkOverlay)
		SEOUL_ATTRIBUTE(Category, "UI")
		SEOUL_ATTRIBUTE(Description,
			"Starts/Stops the Network Overlay (for debugging)")
		SEOUL_ATTRIBUTE(DisplayName, "Toggle Network Overlay")
	SEOUL_METHOD(ToggleVisualizeClickInput)
		SEOUL_ATTRIBUTE(Category, "UI")
		SEOUL_ATTRIBUTE(Description,
			"Enable/disable visualization of hit areas\n"
			"that accept clicks/taps.")
		SEOUL_ATTRIBUTE(DisplayName, "Toggle Click Input Visualization")
	SEOUL_METHOD(ToggleVisualizeDragInput)
		SEOUL_ATTRIBUTE(Category, "UI")
		SEOUL_ATTRIBUTE(Description,
			"Enable/disable visualization of hit areas\n"
			"that accept drags.")
		SEOUL_ATTRIBUTE(DisplayName, "Toggle Drag Input Visualization")
	SEOUL_METHOD(SetAspectRatioGuide)
		SEOUL_ATTRIBUTE(Category, "Rendering")
		SEOUL_ATTRIBUTE(Description,
			"Set the current aspect ratio guide mode. Enables\n"
			"a frame to show various aspect ratios.")
		SEOUL_ATTRIBUTE(DisplayName, "Aspect Ratio Guide")
		SEOUL_ARG_ATTRIBUTE(0, GetCurrentValue, UI::GetCurrentAspectRatioGuide)
	SEOUL_METHOD(SetBatchOptimizer)
		SEOUL_ATTRIBUTE(Category, "Rendering")
		SEOUL_ATTRIBUTE(Description,
			"Enable/disable the draw call batch optimizer.")
		SEOUL_ATTRIBUTE(DisplayName, "Batch Optimizer")
		SEOUL_ARG_ATTRIBUTE(0, GetCurrentValue, UI::GetBatchOptimizer)
	SEOUL_METHOD(SetOcclusionOptimizer)
		SEOUL_ATTRIBUTE(Category, "Rendering")
		SEOUL_ATTRIBUTE(Description,
			"Enable/disable the occlusion optimizer.")
		SEOUL_ATTRIBUTE(DisplayName, "Occlusion Optimizer")
		SEOUL_ARG_ATTRIBUTE(0, GetCurrentValue, UI::GetOcclusionOptimizer)
	SEOUL_METHOD(SetOverfillOptimizer)
		SEOUL_ATTRIBUTE(Category, "Rendering")
		SEOUL_ATTRIBUTE(Description,
			"Enable/disable the overfill optimizer.")
		SEOUL_ATTRIBUTE(DisplayName, "Overfill Optimizer")
		SEOUL_ARG_ATTRIBUTE(0, GetCurrentValue, UI::GetOverfillOptimizer)
	SEOUL_METHOD(SetFalconRenderMode)
		SEOUL_ATTRIBUTE(Category, "Rendering")
		SEOUL_ATTRIBUTE(Description,
			"Override the current UI rendering mode.")
		SEOUL_ATTRIBUTE(DisplayName, "Set Falcon Render Mode")
		SEOUL_ARG_ATTRIBUTE(0, GetCurrentValue, UI::GetFalconRenderModeCurrent)
	SEOUL_METHOD(TriggerTransition)
		SEOUL_ARG_ATTRIBUTE(0, UITriggersEnumLike)
		SEOUL_ATTRIBUTE(Category, "UI")
		SEOUL_ATTRIBUTE(Description,
			"Fire a UI trigger that will trigger a current transition\n"
			"in the UI state machine.")
		SEOUL_ATTRIBUTE(DisplayName, "Trigger Transition")
		SEOUL_ATTRIBUTE(CommandNeedsButton)
	SEOUL_METHOD(SetLanguage)
		SEOUL_ARG_ATTRIBUTE(0, SetLanguageEnumLike)
		SEOUL_ATTRIBUTE(Category, "Localization")
		SEOUL_ATTRIBUTE(Description,
			"Set the current loc system language.")
		SEOUL_ATTRIBUTE(DisplayName, "Set Language")
	SEOUL_METHOD(ToggleDontUseFallbackLanguage)
		SEOUL_ATTRIBUTE(Category, "Localization")
		SEOUL_ATTRIBUTE(Description,
						"Toggles displaying default loc strings for other languages")
		SEOUL_ATTRIBUTE(DisplayName, "Toggle Display Default Strings")
		SEOUL_ATTRIBUTE(CommandNeedsButton)
	SEOUL_METHOD(SetLanguagePlatform)
		SEOUL_ATTRIBUTE(Category, "Localization")
		SEOUL_ATTRIBUTE(Description,
			"Set the platform used for any platform specific loc overrides.")
		SEOUL_ATTRIBUTE(DisplayName, "Set Language Platform")
		SEOUL_ARG_ATTRIBUTE(0, GetCurrentValue, UI::GetLanguagePlatformCurrent)
	SEOUL_METHOD(ToggleGuide9over16)
		SEOUL_ATTRIBUTE(Category, "Rendering")
		SEOUL_ATTRIBUTE(Description,
			"Convenience cheat, toggles between no guide and\n"
			"the 9x6 guide.")
		SEOUL_ATTRIBUTE(DisplayName, "Toggle Guide 9:16")
	SEOUL_METHOD(ToggleOnlyShowLocTokens)
		SEOUL_ATTRIBUTE(Category, "Localization")
		SEOUL_ATTRIBUTE(Description,
			"Toggle loc token display. When set to true,\n"
			"all localized strings are replaced with their\n"
			"tokens.")
		SEOUL_ATTRIBUTE(DisplayName, "Toggle Only Show Loc Tokens")
	SEOUL_METHOD(ValidateUiFiles)
		SEOUL_ATTRIBUTE(Category, "UI")
		SEOUL_ATTRIBUTE(Description,
			"Runs validation on all .fla and .swf files available to\n"
			"the game. Checks for incorrect linkage, sharing,\n"
			"etc.")
		SEOUL_ATTRIBUTE(DisplayName, "Validate")
SEOUL_END_TYPE()
#endif // /#if SEOUL_ENABLE_CHEATS

} // namespace Seoul
