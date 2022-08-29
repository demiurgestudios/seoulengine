/**
 * \file UIMovie.h
 * \brief A UI::Movie encapsulates, in most cases, a Falcon scene
 * graph, and usually corresponds to a single instantiation of
 * a Flash SWF file. It can also be used as a UI "state", to tie
 * behavior to various UI contexts, in which case it will have no
 * corresponding Flash SWF file (the Falcon graph will be empty).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_MOVIE_H
#define UI_MOVIE_H

#include "CheckedPtr.h"
#include "HashSet.h"
#include "FalconAdvanceInterface.h"
#include "FilePath.h"
#include "InputDevice.h"
#include "SharedPtr.h"
#include "Prereqs.h"
#include "ReflectionTypeUtil.h"
#include "SeoulProfiler.h"
#include "Settings.h"
#include "UIMotionCollection.h"
#include "UIMovieContent.h"
#include "UIMovieHandle.h"
#include "UITween.h"
namespace Seoul { class RenderPass; }
namespace Seoul { namespace Falcon { class HitTester; } }
namespace Seoul { namespace Falcon { class Instance; } }
namespace Seoul { namespace UI { class Animation2DNetworkInstance; } }

namespace Seoul
{

// Forward declarations
class RenderCommandStreamBuilder;
class RenderSurface2D;

namespace UI
{

class FxInstance;
struct HitPoint;
class MovieInternal;
class Renderer;
class State;
class TweenCompletionInterface;

/**
 * UI::Movie is the base class for all Flash Movie files (*.SWF). A subclass of UI::Movie should
 * be defined for each .SWF file that will be used in the current UI system, and in some instances, more than
 * one subclass of UI::Movie will use the same SWF file (multiple confirmation dialogues).
 */
class Movie SEOUL_ABSTRACT : public Falcon::AdvanceInterface
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(Movie);

	virtual ~Movie();

	/**
	 * @return The FilePath to the .fcn data associated with this UIMovie.
	 * Will be invalid if this UI::Movie has no .fcn file.
	 */
	FilePath GetFilePath() const
	{
		return m_FilePath;
	}

	/**
	 * @return Indirect handle reference to this UIMovie.
	 */
	const MovieHandle& GetHandle() const
	{
		return m_hThis;
	}

	Falcon::HitTester GetHitTester() const;

	/**
	 * @return The movie type of this UIMovie. May be equal to
	 * GetReflectionThis().GetType().GetName() in cases where an explicit
	 * C++ runtime type is available to back the movie. Must always be
	 * unique within the state machine of the movie instance.
	 */
	HString GetMovieTypeName() const
	{
		return m_MovieTypeName;
	}

	// Kick off a sound event contained in this UI::Movie
	void StartSoundEvent(HString soundEventId);

	// Kick off a sound event contained in this UI::Movie that will be stopped when the movie is destroyed.
	void StartSoundEventWithOptions(HString soundEventId, Bool bStopOnDestruction);

	// Methods to trigger and manipulate tracked sound events - tracked sound events can be manipulated
	// by id once they are started.
	Int32 StartTrackedSoundEvent(HString soundEventId);
	Int32 StartTrackedSoundEventWithOptions(HString soundEventId, Bool bStopOnDestruction);
	void StopTrackedSoundEvent(Int32 iId);
	void StopTrackedSoundEventImmediately(Int32 iId);
	void SetTrackedSoundEventParameter(Int32 iId, HString parameterName, Float fValue);
	void TriggerTrackedSoundEventCue(Int32 iId);

	// Get the frame delta time (1.0 / FPS) of this UIMovie.
	Float GetFrameDeltaTimeInSeconds() const;

	// Binding for script.
	Vector2D GetMousePositionFromWorld(const Vector2D& v) const;

	// Attempt to populate rpRoot with a reference counted pointer of
	// the root MovieClip of this Movie's scene. Return true on success
	// (rpRoot was modified and is non-null) or false otherwise.
	Bool GetRootMovieClip(SharedPtr<Falcon::MovieClipInstance>& rpRoot) const;

	Int AddMotion(const SharedPtr<Motion>& pMotion);

	void CancelMotion(Int iIdentifier);
	void CancelAllMotions(const SharedPtr<Falcon::Instance>& pInstance);

	Int AddTween(
		const SharedPtr<Falcon::Instance>& pInstance,
		TweenTarget eTarget,
		TweenType eTweenType,
		Float fStartValue,
		Float fEndValue,
		Float fDurationInSeconds,
		const SharedPtr<TweenCompletionInterface>& pCompletionInterface = SharedPtr<TweenCompletionInterface>());

	void CancelTween(Int iIdentifier);
	void CancelAllTweens(const SharedPtr<Falcon::Instance>& pInstance);

	// Whether we're paused. When true,
	// we don't tick/process several Falcon-related functions.
	Bool IsPaused() const
	{
		return m_bPaused;
	}

	void SetPaused(bool bPaused)
	{
		m_bPaused = bPaused;
	}

	Bool StateMachineRespectsInputWhitelist() const;

	HString GetStateMachineName() const;

	// If this is true, movies "under" this movie, either in the current state or in state machine
	// below this, will not get rendered.
	Bool BlocksRenderBelow() const
	{
		return m_bBlocksRenderBelow;
	}

	Bool FlushesDeferredDraw() const
	{
		return m_bFlushDeferredDraw;
	}

	void SetBlocksRenderBelow(Bool bValue)
	{
		m_bBlocksRenderBelow = bValue;
	}

	Bool AllowInputToScreensBelow() const
	{
		return m_bAllowInputToScreensBelow;
	}

	HString PassthroughInputTrigger() const
	{
		return m_PassthroughInputTrigger;
	}

	void SetPassthroughInputTrigger(HString value)
	{
		m_PassthroughInputTrigger = value;
	}

	HString PassthroughInputFunction() const
	{
		return m_PassthroughInputFunction;
	}

	void SetPassthroughInputFunction(HString value)
	{
		m_PassthroughInputFunction = value;
	}

	Bool AcceptingInput() const
	{
		return m_bAcceptInput;
	}

	void SetAcceptInput(Bool bAccept)
	{
		m_bAcceptInput = bAccept;
	}

	/**
	 * @return The movie next to this in the UI::State stack.
	 */
	CheckedPtr<Movie> GetNextMovie() const
	{
		return m_pNext;
	}

	void SetMovieRendererDependentState();

	/**
	 * @return The movie prior to this in the UI::State stack.
	 */
	CheckedPtr<Movie> GetPrevMovie() const
	{
		return m_pPrev;
	}

	/**
	 * @return True if this UI::Movie is the top of is stack (within an UI::State), false
	 * otherwise.
	 */
	Bool IsTop() const
	{
		return !m_pPrev.IsValid();
	}

	// Return the given render viewport bounds in this movie's world space.
	Falcon::Rectangle ViewportToWorldBounds(const Viewport& viewport) const;
	Falcon::Rectangle ViewportToWorldBounds() const
	{
		return ViewportToWorldBounds(GetViewport());
	}

	// Return the stage position in world twips from the specified viewport mouse position in screen pixels.
	Vector2D GetMousePositionInWorld(
		const Point2DInt& mousePosition,
		Bool& rbOutsideViewport) const;
	Vector2D GetMousePositionInWorld(
		Int iX,
		Int iY,
		Bool& rbOutsideViewport) const
	{
		return GetMousePositionInWorld(Point2DInt(iX, iY), rbOutsideViewport);
	}
	Vector2D GetMousePositionInWorld(const Point2DInt& mousePosition) const
	{
		Bool bUnusedOutsideViewport;
		return GetMousePositionInWorld(mousePosition, bUnusedOutsideViewport);
	}
	Vector2D GetMousePositionInWorld(Int iX, Int iY) const
	{
		Bool bUnusedOutsideViewport;
		return GetMousePositionInWorld(Point2DInt(iX, iY) , bUnusedOutsideViewport);
	}

	// Returns the height or width of the movie. Defaults to 1.0 if this Movie has no stage dimensions.
	Float GetMovieHeight() const;
	Float GetMovieWidth() const;

	void SetAllowInputToScreensBelow(Bool b)
	{
		m_bAllowInputToScreensBelow = b;
	}

	const MovieContent& GetContent() const
	{
		return m_Content;
	}

	MovieContent& GetContent()
	{
		return m_Content;
	}

	Vector3D ToFxWorldPosition(
		Float fX,
		Float fY,
		Float fDepth3D = 0.0f) const;

#if SEOUL_WITH_ANIMATION_2D
	void AddActiveAnimation2D(Animation2DNetworkInstance* pNetworkInstance);
	void RemoveActiveAnimation2D(Animation2DNetworkInstance* pNetworkInstance);
#endif // /#if SEOUL_WITH_ANIMATION_2D

	void AddActiveFx(FxInstance* pFxInstance);
	void RemoveActiveFx(FxInstance* pFxInstance);
	void EnableTickEvents(Falcon::Instance* pInstance);
	void DisableTickEvents(Falcon::Instance* pInstance);
	void EnableTickScaledEvents(Falcon::Instance* pInstance);
	void DisableTickScaledEvents(Falcon::Instance* pInstance);

	virtual void OnEditTextStartEditing(const SharedPtr<Falcon::MovieClipInstance>& pInstance) {};
	virtual void OnEditTextStopEditing(const SharedPtr<Falcon::MovieClipInstance>& pInstance) {};
	virtual void OnEditTextApply(const SharedPtr<Falcon::MovieClipInstance>& pInstance) {};

	/**
	 * Sanity check that OnConstructMovie() has completed.
	 *
	 * Primarily useful to subclasses to verify initialization
	 * and destruction behavior.
	 */
	Bool IsConstructed() const
	{
		return m_bConstructed;
	}

	/**
	* Return the last viewport captured during rendering. May be
	* different from GetViewport() depending on timing.
	*/
	const Viewport& GetLastViewport() const
	{
		return m_LastViewport;
	}
	void SetLastViewport(const Viewport& viewport)
	{
		if (m_LastViewport.m_iViewportWidth > 0 &&
			m_LastViewport != viewport)
		{
			m_bLastViewportChanged = true;
		}
		m_LastViewport = viewport;
	}

	// Convenience, get the duration of a factoried FX
	// based on its template id. Returns 0.0f if the
	// given FX id is invalid.
	Float GetFxDuration(HString id);

	// Check whether there are any active FX playing in this
	// movie.
	Bool HasActiveFx(Bool bIncludeLooping);

	// Utility for several instance types that are directly ticked
	// without graph traversal - verify that this instance
	// is visible and can be reached from the root
	// node of this UIMovie.
	Bool IsReachableAndVisible(Falcon::Instance const* pInstance) const;

#if SEOUL_HOT_LOADING
	/**
	 * All movies are part of a hot reload by default. Some movies can explicitly exclude themselves,
	 * if they know that any hot loads which normally affect the UI, do not affect them.
	 */
	virtual Bool IsPartOfHotReload() const
	{
		return true;
	}
#endif // /#if SEOUL_HOT_LOADING

	// Compute the top/bottom coordinates of the rendering viewport. Can be overriden in subclasses to customize
	// behavior.
	virtual Vector2D ComputeStageTopBottom(const Viewport& viewport, Float fStageHeight) const;

	// Dev only additional disambiguator for screens that are effectively
	// multiple screens. Currently used by automated testing to identify
	// changes in state that are otherwise invisible to a testing harness.
	virtual HString GetDevOnlyInternalStateId() const { return HString(); }

protected:
	Movie();

	// Falcon::AddInterface overrides
	virtual void FalconOnAddToParent(
		Falcon::MovieClipInstance* pParent,
		Falcon::Instance* pInstance,
		const HString& sClassName) SEOUL_OVERRIDE
	{
		// Nop by default.
	}

	virtual void FalconOnClone(
		Falcon::Instance const* pFromInstance,
		Falcon::Instance* pToInstance) SEOUL_OVERRIDE
	{
		// Nop by default.
	}
	// /Falcon::AddInterface overrides

	// Falcon::AdvanceInterface overrides
	virtual void FalconDispatchEnterFrameEvent(
		Falcon::Instance* pInstance) SEOUL_OVERRIDE;
	virtual void FalconDispatchEvent(
		const HString& sEventName,
		Falcon::SimpleActions::EventType eType,
		Falcon::Instance* pInstance) SEOUL_OVERRIDE;
	virtual float FalconGetDeltaTimeInSeconds() const SEOUL_OVERRIDE;
	virtual bool FalconLocalize(
		const HString& sLocalizationToken,
		String& rsLocalizedText) SEOUL_OVERRIDE SEOUL_SEALED;
	// /Falcon::AdvanceInterface overrides

	// Developer only utility. Return a list of points that can be potentially
	// hit based on the input test max. This applies to all state machines and all
	// movies currently active.
	typedef Vector<HitPoint, MemoryBudgets::UIRuntime> HitPoints;
	Bool GetHitPoints(
		HString stateMachine,
		HString state,
		UInt8 uInputMask,
		HitPoints& rvHitPoints) const;

	// Input entry points that can be overriden in subclasses for specialized input handling behavior.
	virtual MovieHitTestResult OnHitTest(
		UInt8 uMask,
		const Point2DInt& mousePosition,
		Movie*& rpHitMovie,
		SharedPtr<Falcon::MovieClipInstance>& rpHitInstance,
		SharedPtr<Falcon::Instance>& rpLeafInstance,
		Vector<Movie*>* rvPassthroughInputs) const;
	virtual void OnPick(
		const Point2DInt& mousePosition,
		Vector< SharedPtr<Falcon::Instance> , MemoryBudgets::UIRuntime>& rv) const;
	virtual MovieHitTestResult OnSendInputEvent(InputEvent eInputEvent);
	virtual MovieHitTestResult OnSendButtonEvent(
		Seoul::InputButton eButtonID,
		Seoul::ButtonEventType eButtonEventType);

	// Polymorphic equivalents to the class's constructor and destructor.
	// Typically low-level and not commonoly overriden.
	virtual void OnConstructMovie(HString movieTypeName);
	virtual void OnDestroyMovie();

	// Optional - if CanSuspendMovie() returns true, then at the point where OnDestroyMovie()
	// would normally be called, OnSuspendMovie() will instead be called. After completion,
	// the movie instance will remain in memory (its destructor will *not* be called).
	//
	// Later, at the point where the movie would normally be instantiated and
	// OnConstructMovie() would be called, the movie will be re-used and instead
	// OnResumeMovie() will be called.
	//
	// Note that the normal destruction flow (OnDestroyMovie() -> ~UIMovie()) will be invoked
	// in shutdown or soft reboot cases, so they must still be supported even if CanSuspendMovie()
	// is supported.
	//
	// IMPORTANT: If CanSuspendMovie() must remain consistent - if it ever returns false it
	// must always return false or vice versa.
	virtual Bool CanSuspendMovie() const { return false; }
	virtual void OnResumeMovie() {}
	virtual void OnSuspendMovie() {}

	// Override this method to performce per-frame advance operations,
	// after all other tick processing has completed (e.g.
	// OnTick(), deferred event processing, and Advance
	// of the Falcon scene graph). Not called while the movie is paused.
	virtual void OnAdvance(Float fDeltaTimeInSeconds) {}

	// Override this method to performce per-frame advance operations
	// whether paused or not, after all other tick processing has completed.
	virtual void OnAdvanceWhenBlocked(Float fDeltaTimeInSeconds) {}

	// Override this method to listen for EnterState() events.
	// This method is called whenever a state transition occurs
	// and this UI::Movie will be in the state being entered. If
	// bWasInPreviousState is true, then this UI::Movie was also in the previous
	// state and is stayling alive across the state transition.
	virtual void OnEnterState(CheckedPtr<State const> pPreviousState, CheckedPtr<State const> pNextState, Bool bWasInPreviousState);

	// Override this method to listen for ExitState() events.
	// This method is called whenever a state transition occurs
	// and this UI::Movie was in the state being exited. If
	// bIsInNextState is true, then this UI::Movie will persistent
	// into the next state. Otherwise, it will be destroyed shortly
	// after this call.
	virtual void OnExitState(CheckedPtr<State const> pPreviousState, CheckedPtr<State const> pNextState, Bool bIsInNextState);

	// Override this method to perform setup after a load.
	// This is called when the movie data (.SWF) and settings content are
	// loaded _or_ reloaded through hotloading.  Override this and make it your
	// main point of initialization to support hotloading.
	virtual void OnLoad() {}

	// Override this method to perform custom posing operations. In all but
	// very special cases, this method should be left without an override, or the base
	// class should be called to handle rendering from within the override.
	virtual void OnPose(RenderPass& rPass, Renderer& rRenderer);

#if SEOUL_ENABLE_CHEATS
	// Developer only method, performs a render pass to visualize input hit testable
	// areas.
	typedef HashSet< SharedPtr<Falcon::MovieClipInstance>, MemoryBudgets::UIData > InputWhitelist;
	virtual void OnPoseInputVisualization(
		const InputWhitelist& inputWhitelist,
		UInt8 uInputMask,
		RenderPass& rPass,
		Renderer& rRenderer);
#endif // /#if SEOUL_ENABLE_CHEATS

	// Override this method to perform per-frame update operations. This method
	// is called on the main thread, after state processing for the current
	// frame, but before any input processing and before Falcon Advance
	// or the overriden Advance() call.
	virtual void OnTick(RenderPass& rPass, Float fDeltaTimeInSeconds) {}

	// Equivalent to OnTick, but called when normal updates to the movie
	// are blocked. Normal logic is not expected to run, unless that logic
	// is completely independent  from loading.
	virtual void OnTickWhenBlocked(RenderPass& rPass, Float fDeltaTimeInSeconds) {}

	// Override this method to react to the per-frame Tick event. Called
	// as part of Advance() processing to run animations at full engine
	// frame rate (vs. Falcon movie framerate). Our typical scenario
	// is a movie file authored at 30 FPS with a game running at 60 FPS.
	virtual void OnDispatchTickEvent(Falcon::Instance* pInstance) const
	{
	}

	// Override this method to react to the time scaled per-frame Tick event. Called
	// as part of Advance() processing to run animations at full engine
	// frame rate (vs. Falcon movie framerate). Our typical scenario
	// is a movie file authored at 30 FPS with a game running at 60 FPS.
	virtual void OnDispatchTickScaledEvent(Falcon::Instance* pInstance) const
	{
	}

	// Override to provide custom broadcast event handling for a movie.
	virtual Bool OnTryBroadcastEvent(
		HString eventName,
		const Reflection::MethodArguments& aMethodArguments,
		Int nArgumentCount);

	virtual Bool AllowClickPassthroughToProceed(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance) const
	{
		return true;
	}

	virtual void OnGlobalMouseButtonPressed(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance)
	{
	}

	virtual void OnGlobalMouseButtonReleased(const Point2DInt& mousePosition)
	{
	}

	virtual void OnMouseButtonPressed(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance,
		Bool bInInstance)
	{
	}

	virtual void OnMouseButtonReleased(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance,
		Bool bInInstance,
		UInt8 uInputCaptureHitTestMask)
	{
	}

	virtual void OnMouseMove(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance,
		Bool bInInstance)
	{
	}

	virtual void OnMouseWheel(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance,
		Float fDelta)
	{
	}

	virtual void OnMouseOut(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance)
	{
	}

	virtual void OnMouseOver(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance)
	{
	}

	virtual void OnLinkClicked(
		const String& sLinkInfo,
		const String& sLinkType,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance)
	{
	}

	virtual void InvokePassthroughInputFunction()
	{
	}

#if SEOUL_HOT_LOADING
	virtual void OnHotLoadBegin() {}
	virtual void OnHotLoadEnd() {}
#endif

	CheckedPtr<State const> GetOwner() const { return m_pOwner; }
	virtual Viewport GetViewport() const;

	// We need to make the tween collection accessible to ScriptUIMovie so that
	// ScriptUIMovie can update TileFallingSettings.
	TweenCollection m_Tweens;

	ScopedPtr<MovieInternal> m_pInternal;

	MovieContent m_Content;

private:
	SEOUL_REFLECTION_FRIENDSHIP(Movie);

	MotionCollection m_MotionCollection;

	MovieHandle m_hThis;

	// Convenience utility used to wrap various types that
	// are ticked in a flat list. Enumeration is performed
	// over a locked snapshot of the list, available to
	// one context at a time.
	template <typename T>
	class TickContainer SEOUL_SEALED
	{
	public:
		typedef Vector<SharedPtr<T>, MemoryBudgets::UIRuntime> Snapshot;

		TickContainer()
			: m_Set()
			, m_vSnapshot()
			, m_Lock()
		{
		}

		~TickContainer()
		{
		}

		void Add(T* p)
		{
			(void)m_Set.Insert(p);
		}

		void Remove(T* p)
		{
			(void)m_Set.Erase(p);
		}

		/**
		 * Capture a snapshot of the current set
		 * for iteration. Snapshot is captured
		 * once for any recursive locking.
		 */
		const Snapshot& Lock()
		{
			// If we're the first locker,
			// repopulate the list.
			if (1 == ++m_Lock)
			{
				m_vSnapshot.Clear();
				m_vSnapshot.Reserve(m_Set.GetSize());
				for (auto const& e : m_Set)
				{
					m_vSnapshot.PushBack(SharedPtr<T>(e));
				}
			}

			return m_vSnapshot;
		}

		void Unlock()
		{
			// If we're the lost locker, release the snapshot.
			if (0 == --m_Lock)
			{
				m_vSnapshot.Clear();
			}
		}

	private:
		typedef HashSet<T*, MemoryBudgets::UIRuntime> Set;
		Set m_Set;
		Snapshot m_vSnapshot;
		Atomic32 m_Lock;

		SEOUL_DISABLE_COPY(TickContainer);
	}; // class TickContainer

	// Utility used to lock a TickContainer. Used
	// to enumerate members of the container while
	// still allowing mutation.
	template <typename T>
	class ContainerLock SEOUL_SEALED
	{
	public:
		typedef typename T::Snapshot::const_iterator const_iterator;
		typedef typename T::Snapshot::ConstIterator ConstIterator;

		ContainerLock(T& r)
			: m_r(r)
			, m_vSnapshot(r.Lock())
		{
		}

		~ContainerLock()
		{
			m_r.Unlock();
		}

		const_iterator begin() const { return m_vSnapshot.begin(); }
		const_iterator end() const { return m_vSnapshot.end(); }

		ConstIterator Begin() const { return m_vSnapshot.Begin(); }
		ConstIterator End() const { return m_vSnapshot.End(); }

	private:
		T& m_r;
		const typename T::Snapshot& m_vSnapshot;

		SEOUL_DISABLE_COPY(ContainerLock);
	}; // class ContainerLock

	typedef TickContainer<Falcon::Instance> TickEventTargets;
	TickEventTargets m_TickEventTargets;
	TickEventTargets m_TickScaledEventTargets;
#if SEOUL_WITH_ANIMATION_2D
	typedef TickContainer<Animation2DNetworkInstance> ActiveAnimation2DInstances;
	ActiveAnimation2DInstances m_ActiveAnimation2DInstances;
#endif // /#if SEOUL_WITH_ANIMATION_2D
	typedef TickContainer<FxInstance> ActiveFxInstances;
	ActiveFxInstances m_ActiveFxInstances;

	typedef Vector<SharedPtr<FxInstance>, MemoryBudgets::Scripting> ToRemoveFxQueue;
	ToRemoveFxQueue m_vToRemoveFxQueue;

	void AdvanceAnimations(Int iAdvanceCount, Float fFrameDeltaTimeInSeconds);

	Float m_fAccumulatedScaledFrameTime;
	CheckedPtr<State const> m_pOwner;
	CheckedPtr<Movie> m_pNext;
	CheckedPtr<Movie> m_pPrev;

	// Begin UI::FxInstance friend functions
	friend class FxInstance;
	void QueueFxToRemove(FxInstance* pInstance);
	// /End UI::FxInstance friend functions

	// Begin UI::State friend functions
	friend class State;
	void PrePose(RenderPass& rPass, Float fDeltaTimeInSeconds);
	void PrePoseWhenBlocked(RenderPass& rPass, Float fDeltaTimeInSeconds);
	void Advance(Float fDeltaTimeInSeconds);
	void AdvanceWhenBlocked(Float fDeltaTimeInSeconds);
	// /End UI::State friend functions

	// Begin UI::Manager friend
	friend class Manager;
	void ConstructMovie(HString movieTypeName);

	SEOUL_PROF_DEF_VAR(m_ProfAdvance)
	SEOUL_PROF_DEF_VAR(m_ProfOnEnterState)
	SEOUL_PROF_DEF_VAR(m_ProfOnExitState)
	SEOUL_PROF_DEF_VAR(m_ProfOnLoad)
	SEOUL_PROF_DEF_VAR(m_ProfPrePose)
	SEOUL_PROF_DEF_VAR(m_ProfPose)

	// Captured during rendering, last viewport
	// used for rendering.
	Viewport m_LastViewport;

	// Tracks viewport changes, used to
	// send viewport changed events.
	Bool m_bLastViewportChanged;

	// FilePath to the .fcn, if this UI::Movie has one.
	FilePath m_FilePath;

	// Used to track when an input event has been handled.
	Atomic32Value<Bool> m_bEventHandled;

	// Type name of this UI::Movie instance within its state machine.
	HString m_MovieTypeName;

	// Used to track calls to ConstructMovie()/DestroyMovie().
	Bool m_bConstructed;

	// /End UI::Manager friends

	Bool m_bPaused;
	Bool m_bBlockInputUntilRendering;
	Bool m_bFlushDeferredDraw;
	Bool m_bBlocksRenderBelow;
	Bool m_bAllowInputToScreensBelow;
	Bool m_bContinueInputOnPassthrough;
	HString m_PassthroughInputTrigger;
	HString m_PassthroughInputFunction;

	Bool m_bAcceptInput;

	// If true, this movie's root node is affected by screen shake.
	Bool m_bScreenShake;

	// Deferred called of OnLoad() (on first update).
	Bool m_bOnLoadCall{};

	SEOUL_DISABLE_COPY(Movie);
}; // class UI::Movie

} // namespace UI
SEOUL_REFLECTION_NO_DEFAULT_COPY(UI::Movie);

} // namespace Seoul

#endif // include guard
