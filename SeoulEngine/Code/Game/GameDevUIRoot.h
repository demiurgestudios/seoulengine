/**
 * \file GameDevUI.h
 * \brief Specialization of DevUIRoot for in-game cheat, inspection, and profiling
 * UI. Distinct and unique from SeoulEditor.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_DEV_UI_H
#define GAME_DEV_UI_H

#include "DevUIRoot.h"
#include "UIContext.h"
#include "UIStackFilter.h"

#if SEOUL_ENABLE_DEV_UI
namespace Seoul { namespace Game { class DevUIMemoryUsageUtil; } }
namespace Seoul { namespace UI { class Manager; } }

namespace Seoul::Game
{

class DevUIRoot SEOUL_SEALED : public DevUI::Root
{
public:
	/**
	 * @return The global singleton instance. Will be nullptr if that instance has not
	 * yet been created.
	 */
	static CheckedPtr<DevUIRoot> Get()
	{
		if (DevUI::Root::Get() && DevUI::Root::Get()->GetType() == DevUI::Type::Game)
		{
			return (DevUIRoot*)DevUI::Root::Get().Get();
		}
		else
		{
			return CheckedPtr<DevUIRoot>();
		}
	}

	// Convenience method - when the Game::DevUI is active, the game UI::Manager becomes
	// a child of the Game::DevUI (game rendering is wrapped in a Game::DevUI window). This
	// requires some care during instantiation (GameDevUI overrides g_UIContext, then
	// UI::Manager is intantiated, then Game::DevUI is instantiated), so this method
	// handles the sequencing for you.
	static UI::Manager* InstantiateUIManagerInGameDevUI(FilePath guiConfigFilePath, UI::StackFilter eStackFilter);

	DevUIRoot();
	~DevUIRoot();

	void SetDemiplaneName(const String& sName)
	{
		m_sDemiplaneName = sName;
	}

	virtual void DisplayNotification(const String& sMessage) SEOUL_OVERRIDE;
	virtual void DisplayTrackedNotification(const String& sMessage, Int32& riId) SEOUL_OVERRIDE;
	virtual void KillNotification(Int32 iId) SEOUL_OVERRIDE;

private:
	struct Notification SEOUL_SEALED
	{
		Notification()
			: m_sMessage()
			, m_fDisplayTime(0.0f)
			, m_Id(-1)
		{
		}

		String m_sMessage;
		Float m_fDisplayTime;
		Atomic32Type m_Id;
	};
	typedef List<Notification, MemoryBudgets::DevUI> Notifications;
	Notifications m_lNotifications;
	Mutex m_Mutex;
	String m_sDemiplaneName;
	ScopedPtr<DevUIMemoryUsageUtil> m_pMemoryUsageUtil;
	Atomic32 m_NotificationId;
	Float m_fCookingMessageDisplayTime;
#if SEOUL_ENABLE_CHEATS
	Bool m_bCanTouchToggle;
#endif // /#if SEOUL_ENABLE_CHEATS
	Bool m_bMiniFPS;
	Bool m_bRuntimeStats;

	virtual void InternalDoSkipPose(Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;
	virtual void InternalDoTickBegin(RenderPass& rPass, Float fDeltaTimeInSeconds, IPoseable* pParent) SEOUL_OVERRIDE;
	virtual void InternalDrawMenuBar(Bool bRootMainMenu) SEOUL_OVERRIDE
	{
		if (InternalBeginMainMenuPrePose(bRootMainMenu))
		{
			InternalEndMainMenuPrePose(bRootMainMenu);
		}
	}
	virtual void InternalOnSwitchToVirtualizedDesktop() SEOUL_OVERRIDE;
	virtual void InternalPrePoseImGuiFrameBegin(RenderPass& rPass, Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;
	virtual void InternalPrePoseImGuiFrameEnd(RenderPass& rPass, Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;

	Bool InternalBeginMainMenuPrePose(Bool bRootMainMenu);
	void InternalEndMainMenuPrePose(Bool bRootMainMenu);
	void InternalPrePoseFileMenu();
	void InternalPrePoseHelpMenu();
	void InternalPrePoseShowMenu();
	void InternalPrePoseViewsMenu();

	void InternalPrePoseDemiplaneState();
	// TODO: Break this out into a refactored DevUIView.
	void InternalPrePoseHotLoadingState();
	void InternalPrePoseMiniFPS(Float fDeltaTimeInSeconds);
	void InternalPrePoseNotifications(Float fDeltaTimeInSeconds);
	void InternalPrePoseRuntimeStats(Float fDeltaTimeInSeconds);
	// /TODO:

	SEOUL_DISABLE_COPY(DevUIRoot);
}; // class Game::DevUIRoot

} // namespace Seoul::Game

#endif // /SEOUL_ENABLE_DEV_UI

#endif // include guard
