/**
 * \file GameDevUI.cpp
 * \brief Specialization of DevUIRoot for in-game cheat, inspection, and profiling
 * UI. Distinct and unique from SeoulEditor.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "CookManager.h"
#include "DevUIImGui.h"
#include "DevUIView.h"
#include "Engine.h"
#include "GameDevUIMainForm.h"
#include "GameDevUIRoot.h"
#include "GameDevUIViewGameUI.h"
#include "GameDevUIMemoryUsageUtil.h"
#include "ReflectionType.h"
#include "RenderDevice.h"
#include "Renderer.h"
#include "ScriptManager.h"
#include "ToString.h"
#include "UIManager.h"

#if SEOUL_ENABLE_DEV_UI
namespace Seoul
{

SEOUL_LINK_ME_NS(class, Game, DevUIViewGameUI);

#if (SEOUL_WITH_ANIMATION_2D && !SEOUL_SHIP)
SEOUL_LINK_ME_NS(class, Game, DevUIViewAnimation2DNetworks);
#endif // /(SEOUL_WITH_ANIMATION_2D && !SEOUL_SHIP)

#if (SEOUL_WITH_ANIMATION_3D && !SEOUL_SHIP)
SEOUL_LINK_ME_NS(class, Game, DevUIViewAnimation3DNetworks);
#endif // /(SEOUL_WITH_ANIMATION_2D && !SEOUL_SHIP)

#if (!SEOUL_SHIP)
SEOUL_LINK_ME_NS(class, Game, DevUIViewLocalization);
SEOUL_LINK_ME_NS(class, Game, DevUIViewUIExplorer);
#endif // /(!SEOUL_SHIP)

namespace Game
{

static const HString kDevMenuOpen("UI_DevMenu_Opened");

static const ImVec4 kvGreen(0, 1, 0, 1);
static const ImVec4 kvRed(1, 0, 0, 1);
static const ImVec4 kvYellow(1, 1, 0, 1);

static inline ImVec4 GetFrameTimeColor(Double fFrameTimeInMilliseconds)
{
	// Derive the target frame time in milliseconds.
	Double const fTargetFrameTimeInMiliseconds = (1.0 / ((Double)
		RenderDevice::Get()->GetDisplayRefreshRate().ToHz())) * 1000.0;

	if (fFrameTimeInMilliseconds < (fTargetFrameTimeInMiliseconds + 1.0)) { return kvGreen; }
	else if (fFrameTimeInMilliseconds < (fTargetFrameTimeInMiliseconds + 10.0)) { return kvYellow; }
	else { return kvRed; }
}

static const size_t kzRedThresholdMemory = (128 * 1024 * 1024);
static const size_t kzYellowThresholdMemory = (96 * 1024 * 1024);

static inline ImVec4 GetMemoryUsageColor(size_t zSizeInBytes)
{
	// Apply thresholds.
	if (zSizeInBytes >= kzRedThresholdMemory) { return kvRed; }
	else if (zSizeInBytes >= kzYellowThresholdMemory) { return kvYellow; }
	else { return kvGreen; }
}

static inline String GetMemoryUsageString(size_t zSizeInBytes)
{
	// Handling for difference thresholds.
	if (zSizeInBytes > 1024 * 1024) { return ToString((UInt64)(zSizeInBytes / (1024 * 1024))) + " MBs"; }
	else if (zSizeInBytes > 1024) { return ToString((UInt64)(zSizeInBytes / 1024)) + " KBs"; }
	else { return ToString((UInt64)zSizeInBytes) + " Bs"; }
}

/** Common flags for hover dialogues (appear above the main form and other windows. */
static UInt32 GetHoverDialogueFlags()
{
	return (
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoInputs);
}

/**
 * Query the pos/size to use for various "hover over" windows, like hot loading
 * query, runtime stats, etc.
 */
static void GetHoverDialoguePosSize(ImVec2& pos, ImVec2& size)
{
	if (DevUIRoot::Get()->IsVirtualizedDesktop())
	{
		if (ImGui::GetWindowPosSizeByName(DevUIRoot::kVirtualizedMainFormName, pos, size, true))
		{
			return;
		}
	}

	pos = ImVec2(0, 0);
	size = ImGui::GetIO().DisplaySize;
}

// Hookage when the Game::DevUI overrides g_UIContext.
static void DisplayNotification(const String& sMessage)
{
	if (DevUIRoot::Get()) { DevUIRoot::Get()->DisplayNotification(sMessage); }
}

static void DisplayTrackedNotification(const String& sMessage, Int32& riId)
{
	if (DevUIRoot::Get()) { DevUIRoot::Get()->DisplayTrackedNotification(sMessage, riId); }
}

static void KillNotification(Int32 iId)
{
	if (DevUIRoot::Get()) { DevUIRoot::Get()->KillNotification(iId); }
}

Viewport g_GameUIRootViewportInDevUI;
static Viewport GetRootViewport()
{
	return g_GameUIRootViewportInDevUI;
}

static IPoseable* SpawnUIManager(const DataStoreTableUtil&, Bool& rbRenderPassOwnsPoseableObject)
{
	rbRenderPassOwnsPoseableObject = false;
	return DevUIRoot::Get();
}

static UI::Context s_OriginalUIContext;
static void OverrideUIContext()
{
	s_OriginalUIContext = UI::g_UIContext; // Capture.

	// Initialize.
	g_GameUIRootViewportInDevUI = RenderDevice::Get()->GetBackBufferViewport();

	// Override.
	UI::g_UIContext =
	{
		&DisplayNotification,
		&DisplayTrackedNotification,
		&KillNotification,
		&GetRootViewport,
		&SpawnUIManager,
	};
}

UI::Manager* DevUIRoot::InstantiateUIManagerInGameDevUI(FilePath guiConfigFilePath, UI::StackFilter eStackFilter)
{
	// Override g_UIContext.
	OverrideUIContext();
	// Instantiate UIManager.
	auto p = SEOUL_NEW(MemoryBudgets::UIRuntime) UI::Manager(guiConfigFilePath, eStackFilter);
	// Instantiate GameDevUI.
	SEOUL_NEW(MemoryBudgets::DevUI) DevUIRoot;
	// Done.
	return p;
}

static DevUI::MainForm* CreateMainForm()
{
	return SEOUL_NEW(MemoryBudgets::DevUI) DevUIMainForm;
}

DevUIRoot::DevUIRoot()
	: DevUI::Root(DevUI::Type::Game, &CreateMainForm)
	, m_lNotifications()
	, m_Mutex()
	, m_sDemiplaneName()
	, m_pMemoryUsageUtil()
	, m_NotificationId()
	, m_fCookingMessageDisplayTime(-1.0f)
#if SEOUL_ENABLE_CHEATS
	, m_bCanTouchToggle(false)
#endif
	// Always on by default in ship builds in non-distribution branches.
#if !SEOUL_BUILD_FOR_DISTRIBUTION && SEOUL_SHIP
	, m_bMiniFPS(true)
#else
	, m_bMiniFPS(false)
#endif
	, m_bRuntimeStats(false)
{
	SEOUL_ASSERT(IsMainThread());
}

DevUIRoot::~DevUIRoot()
{
	SEOUL_ASSERT(IsMainThread());

	// Restore context.
	UI::g_UIContext = s_OriginalUIContext;
}

void DevUIRoot::DisplayNotification(const String& sMessage)
{
	Notification notification;
	notification.m_sMessage = sMessage;

	Lock lock(m_Mutex);
	m_lNotifications.PushBack(notification);
}

void DevUIRoot::DisplayTrackedNotification(const String& sMessage, Int32& riId)
{
	auto const id = ++m_NotificationId;
	riId = (Int32)id;

	Notification notification;
	notification.m_sMessage = sMessage;
	notification.m_Id = id;
	notification.m_fDisplayTime = -1.0f;

	Lock lock(m_Mutex);
	m_lNotifications.PushBack(notification);
}

void DevUIRoot::KillNotification(Int32 iId)
{
	Atomic32Type id = (Atomic32Type)iId;

	Lock lock(m_Mutex);
	for (auto i = m_lNotifications.Begin(); m_lNotifications.End() != i; ++i)
	{
		if (i->m_Id == id)
		{
			m_lNotifications.Erase(i);
			return;
		}
	}
}

void DevUIRoot::InternalDoSkipPose(Float fDeltaTimeInSeconds)
{
	// Pass-through to UIManager.
	UI::Manager::Get()->SkipPose(fDeltaTimeInSeconds);
}

void DevUIRoot::InternalDoTickBegin(RenderPass& rPass, Float fDeltaTimeInSeconds, IPoseable* pParent)
{
	// Pass-through to UIManager.
	UI::Manager::Get()->PrePose(fDeltaTimeInSeconds, rPass, pParent);

#if SEOUL_ENABLE_CHEATS
	// Mobile cheat display hook - 5 fingers on the screen.
	auto pMouseDevice = InputManager::Get()->FindFirstMouseDevice();
	if (nullptr != pMouseDevice && pMouseDevice->IsMultiTouchDevice())
	{
		auto pMultiTouchDevice = (MultiTouchDevice*)pMouseDevice;
		auto const uTouchCount = pMultiTouchDevice->GetTouchCount();

		// Fingers released, reset touch toggle capability.
		if (0u == uTouchCount)
		{
			m_bCanTouchToggle = true;
		}
		// Five fingers on the screen, if capable, toggle.
		else if (5u == uTouchCount && m_bCanTouchToggle)
		{
			m_bMainMenuVisible = !m_bMainMenuVisible;
			m_bCanTouchToggle = false;
		}
	}

	// Input hooks.
	{
		if (InputManager::Get()->WasBindingPressed("ToggleMainMenuBar"))
		{
			m_bMainMenuVisible = !m_bMainMenuVisible;
		}
		if (InputManager::Get()->WasBindingPressed("ToggleRuntimeStats"))
		{
			m_bRuntimeStats = !m_bRuntimeStats;
		}
	}
#endif // /#if SEOUL_ENABLE_CHEATS
}

void DevUIRoot::InternalOnSwitchToVirtualizedDesktop()
{
	// Also dock the game UI into the main form of the virtualized UI.
	ImGui::DockWindowByName(DevUIViewGameUI::StaticGetId().CStr(), m_uMainFormDockSpaceID);
}

void DevUIRoot::InternalPrePoseImGuiFrameBegin(RenderPass& rPass, Float fDeltaTimeInSeconds)
{
	UI::Manager::Get()->SetCondition(kDevMenuOpen, IsMainMenuVisible());
}

void DevUIRoot::InternalPrePoseImGuiFrameEnd(RenderPass& rPass, Float fDeltaTimeInSeconds)
{
	if (m_bRuntimeStats)
	{
		InternalPrePoseRuntimeStats(fDeltaTimeInSeconds);
	}
	else if (m_bMiniFPS)
	{
		InternalPrePoseMiniFPS(fDeltaTimeInSeconds);
	}
	else if (m_pMemoryUsageUtil.IsValid())
	{
		m_pMemoryUsageUtil.Reset();
	}

	InternalPrePoseNotifications(fDeltaTimeInSeconds);
	InternalPrePoseHotLoadingState();
	InternalPrePoseDemiplaneState();
}

Bool DevUIRoot::InternalBeginMainMenuPrePose(Bool bRootMainMenu)
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
		InternalPrePoseFileMenu();
		return true;
	}

	return false;
}

void DevUIRoot::InternalEndMainMenuPrePose(Bool bRootMainMenu)
{
	InternalPrePoseShowMenu();
	if (m_pActiveMainForm)
	{
		m_pActiveMainForm->PrePoseMainMenu();
	}
	InternalPrePoseHelpMenu();
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

void DevUIRoot::InternalPrePoseFileMenu()
{
	using namespace ImGui;

	// No File menu on mobile.
	if (IsMobile())
	{
		return;
	}

	if (BeginMenu("File"))
	{
#if !SEOUL_SHIP // Developer only support for virtualized desktop.
		Bool bVirtualized = IsVirtualizedDesktop();
		if (MenuItem("Virtualized", nullptr, &bVirtualized, RenderDevice::Get()->SupportsVirtualizedDesktop()))
		{
			RenderDevice::Get()->SetVirtualizedDesktop(bVirtualized);
		}
		Separator();
#endif // /!SEOUL_SHIP

		if (MenuItem("Exit"))
		{
			Engine::Get()->PostNativeQuitMessage();
		}
		EndMenu();
	}
}

void DevUIRoot::InternalPrePoseHelpMenu()
{
	using namespace ImGui;

	// No Help menu on mobile.
	if (IsMobile())
	{
		return;
	}

	Bool bShowAboutSeoulEngine = false;
	if (BeginMenu("Help"))
	{
		if (MenuItem("About SeoulEngine"))
		{
			bShowAboutSeoulEngine = true;
		}

		EndMenu();
	}

	if (bShowAboutSeoulEngine)
	{
		OpenPopup("About SeoulEngine");
	}
	if (BeginPopupModalEx("About SeoulEngine", GetWindowCenter(), nullptr, ImGuiWindowFlags_NoResize))
	{
		Text("SeoulEngine\n\nCopyright (C) Demiurge Studios 2012-2021.");
		if (Button("OK", ImVec2(120, 0)))
		{
			CloseCurrentPopup();
		}

		EndPopup();
	}
}

void DevUIRoot::InternalPrePoseShowMenu()
{
	// No Show menu on mobile.
	if (IsMobile())
	{
		return;
	}

	using namespace ImGui;
	if (BeginMenu("Show"))
	{
		(void)MenuItem("Runtime Stats", nullptr, &m_bRuntimeStats);
		(void)MenuItem("Views Always", nullptr, &m_bAlwaysShowViews);
		EndMenu();
	}
}

void DevUIRoot::InternalPrePoseDemiplaneState()
{
	using namespace ImGui;

	if (m_sDemiplaneName.IsEmpty())
	{
		return;
	}

	ImVec2 const size(184.0f, 54.0f);

	ImVec2 basePos, baseSize;
	GetHoverDialoguePosSize(basePos, baseSize);

	ImVec2 pos;
	pos.x = basePos.x + GetStyle().WindowPadding.x;
	pos.y = (basePos.y + baseSize.y - size.y - 2.0f * GetStyle().WindowPadding.y);
	SetNextWindowPos(pos);
	SetNextWindowSize(size);
	SetNextWindowBgAlpha(0.75f);

	if (Begin(
		"Demiplane Status",
		nullptr,
		GetHoverDialogueFlags()))
	{
		ImVec4 color(1, 1, 0, 1);
		auto pEngine = Engine::Get();
		if (pEngine)
		{
			color.y = (Float)Sin(pEngine->GetSecondsSinceStartup() * 2.5);
		}
		TextColored(color, "CONNECTED TO DEMIPLANE");
		Text("%s", m_sDemiplaneName.CStr());
	}
	End();
}

void DevUIRoot::InternalPrePoseHotLoadingState()
{
#if SEOUL_HOT_LOADING
	static const UInt32 kuMaxFiles = 25u;
	static const HString ksCancelHotLoad("CancelHotLoad");
	static const HString ksHotLoad("HotLoad");

	using namespace ImGui;

	auto const& tChanges = Content::LoadManager::Get()->GetContentChanges();
	if (!tChanges.IsEmpty())
	{
		ImVec2 basePos, baseSize;
		GetHoverDialoguePosSize(basePos, baseSize);

		ImVec2 pos;
		pos.x = basePos.x;
		pos.y = basePos.y + GetMainMenuBarHeight();
		SetNextWindowPos(pos);
		if (Begin(
			"Hot Loading Ready",
			nullptr,
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings))
		{
			// Draw a header line.
			Text("The following files have changed, press %s to hot load, or press %s to clear the hot load.",
				InputManager::Get()->BindingToString(ksHotLoad).CStr(),
				InputManager::Get()->BindingToString(ksCancelHotLoad).CStr());

			// Construct a vector of filenames. Limit to a reasonable number.
			UInt32 const uCount = Min(kuMaxFiles, tChanges.GetSize());
			Vector<String, MemoryBudgets::Content> vsFilenames;
			vsFilenames.Reserve(uCount);

			{
				UInt32 u = 0u;
				auto const iBegin = tChanges.Begin();
				auto const iEnd = tChanges.End();
				for (auto i = iBegin; iEnd != i; ++i)
				{
					auto const s(i->First.ToSerializedUrl());

					// Skip redundant - happens with texture file.
					if (!vsFilenames.IsEmpty() && vsFilenames.Back() == s)
					{
						continue;
					}

					vsFilenames.PushBack(s);
					++u;
					if (u >= uCount)
					{
						break;
					}
				}
			}

			// Alphabetical sort.
			QuickSort(vsFilenames.Begin(), vsFilenames.End());

			// Now draw each entry in the filename Vector<>. Limit so the
			// contents don't get out of hand.e
			for (auto i = vsFilenames.Begin(); vsFilenames.End() != i; ++i)
			{
				BulletText("%s", i->CStr());
			}

			if (tChanges.GetSize() > uCount)
			{
				BulletText("...%u more files", (tChanges.GetSize() - uCount));
			}

		}
		End();
	}
#endif // /#if SEOUL_HOT_LOADING
}

void DevUIRoot::InternalPrePoseMiniFPS(Float fDeltaTimeInSeconds)
{
	using namespace ImGui;

	ImVec2 const size(110.0f, 18.0f);

	ImVec2 basePos, baseSize;
	GetHoverDialoguePosSize(basePos, baseSize);

	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));
	ImVec2 pos;
	pos.x = (basePos.x + baseSize.x - size.x - 2.0f * GetStyle().WindowPadding.x);
	pos.y = (basePos.y + baseSize.y - size.y - 2.0f * GetStyle().WindowPadding.y);
	SetNextWindowPos(pos);
	SetNextWindowSize(size);
	SetNextWindowBgAlpha(0.75f);

	if (Begin(
		"Mini FPS",
		nullptr,
		GetHoverDialogueFlags()))
	{
		// FPS and actual time spent in engine (on the main thread).
		auto const meanFrameTime = Renderer::Get()->GetFrameRateTracking().GetMeanFrameTicks();
		auto const fMeanFrameTimeInMilliseconds = SeoulTime::ConvertTicksToMilliseconds(meanFrameTime.First);
		auto const frameTimeColor = GetFrameTimeColor(fMeanFrameTimeInMilliseconds);
		auto const fMeanFramesPerSecond = Max(1.0 / SeoulTime::ConvertTicksToSeconds(meanFrameTime.Second), 0.0);

		TextColored(frameTimeColor, "%02d FPS (%.1f ms)", (Int)Round(fMeanFramesPerSecond), fMeanFrameTimeInMilliseconds);
	}
	End();
	PopStyleVar();
}

void DevUIRoot::InternalPrePoseNotifications(Float fDeltaTimeInSeconds)
{
	using namespace ImGui;

	static const Float kfShortNotificationDisplayTime = 3.0f;
	static const Float kfLongNotificationDisplayMaxTime = 30.0f;
	static const Float kfCookingDisplayTime = 1.0f;

	FilePath const filePath = CookManager::Get()->GetCurrent();
	if (filePath.IsValid())
	{
		m_fCookingMessageDisplayTime = kfCookingDisplayTime;
	}

	Bool const bCooking = (m_fCookingMessageDisplayTime >= 0.0f);
	Bool const bScriptHotLoad = (Seoul::Script::Manager::Get()->IsInAppScriptHotLoad());
	m_fCookingMessageDisplayTime -= fDeltaTimeInSeconds;

	Lock lock(m_Mutex);
	if (bCooking || bScriptHotLoad || !m_lNotifications.IsEmpty())
	{
		Int iCount = 0;
		iCount += (bCooking ? 1 : 0);
		iCount += (bScriptHotLoad ? 1 : 0);
		iCount += (!m_lNotifications.IsEmpty() ? 1 : 0);
		ImVec2 const size(200.0f, 25.0f + GetFontSize() * (Float)iCount);

		ImVec2 basePos, baseSize;
		GetHoverDialoguePosSize(basePos, baseSize);

		ImVec2 const pos(basePos.x + baseSize.x - size.x, basePos.y + baseSize.y - size.y);
		SetNextWindowPos(pos);
		SetNextWindowSize(size);

		if (Begin(
			"Notifications",
			nullptr,
			GetHoverDialogueFlags()))
		{
			if (bCooking)
			{
				Text("Cooking...");
			}

			if (bScriptHotLoad)
			{
				Text("Loading Script Vm...");
			}

			while (!m_lNotifications.IsEmpty())
			{
				auto& r = m_lNotifications.Front();
				if (r.m_fDisplayTime >= 0.0f)
				{
					if (r.m_fDisplayTime >= kfShortNotificationDisplayTime)
					{
						m_lNotifications.PopFront();
					}
					else
					{
						Text("%s", r.m_sMessage.CStr());
						r.m_fDisplayTime += fDeltaTimeInSeconds;
						break;
					}
				}
				else
				{
					if (r.m_fDisplayTime <= -kfLongNotificationDisplayMaxTime)
					{
						m_lNotifications.PopFront();
					}
					else
					{
						Text("%s", r.m_sMessage.CStr());
						r.m_fDisplayTime -= fDeltaTimeInSeconds;
						break;
					}
				}
			}
		}
		End();
	}
}

void DevUIRoot::InternalPrePoseRuntimeStats(Float fDeltaTimeInSeconds)
{
	using namespace ImGui;

	ImVec2 const size(130.0f, 96.0f);

	ImVec2 basePos, baseSize;
	GetHoverDialoguePosSize(basePos, baseSize);

	ImVec2 pos;
	pos.x = (basePos.x + baseSize.x - size.x - 2.0f * GetStyle().WindowPadding.x);
	pos.y = basePos.y + GetMainMenuBarHeight();
	SetNextWindowPos(pos);
	SetNextWindowSize(size);
	SetNextWindowBgAlpha(0.75f);

	if (Begin(
		"Runtime Stats",
		nullptr,
		GetHoverDialogueFlags()))
	{
		// FPS and actual time spent in engine (on the main thread).
		auto const maxFrameTime = Renderer::Get()->GetFrameRateTracking().GetMaxFrameTicks();
		auto const meanFrameTime = Renderer::Get()->GetFrameRateTracking().GetMeanFrameTicks();

		Double const fMaxFrameTimeInMilliseconds = SeoulTime::ConvertTicksToMilliseconds(maxFrameTime.First);
		Double const fMeanFrameTimeInMilliseconds = SeoulTime::ConvertTicksToMilliseconds(meanFrameTime.First);
		auto const frameTimeColor = GetFrameTimeColor(fMeanFrameTimeInMilliseconds);

		Double const fMinFramesPerSecond = Max(1.0 / SeoulTime::ConvertTicksToSeconds(maxFrameTime.Second), 0.0);
		Double const fMeanFramesPerSecond = Max(1.0 / SeoulTime::ConvertTicksToSeconds(meanFrameTime.Second), 0.0);

		auto const viewport = RenderDevice::Get()->GetBackBufferViewport();
		auto const iVsyncInterval = RenderDevice::Get()->GetVsyncInterval();
		Text("(%d x %d) (%d)", viewport.m_iViewportWidth, viewport.m_iViewportHeight, iVsyncInterval);
		TextColored(frameTimeColor, "%d(%d) FPS", (Int)Round(fMeanFramesPerSecond), (Int)Round(fMinFramesPerSecond));
		TextColored(frameTimeColor, "%.1f(%.1f) ms", fMeanFrameTimeInMilliseconds, fMaxFrameTimeInMilliseconds);

		// Memory usage.
		if (!m_pMemoryUsageUtil.IsValid())
		{
			m_pMemoryUsageUtil.Reset(SEOUL_NEW(MemoryBudgets::DevUI) DevUIMemoryUsageUtil);
		}

		size_t const zTotalPrivate = m_pMemoryUsageUtil->GetLastMemoryUsagePrivateSample();
		if (zTotalPrivate > 0u)
		{
			size_t const zTotalWorking = m_pMemoryUsageUtil->GetLastMemoryUsageWorkingSample();
			if (zTotalWorking == zTotalPrivate)
			{
				TextColored(
					GetMemoryUsageColor(zTotalWorking),
					"%s",
					GetMemoryUsageString(zTotalWorking).CStr());
			}
			else
			{
				TextColored(
					GetMemoryUsageColor(zTotalPrivate),
					"%s(%s)",
					GetMemoryUsageString(zTotalWorking).CStr(),
					GetMemoryUsageString(zTotalPrivate).CStr());
			}
		}
	}
	End();
}

} // namespace Game

} // namespace Seoul

#endif // /SEOUL_ENABLE_DEV_UI
