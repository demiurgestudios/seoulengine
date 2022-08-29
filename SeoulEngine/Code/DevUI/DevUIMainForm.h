/**
 * \file DevUIMainForm.h
 * \brief Base interface for a main form, implements a collection of views
 * and exposing the controller for those views (in the model-view-controller).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEV_UI_MAIN_FORM_H
#define DEV_UI_MAIN_FORM_H

#include "InputDevice.h"
#include "Prereqs.h"
#include "SeoulHString.h"
#include "SeoulString.h"
#include "Vector.h"
namespace Seoul { namespace DevUI { class Controller; } }
namespace Seoul { namespace DevUI { class View; } }
namespace Seoul { class RenderPass; }
namespace Seoul { namespace Reflection { class Type; } }
struct ImGuiTextBuffer;

namespace Seoul::DevUI
{

class MainForm SEOUL_ABSTRACT
{
public:
	virtual ~MainForm();

	/** Get the controller for this main form. */
	virtual Controller& GetController() = 0;

	// Settings hooks.
	virtual void ImGuiPrepForLoadSettings() {}
	void ImGuiReadLine(void* pEntry, Byte const* sLine);
	void* ImGuiReadOpen(HString id);
	void ImGuiWriteAll(ImGuiTextBuffer* buf);

	Bool OnMouseButton(InputDevice* pInputDevice, InputButton eButtonID, ButtonEventType eEventType, CheckedPtr<View>* ppCaptureView = nullptr);
	void OnMouseMove(Int iX, Int iY, Bool bWillCapture);
	Bool OnMouseWheel(InputDevice* pInputDevice, InputDevice::Axis* pAxis, CheckedPtr<View>* ppCaptureView = nullptr);
	Bool OnKeyPressed(InputButton eButton, UInt32 uModifiers, CheckedPtr<View>* ppCaptureView = nullptr);

	// Entry point for the main form to prepare and
	// enqueue render operations.
	void PrePose(RenderPass& rPass, Bool bVisible);

	// Entry point for the main form to fill in its portion
	// of the DevUI's main menu. The DevUI fills
	// out common portions of the main menu then
	// calls this to fill in the form specific portions.
	virtual void PrePoseMainMenu() = 0;

	// Entry point for the main form to fill in its portion
	// of the &Windows menu of the main menu. Called
	// as the construction of that menu is in progress.
	void PrePoseWindowsMenu();

	// Entry point for ticking all views owned by this MainForm.
	void TickBegin(Float fDeltaTimeInSeconds);
	void TickEnd(Float fDeltaTimeInSeconds);

	struct ViewEntry SEOUL_SEALED
	{
		ViewEntry()
			: m_pView(nullptr)
			, m_sPrunedName()
			, m_Name()
			, m_bLastOpen(false)
		{
		}

		Bool operator<(const ViewEntry& b) const
		{
			return (m_sPrunedName < b.m_sPrunedName);
		}

		CheckedPtr<View> m_pView;
		String m_sPrunedName;
		HString m_Name;
		Bool m_bLastOpen;
	}; // struct ViewEntry
	typedef Vector<ViewEntry, MemoryBudgets::DevUI> Views;

protected:
	MainForm(const Views& vViews);

	const Views& GetViews() const { return m_vViews; }

	Bool InternalFindBestOpened(ViewEntry const** pp) const;
	HString InternalFindFirstOpened() const;

private:
	Views m_vViews;

	SEOUL_DISABLE_COPY(MainForm);
}; // class DevUI::MainForm

} // namespace Seoul::DevUI

#endif // include guard
