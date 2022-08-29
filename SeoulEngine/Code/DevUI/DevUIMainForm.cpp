/**
 * \file DevUIMainForm.cpp
 * \brief Base interface for a main form, implements a collection of views
 * and exposing the controller for those views (in the model-view-controller).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIImGui.h"
#include "DevUIMainForm.h"
#include "DevUIRoot.h"
#include "DevUIViewLog.h"
#include "DevUIView.h"
#include "ReflectionDefine.h"

namespace Seoul::DevUI
{

MainForm::MainForm(const Views& vViews)
	: m_vViews(vViews)
{
}

MainForm::~MainForm()
{
	// Cleanup views.
	for (Int32 i = (Int32)m_vViews.GetSize() - 1; i >= 0; --i)
	{
		ViewEntry& r = m_vViews[i];
		SafeDelete(r.m_pView);
	}
	m_vViews.Clear();
}

void MainForm::ImGuiReadLine(void* pEntry, Byte const* sLine)
{
	auto pViewEntry = (ViewEntry*)pEntry;
	if (nullptr == pViewEntry)
	{
		return;
	}

	// Common case - get the value.
	Int i = 0;
	if (SSCANF(sLine, "Enabled=%d", &i) == 1)
	{
		pViewEntry->m_pView->SetOpen(0 != i);
		pViewEntry->m_bLastOpen = (0 != i);
	}
}

void* MainForm::ImGuiReadOpen(HString id)
{
	// Search for matching id.
	for (auto& e : m_vViews)
	{
		if (e.m_pView->GetId() == id)
		{
			// Found, return entry.
			return &e;
		}
	}

	return nullptr;
}

void MainForm::ImGuiWriteAll(ImGuiTextBuffer* buf)
{
	for (auto const& e : m_vViews)
	{
		buf->appendf("[%s][%s]\n", "DevUI", e.m_pView->GetId().CStr());
		buf->appendf("Enabled=%d\n", e.m_pView->IsOpen() ? 1 : 0);
		buf->appendf("\n");
	}
}

Bool MainForm::OnMouseButton(InputDevice* pInputDevice, InputButton eButtonID, ButtonEventType eEventType, CheckedPtr<View>* ppCaptureView /*= nullptr*/)
{
	for (auto const& e : m_vViews)
	{
		if (e.m_pView.IsValid())
		{
			if (e.m_pView->OnMouseButton(
				pInputDevice,
				eButtonID,
				eEventType))
			{
				// Capture.
				if (ppCaptureView)
				{
					*ppCaptureView = e.m_pView;
				}

				// Return immediately.
				return true;
			}
		}
	}

	return false;
}

void MainForm::OnMouseMove(Int iX, Int iY, Bool bWillCapture)
{
	for (auto const& e : m_vViews)
	{
		if (e.m_pView)
		{
			// Deliver.
			e.m_pView->OnMouseMove(iX, iY, bWillCapture);
		}
	}
}

Bool MainForm::OnMouseWheel(InputDevice* pInputDevice, InputDevice::Axis* pAxis, CheckedPtr<View>* ppCaptureView /*= nullptr*/)
{
	for (auto const& e : m_vViews)
	{
		if (e.m_pView.IsValid())
		{
			if (e.m_pView->OnMouseWheel(pInputDevice, pAxis))
			{
				// Capture.
				if (ppCaptureView)
				{
					*ppCaptureView = e.m_pView;
				}

				// Return immediately.
				return true;
			}
		}
	}

	return false;
}

Bool MainForm::OnKeyPressed(InputButton eButton, UInt32 uModifiers, CheckedPtr<View>* ppCaptureView /*= nullptr*/)
{
	for (auto const& e : m_vViews)
	{
		if (e.m_pView.IsValid())
		{
			if (e.m_pView->OnKeyPressed(eButton, uModifiers))
			{
				// Capture.
				if (ppCaptureView)
				{
					*ppCaptureView = e.m_pView;
				}

				// Return immediately.
				return true;
			}
		}
	}

	return false;
}

void MainForm::PrePose(RenderPass& rPass, Bool bVisible)
{
	for (auto const& e : m_vViews)
	{
		auto pView = e.m_pView;
		pView->PrePose(GetController(), rPass, bVisible || pView->IsAlwaysOpen());
	}
}

void MainForm::PrePoseWindowsMenu()
{
	if (m_vViews.IsEmpty())
	{
		return;
	}
	
	using namespace ImGui;
	Separator();
	// TODO: Eliminate copy.
	auto vSorted = m_vViews;
	QuickSort(vSorted.Begin(), vSorted.End());

	// Process views.
	for (auto const& e : vSorted)
	{
		auto bSelected = (e.m_pView ? e.m_pView->IsOpen() : e.m_bLastOpen);
		auto const bEnabled = (e.m_pView && !e.m_pView->IsAlwaysOpen());
		if (MenuItem(e.m_sPrunedName.CStr(), nullptr, &bSelected, bEnabled))
		{
			e.m_pView->SetOpen(bSelected);
		}
	}
}

void MainForm::TickBegin(Float fDeltaTimeInSeconds)
{
	for (auto const& e : m_vViews)
	{
		auto pView = e.m_pView;
		pView->Tick(GetController(), fDeltaTimeInSeconds);
	}
}

void MainForm::TickEnd(Float fDeltaTimeInSeconds)
{
	// On mobile, make sure only one view is open.
	if (Root::Get()->IsMobile())
	{
		ViewEntry const* p = nullptr;
		auto bOpen = InternalFindBestOpened(&p);
		if (bOpen)
		{
			for (auto& e : m_vViews)
			{
				auto pView = e.m_pView;

				// Skip views that are always open.
				if (pView->IsAlwaysOpen())
				{
					continue;
				}

				// Skip the found entry.
				if (&e == p)
				{
					continue;
				}

				pView->SetOpen(false);
			}
		}
	}

	// Check for changes.
	for (auto& e : m_vViews)
	{
		if (e.m_bLastOpen != e.m_pView->IsOpen())
		{
			e.m_bLastOpen = e.m_pView->IsOpen();
			ImGui::MarkIniSettingsDirty();
		}
	}
}

Bool MainForm::InternalFindBestOpened(ViewEntry const** pp) const
{
	Bool bFound = false;
	for (auto const& e : m_vViews)
	{
		// Don't consider views that are always open.
		if (e.m_pView->IsAlwaysOpen())
		{
			continue;
		}

		// Not opened, skip.
		if (!e.m_pView->IsOpen())
		{
			continue;
		}

		// Preference the logger.
		if (!bFound
#if SEOUL_LOGGING_ENABLED
			|| ViewLog::GetStaticId() == e.m_pView->GetId()
#endif
			)
		{
			bFound = true;
			if (nullptr != pp) { *pp = &e; }
		}
	}

	return bFound;
}

HString MainForm::InternalFindFirstOpened() const
{
	for (auto const& e : m_vViews)
	{
		// Don't consider views that are always open.
		if (e.m_pView->IsAlwaysOpen())
		{
			continue;
		}

		if (e.m_pView->IsOpen())
		{
			return e.m_pView->GetId();
		}
	}

	return HString();
}

} // namespace Seoul::DevUI
