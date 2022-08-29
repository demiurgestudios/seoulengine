/**
 * \file GameAutomation.h
 * \brief Global Singletons that owns a Lua script VM and functionality
 * for automation and testing of a game application. Developer
 * functionality only.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_AUTOMATION_H
#define GAME_AUTOMATION_H

#include "Delegate.h"
#include "FilePath.h"
#include "FixedArray.h"
#include "Once.h"
#include "SeoulTime.h"
#include "SharedPtr.h"
#include "Singleton.h"
#include "Vector.h"
namespace Seoul { namespace Game { class AutomationScriptObject; } }
namespace Seoul { namespace Game { class AutomationVmCreateJob; } }
namespace Seoul { namespace Game { struct PatcherDisplayStats; } }
namespace Seoul { namespace Script { class Vm; } }
namespace Seoul { namespace Reflection { class Any; } }
namespace Seoul { namespace UI { struct HitPoint; } }

namespace Seoul::Game
{

/** Utility structure, describes global settings to configure GameAutomation. */
struct AutomationSettings SEOUL_SEALED
{
	AutomationSettings()
		: m_sMainScriptFileName()
		, m_bAutomatedTesting(false)
	{
	}

	/** Root script file to run to populate the automation script VM. Relative to the Scripts folder. */
	String m_sMainScriptFileName;

	/**
	 * If automation is enabled for testing, configures various systems
	 * for this purposes. In particular:
	 * - effectively disables the framerate cap (sets to 1000 FPS).
	 * - fixes the Engine per-frame tick (always reports 1.0 / 60.0 seconds).
	 */
	Bool m_bAutomatedTesting;
}; // struct Game::AutomationSettings

class Automation SEOUL_SEALED : public Singleton<Automation>
{
public:
	SEOUL_DELEGATE_TARGET(Automation);

	Automation(const AutomationSettings& settings);
	~Automation();

	// Used in some cases to add additional warnings,
	// for report at the end of an automation run.
	Atomic32Type GetAdditionalWarningCount() const
	{
		return m_AdditionalWarningCount;
	}

	void IncrementAdditionalWarningCount()
	{
		++m_AdditionalWarningCount;
	}

	// Call during game shutdown, after clearing the UI
	// but before destroying the game scripting environment.
	Bool PreShutdown();

	// Call prior to running any other Tick() functions for the frame.
	// If this method returns false, it is equivalent to a return value
	// of false from Game::Main and indicates that the app should exit.
	Bool PreTick();

	// Call after running any other Tick() functions for the frame.
	// If this method returns false, it is equivalent to a return value
	// of false from Game::Main and indicates that the app should exit.
	Bool PostTick();

	// Can be called by the app environment to set various global
	// state that can then be queried by the automation script
	// environment.
	Bool SetGlobalState(HString key, const Reflection::Any& anyValue);

	// Return settings used to configure automation.
	const AutomationSettings& GetSettings() const { return m_Settings; }

	// Accumulate a server/ntp delta time sample.
	static void AccumulateServerTimeDeltaInMilliseconds(Atomic32Type milliseconds);

	// Debugging feature, enable periodic server time checks
	// using an NTP client.
	static void EnableServerTimeChecking(Bool bEnable);

	// True/false if server time checking is enabled.
	static Bool IsServerTimeCheckingEnabled() { return !s_LastServerTimeCheckUptime.IsZero(); }

	// Enable or disable performance testing.
	Bool GetEnablePerfTesting() const { return m_bEnablePerfTesting; }
	void SetEnablePerfTesting(Bool bEnablePerfTesting)
	{
		m_bEnablePerfTesting = bEnablePerfTesting;
	}

	// Hook for reporting patcher times.
	void OnPatcherClose(
		Float32 fPatcherDisplayTimeInSeconds,
		const PatcherDisplayStats& stats);

private:
	Atomic32 m_AdditionalWarningCount;
	SharedPtr<Script::Vm> m_pVm;
	AutomationSettings const m_Settings;
	Bool m_bEnablePerfTesting;
	Bool m_bIsEnabled;
	static TimeInterval s_LastServerTimeCheckUptime;
	static Atomic32 s_MaxServerTimeDeltaInMilliseconds;
	UInt64 m_uLongFrames;
	UInt64 m_uTotalFrames;
	TimeInterval const m_FirstHeartbeatUptime;
	TimeInterval m_LastHeartbeatUptime;

	// Storage for script binder.
	friend class AutomationScriptObject;
	typedef Vector<UI::HitPoint, MemoryBudgets::UIRuntime> HitPoints;
	HitPoints m_vHitPoints;
	// /Storage for script binder.

	void OnUIStateChange(HString stateMachineId, HString previousStateId, HString nextStateId);
	void InternalApplyAutomatedTestingMode();
	void InternalApplyPerformanceTesting();
	void InternalLoadVm();
	void InternalRunClChecks();
	void InternalRunSaveLoadManagerChecks();
	Once m_ClCheckOnce;
	Bool m_bExpectServerTime = false;

	SEOUL_DISABLE_COPY(Automation);
}; // class Game::Automation

} // namespace Seoul::Game

#endif // include guard
