/**
 * \file UIState.h
 * \brief State node inserted into the StateMachine<> that is
 * used by UI::Manager to define layers of UI stacks. Manages
 * UI::Movie subclasses.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_STATE_H
#define UI_STATE_H

#include "InputManager.h"
#include "List.h"
#include "Prereqs.h"
#include "StateMachine.h"
#include "UIData.h"
namespace Seoul { class RenderPass; }
namespace Seoul { namespace Falcon { class MovieClipInstance; } }

namespace Seoul
{

// Forward declarations
class RenderSurface2D;
struct Viewport;

namespace UI
{

struct HitPoint;
class Movie;
class Renderer;

/**
 * An UI::State contains one or more UI::Movies, in a stack. The
 * UI::Manager likewise contains one or more StateMachine<UIState> instances
 * in a stack. Together, this creates the hierarchy of Flash movies that are
 * rendered, ticked, and receive input.
 */
class State SEOUL_SEALED
{
public:
	State(StateMachine<State>& rOwner, HString stateIdentifier);
	~State();

	// Get the configuration node associated with this state.
	Bool GetConfiguration(DataStore const*& rpDataStore, DataNode& rDataNode) const;

	// Returns the name of the state
	HString GetStateIdentifier() const
	{
		return m_StateIdentifier;
	}

	// Returns the head of the movie stack for this state
	const CheckedPtr<Movie> GetMovieStackHead() const
	{
		return m_pMovieStackHead;
	}

	HString GetStateMachineName() const
	{
		return m_rOwner.GetName();
	}

private:
	// Begin StateMachine friend functions
	friend class StateMachine<State>;

	// Invoked by StateMachine<> when this State has been
	// entered from pPreviousState, or nullptr if no previous
	// state.
	Bool EnterState(CheckedPtr<State> pPreviousState);

	// Invoked by StateMachine<> when this State is outgoing
	// to pNextState, or nullptr if no next
	// state.
	void ExitState(CheckedPtr<State> pNextState);

	// Invoked by StateMachine<> when this State is incoming
	// and transition is fully complete (any previous state has
	// been destroyed).
	void TransitionComplete();
	// /End StateMachine friend functions

	void ApplyStateTransitionConditions(DataStore& rDataStore, DataNode& stateConfiguration, Bool bEnteringState);

	// Begin UI::Manager/UIStack friend functions
	friend class Manager;
	friend class Stack;

	// Called by UI::Manager once per frame (on the main thread).
	void PrePose(RenderPass& rPass, Float fDeltaTimeInSeconds);

	// Called by UI::Manager once per frame (on the main thread),
	// when a state above this state is blocking rendering and updates
	// (e.g. waiting for FCN files to load).
	void PrePoseWhenBlocked(RenderPass& rPass, Float fDeltaTimeInSeconds);

	// Called by UI::Manager once per frame unless a screen above it
	// has blocked updates (e.g. waiting for FCN files to load).
	void Advance(Float fDeltaTimeInSeconds);

	// Always called once per frame even when blocked (either by UI::Manager or Advance)
	void AdvanceWhenBlocked(Float fDeltaTimeInSeconds);

	// Called by UI::Manager once per frame.
	void Pose(RenderPass& rPass, Renderer& rRenderer);

#if SEOUL_HOT_LOADING
	void HotLoadBegin();
	void HotLoadEnd();
#endif

#if SEOUL_ENABLE_CHEATS
	// Called by UI::Manager once per frame when the developer only input visualization mode is enabled.
	typedef HashSet< SharedPtr<Falcon::MovieClipInstance>, MemoryBudgets::UIData > InputWhitelist;
	Bool PoseInputVisualization(
		const InputWhitelist& inputWhitelist,
		UInt8 uInputMask,
		RenderPass& rPass,
		Renderer& rRenderer);
#endif // /#if SEOUL_ENABLE_CHEATS

	// Returns true if any of the movies in this state block render below them.
	Bool BlocksRenderBelow();

	// Delivered to all movies and all states as a form of "global" mouse event
	// messaging. Hit instance is the instance hit on mouse down, or null if
	// no hit occurred.
	Bool OnGlobalMouseButtonPressed(
		const Point2DInt& pos,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance);
	Bool OnGlobalMouseButtonReleased(const Point2DInt& pos);

public:
	// Developer only utility. Return a list of points that can be potentially
	// hit based on the input test max. This applies to all state machines and all
	// movies currently active.
	typedef Vector<HitPoint, MemoryBudgets::UIRuntime> HitPoints;
	Bool GetHitPoints(
		HString stateMachine,
		UInt8 uInputMask,
		HitPoints& rvHitPoints) const;

	// Populate with the movie and sprite instance of a point hit test.
	MovieHitTestResult HitTest(
		UInt8 uMask,
		const Point2DInt& mousePosition,
		Movie*& rpHitMovie,
		SharedPtr<Falcon::MovieClipInstance>& rpHitInstance,
		SharedPtr<Falcon::Instance>& rpLeafInstance,
		Vector<Movie*>* rvPassthroughInputs) const;

	struct PickEntry SEOUL_SEALED
	{
		CheckedPtr<Movie> m_pHitMovie;
		SharedPtr<Falcon::Instance> m_pHitInstance;
	}; // struct PickEntry

	void Pick(
		const Point2DInt& mousePosition,
		Vector<PickEntry, MemoryBudgets::UIRuntime>& rv) const;

private:

	// Called by UI::Manager() to dispatch special input events.
	MovieHitTestResult SendInputEvent(InputEvent eInputEvent);

	// Called by UI::Manager() to dispatch special input events.
	MovieHitTestResult SendButtonEvent(
		Seoul::InputButton eButtonID,
		Seoul::ButtonEventType eButtonEventType);

	// /End UI::Manager

	// Called by UI::Manager() when UI::Manager::BroadcastEvent() is called
	Bool OnBroadcastEvent(
		HString sTargetType,
		HString sEvent,
		const Reflection::MethodArguments& aArguments,
		Int nArgumentCount);
	// /End UI::Manager friend functions

	StateMachine<State>& m_rOwner;
	CheckedPtr<Movie> m_pMovieStackHead;
	CheckedPtr<Movie> m_pMovieStackTail;
	HString m_StateIdentifier;
	Bool m_bSuppressOcclusionOptimizer;

	CheckedPtr<Movie> FindMovieByTypeName(HString movieTypeName) const;

	SEOUL_DISABLE_COPY(State);
}; // class UI::State;

} // namespace UI

template <>
struct StateTraits<UI::State>
{
	/**
	 * Specialization of StateTraits<UIState>, returns
	 * a new UIState.
	 */
	inline static UI::State* NewState(StateMachine<UI::State>& rOwner, HString stateIdentifier)
	{
		return SEOUL_NEW(MemoryBudgets::UIRuntime) UI::State(rOwner, stateIdentifier);
	}
};

} // namespace Seoul

#endif // include guard
