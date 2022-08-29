/**
 * \file UIManager.h
 * \brief Global singleton, manages game UI.
 *
 * UI::Manager owns the stack of UI::StateMachines that fully defined
 * the data driven layers of UI state and behavior. UI::Manager also
 * performs input management for the UI system.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "AtomicRingBuffer.h"
#include "CommandStream.h"
#include "ContentStore.h"
#include "Delegate.h"
#include "FalconEditTextLink.h"
#include "FalconFont.h"
#include "FalconGlobalConfig.h"
#include "FilePath.h"
#include "HashSet.h"
#include "HashTable.h"
#include "IPoseable.h"
#include "ITextEditable.h"
#include "List.h"
#include "Logger.h"
#include "Singleton.h"
#include "UIFixedAspectRatio.h"
#include "UIStack.h"
#include "UIStackFilter.h"
namespace Seoul { namespace Falcon { class EditTextInstance; } }
namespace Seoul { namespace Falcon { class FCNFile; } }
namespace Seoul { namespace Falcon { struct FontOverrides; } }
namespace Seoul { namespace Falcon { class MovieClipInstance; } }
namespace Seoul { namespace Falcon { class Stage3DSettings; } }
namespace Seoul { namespace Falcon { struct TextEffectSettings; } }

namespace Seoul::UI
{

/** Events::Manager event fired on any UI::Manager state changes. */
static const HString StateChangeEventId("UIStateChangeEvent");

/**
 * The Events::Manager id of an event to register
 * to receive an event whenever a triggered is fired
 * (via TriggerTransition) that fails to activate any
 * state transitions.
 */
static const HString TriggerFailedToFireTransitionEventId("TriggerFailedToFireTransitionEventId");

static const HString HotReloadBeginEventId("UIHotReloadBeginEvent");
static const HString HotReloadEndEventId("UIHotReloadEndEvent");

// Forward declarations
class AdvanceInterfaceDeferredDispatch;
class Renderer;

/** SeoulEngine wrapper around Falcon FCN file data. */
class FCNFileData SEOUL_SEALED
{
public:
	FCNFileData(const SharedPtr<Falcon::FCNFile>& pFCNFile, const FilePath& filePath);
	~FCNFileData();

	// Populate arguments with an exact copy of associated data.
	void CloneTo(
		SharedPtr<Falcon::MovieClipInstance>& rpRootInstance,
		ScopedPtr<AdvanceInterfaceDeferredDispatch>& rpAdvanceInterface) const;

	/** @return The Falcon::FCNFile data wrapped. */
	const SharedPtr<Falcon::FCNFile>& GetFCNFile() const
	{
		return m_pFCNFile;
	}

private:
	SEOUL_REFERENCE_COUNTED(FCNFileData);

	SharedPtr<Falcon::FCNFile> m_pFCNFile;
	SharedPtr<Falcon::MovieClipInstance> m_pTemplateRootInstance;
	ScopedPtr<AdvanceInterfaceDeferredDispatch> m_pTemplateAdvanceInterface;
	FilePath m_filePath;

	SEOUL_DISABLE_COPY(FCNFileData);
}; // class FCNFileData

class Manager SEOUL_SEALED : public Singleton<Manager>, public IPoseable, public ITextEditable
{
public:
	SEOUL_DELEGATE_TARGET(Manager);

	typedef HashTable<HString, CheckedPtr<Falcon::Stage3DSettings>, MemoryBudgets::UIData> Stage3DSettingsTable;
	typedef HashTable<HString, CheckedPtr<Falcon::TextEffectSettings>, MemoryBudgets::UIData> TextEffectSettingsTable;

	Manager(
		FilePath guiConfigFilePath,
		StackFilter eStackFilter);
	~Manager();

	/**
	 * @return Returns the FilePath currently being used to configure the UI system.
	 */
	FilePath GetGuiConfigFilePath() const
	{
		return m_GuiConfigFilePath;
	}

	// Used for runtime updating. This is *not* the code path for hot loading -
	// that is handled automatically by OnFileLoadComplete(). Instead,
	// this is used for controlled updating (e.g. by Game::Patcher) of
	// a shipped product.
	void ApplyFileChange(FilePath filePath);

	// Clear any suspended movies. Destroys the movies and discards their data.
	void ClearSuspended();

	// First step of UI::Manager shutdown - places the
	// stack in the default (no initialize) state but
	// does not clear structures.
	void ShutdownPrep();

	// Second step of UI::Manager shutdown - call after disabling
	// network file IO/waiting for content loads (if applicable).
	void ShutdownComplete();

#if SEOUL_HOT_LOADING
	// Similar to Clear(), except attempts to restore the current state machine state after the clear.
	void HotReload();

	void ShelveDataForHotLoad(const String& sId, const DataStore& ds);
	SharedPtr<DataStore> UnshelveDataFromHotLoad(const String& sId) const;
#endif // /#if SEOUL_HOT_LOADING

	// Trigger a normal state transition. Should be a valid transition out of the
	// current state.
	void TriggerTransition(HString triggerName);

	// Force an unevaluated state transition to the target state for the target state machine.
	// NOTE: stateMachineName corresponds to the base name of the file that was used
	// to configure the state machine without the extension - for example, config://UI/MyStateMachine.json would
	// have the name MyStateMachine
	void GotoState(HString stateMachineName, HString stateName);

	// Get/set UI conditions. GetCondition() will immediately reflect the
	// value set via SetCondition(), but condition changes will not be applied
	// to state machine transition lock until the next evaluation window
	// (once per frame update).
	typedef HashTable<HString, Bool, MemoryBudgets::StateMachine> Conditions;
	void GetConditions(Conditions& rt) const;
	Bool GetCondition(HString conditionName) const;
	void SetCondition(HString conditionName, Bool bValue);

	/** Retrieve the current trigger history. Only available in builds with logging enabled. */
	struct TriggerHistoryEntry SEOUL_SEALED
	{
		TriggerHistoryEntry(
			HString triggerName = HString(),
			HString stateMachine = HString(),
			HString fromState = HString(),
			HString toState = HString())
			: m_TriggerName(triggerName)
			, m_StateMachine(stateMachine)
			, m_FromState(fromState)
			, m_ToState(toState)
		{
		}

		HString m_TriggerName;
		HString m_StateMachine;
		HString m_FromState;
		HString m_ToState;
	}; // struct TriggerHistoryEntry
	typedef Vector<TriggerHistoryEntry, MemoryBudgets::UIDebug> TriggerHistory;
	void GetTriggerHistory(TriggerHistory& rv) const
	{
#if SEOUL_LOGGING_ENABLED
		rv.Clear();
		rv.Reserve(m_vTriggerHistory.GetSize());
		for (UInt32 i = m_uTriggerHistoryHead; i < m_vTriggerHistory.GetSize(); ++i)
		{
			// An empty trigger means we've hit the end of the circular buffer.
			auto const& entry = m_vTriggerHistory[i];
			if (entry.m_TriggerName.IsEmpty())
			{
				break;
			}

			rv.PushBack(entry);
		}
		for (UInt32 i = 0u; i < m_uTriggerHistoryHead; ++i)
		{
			rv.PushBack(m_vTriggerHistory[i]);
		}
#else
		rv.Clear();
#endif // /#if SEOUL_LOGGING_ENABLED
	}

	// Broadcasts an event to all active movies in all state machines
	// (if sTarget is empty) or to a specific movie (if sTarget is
	// not empty and is set to a movie type name).
	//
	// Return true if the event was received, false otherwise.
	Bool BroadcastEventTo(
		HString sTarget,
		HString sEvent)
	{
		Reflection::MethodArguments aEmptyArguments;
		return BroadcastEventTo(sTarget, sEvent, aEmptyArguments, 0);
	}

	// Broadcasts an event to all active movies in all state machines
	// (if sTarget is empty) or to a specific movie (if sTarget is
	// not empty and is set to a movie type name).
	//
	// If bPersistent is true and the delivery fails, it will
	// be queued for delivery. Delivery will be attempted
	// repeatedly until it succeeds.
	//
	// Return true if the event was received, false otherwise.
	Bool BroadcastEventTo(
		HString sTarget,
		HString sEvent,
		const Reflection::MethodArguments& aArguments,
		Int nArgumentCount,
		Bool bPersistent = false);

	// Broadcasts an event with 1 argument to all active movies in all state machines
	// (if sTarget is empty) or to a specific movie (if sTarget is
	// not empty and is set to a movie type name).
	//
	// Return true if the event was received, false otherwise.
	template <typename A1>
	Bool BroadcastEventTo(
		HString sTarget,
		HString sEvent,
		const A1& a1)
	{
		Reflection::MethodArguments aArguments;
		aArguments[0] = a1;
		return BroadcastEventTo(sTarget, sEvent, aArguments, 1);
	}

	// Broadcasts an event with 2 arguments to all active movies in all state machines
	// (if sTarget is empty) or to a specific movie (if sTarget is
	// not empty and is set to a movie type name).
	//
	// Return true if the event was received, false otherwise.
	template <typename A1, typename A2>
	Bool BroadcastEventTo(
		HString sTarget,
		HString sEvent,
		const A1& a1,
		const A2& a2)
	{
		Reflection::MethodArguments aArguments;
		aArguments[0] = a1;
		aArguments[1] = a2;
		return BroadcastEventTo(sTarget, sEvent, aArguments, 2);
	}

	// Broadcasts an event with 3 arguments to all active movies in all state machines
	// (if sTarget is empty) or to a specific movie (if sTarget is
	// not empty and is set to a movie type name).
	template <typename A1, typename A2, typename A3>
	Bool BroadcastEventTo(
		HString sTarget,
		HString sEvent,
		const A1& a1,
		const A2& a2,
		const A3& a3)
	{
		Reflection::MethodArguments aArguments;
		aArguments[0] = a1;
		aArguments[1] = a2;
		aArguments[2] = a3;
		return BroadcastEventTo(sTarget, sEvent, aArguments, 3);
	}

	// Broadcasts an event with 4 arguments to all active movies in all state machines
	// (if sTarget is empty) or to a specific movie (if sTarget is
	// not empty and is set to a movie type name).
	template <typename A1, typename A2, typename A3, typename A4>
	Bool BroadcastEventTo(
		HString sTarget,
		HString sEvent,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4)
	{
		Reflection::MethodArguments aArguments;
		aArguments[0] = a1;
		aArguments[1] = a2;
		aArguments[2] = a3;
		aArguments[3] = a4;
		return BroadcastEventTo(sTarget, sEvent, aArguments, 4);
	}

	// Broadcasts an event with 5 arguments to all active movies in all state machines
	// (if sTarget is empty) or to a specific movie (if sTarget is
	// not empty and is set to a movie type name).
	template <typename A1, typename A2, typename A3, typename A4, typename A5>
	Bool BroadcastEventTo(
		HString sTarget,
		HString sEvent,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5)
	{
		Reflection::MethodArguments aArguments;
		aArguments[0] = a1;
		aArguments[1] = a2;
		aArguments[2] = a3;
		aArguments[3] = a4;
		aArguments[4] = a5;
		return BroadcastEventTo(sTarget, sEvent, aArguments, 5);
	}

	// Broadcasts an event with 6 arguments to all active movies in all state machines
	// (if sTarget is empty) or to a specific movie (if sTarget is
	// not empty and is set to a movie type name).
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
	Bool BroadcastEventTo(
		HString sTarget,
		HString sEvent,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6)
	{
		Reflection::MethodArguments aArguments;
		aArguments[0] = a1;
		aArguments[1] = a2;
		aArguments[2] = a3;
		aArguments[3] = a4;
		aArguments[4] = a5;
		aArguments[5] = a6;
		return BroadcastEventTo(sTarget, sEvent, aArguments, 6);
	}

	// Broadcasts an event with 7 arguments to all active movies in all state machines
	// (if sTarget is empty) or to a specific movie (if sTarget is
	// not empty and is set to a movie type name).
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
	Bool BroadcastEventTo(
		HString sTarget,
		HString sEvent,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7)
	{
		Reflection::MethodArguments aArguments;
		aArguments[0] = a1;
		aArguments[1] = a2;
		aArguments[2] = a3;
		aArguments[3] = a4;
		aArguments[4] = a5;
		aArguments[5] = a6;
		aArguments[6] = a7;
		return BroadcastEventTo(sTarget, sEvent, aArguments, 7);
	}

	// Broadcasts an event with 8 arguments to all active movies in all state machines
	// (if sTarget is empty) or to a specific movie (if sTarget is
	// not empty and is set to a movie type name).
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
	Bool BroadcastEventTo(
		HString sTarget,
		HString sEvent,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8)
	{
		Reflection::MethodArguments aArguments;
		aArguments[0] = a1;
		aArguments[1] = a2;
		aArguments[2] = a3;
		aArguments[3] = a4;
		aArguments[4] = a5;
		aArguments[5] = a6;
		aArguments[6] = a7;
		aArguments[7] = a8;
		return BroadcastEventTo(sTarget, sEvent, aArguments, 8);
	}

	// Broadcasts an event to all active movies in all state machines
	Bool BroadcastEvent(
		HString sEvent)
	{
		return BroadcastEventTo(HString(), sEvent);
	}

	// Broadcasts an event to all active movies in all state machines
	Bool BroadcastEvent(
		HString sEvent,
		const Reflection::MethodArguments& aArguments,
		Int nArgumentCount,
		Bool bPersistent = false)
	{
		return BroadcastEventTo(HString(), sEvent, aArguments, nArgumentCount, bPersistent);
	}

	// Broadcasts an event with 1 argument to all active movies in all state machines
	template <typename A1>
	Bool BroadcastEvent(
		HString sEvent,
		const A1& a1)
	{
		return BroadcastEventTo(HString(), sEvent, a1);
	}

	// Broadcasts an event with 2 arguments to all active movies in all state machines
	template <typename A1, typename A2>
	Bool BroadcastEvent(
		HString sEvent,
		const A1& a1,
		const A2& a2)
	{
		return BroadcastEventTo(HString(), sEvent, a1, a2);
	}

	// Broadcasts an event with 3 arguments to all active movies in all state machines
	template <typename A1, typename A2, typename A3>
	Bool BroadcastEvent(
		HString sEvent,
		const A1& a1,
		const A2& a2,
		const A3& a3)
	{
		return BroadcastEventTo(HString(), sEvent, a1, a2, a3);
	}

	// Broadcasts an event with 4 arguments to all active movies in all state machines
	template <typename A1, typename A2, typename A3, typename A4>
	Bool BroadcastEvent(
		HString sEvent,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4)
	{
		return BroadcastEventTo(HString(), sEvent, a1, a2, a3, a4);
	}

	// Broadcasts an event with 5 arguments to all active movies in all state machines
	template <typename A1, typename A2, typename A3, typename A4, typename A5>
	Bool BroadcastEvent(
		HString sEvent,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5)
	{
		return BroadcastEventTo(HString(), sEvent, a1, a2, a3, a4, a5);
	}

	// Broadcasts an event with 6 arguments to all active movies in all state machines
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
	Bool BroadcastEvent(
		HString sEvent,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6)
	{
		return BroadcastEventTo(HString(), sEvent, a1, a2, a3, a4, a5, a6);
	}

	// Broadcasts an event with 7 arguments to all active movies in all state machines
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6,  typename A7>
	Bool BroadcastEvent(
		HString sEvent,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7)
	{
		return BroadcastEventTo(HString(), sEvent, a1, a2, a3, a4, a5, a6, a7);
	}

	// Broadcasts an event with 8 arguments to all active movies in all state machines
	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6,  typename A7,  typename A8>
	Bool BroadcastEvent(
		HString sEvent,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8)
	{
		return BroadcastEventTo(HString(), sEvent, a1, a2, a3, a4, a5, a6, a7, a8);
	}

	// DEPRECATED: Please migrate to usage of the script SetTexture and AddChildBitmap
	// APIs.
	void UpdateTextureReplacement(
		FilePathRelativeFilename symbolName,
		FilePath filePath);

	// DEPRECATED: Please migrate to usage of the script SetTexture and AddChildBitmap
	// APIs.
	void UpdateTextureReplacement(
		HString symbolName,
		FilePath filePath)
	{
		// TODO: Silly, but UpdateTextureReplacement() is deprecated anyway.
		UpdateTextureReplacement(FilePathRelativeFilename(symbolName.CStr(), symbolName.GetSizeInBytes()), filePath);
	}

	SharedPtr<Falcon::MovieClipInstance> GetRootMovieClip(
		HString sStateMachine,
		HString sTarget,
		CheckedPtr<Movie>& rpOwner) const;

	/**
	 * @return The DataStore that contains the current configuration
	 * settings of this UIManager.
	 */
	SharedPtr<DataStore> GetSettings() const
	{
		return m_pUIStack->GetSettings();
	}

	/**
	 * @return The FilePath associated with the global UI settings.
	 */
	FilePath GetSettingsFilePath() const
	{
		return m_pUIStack->GetSettingsFilePath();
	}

	// Return true if movieTypeName describes a native movie
	// (instantiated with Reflection and corresponding 1-to-1 with
	// a C++ instance or false if movieTypeName describes a
	// custom movie, which is backed by an application specific
	// implementation.
	Bool IsNativeMovie(HString movieTypeName) const;

	// IPoseable overrides
	virtual void PrePose(
		Float fDeltaTime,
		RenderPass& rPass,
		IPoseable* pParent = nullptr) SEOUL_OVERRIDE;

	virtual void Pose(
		Float fDeltaTime,
		RenderPass& rPass,
		IPoseable* pParent = nullptr) SEOUL_OVERRIDE;

	virtual void SkipPose(Float fDeltaTime) SEOUL_OVERRIDE;
	// /IPoseable overrides

	// Hook from enclosing UI (DevUI).
	Bool PassThroughAxisEvent(InputDevice* pInputDevice, InputDevice::Axis* pAxis) { return HandleAxisEvent(pInputDevice, pAxis); }
	Bool PassThroughButtonEvent(InputDevice* pInputDevice, InputButton eButtonID, ButtonEventType eEventType) { return HandleButtonEvent(pInputDevice, eButtonID, eEventType); }
	Bool PassThroughMouseMoveEvent(Int iX, Int iY) { return HandleMouseMoveEvent(iX, iY); }
	void PassThroughPose(Float fDeltaTime, RenderPass& rPass);

#if SEOUL_ENABLE_CHEATS
	/**
	 * @return Developer only feature - if enabled, draws all shapes
	 * that can accept input which pass the given mask. Set to 0u
	 * to disable input visualization (the default).
	 */
	UInt8 GetInputVisualizationMode() const
	{
		return m_uInputVisualizationMode;
	}

	void SetInputVisualizationMode(UInt8 uMode)
	{
		m_uInputVisualizationMode = uMode;
	}
#endif // /#if SEOUL_ENABLE_CHEATS

	/**
	 * Set a custom fallback handler when instantiation of an UI::Movie
	 * via Reflection fails.
	 */
	typedef Delegate<Movie*(HString typeName)> CustomUIMovieInstantiator;
	const CustomUIMovieInstantiator& GetCustomUIMovieInstantiator() const
	{
		return m_CustomUIMovieInstantiator;
	}

	void SetCustomUIMovieInstantiator(const CustomUIMovieInstantiator& delegate)
	{
		m_CustomUIMovieInstantiator = delegate;
	}

	Content::Handle<FCNFileData> GetFCNFileData(FilePath filePath)
	{
		return m_FCNFiles.GetContent(filePath);
	}

	Renderer& GetRenderer() const
	{
		return *m_pRenderer;
	}

	Content::Handle<Falcon::CookedTrueTypeFontData> GetTrueTypeFontData(HString fontName, Bool bBold, Bool bItalic);
	Bool GetFontOverrides(HString fontName, Bool bBold, Bool bItalic, Falcon::FontOverrides& rOverrides) const;

	const Point2DInt& GetMousePosition() const
	{
		return m_MousePosition;
	}

	// Initiate/end text editing of a particular Falcon::EditTextInstance.
	Bool StartTextEditing(
		Movie* pOwnerMovie,
		SharedPtr<Falcon::MovieClipInstance> pEventReciever,
		Falcon::EditTextInstance* pInstance,
		const String& sDescription,
		const StringConstraints& constraints,
		Bool bAllowNonLatinKeyboard);
	void StopTextEditing();

	/**
	 * @return True if the UI system is waiting for FCN files to load, false otherwise.
	 * While waiting for loads, the following are true:
	 * - input is suppressed.
	 * - the condition, transition, and goto queues are suppressed.
	 * - UI::Movie::OnTick() is not called.
	 */
	Bool IsWaitingForLoads() const
	{
		return m_WaitingForLoads.IsLoading();
	}

	void DebugLogEntireUIState() const;

	HString GetStateMachineCurrentStateId(HString sStateMachineName) const;

	// Developer only utility. Return a list of points that can be potentially
	// hit based on the input test mask. This applies to all state machines and all
	// movies currently active.
	typedef Vector<HitPoint, MemoryBudgets::UIRuntime> HitPoints;
	void GetHitPoints(UInt8 uInputMask, HitPoints& rvHitPoints) const;

	// Developer only utility - retrieve a read-only reference to the current UI stack.
	const Stack::StackVector& GetStack() const
	{
		return m_pUIStack->GetStack();
	}

	/**
	 * @return The path to the globally configured set of stage 3D
	 * settings.
	 */
	FilePath GetStage3DSettingsFilePath() const
	{
		return m_pUIStack->GetStage3DSettingsFilePath();
	}

	/** @return A read-only reference to the global table of stage 3D settings. */
	const Stage3DSettingsTable& GetStage3DSettingsTable() const
	{
		return m_pUIStack->GetStage3DSettingsTable();
	}

	/** Binding for Falcon, exposes global Falcon stage 3D settings. */
	CheckedPtr<Falcon::Stage3DSettings> GetStage3DSettings(HString id) const
	{
		return m_pUIStack->GetStage3DSettings(id);
	}

	/**
	 * @return The path to the globally configured set of text effect
	 * settings.
	 */
	FilePath GetTextEffectSettingsFilePath() const
	{
		return m_pUIStack->GetTextEffectSettingsFilePath();
	}

	/** @return A read-only reference to the global table of text effect settings. */
	const TextEffectSettingsTable& GetTextEffectSettingsTable() const
	{
		return m_pUIStack->GetTextEffectSettingsTable();
	}

	/**
	 * Binding for Falcon, exposes global text effect settings that can be referenced by
	 * individual text chunks for advanced rendering settings.
	 */
	CheckedPtr<Falcon::TextEffectSettings> GetTextEffectSettings(HString id) const
	{
		return m_pUIStack->GetTextEffectSettings(id);
	}

	Viewport ComputeViewport() const;

	const Vector2D& GetFixedAspectRatio() const
	{
		return m_vFixedAspectRatio;
	}

	const Vector2D& GetMinAspectRatio() const
	{
		return m_vMinAspectRatio;
	}

	FixedAspectRatio::Enum GetFixedAspectRatioMode() const
	{
		return FixedAspectRatio::ToEnum(m_vFixedAspectRatio);
	}

	void SetFixedAspectRatio(Float fNumerator, Float fDenominator)
	{
		m_vFixedAspectRatio.X = fNumerator;
		m_vFixedAspectRatio.Y = fDenominator;
	}

	void SetFixedAspectRatio(FixedAspectRatio::Enum eMode)
	{
		(void)FixedAspectRatio::ToRatio(eMode, m_vFixedAspectRatio);
	}

	Float ComputeUIRendererFxCameraWorldHeight(const Viewport& viewport) const;

	// Instantiate a movie. Will be consumed from the pre-fetched waiting set
	// if possible, otherwise it will be created fresh.
	CheckedPtr<Movie> InstantiateMovie(HString typeName);

#if !SEOUL_SHIP
	// Validate a specific file - expected to be a .SWF file. Will also
	// validate against the corresponding .FLA file (if it exists).
	Bool ValidateUiFile(FilePath filePath, Bool bSynchronous);
	// Validate a specific file - can be a .SWF or .FLA file. Will also
	// validate against the corresponding .FLA or .SWF file (if it exists).
	Bool ValidateUiFile(const String& sFilename, Bool bSynchronous);

	// Developer only utility - runs a validation pass on all
	// SWF and FLA files available to the app. Errors generate warnings.
	// Synchronous or not based on given argument.
	//
	// Return value is always true unless synchronous is also
	// true, in which case it is only true if all SWF and FLA files
	// were validated with no warnings or errors.
	Bool ValidateUiFiles(const String& sExcludeWildcard, Bool bSynchronous);
#endif // /!SEOUL_SHIP

	/**
	 * Current use case is for a FTUE. When not empty, input is limited exclusively
	 * to this MovieClip set.
	 */
	typedef HashSet< SharedPtr<Falcon::MovieClipInstance>, MemoryBudgets::UIData > InputWhitelist;
	void AddToInputWhitelist(const SharedPtr<Falcon::MovieClipInstance>& p);
	void ClearInputWhitelist();
	void RemoveFromInputWhitelist(const SharedPtr<Falcon::MovieClipInstance>& p);
#if !SEOUL_SHIP
	Vector<String> DebugGetInputWhitelistPaths() const;
#endif

	/**
	 * Enable/disable UI action events - when
	 * disabled, only mouse movement and clicks/taps
	 * are allowed.
	 */
	void SetInputActionsEnabled(Bool bEnabled)
	{
		m_bInputActionsEnabled = bEnabled;
	}

	Bool HasPendingTransitions() const;

	HString GetInputWhiteListBeginState() const;
	Bool MovieStateMachineRespectsInputWhiteList(const Movie* pMovie) const;
	Bool StateMachineRespectsInputWhiteList(const HString sStateMachineName) const;

	/**
	 * Special handling around condition variables used to control transition from
	 * patching to full game state in a game application.
	 *
	 * @params[in] bForceImmediate If true, restart is triggered immediately and occurs
	 * without delay. Otherwise, may be gated by one or more latching variables
	 * that must become false before the restart will be triggered.
	 */
	void TriggerRestart(Bool bForceImmediate);

	/**
	 * Handling for FCNFiles in the process of loading.
	 * Used for resolving sources for content within the FCNFile.
	 * e.g. a Bitmap whose source is within the FCNFile that contains it.
	 */
private:
	Mutex m_InProgressFCNFileMutex;
	HashTable<FilePath, SharedPtr<Falcon::FCNFile> > m_tInProgressFCNFiles;
public:
	void AddInProgressFCNFile(const FilePath& filePath, const SharedPtr<Falcon::FCNFile>& pFileData);
	Bool GetInProgressFCNFile(const FilePath& filePath, SharedPtr<Falcon::FCNFile>& pFileData) const;
	void RemoveInProgressFCNFile(const FilePath& filePath);

private:
	void InternalApplyAspectRatioSettings(Bool bUpdate);
	Bool InternalCheckAndWaitForLoads(CheckedPtr<Stack::StateMachine> pStateMachine, HString targetStateIdentifier);
	void InternalClear(Bool bClearFCNData, Bool bShutdown);
	void InternalClearInputCapture();
	void InternalClearInputOver();
	void InternalClearPrep(Bool bDestroyStack);
	void InternalEvaluateWantsRestart();
	void InternalHandleInputAndAdvance(Float fDeltaTime);
	void InternalPose(RenderPass& rPass);

	/**
	 * Helper structure used to enqueue modifications of condition variables. Actual
	 * modifications are applied at a specific point in the UI update flow, the queue is
	 * used to gather modifications over the course of a frame.
	 */
	struct PackedUpdate
	{
		PackedUpdate(HString name, HString value)
			: m_Name(name)
			, m_Value(value)
		{
		}

		HString m_Name;
		HString m_Value;
	};

	CustomUIMovieInstantiator m_CustomUIMovieInstantiator;
	InputWhitelist m_InputWhitelist;
	Mutex m_InputWhitelistMutex;

	typedef AtomicRingBuffer<PackedUpdate*, MemoryBudgets::UIRuntime> ConditionQueue;
	ConditionQueue m_UIConditionQueue;

	typedef AtomicRingBuffer<PackedUpdate*, MemoryBudgets::UIRuntime> GotoStateQueue;
	GotoStateQueue m_UIGotoStateQueue;

	typedef AtomicRingBuffer<PackedUpdate*, MemoryBudgets::UIRuntime> TriggerQueue;
	TriggerQueue m_UITriggerQueue;

	void ApplyGotoStates(Bool& rbStateTransitionActivated);
	Bool EvaluateConditionsUntilSettled(Bool& rbStateTransitionActivated, UInt32 nMaxIterations = 10u);
	void ApplyConditionsToStateMachines();
	void ApplyUIConditionsAndTriggersToStateMachines(Bool& rbStateTransitionActivated);
#if SEOUL_HOT_LOADING
	Bool ApplyHotReload();
#endif

	Bool HandleAxisEvent(InputDevice* pInputDevice, InputDevice::Axis* pAxis);
	Bool HandleButtonEvent(InputDevice* pInputDevice, InputButton eButtonID, ButtonEventType eEventType);
	Bool HandleMouseMoveEvent(Int iX, Int iY);

	void SetConditionsForTransition(const DataStore& stateMachineDataStore, const DataNode& activatedTransition);
	Bool TransitionCapturesTriggers(const DataStore& stateMachineDataStore, const DataNode& activatedTransition);

	/**
	 * Queued input event received from InputManager, dispatched to screens
	 * during Pose().
	 */
	struct QueuedInputEvent SEOUL_SEALED
	{
		QueuedInputEvent(InputDevice::Type eDeviceType, InputAxis eAxis, Float fState)
			: m_eEventType(kEventTypeAxis)
			, m_eDeviceType(eDeviceType)
			, m_eAxis(eAxis)
			, m_fState(fState)
		{
		}

		QueuedInputEvent(InputDevice::Type eDeviceType, InputButton eButtonID, ButtonEventType eEventType)
			: m_eEventType(kEventTypeButton)
			, m_eDeviceType(eDeviceType)
			, m_eButtonID(eButtonID)
			, m_eButtonEventType(eEventType)
		{
		}

		QueuedInputEvent(InputDevice::Type eDeviceType, UniChar cChar)
			: m_eEventType(kEventTypeChar)
			, m_eDeviceType(eDeviceType)
			, m_cChar(cChar)
		{
		}

		QueuedInputEvent(HString bindingName, ButtonEventType eEventType)
			: m_eEventType(kEventTypeBinding)
			, m_eDeviceType(InputDevice::Unknown)
			, m_BindingName(bindingName)
			, m_eButtonID(InputButton::ButtonUnknown)
			, m_eButtonEventType(eEventType)
		{
			SEOUL_ASSERT(!bindingName.IsEmpty());
		}

		enum EEventType
		{
			kEventTypeAxis,
			kEventTypeButton,
			kEventTypeChar,
			kEventTypeBinding,
		};

		/** Type of this event */
		EEventType m_eEventType;

		/** Device type which generated the input event */
		InputDevice::Type m_eDeviceType;

		/** Binding name (binding events only) */
		HString m_BindingName;

		union
		{
			/** Axis-specific data */
			struct
			{
				InputAxis m_eAxis;
				Float m_fState;
			};

			/** Button-specific data */
			struct
			{
				InputButton m_eButtonID;
				ButtonEventType m_eButtonEventType;
			};

			/** Char-specific data */
			struct
			{
				UniChar m_cChar;
			};
		};
	};

	typedef Vector<QueuedInputEvent, MemoryBudgets::UIRuntime> InputEvents;
	InputEvents m_vInputEventsToDispatch;
	InputEvents m_vPendingInputEvents;

	FilePath const m_GuiConfigFilePath;
	Vector2D m_vFixedAspectRatio;
	Vector2D m_vMinAspectRatio;
	Float m_fLastBackBufferAspectRatio;
	ScopedPtr<Stack> m_pUIStack;
	ScopedPtr<Renderer> m_pRenderer;
	Movie* m_pTextEditingMovie;
	SharedPtr<Falcon::EditTextInstance> m_pTextEditingInstance;
	SharedPtr<Falcon::MovieClipInstance> m_pTextEditingEventReceiver;
	StringConstraints m_TextEditingConstraints;
	String m_sTextEditingBuffer;
	Movie* m_pInputOverMovie;
	SharedPtr<Falcon::MovieClipInstance> m_pInputOverInstance;
	Movie* m_pInputCaptureMovie;
	SharedPtr<Falcon::MovieClipInstance> m_pInputCaptureInstance;
	SharedPtr<Falcon::EditTextLink> m_pInputCaptureLink;
	Bool m_bMouseIsDownOutsideOriginalCaptureInstance;
	Point2DInt m_InputCaptureMousePosition;
	Point2DInt m_MousePosition;
	Point2DInt m_PreviousMousePosition;
	Bool m_bInputActionsEnabled;
	Int m_iHorizontalInputCaptureDragThreshold;
	Int m_iVerticalInputCaptureDragThreshold;
	UInt8 m_uInputCaptureHitTestMask;
	StackFilter const m_eStackFilter;

	Viewport m_LastViewport;

	// Begin UI::Manager friends
	friend class MovieContent;
	friend class MovieInternal;
	friend class Stack;
	friend struct Content::Traits<FCNFileData>;
	Content::Store<FCNFileData> m_FCNFiles;
	friend struct Content::Traits<Falcon::CookedTrueTypeFontData>;
	Content::Store<Falcon::CookedTrueTypeFontData> m_UIFonts;
	// /End UI::MovieInternal friends

	friend class ContentLoader;
	Atomic32Value<Bool> m_bInPrePose;

	/**
	 * Utility meant to be the construct delegate in a ScopedAction<>,
	 * marks that UI::Manager is currently in its PrePose() method.
	 */
	void BeginPrePose()
	{
		m_bInPrePose = true;
	}

	/**
	 * Utility meant to be the destruct delegate in a ScopedAction<>,
	 * marks that UI::Manager is no longer in its PrePose() method.
	 */
	void EndPrePose()
	{
		m_bInPrePose = false;
	}

#if SEOUL_ENABLE_CHEATS
	UInt8 m_uInputVisualizationMode;
#endif // /#if SEOUL_ENABLE_CHEATS

	/**
	 * Utility, used to track file dependencies of movies we're about
	 * to transition to.
	 */
	class WaitingForLoads SEOUL_SEALED
	{
	public:
		struct WaitingForData SEOUL_SEALED
		{
			WaitingForData()
				: m_hData()
				, m_pMovieData()
				, m_MovieTypeName()
			{
			}

			Content::Handle<FCNFileData> m_hData;
			CheckedPtr<Movie> m_pMovieData;
			HString m_MovieTypeName;
		}; // struct WaitingForData

		WaitingForLoads();
		~WaitingForLoads();

		// Append a new data instance to the waiting for loads set.
		void Add(const WaitingForData& data);

		// Dispatch broadcast events to any suspended movies.
		Bool BroadcastEventToSuspended(
			HString sTarget,
			HString sEvent,
			const Reflection::MethodArguments& aArguments,
			Int nArgumentCount) const;

		// Immediately clear waitings load - not a part of normal code flow. Meant
		// to be used in UI::Manager::Clear() and similar code paths only.
		void Clear();

		// Clear any suspended movies. Destroys the movies and discards their data.
		void ClearSuspended();

		/** @return True if there are any entires on the waiting for loads set, false otherwise. */
		Bool HasEntries() const
		{
			return !m_vWaiting.IsEmpty();
		}

		// Attempt to consume an instance from the waiting for loads set.
		CheckedPtr<Movie> Instantiate(HString movieTypeName);

		/** @return True if any dependencies are still loading, false otherwise. */
		Bool IsLoading() const
		{
			return m_bLoading;
		}

		// Per-frame handling of the waiting for loads set.
		void Process();

		/** Retrieve currently associated state machine. */
		CheckedPtr<Stack::StateMachine> GetMachine() const
		{
			return m_pMachine;
		}

		/** Associate an active state machine with the set. */
		void SetMachine(CheckedPtr<Stack::StateMachine> pMachine)
		{
			m_pMachine = pMachine;
		}

		/** Attempt to insert a movie into the suspend table - fails if one already exists with the movie's type name. */
		Bool SuspendMovie(CheckedPtr<Movie> p);

	private:
		SEOUL_DISABLE_COPY(WaitingForLoads);

		CheckedPtr<Movie> NewMovie(HString movieTypeName);

		// Used for caching suspendable movies.
		typedef HashTable<HString, CheckedPtr<Movie>, MemoryBudgets::UIRuntime> SuspendedMovies;
		SuspendedMovies m_tSuspended;
		CheckedPtr<Movie> ResumeMovie(HString movieTypeName);

		typedef Vector< WaitingForData, MemoryBudgets::UIRuntime > WaitingForFCNFiles;
		CheckedPtr<Stack::StateMachine> m_pMachine;
		WaitingForFCNFiles m_vWaiting;
		UInt32 m_uLastConstructFrame;
		Bool m_bLoading;
	}; // class WaitingForLoads
	WaitingForLoads m_WaitingForLoads;

	friend class Movie;
	friend class State;
	void DestroyMovie(CheckedPtr<Movie>& rpMovie);

	Bool HitTest(
		UInt8 uMask,
		const Point2DInt& mousePosition,
		Movie*& rpHitMovie,
		SharedPtr<Falcon::MovieClipInstance>& rpHitInstance,
		SharedPtr<Falcon::Instance>& rpLeafInstance,
		Vector<Movie*>* rvPassthroughInputs) const;
	Bool SendInputEvent(InputEvent eInputEvent);
	Bool SendButtonEvent(
		Seoul::InputButton eButtonID,
		Seoul::ButtonEventType eButtonEventType);

	enum ClearAction
	{
		kClearActionNone,
		kClearActionIncludingFCN,
		kClearActionExcludingFCN,
	};
	ClearAction m_ePendingClear;

	struct PersistentBroadcastEvent SEOUL_SEALED
	{
		PersistentBroadcastEvent()
			: m_sTarget()
			, m_sEvent()
			, m_aArguments()
			, m_uArgumentCount(0)
		{
		}

		HString m_sTarget;
		HString m_sEvent;
		Reflection::MethodArguments m_aArguments;
		Int m_uArgumentCount;
	}; // struct PersistentBroadcastEvent
	typedef List<PersistentBroadcastEvent, MemoryBudgets::UIRuntime> PersistentBroadcastEvents;
	PersistentBroadcastEvents m_lPersistentBroadcastEvents;

#if SEOUL_HOT_LOADING
	Bool m_bInHotReload;
	Bool m_bPendingHotReload;
	HashTable<String, SharedPtr<DataStore>, MemoryBudgets::UIRuntime> m_tHotLoadStash;
#endif // /#if SEOUL_HOT_LOADING

	// ITextEditable overrides
	virtual void TextEditableApplyChar(UniChar c) SEOUL_OVERRIDE;
	virtual void TextEditableApplyText(const String& sText) SEOUL_OVERRIDE;
	virtual void TextEditableEnableCursor() SEOUL_OVERRIDE;
	virtual void TextEditableStopEditing() SEOUL_OVERRIDE;
	// /ITextEditable

	Conditions m_tConditions;
	Mutex m_ConditionTableMutex;
	Atomic32Value<Bool> m_bWantsRestart;

#if SEOUL_LOGGING_ENABLED
	TriggerHistory m_vTriggerHistory;
	UInt32 m_uTriggerHistoryHead;
	void AddTriggerHistory(
		HString triggerName,
		HString stateMachine = HString(),
		HString fromState = HString(),
		HString toState = HString())
	{
		m_vTriggerHistory[m_uTriggerHistoryHead++] = TriggerHistoryEntry(
			triggerName,
			stateMachine,
			fromState,
			toState);
		m_uTriggerHistoryHead = (m_uTriggerHistoryHead % m_vTriggerHistory.GetSize());
	}
#endif // /#if SEOUL_LOGGING_ENABLED

	SEOUL_DISABLE_COPY(Manager);
}; // class UI::Manager

} // namespace Seoul::UI

#endif // include guard
