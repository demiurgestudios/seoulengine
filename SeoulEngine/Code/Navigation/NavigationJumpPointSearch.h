/**
 * \file NavigationJumpPointSearch.h
 * SeoulEngine implementation of Jump Point Search.
 *
 * Adapted from:
 *   Public domain Jump Point Search implementation by False.Genesis
 *   https://github.com/fgenesis/jps
 *
 * References:
 *   http://users.cecs.anu.edu.au/~dharabor/data/papers/harabor-grastien-aaai11.pdf
 *   Jumper (https://github.com/Yonaba/Jumper)
 *   PathFinding.js (https://github.com/qiao/PathFinding.js)
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NAVIGATION_JUMP_POINT_SEARCH_H
#define NAVIGATION_JUMP_POINT_SEARCH_H

#include "Algorithms.h"
#include "HashTable.h"
#include "NavigationPosition.h"
#include "Prereqs.h"
#include "SeoulMath.h"
#include "Vector.h"

#if SEOUL_WITH_NAVIGATION

namespace Seoul::Navigation
{

namespace JumpPointSearch
{

enum class Result
{
	kNoPath,
	kFoundPath,
	kNeedMoreSteps,
	kEmptyPath,
};

class Node SEOUL_SEALED
{
public:
	Node(const Position& p)
		: m_uF(0u)
		, m_uG(0u)
		, m_Position(p)
		, m_pParent(nullptr)
		, m_uFlags(0u)
	{
	}

	UInt32 m_uF;
	UInt32 m_uG;
	Position const m_Position;
	Node const* m_pParent;

	Bool IsClosed() const
	{
		return (0u != (m_uFlags & 2u));
	}

	Bool IsOpen() const
	{
		return (0u != (m_uFlags & 1u));
	}

	void SetClosed()
	{
		m_uFlags |= 2u;
	}

	void SetOpen()
	{
		m_uFlags |= 1u;
	}

private:
	UInt32 m_uFlags;

	SEOUL_DISABLE_COPY(Node);
}; // class Node
typedef Vector<Node*, MemoryBudgets::Navigation> NodeVector;

inline static UInt32 Manhattan(const Position& positionA, const Position& positionB)
{
	return
		Abs((Int32)positionA.m_uX - (Int32)positionB.m_uX) +
		Abs((Int32)positionA.m_uY - (Int32)positionB.m_uY);
}

inline static UInt32 Euclidean(Node const* pA, Node const* pB)
{
	Float32 const fX = (Float32)((Int32)pA->m_Position.m_uX - (Int32)pB->m_Position.m_uX);
	Float32 const fY = (Float32)((Int32)pA->m_Position.m_uY - (Int32)pB->m_Position.m_uY);

	return (UInt32)((Int32)Sqrt(fX * fX + fY * fY));
}

class OpenList SEOUL_SEALED
{
public:
	void Clear()
	{
		m_vNodes.Clear();
	}

	void Fixup()
	{
		std::make_heap(m_vNodes.Begin(), m_vNodes.End(), CompareNodePtr);
	}

	Bool IsEmpty() const
	{
		return m_vNodes.IsEmpty();
	}

	Node* Pop()
	{
		std::pop_heap(m_vNodes.Begin(), m_vNodes.End(), CompareNodePtr);
		Node *node = m_vNodes.Back();
		m_vNodes.PopBack();
		return node;
	}

	void Push(Node* pNode)
	{
		SEOUL_ASSERT(nullptr != pNode);
		m_vNodes.PushBack(pNode);
		std::push_heap(m_vNodes.Begin(), m_vNodes.End(), CompareNodePtr);
	}

private:
	static Bool CompareNodePtr(Node const* pA, Node const* pB)
	{
		return (pA->m_uF > pB->m_uF);
	}
	NodeVector m_vNodes;
}; // class OpenList

template <typename T>
class Searcher SEOUL_SEALED
{
public:
	Searcher()
		: m_pEndNode(nullptr)
		, m_iSkip(1)
		, m_iStepsRemain(0)
		, m_uStepsDone(0)
	{
	}

	~Searcher()
	{
		SafeDeleteTable(m_tNodes);
	}

	// One-off, full path find.
	Bool FindPath(
		const T& grid,
		Positions* pvPath,
		const Position& start,
		const Position& end,
		UInt32 uStep);

	// Incremental path find.
	Result FindPathInit(const T& grid, Position start, Position end);
	Result FindPathStep(const T& grid, Int32 iLimit);
	Bool FindPathFinish(Positions* pvPath, UInt32 uStep);

	UInt32 GetStepsDone() const
	{
		return m_uStepsDone;
	}

	UInt32 GetNodesExpanded() const
	{
		return m_tNodes.GetSize();
	}

	void SetSkip(Int32 iSkip)
	{
		m_iSkip = Max(1, iSkip);
	}

private:
	typedef HashTable<Position, Node*, MemoryBudgets::Navigation> NodeTable;

	Node* m_pEndNode;
	Int32 m_iSkip;
	Int32 m_iStepsRemain;
	UInt32 m_uStepsDone;
	OpenList m_Open;
	NodeTable m_tNodes;

	Int32 FindNeighbors(const T& grid, Node const* pNode, Position* wptr) const;
	Node* GetNode(const T& grid, const Position& position);
	Bool GeneratePath(Positions* pvPath, UInt32 uStep) const;
	void IdentifySuccessors(const T& grid, Node const* pNode);

	Bool JumpP(const T& grid, Position& p, const Position& src);
	Bool JumpD(const T& grid, Position& p, Int32 iDx, Int32 iDy);
	Bool JumpX(const T& grid, Position& p, Int32 iDx);
	Bool JumpY(const T& grid, Position& p, Int32 iDy);

	SEOUL_DISABLE_COPY(Searcher);
}; // class Searcher

template <typename T>
inline Node* Searcher<T>::GetNode(const T& grid, const Position& position)
{
	SEOUL_ASSERT(grid(position.m_uX, position.m_uY));

	// Conditional insert - if successfuly, instantiate
	// a node on the heap and insert.
	Pair<NodeTable::Iterator, Bool> const res = m_tNodes.Insert(
		position,
		nullptr);
	if (res.Second)
	{
		res.First->Second = SEOUL_NEW(MemoryBudgets::Navigation) Node(position);
	}

	return res.First->Second;
}

template <typename T>
inline Bool Searcher<T>::JumpP(
	const T& grid,
	Position& p,
	const Position& src)
{
	SEOUL_ASSERT(grid(p.m_uX, p.m_uY));

	Int32 iDx = (Int32)p.m_uX - (Int32)src.m_uX;
	Int32 iDy = (Int32)p.m_uY - (Int32)src.m_uY;
	SEOUL_ASSERT(0 != iDx || 0 != iDy);

	if (0 != iDx && 0 != iDy)
	{
		return JumpD(grid, p, iDx, iDy);
	}
	else if (0 != iDx)
	{
		return JumpX(grid, p, iDx);
	}
	else if (0 != iDy)
	{
		return JumpY(grid, p, iDy);
	}

	SEOUL_FAIL("Programmer error, unexpected case reached.");
	return false;
}

template <typename T>
inline Bool Searcher<T>::JumpD(
	const T& grid,
	Position& p,
	Int32 iDx,
	Int32 iDy)
{
	SEOUL_ASSERT(grid(p.m_uX, p.m_uY));
	SEOUL_ASSERT(0 != iDx && 0 != iDy);

	const Position& endPosition = m_pEndNode->m_Position;
	Int32 iSteps = 0;
	Bool bReturn = true;

	while (true)
	{
		if (p == endPosition)
		{
			break;
		}

		++iSteps;
		Int32 const iX = (Int32)p.m_uX;
		Int32 const iY = (Int32)p.m_uY;

		if ((grid(iX - iDx, iY + iDy) && !grid(iX - iDx, iY)) ||
			(grid(iX + iDx, iY - iDy) && !grid(iX, iY - iDy)))
		{
			break;
		}

		Bool const bGdx = grid(iX + iDx, iY);
		Bool const bGdy = grid(iX, iY + iDy);

		Position testP = Position(iX + iDx, iY);
		if (bGdx && JumpX(grid, testP, iDx))
		{
			break;
		}

		testP = Position(iX, iY + iDy);
		if (bGdy && JumpY(grid, testP, iDy))
		{
			break;
		}

		if ((bGdx || bGdy) && grid(iX + iDx, iY + iDy))
		{
			p.m_uX += iDx;
			p.m_uY += iDy;
		}
		else
		{
			bReturn = false;
			break;
		}
	}

	m_uStepsDone += (UInt32)iSteps;
	m_iStepsRemain -= iSteps;
	return bReturn;
}

template <typename T>
inline Bool Searcher<T>::JumpX(
	const T& grid,
	Position& p,
	Int32 iDx)
{
	SEOUL_ASSERT(0 != iDx);
	SEOUL_ASSERT(grid(p.m_uX, p.m_uY));

	UInt32 const uY = p.m_uY;
	const Position& endPosition = m_pEndNode->m_Position;
	Int32 const iSkip = m_iSkip;
	Int32 iSteps = 0;
	Bool bReturn = true;

	UInt32 uA = ~(
		(!!grid(p.m_uX, (UInt32)((Int32)uY + iSkip))) |
		((!!grid(p.m_uX, (UInt32)((Int32)uY - iSkip))) << 1));

	while (true)
	{
		UInt32 const uXX = (UInt32)((Int32)p.m_uX + iDx);
		UInt32 const uB =
			(!!grid(uXX, (UInt32)((Int32)uY + iSkip))) |
			((!!grid(uXX, (UInt32)((Int32)uY - iSkip))) << 1);

		if (0u != (uB & uA) || p == endPosition)
		{
			break;
		}

		if (!grid(uXX, uY))
		{
			bReturn = false;
			break;
		}

		p.m_uX += iDx;
		uA = ~uB;
		++iSteps;
	}

	m_uStepsDone += (UInt32)iSteps;
	m_iStepsRemain -= iSteps;
	return bReturn;
}

template <typename T>
inline Bool Searcher<T>::JumpY(
	const T& grid,
	Position& p,
	Int32 iDy)
{
	SEOUL_ASSERT(0 != iDy);
	SEOUL_ASSERT(grid(p.m_uX, p.m_uY));

	UInt32 const uX = p.m_uX;
	const Position& endPosition = m_pEndNode->m_Position;
	Int32 const iSkip = m_iSkip;
	Int32 iSteps = 0;
	Bool bReturn = true;

	UInt32 uA = ~(
		(!!grid((UInt32)((Int32)uX + iSkip), p.m_uY)) |
		((!!grid((UInt32)((Int32)uX - iSkip), p.m_uY)) << 1));

	while (true)
	{
		UInt32 const uYY = (UInt32)((Int32)p.m_uY + iDy);
		UInt32 const uB =
			(!!grid((UInt32)((Int32)uX + iSkip), uYY)) |
			((!!grid((UInt32)((Int32)uX - iSkip), uYY)) << 1);

		if (0u != (uA & uB) || p == endPosition)
		{
			break;
		}

		if (!grid(uX, uYY))
		{
			bReturn = false;
			break;
		}

		p.m_uY = (UInt32)((Int32)p.m_uY + iDy);
		uA = ~uB;
	}

	m_uStepsDone += (UInt32)iSteps;
	m_iStepsRemain -= iSteps;
	return bReturn;
}

#define SEOUL_JPS_CHECKGRID(iDx, iDy) (grid((UInt32)((Int32)uX + (iDx)), (UInt32)((Int32)uY + (iDy))))
#define SEOUL_JPS_ADDPOS(iDx, iDy) 	do { *w++ = Position((UInt32)((Int32)uX + (iDx)), (UInt32)((Int32)uY + (iDy))); } while(0)
#define SEOUL_JPS_ADDPOS_CHECK(iDx, iDy) do { if (SEOUL_JPS_CHECKGRID(iDx, iDy)) SEOUL_JPS_ADDPOS(iDx, iDy); } while(0)
#define SEOUL_JPS_ADDPOS_NO_TUNNEL(iDx, iDy) do { if (grid((UInt32)((Int32)uX + (iDx)), uY) || grid(uX, (UInt32)((Int32)uY + (iDy)))) SEOUL_JPS_ADDPOS_CHECK(iDx, iDy); } while(0)
template <typename T>
Int32 Searcher<T>::FindNeighbors(const T& grid, Node const* pNode, Position* wptr) const
{
	Position* w = wptr;
	UInt32 const uX = pNode->m_Position.m_uX;
	UInt32 const uY = pNode->m_Position.m_uY;
	Int32 const iSkip = m_iSkip;

	if (nullptr == pNode->m_pParent)
	{
		// straight moves
		SEOUL_JPS_ADDPOS_CHECK(-iSkip, 0);
		SEOUL_JPS_ADDPOS_CHECK(0, -iSkip);
		SEOUL_JPS_ADDPOS_CHECK(0, iSkip);
		SEOUL_JPS_ADDPOS_CHECK(iSkip, 0);

		// diagonal moves + prevent tunneling
		SEOUL_JPS_ADDPOS_NO_TUNNEL(-iSkip, -iSkip);
		SEOUL_JPS_ADDPOS_NO_TUNNEL(-iSkip, iSkip);
		SEOUL_JPS_ADDPOS_NO_TUNNEL(iSkip, -iSkip);
		SEOUL_JPS_ADDPOS_NO_TUNNEL(iSkip, iSkip);

		return (Int32)(w - wptr);
	}

	// jump directions (both -1, 0, or 1)
	Int32 iDx = (Int32)uX - (Int32)pNode->m_pParent->m_Position.m_uX;
	iDx /= Max(Abs(iDx), 1);
	iDx *= iSkip;
	Int32 iDy = (Int32)uY - (Int32)pNode->m_pParent->m_Position.m_uY;
	iDy /= Max(Abs(iDy), 1);
	iDy *= iSkip;

	if (0 != iDx && 0 != iDy)
	{
		// diagonal
		// natural neighbors
		Bool bWalkX = false;
		Bool bWalkY = false;
		if ((bWalkX = grid((UInt32)((Int32)uX + iDx), uY)))
		{
			*w++ = Position((UInt32)((Int32)uX + iDx), uY);
		}

		if ((bWalkY = grid(uX, (UInt32)((Int32)uY + iDy))))
		{
			*w++ = Position(uX, (UInt32)((Int32)uY + iDy));
		}

		if (bWalkX || bWalkY)
		{
			SEOUL_JPS_ADDPOS_CHECK(iDx, iDy);
		}

		// forced neighbors
		if (bWalkY && !SEOUL_JPS_CHECKGRID(-iDx, 0))
		{
			SEOUL_JPS_ADDPOS_CHECK(-iDx, iDy);
		}

		if (bWalkX && !SEOUL_JPS_CHECKGRID(0, -iDy))
		{
			SEOUL_JPS_ADDPOS_CHECK(iDx, -iDy);
		}
	}
	else if (0 != iDx)
	{
		// along X axis
		if (SEOUL_JPS_CHECKGRID(iDx, 0))
		{
			SEOUL_JPS_ADDPOS(iDx, 0);

			// Forced neighbors (+ prevent tunneling)
			if (!SEOUL_JPS_CHECKGRID(0, iSkip))
			{
				SEOUL_JPS_ADDPOS_CHECK(iDx, iSkip);
			}

			if (!SEOUL_JPS_CHECKGRID(0, -iSkip))
			{
				SEOUL_JPS_ADDPOS_CHECK(iDx, -iSkip);
			}
		}
	}
	else if (0 != iDy)
	{
		// along Y axis
		if (SEOUL_JPS_CHECKGRID(0, iDy))
		{
			SEOUL_JPS_ADDPOS(0, iDy);

			// Forced neighbors (+ prevent tunneling)
			if (!SEOUL_JPS_CHECKGRID(iSkip, 0))
			{
				SEOUL_JPS_ADDPOS_CHECK(iSkip, iDy);
			}

			if (!SEOUL_JPS_CHECKGRID(-iSkip, 0))
			{
				SEOUL_JPS_ADDPOS_CHECK(-iSkip, iDy);
			}
		}
	}

	return (Int32)(w - wptr);
}
#undef SEOUL_JPS_ADDPOS
#undef SEOUL_JPS_ADDPOS_CHECK
#undef SEOUL_JPS_ADDPOS_NO_TUNNEL
#undef SEOUL_JPS_CHECKGRID

template <typename T>
void Searcher<T>::IdentifySuccessors(const T& grid, Node const* pNode)
{
	Position aBuffer[8];
	Int32 const iNumber = FindNeighbors(grid, pNode, &aBuffer[0]);
	for (Int32 i = iNumber - 1; i >= 0; --i)
	{
		Position jumpPoint = aBuffer[i];
		if (!JumpP(grid, jumpPoint, pNode->m_Position))
		{
			continue;
		}

		// Now that the grid position is definitely a valid jump point, we have to create the actual node.
		Node* pJumpNode = GetNode(grid, jumpPoint);
		SEOUL_ASSERT(pJumpNode && pJumpNode != pNode);
		if (!pJumpNode->IsClosed())
		{
			UInt32 const uExtraG = Euclidean(pJumpNode, pNode);
			UInt32 const uNewG = pNode->m_uG + uExtraG;
			if (!pJumpNode->IsOpen() || uNewG < pJumpNode->m_uG)
			{
				pJumpNode->m_uG = uNewG;
				pJumpNode->m_uF = pJumpNode->m_uG + Manhattan(pJumpNode->m_Position, m_pEndNode->m_Position);
				pJumpNode->m_pParent = pNode;
				if (!pJumpNode->IsOpen())
				{
					m_Open.Push(pJumpNode);
					pJumpNode->SetOpen();
				}
				else
				{
					m_Open.Fixup();
				}
			}
		}
	}
}

template <typename T>
Bool Searcher<T>::GeneratePath(
	Positions* pvPath,
	UInt32 uStep) const
{
	// Early out if not emitting points.
	if (nullptr == pvPath)
	{
		return true;
	}

	if (nullptr == m_pEndNode)
	{
		return false;
	}

	Node const* pNext = m_pEndNode;
	Node const* pPrev = m_pEndNode->m_pParent;

	// Return a valid path that is only the last node
	// if no parent.
	if (nullptr == pPrev)
	{
		pvPath->PushBack(m_pEndNode->m_Position);
		pvPath->PushBack(m_pEndNode->m_Position);
		return true;
	}

	UInt32 const uOffset = pvPath->GetSize();
	if (0u != uStep)
	{
		do
		{
			UInt32 const uX = pNext->m_Position.m_uX;
			UInt32 const uY = pNext->m_Position.m_uY;

			Int32 iDx = (Int32)pPrev->m_Position.m_uX - (Int32)uX;
			Int32 iDy = (Int32)pPrev->m_Position.m_uY - (Int32)uY;
			SEOUL_ASSERT(0 == iDx || 0 == iDy || Abs(iDx) == Abs(iDy));

			Int32 const iSteps = Max(Abs(iDx), Abs(iDy));
			iDx /= Max(Abs(iDx), 1);
			iDy /= Max(Abs(iDy), 1);
			iDx *= (Int32)uStep;
			iDy *= (Int32)uStep;
			Int32 iDxa = 0;
			Int32 iDya = 0;
			for (Int32 i = 0; i < iSteps; i += (Int32)uStep)
			{
				pvPath->PushBack(Position((UInt32)((Int32)uX + iDxa), (UInt32)((Int32)uY + iDya)));
				iDxa += iDx;
				iDya += iDy;
			}
			pNext = pPrev;
			pPrev = pPrev->m_pParent;
		} while (nullptr != pPrev);
	}
	else
	{
		do
		{
			SEOUL_ASSERT(pNext != pNext->m_pParent);
			pvPath->PushBack(pNext->m_Position);
			pNext = pNext->m_pParent;
		} while (pNext);
	}

	Reverse(pvPath->Begin() + uOffset, pvPath->End());
	return true;
}

template <typename T>
Bool Searcher<T>::FindPath(
	const T& grid,
	Positions* pvPath,
	const Position& start,
	const Position& end,
	UInt32 uStep)
{
	Result eResult = FindPathInit(grid, start, end);

	// kEmptyPath means a path find from start to itself. We always want to
	// output a path when returning true, so it will only contain 2 nodes,
	// start and end.
	if (eResult == Result::kEmptyPath)
	{
		if (nullptr != pvPath)
		{
			pvPath->PushBack(start);
			pvPath->PushBack(end);
		}

		return true;
	}

	while (true)
	{
		switch (eResult)
		{
		case Result::kNeedMoreSteps:
			eResult = FindPathStep(grid, 0);
			break;

		case Result::kFoundPath:
			return FindPathFinish(pvPath, uStep);

		case Result::kNoPath: // fall-through
		default:
			return false;
		};
	}
}

template <typename T>
Result Searcher<T>::FindPathInit(
	const T& grid,
	Position start,
	Position end)
{
	SafeDeleteTable(m_tNodes);
	m_Open.Clear();
	m_pEndNode = nullptr;
	m_uStepsDone = 0u;

	// If m_iSkip is > 1, make sure the points are aligned so that the search will always hit them
	start.m_uX = (start.m_uX / m_iSkip) * m_iSkip;
	start.m_uY = (start.m_uY / m_iSkip) * m_iSkip;
	end.m_uX = (end.m_uX / m_iSkip) * m_iSkip;
	end.m_uY = (end.m_uY / m_iSkip) * m_iSkip;

	if (start == end)
	{
		// There is only a path if this single position is walkable.
		// But since the starting position is omitted, there is nothing to do here.
		return grid(end.m_uX, end.m_uY) ? Result::kEmptyPath : Result::kNoPath;
	}

	// If start or end point are obstructed, don't even start
	if (!grid(start.m_uX, start.m_uY) || !grid(end.m_uX, end.m_uY))
	{
		return Result::kNoPath;
	}

	m_pEndNode = GetNode(grid, end);
	Node* pStartNode = GetNode(grid, start);
	SEOUL_ASSERT(nullptr != pStartNode && nullptr != m_pEndNode);

	m_Open.Push(pStartNode);

	return Result::kNeedMoreSteps;
}

template <typename T>
Result Searcher<T>::FindPathStep(const T& grid, Int32 iLimit)
{
	m_iStepsRemain = iLimit;
	do
	{
		if (m_Open.IsEmpty())
		{
			return Result::kNoPath;
		}

		Node* pNode = m_Open.Pop();
		pNode->SetClosed();
		if (pNode == m_pEndNode)
		{
			return Result::kFoundPath;
		}

		IdentifySuccessors(grid, pNode);
	} while (m_iStepsRemain >= 0);

	return Result::kNeedMoreSteps;
}

template <typename T>
Bool Searcher<T>::FindPathFinish(
	Positions* pvPath,
	UInt32 uStep)
{
	return GeneratePath(pvPath, uStep);
}

} // namespace JumpPointSearch

} // namespace Seoul::Navigation

#endif // /#if SEOUL_WITH_NAVIGATION

#endif // include guard
