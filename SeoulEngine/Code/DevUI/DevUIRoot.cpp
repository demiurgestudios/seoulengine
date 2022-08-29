/**
 * \file DevUIRoot.cpp
 * \brief Base class of the global singleton used for developer UI.
 * Currently specialized for runtime app builds (GameDevUI) and
 * EditorUI.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BuildDistroPublic.h"
#include "DevUIConfig.h"
#include "DevUIController.h"
#include "DevUIImGui.h"
#include "DevUIImGuiFont.h"
#include "DevUIImGuiRenderer.h"
#include "DevUIImGuiRendererSettings.h"
#include "DevUIMainForm.h"
#include "DevUIRoot.h"
#include "DevUIView.h"
#include "DevUIUtil.h"
#include "DevUIViewCommands.h"
#include "DevUIViewLog.h"
#include "Engine.h"
#include "FileManager.h"
#include "EventsManager.h"
#include "GamePaths.h"
#include "InputManager.h"
#include "MouseCursor.h"
#include "PlatformData.h"
#include "ReflectionDefine.h"
#include "ReflectionRegistry.h"
#include "ReflectionType.h"
#include "RenderCommandStreamBuilder.h"
#include "RenderDevice.h"
#include "RenderPass.h"
#include "ScopedAction.h"
#include "ThreadId.h"

#if SEOUL_ENABLE_DEV_UI
namespace Seoul
{

#if SEOUL_ENABLE_CHEATS
SEOUL_LINK_ME_NS(class, DevUI, Commands);
#endif // /SEOUL_ENABLE_CHEATS

#if (SEOUL_ENABLE_CHEATS)
SEOUL_LINK_ME_NS(class, DevUI, ViewCommands);
#endif // /(SEOUL_ENABLE_CHEATS)

#if (!SEOUL_SHIP)
SEOUL_LINK_ME_NS(class, DevUI, ViewEngineStats);
SEOUL_LINK_ME_NS(class, DevUI, ViewMemoryUsage);
#endif // /(!SEOUL_SHIP)

#if (SEOUL_LOGGING_ENABLED)
SEOUL_LINK_ME_NS(class, DevUI, ViewLog);
#endif // /(SEOUL_LOGGING_ENABLED)

SEOUL_BEGIN_ENUM(DevUI::Mode)
	SEOUL_ENUM_N("Mobile", DevUI::Mode::Mobile)
	SEOUL_ENUM_N("Desktop", DevUI::Mode::Desktop)
	SEOUL_ENUM_N("VirtualizedDesktop", DevUI::Mode::VirtualizedDesktop)
SEOUL_END_ENUM()

namespace DevUI
{

/** Name of the main form window in virtualized mode (in non-virtualized mode, we use a regular OS window). */
Byte const* const Root::kVirtualizedMainFormName = "MainForm";
Byte const* const Root::kVirtualizedMainFormDockSpaceName = "##MainFormDockSpace";

static const HString kWindowScale("WindowScale=");

static void* ImGuiAllocate(size_t zSizeInBytes, void* ud)
{
	return MemoryManager::Allocate(zSizeInBytes, MemoryBudgets::DevUI);
}

static void ImGuiDeallocate(void* pAddressToDeallocate, void* ud)
{
	MemoryManager::Deallocate(pAddressToDeallocate);
}

static const HString kDevUIGlobalSettings("DevUIGlobalSettings");

/** Called to start the load a single view entry. */
void* Root::ImGuiReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* sId)
{
	// Must have an HString name to be loadable.
	HString const id(sId);

	// Special case.
	if (kDevUIGlobalSettings == id)
	{
		return ((void*)1);
	}

	// Dispatch to main form.
	auto pActiveMainForm(Root::Get()->m_pActiveMainForm);
	if (pActiveMainForm)
	{
		return pActiveMainForm->ImGuiReadOpen(id);
	}

	return nullptr;
}

/** Called to load a single view entry. */
void Root::ImGuiReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* pEntry, const char* sLine)
{
	// Acquire entry.
	if (nullptr == pEntry)
	{
		return;
	}

	// Special case, global setting.
	if (((void*)1) == pEntry)
	{
		// Special value that means main menu visible or not.
		Int i = 0;
		if (SSCANF(sLine, "Enabled=%d", &i) == 1)
		{
			Root::Get()->m_bMainMenuVisible = (0 != i);
		}
		Float fInverseWindowScale = 0.0f;
		if (SSCANF(sLine, "WindowScale=%f", &fInverseWindowScale) == 1)
		{
			Root::Get()->m_fConfiguredInverseWindowScale = Clamp(fInverseWindowScale, Util::kfMinInverseWindowScale, Util::kfMaxInverseWindowScale);
		}
		return;
	}

	// Dispatch to model.
	auto pActiveMainForm(Root::Get()->m_pActiveMainForm);
	if (pActiveMainForm)
	{
		pActiveMainForm->ImGuiReadLine(pEntry, sLine);
	}
}

/** Save all views to ImGui settings. */
void Root::ImGuiWriteAll(ImGuiContext*, ImGuiSettingsHandler*, ImGuiTextBuffer* buf)
{
	// Special.
	buf->appendf("[%s][%s]\n", "DevUI", kDevUIGlobalSettings.CStr());
	buf->appendf("Enabled=%d\n", Root::Get()->m_bMainMenuVisible ? 1 : 0);
	if (Root::Get()->m_fConfiguredInverseWindowScale >= Util::kfMinInverseWindowScale)
	{
		buf->appendf("WindowScale=%f\n", Root::Get()->m_fConfiguredInverseWindowScale);
	}
	buf->appendf("\n");

	// Main form, common case.
	auto pActiveMainForm(Root::Get()->m_pActiveMainForm);
	if (pActiveMainForm)
	{
		pActiveMainForm->ImGuiWriteAll(buf);
	}
}

static void InternalStaticInitializeImGui()
{
	// Override memory allocation.
	ImGui::SetAllocatorFunctions(
		ImGuiAllocate,
		ImGuiDeallocate);

	// Compute, needed in a few spots.
	auto const fWindowScale = Root::Get()->GetWindowScale();
	auto const fPixelSize = 16.0f / fWindowScale;

	// Init a font atlas.
	ImFontAtlas* pAtlas = SEOUL_NEW(MemoryBudgets::DevUI) ImFontAtlas;
	{
		ImFontConfig cfg;
		cfg.OversampleH = cfg.OversampleV = 1;
		cfg.PixelSnapH = true;
		cfg.SizePixels = fPixelSize; // Oversampled based on window scale.
		pAtlas->AddFontFromMemoryCompressedTTF(
			ImGuiFont::GetDataTTF(),
			ImGuiFont::GetSize(),
			fPixelSize,
			&cfg,
			ImGuiFont::GetGlyphRanges());
	}

	// Init ImGui.
	ImGui::CreateContext(pAtlas);

	// Register our data handlers.
	ImGui::RegisterSettingsHandler(
		"DevUI",
		Root::ImGuiReadOpen,
		Root::ImGuiReadLine,
		Root::ImGuiWriteAll);

	// Configure.
	ImGuiIO& io = ImGui::GetIO();

	// Need to set the global font scale to the inverse of the pixel
	// size for things to work out - we apply the actual scaling
	// indepenent of ImGui, but we need to do this so that font
	// glyphs have enough resolution for the increased size.
	io.FontGlobalScale = fWindowScale;

	// Disable automatic ini handling (handled manually), disable log.
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;
}

static void InternalStaticShutdownImGui()
{
	// Grab the atlas to cleanup first.
	auto pAtlas = ImGui::GetIO().Fonts;
	// Cleanup ImGui.
	ImGui::DestroyContext();
	// Cleanup.
	SafeDelete(pAtlas);
}

// Settings not loaded in Ship - DevUI::Root exists only in limited
// Ship builds (not in distribution branches) to display the FPS
// counter.
#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)
/** Name to use for imgui settings. */
static String GetImGuiIniFilename()
{
	// We add a prefix for defined(SEOUL_PROFILING_BUILD) builds
	// since multiple features are not available (the Dev UI is enabled
	// in profiling builds only to support now deprecated cheats
	// in profiling).
#if defined(SEOUL_PROFILING_BUILD)
#	define SEOUL_NAME_PREFIX "profiling_"
#else
#	define SEOUL_NAME_PREFIX ""
#endif // /defined(SEOUL_PROFILING_BUILD)

	// Unique delimiter for virtualized mode.
	auto const sVirtualized = Root::Get()->IsVirtualizedDesktop() ? "_virt" : "";

	// If in a distribution branch and if desired,
	// use a unique config file when loading/saving to/from the branch.
	if (g_kbBuildForDistribution)
	{
		if (GetDevUIConfig().m_GlobalConfig.m_bUniqueLayoutForBranches)
		{
			auto const sIniFile(Path::Combine(GamePaths::Get()->GetSaveDir(), String::Printf(SEOUL_NAME_PREFIX "devui%s_branch.ini", sVirtualized)));
			return sIniFile;
		}
	}

	// Otherwise, use the default.
	auto const sIniFile(Path::Combine(GamePaths::Get()->GetSaveDir(), String::Printf(SEOUL_NAME_PREFIX "devui%s.ini", sVirtualized)));
	return sIniFile;

#	undef SEOUL_NAME_PREFIX
}
#endif // /(!SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD))

/**
 * Quickie workaround - we need the window scale value before initializing
 * ImGui, but can't use the normal handling until ImGui is initialized.
 *
 * Only an issue the very first time we load imGui.
 */
static Float SpecialLoadInverseWindowScale(void*& rp, UInt32& ru)
{
	// Settings not loaded in Ship - DevUI::Root exists only in limited
	// Ship builds (not in distribution branches) to display the FPS
	// counter.
#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)
	if (FileManager::Get()->ReadAll(GetImGuiIniFilename(), rp, ru, 0u, MemoryBudgets::DevUI))
	{
		Byte const* pS = (Byte const*)rp;
		Byte const* const pEnd = pS + ru;
		auto const uSize = kWindowScale.GetSizeInBytes();
		pS += uSize;
		while (pS < pEnd)
		{
			Float f = 0.0f;
			if (0 == strncmp(pS - uSize, kWindowScale.CStr(), uSize) &&
				1 == SSCANF(pS, "%f", &f))
			{
				return f;
			}

			++pS;
		}
	}
#endif // /(!SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)

	return -1.0f;
}

/** Read settings - always. */
static Bool LoadImGuiSettings(void* p, UInt32 u)
{
	// Settings not loaded in Ship - DevUI::Root exists only in limited
	// Ship builds (not in distribution branches) to display the FPS
	// counter.
#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)
	auto const deferred(MakeDeferredAction([&]() { MemoryManager::Deallocate(p); }));

	if (nullptr != p ||
		FileManager::Get()->ReadAll(GetImGuiIniFilename(), p, u, 0u, MemoryBudgets::DevUI))
	{
		ImGui::LoadIniSettingsFromMemory((const char*)p, u);
		return true;
	}
#endif // /(!SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD))

	return false;
}

/** Write settings - unless forced, based on save need as reported by ImGui. */
static void SaveImGuiSettings(Bool bForce = false)
{
	// Settings not saved in Ship - DevUI::Root exists only in limited
	// Ship builds (not in distribution branches) to display the FPS
	// counter.
#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)
	if (bForce || ImGui::GetIO().WantSaveIniSettings)
	{
		ImGui::GetIO().WantSaveIniSettings = false;

		size_t zSize = 0;
		void const* p = (void const*)ImGui::SaveIniSettingsToMemory(&zSize);

		auto const s(GetImGuiIniFilename());
		(void)FileManager::Get()->WriteAll(s, p, (UInt32)zSize);
	}
#endif // /(!SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD))
}

static inline Mode GetInitialMode()
{
#if SEOUL_DEVUI_MOBILE
#	if !SEOUL_SHIP
		return RenderDevice::Get()->IsVirtualizedDesktop() ? Mode::VirtualizedDesktop : Mode::Mobile;
#	else
		return Mode::Mobile;
#	endif
#else
#	if !SEOUL_SHIP
		return RenderDevice::Get()->IsVirtualizedDesktop() ? Mode::VirtualizedDesktop : Mode::Desktop;
#	else
		return Mode::Desktop;
#	endif
#endif
}

/**
 * If the window scale has been overriden by user configuration,
 * this function returns that value. Otherwise, it returns
 * a "reasonable default" based on the platform and reported
 * system DPI.
 */
static Float ComputeOrReadWindowScale()
{
	// Configured.
	auto const fConfig = Root::Get()->GetConfiguredInverseWindowScale();
	if (fConfig >= Util::kfMinInverseWindowScale)
	{
		return 1.0f / fConfig;
	}

	// In this configuration, the DevUI::Root is being used only
	// to show mini FPS counter. So we fixed the scale
	// factor based on platform.
#if !SEOUL_BUILD_FOR_DISTRIBUTION && SEOUL_SHIP
#	if SEOUL_DEVUI_MOBILE
	return 0.5f;
#	else
	return 1.0f;
#	endif
#else

	// Smaller value = bigger font and padding on mobile.
#if SEOUL_DEVUI_MOBILE
	auto const fNumerator = 75.0f;
#else
	auto const fNumerator = 100.0f;
#endif

	// DPI based scale.
	PlatformData data;
	Engine::Get()->GetPlatformData(data);
	auto const fMax = data.m_vScreenPPI.GetMaxComponent();
	if (!IsZero(fMax))
	{
		return fNumerator / fMax;
	}
	return 1.0f;

#endif
}

Root::Root(Type eType, MainForm* (*createInitialMainForm)())
	: m_eType(eType)
	, m_uMainFormDockSpaceID(ImGui::GetGlobalDockSpaceID(kVirtualizedMainFormDockSpaceName))
	, m_LastOsWindowPos(0, 0)
	, m_LastOsWindowSize(0, 0)
	, m_eCurrentMode(GetInitialMode())
	, m_eDesiredMode(m_eCurrentMode)
	, m_aMouseCaptures()
	, m_aKeysCaptured()
	, m_pImGuiRenderer()
	, m_vMainForms()
	, m_pActiveMainForm()
	, m_bMainMenuVisible(false)
	, m_bAlwaysShowViews(false)
	, m_bWantTextInput(false)
	, m_bImGuiFrameReady(false)
	, m_bPendingTextEditableApplyText(false)
{
	SEOUL_ASSERT(IsMainThread());

	// Initial configuration, post settings load.
	void* p = nullptr;
	UInt32 u = 0u;
	m_fConfiguredInverseWindowScale = SpecialLoadInverseWindowScale(p, u);
	m_fWindowScale = ComputeOrReadWindowScale();
	m_fMouseScale = m_fWindowScale;

	// Setup the global ImGuiIO context.
	InternalStaticInitializeImGui();
	// Initial config.
	InternalApplyModeToImGui();

	// Setup the renderer.
	m_pImGuiRenderer.Reset(SEOUL_NEW(MemoryBudgets::DevUI) ImGuiRenderer);

	// Cache the initial main form.
	if (createInitialMainForm)
	{
		auto pInitialMainForm(createInitialMainForm());
		if (pInitialMainForm)
		{
			m_vMainForms.PushBack(pInitialMainForm);
			m_pActiveMainForm = m_vMainForms.Back();
		}
	}

	// Load settings now that views have been created.
	InternalLoadImGuiSettings(p, u);
	p = nullptr;
	u = 0u;

	// Register input callbacks.
	Events::Manager::Get()->RegisterCallback(
		g_EventAxisEvent,
		SEOUL_BIND_DELEGATE(&Root::HandleAxisEvent, this));
	Events::Manager::Get()->MoveLastCallbackToFirst(g_EventAxisEvent);
	Events::Manager::Get()->RegisterCallback(
		g_EventButtonEvent,
		SEOUL_BIND_DELEGATE(&Root::HandleButtonEvent, this));
	Events::Manager::Get()->MoveLastCallbackToFirst(g_EventButtonEvent);
	Events::Manager::Get()->RegisterCallback(
		g_MouseMoveEvent,
		SEOUL_BIND_DELEGATE(&Root::HandleMouseMoveEvent, this));
	Events::Manager::Get()->MoveLastCallbackToFirst(g_MouseMoveEvent);
}

Root::~Root()
{
	SEOUL_ASSERT(IsMainThread());

	// Unregister input callbacks.
	Events::Manager::Get()->UnregisterCallback(
		g_MouseMoveEvent,
		SEOUL_BIND_DELEGATE(&Root::HandleMouseMoveEvent, this));
	Events::Manager::Get()->UnregisterCallback(
		g_EventButtonEvent,
		SEOUL_BIND_DELEGATE(&Root::HandleButtonEvent, this));
	Events::Manager::Get()->UnregisterCallback(
		g_EventAxisEvent,
		SEOUL_BIND_DELEGATE(&Root::HandleAxisEvent, this));

	// Save prior to cleanup so we capture view state. Force
	// on shutdown so we capture last minute changes.
	SaveImGuiSettings(true);

	// Cleanup main forms.
	m_pActiveMainForm.Reset();
	SafeDeleteVector(m_vMainForms);

	// Cleanup the renderer.
	m_pImGuiRenderer.Reset();

	// Shutdown ImGui
	InternalStaticShutdownImGui();
}

/** Update the user window scale override. */
void Root::SetConfiguredInverseWindowScale(Float fScale)
{
	m_fConfiguredInverseWindowScale = Clamp(fScale, Util::kfMinInverseWindowScale, Util::kfMaxInverseWindowScale);
}

Bool Root::HandleAxisEvent(InputDevice* pInputDevice, InputDevice::Axis* pAxis)
{
	SEOUL_ASSERT(IsMainThread());

	Bool bReturn = false;

	// Views get first crack.

	// On mouse wheel, give views a chance to capture.
	if (pAxis->GetID() == InputAxis::MouseWheel && pAxis->GetRawState() != 0)
	{
		// The first capture gets dibs - and if it is set,
		// even if it doesn't care about the mouse wheel,
		// it suppresses the mouse wheel from other targets.
		for (auto const& pView : m_aMouseCaptures)
		{
			if (pView)
			{
				// Don't care about return in this case.
				(void)pView->OnMouseWheel(pInputDevice, pAxis);
				// Return immediately.
				return true;
			}
		}

		// Otherwise, give all views a chance to capture.
		if (m_pActiveMainForm)
		{
			if (m_pActiveMainForm->OnMouseWheel(pInputDevice, pAxis))
			{
				// Return immediately.
				return true;
			}
		}
	}

	ImGuiIO& io = ImGui::GetIO();
	switch (pAxis->GetID())
	{
	case InputAxis::MouseWheel:
		if (io.WantCaptureMouse)
		{
			if (pAxis->GetRawState() != 0)
			{
				io.AddMouseWheelEvent(0.0f, pAxis->GetRawState() > 0.0f ? 1.0f : -1.0f);
			}

			// We capture input when ImGui wants it.
			bReturn = true;
		}
		break;
	default:
		break;
	};

	return bReturn;
}

Bool Root::HandleButtonEvent(
	InputDevice* pInputDevice,
	InputButton eButtonID,
	ButtonEventType eEventType)
{
	SEOUL_ASSERT(IsMainThread());

	Bool bReturn = false;

	// Used throughout.
	ImGuiIO& io = ImGui::GetIO();

	// Give views first crack.

	// Mouse handling.
	if (pInputDevice->GetDeviceType() == InputDevice::Mouse)
	{
		// For all, grab index.
		Int i = -1;
		switch (eButtonID)
		{
		case InputButton::MouseLeftButton: i = 0; break;
		case InputButton::MouseRightButton: i = 1; break;
		case InputButton::MouseMiddleButton: i = 2; break;
		default:
			break;
		};

		// Only continue if one of the moue buttons we support.
		if (i >= 0)
		{
			// If there is already a capture for this mouse index, deliver the event exclusively
			// to that capture.
			if (m_aMouseCaptures[i])
			{
				// Don't care about return in this case, since view is already a capture.
				(void)m_aMouseCaptures[i]->OnMouseButton(
					pInputDevice,
					eButtonID,
					eEventType);

				// If a release event, also clear the capture.
				if (ButtonEventType::ButtonReleased == eEventType)
				{
					m_aMouseCaptures[i] = nullptr;
				}

				// Always return immediately, capture.
				return true;
			}

			// On press of a mouse button, check if any view wants to capture the mouse button.
			// This is used for (e.g.) game viewports as contained in an imgui window.
			if (ButtonEventType::ButtonReleased != eEventType)
			{
				if (m_pActiveMainForm)
				{
					if (m_pActiveMainForm->OnMouseButton(pInputDevice, eButtonID, eEventType, &m_aMouseCaptures[i]))
					{
						// Return immediately.
						return true;
					}
				}
			}
		}
	}

	// On press of a non-special, known keyboard buttons, dispatch an OnInputReleased() event
	// to all views.
	if (pInputDevice->GetDeviceType() == InputDevice::Keyboard &&
		ButtonEventType::ButtonReleased != eEventType &&
		InputButton::ButtonUnknown != eButtonID &&
		!InputManager::IsSpecial(eButtonID))
	{
		UInt32 uSpecialKeys = 0u;
		uSpecialKeys |= (ImGui::IsKeyDown(ImGuiKey_LeftAlt) ? InputManager::kLeftAlt : 0u);
		uSpecialKeys |= (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) ? InputManager::kLeftControl : 0u);
		uSpecialKeys |= (ImGui::IsKeyDown(ImGuiKey_LeftShift) ? InputManager::kLeftShift : 0u);
		uSpecialKeys |= (ImGui::IsKeyDown(ImGuiKey_RightAlt) ? InputManager::kRightAlt : 0u);
		uSpecialKeys |= (ImGui::IsKeyDown(ImGuiKey_RightCtrl) ? InputManager::kRightControl : 0u);
		uSpecialKeys |= (ImGui::IsKeyDown(ImGuiKey_RightShift) ? InputManager::kRightShift : 0u);

		// Dispatch to the active main form.
		if (m_pActiveMainForm)
		{
			if (m_pActiveMainForm->OnKeyPressed(eButtonID, uSpecialKeys))
			{
				bReturn = true;
			}
		}
	}

	// Now update key state.
	{
		auto eTo = ToImGuiKey(eButtonID);
		if (ImGuiKey_None != eTo)
		{
			io.AddKeyEvent(eTo, (ButtonEventType::ButtonReleased != eEventType));
		}
	}

	// TODO: Figure out a better way of maintaining this list of keys.
	switch (eButtonID)
	{
	case InputButton::KeyBrowserBack: // fall-through
	case InputButton::KeySpace: // fall-through
	case InputButton::KeyTab: // fall-through
	case InputButton::KeyLeft: // fall-through
	case InputButton::KeyRight: // fall-through
	case InputButton::KeyUp: // fall-through
	case InputButton::KeyDown: // fall-through
	case InputButton::KeyPageUp: // fall-through
	case InputButton::KeyPageDown: // fall-through
	case InputButton::KeyHome: // fall-through
	case InputButton::KeyEnd: // fall-through
	case InputButton::KeyDelete: // fall-through
	case InputButton::KeyBackspace: // fall-through
	case InputButton::KeyEnter: // fall-through
	case InputButton::KeyEscape: // fall-through
	case InputButton::KeyF1: // fall-through
	case InputButton::KeyF2: // fall-through
	case InputButton::KeyF3: // fall-through
	case InputButton::KeyF4: // fall-through
	case InputButton::KeyF5: // fall-through
	case InputButton::KeyF6: // fall-through
	case InputButton::KeyF7: // fall-through
	case InputButton::KeyF8: // fall-through
	case InputButton::KeyF9: // fall-through
	case InputButton::KeyF10: // fall-through
	case InputButton::KeyF11: // fall-through
	case InputButton::KeyF12: // fall-through
	case InputButton::KeyA: // fall-through
	case InputButton::KeyB: // fall-through
	case InputButton::KeyC: // fall-through
	case InputButton::KeyD: // fall-through
	case InputButton::KeyE: // fall-through
	case InputButton::KeyF: // fall-through
	case InputButton::KeyG: // fall-through
	case InputButton::KeyH: // fall-through
	case InputButton::KeyI: // fall-through
	case InputButton::KeyJ: // fall-through
	case InputButton::KeyK: // fall-throug
	case InputButton::KeyL: // fall-through
	case InputButton::KeyM: // fall-through
	case InputButton::KeyN: // fall-through
	case InputButton::KeyO: // fall-through
	case InputButton::KeyP: // fall-through
	case InputButton::KeyQ: // fall-through
	case InputButton::KeyR: // fall-through
	case InputButton::KeyS: // fall-through
	case InputButton::KeyT: // fall-through
	case InputButton::KeyU: // fall-through
	case InputButton::KeyV: // fall-through
	case InputButton::KeyW: // fall-through
	case InputButton::KeyX: // fall-through
	case InputButton::KeyY: // fall-through
	case InputButton::KeyZ:
		bReturn = bReturn || io.WantCaptureKeyboard || io.WantTextInput;
		break;

	case InputButton::KeyLeftAlt: // fall-through
	case InputButton::KeyRightAlt:
		io.AddKeyEvent(ImGuiKey_ModAlt, (ButtonEventType::ButtonReleased != eEventType));
		bReturn = bReturn || io.WantCaptureKeyboard || io.WantTextInput;
		break;

	case InputButton::KeyLeftControl: // fall-through
	case InputButton::KeyRightControl:
		io.AddKeyEvent(ImGuiKey_ModCtrl, (ButtonEventType::ButtonReleased != eEventType));
		bReturn = bReturn || io.WantCaptureKeyboard || io.WantTextInput;
		break;

	case InputButton::KeyLeftShift: // fall-through
	case InputButton::KeyRightShift:
		io.AddKeyEvent(ImGuiKey_ModShift, (ButtonEventType::ButtonReleased != eEventType));
		bReturn = bReturn || io.WantCaptureKeyboard || io.WantTextInput;
		break;

	case InputButton::KeyLeftWindows: // fall-through
	case InputButton::KeyRightWindows:
		io.AddKeyEvent(ImGuiKey_ModSuper, (ButtonEventType::ButtonReleased != eEventType));
		bReturn = bReturn || io.WantCaptureKeyboard || io.WantTextInput;
		break;

	case InputButton::MouseLeftButton:
		// Particularly for mobile, make sure mouse position is refreshed on button events.
		{
			auto const mousePos = InputManager::Get()->GetMousePosition();
			(void)HandleMouseMoveEvent(mousePos.X, mousePos.Y);
		}
		io.AddMouseButtonEvent(0, (ButtonEventType::ButtonReleased != eEventType));
		bReturn = bReturn || io.WantCaptureMouse || (io.MouseDown[0] && ImGui::WillWantCaptureMousePos(ImGui::GetIO().MousePos));
		break;
	case InputButton::MouseMiddleButton:
		// Particularly for mobile, make sure mouse position is refreshed on button events.
		{
			auto const mousePos = InputManager::Get()->GetMousePosition();
			(void)HandleMouseMoveEvent(mousePos.X, mousePos.Y);
		}
		io.AddMouseButtonEvent(2, (ButtonEventType::ButtonReleased != eEventType));
		bReturn = bReturn || io.WantCaptureMouse || (io.MouseDown[2] && ImGui::WillWantCaptureMousePos(ImGui::GetIO().MousePos));
		break;
	case InputButton::MouseRightButton:
		// Particularly for mobile, make sure mouse position is refreshed on button events.
		{
			auto const mousePos = InputManager::Get()->GetMousePosition();
			(void)HandleMouseMoveEvent(mousePos.X, mousePos.Y);
		}
		io.AddMouseButtonEvent(1, (ButtonEventType::ButtonReleased != eEventType));
		bReturn = bReturn || io.WantCaptureMouse || (io.MouseDown[1] && ImGui::WillWantCaptureMousePos(ImGui::GetIO().MousePos));
		break;
	default:
		break;
	};

	// Capture handling.
	if (ButtonEventType::ButtonReleased != eEventType)
	{
		if (bReturn)
		{
			m_aKeysCaptured[(Int)eButtonID] = true;
		}
	}
	else
	{
		if (m_aKeysCaptured[(Int)eButtonID])
		{
			bReturn = true;
		}
		m_aKeysCaptured[(Int)eButtonID] = false;
	}

	// TODO: Adding back in touch-bottom-right-corner to bring up the dev UI, but only
	// in iOS Simulator.
	// TODO: I am putting this below the normal handling so we can toggle the dev UI
	// by touching an empty space, without accidentally dismissing while touching the dev UI.
#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)
#if SEOUL_DEVUI_MOBILE
	if (!bReturn)
	{
		Point2DInt mousePosition;
		mousePosition = InputManager::Get()->GetMousePosition();
		Viewport const backBufferViewport = RenderDevice::Get()->GetBackBufferViewport();
		PlatformData data;
		Engine::Get()->GetPlatformData(data);
		// Create the hit area in terms of inches rather than pixels if possible,
		// helps on devices with small screens and high PPI
		Point2DInt hitArea;
		if (!IsZero(data.m_vScreenPPI.X) && !IsZero(data.m_vScreenPPI.Y))
		{
			// Hard-coded to a quarter of an inch
			hitArea = Point2DInt((Int) (0.25f * data.m_vScreenPPI.X), (Int)(0.25f * data.m_vScreenPPI.Y));
		}
		else
		{
			hitArea = Point2DInt((Int)(backBufferViewport.m_iTargetWidth * 0.05), (Int)(backBufferViewport.m_iTargetHeight * 0.05));
		}

		Bool const bPressed = (eEventType == ButtonEventType::ButtonPressed || eEventType == ButtonEventType::ButtonRepeat);
		Bool const bInRegion = (
			mousePosition.X > backBufferViewport.m_iViewportX + backBufferViewport.m_iViewportWidth - hitArea.X &&
			mousePosition.Y > backBufferViewport.m_iViewportY + backBufferViewport.m_iViewportHeight - hitArea.Y);

		// Only toggle if touching within the bottom-right region.
		if (bInRegion)
		{
			if ((InputButton::MouseLeftButton == eButtonID || InputButton::TouchButtonFirst == eButtonID) && bPressed)
			{
				m_bMainMenuVisible = !m_bMainMenuVisible;
				return true;
			}
		}
	}
#endif // /SEOUL_DEVUI_MOBILE
#endif // /#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)

	return bReturn;
}

Bool Root::HandleMouseMoveEvent(
	Int iX,
	Int iY)
{
	SEOUL_ASSERT(IsMainThread());

	ImGuiIO& io = ImGui::GetIO();

	// Apply new mouse position.
	auto const viewport = RenderDevice::Get()->GetBackBufferViewport();
	io.AddMousePosEvent(
		(Float)(iX - viewport.m_iViewportX) * m_fMouseScale,
		(Float)(iY - viewport.m_iViewportY) * m_fMouseScale);

#if SEOUL_DEVUI_MOBILE
	// If on a mobile platform, also set MousePosPrev when a button is
	// not depressed. This avoids sudden mouse changes.
	if (!io.MouseDown[0])
	{
		io.MousePosPrev = io.MousePos;
	}
#endif // /SEOUL_DEVUI_MOBILE

	// When captured, capture move also.
	auto bReturn = io.WantCaptureMouse;
	// Also deliver move to the active main form.
	if (m_pActiveMainForm)
	{
		m_pActiveMainForm->OnMouseMove(iX, iY, bReturn);
	}

	return bReturn;
}

void Root::TextEditableApplyChar(UniChar c)
{
	SEOUL_ASSERT(IsMainThread());

	ImGuiIO& io = ImGui::GetIO();

	// TODO: This is not necessarily correct on platforms with 16-bit wchar_t types.
	if (0 != iswprint((wchar_t)c))
	{
		Byte aUTF8[5];
		UInt32 const characters = (UInt32)UTF8EncodeChar(c, aUTF8);
		if (characters > 0)
		{
			aUTF8[characters] = '\0';
			io.AddInputCharactersUTF8(aUTF8);
		}
	}
}

void Root::TextEditableApplyText(const String& sText)
{
	SEOUL_ASSERT(IsMainThread());

	ImGuiIO& io = ImGui::GetIO();
	io.ClearInputCharacters();
	io.AddInputCharactersUTF8(sText.CStr());
	m_bPendingTextEditableApplyText = true; // Necessary to give the characters a chance to commit. Immediate-mode GUI at its clunkiest.
}

void Root::TextEditableEnableCursor()
{
	SEOUL_ASSERT(IsMainThread());

	// TODO:
}

void Root::TextEditableStopEditing()
{
	SEOUL_ASSERT(IsMainThread());
	if (!m_bPendingTextEditableApplyText)
	{
		// Kill immediately if not applied.
		ImGui::PublicClearActiveID();
	}
}

// IPoseable overrides
void Root::PrePose(Float fDeltaTimeInSeconds, RenderPass& rPass, IPoseable* pParent /*= nullptr*/)
{
	InternalDoTickBegin(rPass, fDeltaTimeInSeconds, pParent);

	// Begin tick views.
	if (m_pActiveMainForm)
	{
		m_pActiveMainForm->TickBegin(fDeltaTimeInSeconds);
	}

	// Tick the controller of the active main form.
	if (m_pActiveMainForm.IsValid())
	{
		m_pActiveMainForm->GetController().Tick(fDeltaTimeInSeconds);
	}

	// PrePose and prepare, mainly imgui.
	InternalPrePose(rPass, Engine::Get()->DevOnlyGetRawSecondsInTick());

	// End tick views.
	if (m_pActiveMainForm)
	{
		m_pActiveMainForm->TickEnd(fDeltaTimeInSeconds);
	}

	InternalDoTickEnd(rPass, fDeltaTimeInSeconds, pParent);

	// Save checking - ImGui controls frequency, so this only happens occasionally.
	SaveImGuiSettings();
}

void Root::Pose(Float fDeltaTimeInSeconds, RenderPass& rPass, IPoseable* pParent /*= nullptr*/)
{
	SEOUL_ASSERT(IsMainThread());

	// Draw.
	{
		auto& rBuilder = *rPass.GetRenderCommandStreamBuilder();
		BeginPass(rBuilder, rPass);
		InternalRender(rPass, rBuilder); // Draw DevUIRoot.
		EndPass(rBuilder, rPass);
	}
}

void Root::SkipPose(Float fDeltaTimeInSeconds)
{
	InternalDoSkipPose(fDeltaTimeInSeconds);
}

void Root::InternalPrePose(RenderPass& rPass, Float fDeltaTimeInSeconds)
{
	// Early out if a frame is already ready to go.
	if (m_bImGuiFrameReady)
	{
		return;
	}

	// Before any further processing, check for a mode change.
	InternalCheckForAndApplyModeChange(rPass);

	// Do the processing.
	InternalPrePoseImGuiFrame(rPass, fDeltaTimeInSeconds);

	// Update text input entry if necessary.
	if (m_bWantTextInput != ImGui::GetIO().WantTextInput)
	{
		m_bWantTextInput = ImGui::GetIO().WantTextInput;

		// Disable text editing.
		if (!m_bWantTextInput)
		{
			Engine::Get()->StopTextEditing(this);
		}
		// Enable text editing.
		else if (m_bWantTextInput)
		{
			// TODO:
			Engine::Get()->StartTextEditing(
				this,
				String(),
				"DevUI",
				StringConstraints(),
				false);
		}
	}

	// Set mouse position to -1, -1 on mobile if not capturing input.
#if SEOUL_DEVUI_MOBILE
	ImGuiIO& io = ImGui::GetIO();
	if (!io.MouseDown[0])
	{
		io.MousePos = ImVec2(-1, -1);
		io.MousePosPrev = io.MousePos;
	}
#endif // /SEOUL_DEVUI_MOBILE
}

void Root::InternalPrePoseImGuiFrame(RenderPass& rPass, Float fDeltaTimeInSeconds)
{
	// Frame will be ready once we exit this function.
	m_bImGuiFrameReady = true;

	// Cache rescale factor.
	Float const fRescale = m_fWindowScale;

	ImGuiIO& io = ImGui::GetIO();
	auto const viewport = RenderDevice::Get()->GetBackBufferViewport();

	// Configure per-frame Imgui values.
	io.DeltaTime = fDeltaTimeInSeconds;
	io.DisplaySize.x = (Float)viewport.m_iViewportWidth * fRescale;
	io.DisplaySize.y = (Float)viewport.m_iViewportHeight * fRescale;

	// Advance.
	ImGui::NewFrame();

	// Apply mobile scrolling - internally will early out
	// if not in a mobile configuration.
	ApplyMobileScrolling();

	// Give subclasses an injection point.
	InternalPrePoseImGuiFrameBegin(rPass, fDeltaTimeInSeconds);

	// PrePose
	if (m_bMainMenuVisible && !IsVirtualizedDesktop())
	{
		// Draw the main menu.
		InternalDrawMenuBar(true);
	}
	// Main form when in virtualized mode.
	else if (IsVirtualizedDesktop())
	{
		// Draw the virtual main form, which includes the main menu bar.
		InternalDrawVirtualizedMainForm();
	}

	// Now draw views.
	if (m_pActiveMainForm)
	{
		m_pActiveMainForm->PrePose(rPass, m_bMainMenuVisible || IsVirtualizedDesktop() || m_bAlwaysShowViews);
	}

	// Give subclasses an injection point.
	InternalPrePoseImGuiFrameEnd(rPass, fDeltaTimeInSeconds);

	// Commit pending now (after new frame, to give
	// characters a chance to commit).
	if (m_bPendingTextEditableApplyText)
	{
		m_bPendingTextEditableApplyText = false;
		ImGui::PublicClearActiveID();
	}

	// Update mouse cursor.
	InternalUpdateMouseCursor();
}

void Root::InternalRender(RenderPass& rPass, RenderCommandStreamBuilder& rBuilder)
{
	SEOUL_ASSERT(IsMainThread());

	// Early out if we don't have a frame to render.
	if (!m_bImGuiFrameReady)
	{
		return;
	}

	// Dispatch commands to the renderer.
	InternalRenderSubmit(rPass);

	// Done with the frame.
	m_bImGuiFrameReady = false;
}

void Root::InternalRenderSubmit(RenderPass& rPass)
{
	// Finalize render command data.
	ImGui::Render();

	// Early out if nothing to render.
	auto pDrawData = ImGui::GetDrawData();
	if (nullptr == pDrawData ||
		!pDrawData->Valid ||
		pDrawData->CmdListsCount == 0)
	{
		return;
	}

	if (!m_pImGuiRenderer->BeginFrame(rPass))
	{
		return;
	}

	// Submit command data.
	m_pImGuiRenderer->Render(*ImGui::GetDrawData(), kVirtualizedMainFormName);

	// Done.
	m_pImGuiRenderer->EndFrame();
}

void Root::InternalUpdateMouseCursor()
{
	// Commit the mouse cursor.
	{
		auto eMouseCursor = Engine::Get()->GetMouseCursor();
		switch (ImGui::GetMouseCursor())
		{
		case ImGuiMouseCursor_TextInput:
			eMouseCursor = MouseCursor::kIbeam;
			break;
		case ImGuiMouseCursor_ResizeNS:
			eMouseCursor = MouseCursor::kArrowUpDown;
			break;
		case ImGuiMouseCursor_ResizeEW:
			eMouseCursor = MouseCursor::kArrowLeftRight;
			break;
		case ImGuiMouseCursor_ResizeNESW:
			eMouseCursor = MouseCursor::kArrowLeftBottomRightTop;
			break;
		case ImGuiMouseCursor_ResizeNWSE:
			eMouseCursor = MouseCursor::kArrowLeftTopRightBottom;
			break;
		case ImGuiMouseCursor_Hand:
			eMouseCursor = MouseCursor::kMove; // TODO: Incorrect.
			break;

		case ImGuiMouseCursor_Arrow: // fall-through
		case ImGuiMouseCursor_None: // fall-through
		default:
			eMouseCursor = MouseCursor::kArrow;
			break;
		};
		Engine::Get()->SetMouseCursor(eMouseCursor);
	}
}

void Root::InternalMainMenuAsTitleBarControls(Bool bRootMainMenu)
{
	using namespace ImGui;

	// No controls if a root (OS window) main menu.
	if (bRootMainMenu)
	{
		return;
	}

	Bool bOpened = true;
	Bool bMinimized = RenderDevice::Get()->IsMinimized();
	auto const bOrigMinimized = bMinimized, bOrigMaximized = m_MainFormState.m_bMaximized;

	MainMenuAsTitleBarControls(bOpened, bMinimized, m_MainFormState.m_bMaximized);

	auto const bWindowMoving = IsWindowMoving();
	auto const bWindowResizing = IsWindowResizing();

	// bOpened = false means exit the entire program.
	if (!bOpened)
	{
		Engine::Get()->PostNativeQuitMessage();
	}

	// Minimized handling.
	if (bMinimized != bOrigMinimized)
	{
		RenderDevice::Get()->ToggleMinimized();
	}
	// If maximized and the window is moving, handle this.
	else if (
		m_MainFormState.m_bMaximized &&
		bWindowMoving &&
		IsMouseDragging(0) &&
		m_MainFormState.m_bMaximized == bOrigMaximized)
	{
		m_MainFormState.m_bMaximized = false;
	}
	// If maximized and the window is resizing, handle this.
	else if (
		m_MainFormState.m_bMaximized &&
		bWindowResizing &&
		IsMouseDragging(0) &&
		m_MainFormState.m_bMaximized == bOrigMaximized)
	{
		// In this case, we just switch back to not maximized but
		// leave the window unaltered.
		m_MainFormState = VirtualDesktopMainFormState{};
	}

	// Maximized/restore handling.
	if (m_MainFormState.m_bMaximized != bOrigMaximized)
	{
		if (bOrigMaximized)
		{
			if (!m_MainFormState.m_RestoreSize.IsZero())
			{
				SetWindowPos(kVirtualizedMainFormName, ImVec2(m_MainFormState.m_RestorePos.X, m_MainFormState.m_RestorePos.Y));
				SetWindowSize(kVirtualizedMainFormName, ImVec2(m_MainFormState.m_RestoreSize.X, m_MainFormState.m_RestoreSize.Y));
			}

			m_MainFormState.m_RestorePos = Vector2D();
			m_MainFormState.m_RestoreSize = Vector2D();
		}
		else
		{
			// Capture sizes.
			Rectangle2DInt target;
			if (GetWindowPosSizeByName(kVirtualizedMainFormName, m_MainFormState.m_RestorePos, m_MainFormState.m_RestoreSize, false) &&
				!m_MainFormState.m_RestoreSize.IsZero() &&
				RenderDevice::Get()->GetMaximumWorkAreaForRectangle(
					Rectangle2DInt(
						(Int)(m_MainFormState.m_RestorePos.X / m_fWindowScale),
						(Int)(m_MainFormState.m_RestorePos.Y / m_fWindowScale),
						(Int)((m_MainFormState.m_RestorePos.X + m_MainFormState.m_RestoreSize.X) / m_fWindowScale),
						(Int)((m_MainFormState.m_RestorePos.Y + m_MainFormState.m_RestoreSize.Y) / m_fWindowScale)),
					target))
			{
				// Account for resize margin.
				target.Expand((Int)-GetWindowsResizeFromEdgesHalfThickness());

				SetWindowPos(kVirtualizedMainFormName, ImVec2((Float)target.m_iLeft, (Float)target.m_iTop) * m_fWindowScale);
				SetWindowSize(kVirtualizedMainFormName, ImVec2((Float)target.GetWidth(), (Float)target.GetHeight()) * m_fWindowScale);
			}
		}
	}
}

void Root::InternalDrawVirtualizedMainForm()
{
#if !SEOUL_SHIP // Developer only support for virtualized desktop.
	using namespace ImGui;

	auto const uFlags =
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse;

	// Make sure the main form is on the bottom.
	SetNextWindowBringToDisplayBack();

	// Initial sizing.
	if (m_LastOsWindowSize.X > 2 && m_LastOsWindowSize.Y > 2)
	{
		// Account for resize margin.
		auto const margin = (Int)GetWindowsResizeFromEdgesHalfThickness();
		m_LastOsWindowPos.X += margin;
		m_LastOsWindowPos.Y += margin;
		m_LastOsWindowSize.X -= (2 * margin);
		m_LastOsWindowSize.Y -= (2 * margin);

		// Match the main form to the old OS window.
		auto const rect = RenderDevice::Get()->GetVirtualizedDesktopRect();
		auto const offsetX = rect.m_iLeft < 0 ? (Float)-rect.m_iLeft : 0.0f;
		auto const offsetY = rect.m_iTop < 0 ? (Float)-rect.m_iTop : 0.0f;
		SetNextWindowPos(ImVec2(offsetX + (Float)m_LastOsWindowPos.X, offsetY + (Float)m_LastOsWindowPos.Y) * m_fWindowScale);
		SetNextWindowSize(ImVec2((Float)m_LastOsWindowSize.X, (Float)m_LastOsWindowSize.Y) * m_fWindowScale);

		// Dispatch handling to subclass.
		InternalOnSwitchToVirtualizedDesktop();
	}

	// Clear.
	m_LastOsWindowPos = Point2DInt(0, 0);
	m_LastOsWindowSize = Point2DInt(0, 0);

	Bool bOpened = true;
	(void)Begin(kVirtualizedMainFormName, &bOpened, uFlags);
	InternalDrawMenuBar(false); // Draw the menu.
	auto const bStatusBar = InternalDrawStatusBar(false); // Draw the (optional) status bar.
	DockSpaceEx(bStatusBar, m_uMainFormDockSpaceID);
	End();

	// bOpened = false means exit the entire program.
	if (!bOpened)
	{
		Engine::Get()->PostNativeQuitMessage();
	}
#endif // /!SEOUL_SHIP
}

void Root::InternalLoadImGuiSettings(void* p, UInt32 u)
{
	// Reset setts that are stored in global state to their
	// defaults on load.
	m_fConfiguredInverseWindowScale = -1.0f;
	m_bMainMenuVisible = false;
	if (m_pActiveMainForm)
	{
		m_pActiveMainForm->ImGuiPrepForLoadSettings();
	}

	// Load settings now that views have been craeted.
	if (!LoadImGuiSettings(p, u))
	{
#if !SEOUL_SHIP // Virtual desktop only available in non-ship builds.
		// If we startup virtualized, apply the first toggle handling so windows are not crazy.
		if (IsVirtualizedDesktop())
		{
			Rectangle2DInt rect;
			if (RenderDevice::Get()->GetMaximumWorkAreaOnPrimary(rect))
			{
				// TODO: Straighten this out so it's more obvious/harder to screw up.
				// We need to place these values back in desktop space.
				auto const adj = RenderDevice::Get()->GetVirtualizedDesktopRect();

				m_LastOsWindowPos = Point2DInt(rect.m_iLeft + adj.m_iLeft, rect.m_iTop + adj.m_iTop);
				m_LastOsWindowSize = Point2DInt(rect.GetWidth(), rect.GetHeight());
			}
		}
#endif //!SEOUL_SHIP
	}
}

// Propagate various settings to ImGui.
void Root::InternalApplyModeToImGui()
{
	auto& io = ImGui::GetIO();

	InternalApplyStyle();

	// Per mode.
	switch (m_eCurrentMode)
	{
	case Mode::Mobile:
	{
		// Disable cursors and edge resize.
		io.BackendFlags &= ~ImGuiBackendFlags_HasMouseCursors;
		io.ConfigWindowsResizeFromEdges = false;

		// Docking disabled.
		io.ConfigFlags &= ~ImGuiConfigFlags_DockingEnable;

		// Center window does not allow drag.
		io.ConfigWindowsMoveFromTitleBarOnly = true;
	}
	break;
	case Mode::Desktop:
	case Mode::VirtualizedDesktop:
	{
		// Enable edge dragging and cursors in non-mobile
		// builds.
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.ConfigWindowsResizeFromEdges = true;

		// Enable docking in non-mobile builds.
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		// Center window allows drag.
		io.ConfigWindowsMoveFromTitleBarOnly = false;
	}
	break;
	};
}

void Root::InternalApplyStyle()
{
	auto& style = ImGui::GetStyle();

	// Style values.
	{
		style.FrameRounding = 3.0f;
	}

	// Colors.
	{
		auto& colors = style.Colors;

		const Vector3D vDim(0.80f, 0.80f, 0.80f);
		// TODO: Orig: const Vector3D vPrimary(0.26f, 0.59f, 0.98f);
		const Vector3D vPrimaryColor(0.941f, 0.318f, 0.200f);
		const Vector3D vPrimaryGray(0.491f, 0.491f, 0.491f);
		// TODO: Orig: const Vector3D vPrimaryBg(0.16f, 0.29f, 0.48f);
		// TODO: Color: const Vector3D vPrimaryBg(0.461f, 0.156f, 0.098f);
		const Vector3D vPrimaryBg(0.241f, 0.241f, 0.241f);
		// TODO: const Vector3D vSeparator(0.10f, 0.40f, 0.75f);
		const Vector3D vSeparator(0.75f, 0.40f, 0.10f);

		auto const gray = [](Float f, Float fAlpha) { return ImVec4(f, f, f, fAlpha); };
		auto const black = [gray](Float fAlpha) { return gray(0.00f, fAlpha); };
		auto const dim = [vDim](Float fAlpha) { return ToImVec4(vDim, fAlpha); };
		auto const primaryBg = [vPrimaryBg](Float fAlpha) { return ToImVec4(vPrimaryBg, fAlpha); };
		auto const primaryColor = [vPrimaryColor](Float fAlpha) { return ToImVec4(vPrimaryColor, fAlpha); };
		auto const primaryGray = [vPrimaryGray](Float fAlpha) { return ToImVec4(vPrimaryGray, fAlpha); };
		auto const sep = [vSeparator](Float fAlpha) { return ToImVec4(vSeparator, fAlpha); };
		auto const white = [gray](Float fAlpha) { return gray(1.0f, fAlpha); };

		colors[ImGuiCol_Border] = gray(0.35f, 0.50f);
		colors[ImGuiCol_BorderShadow] = black(0.00f);
		colors[ImGuiCol_Button] = primaryGray(0.40f);
		colors[ImGuiCol_ButtonActive] = primaryColor(1.00);
		colors[ImGuiCol_ButtonHovered] = primaryColor(0.78f);
		colors[ImGuiCol_CheckMark] = primaryColor(1.00f);
		colors[ImGuiCol_ChildBg] = black(0.00f);
		colors[ImGuiCol_DockingEmptyBg] = gray(0.20f, 1.00f);
		colors[ImGuiCol_DockingPreview] = primaryColor(0.70f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_FrameBg] = primaryBg(0.54f);
		colors[ImGuiCol_FrameBgActive] = primaryColor(0.67f);
		colors[ImGuiCol_FrameBgHovered] = primaryColor(0.40f);
		colors[ImGuiCol_Header] = primaryGray(0.31f);
		colors[ImGuiCol_HeaderActive] = primaryColor(1.00f);
		colors[ImGuiCol_HeaderHovered] = primaryColor(0.80f);
		colors[ImGuiCol_MenuBarBg] = gray(0.159f, 1.00f);
		colors[ImGuiCol_ModalWindowDimBg] = dim(0.35f);
		colors[ImGuiCol_NavHighlight] = primaryColor(1.00f);
		colors[ImGuiCol_NavWindowingDimBg] = dim(0.20f);
		colors[ImGuiCol_NavWindowingHighlight] = white(0.70f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotLines] = gray(0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PopupBg] = gray(0.08f, 0.94f);
		colors[ImGuiCol_ResizeGrip] = primaryGray(0.20f);
		colors[ImGuiCol_ResizeGripActive] = primaryColor(0.95f);
		colors[ImGuiCol_ResizeGripHovered] = primaryColor(0.67f);
		colors[ImGuiCol_ScrollbarBg] = gray(0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = gray(0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = gray(0.51f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = gray(0.41f, 1.00f);
		colors[ImGuiCol_SliderGrab] = primaryGray(0.40f);
		colors[ImGuiCol_SliderGrabActive] = primaryColor(1.00f);
		colors[ImGuiCol_TableBorderLight] = gray(0.25f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = gray(0.35f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = gray(0.20f, 1.00f);
		colors[ImGuiCol_TableRowBg] = black(0.00f);
		colors[ImGuiCol_TableRowBgAlt] = white(0.06f);
		colors[ImGuiCol_Text] = white(1.00f);
		colors[ImGuiCol_TextDisabled] = gray(0.50f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = primaryColor(0.35f);
		colors[ImGuiCol_TitleBg] = gray(0.04f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = primaryBg(1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = black(0.51f);
		colors[ImGuiCol_WindowBg] = gray(0.098f, 0.94f);

		// Derived values.
		colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
		colors[ImGuiCol_SeparatorActive] = sep(1.00f);
		colors[ImGuiCol_SeparatorHovered] = sep(0.78f);

		colors[ImGuiCol_Tab] = Lerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
		colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
		colors[ImGuiCol_TabActive] = Lerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
		colors[ImGuiCol_TabUnfocused] = Lerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
		colors[ImGuiCol_TabUnfocusedActive] = Lerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
	}
}

// Before any further processing, check for a mode change.
void Root::InternalCheckForAndApplyModeChange(RenderPass& rPass)
{
#if !SEOUL_SHIP // Virtualization only available in developer builds.
	// Virtualization support is kept in-sync with the render device.
	if ((m_eDesiredMode == Mode::VirtualizedDesktop) != RenderDevice::Get()->IsVirtualizedDesktop())
	{
		if (RenderDevice::Get()->IsVirtualizedDesktop())
		{
			m_eDesiredMode = Mode::VirtualizedDesktop;
		}
		else
		{
#if SEOUL_DEVUI_MOBILE
			m_eDesiredMode = Mode::Mobile;
#else
			m_eDesiredMode = Mode::Desktop;
#endif
		}
	}
#endif // /!SEOUL_SHIP

	// Change due to mode.
	auto bChanged = (m_eCurrentMode != m_eDesiredMode);
	// Update window scale.
	m_fWindowScale = ComputeOrReadWindowScale();
	// If mouse is not pressed, update mouse scale - we also
	// use this as a trigger to recompute the font atlas
	// for the next scale (which means an entire reload/refresh
	// of ImGui).
	if (!ImGui::IsMouseDown(0))
	{
		bChanged = bChanged || (m_fMouseScale != m_fWindowScale);
		m_fMouseScale = m_fWindowScale;
	}

	// Now check for a desired vs. current change and apply handling.
	if (bChanged)
	{
		// Backup, used to make sure the discarded frame is realistic.
		// In case it's zero (for some unexpected reason), clamp, since
		// ImGui will assert if a delta time of 0 is provided.
		auto const fDeltaTime = Max(ImGui::GetIO().DeltaTime, 0.0001f);

		HashTable<UInt64, Int, MemoryBudgets::DevUI> t;

		// Force a save prior to mode switch. This will
		// save to the layout file appropriate for the mode.
		ImGui::SaveStateStorage(t); // Save trees/collapsables.
		SaveImGuiSettings(true);

		// Apply mode.
		m_eCurrentMode = m_eDesiredMode;

		// Reset ImGui - tear down, then recreate,
		// reinitialize font texture, then reapply config.
		InternalStaticShutdownImGui();
		InternalStaticInitializeImGui();
		m_pImGuiRenderer->ReInitFontTexture();
		InternalLoadImGuiSettings();

		// Update window scale.
		m_fWindowScale = ComputeOrReadWindowScale();
		// If mouse is not pressed, update mouse scale.
		if (!ImGui::IsMouseDown(0))
		{
			m_fMouseScale = m_fWindowScale;
		}

		// Finish ImGui setup.
		InternalApplyModeToImGui();

		// Dummy frame to reinitialize.
		InternalPrePoseImGuiFrame(rPass, fDeltaTime);
		ImGui::EndFrame(); // This discards the frame without render.
		m_bImGuiFrameReady = false; // No longer a frame ready.

		// Restore state (trees/collapsables).
		ImGui::LoadStateStorage(t);
	}
}

static const Float kMobileScrollingDamping = 0.9f;

void Root::ApplyMobileScrolling()
{
	// Early check.
	if (!IsMobile())
	{
		return;
	}

	using namespace ImGui;

	auto& io = GetIO();

	// Query for the current active window.
	UInt32 uActiveWindowId = 0;
	// If we're scrolling, use the cached id.
	if (m_bMobileScrolling)
	{
		uActiveWindowId = m_uMobileScrollingWindowId;
	}
	// Otherwise, query - if this fails, kill all scrolling.
	else
	{
		if (!GetActiveWindowID(uActiveWindowId))
		{
			KillMobileScrolling();
			return;
		}
	}

	// Entire face drag to scroll.
	if (IsMouseDragging(0))
	{
		auto const res = ApplyMobileScrollingDelta(uActiveWindowId, Vector2D(io.MouseDelta.x, io.MouseDelta.y), false);
		if (res.First || res.Second)
		{
			// Any movement engages mobile scrolling. This prevents clicks from activating
			// elements.
			m_bMobileScrolling = true;
			// Cache active.
			m_uMobileScrollingWindowId = uActiveWindowId;
		}

		// Skip velocity compute if no passage of time.
		if (!IsZero(io.DeltaTime))
		{
			// Update velocity based on last fling/delta.
			m_vMobileScrollingVelocity = Vector2D(io.MouseDelta.x, io.MouseDelta.y) / io.DeltaTime;
		}
	}
	else if (m_bMobileScrolling)
	{
		// Mouse down while scrolling but not dragging
		// means a finger has been placed. This kill scrolling,
		// unless we start dragging again.
		if (IsMouseDown(0))
		{
			KillMobileScrolling();
		}
		// Otherwise, apply velocity.
		else
		{
			auto const vDelta = m_vMobileScrollingVelocity * io.DeltaTime;
			auto const res = ApplyMobileScrollingDelta(uActiveWindowId, vDelta, true);
			// Kill along individual axes.
			if (!res.First) { m_vMobileScrollingVelocity.X = 0.0f; }
			if (!res.Second) { m_vMobileScrollingVelocity.Y = 0.0f; }
			if (!res.First && !res.Second)
			{
				KillMobileScrolling(); // No delta, kill.
			}
			else
			{
				m_vMobileScrollingVelocity *= kMobileScrollingDamping; // Linear damping to bring to rest.
			}
		}
	}

	if (m_bMobileScrolling)
	{
		// Don't allow activate.
		PublicClearActiveID();
	}
}

typedef bool(*SetWindowScrollFunc)(ImGuiID, float);
static const SetWindowScrollFunc SetWindowScrollFuncs[2] =
{
	ImGui::SetWindowScrollX,
	ImGui::SetWindowScrollY,
};

Pair<Bool, Bool> Root::ApplyMobileScrollingDelta(UInt32 uWindowId, const Vector2D& v, Bool bVelocity)
{
	using namespace ImGui;

	FixedArray<Float, 2> aScroll;
	FixedArray<Float, 2> aScrollMax;
	if (!GetWindowScrollValues(uWindowId, aScroll[0], aScrollMax[0], aScroll[1], aScrollMax[1]))
	{
		return MakePair(false, false);
	}

	FixedArray<Bool, 2> ret;
	for (Int i = 0; i < 2; ++i)
	{
		auto const delta = v[i];
		if (IsZero(delta))
		{
			continue;
		}

		auto const scroll = aScroll[i];
		auto const scrollmax = aScrollMax[i];
		auto const newscroll = Clamp(scroll - delta, 0.0f, scrollmax);

		// If we're scrolling with velocity, the change
		// must change by a whole pixel. Otherwise, any change
		// is considered.
		Bool bChanged = false;
		if (bVelocity)
		{
			bChanged = (Int)scroll != (Int)newscroll;
		}
		else
		{
			bChanged = (scroll != newscroll);
		}

		if (bChanged && SetWindowScrollFuncs[i](uWindowId, newscroll))
		{
			ret[i] = true;
		}
	}

	return MakePair(ret[0], ret[1]);
}

void Root::KillMobileScrolling()
{
	m_bMobileScrolling = false;
	m_uMobileScrollingWindowId = 0;
	m_vMobileScrollingVelocity = Vector2D::Zero();
}

} // namespace DevUI

} // namespace Seoul

#endif // /SEOUL_ENABLE_DEV_UI
