/**
 * \file DevUIView.cpp
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

#include "DevUIImGui.h"
#include "DevUIRoot.h"
#include "DevUIView.h"
#include "InputKeys.h"
#include "ReflectionDefine.h"

#if SEOUL_ENABLE_DEV_UI

namespace Seoul
{

SEOUL_TYPE(DevUI::View);

namespace DevUI
{

View::View()
	: m_bDesiredOpen(false)
{
}

View::~View()
{
}

void View::PrePose(Controller& rController, RenderPass& rPass, Bool bVisible)
{
	using namespace ImGui;

	DoPrePoseAlways(rController, rPass, IsOpen() && bVisible);

	if (!IsOpen() || !bVisible)
	{
		DoSkipPose(rController, rPass);
		return;
	}

	// Initial - may be addendums from various configs.
	auto uFlags = GetFlags();
	if (IsAlwaysOpen() || !IsCloseable())
	{
		uFlags |= ImGuiWindowFlags_NoCloseButton;
	}

	// On mobile, views always fill the entire screen and are not
	// movable.
	if (Root::Get()->IsMobile())
	{
		auto const vDisplay = ImGui::GetIO().DisplaySize;
		auto const fBarHeight = GetMainMenuBarHeight();
		SetNextWindowPos(ImVec2(0, fBarHeight));
		SetNextWindowSize(ImVec2(vDisplay.x + 1, (vDisplay.y + 1 - fBarHeight)));

		// Mobile windows cannot be resized, moved, closed, or collapsed.
		uFlags |= ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoSavedSettings;
	}
	else
	{
		Vector2D vPos;
		if (GetInitialPosition(vPos))
		{
			SetNextWindowPos(ImVec2(vPos.X, vPos.Y), ImGuiCond_FirstUseEver);
		}

		auto const v(GetInitialSize());
		SetNextWindowSize(ImVec2(v.X, v.Y), ImGuiCond_FirstUseEver);
	}

	// Whether we have a close button or not.
	auto pOpen = &m_bDesiredOpen;
	if (IsAlwaysOpen() || !IsCloseable())
	{
		pOpen = nullptr;
	}

	PreBegin();
	if (!Begin(GetId().CStr(), pOpen, uFlags))
	{
		DoSkipPose(rController, rPass);
		End();
		PostEnd();
		return;
	}

	// Handle CTRL+F4 window closing.
	if (!Root::Get()->IsMobile())
	{
		if (IsWindowFocused() &&
			GetIO().KeyCtrl &&
			IsKeyReleased(ImGuiKey_F4))
		{
			m_bDesiredOpen = false;
		}
	}

	// Last check - unclear, but various situations with docking
	// can result in this as false even though Begin() succeeded.
	if (!ImGui::IsWindowActiveAndVisible())
	{
		DoSkipPose(rController, rPass);
		End();
		PostEnd();
		return;
	}

	DoPrePose(rController, rPass);

	End();
	PostEnd();
}

/**
 * Entry point for updating the view. Called every frame,
 * even if the view is not open.
 */
void View::Tick(Controller& rController, Float fDeltaTimeInSeconds)
{
	DoTick(rController, fDeltaTimeInSeconds);
}

} // namespace DevUI

} // namespace Seoul

#endif // /SEOUL_ENABLE_DEV_UI
