/**
 * \file GameDevUIViewGameUI.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_DEV_UI_VIEW_GAME_UI_H
#define GAME_DEV_UI_VIEW_GAME_UI_H

#include "DevUIConfig.h"
#include "DevUIView.h"
#include "FalconInstance.h"
#include "RenderDevice.h"
#include "Singleton.h"
#include "UIMovieHandle.h"
#include "Viewport.h"
struct ImDrawCmd;
struct ImDrawList;

#if SEOUL_ENABLE_DEV_UI

namespace Seoul::Game
{

/**
 * When the DevUI is active, the core game UI (drawn by UI::Manager)
 * is restricted to a window of the DevUI, so it can be positioned
 * and sorted in consideration of other developer views.
 */
class DevUIViewGameUI SEOUL_SEALED : public DevUI::View, public Singleton<DevUIViewGameUI>
{
public:
	DevUIViewGameUI();
	~DevUIViewGameUI();

	static HString StaticGetId()
	{
		static const HString kId("Game UI");
		return kId;
	}

	// DevUIView overrides
	virtual Bool IsAlwaysOpen() const SEOUL_OVERRIDE { return true; }

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticGetId();
	}

	virtual Bool OnMouseButton(InputDevice* pInputDevice, InputButton eButtonID, ButtonEventType eEventType) SEOUL_OVERRIDE;
	virtual void OnMouseMove(Int iX, Int iY, Bool bWillCapture) SEOUL_OVERRIDE;
	virtual Bool OnMouseWheel(InputDevice* pInputDevice, InputDevice::Axis* pAxis) SEOUL_OVERRIDE;
	// /DevUIView overrides

	/** Rendering of a selection indicator, used by DevUIViewUIExplorer. */
	void HighlightSelect(
		const UI::MovieHandle& h = UI::MovieHandle(),
		const SharedPtr<Falcon::Instance>& p = SharedPtr<Falcon::Instance>())
	{
		m_hSelected = h;
		m_pSelected = p;
	}

	/** Getters for current selection, if any. */
	const UI::MovieHandle& GetSelectedMovie() const { return m_hSelected; }
	const SharedPtr<Falcon::Instance>& GetSelectedInstance() const { return m_pSelected; }

	/** True if the viewport area of the game is visible, false otherwise. */
	Bool HoverGameView() const;

	void TakeScreenshot(const DevUI::Config::ScreenshotConfig& config)
	{
		m_vPendingScreenshots.PushBack(config);
	}

private:
	// DevUIView overrides
	virtual void PreBegin() SEOUL_OVERRIDE;
	virtual void PostEnd() SEOUL_OVERRIDE;
	virtual void DoPrePose(DevUI::Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	virtual void DoSkipPose(DevUI::Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	virtual UInt32 GetFlags() const SEOUL_OVERRIDE;
	virtual Bool GetInitialPosition(Vector2D& rv) const SEOUL_OVERRIDE;
	virtual Vector2D GetInitialSize() const SEOUL_OVERRIDE;
	// /DevUIView overrides

	static void RenderUIManager(const ImDrawList* pParentList, const ImDrawCmd* pCommand, void* /*pUnusedContext*/);
	void DrawSelection();

	Vector2D m_vImGuiInnerRectMin;
	Vector2D m_vImGuiInnerRectMax;
	UI::MovieHandle m_hSelected;
	SharedPtr<Falcon::Instance> m_pSelected;

	void InternalPrePoseFxState();
	Vector3D m_vStartFxOffset{};

	void InternalTakeScreenshot(RenderCommandStreamBuilder& rBuilder, const DevUI::Config::ScreenshotConfig& config);

	typedef Vector<DevUI::Config::ScreenshotConfig, MemoryBudgets::DevUI> PendingScreenshots;
	PendingScreenshots m_vPendingScreenshots;

	SEOUL_DISABLE_COPY(DevUIViewGameUI);
}; // class Game::DevUIViewGameUI

} // namespace Seoul::Game

#endif // /SEOUL_ENABLE_DEV_UI

#endif // include guard
