/**
 * \file FalconDisplayList.cpp
 * \brief A Falcon DisplayList encapsulates a Flash display list.
 *
 * Display lists are depth-ordered lists of children. A display list
 * is a direct component of a MovieClip.
 *
 * Children in a display list are exactly ordered by their depth value.
 * Depth values do not need to be contiguous (a display list can
 * and usually does contain spares depth values).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconClipper.h"
#include "FalconDisplayList.h"
#include "FalconEditTextInstance.h"
#include "FalconGlobalConfig.h"
#include "FalconMovieClipDefinition.h"
#include "FalconMovieClipInstance.h"
#include "FalconRenderPoser.h"
#include "FalconRenderFeature.h"
#include "FalconRenderState.h"
#include "FalconShapeInstance.h"
#include "FalconTexture.h"

namespace Seoul::Falcon
{

/** Utility, used to order instances based on their depth in parent. */
struct SortInstanceByDepth SEOUL_SEALED
{
	Bool operator()(const SharedPtr<Instance>& pA, const SharedPtr<Instance>& pB) const
	{
		return (pA->GetDepthInParent() < pB->GetDepthInParent());
	}
};

/** Utility, used to order instances based on their 3D depth. */
struct SortInstanceByDepth3D SEOUL_SEALED
{
	Bool operator()(const SharedPtr<Instance>& pA, const SharedPtr<Instance>& pB) const
	{
		return (pA->GetDepth3D() < pB->GetDepth3D());
	}
};

static inline Bool Intersects(
	const Vector2D& vCullExtents,
	const Vector2D& vCullCenter,
	const Matrix2x3& mToWorld,
	const DisplayListCulling::Bounds& bounds)
{
	// Compute valus and difference.
	Vector2D const vObjectCenter(Matrix2x3::TransformPosition(mToWorld, bounds.m_vCenter));
	Vector2D const& vObjectExtents(bounds.m_vExtents);
	Vector2D const vDiff(vCullCenter - vObjectCenter);

	// Transform axes into world space and take
	// absolute value to compute "effective radius".
	Vector2D vAbsXY;
	vAbsXY.X = Abs(mToWorld.M00 * vObjectExtents.X) + Abs(mToWorld.M01 * vObjectExtents.Y);
	vAbsXY.Y = Abs(mToWorld.M10 * vObjectExtents.X) + Abs(mToWorld.M11 * vObjectExtents.Y);

	// Compare effective radius in world space against the cull extents,
	// adjusted by offset difference.
	if ((Abs(vDiff.X) - vAbsXY.X) > vCullExtents.X)
	{
		return false;
	}

	if ((Abs(vDiff.Y) - vAbsXY.Y) > vCullExtents.Y)
	{
		return false;
	}

	return true;
}

// Update culling info, regenerating the list of unculled/reachable
// children.
void DisplayListCulling::Refresh(Render::Poser& rPoser, const Matrix2x3& mParent, const List& vList)
{
	// Clear the list to rebuild.
	m_vList.Clear();

	// Cache the number of input children and the cull rectangle.
	Rectangle const cullRectangle = rPoser.GetState().m_WorldCullRectangle;
	UInt32 const uSize = vList.GetSize();

	// Cache some values we'll use.
	Vector2D const vCullExtents(Vector2D(
		0.5f * cullRectangle.GetWidth(),
		0.5f * cullRectangle.GetHeight()));
	Vector2D const vCullCenter(
		Vector2D(cullRectangle.m_fLeft, cullRectangle.m_fTop) +
		vCullExtents);

	// Clamp the invalidate value as required.
	if (m_uNextCacheInvalidate >= uSize)
	{
		m_uNextCacheInvalidate = 0u;
	}

	for (UInt32 i = 0; i < uSize; ++i)
	{
		// Cache instance and depth.
		Instance* pInstance = vList[i].GetPtr();
		UInt16 const uDepth = pInstance->GetDepthInParent();

		// TODO: Reasonable but may be surprising in
		// some use cases.

		// Get the cached local bounds of the child - regenerate
		// if not available, or if this is the entry to invalidate
		// during this pass (in order to keep the cache fresh).
		Bounds localBounds;
		if (m_uNextCacheInvalidate == i ||
			!m_tLocalBoundsCache.GetValue(uDepth, localBounds))
		{
			Rectangle boundsRectangle;
			if (!pInstance->ComputeLocalBounds(boundsRectangle))
			{
				// No bounds, add to the list and continue
				// immediately.
				m_vList.PushBack(vList[i]);
				continue;
			}

			localBounds.m_vCenter = boundsRectangle.GetCenter();
			localBounds.m_vExtents = Vector2D(0.5f * boundsRectangle.GetWidth(), 0.5f * boundsRectangle.GetHeight());
			SEOUL_VERIFY(m_tLocalBoundsCache.Overwrite(uDepth, localBounds).Second);
		}

		// Get the full transform to convert the local bounds into world space.
		Matrix2x3 const mChildWorld = (mParent * pInstance->GetTransform());

		// Check if the child is within the cull rectangle.
		Bool const bIntersects = Intersects(vCullExtents, vCullCenter, mChildWorld, localBounds);

		// If within the cull rectangle, add to the unculled list.
		if (bIntersects)
		{
			m_vList.PushBack(vList[i]);
		}
	}

	// Increment the invalid field.
	m_uNextCacheInvalidate++;
}

#if SEOUL_ENABLE_CHEATS
typedef HashSet< SharedPtr<MovieClipInstance>, MemoryBudgets::UIData > InputWhitelist;

static void DrawInputVisualization(
	const InputWhitelist& inputWhitelist,
	RGBA color,
	Render::Poser& rPoser,
	const Matrix2x3& mParent,
	MovieClipInstance* pParent,
	const SharedPtr<Instance>& p)
{
	// Don't include unless inputWhitelist is empty or
	// the parent is in the whitelist.
	if (!inputWhitelist.IsEmpty() &&
		!inputWhitelist.HasKey(SharedPtr<MovieClipInstance>(pParent)))
	{
		return;
	}

	// Draw.
	p->PoseInputVisualization(rPoser, mParent, color);
}

static void PoseInputVisualization(
	const InputWhitelist& inputWhitelist,
	RGBA color,
	UInt8 uInputMask,
	Render::Poser& rPoser,
	const Matrix2x3& mParent,
	const ColorTransformWithAlpha cxParent,
	Bool bHitTestSelf,
	Bool bHitTestChildren,
	Bool bExactHitTest,
	const SharedPtr<Instance>& p)
{
	if (bHitTestChildren && p->GetType() == InstanceType::kMovieClip)
	{
		((MovieClipInstance*)p.GetPtr())->PoseInputVisualizationChildren(
			inputWhitelist,
			uInputMask,
			rPoser,
			mParent,
			cxParent);
	}

	if (bHitTestSelf)
	{
		if (p->GetType() != InstanceType::kMovieClip)
		{
			DrawInputVisualization(inputWhitelist, color, rPoser, mParent, p->GetParent(), p);
		}
	}
}

static void PoseInputVisualization(
	const InputWhitelist& inputWhitelist,
	RGBA color,
	UInt8 uInputMask,
	Render::Poser& rPoser,
	const Matrix2x3& mParent,
	const ColorTransformWithAlpha cxParent,
	Bool bHitTestSelf,
	Bool bHitTestChildren,
	Bool bExactHitTest,
	const DisplayList::List& vList,
	Int32 iBegin,
	Int32 iEnd)
{
	// Look for the first mask - if none found, we draw our entire range from back to front
	// (which is actually the render order from front to back).
	Int32 iMaskBegin = iEnd;
	Int32 iMaskEnd = 0;
	for (auto iMask = iBegin; iMask < iEnd; ++iMask)
	{
		auto const& pMask = vList[iMask];
		auto const uClipDepth = pMask->GetClipDepth();

		// Clip depth of 0 means the shape is not a mask, so
		// continue on to the next shape.
		if (0 == uClipDepth)
		{
			continue;
		}

		// Found a mask, need to find the mask end now
		// (the end is the first shape which has a depth
		// greater than the mask's clip depth).
		iMaskBegin = iMask;
		iMaskEnd = iMaskBegin + 1;
		for (; iMaskEnd < iEnd; ++iMaskEnd)
		{
			auto const& p = vList[iMaskEnd];
			if (p->GetDepthInParent() > uClipDepth)
			{
				// Found the first shape that is
				// not affected by the mask,
				// which is the mask end.
				break;
			}
		}

		// Done with the loop if we get here,
		// we found the start of the first mask
		// and its corresponding end.
		break;
	}

	// If we found a mask, recursively process the
	// area after the mask first, then process the
	// mask region.
	if (iMaskEnd > 0)
	{
		// Region after the mask, which
		// may include other masks.
		if (iMaskEnd < iEnd)
		{
			PoseInputVisualization(
				inputWhitelist,
				color,
				uInputMask,
				rPoser,
				mParent,
				cxParent,
				bHitTestSelf,
				bHitTestChildren,
				bExactHitTest,
				vList,
				iMaskEnd,
				iEnd);
		}

		// Region inside the mask region.
		if (iMaskBegin + 1 < iMaskEnd)
		{
			auto const& pMaskShape = vList[iMaskBegin];
			Bool const bScissor = pMaskShape->GetScissorClip();

			Bool bDraw = false;
			if (bScissor)
			{
				Rectangle rect;
				if (pMaskShape->ComputeLocalBounds(rect))
				{
					rect = TransformRectangle(pMaskShape->ComputeWorldTransform(), rect);
					rPoser.BeginScissorClip(rect);
					bDraw = true;
				}
			}
			else
			{
				pMaskShape->ComputeMask(
					mParent,
					cxParent,
					rPoser);

				// Start masking.
				bDraw = rPoser.ClipStackPush();
			}

			// Start masking.
			if (bDraw)
			{
				// Recursively process the range of the mask.
				PoseInputVisualization(
					inputWhitelist,
					color,
					uInputMask,
					rPoser,
					mParent,
					cxParent,
					bHitTestSelf,
					bHitTestChildren,
					bExactHitTest,
					vList,
					iMaskBegin + 1,
					iMaskEnd);

				// Complete masking.
				if (bScissor)
				{
					rPoser.EndScissorClip();
				}
				else
				{
					rPoser.ClipStackPop();
				}
			}
		}
	}

	// Finally, draw end to begin (which renders front-to-back)
	// any remaining elements prior to the mask begin, which
	// may be the entire list if no masks were found.
	for (auto i = iMaskBegin - 1; i >= iBegin; --i)
	{
		auto const& p = vList[i];
		PoseInputVisualization(
			inputWhitelist,
			color,
			uInputMask,
			rPoser,
			mParent,
			cxParent,
			bHitTestSelf,
			bHitTestChildren,
			bExactHitTest,
			p);
	}
}

/** Developer only cheat, renders input testable shapes. */
void DisplayList::PoseInputVisualization(
	const InputWhitelist& inputWhitelist,
	RGBA color,
	UInt8 uInputMask,
	Render::Poser& rPoser,
	const Matrix2x3& mParent,
	const ColorTransformWithAlpha cxParent,
	Bool bHitTestSelf,
	Bool bHitTestChildren,
	Bool bExactHitTest)
{
	MaintainList();

	// Refresh culling info if enabled.
	if (nullptr != m_pCulling)
	{
		m_pCulling->Refresh(rPoser, mParent, m_vList);
	}

	// Unlike normal rendering, input visualization draws front-to-back, so
	// we need to iterate over this list in reverse order.
	const List& vList = (nullptr != m_pCulling ? m_pCulling->GetList() : m_vList);
	Int32 const iSize = (Int32)vList.GetSize();

	Seoul::Falcon::PoseInputVisualization(
		inputWhitelist,
		color,
		uInputMask,
		rPoser,
		mParent,
		cxParent,
		bHitTestSelf,
		bHitTestChildren,
		bExactHitTest,
		vList,
		0,
		iSize);
}
#endif // /#if SEOUL_ENABLE_CHEATS

void DisplayList::FindBestCullNode(
	MovieClipInstance* pOwner,
	UInt32 uSearchDepth,
	MovieClipInstance*& rpCurrentCullInstance,
	MovieClipInstance*& rpBestInstance,
	UInt32& ruBestCount) const
{
	const AutoCullingConfig& config = g_Config.m_AutoCullingConfig;

	// Set the cull instance if we are one.
	if (nullptr != m_pCulling)
	{
		rpCurrentCullInstance = pOwner;
	}

	// Cache child count - if greater than current best,
	// we are the new best.
	UInt32 const uSize = m_vList.GetSize();
	if (uSize > ruBestCount)
	{
		rpBestInstance = pOwner;
		ruBestCount = uSize;
	}

	// Stop recursion if we have more children than the threshold.
	if (uSize > config.m_uMaxChildCountForTraversal)
	{
		return;
	}

	// Done if we're about to exceed the max traversal depth.
	if (uSearchDepth >= config.m_uMaxTraversalDepth)
	{
		return;
	}

	// Otherwise, iterate and recurse on MovieClips.
	for (UInt32 i = 0; i < uSize; ++i)
	{
		Instance* pInstance = m_vList[i].GetPtr();
		if (pInstance->GetType() == InstanceType::kMovieClip)
		{
			MovieClipInstance* pChildOwner = ((MovieClipInstance*)pInstance);
			pChildOwner->FindBestCullNode(
				uSearchDepth + 1,
				rpCurrentCullInstance,
				rpBestInstance,
				ruBestCount);
		}
	}
}

Bool DisplayList::ComputeBounds(Rectangle& rBounds)
{
	MaintainList();

	List::SizeType const zSize = m_vList.GetSize();
	if (0 == zSize)
	{
		return false;
	}

	List::SizeType i = 0;
	// This loop goes over the children until it finds one with bounds (ComputeBounds returns true).
	for (; i < zSize; ++i)
	{
		Instance* p = m_vList[i].GetPtr();
		// Once we find at least one child with bounds, we can just go through the rest and accumulate freely
		// from this point, the outer loop is basically done because we're going to return true instead of ever
		// going back to it, the rest of the children are traversed by this inner loop.
		if (p->ComputeBounds(rBounds))
		{
			++i;
			for (; i < zSize; ++i)
			{
				p = m_vList[i].GetPtr();

				Rectangle bounds;
				if (p->ComputeBounds(bounds))
				{
					rBounds = Rectangle::Merge(rBounds, bounds);
				}
			}

			return true;
		}
	}

	return false;
}

Bool DisplayList::ComputeHitTestableLocalBounds(Rectangle& rBounds, Bool bHitTestSelf, Bool bHitTestChildren, UInt8 uHitTestMask)
{
	MaintainList();

	List::SizeType const zSize = m_vList.GetSize();
	if (0 == zSize)
	{
		return false;
	}

	List::SizeType i = 0;
	Bool bChildWithBoundsFound = false;
	for (; i < zSize; ++i)
	{
		Instance* p = m_vList[i].GetPtr();
		if (p->GetType() == InstanceType::kMovieClip) // If the child is a MovieClip and we want to test our children, recurse
		{
			if (bHitTestChildren)
			{
				Rectangle bounds;
				if (((MovieClipInstance*)p)->ComputeHitTestableLocalBounds(bounds, uHitTestMask))
				{
					bChildWithBoundsFound = true;
					bounds = TransformRectangle(p->GetTransform(), bounds);
					rBounds = Rectangle::Merge(rBounds, bounds);
				}
			}
		}
		else // This child is not a MovieClip, if we are hit testing ourself, accumulate it's bounds
		{
			if (bHitTestSelf)
			{
				Rectangle bounds;
				if (p->ComputeBounds(bounds))
				{
					bChildWithBoundsFound = true;
					rBounds = Rectangle::Merge(rBounds, bounds);
				}
			}
		}
	}

	return bChildWithBoundsFound;
}

HitTestResult DisplayList::ExactHitTest(
	HitTester& rTester,
	MovieClipInstance* pOwner,
	UInt8 uSelfMask,
	UInt8 uChildrenMask,
	const Matrix2x3& mParent,
	Float32 fWorldX,
	Float32 fWorldY,
	SharedPtr<MovieClipInstance>& rp,
	SharedPtr<Instance>& rpLeafInstance,
	Bool bHitOwner,
	Bool bHitChildren)
{
	MaintainList();

	const List& vList = (nullptr != m_pCulling ? m_pCulling->GetList() : m_vList);
	Int32 const iSize = (Int32)vList.GetSize();
	for (Int32 i = (iSize - 1); i >= 0; )
	{
		// First, check for a hit.
		HitTestResult const eResult = DoExactHitTest(
			rTester,
			i,
			pOwner,
			uSelfMask,
			uChildrenMask,
			mParent,
			fWorldX,
			fWorldY,
			rp,
			rpLeafInstance,
			bHitOwner,
			bHitChildren);

		if (HitTestResult::kNoHit == eResult)
		{
			--i;
			continue;
		}

		UInt16 const uCandidateDepth = vList[i]->GetDepthInParent();

		// TODO: This can be optimized if we knew of masks ahead of time:
		// - could maintain a separate list in DisplayList, smaller O(n).
		// - could maintain a mask count, and if it's 0, skip this step.

		// Now, check for masking.
		Bool bDone = true;
		--i;
		for (; i >= 0; --i)
		{
			const SharedPtr<Instance>& p = vList[i];
			UInt16 const uClipDepth = p->GetClipDepth();
			if (uClipDepth >= uCandidateDepth)
			{
				if (p->GetType() == InstanceType::kMovieClip)
				{
					if (!((MovieClipInstance*)p.GetPtr())->MaskHitTest(
						rTester,
						mParent,
						fWorldX,
						fWorldY))
					{
						bDone = false;
						break;
					}
				}
				else
				{
					auto const v = rTester.InverseDepthProject(fWorldX, fWorldY);
					if (!p->ExactHitTest(mParent, v.X, v.Y))
					{
						bDone = false;
						break;
					}
				}
			}
		}

		if (bDone)
		{
			return eResult;
		}
	}

	return HitTestResult::kNoHit;
}

HitTestResult DisplayList::HitTest(
	HitTester& rTester,
	MovieClipInstance* pOwner,
	UInt8 uSelfMask,
	UInt8 uChildrenMask,
	const Matrix2x3& mParent,
	Float32 fWorldX,
	Float32 fWorldY,
	SharedPtr<MovieClipInstance>& rp,
	SharedPtr<Instance>& rpLeafInstance,
	Bool bHitOwner,
	Bool bHitChildren)
{
	MaintainList();

	const List& vList = (nullptr != m_pCulling ? m_pCulling->GetList() : m_vList);
	Int32 const iSize = (Int32)vList.GetSize();
	for (Int32 i = (iSize - 1); i >= 0; )
	{
		// First, check for a hit.
		HitTestResult const eResult = DoHitTest(
			rTester,
			i,
			pOwner,
			uSelfMask,
			uChildrenMask,
			mParent,
			fWorldX,
			fWorldY,
			rp,
			rpLeafInstance,
			bHitOwner,
			bHitChildren);

		if (HitTestResult::kNoHit == eResult)
		{
			--i;
			continue;
		}

		UInt16 const uCandidateDepth = vList[i]->GetDepthInParent();

		// TODO: This can be optimized if we knew of masks ahead of time:
		// - could maintain a separate list in DisplayList, smaller O(n).
		// - could maintain a mask count, and if it's 0, skip this step.

		// Now, check for masking.
		Bool bDone = true;
		--i;
		for (; i >= 0; --i)
		{
			const SharedPtr<Instance>& p = vList[i];
			UInt16 const uClipDepth = p->GetClipDepth();
			if (uClipDepth > uCandidateDepth)
			{
				if (p->GetType() == InstanceType::kMovieClip)
				{
					if (!((MovieClipInstance*)p.GetPtr())->MaskHitTest(
						rTester,
						mParent,
						fWorldX,
						fWorldY))
					{
						bDone = false;
						break;
					}
				}
				else
				{
					auto const v = rTester.InverseDepthProject(fWorldX, fWorldY);
					if (!p->ExactHitTest(mParent, v.X, v.Y))
					{
						bDone = false;
						break;
					}
				}
			}
		}

		if (bDone)
		{
			return eResult;
		}
	}

	return HitTestResult::kNoHit;
}

/**
 * Convenience utility, shifts all children depth by 1.
 * Typically useful to insert a child at the back of the
 * drawing order (at depth 1).
 *
 * @return The maximum depth + 1 after shifting. This
 * is the depth at which a new element can be added
 * to be in front of all existing elements.
 */
UInt16 DisplayList::IncreaseAllChildDepthByOne()
{
	// Make sure the current state is fresh.
	MaintainList();

	// First, increase all the depths.
	Int32 const iSize = (Int32)m_vList.GetSize();
	for (Int32 i = (iSize - 1); i >= 0; --i)
	{
		m_vList[i]->m_uDepthInParent++;
	}

	// Now refresh all the lookup structures.
	NameToDepth tNameToDepth; tNameToDepth.Swap(m_tNameToDepth);
	DepthToName tDepthToName; tDepthToName.Swap(m_tDepthToName);
	Table tTable; tTable.Swap(m_tTable);

	for (auto e : tNameToDepth) { SEOUL_VERIFY(m_tNameToDepth.Insert(e.First, e.Second + 1).Second); }
	for (auto e : tDepthToName) { SEOUL_VERIFY(m_tDepthToName.Insert(e.First + 1, e.Second).Second); }
	for (auto e : tTable) { SEOUL_VERIFY(m_tTable.Insert(e.First + 1, e.Second).Second); }

	// Return the next depth - 1 if empty, or the last entry + 1
	// otherwise.
	return (0 == iSize ? 1 : m_vList.Back()->m_uDepthInParent + 1);
}

Bool DisplayList::MaskHitTest(
	HitTester& rTester,
	const Matrix2x3& mParent,
	Float32 fWorldX,
	Float32 fWorldY)
{
	MaintainList();

	const List& vList = (nullptr != m_pCulling ? m_pCulling->GetList() : m_vList);
	Int32 const iSize = (Int32)vList.GetSize();
	for (Int32 i = (iSize - 1); i >= 0; --i)
	{
		const SharedPtr<Instance>& p = vList[i];
		if (p->GetType() == InstanceType::kMovieClip)
		{
			if (((MovieClipInstance*)p.GetPtr())->MaskHitTest(
				rTester,
				mParent,
				fWorldX,
				fWorldY))
			{
				return true;
			}
		}
		else
		{
			auto const v = rTester.InverseDepthProject(fWorldX, fWorldY);
			if (p->ExactHitTest(mParent, v.X, v.Y))
			{
				return true;
			}
		}
	}

	return false;
}

void DisplayList::Pick(
	HitTester& rTester,
	MovieClipInstance* pOwner,
	const Matrix2x3& mParent,
	const ColorTransformWithAlpha cxParent,
	Float32 fWorldX,
	Float32 fWorldY,
	Vector< SharedPtr<Falcon::Instance>, MemoryBudgets::UIRuntime>& rv)
{
	MaintainList();

	const List& vList = (nullptr != m_pCulling ? m_pCulling->GetList() : m_vList);
	auto const vTest = rTester.InverseDepthProject(fWorldX, fWorldY);

	// First apply masking - walk the entire a list and find the maximum
	// depth that masks the world X and Y positions.
	//
	// Since we're gathering all hits, we can walk this list front to
	// back and just push/pop the clip depths as they are encountered.
	Vector<UInt16, MemoryBudgets::UIRuntime> vClipDepths;
	for (auto const& p : vList)
	{
		// If we've hit the depth >= the current clip, pop it.
		while (!vClipDepths.IsEmpty() && p->GetDepthInParent() >= vClipDepths.Back())
		{
			vClipDepths.PopBack();
		}

		// Clipped.
		if (!vClipDepths.IsEmpty() && vClipDepths.Back() > p->GetDepthInParent())
		{
			continue;
		}

		// If a clip shape, check if the pick point is culled - if so, we immediately
		// add the clip shape as a blocker.
		if (p->GetClipDepth() != 0u)
		{
			// Now actual check for masking.
			if (p->GetType() == InstanceType::kMovieClip)
			{
				if (!((MovieClipInstance*)p.GetPtr())->MaskHitTest(
					rTester,
					mParent,
					fWorldX,
					fWorldY))
				{
					vClipDepths.PushBack(p->GetClipDepth());
					continue;
				}
			}
			else if (!p->ExactHitTest(mParent, vTest.X, vTest.Y))
			{
				vClipDepths.PushBack(p->GetClipDepth());
				continue;
			}
		}

		// TODO: Reconsider - we don't consider the alpha
		// to match Flash behavior (see below). I've never double checked what
		// happens if you (just) set the visibility of a mask to
		// false and logically it makes sense for visibility and alpha==0.0
		// to have the same behavior (or, in other words, visibility should
		// possibly not be considered here).

		// Invisible shape, skip.
		if (!p->GetVisible())
		{
			// If a mask and not visible, this is equivalent to
			// a failed outside check above - so add as a clip shape.
			if (p->GetClipDepth() != 0u)
			{
				vClipDepths.PushBack(p->GetClipDepth());
				continue;
			}
		}

		// Invisible shape, possibly skip.
		ColorTransformWithAlpha const cxChild = (cxParent * p->GetColorTransformWithAlpha());
		if (cxChild.m_fMulA == 0.0f)
		{
			// If not a mask, skip.
			if (p->GetClipDepth() == 0u)
			{
				continue;
			}
			// Unlike many code paths, alpha == 0.0 is not considered here. Flash
			// does not hide the mask (or the shapes it reveals) if the cumulative
			// alpha at that mask is 0.0.
		}

		// Finally, check for a hit if not a mask.
		if (p->GetClipDepth() == 0u)
		{
			if (p->GetType() == InstanceType::kMovieClip)
			{
				((MovieClipInstance*)p.GetPtr())->Pick(
					rTester,
					mParent,
					cxParent,
					fWorldX,
					fWorldY,
					rv);
			}
			else
			{
				auto const v = rTester.InverseDepthProject(fWorldX, fWorldY);
				if (p->ExactHitTest(mParent, v.X, v.Y))
				{
					// Add.
					rv.PushBack(p);
				}
			}
		}
	}
}

// Like remove all, but traverses children first.
// Effectively, completely dismantes the tree
// of children from this DisplayList.
void DisplayList::RemoveAllRecursive()
{
	for (auto const& p : m_vList)
	{
		if (p->GetType() == InstanceType::kMovieClip)
		{
			((MovieClipInstance*)p.GetPtr())->RemoveAllChildrenRecursive();
		}
	}

	RemoveAll();
}

void DisplayList::ReorderFromDepth3D()
{
	// Sort by 3D depth.
	SortInstanceByDepth3D sortInstanceByDepth;
	QuickSort(m_vList.Begin(), m_vList.End(), sortInstanceByDepth);

	// Nothing else needs to be updated - we
	// use m_bSortByDepth3D to track that children
	// of this node will never be sorted by their
	// depth value, only by their 3D depth value.

	// Fully sorted now.
	m_bListNeedsSort = false;
}

void DisplayList::SetAtDepth(
	AddInterface& rInterface,
	MovieClipInstance* pOwner,
	UInt16 uDepth,
	const SharedPtr<Instance>& p)
{
	if (nullptr != m_pCulling)
	{
		// TODO: Probably, the fully correct behavior here is
		// to remove the node from uDepth from m_pCulling's list here,
		// and then add p if it is not culled, although this would require
		// access to the renderer's world culling region.

		m_pCulling->UncacheLocalBounds(uDepth);
	}

	if (nullptr != p->m_pParent)
	{
		// NOTE: Technically, we could skip this operation if
		// m_pParent == pOwner, and m_uDepthInParent == uDepth already,
		// but we want to perform a remove in this case so that the
		// global RemoveFromParent handling is invoked and any dependent
		// client functionality has an opportunity to refresh in response
		// to this change.
		p->m_pParent->RemoveChildAtDepth(p->m_uDepthInParent);
		SEOUL_ASSERT(0 == p->m_uDepthInParent);
		SEOUL_ASSERT(nullptr == p->m_pParent);
	}

	p->m_uDepthInParent = uDepth;
	p->m_pParent = pOwner;

	SharedPtr<Instance>* pp = m_tTable.Find(uDepth);
	if (nullptr == pp)
	{
		SEOUL_VERIFY(m_tTable.Insert(uDepth, p).Second);
		if (!m_bSortByDepth3D && !m_vList.IsEmpty() && m_vList.Back()->GetDepthInParent() > uDepth)
		{
			m_bListNeedsSort = true;
		}
		m_vList.PushBack(p);
	}
	else
	{
		auto i = m_vList.Find(*pp);
		(*i) = p;
		*pp = p;
	}

	// Associate name.
	UpdateName(p->GetName(), uDepth);

	// Send out add events for MovieClips with a class name.
	if (p->GetType() == InstanceType::kMovieClip)
	{
		auto pMovieClip = (MovieClipInstance*)p.GetPtr();
		pMovieClip->ReportOnAddToParentIfNeeded(rInterface);
	}
}

void DisplayList::UpdateName(HString name, UInt16 uDepth)
{
	if (name.IsEmpty())
	{
		HString oldName;
		if (m_tDepthToName.GetValue(uDepth, oldName))
		{
			(void)m_tNameToDepth.Erase(oldName);
			(void)m_tDepthToName.Erase(uDepth);
		}
	}
	else
	{
		SEOUL_VERIFY(m_tNameToDepth.Overwrite(name, uDepth).Second);
		SEOUL_VERIFY(m_tDepthToName.Overwrite(uDepth, name).Second);
	}
}

HitTestResult DisplayList::DoExactHitTest(
	HitTester& rTester,
	Int32 i,
	MovieClipInstance* pOwner,
	UInt8 uSelfMask,
	UInt8 uChildrenMask,
	const Matrix2x3& mParent,
	Float32 fWorldX,
	Float32 fWorldY,
	SharedPtr<MovieClipInstance>& rp,
	SharedPtr<Instance>& rpLeafInstance,
	Bool bHitOwner,
	Bool bHitChildren)
{
	const List& vList = (nullptr != m_pCulling ? m_pCulling->GetList() : m_vList);
	const SharedPtr<Instance>& p(vList[i]);
	if (p->GetType() == InstanceType::kMovieClip)
	{
		if (bHitChildren)
		{
			HitTestResult const eResult = ((MovieClipInstance*)p.GetPtr())->HitTest(
				rTester,
				uChildrenMask,
				mParent,
				fWorldX,
				fWorldY,
				rp,
				rpLeafInstance);

			if (HitTestResult::kNoHit != eResult)
			{
				return eResult;
			}
		}
	}
	else
	{
		// Owner passed input masks, perform a hit test
		// to determine if we want to capture the owner or not.
		if (bHitOwner)
		{
			auto const v = rTester.InverseDepthProject(fWorldX, fWorldY);
			if (p->ExactHitTest(mParent, v.X, v.Y))
			{
				rp.Reset(pOwner);
				rpLeafInstance = p;
				return HitTestResult::kHit;
			}
		}
		// TODO: I believe pOwner is guaranteed to be non-null
		// in practice - make that a precondition and don't check it here.

		// If the owner did not pass mask tests, but it has the bAbsorbOtherInput
		// flag set to true, check if it is a blocker. It prevents
		// other types of input from propagating but is not itself captured.
		else if (pOwner && pOwner->GetAbsorbOtherInput())
		{
			auto const v = rTester.InverseDepthProject(fWorldX, fWorldY);
			if (p->ExactHitTest(mParent, v.X, v.Y))
			{
				rp.Reset();
				rpLeafInstance.Reset();
				return HitTestResult::kNoHitStopTesting;
			}
		}
	}

	return HitTestResult::kNoHit;
}

HitTestResult DisplayList::DoHitTest(
	HitTester& rTester,
	Int32 i,
	MovieClipInstance* pOwner,
	UInt8 uSelfMask,
	UInt8 uChildrenMask,
	const Matrix2x3& mParent,
	Float32 fWorldX,
	Float32 fWorldY,
	SharedPtr<MovieClipInstance>& rp,
	SharedPtr<Instance>& rpLeafInstance,
	Bool bHitOwner,
	Bool bHitChildren)
{
	const List& vList = (nullptr != m_pCulling ? m_pCulling->GetList() : m_vList);
	const SharedPtr<Instance>& p(vList[i]);
	if (p->GetType() == InstanceType::kMovieClip)
	{
		// Children first - we want to hit inner movie clips that are registered
		// for self testing before we hit p via bubble propagation.
		if (bHitChildren)
		{
			HitTestResult const eResult = ((MovieClipInstance*)p.GetPtr())->HitTest(
				rTester,
				uChildrenMask,
				mParent,
				fWorldX,
				fWorldY,
				rp,
				rpLeafInstance);

			if (HitTestResult::kNoHit != eResult)
			{
				return eResult;
			}
		}
	}
	else
	{
		// Owner passed input masks, perform a hit test
		// to determine if we want to capture the owner or not.
		if (bHitOwner)
		{
			auto const v = rTester.InverseDepthProject(fWorldX, fWorldY);
			if (p->HitTest(mParent, v.X, v.Y))
			{
				rp.Reset(pOwner);
				rpLeafInstance = p;
				return HitTestResult::kHit;
			}
		}
		// TODO: I believe pOwner is guaranteed to be non-null
		// in practice - make that a precondition and don't check it here.

		// If the owner did not pass mask tests, but it has the bAbsorbOtherInput
		// flag set to true, check if it is a blocker. It prevents
		// other types of input from propagating but is not itself captured.
		else if (pOwner && pOwner->GetAbsorbOtherInput())
		{
			auto const v = rTester.InverseDepthProject(fWorldX, fWorldY);
			if (p->HitTest(mParent, v.X, v.Y))
			{
				rp.Reset();
				rpLeafInstance.Reset();
				return HitTestResult::kNoHitStopTesting;
			}
		}
	}

	return HitTestResult::kNoHit;
}
void DisplayList::MaintainList()
{
	if (!m_bListNeedsSort)
	{
		return;
	}

	SortInstanceByDepth sortInstanceByDepth;
	QuickSort(m_vList.Begin(), m_vList.End(), sortInstanceByDepth);
	m_bListNeedsSort = false;
}

void DisplayList::Mask(
	List::SizeType& i,
	const SharedPtr<Instance>& pMaskShape,
	Render::Poser& rPoser,
	const Matrix2x3& mParent,
	const ColorTransformWithAlpha& cxParent)
{
	UInt16 const uClipDepth = pMaskShape->GetClipDepth();
	Bool const bScissor = pMaskShape->GetScissorClip();

	const List& vList = (nullptr != m_pCulling ? m_pCulling->GetList() : m_vList);
	List::SizeType const zSize = vList.GetSize();

	Bool bDraw = false;
	if (bScissor)
	{
		Rectangle rect;
		if (pMaskShape->ComputeLocalBounds(rect))
		{
			rect = TransformRectangle(pMaskShape->ComputeWorldTransform(), rect);
			rPoser.BeginScissorClip(rect);
			bDraw = true;
		}
	}
	else
	{
		// Get the mask.
		pMaskShape->ComputeMask(
			mParent,
			cxParent,
			rPoser);

		// Start masking.
		bDraw = rPoser.ClipStackPush();
	}

	++i;

	for (; i < zSize; )
	{
		const SharedPtr<Instance>& p = vList[i];
		UInt16 const uDepth = p->GetDepthInParent();
		if (uDepth > uClipDepth)
		{
			break;
		}

		// Handle the sub shape unless the mask has no shape, in which
		// case nothing that is affected by the mask will be visible.
		if (bDraw)
		{
			if (p->GetClipDepth() != 0)
			{
				Mask(i, p, rPoser, mParent, cxParent);
			}
			else
			{
				p->Pose(rPoser, mParent, cxParent);
				++i;
			}
		}
		// Just skip the element.
		else
		{
			++i;
		}
	}

	// Complete masking.
	if (bDraw)
	{
		if (bScissor)
		{
			rPoser.EndScissorClip();
		}
		else
		{
			rPoser.ClipStackPop();
		}
	}
}

} // namespace Seoul::Falcon
