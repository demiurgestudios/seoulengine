/**
 * \file SpatialTree.h
 * \brief Dynamic spatial query structure. Implements a dynamic
 * binary tree that forms a bounding volume hierarchy of AABBs.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SPATIAL_TREE_H
#define SPATIAL_TREE_H

#include "AABB.h"
#include "Frustum.h"
#include "Prereqs.h"
#include "SpatialId.h"
#include "Vector.h"

namespace Seoul
{

// TODO: Add some logic to progressively update
//               node parents to rebalance an unbalanced tree.
// TODO: Reduce size of SpatialNode.
// TODO: Rename to DynamicSpatialTree, merge TriangleTree from old physics
//               code and convert to a general purpose static tree (use the single index
//               skip trick and a kdTree.

/** Track node in the SpatialTree structure. */
struct SpatialNode SEOUL_SEALED
{
	SpatialNode()
		: m_AABB()
		, m_uParent(0)
		, m_uObject(0)
		, m_uChildA(0)
		, m_uChildB(0)
	{
		Reset();
	}

	/** @return true if the current node is a leaf (contains an object reference). */
	Bool IsLeaf() const
	{
		return (kuInvalidSpatialId != m_uObject);
	}

	/** Clear all state of this Node, except for the AABB. */
	void Reset()
	{
		m_uParent = kuInvalidSpatialId;
		m_uObject = kuInvalidSpatialId;
		m_uChildA = kuInvalidSpatialId;
		m_uChildB = kuInvalidSpatialId;
	}

	AABB m_AABB;
	SpatialId m_uParent;
	SpatialId m_uObject;
	SpatialId m_uChildA;
	SpatialId m_uChildB;
}; // struct SpatialNode
SEOUL_STATIC_ASSERT(sizeof(SpatialNode) == sizeof(AABB) + 8);
template <> struct CanMemCpy<SpatialNode> { static const Bool Value = true; };
template <> struct CanZeroInit<SpatialNode> { static const Bool Value = true; };

/** A dynamic bounding volume hierarchy of AABBs. */
template <typename T>
class SpatialTree SEOUL_SEALED
{
public:
	/** Fixed stack space of Query. Exceeding the space results in a recursive call. */
	static const UInt32 kQueryStackSize = 64u;

	typedef Vector<SpatialId, MemoryBudgets::SpatialSorting> Ids;
	typedef Vector<SpatialNode, MemoryBudgets::SpatialSorting> Nodes;
	typedef Vector<T, MemoryBudgets::SpatialSorting> Objects;

	/**
	 * Construct a spatial tree with uInitialCapacity node storage and
	 * the desired expansion constants.
	 *
	 * Expansion is used to oversize AABBs on insertion. Useful to
	 * minimize the degree of reinsertion in exchange for Query()
	 * accuracy.
	 */
	SpatialTree(UInt32 uInitialCapacity = 0u, Float fAABBExpansion = 0.0f)
		: m_uRoot(kuInvalidSpatialId)
		, m_uFree(kuInvalidSpatialId)
		, m_vNodes()
		, m_fAABBExpansion(fAABBExpansion)
	{
		if (uInitialCapacity > 0u)
		{
			m_vNodes.Reserve(uInitialCapacity);
		}
	}

	~SpatialTree()
	{
	}

	/**
	 * Insert a new object into the tree.
	 *
	 * @return The node id used to store the object.
	 */
	SpatialId Add(const T& object, const AABB& aabb)
	{
		// Allocate a new leaf node.
		SpatialId const uReturn = AllocateNode();

		// Setup the node with the object data.
		SpatialNode& r = GetNode(uReturn);
		r.m_AABB = aabb;
		r.m_AABB.Expand(m_fAABBExpansion);
		r.m_uObject = AddObject(object);

		// Insert the node into the tree.
		AddLeafNode(uReturn);

		return uReturn;
	}

	/** @return The total number of active nodes. For debugging/testing, O(n) of the number of nodes. */
	UInt32 ComputeFreeNodeCount() const
	{
		UInt32 uReturn = 0u;
		SpatialId uNode = m_uFree;
		while (kuInvalidSpatialId != uNode)
		{
			++uReturn;
			uNode = GetNode(uNode).m_uParent;
		}
		return uReturn;
	}

	/** @return The total number of allocate nodes. Not the same as the number of active nodes in the tree. */
	UInt32 GetNodeCapacity() const
	{
		return m_vNodes.GetSize();
	}

	/** Get the tree AABB for the object associated with node uNodeId. */
	const AABB& GetObjectAABB(SpatialId uNodeId) const
	{
		return GetNode(uNodeId).m_AABB;
	}

	/** Get the object associated with node uNodeId. */
	const T& GetObject(SpatialId uNodeId) const
	{
		return m_vObjects[GetNode(uNodeId).m_uObject];
	}

	/**
	 * @return The full list of Objects in this tree.
	 *
	 * Will contain "holes" (objects with their default value that are not actually
	 * members of the tree). As a result, this list is typically only useful when T is
	 * a pointer type.
	 */
	const Objects& GetObjects() const
	{
		return m_vObjects;
	}

	// TODO: Sensible to return Max() in the fallback case?

	/** @return The overall dimensions of the tree. Will be AABB::MaxAABB() if the tree is empty. */
	AABB GetRootAABB() const
	{
		return (kuInvalidSpatialId == m_uRoot
			? AABB::MaxAABB()
			: GetNode(m_uRoot).m_AABB);
	}

	/** Issue a spatial query with an AABB against the tree. */
	template <typename U>
	void Query(U& callback, const AABB& aabb) const
	{
		InnerQuery(callback, aabb, m_uRoot);
	}

	/** Issue a spatial query with a Frustum against the tree. */
	template <typename U>
	void Query(U& callback, const Frustum& frustum) const
	{
		InnerQuery(callback, frustum, m_uRoot);
	}

	/**
	 * Remove a leaf node containing an object from the tree. Must use the node id returned from Add().
	 *
	 * \pre Must call with a valid uNode id.
	 */
	void Remove(SpatialId uNode)
	{
		RemoveObject(GetNode(uNode).m_uObject);
		RemoveLeafNode(uNode);
		ReleaseNode(uNode);
	}

	/**
	 * Update the leaf node's AABB, referenced by uNode in this tree.
	 *
	 * \pre Must call with a valid uNode id.
	 */
	Bool Update(SpatialId uNode, const AABB& aabb)
	{
		SpatialNode& r = GetNode(uNode);
		if (r.m_AABB.Contains(aabb))
		{
			return false;
		}

		// Remove and then reinsert the node.
		RemoveLeafNode(uNode);
		r.m_AABB = aabb;
		r.m_AABB.Expand(m_fAABBExpansion);
		AddLeafNode(uNode);
		return true;
	}

private:
	SpatialId m_uRoot;
	SpatialId m_uFree;
	Nodes m_vNodes;
	Ids m_vFreeObjects;
	Objects m_vObjects;
	Float const m_fAABBExpansion;

	/** Add a new leaf node (contains an object) to the node tree. */
	void AddLeafNode(SpatialId uLeaf)
	{
		// No root yet, insert and return immediately.
		if (kuInvalidSpatialId == m_uRoot)
		{
			m_uRoot = uLeaf;
			GetNode(uLeaf).m_uParent = kuInvalidSpatialId;
			return;
		}

		// Cache the AABB of the leaf node.
		AABB const leafAABB(GetNode(uLeaf).m_AABB);

		// Find the sibling to join to the leaf node.
		SpatialId uSibling = m_uRoot;
		while (!GetNode(uSibling).IsLeaf())
		{
			// Cache the sibling data and compute its surface area.
			const SpatialNode& sibling = GetNode(uSibling);
			Float const fSurfaceArea = sibling.m_AABB.GetSurfaceArea();

			// Compute the surface area of the current node expanded
			// to contain the leaf (this is the case of the current
			// insertion point).
			Float fExpandedSurfaceArea;
			{
				AABB const mergedAABB(AABB::CalculateMerged(sibling.m_AABB, leafAABB));
				fExpandedSurfaceArea = mergedAABB.GetSurfaceArea();
			}

			// Compute the final cost of the current node, and
			// the relative cost, factoring in the removal of existing.
			Float const fCurrentCost = (2.0f * fExpandedSurfaceArea);
			Float const fGrowthCost = (2.0f * (fExpandedSurfaceArea - fSurfaceArea));

			// Compute cost of two children of the sibling.
			SpatialId const uChildA = sibling.m_uChildA;
			Float const fChildACost = (GetInsertionCost(uChildA, leafAABB) + fGrowthCost);
			SpatialId const uChildB = sibling.m_uChildB;
			Float const fChildBCost = (GetInsertionCost(uChildB, leafAABB) + fGrowthCost);

			// Done if the cost of descent into either child is greater than inserting into
			// current.
			if (fChildACost >= fCurrentCost && fChildBCost >= fCurrentCost)
			{
				break;
			}

			// Proceed into the best child.
			uSibling = (fChildACost < fChildBCost ? uChildA : uChildB);
		}

		// Split and insert.
		{
			// Generate a new node - must be done first so we don't
			// invalidate references.
			SpatialId const uNewParent = AllocateNode();
			SpatialNode& sibling = GetNode(uSibling);
			SpatialId const uOldParent = sibling.m_uParent;

			// Setup the new parent.
			SpatialNode& parent = GetNode(uNewParent);
			parent.m_uParent = uOldParent;
			parent.m_uObject = kuInvalidSpatialId;
			parent.m_AABB = AABB::CalculateMerged(leafAABB, sibling.m_AABB);

			// Hook up the new node.
			parent.m_uChildA = uSibling;
			parent.m_uChildB = uLeaf;
			sibling.m_uParent = uNewParent;
			GetNode(uLeaf).m_uParent = uNewParent;

			// Root vs. child handling.
			if (uOldParent == kuInvalidSpatialId)
			{
				// If no previous parent, inserting as root.
				m_uRoot = uNewParent;
			}
			else
			{
				// Reparent.
				SpatialNode& oldParent = GetNode(uOldParent);
				if (oldParent.m_uChildA == uSibling)
				{
					oldParent.m_uChildA = uNewParent;
				}
				else
				{
					oldParent.m_uChildB = uNewParent;
				}
			}
		}

		// Fixup AABBs to the root.
		RecomputeAABBsToRoot(GetNode(uLeaf).m_uParent);
	}

	/** Add a object to our list of tracked objects. */
	SpatialId AddObject(const T& object)
	{
		if (m_vFreeObjects.IsEmpty())
		{
			SEOUL_ASSERT(m_vObjects.GetSize() < kuInvalidSpatialId);
			SpatialId const uReturn = (SpatialId)m_vObjects.GetSize();
			m_vObjects.PushBack(object);
			return uReturn;
		}

		SpatialId const uReturn = m_vFreeObjects.Back();
		m_vFreeObjects.PopBack();
		m_vObjects[uReturn] = object;
		return uReturn;
	}

	/** Generate a new node, useful for any type (leaf or internal). */
	SpatialId AllocateNode()
	{
		// No nodes on the free list, instantiate a new one.
		if (kuInvalidSpatialId == m_uFree)
		{
			// Sanity check.
			SEOUL_ASSERT(m_vNodes.GetSize() < kuInvalidSpatialId);

			SpatialId const uReturn = (SpatialId)m_vNodes.GetSize();
			m_vNodes.PushBack(SpatialNode());
			return uReturn;
		}

		// Reuse a current free node.
		{
			SpatialId const uReturn = m_uFree;
			{
				SpatialNode& r = GetNode(uReturn);
				m_uFree = r.m_uParent;
				r.Reset();
			}

			return uReturn;
		}
	}

	/**
	 * Compute the code of inserting a new node with insertNodeAABB as a sibling
	 * of uNode.
	 */
	Float GetInsertionCost(SpatialId uNode, const AABB& insertNodeAABB) const
	{
		const SpatialNode& node = GetNode(uNode);
		AABB const mergedAABB = AABB::CalculateMerged(insertNodeAABB, node.m_AABB);

		// If the sibling is a leaf, we're about to terminate if we
		// choose it, so include the total size.
		if (node.IsLeaf())
		{
			return mergedAABB.GetSurfaceArea();
		}
		// If the sibling is an inner node, we factor out its area, since
		// we will be traversing into it and considering its subtree,
		// if it is the lowest cost.
		else
		{
			return (mergedAABB.GetSurfaceArea() - node.m_AABB.GetSurfaceArea());
		}
	}

	// Node convenience accessors.
	const SpatialNode& GetNode(SpatialId uNode) const   { return m_vNodes[uNode]; }
	      SpatialNode& GetNode(SpatialId uNode)         { return m_vNodes[uNode]; }

	/**
	 * (Possibly) recursive inner of a Query() call - uses an id stack
	 * on the function stack until it is full, then recurses.
	 */
	template <typename U>
	Bool InnerQuery(U& callback, const AABB& aabb, SpatialId uRoot) const
	{
		FixedArray<SpatialId, kQueryStackSize> aStack;

		// Initial stack population.
		UInt32 uStack = 0u;
		aStack[uStack++] = uRoot;

		// Loop until consumed.
		while (uStack > 0u)
		{
			// Pop the next node - if invalid, skip.
			SpatialId const uNode = aStack[--uStack];
			if (kuInvalidSpatialId == uNode)
			{
				continue;
			}

			// Get the node and check for intersection.
			const SpatialNode& node = GetNode(uNode);
			if (!node.m_AABB.Intersects(aabb))
			{
				continue;
			}

			// If the node is a leaf, invoke the callback on its
			// object - if the callback returns false, it means
			// "stop querying", so return immediately.
			if (node.IsLeaf())
			{
				if (!callback(m_vObjects[node.m_uObject]))
				{
					return false;
				}
			}
			else
			{
				// If we don't have enough id stack space for 2 more ids,
				// push the first child id, then recurse on the second.
				if (uStack + 1 == aStack.GetSize())
				{
					aStack[uStack++] = node.m_uChildA;
					if (!InnerQuery(callback, aabb, node.m_uChildB))
					{
						return false;
					}
				}
				// Otherwise, push both children and iterate.
				else
				{
					aStack[uStack++] = node.m_uChildA;
					aStack[uStack++] = node.m_uChildB;
				}
			}
		}

		return true;
	}

	/**
	 * (Possibly) recursive inner of a Query() call - uses an id stack
	 * on the function stack until it is full, then recurses.
	 */
	template <typename U>
	Bool InnerQuery(U& callback, const Frustum& frustum, SpatialId uRoot) const
	{
		FixedArray<SpatialId, kQueryStackSize> aStack;

		// Initial stack population.
		UInt32 uStack = 0u;
		aStack[uStack++] = uRoot;

		// Loop until consumed.
		while (uStack > 0u)
		{
			// Pop the next node - if invalid, skip.
			SpatialId const uNode = aStack[--uStack];
			if (kuInvalidSpatialId == uNode)
			{
				continue;
			}

			// Get the node and check for intersection.
			const SpatialNode& node = GetNode(uNode);
			if (FrustumTestResult::kDisjoint == frustum.Intersects(node.m_AABB))
			{
				continue;
			}

			// If the node is a leaf, invoke the callback on its
			// object - if the callback returns false, it means
			// "stop querying", so return immediately.
			if (node.IsLeaf())
			{
				if (!callback(m_vObjects[node.m_uObject]))
				{
					return false;
				}
			}
			else
			{
				// If we don't have enough id stack space for 2 more ids,
				// push the first child id, then recurse on the second.
				if (uStack + 1 == aStack.GetSize())
				{
					aStack[uStack++] = node.m_uChildA;
					if (!InnerQuery(callback, frustum, node.m_uChildB))
					{
						return false;
					}
				}
				// Otherwise, push both children and iterate.
				else
				{
					aStack[uStack++] = node.m_uChildA;
					aStack[uStack++] = node.m_uChildB;
				}
			}
		}

		return true;
	}

	/**
	 * On changes to children AABB, this function recurses to root
	 * and recomputes AABBs (O(log n) for a balanced tree, where
	 * n is the total number of nodes in the tree).
	 */
	void RecomputeAABBsToRoot(SpatialId uParent)
	{
		// Loop until we hit the root.
		while (kuInvalidSpatialId != uParent)
		{
			// Get the current parent.
			SpatialNode& parent = GetNode(uParent);

			// Recompute its AABB from its children.
			parent.m_AABB = AABB::CalculateMerged(
				GetNode(parent.m_uChildA).m_AABB,
				GetNode(parent.m_uChildB).m_AABB);

			// Iterate.
			uParent = parent.m_uParent;
		}
	}

	/** Push a node onto the free list. */
	void ReleaseNode(SpatialId uNode)
	{
		GetNode(uNode).m_uParent = m_uFree;
		m_uFree = uNode;
	}

	/** Given a previously created leaf node with AddLeafNode(), remove it from the tree. */
	void RemoveLeafNode(SpatialId uLeaf)
	{
		// The root node is an easy case.
		if (uLeaf == m_uRoot)
		{
			m_uRoot = kuInvalidSpatialId;
			return;
		}

		// Get the parent, and its parent, then find our
		// sibling.
		SpatialId const uParent = GetNode(uLeaf).m_uParent;
		const SpatialNode& parent = GetNode(uParent);
		SpatialId const uParentParent = parent.m_uParent;
		SpatialId const uSibling = (uLeaf == parent.m_uChildA
			? parent.m_uChildB
			: parent.m_uChildA);

		// If our parent has no parent, then it is the root,
		// and we only need to replace the root with our sibling.
		if (kuInvalidSpatialId == uParentParent)
		{
			m_uRoot = uSibling;
			GetNode(uSibling).m_uParent = kuInvalidSpatialId;
			ReleaseNode(uParent);
		}
		else
		{
			// Pop parent and remove.
			{
				SpatialNode& parentParent = GetNode(uParentParent);
				if (parentParent.m_uChildA == uParent)
				{
					parentParent.m_uChildA = uSibling;
				}
				else
				{
					parentParent.m_uChildB = uSibling;
				}
				GetNode(uSibling).m_uParent = uParentParent;
				ReleaseNode(uParent);
			}

			// Fixup AABBs to the root.
			RecomputeAABBsToRoot(uParentParent);
		}
	}

	/** Release a tracked object. */
	void RemoveObject(SpatialId uObject)
	{
		SEOUL_ASSERT(m_vObjects.GetSize() > uObject);
		SEOUL_ASSERT(!m_vFreeObjects.Contains(uObject));

		// TODO: I think what we want here for this to be 100% semantically
		// correct is to call ~T() here. But that means we need to manage
		// m_vObjects like HashTable<> and perform all operations ourselves,
		// so we're not (for example) calling a copy constructor on a destroyed
		// element when resizing the memory block.
		m_vObjects[uObject] = T();
		m_vFreeObjects.PushBack(uObject);
	}

	SEOUL_DISABLE_COPY(SpatialTree);
}; // class SpatialTree

} // namespace Seoul

#endif // include guard
