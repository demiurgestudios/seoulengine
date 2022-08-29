/**
 * \file GameDevUIViewUIExplorer.h
 * \brief A developer UI view that displays current UI
 * graph state.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_DEV_UI_VIEW_UI_EXPLORER_H
#define GAME_DEV_UI_VIEW_UI_EXPLORER_H

#include "DevUIView.h"
#include "HashTable.h"
#include "SeoulHString.h"
#include "UIManager.h"
#include "UIMovieHandle.h"
#include "Vector.h"

#if (SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)

namespace Seoul::Game
{

class DevUIViewUIExplorer SEOUL_SEALED : public DevUI::View
{
public:
	DevUIViewUIExplorer();
	~DevUIViewUIExplorer();

	// DevUIView overrides
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		static const HString kId("UI Explorer");
		return kId;
	}

protected:
	virtual void DoPrePose(DevUI::Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	virtual void DoPrePoseAlways(DevUI::Controller& rController, RenderPass& rPass, Bool bVisible) SEOUL_OVERRIDE;
	virtual UInt32 GetFlags() const SEOUL_OVERRIDE;
	virtual Vector2D GetInitialSize() const SEOUL_OVERRIDE
	{
		return Vector2D(400, 600);
	}
	// /DevUIView overrides

private:
	Bool CheckExpansion(void* p);
	Bool Pick();
	void PoseExplorer();
	void PoseInstance(
		UI::Movie const* pMovie,
		const SharedPtr<Falcon::Instance>& pInstance);
	void PoseMovieClip(
		UI::Movie const* pMovie,
		const SharedPtr<Falcon::MovieClipInstance>& pMovieClip);
	void PoseState();
	void PoseStage3DSettings();
	void PoseTextEffectSettings();
	void UpdateSelected(
		UI::Movie const* pMovie,
		const SharedPtr<Falcon::Instance>& pInstance);

	Bool m_bNeedScroll;
	typedef Vector<void*, MemoryBudgets::DevUI> Expansion;
	Expansion m_vExpansion;

	typedef HashTable<HString, Bool, MemoryBudgets::StateMachine> Conditions;
	Conditions m_tLastConditions;

	struct ConditionEntry SEOUL_SEALED
	{
		ConditionEntry()
			: m_Name()
			, m_bValue(false)
		{
		}

		Bool operator<(const ConditionEntry& b) const
		{
			return (strcmp(m_Name.CStr(), b.m_Name.CStr()) < 0);
		}

		HString m_Name;
		Bool m_bValue;
	}; // struct ConditionEntry
	typedef Vector<ConditionEntry, MemoryBudgets::DevUI> SortedConditions;
	SortedConditions m_vSortedConditions;
	UI::Manager::TriggerHistory m_vTriggerHistory;
	Bool m_bStage3DSettingsDirty;
	Bool m_bTextEffectSettingsDirty;

	SEOUL_DISABLE_COPY(DevUIViewUIExplorer);
}; // class Game::DevUIViewUIExplorer

} // namespace Seoul::Game

#endif // /(SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)

#endif // include guard
