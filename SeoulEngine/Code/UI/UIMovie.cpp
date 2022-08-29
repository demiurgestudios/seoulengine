/**
 * \file UIMovie.cpp
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

#include "Camera.h"
#include "Engine.h"
#include "FalconHitTester.h"
#include "FalconMovieClipInstance.h"
#include "FxManager.h"
#include "LocManager.h"
#include "Logger.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ReflectionRegistry.h"
#include "RenderDevice.h"
#include "SettingsManager.h"
#include "UIAdvanceInterfaceDeferredDispatch.h"
#include "UIAnimation2DNetworkInstance.h"
#include "UIFxInstance.h"
#include "UIManager.h"
#include "UIMovie.h"
#include "UIMovieInternal.h"
#include "UIRenderer.h"
#include "UIUtil.h"

namespace Seoul
{

// Tolerance used to avoid error build up in the accumulation buffer
// and to allow a bit of undershoot. Currently set to 0.5 ms pending
// further testing and refinement.
static const Float kfAccumulationSlopInSeconds = (Float)(0.5f / 1000.0);

// Target frame time - we apply this to tween and movie advancement.
// MovieInternal handles its own time bucketing at the movie frame
// rate.
static const Float kfFixedFrameTimeInSeconds = (1.0f / 60.0f);

// Placeholder "instance" id for a pass through hit point.
static const HString kPassthroughId("passthrough");

SEOUL_BEGIN_TYPE(UI::Movie, TypeFlags::kDisableNew)
	SEOUL_METHOD(AcceptingInput)
	SEOUL_METHOD(GetMovieTypeName)
	SEOUL_METHOD(HasActiveFx)
	SEOUL_METHOD(IsTop)
	SEOUL_METHOD(SetAcceptInput)
	SEOUL_METHOD(SetAllowInputToScreensBelow)
	SEOUL_METHOD(SetPaused)
	SEOUL_METHOD(SetTrackedSoundEventParameter)
	SEOUL_METHOD(StartSoundEvent)
	SEOUL_METHOD(StartSoundEventWithOptions)
	SEOUL_METHOD(StartTrackedSoundEvent)
	SEOUL_METHOD(StartTrackedSoundEventWithOptions)
	SEOUL_METHOD(StopTrackedSoundEvent)
	SEOUL_METHOD(StopTrackedSoundEventImmediately)
	SEOUL_METHOD(TriggerTrackedSoundEventCue)
	SEOUL_METHOD(GetMovieHeight)
	SEOUL_METHOD(GetMovieWidth)

	SEOUL_PROPERTY_N("BlockInputUntilRendering", m_bBlockInputUntilRendering)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("FlushDeferredDraw", m_bFlushDeferredDraw)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("BlocksRenderBelow", m_bBlocksRenderBelow)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("AllowInputToScreensBelow", m_bAllowInputToScreensBelow)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("ContinueInputOnPassthrough", m_bContinueInputOnPassthrough)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("PassthroughInputTrigger", m_PassthroughInputTrigger)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("PassthroughInputFunction", m_PassthroughInputFunction)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("ScreenShake", m_bScreenShake)
		SEOUL_ATTRIBUTE(NotRequired)

	SEOUL_METHOD(StateMachineRespectsInputWhitelist)

SEOUL_END_TYPE()

namespace UI
{

/** Recursive developer utility, return rectangles in viewport space of potentially hit testable scene nodes. */
static inline Bool LeafGetUIHitPoints(
	const Movie& movie,
	Falcon::HitTester& tester,
	HString stateMachine,
	HString state,
	const SharedPtr<Falcon::MovieClipInstance>& pRoot,
	const Falcon::Rectangle& viewportRectangle,
	const SharedPtr<Falcon::MovieClipInstance>& pNode,
	const Matrix2x3& mWorld,
	const SharedPtr<Falcon::Instance>& pChild,
	UInt8 uInputMask,
	State::HitPoints& rvHitPoints)
{
	Falcon::Rectangle rectangle;
	if (!pChild->ComputeBounds(rectangle))
	{
		return false;
	}

	rectangle = Falcon::TransformRectangle(mWorld, rectangle);

	// Clip the hit rectangle to viewportRectangle and if fully clipped,
	// return false.
	rectangle.m_fLeft = Max(rectangle.m_fLeft, viewportRectangle.m_fLeft);
	rectangle.m_fRight = Min(rectangle.m_fRight, viewportRectangle.m_fRight);
	rectangle.m_fTop = Max(rectangle.m_fTop, viewportRectangle.m_fTop);
	rectangle.m_fBottom = Min(rectangle.m_fBottom, viewportRectangle.m_fBottom);
	if (rectangle.GetWidth() <= 0.0f || rectangle.GetHeight() <= 0.0f)
	{
		return false;
	}

	Vector2D const vCenter = rectangle.GetCenter();
	Vector2D const vWorld = tester.DepthProject(vCenter);
	Vector2D vTapPoint;

	Falcon::HitTestResult eResult = Falcon::HitTestResult::kNoHit;
	SharedPtr<Falcon::MovieClipInstance> pInstance;
	SharedPtr<Falcon::Instance> pLeaf;

	// Try up to four random points in the shape trying to find something clickable
	for (UInt uTryCount = 0; uTryCount < 4; ++uTryCount)
	{
		vTapPoint = tester.DepthProject(Vector2D(
			rectangle.m_fLeft + GlobalRandom::UniformRandomFloat32() * rectangle.GetWidth(),
			rectangle.m_fTop + GlobalRandom::UniformRandomFloat32() * rectangle.GetHeight()));

		// Test against the root - while doing so, we need to reset
		// the hit tester.
		auto const e = tester.ReplaceDepth3D(0.0f, 0);
		eResult = pRoot->HitTest(
			tester,
			uInputMask,
			vTapPoint.X,
			vTapPoint.Y,
			pInstance,
			pLeaf);
		tester.ReplaceDepth3D(e.First, e.Second);

		if (Falcon::HitTestResult::kHit == eResult &&
			pLeaf == pChild &&
			pInstance == pNode)
		{
			break;
		}
	}

	// If we haven't found a hit randomly, try the center as a last resort
	if (Falcon::HitTestResult::kHit != eResult)
	{
		vTapPoint = vWorld;

		// Test against the root - while doing so, we need to reset
		// the hit tester.
		auto const e = tester.ReplaceDepth3D(0.0f, 0);
		eResult = pRoot->HitTest(
			tester,
			uInputMask,
			vTapPoint.X,
			vTapPoint.Y,
			pInstance,
			pLeaf);
		tester.ReplaceDepth3D(e.First, e.Second);
	}

	// If we hit this child from the root, include it.
	if (Falcon::HitTestResult::kHit == eResult &&
		pLeaf == pChild &&
		pInstance == pNode)
	{
		HitPoint point;
		point.m_pInstance = pNode;
		point.m_Class = pNode->GetMovieClipDefinition()->GetClassName();
		point.m_Id = pNode->GetName();
		point.m_vTapPoint = movie.GetMousePositionFromWorld(vTapPoint);
		point.m_vCenterPoint = movie.GetMousePositionFromWorld(vWorld);
		point.m_Movie = movie.GetMovieTypeName();
		point.m_DevOnlyInternalStateId = movie.GetDevOnlyInternalStateId();
		point.m_State = state;
		point.m_StateMachine = stateMachine;
		rvHitPoints.PushBack(point);
		return true;
	}

	return false;
}

/** Recursive developer utility, return rectangles in viewport space of potentially hit testable scene nodes. */
static inline void GetUIHitPoints(
	const Movie& movie,
	Falcon::HitTester& tester,
	HString stateMachine,
	HString state,
	const SharedPtr<Falcon::MovieClipInstance>& pRoot,
	const Falcon::Rectangle& viewportRectangle,
	const SharedPtr<Falcon::MovieClipInstance>& pNode,
	const Matrix2x3& mParent,
	UInt8 uInputMask,
	State::HitPoints& rvHitPoints)
{
	// Skip nodes which are not visible.
	if (!pNode->GetVisible())
	{
		return;
	}

	Matrix2x3 const mWorld = (mParent * pNode->GetTransform());
	tester.PushDepth3D(pNode->GetDepth3D(), pNode->GetIgnoreDepthProjection());
	if (0u != (pNode->GetHitTestSelfMask() & uInputMask) && !pNode->GetInputActionDisabled())
	{
		Int32 const iChildren = (Int32)pNode->GetChildCount();
		for (Int32 i = (iChildren - 1); i >= 0; --i)
		{
			SharedPtr<Falcon::Instance> pChild;
			if (pNode->GetChildAt(i, pChild))
			{
				if (pChild->GetType() != Falcon::InstanceType::kMovieClip)
				{
					if (LeafGetUIHitPoints(
						movie,
						tester,
						stateMachine,
						state,
						pRoot,
						viewportRectangle,
						pNode,
						mWorld,
						pChild,
						uInputMask,
						rvHitPoints))
					{
						break;
					}
				}
			}
		}
	}

	if (0u != (pNode->GetHitTestChildrenMask() & uInputMask))
	{
		Int32 const iChildren = (Int32)pNode->GetChildCount();
		for (Int32 i = (iChildren - 1); i >= 0; --i)
		{
			SharedPtr<Falcon::Instance> pChild;
			if (pNode->GetChildAt(i, pChild) &&
				pChild->GetType() == Falcon::InstanceType::kMovieClip)
			{
				SharedPtr<Falcon::MovieClipInstance> pt((Falcon::MovieClipInstance*)pChild.GetPtr());
				GetUIHitPoints(
					movie,
					tester,
					stateMachine,
					state,
					pRoot,
					viewportRectangle,
					pt,
					mWorld,
					uInputMask,
					rvHitPoints);
			}
		}
	}

	tester.PopDepth3D(pNode->GetDepth3D(), pNode->GetIgnoreDepthProjection());
}

Movie::Movie()
	: m_pInternal(nullptr)
	, m_hThis()
	, m_fAccumulatedScaledFrameTime(0.0f)
	, m_pOwner(nullptr)
	, m_pNext(nullptr)
	, m_pPrev(nullptr)
	, m_LastViewport()
	, m_bLastViewportChanged(false)
	, m_FilePath()
	, m_bEventHandled(false)
	, m_MovieTypeName()
	, m_bConstructed(false)
	, m_bPaused(false)
	, m_bBlockInputUntilRendering(true)
	, m_bFlushDeferredDraw(false)
	, m_bBlocksRenderBelow(false)
	, m_bAllowInputToScreensBelow(false)
	, m_bContinueInputOnPassthrough(false)
	, m_PassthroughInputTrigger()
	, m_PassthroughInputFunction()
	, m_bAcceptInput(true)
	, m_bScreenShake(false)
{
	// Allocate a handle for this.
	m_hThis = MovieHandleTable::Allocate(this);
}

Movie::~Movie()
{
	// Free our handle.
	MovieHandleTable::Free(m_hThis);

	// Sanity checks.
	SEOUL_ASSERT(!m_bConstructed);
	SEOUL_ASSERT(!m_pNext.IsValid());
	SEOUL_ASSERT(!m_pPrev.IsValid());
	SEOUL_ASSERT(!m_pInternal.IsValid());
}

/**
 * @return A "world space" position, used for FX, mapped
 * to the current UI::Movie, converting movie space
 * pixels into FX world space.
 */
Vector3D Movie::ToFxWorldPosition(
	Float fXIn,
	Float fYIn,
	Float fDepth3D /*= 0.0f*/) const
{
	SharedPtr<Falcon::FCNFile> pFile(m_pInternal->GetFCNFile());
	if (!pFile.IsValid())
	{
		return Vector3D::Zero();
	}

	Viewport const activeViewport = GetViewport();
	Float const fStageWidth = pFile->GetBounds().GetWidth();
	auto const vStageCoords = ComputeStageTopBottom(activeViewport, pFile->GetBounds().GetHeight());
	Float32 const fStageTopRenderCoord = vStageCoords.X;
	Float32 const fStageBottomRenderCoord = vStageCoords.Y;

	Float const fHalfWorldHeight = (Manager::Get()->ComputeUIRendererFxCameraWorldHeight(activeViewport) * 0.5f);

	Float const fHeight = Max(fStageBottomRenderCoord - fStageTopRenderCoord, 1.0f);
	Float const fRatio = fStageWidth / fHeight;

	Float const fX = ((fXIn / Max(fStageWidth, 1.0f)) * 2.0f - 1.0f) * fRatio * fHalfWorldHeight;
	Float const fY = -(((fYIn - fStageTopRenderCoord) / fHeight) * (2.0f * fHalfWorldHeight) - fHalfWorldHeight);

	return Vector3D(fX, fY, fDepth3D);
}

#if SEOUL_WITH_ANIMATION_2D
void Movie::AddActiveAnimation2D(Animation2DNetworkInstance* pNetworkInstance)
{
	m_ActiveAnimation2DInstances.Add(pNetworkInstance);
}

void Movie::RemoveActiveAnimation2D(Animation2DNetworkInstance* pNetworkInstance)
{
	m_ActiveAnimation2DInstances.Remove(pNetworkInstance);
}
#endif // /#if SEOUL_WITH_ANIMATION_2D

void Movie::AddActiveFx(FxInstance* pFxInstance)
{
	m_ActiveFxInstances.Add(pFxInstance);
}

void Movie::RemoveActiveFx(FxInstance* pFxInstance)
{
	m_ActiveFxInstances.Remove(pFxInstance);
}

void Movie::EnableTickEvents(Falcon::Instance* pInstance)
{
	m_TickEventTargets.Add(pInstance);
}

void Movie::DisableTickEvents(Falcon::Instance* pInstance)
{
	m_TickEventTargets.Remove(pInstance);
}

void Movie::EnableTickScaledEvents(Falcon::Instance* pInstance)
{
	m_TickScaledEventTargets.Add(pInstance);
}

void Movie::DisableTickScaledEvents(Falcon::Instance* pInstance)
{
	m_TickScaledEventTargets.Remove(pInstance);
}

Bool Movie::StateMachineRespectsInputWhitelist() const
{
	return Manager::Get()->MovieStateMachineRespectsInputWhiteList(this);
}

HString Movie::GetStateMachineName() const
{
	if (!m_pOwner.IsValid())
	{
		return HString();
	}

	return m_pOwner->GetStateMachineName();
}

/**
 * Convenience, get the duration of a factoried FX
 * based on its template id. Returns 0.0f if the
 * given FX id is invalid.
 */
Float Movie::GetFxDuration(HString id)
{
	Float f = 0.0f;
	if (m_Content.GetFx().GetFxDuration(id, f))
	{
		return f;
	}

	SEOUL_WARN("%s: No fx duration for '%s', check that fx was preloaded early enough.",
		GetMovieTypeName().CStr(),
		id.CStr());

	return f;
}

Bool Movie::HasActiveFx(Bool bIncludeLooping)
{
	ContainerLock<ActiveFxInstances> lock(m_ActiveFxInstances);

	// If including looping, look for the first reachable FX.
	if (bIncludeLooping)
	{
		for (auto const& p : lock)
		{
			if (IsReachableAndVisible(p.GetPtr()))
			{
				return true;
			}
		}
	}
	// Otherwise, iterate and find if any of the FX are
	// not looping. O(n).
	else
	{
		for (auto const& p : lock)
		{
			if (!p->GetProperties().m_bHasLoops && !p->GetTreatAsLooping())
			{
				if (IsReachableAndVisible(p.GetPtr()))
				{
					return true;
				}
			}
		}
	}

	return false;
}

/**
 * Utility for several instance types that are directly ticked
 * without graph traversal - verify that this instance
 * is visible and can be reached from the root
 * node of this UIMovie.
 */
Bool Movie::IsReachableAndVisible(Falcon::Instance const* pInstance) const
{
	// No root, not reachable.
	SharedPtr<Falcon::MovieClipInstance> pRoot;
	if (!GetRootMovieClip(pRoot))
	{
		return false;
	}

	// TODO: Eliminate the need for this graph traversal.
	//
	// Check visible - also, search for root.
	Bool bReachable = false;
	while (nullptr != pInstance)
	{
		// Not visible, false immediately.
		if (!pInstance->GetVisible())
		{
			return false;
		}

		// Root reachable.
		if (pInstance == pRoot.GetPtr())
		{
			bReachable = true;
		}

		// Advance.
		pInstance = pInstance->GetParent();
	}

	return bReachable;
}

Falcon::HitTester Movie::GetHitTester() const
{
	Falcon::Rectangle bounds;
	{
		SharedPtr<Falcon::FCNFile> pFile(m_pInternal->GetFCNFile());
		if (pFile.IsValid())
		{
			bounds = pFile->GetBounds();
		}
	}

	return Manager::Get()->GetRenderer().GetHitTester(
		*this,
		bounds,
		GetViewport());
}

/**
 * Attempt to kick off a sound event with Sound::Event
 * identifier soundEventId.
 */
void Movie::StartSoundEvent(HString soundEventId)
{
	if (!m_Content.GetSoundEvents().StartSoundEvent(soundEventId))
	{
		// This is always a configuration error due to the implementation
		// of StartSoundEvent(), so we bring attention to it with a warning.
		SEOUL_LOG_UI("%s: Failed triggering sound event %s, "
			"check that this event is properly configured in the "
			"UI config file.",
			GetMovieTypeName().CStr(),
			soundEventId.CStr());
	}
}

/**
 * Attempt to kick off a sound event with Sound::Event identifier soundEventId.
 * If bStopOnDestruction is true, the sound will be stopped when this
 * UI::Movie is destroyed, otherwise its tail will be allowed to play
 * to completion (looping sounds are always stopped on destruction).
 */
void Movie::StartSoundEventWithOptions(HString soundEventId, Bool bStopOnDestruction)
{
	if (!m_Content.GetSoundEvents().StartSoundEvent(
		soundEventId,
		Vector3D::Zero(),
		Vector3D::Zero(),
		bStopOnDestruction))
	{
		// This is always a configuration error due to the implementation
		// of StartSoundEvent(), so we bring attention to it with a warning.
		SEOUL_LOG_UI("%s: Failed triggering sound event %s, "
			"check that this event is properly configured in the "
			"UI config file.",
			GetMovieTypeName().CStr(),
			soundEventId.CStr());
	}
}

/**
 * Attempt to kick off a tracked sound event with Sound::Event identifier
 * soundEventId.
 */
Int32 Movie::StartTrackedSoundEvent(HString soundEventId)
{
	Int32 iReturn = -1;
	if (!m_Content.GetSoundEvents().StartTrackedSoundEvent(
		soundEventId,
		iReturn))
	{
		SEOUL_LOG_UI("Failed triggering sound event %s with id %d, "
			"check that this event is properly configured in the "
			"UI config file.",
			soundEventId.CStr(),
			iReturn);
		return -1;
	}

	return iReturn;
}

/**
 * Attempt to kick off a tracked sound event with Sound::Event identifier
 * soundEventId.
 *
 * If bStopOnDestruction is true, the sound will be stopped when this
 * UI::Movie is destroyed, otherwise its tail will be allowed to play
 * to completion (looping sounds are always stopped on destruction).
 */
Int32 Movie::StartTrackedSoundEventWithOptions(HString soundEventId, Bool bStopOnDestruction)
{
	Int32 iReturn = -1;
	if (!m_Content.GetSoundEvents().StartTrackedSoundEvent(
		soundEventId,
		iReturn,
		Vector3D::Zero(),
		Vector3D::Zero(),
		bStopOnDestruction))
	{
		SEOUL_LOG_UI("Failed triggering sound event %s with id %d, "
			"check that this event is properly configured in the "
			"UI config file.",
			soundEventId.CStr(),
			iReturn);
		return -1;
	}

	return iReturn;
}

/**
 * Attempt to stop tracked sound event with id iId.
 */
void Movie::StopTrackedSoundEvent(Int32 iId)
{
	if (!m_Content.GetSoundEvents().StopTrackedSoundEvent(iId))
	{
		SEOUL_LOG_UI("Failed stopping sound event with id %d, "
			"check that this event was started.",
			iId);
	}
}

/**
 * Attempt to stop tracked sound event with id iId, does
 * not play the event's tail (will stop instantaneously).
 */
void Movie::StopTrackedSoundEventImmediately(Int32 iId)
{
	if (!m_Content.GetSoundEvents().StopTrackedSoundEvent(
		iId,
		true))
	{
		SEOUL_LOG_UI("Failed stopping sound event with name %d, "
			"check that this event was started.",
			iId);
	}
}

/**
 * Attempt to update parameter parameterName to fValue in tracked sound event
 * with id iId.
 */
void Movie::SetTrackedSoundEventParameter(Int32 iId, HString parameterName, Float fValue)
{
	if (!m_Content.GetSoundEvents().SetTrackedSoundEventParameter(
		iId,
		parameterName,
		fValue))
	{
		SEOUL_LOG_UI("Failed setting parameter %s of sound event with id %d to value %f, "
			"check that this event was started.",
			parameterName.CStr(),
			iId,
			fValue);
	}
}

/**
 * Attempt to trigger a cue on tracked sound event with id iId.
 */
void Movie::TriggerTrackedSoundEventCue(Int32 iId)
{
	if (!m_Content.GetSoundEvents().TriggerTrackedSoundEventCue(iId))
	{
		SEOUL_LOG_UI("Failed triggering cue of sound event with id %d, "
			"check that this event was started.",
			iId);
	}
}

/** Get the frames per second of this UIMovie. */
Float Movie::GetFrameDeltaTimeInSeconds() const
{
	SharedPtr<Falcon::FCNFile> pFCNFile(m_pInternal->GetFCNFile());
	if (pFCNFile.IsValid())
	{
		return (1.0f / pFCNFile->GetFramesPerSecond());
	}

	return 0.0f;
}

/** Binding for script. */
Vector2D Movie::GetMousePositionFromWorld(const Vector2D& vWorldPosition) const
{
	SharedPtr<Falcon::FCNFile> pFile(m_pInternal->GetFCNFile());
	if (!pFile.IsValid())
	{
		return vWorldPosition;
	}

	// Compute stage layout
	auto const viewport = GetViewport();
	Float const fStageWidth = pFile->GetBounds().GetWidth();
	auto const vStageCoords = ComputeStageTopBottom(viewport, pFile->GetBounds().GetHeight());
	Float32 const fStageTopRenderCoord = vStageCoords.X;
	Float32 const fStageBottomRenderCoord = vStageCoords.Y;
	Float const fVisibleHeight = (fStageBottomRenderCoord - fStageTopRenderCoord);
	Float const fVisibleWidth = (fVisibleHeight * viewport.GetViewportAspectRatio());
	Float const fVisibleTop = fStageTopRenderCoord;
	Float const fVisibleLeft = (fStageWidth - fVisibleWidth) / 2.0f;

	Vector2D vReturn;
	vReturn.X = (((vWorldPosition.X - fVisibleLeft) / fVisibleWidth) * (Float)viewport.m_iViewportWidth + (Float)viewport.m_iViewportX);
	vReturn.Y = (((vWorldPosition.Y - fVisibleTop) / fVisibleHeight) * (Float)viewport.m_iViewportHeight + (Float)viewport.m_iViewportY);
	return vReturn;
}

/**
 * Attempt to populate rpRoot with a reference counted pointer of
 * the root MovieClip of this Movie's scene. Return true on success
 * (rpRoot was modified and is non-null) or false otherwise.
 */
Bool Movie::GetRootMovieClip(SharedPtr<Falcon::MovieClipInstance>& rpRoot) const
{
	if (m_pInternal.IsValid())
	{
		// If we have an internal pointer and a valid
		// root MovieClip, populate and return true.
		if (m_pInternal->GetRoot().IsValid())
		{
			rpRoot = m_pInternal->GetRoot();
			return true;
		}
	}

	return false;
}

/**
 * Convert the given viewport into a world space
 * bounds based on the current movie.
 */
Falcon::Rectangle Movie::ViewportToWorldBounds(const Viewport& viewport) const
{
	// Get the mouse position of both corners.
	const Vector2D vLT = GetMousePositionInWorld(Point2DInt(viewport.m_iViewportX, viewport.m_iViewportY));
	const Vector2D vRB = GetMousePositionInWorld(Point2DInt(viewport.m_iViewportX + viewport.m_iViewportWidth, viewport.m_iViewportY + viewport.m_iViewportHeight));

	// Assemble and return.
	return Falcon::Rectangle::Create(vLT.X, vRB.X, vLT.Y, vRB.Y);
}

/** @return the stage position in world twips from the specified viewport mouse position in screen pixels. */
Vector2D Movie::GetMousePositionInWorld(
	const Point2DInt& mousePosition,
	Bool& rbOutsideViewport) const
{
	Float const fStageWidth = GetMovieWidth();
	Float const fStageHeight = GetMovieHeight();

	auto const viewport = GetViewport();
	rbOutsideViewport = (
		mousePosition.X < viewport.m_iViewportX ||
		mousePosition.X >(viewport.m_iViewportX + viewport.m_iViewportWidth) ||
		mousePosition.Y < viewport.m_iViewportY ||
		mousePosition.Y >(viewport.m_iViewportY + viewport.m_iViewportHeight));

	Float const fRelativeX = (mousePosition.X - viewport.m_iViewportX) / (Float)viewport.m_iViewportWidth;
	Float const fRelativeY = (mousePosition.Y - viewport.m_iViewportY) / (Float)viewport.m_iViewportHeight;

	// Compute stage layout
	auto const vStageCoords = ComputeStageTopBottom(viewport, fStageHeight);
	Float32 const fStageTopRenderCoord = vStageCoords.X;
	Float32 const fStageBottomRenderCoord = vStageCoords.Y;
	Float const fVisibleHeight = (fStageBottomRenderCoord - fStageTopRenderCoord);
	Float const fVisibleWidth = (fVisibleHeight * viewport.GetViewportAspectRatio());
	Float const fVisibleTop = fStageTopRenderCoord;
	Float const fVisibleLeft = (fStageWidth - fVisibleWidth) / 2.0f;

	return Vector2D(
		(fRelativeX * fVisibleWidth) + fVisibleLeft,
		(fRelativeY * fVisibleHeight) + fVisibleTop);
}

Float Movie::GetMovieHeight() const
{
	SharedPtr<Falcon::FCNFile> pFile(m_pInternal->GetFCNFile());
	return pFile.IsValid() ? pFile->GetBounds().GetHeight() : (Float)GetViewport().m_iViewportHeight;
}

Float Movie::GetMovieWidth() const
{
	SharedPtr<Falcon::FCNFile> pFile(m_pInternal->GetFCNFile());
	return pFile.IsValid() ? pFile->GetBounds().GetWidth() : (Float)GetViewport().m_iViewportWidth;
}

Int Movie::AddMotion(const SharedPtr<Motion>& pMotion)
{
	return m_MotionCollection.AddMotion(pMotion);
}

void Movie::CancelMotion(Int iIdentifier)
{
	m_MotionCollection.CancelMotion(iIdentifier);
}

void Movie::CancelAllMotions(const SharedPtr<Falcon::Instance>& pInstance)
{
	m_MotionCollection.CancelAllMotions(pInstance);
}

Int Movie::AddTween(
	const SharedPtr<Falcon::Instance>& pInstance,
	TweenTarget eTarget,
	TweenType eTweenType,
	Float fStartValue,
	Float fEndValue,
	Float fDurationInSeconds,
	const SharedPtr<TweenCompletionInterface>& pCompletionInterface /*= SharedPtr<UITweenCompletionInterface>()*/)
{
	CheckedPtr<Tween> pTween(m_Tweens.AcquireTween());

	// Init tween values
	pTween->SetCompletionInterface(pCompletionInterface);
	pTween->SetDurationInSeconds(fDurationInSeconds);
	pTween->SetEndValue(fEndValue);
	pTween->SetInstance(pInstance);
	pTween->SetStartValue(fStartValue);
	pTween->SetTarget(eTarget);
	pTween->SetType(eTweenType);

	return pTween->GetIdentifier();
}

void Movie::CancelTween(Int iIdentifier)
{
	m_Tweens.CancelTween(iIdentifier);
}

void Movie::CancelAllTweens(const SharedPtr<Falcon::Instance>& pInstance)
{
	m_Tweens.CancelAllTweens(pInstance);
}

// Falcon::AdvanceInterface overrides
void Movie::FalconDispatchEnterFrameEvent(
	Falcon::Instance* pInstance)
{
	FalconDispatchEvent(FalconConstants::kEnterFrame, Falcon::SimpleActions::EventType::kEventDispatch, pInstance);
}

float Movie::FalconGetDeltaTimeInSeconds() const
{
	return Engine::Get()->GetSecondsInTick();
}

void Movie::FalconDispatchEvent(
	const HString& sEventName,
	Falcon::SimpleActions::EventType /*eType*/,
	Falcon::Instance* pInstance)
{
	if (FalconDispatchGotoEvent(pInstance, sEventName))
	{
		return;
	}

	// Method name is always event name for remaining types.
	HString const methodName(sEventName);

	using namespace Reflection;
	Method const* pMethod = GetReflectionThis().GetType().GetMethod(methodName);
	if (nullptr != pMethod)
	{
		// TODO: Report/track this error?
		(void)pMethod->TryInvoke(GetReflectionThis());
	}
}

bool Movie::FalconLocalize(
	const HString& sLocalizationToken,
	String& rsLocalizedText)
{
	HString const token(sLocalizationToken);
	String const sText(LocManager::Get()->Localize(token));
	rsLocalizedText = String(sText.CStr(), sText.GetSize());
	return true;
}
// /Falcon::AdvanceInterface overrides

/**
 * Called on the main thread by UI::State to give
 * this UI::Movie instance an opportunity to perform
 * construction after its vtable has been initialized.
 */
void Movie::OnConstructMovie(HString movieTypeName)
{
	SEOUL_ASSERT(IsMainThread());
	SEOUL_ASSERT(!m_bConstructed);

	// Cache the movie type name.
	m_MovieTypeName = movieTypeName;

	// Setup profiling variables in profiling builds.
	SEOUL_PROF_INIT_VAR(m_ProfAdvance, String(m_MovieTypeName) + ".Advance");
	SEOUL_PROF_INIT_VAR(m_ProfOnEnterState, String(m_MovieTypeName) + ".OnEnterState");
	SEOUL_PROF_INIT_VAR(m_ProfOnExitState, String(m_MovieTypeName) + ".OnExitState");
	SEOUL_PROF_INIT_VAR(m_ProfOnLoad, String(m_MovieTypeName) + ".OnLoad");
	SEOUL_PROF_INIT_VAR(m_ProfPrePose, String(m_MovieTypeName) + ".PrePose");
	SEOUL_PROF_INIT_VAR(m_ProfPose, String(m_MovieTypeName) + ".Pose");

	// Initially reset the FilePath.
	m_FilePath.Reset();

	// Get configuration settings if available and assign the movie file path.
	SharedPtr<DataStore> pSettings = Manager::Get()->GetSettings();
	DataNode movieConfig;
	if (pSettings.IsValid())
	{
		if (pSettings->GetValueFromTable(pSettings->GetRootNode(), m_MovieTypeName, movieConfig))
		{
			// Cache the movie file path, may be invalid if this UI::Movie does not have an FCN file.
			DataNode movieFilePathValue;
			(void)pSettings->GetValueFromTable(movieConfig, FalconConstants::kMovieFilePath, movieFilePathValue);
			(void)pSettings->AsFilePath(movieFilePathValue, m_FilePath);
		}
	}

	// Initialize m_pInternal.
	m_pInternal.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) MovieInternal(m_FilePath, m_MovieTypeName));
	m_pInternal->Initialize();

	// If we have configuration settings, do further initialization.
	if (pSettings.IsValid() && !movieConfig.IsNull())
	{
		// Deserialize settings into the movie object.
		(void)SettingsManager::Get()->DeserializeObject(
			Manager::Get()->GetSettingsFilePath(),
			GetReflectionThis(),
			m_MovieTypeName);

		// Configure content
		m_Content.Configure(
			Manager::Get()->GetSettingsFilePath(),
			*pSettings,
			movieConfig,
			false,  // bAppend = false
			m_MovieTypeName);
	}

	// Constructed.
	m_bConstructed = true;
}

void Movie::ConstructMovie(HString movieTypeName)
{
	OnConstructMovie(movieTypeName);

	// If a specialization didn't dispatch events inside OnConstructMovie(),
	// they will be pending, so dispatch them now.
	m_pInternal->DispatchEvents(*this);
}

void Movie::OnDestroyMovie()
{
	// Sanity check.
	SEOUL_ASSERT(m_bConstructed);
	SEOUL_ASSERT(!m_pNext.IsValid());
	SEOUL_ASSERT(!m_pPrev.IsValid());
	SEOUL_ASSERT(m_pInternal.IsValid());

	// Release Depth3DSource and ParentIfWorldspace if world reference
	// is inside any Fx instances to eliminate
	// possible cycles.
	{
		ContainerLock<ActiveFxInstances> lock(m_ActiveFxInstances);
		for (auto const& p : lock)
		{
			p->SetDepthSource(SharedPtr<Falcon::Instance>());
			p->SetParentIfWorldspace(SharedPtr<Falcon::Instance>());
		}
	}

	// No longer constructed.
	m_bConstructed = false;

	// Destroy our internal data.
	m_pInternal.Reset();
}

void Movie::AdvanceAnimations(Int iAdvanceCount, Float fFrameDeltaTimeInSeconds)
{
	// Dispatch tick events.
	{
		ContainerLock<TickEventTargets> lock(m_TickEventTargets);
		for (auto const& p : lock)
		{
			OnDispatchTickEvent(p.GetPtr());
		}
	}

	// Dispatch tick scaled events.
	{
		ContainerLock<TickEventTargets> lock(m_TickScaledEventTargets);
		for (auto const& p : lock)
		{
			OnDispatchTickScaledEvent(p.GetPtr());
		}
	}

	// TODO: Decide how we want to cull animations
	// that don't need to be ticked (e.g. off screen).

	// We fixed step advance tweens, animation, and fx
	// for the given number of advancements. An exemption -
	// animations can explicitly request variable time stepping
	// to maximize smoothness, in which case they are ticked
	// once for the full delta frame time.
	for (Int iAdvance = 0; iAdvance < iAdvanceCount; ++iAdvance)
	{
		// Advance tweens.
		m_Tweens.Advance(*this, kfFixedFrameTimeInSeconds);

		// Advance motion.
		m_MotionCollection.Advance(kfFixedFrameTimeInSeconds);

		// Tick Animation2D.
#if SEOUL_WITH_ANIMATION_2D
		{
			ContainerLock<ActiveAnimation2DInstances> lock(m_ActiveAnimation2DInstances);
			for (auto const& p : lock)
			{
				// Variable time stepa animations use the full delta T.
				// Otherwise, they're sub-stepped.
				if (p->GetVariableTimeStep())
				{
					if (0 == iAdvance)
					{
						p->Tick(fFrameDeltaTimeInSeconds);
					}
				}
				else
				{
					p->Tick(kfFixedFrameTimeInSeconds);
				}
			}
		}
#endif // /#if SEOUL_WITH_ANIMATION_2D

		// Tick Fx.
		{
			ContainerLock<ActiveFxInstances> lock(m_ActiveFxInstances);
			for (auto const& p : lock)
			{
				p->Tick(kfFixedFrameTimeInSeconds);
			}
		}
	}

	Int32 const nSize = (Int32)m_vToRemoveFxQueue.GetSize();
	for (Int32 i = 0; i < nSize; ++i)
	{
		SharedPtr<FxInstance> p(m_vToRemoveFxQueue[i]);
		if (p->GetParent())
		{
			p->GetParent()->RemoveChildAtDepth(p->GetDepthInParent());
		}
	}

	m_vToRemoveFxQueue.Clear();
}

/**
 * Based on any min-max aspect ratio setting, compute
 * the stage top/bottom render coordinates.
 */
Vector2D Movie::ComputeStageTopBottom(const Viewport& viewport, Float fStageHeight) const
{
	// Easy case, no min, so stage top/bottom is just 0 and the stage height.
	if (Manager::Get()->GetMinAspectRatio().IsZero())
	{
		return Vector2D(0.0f, fStageHeight);
	}

	// Compute viewport ratio.
	auto const fViewportRatio = viewport.GetViewportAspectRatio();

	// If below min, clamp.
	auto const fMinRatio = (Manager::Get()->GetMinAspectRatio().X / Manager::Get()->GetMinAspectRatio().Y);
	if (fViewportRatio < fMinRatio)
	{
		// Divide viewport width by min to compute how much we oversize the stage.
		auto const fDesiredStageWidth = (fStageHeight * fMinRatio);
		auto const fDesiredStageHeight = (fDesiredStageWidth / fViewportRatio);
		auto const fStageChange = (fDesiredStageHeight - fStageHeight);
		auto const fPadding = Max(0.5f * fStageChange, 0.0f);

		// Done.
		return Vector2D(-fPadding, fStageHeight + fPadding);
	}

	// Done.
	return Vector2D(0.0f, fStageHeight);
}

/** Cache the state we're entering as our active state. */
void Movie::OnEnterState(CheckedPtr<State const> pPreviousState, CheckedPtr<State const> pNextState, Bool bWasInPreviousState)
{
	m_pOwner = pNextState;
}

/** If we're not in the next state, clear our owner state. */
void Movie::OnExitState(CheckedPtr<State const> pPreviousState, CheckedPtr<State const> pNextState, Bool bIsInNextState)
{
	if (!bIsInNextState)
	{
		m_pOwner.Reset();
	}
}

Bool Movie::OnTryBroadcastEvent(
	HString eventName,
	const Reflection::MethodArguments& aMethodArguments,
	Int nArgumentCount)
{
	using namespace Reflection;

	// Get the this pointer and Type of the movie.
	const WeakAny& reflectionThis = GetReflectionThis();
	const Type& type = reflectionThis.GetType();

	// Resolve the method.
	Method const* pMethod = type.GetMethod(eventName);

	// If successful, invoke it.
	if (nullptr != pMethod)
	{
		MethodInvokeResult result = pMethod->TryInvoke(reflectionThis, aMethodArguments);
		if (!result)
		{
			// If the invalid argument index is >= 0, it corresponds to the argument
			// that caused the invoke fail.
			if (result.GetInvalidArgument() >= 0)
			{
				SEOUL_WARN("Failed broadcasting event %s, invalid arguments %d, expected type: %s, got type: %s\n",
					eventName.CStr(),
					result.GetInvalidArgument(),
					GetTypeString(pMethod->GetTypeInfo().GetArgumentTypeInfo(result.GetInvalidArgument())).CStr(),
					GetTypeString(aMethodArguments[result.GetInvalidArgument()].GetTypeInfo()).CStr());
			}
			// Otherwise, the "this" pointer failed to cast, which we never expected to happen,
			// but handle here as a generic "wtf" error.
			else
			{
				SEOUL_WARN("Failed broadcasting event %s, invoke error, check that all arguments to the method are valid.",
					eventName.CStr());
			}

			return false;
		}
		else
		{
			return true;
		}
	}

	return false;
}

Viewport Movie::GetViewport() const
{
	return Manager::Get()->ComputeViewport();
}

void Movie::QueueFxToRemove(FxInstance* pInstance)
{
	m_vToRemoveFxQueue.PushBack(SharedPtr<FxInstance>(pInstance));
}

/**
 * Actions that must be perform during PrePose, even if this UI::Movie
 * is blocked waiting for FCN content to load.
 */
void Movie::PrePoseWhenBlocked(RenderPass& rPass, Float fDeltaTimeInSeconds)
{
	SEOUL_ASSERT(IsMainThread());

	// Poll content.
	m_Content.Poll();

	// If we're not pauseable, or if we are and the game isn't paused...
	if (!IsPaused())
	{
		// Let the UI::Movie subclass tick for the current frame.
		OnTickWhenBlocked(rPass, fDeltaTimeInSeconds);
	}
}

/**
 * Developer only utility. Return a list of points that can be potentially
 * hit based on the input test max. This applies to all state machines and all
 * movies currently active.
 */
Bool Movie::GetHitPoints(
	HString stateMachine,
	HString state,
	UInt8 uInputMask,
	HitPoints& rvHitPoints) const
{
	if (!m_bAcceptInput)
	{
		return true;
	}

	Bool const bStopTesting = !m_bAllowInputToScreensBelow;
	SharedPtr<Falcon::FCNFile> pFile(m_pInternal->GetFCNFile());
	if (!pFile.IsValid())
	{
		return bStopTesting;
	}

	SharedPtr<Falcon::MovieClipInstance> pRoot(m_pInternal->GetRoot());
	if (!pRoot.IsValid())
	{
		return bStopTesting;
	}

	Viewport const viewport = GetViewport();
	Vector2D const vUL = GetMousePositionInWorld(Point2DInt(viewport.m_iViewportX, viewport.m_iViewportY));
	Vector2D const vLR = GetMousePositionInWorld(Point2DInt(viewport.m_iViewportX + viewport.m_iViewportWidth, viewport.m_iViewportY + viewport.m_iViewportHeight));

	Falcon::Rectangle const viewportRectangle = Falcon::Rectangle::Create(vUL.X, vLR.X, vUL.Y, vLR.Y);

	auto tester(Manager::Get()->GetRenderer().GetHitTester(
		*this,
		pFile->GetBounds(),
		viewport));

	GetUIHitPoints(
		*this,
		tester,
		stateMachine,
		state,
		pRoot,
		viewportRectangle,
		pRoot,
		Matrix2x3::Identity(),
		uInputMask,
		rvHitPoints);

	// If we have a pass through configured, include a point for that as well.
	if (!m_PassthroughInputFunction.IsEmpty() || !m_PassthroughInputTrigger.IsEmpty())
	{
		HitPoint passthrough;
		passthrough.m_Id = kPassthroughId;
		passthrough.m_State = state;
		passthrough.m_StateMachine = stateMachine;
		passthrough.m_Movie = GetMovieTypeName();

		// Add a pass through point for each edge.
		passthrough.m_vCenterPoint = GetMousePositionFromWorld(Vector2D((vUL.X + vLR.X) * 0.5f, (vUL.Y + vLR.Y) * 0.5f));
		passthrough.m_vTapPoint = GetMousePositionFromWorld(Vector2D(vUL.X + 1.0f, (vUL.Y + vLR.Y) * 0.5f));
		rvHitPoints.PushBack(passthrough);
		passthrough.m_vTapPoint = GetMousePositionFromWorld(Vector2D(vLR.X - 1.0f, (vUL.Y + vLR.Y) * 0.5f));
		rvHitPoints.PushBack(passthrough);
		passthrough.m_vTapPoint = GetMousePositionFromWorld(Vector2D((vUL.X + vLR.X) * 0.5f, vUL.Y + 1.0f));
		rvHitPoints.PushBack(passthrough);
		passthrough.m_vTapPoint = GetMousePositionFromWorld(Vector2D((vUL.X + vLR.X) * 0.5f, vLR.Y - 1.0f));
		rvHitPoints.PushBack(passthrough);
	}

	return bStopTesting;
}

MovieHitTestResult Movie::OnHitTest(
	UInt8 uMask,
	const Point2DInt& mousePosition,
	Movie*& rpHitMovie,
	SharedPtr<Falcon::MovieClipInstance>& rpHitInstance,
	SharedPtr<Falcon::Instance>& rpLeafInstance,
	Vector<Movie*>* rvPassthroughInputs) const
{
	if (!m_bAcceptInput)
	{
		return MovieHitTestResult::kNoHitStopTesting;
	}

	MovieHitTestResult const eNoHitReturn = (m_bAllowInputToScreensBelow
		? MovieHitTestResult::kNoHit
		: MovieHitTestResult::kNoHitStopTesting);

	SharedPtr<Falcon::FCNFile> pFile(m_pInternal->GetFCNFile());
	if (!pFile.IsValid())
	{
		return eNoHitReturn;
	}

	const Vector2D vWorldPosition = GetMousePositionInWorld(mousePosition);

	SharedPtr<Falcon::MovieClipInstance> pRoot(m_pInternal->GetRoot());
	if (!pRoot.IsValid())
	{
		return eNoHitReturn;
	}

	auto tester(Manager::Get()->GetRenderer().GetHitTester(
		*this,
		pFile->GetBounds(),
		GetViewport()));

	SharedPtr<Falcon::MovieClipInstance> pHitMovieClipInstance;
	SharedPtr<Falcon::Instance> pLeafInstance;
	Falcon::HitTestResult const eResult = pRoot->HitTest(
		tester,
		uMask,
		Matrix2x3::Identity(),
		vWorldPosition.X,
		vWorldPosition.Y,
		pHitMovieClipInstance,
		pLeafInstance);

	if (Falcon::HitTestResult::kHit == eResult)
	{
		rpHitInstance = pHitMovieClipInstance;
		rpLeafInstance = pLeafInstance;
		rpHitMovie = const_cast<Movie*>(this);
		return MovieHitTestResult::kHit;
	}
	else if (Falcon::HitTestResult::kNoHitStopTesting == eResult)
	{
		return MovieHitTestResult::kNoHitStopTesting;
	}
	else if (Falcon::HitTestResult::kNoHit == eResult)
	{
		if (!m_PassthroughInputTrigger.IsEmpty() || !m_PassthroughInputFunction.IsEmpty())
		{
			rpHitMovie = const_cast<Movie*>(this);

			if (nullptr != rvPassthroughInputs)
			{
				rvPassthroughInputs->PushBack(rpHitMovie);
			}

			if (!m_bContinueInputOnPassthrough)
			{
				return MovieHitTestResult::kNoHitTriggerBack;
			}
		}
	}

	return eNoHitReturn;
}

void Movie::OnPick(
	const Point2DInt& mousePosition,
	Vector< SharedPtr<Falcon::Instance>, MemoryBudgets::UIRuntime>& rv) const
{
	SharedPtr<Falcon::FCNFile> pFile(m_pInternal->GetFCNFile());
	if (!pFile.IsValid())
	{
		return;
	}

	Bool bOutsideViewport = false;
	const Vector2D vWorldPosition = GetMousePositionInWorld(mousePosition, bOutsideViewport);

	SharedPtr<Falcon::MovieClipInstance> pRoot(m_pInternal->GetRoot());
	if (!pRoot.IsValid())
	{
		return;
	}

	auto tester(Manager::Get()->GetRenderer().GetHitTester(
		*this,
		pFile->GetBounds(),
		GetViewport()));

	pRoot->Pick(
		tester,
		Matrix2x3::Identity(),
		Falcon::ColorTransformWithAlpha::Identity(),
		vWorldPosition.X,
		vWorldPosition.Y,
		rv);
}

MovieHitTestResult Movie::OnSendInputEvent(InputEvent eInputEvent)
{
	if (!m_bAcceptInput)
	{
		return MovieHitTestResult::kNoHitStopTesting;
	}

	MovieHitTestResult const eNoHitReturn = (m_bAllowInputToScreensBelow
		? MovieHitTestResult::kNoHit
		: MovieHitTestResult::kNoHitStopTesting);

	return eNoHitReturn;
}

MovieHitTestResult Movie::OnSendButtonEvent(
	Seoul::InputButton eButtonID,
	Seoul::ButtonEventType eButtonEventType)
{
	if (!m_bAcceptInput)
	{
		return MovieHitTestResult::kNoHitStopTesting;
	}

	MovieHitTestResult const eNoHitReturn = (m_bAllowInputToScreensBelow
		? MovieHitTestResult::kNoHit
		: MovieHitTestResult::kNoHitStopTesting);

	return eNoHitReturn;
}

/**
 * Called once per frame as part of rendering on the main thread - dispatches
 * queued Lua -> C++ callbacks and invokes the subclasses update method.
 */
void Movie::PrePose(RenderPass& rPass, Float fDeltaTimeInSeconds)
{
	SEOUL_ASSERT(IsMainThread());

	// TODO: This is a hack - we've introduced dependencies in per-movie
	// logic that can be render dependent (specifically, screen resolution, viewport
	// clamping, and the mapping between UI world space to viewport space).
	//
	// Generally need to fix this. Likely solution is to promote all values to be
	// stored in the movie and hide the UI's renderer from public access.
	SetMovieRendererDependentState();

	// Poll content.
	m_Content.Poll();

	// Force refresh FX so world space particles
	// are updated with the viewport change.
	if (m_bLastViewportChanged)
	{
		{
			ContainerLock<ActiveFxInstances> lock(m_ActiveFxInstances);
			for (auto const& p : lock)
			{
				p->Tick(0.0f);
			}
		}

		m_bLastViewportChanged = false;
	}

	// If we're not pauseable, or if we are and the game isn't paused...
	if (!IsPaused())
	{
		// Let the UI::Movie subclass tick for the current frame.
		OnTick(rPass, fDeltaTimeInSeconds);
	}
}

// TODO: This is a hack - we've introduced dependencies in per-movie
// logic that can be render dependent (specifically, screen resolution, viewport
// clamping, and the mapping between UI world space to viewport space).
//
// Generally need to fix this. Likely solution is to promote all values to be
// stored in the movie and hide the UI's renderer from public access.
void Movie::SetMovieRendererDependentState()
{
	SharedPtr<Falcon::FCNFile> pFile(m_pInternal->GetFCNFile());
	if (pFile.IsValid())
	{
		// TODO: find a better way to fix the viewport problem.
		Manager::Get()->GetRenderer().SetMovieDependentState(this, GetViewport(), pFile->GetBounds());
	}
}

void Movie::AdvanceWhenBlocked(Float fDeltaTimeInSeconds)
{
	OnAdvanceWhenBlocked(fDeltaTimeInSeconds);
}

void Movie::Advance(Float fBaseDeltaTimeInSeconds)
{
	// TODO: This is a hack - we've introduced dependencies in per-movie
	// logic that can be render dependent (specifically, screen resolution, viewport
	// clamping, and the mapping between UI world space to viewport space).
	//
	// Generally need to fix this. Likely solution is to promote all values to be
	// stored in the movie and hide the UI's renderer from public access.
	SetMovieRendererDependentState();

	auto const fScaledDeltaTimeInSeconds = (Float)(fBaseDeltaTimeInSeconds * Engine::Get()->GetSecondsInTickScale());
	AdvanceWhenBlocked(fBaseDeltaTimeInSeconds);

	// If we're not pauseable, or if we are and the game isn't paused...
	if (!IsPaused())
	{
		// Accumulate time.
		m_fAccumulatedScaledFrameTime += fScaledDeltaTimeInSeconds;

		// Determine how many fixed step advancements will
		// occur for this frame.
		Int iAdvance = 0;
		while (m_fAccumulatedScaledFrameTime + kfAccumulationSlopInSeconds >= kfFixedFrameTimeInSeconds)
		{
			++iAdvance;
			m_fAccumulatedScaledFrameTime -= kfFixedFrameTimeInSeconds;
		}

		// If our accumulated time after processing is at or
		// below kfAccumulationSlopInSeconds, just zero it out.
		if (m_fAccumulatedScaledFrameTime <= kfAccumulationSlopInSeconds)
		{
			m_fAccumulatedScaledFrameTime = 0.0f;
		}

		// Advance the movie itself.
		m_pInternal->Advance(*this, fBaseDeltaTimeInSeconds);

		// Animations, tweens, and fx.
		AdvanceAnimations(iAdvance, fScaledDeltaTimeInSeconds);

		// If factoring in screen shake, apply that now.
		if (m_bScreenShake && FxManager::Get())
		{
			SharedPtr<Falcon::MovieClipInstance> pRoot;
			if (GetRootMovieClip(pRoot))
			{
				pRoot->SetPosition(FxManager::Get()->GetScreenShakeOffset());
			}
		}

		// Allow subclasses to advance.
		OnAdvance(fBaseDeltaTimeInSeconds);
	}
}

/**
 * Draws the Falcon movie instance.
 */
void Movie::OnPose(RenderPass& rPass, Renderer& rRenderer)
{
	m_pInternal->Pose(this, rRenderer);
}

#if SEOUL_ENABLE_CHEATS
/**
 * Developer only method, performs a render pass to visualize input hit testable
 * areas.
 */
void Movie::OnPoseInputVisualization(
	const InputWhitelist& inputWhitelist,
	UInt8 uInputMask,
	RenderPass& rPass,
	Renderer& rRenderer)
{
	m_pInternal->PoseInputVisualization(
		inputWhitelist,
		this,
		uInputMask,
		rRenderer);
}
#endif // /#if SEOUL_ENABLE_CHEATS

} // namespace UI

} // namespace Seoul
