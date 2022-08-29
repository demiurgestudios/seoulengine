/**
 * \file FalconDisplayList.h
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

#pragma once
#ifndef FALCON_DISPLAY_LIST_H
#define FALCON_DISPLAY_LIST_H

#include "Algorithms.h"
#include "FalconHitTester.h"
#include "FalconInstance.h"
#include "FalconRenderPoser.h"
#include "HashSet.h"
#include "Vector.h"

namespace Seoul::Falcon
{

// forward declarations
class MovieClipInstance;

enum class HitTestResult
{
	// Used in cases where a hit did not occur, but an instance wants to prevent further hit testing.
	kNoHitStopTesting = -1,

	// Used to report no hit occurred.
	kNoHit,

	// Used to report that a hit occurred.
	kHit,
};

// Internal utility, used if a MovieClip has culling enabled.
class DisplayListCulling SEOUL_SEALED
{
public:
	typedef Vector< SharedPtr<Instance>, MemoryBudgets::Falcon > List;

	DisplayListCulling()
		: m_tLocalBoundsCache()
		, m_vList()
		, m_uNextCacheInvalidate(0u)
	{
	}

	~DisplayListCulling()
	{
	}

	const List& GetList() const
	{
		return m_vList;
	}

	void RemoveAll()
	{
		m_tLocalBoundsCache.Clear();
		m_vList.Clear();
	}

	void RemoveAtDepth(UInt16 uDepth)
	{
		UncacheLocalBounds(uDepth);
		UInt32 const uList = m_vList.GetSize();
		for (UInt32 i = 0; i < uList; ++i)
		{
			if (m_vList[i]->GetDepthInParent() == uDepth)
			{
				m_vList.Erase(m_vList.Begin() + i);
				return;
			}
		}
	}

	void UncacheLocalBounds(UInt16 uDepth)
	{
		(void)m_tLocalBoundsCache.Erase(uDepth);
	}

	void Refresh(Render::Poser& rPoser, const Matrix2x3& mParent, const List& vList);

	struct Bounds
	{
		Vector2D m_vCenter;
		Vector2D m_vExtents;
	};

private:
	typedef HashTable<UInt16, Bounds, MemoryBudgets::Falcon> LocalBoundsCache;
	LocalBoundsCache m_tLocalBoundsCache;
	List m_vList;
	UInt32 m_uNextCacheInvalidate;

	SEOUL_DISABLE_COPY(DisplayListCulling);
}; // class DisplayListCulling

class DisplayList SEOUL_SEALED
{
public:
	typedef HashTable< UInt16, SharedPtr<Instance>, MemoryBudgets::Falcon > Table;
	typedef HashTable< HString, UInt16, MemoryBudgets::Falcon > NameToDepth;
	typedef HashTable< UInt16, HString, MemoryBudgets::Falcon > DepthToName;
	typedef Vector< SharedPtr<Instance>, MemoryBudgets::Falcon > List;

	DisplayList()
		: m_tNameToDepth()
		, m_tDepthToName()
		, m_tTable()
		, m_vList()
		, m_pCulling(nullptr)
		, m_bListNeedsSort(false)
		, m_bSortByDepth3D(false)
	{
	}

	~DisplayList()
	{
		SafeDelete(m_pCulling);

		auto const iBegin = m_tTable.Begin();
		auto const iEnd = m_tTable.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			i->Second->m_uDepthInParent = 0;
			i->Second->m_pParent = nullptr;
		}
	}

	void Advance(AdvanceInterface& rInterface)
	{
		MaintainList();

		const List& vList = (nullptr != m_pCulling ? m_pCulling->GetList() : m_vList);
		List::SizeType const zSize = vList.GetSize();
		for (List::SizeType i = 0; i < zSize; ++i)
		{
			vList[i]->Advance(rInterface);
		}
	}

	void AdvanceToFrame0(AddInterface& rInterface)
	{
		MaintainList();

		const List& vList = (nullptr != m_pCulling ? m_pCulling->GetList() : m_vList);
		List::SizeType const zSize = vList.GetSize();
		for (List::SizeType i = 0; i < zSize; ++i)
		{
			vList[i]->AdvanceToFrame0(rInterface);
		}
	}

	void CloneTo(AddInterface& rInterface, MovieClipInstance* pCloneOwner, DisplayList& rClone) const
	{
		// Remove any existing children from clone first.
		rClone.RemoveAll();

		// Copy simple structures through.
		rClone.m_tNameToDepth = m_tNameToDepth;
		rClone.m_tDepthToName = m_tDepthToName;
		rClone.m_bListNeedsSort = m_bListNeedsSort;
		rClone.m_bSortByDepth3D = m_bSortByDepth3D;

		// Blank out the table and list for population.
		{
			List vEmpty;
			rClone.m_vList.Swap(vEmpty);
		}
		{
			Table tEmpty;
			rClone.m_tTable.Swap(tEmpty);
		}

		// Clone Children - enumerate all entries in
		// m_vList, clone it, and then append it to the clone's
		// list and table.
		UInt32 const uSize = m_vList.GetSize();
		rClone.m_vList.Reserve(uSize);
		for (UInt32 i = 0; i < uSize; ++i)
		{
			// Cache the source instance.
			const SharedPtr<Instance>& pSource(m_vList[i]);
			UInt16 const uDepth = pSource->GetDepthInParent();

			// Populate the clone.
			SharedPtr<Instance> pClone(pSource->Clone(rInterface));
			pClone->m_uDepthInParent = uDepth;
			pClone->m_pParent = pCloneOwner;

			// Insert the clone.
			rClone.m_vList.PushBack(pClone);
			SEOUL_VERIFY(rClone.m_tTable.Insert(uDepth, pClone).Second);
		}

		// Disable culling (to flush any existing state) and
		// then enable it, if specified.
		rClone.DisableCulling();
		if (nullptr != m_pCulling)
		{
			rClone.EnableCulling();
		}
	}

	Bool ComputeBounds(Rectangle& rBounds);
	Bool ComputeHitTestableLocalBounds(Rectangle& rBounds, Bool bHitTestSelf, Bool bHitTestChildren, UInt8 uHitTestMask);

	void ComputeMask(
		const Matrix2x3& mParent,
		const ColorTransformWithAlpha& cxParent,
		Render::Poser& rPoser)
	{
		// Make sure the display list is up-to-date.
		MaintainList();

		// Iterate over child shapes and factor in their
		// contribution to the mask.
		const List& vList = (nullptr != m_pCulling ? m_pCulling->GetList() : m_vList);
		List::SizeType const zSize = vList.GetSize();
		for (List::SizeType i = 0; i < zSize; ++i)
		{
			const SharedPtr<Instance>& p = vList[i];
			p->ComputeMask(mParent, cxParent, rPoser);
		}
	}

	void DisableCulling()
	{
		SafeDelete(m_pCulling);
	}

	void Pose(
		Render::Poser& rPoser,
		const Matrix2x3& mParent,
		const ColorTransformWithAlpha& cxParent)
	{
		MaintainList();

		// Refresh culling info if enabled.
		if (nullptr != m_pCulling)
		{
			m_pCulling->Refresh(rPoser, mParent, m_vList);
		}

		const List& vList = (nullptr != m_pCulling ? m_pCulling->GetList() : m_vList);
		List::SizeType const zSize = vList.GetSize();
		for (List::SizeType i = 0; i < zSize; )
		{
			const SharedPtr<Instance>& p = vList[i];
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
	}

	// Developer only feature, traversal for rendering hit testable areas.
#if SEOUL_ENABLE_CHEATS
	typedef HashSet< SharedPtr<Falcon::MovieClipInstance>, MemoryBudgets::UIData > InputWhitelist;
	void PoseInputVisualization(
		const InputWhitelist& inputWhitelist,
		RGBA color,
		UInt8 uInputMask,
		Render::Poser& rPoser,
		const Matrix2x3& mParent,
		const ColorTransformWithAlpha cxParent,
		Bool bHitTestSelf,
		Bool bHitTestChildren,
		Bool bExactHitTest);
#endif // /#if SEOUL_ENABLE_CHEATS

	void EnableCulling()
	{
		if (nullptr == m_pCulling)
		{
			m_pCulling = SEOUL_NEW(MemoryBudgets::Falcon) DisplayListCulling;
		}
	}

	Bool IsCulling() const { return (nullptr != m_pCulling); }

	void FindBestCullNode(
		MovieClipInstance* pOwner,
		UInt32 uSearchDepth,
		MovieClipInstance*& rpCurrentCullInstance,
		MovieClipInstance*& rpBestInstance,
		UInt32& ruBestCount) const;

	Table::SizeType GetCount() const
	{
		return m_tTable.GetSize();
	}

	Bool GetAtDepth(UInt16 uDepth, SharedPtr<Instance>& rp) const
	{
		return m_tTable.GetValue(uDepth, rp);
	}

	Bool GetAtIndex(List::SizeType uIndex, SharedPtr<Instance>& rp) const
	{
		const_cast<DisplayList&>(*this).MaintainList();

		if (uIndex >= m_vList.GetSize())
		{
			return false;
		}

		rp = m_vList[uIndex];
		return true;
	}

	Bool GetByName(const HString& sName, SharedPtr<Instance>& rp) const
	{
		UInt16 uDepth = 0;
		if (!m_tNameToDepth.GetValue(sName, uDepth))
		{
			return false;
		}

		return m_tTable.GetValue(uDepth, rp);
	}

	Bool GetNameAtDepth(UInt16 uDepth, HString& rsName) const
	{
		return m_tDepthToName.GetValue(uDepth, rsName);
	}

	Bool HasAtDepth(UInt16 uDepth) const
	{
		return m_tTable.HasValue(uDepth);
	}

	HitTestResult ExactHitTest(
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
		Bool bHitChildren);

	HitTestResult HitTest(
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
		Bool bHitChildren);

	// Convenience utility, shifts all children depth by 1.
	// Typically useful to insert a child at the back of the
	// drawing order (at depth 1).
	UInt16 IncreaseAllChildDepthByOne();

	Bool MaskHitTest(
		HitTester& rTester,
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY);

	void Pick(
		HitTester& rTester,
		MovieClipInstance* pOwner,
		const Matrix2x3& mParent,
		const ColorTransformWithAlpha cxParent,
		Float32 fWorldX,
		Float32 fWorldY,
		Vector< SharedPtr<Falcon::Instance>, MemoryBudgets::UIRuntime>& rv);

	void RemoveAll()
	{
		{
			auto const iBegin = m_tTable.Begin();
			auto const iEnd = m_tTable.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				i->Second->m_uDepthInParent = 0;
				i->Second->m_pParent = nullptr;
			}
		}

		if (nullptr != m_pCulling)
		{
			m_pCulling->RemoveAll();
		}

		m_tNameToDepth.Clear();
		m_tDepthToName.Clear();
		m_tTable.Clear();
		m_vList.Clear();
		m_bListNeedsSort = false;
	}

	// Like remove all, but traverses children first.
	// Effectively, completely dismantes the tree
	// of children from this DisplayList.
	void RemoveAllRecursive();

	Bool RemoveAtDepth(UInt16 uDepth)
	{
		if (nullptr != m_pCulling)
		{
			m_pCulling->RemoveAtDepth(uDepth);
		}

		SharedPtr<Instance> pInstance;
		if (m_tTable.GetValue(uDepth, pInstance))
		{
			pInstance->m_uDepthInParent = 0;
			pInstance->m_pParent = nullptr;

			SEOUL_VERIFY(m_tTable.Erase(uDepth));
			if (m_vList.Back() == pInstance)
			{
				m_vList.PopBack();
			}
			else
			{
				// TODO: If !m_bListNeedsSort && !m_bSortByDepth3D
				// we can use LowerBound() to find the entry instead of
				// a linear search.

				auto i = m_vList.Find(pInstance);
				m_vList.Erase(i);
			}

			UpdateName(HString(), uDepth);
			return true;
		}

		UpdateName(HString(), uDepth);
		return false;
	}

	Bool RemoveAtIndex(List::SizeType uIndex)
	{
		MaintainList();

		if (uIndex >= m_vList.GetSize())
		{
			return false;
		}

		return RemoveAtDepth(m_vList[uIndex]->GetDepthInParent());
	}

	Bool RemoveByName(const HString& sName)
	{
		UInt16 uDepth = 0;
		if (!m_tNameToDepth.GetValue(sName, uDepth))
		{
			return false;
		}

		return RemoveAtDepth(uDepth);
	}

	void ReorderFromDepth3D();

	void SetAtDepth(
		AddInterface& rInterface,
		MovieClipInstance* pOwner,
		UInt16 uDepth,
		const SharedPtr<Instance>& p);

	Bool GetSortByDepth3D() const
	{
		return m_bSortByDepth3D;
	}

	void SetSortByDepth3D(Bool bDepth3D)
	{
		m_bSortByDepth3D = bDepth3D;
	}

	void UpdateName(HString name, UInt16 uDepth);

private:
	NameToDepth m_tNameToDepth;
	DepthToName m_tDepthToName;
	Table m_tTable;
	List m_vList;
	DisplayListCulling* m_pCulling;
	Bool m_bListNeedsSort;
	Bool m_bSortByDepth3D;

	HitTestResult DoExactHitTest(
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
		Bool bHitChildren);
	HitTestResult DoHitTest(
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
		Bool bHitChildren);

	void MaintainList();

	void Mask(
		List::SizeType& i,
		const SharedPtr<Instance>& pMaskShape,
		Render::Poser& rPoser,
		const Matrix2x3& mParent,
		const ColorTransformWithAlpha& cxParent);

	SEOUL_DISABLE_COPY(DisplayList);
}; // class DisplayList

} // namespace Seoul::Falcon

#endif // include guard
