/**
 * \file UIStack.h
 * \brief The overall stack of UI::States that fully define the current
 * state of UI inside UIManager. This class is an internal
 * class to UI::Manager and is not likely useful outside UIManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_STACK_H
#define UI_STACK_H

#include "Delegate.h"
#include "HashSet.h"
#include "Mutex.h"
#include "Settings.h"
#include "StateMachine.h"
#include "UIData.h"
#include "UIMovieContent.h"
#include "UIStackFilter.h"
#include "UIState.h"
#include "Vector.h"
namespace Seoul { namespace Falcon { class Stage3DSettings; } }
namespace Seoul { namespace Falcon { struct TextEffectSettings; } }

namespace Seoul::UI
{

/**
 * UI::Stack encapsulates the stack of state machines that defines the UI system - it is
 * used internally by UI::Manager and is likely not useful outside that context.
 */
class Stack SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(Stack);

	typedef HashTable<HString, CheckedPtr<Falcon::Stage3DSettings>, MemoryBudgets::UIData> Stage3DSettingsTable;
	typedef HashTable<HString, CheckedPtr<Falcon::TextEffectSettings>, MemoryBudgets::UIData> TextEffectSettingsTable;
	typedef StateMachine<State> StateMachine;

	struct StackEntry SEOUL_SEALED
	{
		StackEntry()
			: m_pMachine()
			, m_FilePath()
			, m_hSettings()
			, m_pSettings()
			, m_ActiveStateId()
			, m_eFilter(StackFilter::kAlways)
		{
		}

		Bool operator==(FilePath filePath) const
		{
			return (m_FilePath == filePath);
		}
		Bool operator!=(FilePath filePath) const
		{
			return !(*this == filePath);
		}

		CheckedPtr<StateMachine> m_pMachine;
		FilePath m_FilePath;
		SettingsContentHandle m_hSettings;
		SharedPtr<DataStore> m_pSettings;
		HString m_ActiveStateId;
		StackFilter m_eFilter;
	}; // struct StackEntry
	typedef Vector<StackEntry, MemoryBudgets::UIRuntime> StackVector;

	Stack(FilePath settingsFilePath, StackFilter eStackFilter);
	~Stack();

	// Per-frame updates.
	void Advance(Float fDeltaTime);

	// Equivalent to ~UIStack(). Used to step destroy on shutdown,
	// so that UI::Stack still exists but is empty during the
	// destruction process.
	void Destroy();

	// Used for runtime updating. This is *not* the code path for hot loading -
	// that is handled automatically by OnFileLoadComplete(). Instead,
	// this is used for controlled updating (e.g. by Game::Patcher) of
	// a shipped product.
	void ApplyFileChange(FilePath filePath);

	/**
	 * @return The DataStore that contains the current configuration
	 * settings of this UIStack.
	 */
	SharedPtr<DataStore> GetSettings() const
	{
		return m_pSettings;
	}

	/**
	 * @return The FilePath associated with the global settings
	 * structure.
	 */
	FilePath GetSettingsFilePath() const
	{
		return m_FilePath;
	}

	/**
	 * @return Get the current UI stack.
	 */
	const StackVector& GetStack() const
	{
		return m_vStack;
	}

	// Called by UI::Manager when at least one state transition is activated
	// Lets UI::Stack update its extended set of content that will be needed
	// by the UI system.
	void OnStateTransitionActivated();

	// Return the path to the globally configured set of shadow
	// settings.
	FilePath GetStage3DSettingsFilePath() const;

	/** @return A read-only reference to the global table of shadow settings. */
	const Stage3DSettingsTable& GetStage3DSettingsTable() const
	{
		return m_tStage3DSettings;
	}

	/** Binding for Falcon, exposes global Falcon planar shadow settings. */
	CheckedPtr<Falcon::Stage3DSettings> GetStage3DSettings(HString id) const
	{
		CheckedPtr<Falcon::Stage3DSettings> pReturn;
		(void)m_tStage3DSettings.GetValue(id, pReturn);
		return pReturn;
	}

	// Return the path to the globally configured set of text effect
	// settings.
	FilePath GetTextEffectSettingsFilePath() const;

	/** @return A read-only reference to the global table of text effect settings. */
	const TextEffectSettingsTable& GetTextEffectSettingsTable() const
	{
		return m_tTextEffectSettings;
	}

	/**
	 * Binding for Falcon, exposes global text effect settings that can be referenced by
	 * individual text chunks for advanced rendering settings.
	 */
	CheckedPtr<Falcon::TextEffectSettings> GetTextEffectSettings(HString id) const
	{
		CheckedPtr<Falcon::TextEffectSettings> pReturn;
		(void)m_tTextEffectSettings.GetValue(id, pReturn);
		return pReturn;
	}

	// Process deferred files changes from calls to ApplyFileChange.
	void ProcessDeferredChanges();

private:
	FilePath const m_FilePath;
	SharedPtr<DataStore> const m_pSettings;
	typedef HashSet<FilePath, MemoryBudgets::UIRuntime> PendingChanges;
	PendingChanges m_PendingChanges;
	SettingsContentHandle m_hSettings;
	SettingsContentHandle m_hStage3DSettings;
	SettingsContentHandle m_hTextEffectSettings;
	StackVector m_vStack;
	Stage3DSettingsTable m_tStage3DSettings;
	TextEffectSettingsTable m_tTextEffectSettings;
	StackFilter const m_eStackFilter;

	Bool ApplyImmediateFileChange(FilePath filePath);
	void CreateStack();
	void LoadStage3DSettings();
	void LoadTextEffectSettings();
	void ProcessDeferredFileChange(FilePath filePath);

#if SEOUL_HOT_LOADING
	void OnFileLoadComplete(FilePath filePath);
#endif // /#if SEOUL_HOT_LOADING

	SEOUL_DISABLE_COPY(Stack);
}; // class UI::Stack

} // namespace Seoul::UI

#endif // include guard
