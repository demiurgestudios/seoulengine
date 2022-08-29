/**
 * \file FalconMovieClipInstance.h
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

#pragma once
#ifndef FALCON_MOVIE_CLIP_INSTANCE_H
#define FALCON_MOVIE_CLIP_INSTANCE_H

#include "FalconDisplayList.h"
#include "FalconInstance.h"
#include "FalconLabelName.h"
#include "HashSet.h"
#include "ReflectionDeclare.h"

namespace Seoul::Falcon
{

// forward declarations
class MovieClipDefinition;

class MovieClipInstance SEOUL_SEALED : public Instance
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(MovieClipInstance);

	MovieClipInstance(const SharedPtr<MovieClipDefinition const>& pMovieClip);
	~MovieClipInstance();

	virtual void Advance(AdvanceInterface& rInterface) SEOUL_OVERRIDE;

	virtual void AdvanceToFrame0(AddInterface& rInterface) SEOUL_OVERRIDE;

	virtual Instance* Clone(AddInterface& rInterface) const SEOUL_OVERRIDE
	{
		MovieClipInstance* pReturn = SEOUL_NEW(MemoryBudgets::Falcon) MovieClipInstance(m_pMovieClip);
		CloneTo(rInterface, *pReturn);
		return pReturn;
	}

	virtual Bool ComputeLocalBounds(Rectangle& rBounds) SEOUL_OVERRIDE;

	Bool ComputeHitTestableLocalBounds(Rectangle& rBounds, UInt8 uHitTestMask);
	Bool ComputeHitTestableBounds(Rectangle& rBounds, UInt8 uHitTestMask);
	Bool ComputeHitTestableWorldBounds(Rectangle& rBounds, UInt8 uHitTestMask);

	virtual void ComputeMask(
		const Matrix2x3& mParent,
		const ColorTransformWithAlpha& cxParent,
		Render::Poser& rPoser) SEOUL_OVERRIDE;

	void DisableCulling()
	{
		m_DisplayList.DisableCulling();
	}

	virtual void Pose(
		Render::Poser& rPoser,
		const Matrix2x3& mParent,
		const ColorTransformWithAlpha& cxParent) SEOUL_OVERRIDE;

	// Developer only feature, traversal for rendering hit testable areas.
#if SEOUL_ENABLE_CHEATS
	typedef HashSet< SharedPtr<Falcon::MovieClipInstance>, MemoryBudgets::UIData > InputWhitelist;
	void PoseInputVisualizationChildren(
		const InputWhitelist& inputWhitelist,
		UInt8 uInputMask,
		Render::Poser& rPoser,
		const Matrix2x3& mParent,
		const ColorTransformWithAlpha cxParent);
#endif // /#if SEOUL_ENABLE_CHEATS

	void EnableCulling()
	{
		m_DisplayList.EnableCulling();
	}

	Bool IsCulling() const { return m_DisplayList.IsCulling(); }

	void FindBestCullNode(
		UInt32 uSearchDepth,
		MovieClipInstance*& rpCurrentCullInstance,
		MovieClipInstance*& rpBestInstance,
		UInt32& ruBestCount) const
	{
		m_DisplayList.FindBestCullNode(
			const_cast<MovieClipInstance*>(this),
			uSearchDepth,
			rpCurrentCullInstance,
			rpBestInstance,
			ruBestCount);
	}

	Bool GetChildAt(Int iIndex, SharedPtr<Instance>& rp) const
	{
		return m_DisplayList.GetAtIndex(iIndex, rp);
	}

	Bool GetChildByName(HString sName, SharedPtr<Instance>& rp) const
	{
		return m_DisplayList.GetByName(sName, rp);
	}

	template <typename T>
	Bool GetChildByName(HString sName, SharedPtr<T>& rp) const
	{
		SharedPtr<Instance> p;
		if (!GetChildByName(sName, p))
		{
			return false;
		}

		if (InstanceTypeOf<T>::Value != p->GetType())
		{
			return false;
		}
		
		rp.Reset(static_cast<T*>(p.GetPtr()));
		return true;
	}

	Bool GetChildByNameFromSubTree(const HString& sName, SharedPtr<Instance>& rp) const
	{
		if (m_DisplayList.GetByName(sName, rp))
		{
			return true;
		}

		DisplayList::List::SizeType const uCount = m_DisplayList.GetCount();
		for (DisplayList::List::SizeType i = 0; i < uCount; ++i)
		{
			SharedPtr<Instance> pInstance;
			if (m_DisplayList.GetAtIndex(i, pInstance) && pInstance->GetType() == InstanceType::kMovieClip)
			{
				if (((MovieClipInstance*)pInstance.GetPtr())->GetChildByNameFromSubTree(sName, rp))
				{
					return true;
				}
			}
		}

		return false;
	}

	template <typename T>
	Bool GetChildByNameFromSubTree(HString sName, SharedPtr<T>& rp) const
	{
		SharedPtr<Instance> p;
		if (!GetChildByNameFromSubTree(sName, p))
		{
			return false;
		}

		if (InstanceTypeOf<T>::Value != p->GetType())
		{
			return false;
		}

		rp.Reset(static_cast<T*>(p.GetPtr()));
		return true;
	}

	UInt32 GetChildCount() const
	{
		return m_DisplayList.GetCount();
	}

	Bool GetChildNameAtDepth(UInt16 uDepth, HString& rsName) const
	{
		return m_DisplayList.GetNameAtDepth(uDepth, rsName);
	}

	Int32 GetCurrentFrame() const
	{
		return m_iCurrentFrame;
	}

	LabelName GetCurrentLabel() const;

	Bool GetExactHitTest() const
	{
		return m_bExactHitTest;
	}

	UInt8 GetHitTestChildrenMask() const
	{
		return m_uHitTestChildrenMask;
	}

	UInt8 GetHitTestSelfMask() const
	{
		return m_uHitTestSelfMask;
	}

	HString GetClassName() const;

	const SharedPtr<MovieClipDefinition const>& GetMovieClipDefinition() const
	{
		return m_pMovieClip;
	}

	Bool GetAbsorbOtherInput() const
	{
		return m_bAbsorbOtherInput;
	}

	Bool GetAutoCulling() const
	{
		return m_bAutoCulling;
	}

	Bool GetAutoDepth3D() const
	{
		return m_bAutoDepth3D;
	}

	bool GetDeferDrawing() const
	{
		return m_bDeferDrawing;
	}

	Bool GetInputActionDisabled() const
	{
		return m_bInputActionDisabled;
	}

	UInt32 GetTotalFrames() const;

	virtual InstanceType GetType() const SEOUL_OVERRIDE
	{
		return InstanceType::kMovieClip;
	}

	Bool GotoAndPlay(AddInterface& rInterface, UInt16 uFrame)
	{
		if (GotoFrame(rInterface, uFrame))
		{
			m_bPlaying = true;
			return true;
		}

		return false;
	}

	Bool GotoAndPlayByLabel(AddInterface& rInterface, const LabelName& sLabel);

	Bool GotoAndStop(AddInterface& rInterface, UInt16 uFrame)
	{
		if (GotoFrame(rInterface, uFrame))
		{
			m_bPlaying = false;
			return true;
		}

		return false;
	}

	Bool GotoAndStopByLabel(AddInterface& rInterface, const LabelName& sLabel);

	Bool HasChildAtDepth(UInt16 uDepth)
	{
		return m_DisplayList.HasAtDepth(uDepth);
	}

	virtual Bool HitTest(
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY,
		Bool bIgnoreVisibility = false) const SEOUL_OVERRIDE
	{
		return false;
	}

	HitTestResult HitTest(
		HitTester& rTester,
		UInt8 uMask,
		Float32 fWorldX,
		Float32 fWorldY,
		SharedPtr<MovieClipInstance>& rp,
		SharedPtr<Instance>& rpLeafInstance)
	{
		Matrix2x3 const mParentTransform = (GetParent()
			? GetParent()->ComputeWorldTransform()
			: Matrix2x3::Identity());

		return HitTest(
			rTester,
			uMask,
			mParentTransform,
			fWorldX,
			fWorldY,
			rp,
			rpLeafInstance);
	}

	HitTestResult HitTest(
		HitTester& rTester,
		UInt8 uMask,
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY,
		SharedPtr<MovieClipInstance>& rp,
		SharedPtr<Instance>& rpLeafInstance)
	{
		if (!GetVisible())
		{
			return HitTestResult::kNoHit;
		}

		UInt8 const uSelfMask = (uMask & m_uHitTestSelfMask);
		Bool const bHitTestSelf = (0 != uSelfMask);
		UInt8 const uChildrenMask = (uMask & m_uHitTestChildrenMask);
		Bool const bHitTestChildren = (0 != uChildrenMask);
		Matrix2x3 const mWorld = (mParent * GetTransform());

		HitTestResult eResult = HitTestResult::kNoHit;
		rTester.PushDepth3D(m_fDepth3D, GetIgnoreDepthProjection());
		if (m_bExactHitTest)
		{
			eResult = m_DisplayList.ExactHitTest(rTester, this, uSelfMask, uChildrenMask, mWorld, fWorldX, fWorldY, rp, rpLeafInstance, bHitTestSelf, bHitTestChildren);
		}
		else
		{
			eResult = m_DisplayList.HitTest(rTester, this, uSelfMask, uChildrenMask, mWorld, fWorldX, fWorldY, rp, rpLeafInstance, bHitTestSelf, bHitTestChildren);
		}
		rTester.PopDepth3D(m_fDepth3D, GetIgnoreDepthProjection());
		return eResult;
	}

	// Convenience utility, shifts all children depth by 1.
	// Typically useful to insert a child at the back of the
	// drawing order (at depth 1).
	UInt16 IncreaseAllChildDepthByOne()
	{
		return m_DisplayList.IncreaseAllChildDepthByOne();
	}

	Bool IsPlaying() const
	{
		return m_bPlaying;
	}

	Bool MaskHitTest(
		HitTester& rTester,
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY)
	{
		if (!GetVisible())
		{
			return false;
		}

		Matrix2x3 const mWorld = (mParent * GetTransform());
		return m_DisplayList.MaskHitTest(rTester, mWorld, fWorldX, fWorldY);
	}

	void Pick(
		HitTester& rTester,
		const Matrix2x3& mParent,
		const ColorTransformWithAlpha cxParent,
		Float32 fWorldX,
		Float32 fWorldY,
		Vector< SharedPtr<Falcon::Instance>, MemoryBudgets::UIRuntime>& rv);

	void Play()
	{
		m_bPlaying = true;
	}

	void RemoveAllChildren()
	{
		m_DisplayList.RemoveAll();
	}

	void RemoveAllChildrenRecursive()
	{
		m_DisplayList.RemoveAllRecursive();
	}

	Bool RemoveChildAt(DisplayList::List::SizeType uIndex)
	{
		return m_DisplayList.RemoveAtIndex(uIndex);
	}

	Bool RemoveChildAtDepth(UInt16 uDepth)
	{
		return m_DisplayList.RemoveAtDepth(uDepth);
	}

	Bool RemoveChildByName(const HString& sName)
	{
		return m_DisplayList.RemoveByName(sName);
	}

	void SetAbsorbOtherInput(Bool bAbsorbOtherInput)
	{
		m_bAbsorbOtherInput = bAbsorbOtherInput;
	}

	void SetAutoCulling(Bool bAutoCulling)
	{
		m_bAutoCulling = bAutoCulling;
	}

	void SetAutoDepth3D(Bool bAutoDepth3D)
	{
		m_bAutoDepth3D = bAutoDepth3D;
	}

	void SetDeferDrawing(Bool bSetDeferDrawing)
	{
		m_bDeferDrawing = bSetDeferDrawing;
	}

	void SetChildAtDepth(AddInterface& rInterface, UInt16 uDepth, const SharedPtr<Instance>& p)
	{
		m_DisplayList.SetAtDepth(rInterface, this, uDepth, p);
	}

	void SetEnableEnterFrame(Bool bEnableEnterFrame)
	{
		m_bEnableEnterFrame = bEnableEnterFrame;
	}

	void SetExactHitTest(Bool bExactHitTest)
	{
		m_bExactHitTest = bExactHitTest;
	}

	void SetHitTestChildrenMask(UInt8 uHitTestChildrenMask)
	{
		m_uHitTestChildrenMask = uHitTestChildrenMask;
	}

	void SetHitTestSelfMask(UInt8 uHitTestSelfMask)
	{
		m_uHitTestSelfMask = uHitTestSelfMask;
	}

	void SetInputActionDisabled(Bool bInputActionDisabled)
	{
		m_bInputActionDisabled = bInputActionDisabled;
	}

	void Stop()
	{
		m_bPlaying = false;
	}

	Bool GetCastPlanarShadows() const
	{
		return m_bCastPlanarShadows;
	}

	// TODO: Likely, selectively implementing
	// Depth3D will result in unexpected behavior.
	//
	// We should just push the 3D depth down into Falcon::Instance.
	virtual Float GetDepth3D() const SEOUL_OVERRIDE
	{
		return m_fDepth3D;
	}

	void ReportOnAddToParentIfNeeded(AddInterface& rInterface);

	void SetCastPlanarShadows(Bool bCastPlanarShadows)
	{
		m_bCastPlanarShadows = bCastPlanarShadows;
	}

	// TODO: Likely, selectively implementing
	// Depth3D will result in unexpected behavior.
	//
	// We should just push the 3D depth down into Falcon::Instance.
	virtual void SetDepth3D(Float f) SEOUL_OVERRIDE
	{
		m_fDepth3D = f;
	}

	void SetReorderChildrenFromDepth3D(Bool b);

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(MovieClipInstance);

	void ApplyNonEventFrameActions();
	void CloneTo(AddInterface& rInterface, MovieClipInstance& rClone) const;
	Bool GotoFrame(AddInterface& rInterface, UInt16 uFrame);

	friend class Instance; // For access to m_DisplayList to call UpdateName when necessary.
	DisplayList m_DisplayList;
	SharedPtr<MovieClipDefinition const> m_pMovieClip;
	Int32 m_iCurrentFrame;
	Float32 m_fDepth3D;

	// 32-bits of other members.
	union
	{
		UInt32 m_uOptions;
		struct
		{
			UInt32 m_uHitTestSelfMask : 8;
			UInt32 m_uHitTestChildrenMask : 8;
			UInt32 m_bPlaying : 1;
			UInt32 m_bAfterGoto : 1;
			UInt32 m_bEnableEnterFrame : 1;
			UInt32 m_bExactHitTest : 1;
			UInt32 m_bAbsorbOtherInput : 1;
			UInt32 m_bAutoCulling : 1;
			UInt32 m_bInputActionDisabled : 1;
			UInt32 m_bCastPlanarShadows : 1;
			UInt32 m_bAutoDepth3D : 1;
			UInt32 m_bDeferDrawing : 1;
			UInt32 m_uUnusedReserved : 6;
		};
	};

	Bool InternalComputeHitTestableLocalBounds(Rectangle& rBounds, UInt8 uHitTestMask);
	Bool InternalComputeHitTestableBounds(Rectangle& rBounds, UInt8 uHitTestMask);
	Bool InternalComputeHitTestableWorldBounds(Rectangle& rBounds, UInt8 uHitTestMask);

	SEOUL_DISABLE_COPY(MovieClipInstance);
}; // class MovieClipInstance
template <> struct InstanceTypeOf<MovieClipInstance> { static const InstanceType Value = InstanceType::kMovieClip; };

} // namespace Seoul::Falcon

#endif // include guard
