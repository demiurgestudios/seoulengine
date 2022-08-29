/**
 * \file UIState.cpp
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

#include "ContentLoadManager.h"
#include "Logger.h"
#include "ReflectionEnum.h"
#include "ReflectionRegistry.h"
#include "ReflectionType.h"
#include "ReflectionUtil.h"
#include "RenderDevice.h"
#include "UIManager.h"
#include "UIMovie.h"
#include "UIRenderer.h"
#include "UIState.h"
#include "UIUtil.h"

namespace Seoul::UI
{

State::State(StateMachine<State>& rOwner, HString stateIdentifier)
	: m_rOwner(rOwner)
	, m_pMovieStackHead(nullptr)
	, m_pMovieStackTail(nullptr)
	, m_StateIdentifier(stateIdentifier)
	, m_bSuppressOcclusionOptimizer(false)
{
}

State::~State()
{
	// End occlusion optimizer suppress on destruction,
	// if we started it.
	if (m_bSuppressOcclusionOptimizer)
	{
		Manager::Get()->GetRenderer().EndOcclusionOptimizerSuppress();
	}

	CheckedPtr<Movie> pTail = m_pMovieStackTail;
	m_pMovieStackTail = nullptr;
	m_pMovieStackHead = nullptr;

	for (CheckedPtr<Movie> p = pTail; p.IsValid(); )
	{
		CheckedPtr<Movie> pToDelete = p;
		p = p->m_pPrev;
		pToDelete->m_pNext.Reset();
		pToDelete->m_pPrev.Reset();

		if (p.IsValid())
		{
			p->m_pNext.Reset();
		}

		// Let the UI::Manager do the rest.
		Manager::Get()->DestroyMovie(pToDelete);
	}
}

/** Get the configuration node associated with this state. */
Bool State::GetConfiguration(DataStore const*& rpDataStore, DataNode& rDataNode) const
{
	const DataStore& dataStore = m_rOwner.GetStateMachineConfiguration();

	// Failure if can't find entry in machine state.	
	DataNode dataNode;
	if (!dataStore.GetValueFromTable(dataStore.GetRootNode(), m_StateIdentifier, dataNode))
	{
		return false;
	}

	// Done, success.
	rpDataStore = &dataStore;
	rDataNode = dataNode;
	return true;
}

Bool State::EnterState(CheckedPtr<State> pPreviousState)
{
	// EnterState() is only ever called once by StateManager - other usages
	// are undefined.
	SEOUL_ASSERT(!m_pMovieStackHead.IsValid());
	SEOUL_ASSERT(!m_pMovieStackTail.IsValid());
	// Required by several bits of handling.
	SEOUL_ASSERT(pPreviousState != this);

	// Get the data store describing the state.
	DataStore& rDataStore = m_rOwner.GetStateMachineConfiguration();

	DataNode stateConfiguration;
	if (!rDataStore.GetValueFromTable(rDataStore.GetRootNode(), m_StateIdentifier, stateConfiguration))
	{
		SEOUL_LOG_UI("Failed transitioning to UI state %s, could not acquire state configuration.\n",
			m_StateIdentifier.CStr());
		return false;
	}

	// Check and record if we're suppressing occlusion optimization. Note
	// that while EnterState() will never be called twice, we protect
	// against it here in case it ever does happen, since once we
	// must only call BeginOcclusionOptimizerSuppress() once and we must
	// only call EndOcclusionOptimizerSuppress() once.
	if (!m_bSuppressOcclusionOptimizer)
	{
		DataNode value;
		if (rDataStore.GetValueFromTable(stateConfiguration, FalconConstants::kSuppressOcclusionOptimizer, value))
		{
			(void)rDataStore.AsBoolean(value, m_bSuppressOcclusionOptimizer);
		}

		if (m_bSuppressOcclusionOptimizer)
		{
			Manager::Get()->GetRenderer().BeginOcclusionOptimizerSuppress();
		}
	}

	DataNode moviesArray;
	(void)rDataStore.GetValueFromTable(stateConfiguration, FalconConstants::kMoviesTableKey, moviesArray);
	if (!moviesArray.IsArray())
	{
		// Allow empty movies array, so we can have "null" states with no Falcons being rendered.
		if (moviesArray.IsNull())
		{
			return true;
		}

		SEOUL_LOG_UI("Failed transitioning to UI state %s, Movies= entry is a %s, not an array.\n",
			m_StateIdentifier.CStr(),
			EnumToString<DataNode::Type>(moviesArray.GetType()));
		return false;
	}

	// Now, populate this UI::State's stack with an instance of each movie type listed in the array.
	UInt32 nMovies = 0u;
	(void)rDataStore.GetArrayCount(moviesArray, nMovies);

	// TODO: Eliminate heap allocation here.
	Vector<HString, MemoryBudgets::UIRuntime> vPersistentMovies;

	// TODO: Need to check for and handle the case that the same movie type is listed twice - this
	// will not break anything, but the behavior will likely be unexpected (instead of getting 2 instances
	// of the same type, only the instance on the bottom of the stack will exist).

	for (UInt32 i = 0u; i < nMovies; ++i)
	{
		DataNode value;
		HString movieTypeName;
		if (!rDataStore.GetValueFromArray(moviesArray, i, value))
		{
			SEOUL_LOG_UI("When transitioning to UI state %s, failed getting movie entry %u in the Movies= array.\n",
				m_StateIdentifier.CStr(),
				i);
			continue;
		}

		{
			Byte const* s = nullptr;
			UInt32 u = 0u;
			if (!rDataStore.AsString(value, s, u))
			{
				SEOUL_LOG_UI("When transitioning to UI state %s, movie entry %u is not an identifier, it is %s.\n",
					m_StateIdentifier.CStr(),
					i,
					EnumToString<DataNode::Type>(value.GetType()));
				continue;
			}

			movieTypeName = HString(s, u);
		}

		// Attempt to get the movie from the previous state.
		CheckedPtr<Movie> pMovie = (pPreviousState.IsValid() ? pPreviousState->FindMovieByTypeName(movieTypeName) : nullptr);

		// This variable is true if we're reusing the movie from the previous state.
		Bool const bPersistentMovie = (pMovie.IsValid());

		// If not a persistent movie, instantiate it.
		if (!bPersistentMovie)
		{
			// Get a Movie instance from the manager.
			// Try to use already instantiated movies via UIManager.
			pMovie = Manager::Get()->InstantiateMovie(movieTypeName);
		}

		if (!pMovie.IsValid())
		{
			SEOUL_WARN("When transitioning to UI state %s, movie entry %u could not be instantiated, typename %s.\n",
				m_StateIdentifier.CStr(),
				i,
				movieTypeName.CStr());
			continue;
		}

		// Tag the movie as persistent or not - movies that are not persistent, insert
		// the type name into vPersistentMovies, otherwise insert an empty entry.
		vPersistentMovies.PushBack(bPersistentMovie
			? HString()
			: movieTypeName);

		// Remove the movie from its existing list, if it's in one.
		if (pPreviousState)
		{
			if (pPreviousState->m_pMovieStackHead == pMovie)
			{
				pPreviousState->m_pMovieStackHead = pMovie->m_pNext;
			}

			if (pPreviousState->m_pMovieStackTail == pMovie)
			{
				pPreviousState->m_pMovieStackTail = pMovie->m_pPrev;
			}
		}

		if (pMovie->m_pNext)
		{
			pMovie->m_pNext->m_pPrev = pMovie->m_pPrev;
		}

		if (pMovie->m_pPrev)
		{
			pMovie->m_pPrev->m_pNext = pMovie->m_pNext;
		}

		// Insert the movie into our list.
		pMovie->m_pNext = nullptr;
		pMovie->m_pPrev = m_pMovieStackTail;
		if (m_pMovieStackTail.IsValid())
		{
			m_pMovieStackTail->m_pNext = pMovie;
		}

		if (!m_pMovieStackHead.IsValid())
		{
			m_pMovieStackHead = pMovie;
		}

		m_pMovieStackTail = pMovie;

		// Call OnExitState() if a movie is persistent, since it will not be called
		// from within rPreviousState's OnExitState() handler (the movie has been
		// removed from rPreviousState's list).
		if (bPersistentMovie)
		{
			SEOUL_PROF_VAR(pMovie->m_ProfOnExitState);

			// TODO: This is a hack - we've introduced dependencies in per-movie
			// logic that can be render dependent (specifically, screen resolution, viewport
			// clamping, and the mapping between UI world space to viewport space).
			//
			// Generally need to fix this. Likely solution is to promote all values to be
			// stored in the movie and hide the UI's renderer from public access.
			pMovie->SetMovieRendererDependentState();

			pMovie->OnExitState(pPreviousState, this, true);
		}
	}

	// Now complete setup of the movies in phases:
	// - first phase, call OnEnterState() on all movies.
	// - second phase, Apply OnEnter() conditions.

	// Phase 1, call OnEnterState().
	UInt32 uMovie = 0u;
	for (CheckedPtr<Movie> pMovie = m_pMovieStackHead; pMovie.IsValid(); pMovie = pMovie->m_pNext)
	{
		SEOUL_PROF_VAR(pMovie->m_ProfOnEnterState);

		HString const movieTypeName = vPersistentMovies[uMovie++];

		// TODO: This is a hack - we've introduced dependencies in per-movie
		// logic that can be render dependent (specifically, screen resolution, viewport
		// clamping, and the mapping between UI world space to viewport space).
		//
		// Generally need to fix this. Likely solution is to promote all values to be
		// stored in the movie and hide the UI's renderer from public access.
		pMovie->SetMovieRendererDependentState();

		// If movieTypeName is empty, then the movie is persistent.
		pMovie->OnEnterState(pPreviousState, this, movieTypeName.IsEmpty());
	}

	// Phase 2, apply all OnEnterState() conditions.
	ApplyStateTransitionConditions(rDataStore, stateConfiguration, true);

	// All phases done.
	return true;
}

void State::ExitState(CheckedPtr<State> pNextState)
{
	// Required by several bits of handling.
	SEOUL_ASSERT(pNextState != this);

	for (CheckedPtr<Movie> p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		// Movies still owned when a state's ExitState() is called are always
		// being destroyed - otherwise, they'd already be owned by the next state.
		p->OnExitState(this, pNextState, false);
	}

	// Apply OnExit conditions
	DataStore& rDataStore = m_rOwner.GetStateMachineConfiguration();
	DataNode stateConfiguration;
	if (rDataStore.GetValueFromTable(rDataStore.GetRootNode(), m_StateIdentifier, stateConfiguration))
	{
		ApplyStateTransitionConditions(rDataStore, stateConfiguration, false);
	}
}

void State::TransitionComplete()
{
	// Run on load on any movies that haven't received that call yet
	// (persistent movies will already have this method called).
	for (CheckedPtr<Movie> p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		if (!p->m_bOnLoadCall)
		{
			p->m_bOnLoadCall = true;

			SEOUL_PROF_VAR(p->m_ProfOnLoad);

			// TODO: This is a hack - we've introduced dependencies in per-movie
			// logic that can be render dependent (specifically, screen resolution, viewport
			// clamping, and the mapping between UI world space to viewport space).
			//
			// Generally need to fix this. Likely solution is to promote all values to be
			// stored in the movie and hide the UI's renderer from public access.
			p->SetMovieRendererDependentState();

			// Invoke.
			p->OnLoad();
		}
	}
}

void State::ApplyStateTransitionConditions(DataStore& rDataStore, DataNode& stateConfiguration, Bool bEnteringState)
{
	// Check the list of conditions to set when entering/exiting this state and set them to the given values
	DataNode conditionsTable;
	(void)rDataStore.GetValueFromTable(
		stateConfiguration,
		bEnteringState
			? FalconConstants::kOnEnterConditionsTableKey
			: FalconConstants::kOnExitConditionsTableKey,
		conditionsTable);

	if (conditionsTable.IsTable())
	{
		auto const iBegin = rDataStore.TableBegin(conditionsTable);
		auto const iEnd = rDataStore.TableEnd(conditionsTable);
		for (auto iter = iBegin; iEnd != iter; ++iter)
		{
			HString sConditionName = iter->First;
			DataNode conditionValue = iter->Second;
			Bool bValue = false;
			if (!rDataStore.AsBoolean(conditionValue, bValue))
			{
				SEOUL_LOG_UI("When transitioning %s UI state %s, failed parsing condition %s in the On%sConditions= table.\n",
					bEnteringState ? "to" : "from",
					m_StateIdentifier.CStr(),
					sConditionName.CStr(),
					bEnteringState ? "Enter" : "Exit");
				continue;
			}

			Manager::Get()->SetCondition(sConditionName, bValue);
		}
	}
}

// Called by UI::Manager once per frame.
void State::PrePose(RenderPass& rPass, Float fDeltaTimeInSeconds)
{
	for (auto p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		SEOUL_PROF_VAR(p->m_ProfPrePose);

		if (p->IsPaused())
		{
			p->PrePoseWhenBlocked(rPass, fDeltaTimeInSeconds);
		}
		else
		{
			p->PrePose(rPass, fDeltaTimeInSeconds);
		}
	}
}

void State::PrePoseWhenBlocked(RenderPass& rPass, Float fDeltaTimeInSeconds)
{
	for (CheckedPtr<Movie> p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		SEOUL_PROF_VAR(p->m_ProfPrePose);

		p->PrePoseWhenBlocked(rPass, fDeltaTimeInSeconds);
	}
}

// Called by UI::Manager once per frame
void State::Advance(Float fDeltaTimeInSeconds)
{
	for (auto p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		SEOUL_PROF_VAR(p->m_ProfAdvance);

		if (p->IsPaused())
		{
			p->AdvanceWhenBlocked(fDeltaTimeInSeconds);
		}
		else
		{
			p->Advance(fDeltaTimeInSeconds);
		}
	}
}

// Called by UI::Manager once per frame.
void State::Pose(RenderPass& rPass, Renderer& rRenderer)
{
	// Find the bottom movie to render
	CheckedPtr<Movie> pBottom = m_pMovieStackTail;
	for (CheckedPtr<Movie> p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		if (p->BlocksRenderBelow())
		{
			pBottom = p;
			break;
		}
	}

	// Pose the movies.
	for (CheckedPtr<Movie> p = pBottom; p.IsValid(); p = p->m_pPrev)
	{
		SEOUL_PROF_VAR(p->m_ProfPose);

		p->OnPose(rPass, rRenderer);
	}
}

#if SEOUL_HOT_LOADING
void State::HotLoadBegin()
{
	for (auto p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		p->OnHotLoadBegin();
	}
}

void State::HotLoadEnd()
{
	for (auto p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		p->OnHotLoadEnd();
	}
}
#endif

#if SEOUL_ENABLE_CHEATS
// Called by UI::Manager once per frame when the developer only input visualization mode is enabled.
Bool State::PoseInputVisualization(
	const InputWhitelist& inputWhitelist,
	UInt8 uInputMask,
	RenderPass& rPass,
	Renderer& rRenderer)
{
	// For input visualization, render moves top down. Stop if a movie blocks input below it.
	for (auto p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		// Blocking input, stop.
		if (!p->AcceptingInput())
		{
			// Return true to indicate this condition.
			return true;
		}

		p->OnPoseInputVisualization(
			inputWhitelist,
			uInputMask,
			rPass,
			rRenderer);

		// This movie blocks input, so stop.
		if (!p->AllowInputToScreensBelow())
		{
			// Return true to indicate this condition.
			return true;
		}
	}

	return false;
}
#endif // /#if SEOUL_ENABLE_CHEATS

// Returns true if any of the movies in this state block input below them
Bool State::BlocksRenderBelow()
{
	for (CheckedPtr<Movie> p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		if (p->BlocksRenderBelow())
		{
			return true;
		}
	}

	return false;
}

// Delivered to all movies and all states as a form of "global" mouse event
// messaging.
Bool State::OnGlobalMouseButtonPressed(
	const Point2DInt& pos,
	const SharedPtr<Falcon::MovieClipInstance>& pInstance)
{
	for (CheckedPtr<Movie> p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		if (!p->AcceptingInput()) { return true; }
		p->OnGlobalMouseButtonPressed(pos, pInstance);
		if (!p->AllowInputToScreensBelow()) { return true; }
	}

	return false;
}

Bool State::OnGlobalMouseButtonReleased(const Point2DInt& pos)
{
	for (CheckedPtr<Movie> p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		if (!p->AcceptingInput()) { return true; }
		p->OnGlobalMouseButtonReleased(pos);
		if (!p->AllowInputToScreensBelow()) { return true; }
	}

	return false;
}

/**
 * Developer only utility. Return a list of points that can be potentially
 * hit based on the input test max. This applies to all state machines and all
 * movies currently active.
 */
Bool State::GetHitPoints(
	HString stateMachine,
	UInt8 uInputMask,
	HitPoints& rvHitPoints) const
{
	for (CheckedPtr<Movie> p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		if (p->GetHitPoints(
			stateMachine,
			GetStateIdentifier(),
			uInputMask,
			rvHitPoints))
		{
			return true;
		}
	}

	return false;
}

MovieHitTestResult State::HitTest(
	UInt8 uMask,
	const Point2DInt& mousePosition,
	Movie*& rpHitMovie,
	SharedPtr<Falcon::MovieClipInstance>& rpHitInstance,
	SharedPtr<Falcon::Instance>& rpLeafInstance,
	Vector<Movie*>* rvPassthroughInputs) const
{
	for (CheckedPtr<Movie> p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		MovieHitTestResult const eResult = p->OnHitTest(
			uMask,
			mousePosition,
			rpHitMovie,
			rpHitInstance,
			rpLeafInstance,
			rvPassthroughInputs);

		if (MovieHitTestResult::kHit == eResult ||
			MovieHitTestResult::kNoHitStopTesting == eResult ||
			MovieHitTestResult::kNoHitTriggerBack == eResult)
		{
			return eResult;
		}
		// Otherwise, keep testing.
	}

	return MovieHitTestResult::kNoHit;
}

void State::Pick(
	const Point2DInt& mousePosition,
	Vector<PickEntry, MemoryBudgets::UIRuntime>& rv) const
{
	Vector<SharedPtr<Falcon::Instance>, MemoryBudgets::UIRuntime> v;
	for (CheckedPtr<Movie> p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		v.Clear();
		p->OnPick(mousePosition, v);
		if (!v.IsEmpty())
		{
			// List comes back in reverse from expected,
			// so traverse it now in reverse.
			for (Int32 i = (Int32)v.GetSize() - 1; i >= 0; --i)
			{
				auto const& pInstance = v[i];

				PickEntry e;
				e.m_pHitInstance = pInstance;
				e.m_pHitMovie = p;
				rv.PushBack(e);
			}
		}

		if (p->BlocksRenderBelow())
		{
			break;
		}
	}
}

/**
 * Called by UI::Manager() when UI::Manager::BroadcastEvent() is called
 */
MovieHitTestResult State::SendInputEvent(InputEvent eInputEvent)
{
	for (CheckedPtr<Movie> p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		MovieHitTestResult const eResult = p->OnSendInputEvent(eInputEvent);

		if (MovieHitTestResult::kHit == eResult ||
			MovieHitTestResult::kNoHitStopTesting == eResult)
		{
			return eResult;
		}
		// Otherwise, keep testing.
	}

	return MovieHitTestResult::kNoHit;
}

/**
* Called by UI::Manager() when UI::Manager::BroadcastEvent() is called
*/
MovieHitTestResult State::SendButtonEvent(
	Seoul::InputButton eButtonID,
	Seoul::ButtonEventType eButtonEventType)
{
	for (CheckedPtr<Movie> p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		MovieHitTestResult const eResult = p->OnSendButtonEvent(eButtonID, eButtonEventType);

		if (MovieHitTestResult::kHit == eResult ||
			MovieHitTestResult::kNoHitStopTesting == eResult)
		{
			return eResult;
		}
		// Otherwise, keep testing.
	}

	return MovieHitTestResult::kNoHit;
}

/**
 * Called by UI::Manager() when UI::Manager::BroadcastEvent() is called
 */
Bool State::OnBroadcastEvent(
	HString sTargetType,
	HString sEvent,
	const Reflection::MethodArguments& aArguments,
	Int nArgumentCount)
{
	using namespace Reflection;

	Bool bReturn = false;

	// If sTargetType is empty, send the event to all movies.
	if (sTargetType.IsEmpty())
	{
		// Iterate over all Movies in this State.
		for (CheckedPtr<Movie> p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
		{
			bReturn = p->OnTryBroadcastEvent(sEvent, aArguments, nArgumentCount) || bReturn;
		}
	}
	// Otherwise, only target a movie with the same type name.
	else
	{
		// Iterate over all Movies in this State.
		for (CheckedPtr<Movie> p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
		{
			// If p has the same type name, broadcast it.
			if (p->GetMovieTypeName() == sTargetType)
			{
				bReturn = p->OnTryBroadcastEvent(sEvent, aArguments, nArgumentCount) || bReturn;

				// We can break now - never more than one movie in a state
				// with the same type name.
				break;
			}
		}
	}

	return bReturn;
}

/**
 * @return A non-null UI::Movie instance owned by this UI::State with typename movieTypeName.
 */
CheckedPtr<Movie> State::FindMovieByTypeName(HString movieTypeName) const
{
	for (CheckedPtr<Movie> p = m_pMovieStackHead; p.IsValid(); p = p->m_pNext)
	{
		if (p->GetMovieTypeName() == movieTypeName)
		{
			return p;
		}
	}

	return nullptr;
}

} // namespace Seoul::UI
