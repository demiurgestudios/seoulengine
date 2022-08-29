/**
 * \file DevUIRoot.h
 * \brief Base class of the global singleton used for developer UI.
 * Currently specialized for runtime app builds (GameDevUI) and
 * SeoulEditor.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEV_UI_ROOT_H
#define DEV_UI_ROOT_H

#include "Atomic32.h"
#include "Delegate.h"
#include "FilePath.h"
#include "InputDevice.h"
#include "ITextEditable.h"
#include "IPoseable.h"
#include "List.h"
#include "Mutex.h"
#include "Singleton.h"
#include "Vector.h"
struct ImGuiContext;
struct ImGuiSettingsHandler;
struct ImGuiTextBuffer;
namespace Seoul { namespace DevUI { class ImGuiRenderer; } }
namespace Seoul { namespace DevUI { class MainForm; } }
namespace Seoul { namespace DevUI { class View; } }
namespace Seoul { namespace Reflection { class Type; } }
namespace Seoul { class VideoCapture; }

#if SEOUL_ENABLE_DEV_UI

namespace Seoul::DevUI
{

enum class Mode
{
	/* Setup oriented towards mobile - windows are immovable and only one can be active at a time over
	 * the game window. Tapping the background of a window is a scrolling action. */
	Mobile,

	/* Standard desktop mode - game is full screen in a standard desktop window and developer UI is
	 * overlayed on top. */
	Desktop,

	/* Virtualized desktop mode - game window has no OS decoration (the chrome is removed) and fills
	 * the entire OS virtual desktop area. The game is itself in a developer UI window, and
	 * all windows can effectively float by usage of OS Widow regions to cut out areas in between
	 * developer UI windows. */
	VirtualizedDesktop,
};

enum class Type
{
	Editor,
	Game,
};

// TODO: Note that DevUI::Root is a Singleton *only* because
// the "dear imgui" is currently not encapsulated and uses
// a global state that is not thread-safe. This is a known
// issue and is in the pipeline to be resolved:
// https://github.com/ocornut/imgui/issues/586
//
// If we have an immediate need for multiple "dear imgui" contexts,
// we can use the TLS variable storage as suggested.
class Root SEOUL_ABSTRACT : public ITextEditable, public IPoseable, public Singleton<Root>
{
public:
	SEOUL_DELEGATE_TARGET(Root);

	/** Name of the main form window in virtualized mode (in non-virtualized mode, we use a regular OS window). */
	static Byte const* const kVirtualizedMainFormName;
	static Byte const* const kVirtualizedMainFormDockSpaceName;

	Type GetType() const { return m_eType; }

	// Bindings for save/load to/from ImGui's ini file.
	static void* ImGuiReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char*);
	static void ImGuiReadLine(ImGuiContext*, ImGuiSettingsHandler*, void*, const char*);
	static void ImGuiWriteAll(ImGuiContext*, ImGuiSettingsHandler*, ImGuiTextBuffer*);

	virtual ~Root();

	Bool AlwaysShowViews() const { return m_bAlwaysShowViews; }

	Bool IsMainMenuVisible() const { return m_bMainMenuVisible; }

	void SetMainMenuVisible(Bool bVisible)
	{
		m_bMainMenuVisible = bVisible;
	}
	virtual void DisplayNotification(const String& sMessage) = 0;
	virtual void DisplayTrackedNotification(const String& sMessage, Int32& riId) = 0;
	virtual void KillNotification(Int32 iId) = 0;

	/**
	 * Current mode of the dev UI - dev UI has a few different configurations
	 * specialized for (e.g.) mobile vs. desktop and virtualized desktop
	 * vs. regular desktop (dev UI overlayed over game rendering).
	 */
	Mode GetMode() const
	{
		return m_eCurrentMode;
	}
	Bool IsDesktop() const { return (Mode::Desktop == m_eCurrentMode); }
	Bool IsMobile() const { return (Mode::Mobile == m_eCurrentMode); }
	Bool IsVirtualizedDesktop() const { return (Mode::VirtualizedDesktop == m_eCurrentMode); }

	/**
	 * For manual key checks, can be used to capture the input
	 * and prevent pass through to the game UI.
	 */
	void CaptureKey(InputButton eButtonID)
	{
		m_aKeysCaptured[(int)eButtonID] = true;
	}

	/**
	 * Identical to GetWindowScale(), but for mouse positions.
	 *
	 * When the mouse is not pressed, this value will always be identical to GetWindowScale().
	 * However, when the mouse is pressed, update of this value is deferred until the mouse
	 * is released. This allows the scale to be modified by ImGui UI, including a draggable
	 * float value, while maintaining a stable mouse position.
	 */
	Float GetMouseScale() const
	{
		return m_fMouseScale;
	}

	/** Get the internal developer UI renderer. */
	ImGuiRenderer& GetRenderer() { return *m_pImGuiRenderer; }

	/**
	 * Scaling - 1.0f means ImGui pixels are 1:1 with render viewport pixels
	 * Smaller values indicates ImGui pixels are *larger* than render
	 * viewport pixels.
	 */
	Float GetWindowScale() const
	{
		return m_fWindowScale;
	}

	/**
	 * For user settings - inverse since this is more natural to humans.
	 *
	 * -1 indicates that the window scale should be procedurally determined
	 * from platform defaults and system DPI.
	 */
	Float GetConfiguredInverseWindowScale() const
	{
		return m_fConfiguredInverseWindowScale;
	}

	// Reset to the default, which allows procedural configuration.
	void ResetConfiguredInverseWindowScale()
	{
		m_fConfiguredInverseWindowScale = -1.0f;
	}

	// Update the user window scale override.
	void SetConfiguredInverseWindowScale(Float fScale);

protected:
	Root(Type eType, MainForm* (*createInitialMainForm)());

	/**
	 * Called by DevUI::Root in SkipPose, allows subclasses to hook into this event.
	 */
	virtual void InternalDoSkipPose(Float fDeltaTimeInSeconds) {}

	/**
	 * Called by DevUI::Root at the very top of InternalDoTick(), allows subclasses to hook
	 * into this event.
	 */
	virtual void InternalDoTickBegin(RenderPass& rPass, Float fDeltaTimeInSeconds, IPoseable* pParent) {}

	/**
	 * Called by DevUI::Root at the end of InternalDoTick(), allows subclasses to hook
	 * into this event.
	 */
	virtual void InternalDoTickEnd(RenderPass& rPass, Float fDeltaTimeInSeconds, IPoseable* pParent) {}

	/**
	 * Common entry for the main menu - either a global main menu (in ImGui terms) or
	 * part of the MainForm window, depending on the current mode of the DevUIRoot.
	 */
	virtual void InternalDrawMenuBar(Bool bRootMainMenu) = 0;

	/**
	 * Called by subclasses when they have completed populating their menu bars, to conditionally
	 * add controls to the menu bar in situations where the menu bar is doubling as a windo'ws title
	 * bar.
	 */
	void InternalMainMenuAsTitleBarControls(Bool bRootMainMenu);

	/**
	 * Hook for drawing a status bar in the main form window - return value indicates whether
	 * the status bar was drawn or not.
	 */
	virtual Bool InternalDrawStatusBar(Bool bRootStatusBar) { return false; }

	/* Called when non-virtualized desktop switches to a virtualized desktop. */
	virtual void InternalOnSwitchToVirtualizedDesktop() {}

	/**
	 * Called by DevUI::Root at the beginning of InternalPrePoseImGuiFrame(), allows subclasses to hook
	 * into this event.
	 */
	virtual void InternalPrePoseImGuiFrameBegin(RenderPass& rPass, Float fDeltaTimeInSeconds) {}

	/**
	 * Called by DevUI::Root at the end of InternalPrePoseImGuiFrame(), allows subclasses to hook
	 * into this event.
	 */
	virtual void InternalPrePoseImGuiFrameEnd(RenderPass& rPass, Float fDeltaTimeInSeconds) {}

private:
	Type const m_eType;
protected:
	UInt32 const m_uMainFormDockSpaceID;
private:
	struct VirtualDesktopMainFormState
	{
		Vector2D m_RestorePos{};
		Vector2D m_RestoreSize{};
		Bool m_bMaximized = false;
	} m_MainFormState;
	Point2DInt m_LastOsWindowPos;
	Point2DInt m_LastOsWindowSize;
	Mode m_eCurrentMode;
	Mode m_eDesiredMode;
	FixedArray<CheckedPtr<View>, 3> m_aMouseCaptures;
	FixedArray<Bool, 512> m_aKeysCaptured;
	ScopedPtr<ImGuiRenderer> m_pImGuiRenderer;
protected:
	typedef Vector<CheckedPtr<MainForm>, MemoryBudgets::Editor> MainForms;
	MainForms m_vMainForms;
	CheckedPtr<MainForm> m_pActiveMainForm;
	Bool m_bMainMenuVisible;
	Bool m_bAlwaysShowViews;
private:
	Bool m_bWantTextInput;
	Bool m_bImGuiFrameReady;
	Bool m_bPendingTextEditableApplyText;

	// InputManager hooks.
	Bool HandleAxisEvent(InputDevice* /*pInputDevice*/, InputDevice::Axis* pAxis);
	Bool HandleButtonEvent(InputDevice* /*pInputDevice*/, InputButton eButtonID, ButtonEventType eEventType);
	Bool HandleMouseMoveEvent(Int iX, Int iY);
	// /InputManager hooks.

	// ITextEditable overrides
	virtual void TextEditableApplyChar(UniChar c) SEOUL_OVERRIDE;
	virtual void TextEditableApplyText(const String& sText) SEOUL_OVERRIDE;
	virtual void TextEditableEnableCursor() SEOUL_OVERRIDE;
	virtual void TextEditableStopEditing() SEOUL_OVERRIDE;
	// /ITextEditable overrides

	// IPoseable overrides
	virtual void PrePose(Float fDeltaTimeInSeconds, RenderPass& rPass, IPoseable* pParent = nullptr) SEOUL_OVERRIDE SEOUL_SEALED;
	virtual void Pose(Float fDeltaTimeInSeconds, RenderPass& rPass, IPoseable* pParent = nullptr) SEOUL_OVERRIDE SEOUL_SEALED;
	virtual void SkipPose(Float fDeltaTimeInSeconds) SEOUL_OVERRIDE SEOUL_SEALED;
	// /IPoseable overrides

	void InternalDrawVirtualizedMainForm();
	void InternalLoadImGuiSettings(void* p = nullptr, UInt32 u = 0u);
	void InternalApplyModeToImGui();
	void InternalApplyStyle();
	void InternalCheckForAndApplyModeChange(RenderPass& rPass);
	void InternalPrePose(RenderPass& rPass, Float fDeltaTimeInSeconds);
	void InternalPrePoseImGuiFrame(RenderPass& rPass, Float fDeltaTimeInSeconds);
	void InternalRender(RenderPass& rPass, RenderCommandStreamBuilder& rBuilder);
	void InternalRenderSubmit(RenderPass& rPass);
	void InternalUpdateMouseCursor();

	Bool m_bMobileScrolling = false;
	UInt32 m_uMobileScrollingWindowId = 0;
	Vector2D m_vMobileScrollingVelocity{};

	// For save/load from ImGui settings. -1 indicates
	// the window scale should be computed from DPI
	// or platform defaults.
	Float m_fConfiguredInverseWindowScale = -1.0f;

	// NOTE: Scaling factor applied - 1.0f indicates that render screen space
	// is 1:1 with ImGui pixels. Smaller values indicate that ImGui pixels
	// are *larger* than render screen space pixels.
	//
	// MouseScale is in most situations identical to WindowScale - however, if
	// the mouse is held, mouse scale update is deferred until the mouse is released.
	// This allows drag actions to remain stable, even if the drag is modifying the
	// scale.
	Float m_fMouseScale = 1.0f;
	Float m_fWindowScale = 1.0f;

	void ApplyMobileScrolling();
	Pair<Bool, Bool> ApplyMobileScrollingDelta(UInt32 uWindowId, const Vector2D& v, Bool bVelocity);
	void KillMobileScrolling();

	SEOUL_DISABLE_COPY(Root);
}; // class DevUI::Root

} // namespace Seoul::DevUI

#endif // /SEOUL_ENABLE_DEV_UI

#endif // include guard
