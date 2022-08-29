/**
 * \file FalconMovieClipInstance.cpp
 * \brief MovieClips form the bulk of the nodes in a typical
 * Falcon scene graph.
 *
 * MovieClips serve two main purposes in a scene graph:
 * - they are the only interior nodes (they can have children).
 * - they support timeline animations, which can both mutate
 *   existing children as well as instantiate new children.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconDefinition.h"
#include "FalconGlobalConfig.h"
#include "FalconMovieClipDefinition.h"
#include "FalconMovieClipInstance.h"
#include "FalconRenderState.h"
#include "FalconStage3DSettings.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_TYPE(Falcon::LabelName);

SEOUL_BEGIN_TYPE(Falcon::MovieClipInstance, TypeFlags::kDisableNew)
	SEOUL_PARENT(Falcon::Instance)
	SEOUL_PROPERTY_PAIR_N("AbsorbOtherInput", GetAbsorbOtherInput, SetAbsorbOtherInput)
	SEOUL_PROPERTY_PAIR_N("AutoCulling", GetAutoCulling, SetAutoCulling)
	SEOUL_PROPERTY_PAIR_N("AutoDepth3D", GetAutoDepth3D, SetAutoDepth3D)
	SEOUL_PROPERTY_PAIR_N("Depth3D", GetDepth3D, SetDepth3D)
	SEOUL_PROPERTY_N_EXT("ClassName", GetClassName)
	SEOUL_PROPERTY_N_EXT("CurrentFrame", GetCurrentFrame)
	SEOUL_PROPERTY_N_EXT("CurrentLabel", GetCurrentLabel)
	SEOUL_PROPERTY_PAIR_N("ExactHitTest", GetExactHitTest, SetExactHitTest)
	SEOUL_PROPERTY_PAIR_N("HitTestChildrenMask", GetHitTestChildrenMask, SetHitTestChildrenMask)
	SEOUL_PROPERTY_PAIR_N("HitTestSelfMask", GetHitTestSelfMask, SetHitTestSelfMask)
	SEOUL_PROPERTY_N_EXT("IsPlaying", IsPlaying)
	SEOUL_PROPERTY_N_EXT("TotalFrames", GetTotalFrames)
SEOUL_END_TYPE()

namespace Falcon
{

#if SEOUL_ENABLE_CHEATS
static const RGBA kInputVizAbsorbInput(RGBA::Create(0, 0, 0, 0));
static const RGBA kInputVizClick(RGBA::Create(255, 128, 64, 196));
static const RGBA kInputVizDrag(RGBA::Create(64, 128, 255, 196));
static const RGBA kInputVizOther(RGBA::Create(255, 255, 255, 196));
#endif // /#if SEOUL_ENABLE_CHEATS

// TODO: This is messy (but convenient). We should probably fold goto* parsing
// into the loader, and refactor AdvanceInterface to contain a unique API for this.
Bool AdvanceInterface::FalconDispatchGotoEvent(Instance* pInstance, const HString& sEventName)
{
	if (pInstance->GetType() != InstanceType::kMovieClip)
	{
		return false;
	}

	MovieClipInstance* pMovieClipInstance = (MovieClipInstance*)pInstance;
	if (FalconIsGotoAndPlayEvent(sEventName))
	{
		(void)pMovieClipInstance->GotoAndPlay(*this, FalconGetGotoAndPlayFrameNumber(sEventName));
		return true;
	}
	else if (FalconIsGotoAndPlayByLabelEvent(sEventName))
	{
		(void)pMovieClipInstance->GotoAndPlayByLabel(*this, FalconGetGotoAndPlayFrameLabel(sEventName));
		return true;
	}
	else if (FalconIsGotoAndStopEvent(sEventName))
	{
		(void)pMovieClipInstance->GotoAndStop(*this, FalconGetGotoAndStopFrameNumber(sEventName));
		return true;
	}
	else if (FalconIsGotoAndStopByLabelEvent(sEventName))
	{
		(void)pMovieClipInstance->GotoAndStopByLabel(*this, FalconGetGotoAndStopFrameLabel(sEventName));
		return true;
	}

	return false;
}

MovieClipInstance::MovieClipInstance(const SharedPtr<MovieClipDefinition const>& pMovieClip)
	: Instance(pMovieClip->GetDefinitionID())
	, m_DisplayList()
	, m_pMovieClip(pMovieClip)
	, m_iCurrentFrame(-1)
	, m_fDepth3D(0.0f)
	, m_uHitTestSelfMask(0)
	, m_uHitTestChildrenMask(0xFF)
	, m_bPlaying(true)
	, m_bAfterGoto(false)
	, m_bEnableEnterFrame(false)
	, m_bExactHitTest(false)
	, m_bAbsorbOtherInput(false)
	, m_bAutoCulling(false)
	, m_bInputActionDisabled(false)
	, m_bCastPlanarShadows(false)
	, m_bAutoDepth3D(false)
	, m_bDeferDrawing(false)
	, m_uUnusedReserved(0)
{
}

MovieClipInstance::~MovieClipInstance()
{
}

void MovieClipInstance::Advance(AdvanceInterface& rInterface)
{
	// TODO: This may be surprising for folks
	// used to working in ActionScript/Flash, but for our uses,
	// SetVisible(false) always means "disable this thing", so
	// we want to avoid the cost of traversing the graph
	// of invisible things. Eventually, we may want an
	// additional value to indicate "dont advance" explicitly
	// instead of using GetVisible().

	// Note that, there is one bit of inconsistency/surprise that
	// is still a WIP. The effects of visible = * and stop() on
	// the timeline apply immediately in all situations (they
	// are applied as part of Goto* processing). gotoAnd* and
	// dispatchEvent() on the timeline, however, are deferred
	// until the Advance() call, which means they are not applied
	// until the first Advance() for which this MovieClipInstance
	// is visible. This also means that 2 calls to GotoAnd* without
	// an Advance() in between can result in a missed gotoAnd*
	// or dispatchEvent() on the timeline, where as visible =* and
	// stop() will always be applied.

	// Don't advance the current MovieClip or its children if
	// the current MovieClip is not visible.
	if (!GetVisible())
	{
		return;
	}

	if (m_bAfterGoto || m_bPlaying)
	{
		const MovieClipDefinition::DisplayListTags& vDisplayListTags = m_pMovieClip->GetDisplayListTags();
		const MovieClipDefinition::FrameOffsets& vFrameOffsets = m_pMovieClip->GetFrameOffsets();

		Int32 const iPreviousFrame = m_iCurrentFrame;

		if (!m_bAfterGoto)
		{
			++m_iCurrentFrame;
			if (m_iCurrentFrame >= (Int32)m_pMovieClip->GetFrameCount())
			{
				m_iCurrentFrame = 0;
			}
		}

		if (iPreviousFrame != m_iCurrentFrame)
		{
			UInt32 const uBegin = (m_iCurrentFrame == 0
				? 0
				: (m_iCurrentFrame - 1 < (Int32)vFrameOffsets.GetSize() ? (vFrameOffsets[m_iCurrentFrame - 1] + 1) : 0));
			UInt32 const uEnd = (((Int32)vFrameOffsets.GetSize() > m_iCurrentFrame)
				? (vFrameOffsets[m_iCurrentFrame] + 1)
				: vDisplayListTags.GetSize());

			for (UInt32 i = uBegin; i < uEnd; ++i)
			{
				vDisplayListTags[i]->Apply(rInterface, this, m_DisplayList);
			}
		}

		// If we just handled a transition from frame -1 to
		// a valid frame, conditionally report add to parent.
		// This occurs at this point so that we provide
		// a consistent view to the external world (children
		// reach frame 0 before the external world knows about
		// the parent reaching frame 0).
		if (iPreviousFrame < 0 && m_iCurrentFrame >= 0)
		{
			ReportOnAddToParentIfNeeded(rInterface);
		}

		if (m_bAfterGoto || iPreviousFrame != m_iCurrentFrame)
		{
			// No longer after a goto, one way or another.
			m_bAfterGoto = false;

			// Cache simple frame actions for the current frame, if there are any.
			SimpleActions::FrameActions const* pFrameActions = m_pMovieClip->GetSimpleActions().m_tFrameActions.Find((UInt16)m_iCurrentFrame);

			// Apply non-event actions, if the frame has changed. These
			// actions were already applied in the case of a goto.
			if (iPreviousFrame != m_iCurrentFrame)
			{
				ApplyNonEventFrameActions();
			}

			// If we have events, fire them.
			if (nullptr != pFrameActions && !pFrameActions->m_vEvents.IsEmpty())
			{
				Vector<HString, MemoryBudgets::Falcon>::SizeType const uEvents = pFrameActions->m_vEvents.GetSize();
				for (Vector<HString, MemoryBudgets::Falcon>::SizeType i = 0; i < uEvents; ++i)
				{
					auto const& evt = pFrameActions->m_vEvents[i];
					HString const sEventName(evt.First);
					rInterface.FalconDispatchEvent(sEventName, evt.Second, this);
				}
			}
		}
	}

	if (m_bEnableEnterFrame)
	{
		rInterface.FalconDispatchEnterFrameEvent(this);
	}

	m_DisplayList.Advance(rInterface);
}

void MovieClipInstance::AdvanceToFrame0(AddInterface& rInterface)
{
	if (m_iCurrentFrame < 0)
	{
		(void)GotoFrame(rInterface, 0);

		// On frame 0 advance, if we have a parent, we now report to
		// add to parent event. This is deferred until after children
		// are created, to maintain a consistent dependency assumption
		// (children are created before we report parent creation,
		// so that external code sees a state where children already
		// exist).
		ReportOnAddToParentIfNeeded(rInterface);
	}
}

Bool MovieClipInstance::ComputeLocalBounds(Rectangle& rBounds)
{
	return m_DisplayList.ComputeBounds(rBounds);
}

Bool MovieClipInstance::ComputeHitTestableLocalBounds(Rectangle& rBounds, UInt8 uHitTestMask)
{
	Rectangle rOutRec = Rectangle::InverseMax();
	if (InternalComputeHitTestableLocalBounds(rOutRec, uHitTestMask))
	{
		rBounds = rOutRec;
		return true;
	}

	return false;
}

Bool MovieClipInstance::InternalComputeHitTestableLocalBounds(Rectangle& rBounds, UInt8 uHitTestMask)
{
	return m_DisplayList.ComputeHitTestableLocalBounds(rBounds,
		(GetHitTestSelfMask() & uHitTestMask) != 0,
		(GetHitTestChildrenMask() & uHitTestMask) != 0,
		uHitTestMask);
}

Bool MovieClipInstance::ComputeHitTestableBounds(Rectangle& rBounds, UInt8 uHitTestMask)
{
	Rectangle rOutRec = Rectangle::InverseMax();
	if (InternalComputeHitTestableBounds(rOutRec, uHitTestMask))
	{
		rBounds = rOutRec;
		return true;
	}

	return false;
}

Bool MovieClipInstance::InternalComputeHitTestableBounds(Rectangle& rBounds, UInt8 uHitTestMask)
{
	Bool bBoundsFound = m_DisplayList.ComputeHitTestableLocalBounds(rBounds,
		(GetHitTestSelfMask() & uHitTestMask) != 0,
		(GetHitTestChildrenMask() & uHitTestMask) != 0,
		uHitTestMask);

	if (bBoundsFound)
	{
		rBounds = TransformRectangle(GetTransform(), rBounds);
	}

	return bBoundsFound;
}

Bool MovieClipInstance::ComputeHitTestableWorldBounds(Rectangle& rBounds, UInt8 uHitTestMask)
{
	Rectangle rOutRec = Rectangle::InverseMax();
	if (InternalComputeHitTestableWorldBounds(rOutRec, uHitTestMask))
	{
		rBounds = rOutRec;
		return true;
	}

	return false;
}

Bool MovieClipInstance::InternalComputeHitTestableWorldBounds(Rectangle& rBounds, UInt8 uHitTestMask)
{
	Bool bBoundsFound = m_DisplayList.ComputeHitTestableLocalBounds(rBounds,
		(GetHitTestSelfMask() & uHitTestMask) != 0,
		(GetHitTestChildrenMask() & uHitTestMask) != 0,
		uHitTestMask);

	if (bBoundsFound)
	{
		if (GetParent())
		{
			rBounds = Falcon::TransformRectangle(GetParent()->ComputeWorldTransform(), rBounds);
		}
	}

	return bBoundsFound;
}

void MovieClipInstance::ComputeMask(
	const Matrix2x3& mParent,
	const ColorTransformWithAlpha& cxParent,
	Render::Poser& rPoser)
{
	// TODO: Reconsider - we don't consider the alpha
	// to match Flash behavior. I've never double checked what
	// happens if you (just) set the visibility of a mask to
	// false and logically it makes sense for visibility and alpha==0.0
	// to have the same behavior (or, in other words, visibility should
	// possibly not be considered here).
	if (!GetVisible())
	{
		return;
	}

	// Unlike many code paths, alpha == 0.0 is not considered here. Flash
	// does not hide the mask (or the shapes it reveals) if the cumulative
	// alpha at that mask is 0.0.

	ColorTransformWithAlpha const cxWorld = (cxParent * GetColorTransformWithAlpha());
	Matrix2x3 const mWorld = (mParent * GetTransform());
	return m_DisplayList.ComputeMask(mWorld, cxWorld, rPoser);
}

#if SEOUL_ENABLE_CHEATS
void MovieClipInstance::PoseInputVisualizationChildren(
	const InputWhitelist& inputWhitelist,
	UInt8 uInputMask,
	Render::Poser& rPoser,
	const Matrix2x3& mParent,
	const ColorTransformWithAlpha cxParent)
{
	if (!GetVisible())
	{
		return;
	}

	// Check whether input testing applies.
	Bool const bSelf = (0u != (uInputMask & m_uHitTestSelfMask));
	Bool const bActionDisabled = GetInputActionDisabled();
	Bool const bAbsorbOther = GetAbsorbOtherInput();
	Bool const bChildren = (0u != (uInputMask & m_uHitTestChildrenMask));

	// Something to do if testing self, children, or absorb other.
	if (bAbsorbOther || bSelf || bChildren)
	{
		// If we're hit testing self, we use the display color. Otherwise,
		// we use a transparent color - we just want to populate the depth
		// buffer, not actually contribute to rendering.
		//
		// Color selection is - use bit0 color if only bit0, use bit1 color
		// if only bit1, otherwise use the "other" color.
		RGBA color = kInputVizOther;
		if (bActionDisabled)
		{
			color = kInputVizAbsorbInput;
		}
		else if (bSelf)
		{
			if (0u == ((uInputMask & m_uHitTestSelfMask) & ~kuClickMouseInputHitTest))
			{
				color = kInputVizClick;
			}
			else if (0u == ((uInputMask & m_uHitTestSelfMask) & ~kuDragMouseInputHitTest))
			{
				color = kInputVizDrag;
			}
		}
		else if (bAbsorbOther)
		{
			color = kInputVizAbsorbInput;
		}

		// Compute the transform and display.
		Matrix2x3 const mWorld = (mParent * GetTransform());
		rPoser.PushDepth3D(m_fDepth3D, GetIgnoreDepthProjection());
		m_DisplayList.PoseInputVisualization(
			inputWhitelist,
			color,
			uInputMask,
			rPoser,
			mWorld,
			cxParent,
			(bSelf || bAbsorbOther),
			bChildren,
			m_bExactHitTest);
		rPoser.PopDepth3D(m_fDepth3D, GetIgnoreDepthProjection());
	}
}
#endif // /#if SEOUL_ENABLE_CHEATS

void MovieClipInstance::Pose(
	Render::Poser& rPoser,
	const Matrix2x3& mParent,
	const ColorTransformWithAlpha& cxParent)
{
	if (!GetVisible())
	{
		return;
	}

	ColorTransformWithAlpha const cxWorld = (cxParent * GetColorTransformWithAlpha());
	if (cxWorld.m_fMulA == 0.0f)
	{
		return;
	}

	// Refresh draw order based on depth 3D, if enabled.
	if (m_DisplayList.GetSortByDepth3D())
	{
		m_DisplayList.ReorderFromDepth3D();
	}

	// Refresh auto culling if enabled.
	if (m_bAutoCulling)
	{
		// Find the best cull node starting at the current node.
		// Function assigns the best node and the (last found)
		// current cull node, and includes the best count,
		// if a best node was found (best is defined as the node
		// with the highest number of children defined in the
		// subtree as limited by g_Config.m_AutoCullingConfig).
		MovieClipInstance* pCurrent = nullptr;
		MovieClipInstance* pBest = nullptr;

		// Start with the min child count, so we don't find a best
		// if no node hits our threshold.
		UInt32 uBestCount = g_Config.m_AutoCullingConfig.m_uMinChildCountForCulling;
		FindBestCullNode(
			0u,
			pCurrent,
			pBest,
			uBestCount);

		// If the best has changed, disable culling on current
		// and enable on best.
		if (pCurrent != pBest)
		{
			// Disable culling on the existing node.
			if (pCurrent)
			{
				pCurrent->DisableCulling();
			}

			// Enable on the best.
			if (pBest)
			{
				pBest->EnableCulling();
			}
		}
	}

	Matrix2x3 const mWorld = (mParent * GetTransform());

	if (m_bAutoDepth3D)
	{
		Float fY = 0.0f;
		Falcon::Rectangle bounds;
		if (ComputeLocalBounds(bounds))
		{
			fY = Falcon::TransformRectangle(mWorld, bounds).m_fBottom;
		}
		else
		{
			fY = mWorld.TY;
		}

		m_fDepth3D = rPoser.GetState().ComputeDepth3D(fY);
	}

	if (m_bDeferDrawing)
	{
		rPoser.BeginDeferDraw();
	}

	if (m_bCastPlanarShadows && rPoser.GetState().m_pStage3DSettings->m_Shadow.GetEnabled())
	{
		rPoser.BeginPlanarShadows();
		rPoser.PushDepth3D(m_fDepth3D, GetIgnoreDepthProjection());
		m_DisplayList.Pose(rPoser, mWorld, cxWorld);
		rPoser.PopDepth3D(m_fDepth3D, GetIgnoreDepthProjection());
		rPoser.EndPlanarShadows();
	}

	rPoser.PushDepth3D(m_fDepth3D, GetIgnoreDepthProjection());
	m_DisplayList.Pose(rPoser, mWorld, cxWorld);
	rPoser.PopDepth3D(m_fDepth3D, GetIgnoreDepthProjection());

	if (m_bDeferDrawing)
	{
		rPoser.EndDeferDraw();
	}
}

static const HString kMovieClip("MovieClip");

HString MovieClipInstance::GetClassName() const
{
	return m_pMovieClip->GetClassName().IsEmpty() ? kMovieClip : m_pMovieClip->GetClassName();
}

LabelName MovieClipInstance::GetCurrentLabel() const
{
	auto const iBegin = m_pMovieClip->GetFrameLabels().Begin();
	auto const iEnd = m_pMovieClip->GetFrameLabels().End();

	Int32 iBest = -1;
	LabelName sBest;
	for (auto i = iBegin; iEnd != i; ++i)
	{
		if (i->Second <= m_iCurrentFrame && i->Second > iBest)
		{
			iBest = i->Second;
			sBest = i->First;
		}
	}

	return sBest;
}

UInt32 MovieClipInstance::GetTotalFrames() const
{
	return m_pMovieClip->GetFrameCount();
}

Bool MovieClipInstance::GotoAndPlayByLabel(AddInterface& rInterface, const LabelName& sLabel)
{
	UInt16 uFrame = 0;
	if (m_pMovieClip->GetFrameLabels().GetValue(sLabel, uFrame))
	{
		return GotoAndPlay(rInterface, uFrame);
	}

	return false;
}

Bool MovieClipInstance::GotoAndStopByLabel(AddInterface& rInterface, const LabelName& sLabel)
{
	UInt16 uFrame = 0;
	if (m_pMovieClip->GetFrameLabels().GetValue(sLabel, uFrame))
	{
		return GotoAndStop(rInterface, uFrame);
	}

	return false;
}

void MovieClipInstance::Pick(
	HitTester& rTester,
	const Matrix2x3& mParent,
	const ColorTransformWithAlpha cxParent,
	Float32 fWorldX,
	Float32 fWorldY,
	Vector< SharedPtr<Falcon::Instance>, MemoryBudgets::UIRuntime>& rv)
{
	// Don't click masks.
	if (0u != GetClipDepth())
	{
		return;
	}

	if (!GetVisible())
	{
		return;
	}

	ColorTransformWithAlpha const cxWorld = (cxParent * GetColorTransformWithAlpha());
	if (cxWorld.m_fMulA == 0.0f)
	{
		return;
	}

	Matrix2x3 const mWorld = (mParent * GetTransform());
	rTester.PushDepth3D(m_fDepth3D, GetIgnoreDepthProjection());
	m_DisplayList.Pick(
		rTester,
		this,
		mWorld,
		cxWorld,
		fWorldX,
		fWorldY,
		rv);
	rTester.PopDepth3D(m_fDepth3D, GetIgnoreDepthProjection());
}

void MovieClipInstance::ReportOnAddToParentIfNeeded(AddInterface& rInterface)
{
	// Only dispatch now if frame 0 construction was
	// already completed. Otherwise, we want
	// frame 0 to be reached so any expected children
	// get created first.
	if (m_iCurrentFrame >= 0 && nullptr != GetParent())
	{
		HString const sClassName(GetMovieClipDefinition()->GetClassName());
		if (!sClassName.IsEmpty() && sClassName != kMovieClip)
		{
			rInterface.FalconOnAddToParent(
				GetParent(),
				this,
				sClassName);
		}
	}
}

void MovieClipInstance::SetReorderChildrenFromDepth3D(Bool b)
{
	m_DisplayList.SetSortByDepth3D(b);
}

void MovieClipInstance::ApplyNonEventFrameActions()
{
	// Cache simple frame actions for the current frame, if there are any.
	SimpleActions::FrameActions const* pFrameActions = m_pMovieClip->GetSimpleActions().m_tFrameActions.Find((UInt16)m_iCurrentFrame);

	// Process stop.
	if (nullptr != pFrameActions && pFrameActions->m_bStop)
	{
		// If we hit a stop, stop playing.
		m_bPlaying = false;
	}

	// Process visible.
	if (nullptr != pFrameActions && pFrameActions->m_eVisibleChange != SimpleActions::kNoVisibleChange)
	{
		SetVisible((pFrameActions->m_eVisibleChange == SimpleActions::kSetVisibleTrue));
	}
}

void MovieClipInstance::CloneTo(AddInterface& rInterface, MovieClipInstance& rClone) const
{
	Instance::CloneTo(rInterface, rClone);
	m_DisplayList.CloneTo(rInterface, &rClone, rClone.m_DisplayList);
	rClone.m_iCurrentFrame = m_iCurrentFrame;
	rClone.m_uOptions = m_uOptions;
}

Bool MovieClipInstance::GotoFrame(AddInterface& rInterface, UInt16 uFrame)
{
	// Cache the frame as a signed int for further processing.
	Int32 iFrame = (Int32)uFrame;

	// Matching Flash behavior - clamp the frame to the last frame.
	iFrame = Min(iFrame, (Int32)m_pMovieClip->GetFrameCount() - 1);

	// Early out for frame already at target frame.
	if (iFrame == m_iCurrentFrame)
	{
		// Apply non-event frame actions after a goto.
		ApplyNonEventFrameActions();

		// Tell the Advance() call that we're after a goto (don't
		// actually advance on the next call, instead, dispath events
		// only).
		m_bAfterGoto = true;

		return true;
	}

	// Reverse, process from m_iCurrentFrame to iFrame + 1
	if (iFrame < m_iCurrentFrame)
	{
		const MovieClipDefinition::DisplayListTags& vDisplayListTags = m_pMovieClip->GetReverseDisplayListTags();
		const MovieClipDefinition::FrameOffsets& vFrameOffsets = m_pMovieClip->GetFrameOffsets();
		Int32 const iStartFrame = m_iCurrentFrame;
		Int32 const iEndFrame = iFrame;

		Int32 const iBegin = (Int32)vFrameOffsets[iStartFrame];
		Int32 const iEnd = (Int32)(vFrameOffsets[iEndFrame] + 1);

		for (Int32 i = iBegin; i >= iEnd; --i)
		{
			vDisplayListTags[i]->Apply(rInterface, this, m_DisplayList);
		}
	}

	// Forward, process from m_iCurrentFrame + 1 to iFrame
	if (iFrame > m_iCurrentFrame)
	{
		const MovieClipDefinition::DisplayListTags& vDisplayListTags = m_pMovieClip->GetDisplayListTags();
		const MovieClipDefinition::FrameOffsets& vFrameOffsets = m_pMovieClip->GetFrameOffsets();
		Int32 const iStartFrame = (m_iCurrentFrame + 1);
		Int32 const iEndFrame = iFrame;

		UInt32 const uBegin = (iStartFrame == 0
			? 0
			: (iStartFrame - 1 < (Int32)vFrameOffsets.GetSize() ? (vFrameOffsets[iStartFrame - 1] + 1) : 0));
		UInt32 const uEnd = (((Int32)vFrameOffsets.GetSize() > iEndFrame)
			? (vFrameOffsets[iEndFrame] + 1)
			: vDisplayListTags.GetSize());

		for (UInt32 i = uBegin; i < uEnd; ++i)
		{
			vDisplayListTags[i]->Apply(rInterface, this, m_DisplayList);
		}
	}

	// Now at the target frame.
	m_iCurrentFrame = iFrame;

	// Apply non-event frame actions after a goto.
	ApplyNonEventFrameActions();

	// Tell the Advance() call that we're after a goto (don't
	// actually advance on the next call, instead, dispath events
	// only).
	m_bAfterGoto = true;

	// Advance children to frame 0 with a GotoFrame(0) as necessary.
	m_DisplayList.AdvanceToFrame0(rInterface);

	return true;
}

} //namespace Falcon

} // namespace Seoul
