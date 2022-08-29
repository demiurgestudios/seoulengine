/**
 * \file DevUIView.h
 * \brief Base interface for a view. Views
 * are various developer UI components that can be rendered
 * with immediate mode UI.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEV_UI_VIEW_H
#define DEV_UI_VIEW_H

#include "InputDevice.h"
#include "InputKeys.h"
#include "Prereqs.h"
#include "SeoulHString.h"
#include "Vector2D.h"

#if SEOUL_ENABLE_DEV_UI
namespace Seoul { namespace DevUI { class ImGuiRenderer; } }
namespace Seoul { namespace DevUI { class Controller; } }
namespace Seoul { class RenderPass; }

namespace Seoul::DevUI
{

// Interface of a view, combined
// together to make up a view in
// an IView.
class View SEOUL_ABSTRACT
{
public:
	virtual ~View();

	/**
	 * Special case - views that return true for this value
	 * cannot be closed and are always visible.
	 *
	 * Views that return true for IsAlwaysOpen() are special cases
	 * (e.g. Game UI).
	 */
	virtual Bool IsAlwaysOpen() const { return false; }

	// Return the label to use for this view, when it is
	// docked and has a header label.
	virtual HString GetId() const = 0;

	/** @return true if this view is currently open/visible, false otherwise. */
	Bool IsOpen() const
	{
		return m_bDesiredOpen || IsAlwaysOpen();
	}

	// Colletion of "raw" input handlers for special cases. NOTE: behavior is as follows:
	// - OnKeyPressed is delivered always from DevUI.
	// - OnMouseWheel is delivered always from DevUI.
	// - OnMouseButton is delievered on a mouse down *only*, *unless* a view returned
	//   true from a previous mouse down case. If so, then that view will also receive
	//   an OnMouseButton call on mouse up (the view is considered to have captured that
	//   mouse button).
	// - OnMouseMove is delivered always from DevUI.

	virtual Bool OnMouseButton(
		InputDevice* pInputDevice,
		InputButton eButtonID,
		ButtonEventType eEventType)
	{
		// Nop
		return false;
	}

	virtual void OnMouseMove(Int iX, Int iY, Bool bWillCapture)
	{
		// Nop.
	}

	virtual Bool OnMouseWheel(
		InputDevice* pInputDevice,
		InputDevice::Axis* pAxis)
	{
		// Nop
		return false;
	}

	virtual Bool OnKeyPressed(InputButton eButton, UInt32 uModifiers)
	{
		// Nop
		return false;
	}

	// Entry point for the view to prepare and enqueue
	// render operations. Only called when the view is open.
	// Must happen during "pre-pose" (part of the Tick pass)
	// since ImGui actions can have side effects that mutate
	// Falcon graph state (or the simulation in general).
	void PrePose(Controller& rController, RenderPass& rPass, Bool bVisible);

	/** Set or unset the visibility of the view. */
	void SetOpen(Bool bOpen)
	{
		m_bDesiredOpen = bOpen;
	}

	// Entry point for updating the view. Called every frame,
	// even if the view is not open.
	void Tick(Controller& rController, Float fDeltaTimeInSeconds);

protected:
	View();

	// Allows temporary (or permanent) disable of a Window's
	// close button.
	virtual Bool IsCloseable() const { return true; }

	// Optional entry point, occurs immediately before
	// the views window is setup.
	virtual void PreBegin()
	{
		// Nop
	}
	// Optional entry point, occurs immediately after
	// the view window is finished.
	virtual void PostEnd()
	{
	}

	// Entry point for the view to prepare and enqueue
	// render operations.
	virtual void DoPrePose(Controller& rController, RenderPass& rPass) = 0;

	// Optional entry point, called always, even
	// if the window is not visible,
	// prior to the context of DoPrePose() (unlike
	// DoPrePose(), this function is not wrapped
	// in the view's window).
	virtual void DoPrePoseAlways(Controller& rController, RenderPass& rPass, Bool bVisible)
	{
		// Nop
	}

	// Optional entry point, called when the window contents
	// will not draw (either because the entire developer UI is hidden,
	// the window is hidden, or the window is collapsed).
	virtual void DoSkipPose(Controller& rController, RenderPass& rPass)
	{
		// Nop
	}

	// Optional entry point, per-frame update work.
	virtual void DoTick(Controller& rController, Float fDeltaTimeInSeconds)
	{
		// Nop
	}
	virtual UInt32 GetFlags() const
	{
		return 0u;
	}
	virtual Bool GetInitialPosition(Vector2D& rv) const
	{
		rv = Vector2D(20.0f, 40.0f);
		return true;
	}
	virtual Vector2D GetInitialSize() const
	{
		return Vector2D::Zero();
	}

private:
	Bool m_bDesiredOpen;

	SEOUL_DISABLE_COPY(View);
}; // class DevUI::View

} // namespace Seoul::DevUI

#endif // /SEOUL_ENABLE_DEV_UI

#endif // include guard
