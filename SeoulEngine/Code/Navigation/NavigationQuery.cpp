/**
 * \file NavigationQuery.cpp
 * \brief Read-only structure for issue path finding and
 * ray queries against a navigation grid. A query
 * is read-only and can be shared across multiple
 * threaded QueryState instances (for which there
 * must be a single instance for each querying entity).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Algorithms.h"
#include "NavigationJumpPointSearch.h"
#include "NavigationGrid.h"
#include "NavigationQuery.h"
#include "NavigationQueryState.h"
#include "SeoulMath.h"

#if SEOUL_WITH_NAVIGATION

namespace Seoul::Navigation
{

/** Identical to path binder, except takes signed coordinates and checks for >= 0. */
class QueryNearestBinder SEOUL_SEALED : QueryPathBinder
{
public:
	QueryNearestBinder(const Query& query)
		: QueryPathBinder(query)
	{
	}

	Bool operator()(Int32 iX, Int32 iY) const
	{
		if (iX >= 0 && iY >= 0)
		{
			return (*((QueryPathBinder*)this))((UInt32)iX, (UInt32)iY);
		}

		return false;
	}

private:
	SEOUL_DISABLE_COPY(QueryNearestBinder);
}; // class QueryNearestBinder

Query::Query(
	const Grid& grid,
	UInt16 uFlags /*= QueryConfig::kNone*/,
	UInt8 uBlockerMask /*= 0xFF*/,
	UInt8 uForcePassableMask /*= 0u*/)
	: m_Grid(grid)
	, m_uFlags(uFlags)
	, m_uBlockerMask(uBlockerMask)
	, m_uForcePassableMask(uForcePassableMask)
	, m_pConnectivity((0u == (uFlags & QueryConfig::kDisableConnectivity))
		? (UInt32*)MemoryManager::AllocateAligned(sizeof(UInt32) * grid.GetWidth() * grid.GetHeight(), MemoryBudgets::Navigation, SEOUL_ALIGN_OF(UInt32))
		: (UInt32*)nullptr)
{
	ComputeConnectivity();
}

Query::~Query()
{
	MemoryManager::Deallocate(m_pConnectivity);
}

/**
 * Find the nearest candidate point
 * (based on this query's masks) to
 * start, at a max of uMaxDistance,
 * which is connected to connectedTo.
 *
 * Fails if connectivity information does
 * not exist for this query.
 *
 * Pre: start and connectedTo must be on the grid.
 *
 * On success, outputs the found point.
 */
Bool Query::FindNearestConnected(
	QueryState& rState,
	const Position& start,
	UInt32 uMaxDistance,
	const Position& connectedTo,
	Position& rNearest) const
{
	// Early out if no connectivity info.
	if (!HasConnectivity())
	{
		return false;
	}

	// Enforce preconditions.
	SEOUL_ASSERT(start.m_uX < m_Grid.GetWidth());
	SEOUL_ASSERT(start.m_uY < m_Grid.GetHeight());
	SEOUL_ASSERT(connectedTo.m_uX < m_Grid.GetWidth());
	SEOUL_ASSERT(connectedTo.m_uY < m_Grid.GetHeight());

	// Get connectivity ID of the target. If this is not connected,
	// no connection can be made.
	UInt32 const uConnectedId = GetConnectivityId(connectedTo.m_uX, connectedTo.m_uY);
	if (uConnectedId == QueryConfig::kuNoConnectivityId)
	{
		return false;
	}

	// Nearest binder, also handles < 0.
	QueryNearestBinder binder(*this);

	// Simple case, already passes.
	if (GetConnectivityId(start.m_uX, start.m_uY) == uConnectedId)
	{
		rNearest = start;
		return true;
	}

	// TODO: Best distance approximation does not handle cases where
	// FindNearestConnected() is used for endpoint path resolution (e.g. path
	// wraps around a target). Ideal solution would like hoist the final point
	// selection out of FindNearestConnected().

	// Iterate in-to-out until the max distance.
	Int32 const iStartX = (Int32)start.m_uX;
	Int32 const iStartY = (Int32)start.m_uY;
	UInt32 uBestDistance = UIntMax;
	for (Int32 iDistance = 1; iDistance <= (Int32)uMaxDistance; ++iDistance)
	{
		// Zig-zag pattern - e.g. 0, 1, -1, 2, -2, 3
		// We always stop on the last positive, to avoid processing a
		// corner twice (e.g. the corner at -3 of the left side will be handled
		// by +3 of the bottom side).
		Int32 iOffset = 0;
		// Step starts out at +1, and is increased by +1 with each loop.
		Int32 iStep = 1;
		// Start with addition, engate each iteration.
		Int32 iSign = 1;
		Int32 const iIterations = (iDistance * 2);
		for (Int32 iIteration = 0; iIteration < iIterations; ++iIteration)
		{
			// If we've found a nearest and iOffset is positive, it
			// means we're done - we'd be evaluating points that
			// are further away from the target, which would potentially
			// return a point that is closer to the connectedTo but further
			// from the target.
			if (UIntMax != uBestDistance && iOffset >= 0)
			{
				return true;
			}

			// Top side
			ResolveNearestConnected(
				iStartX + iOffset,
				iStartY - iDistance,
				uConnectedId,
				connectedTo,
				rNearest,
				uBestDistance);

			// Right side
			ResolveNearestConnected(
				iStartX + iDistance,
				iStartY + iOffset,
				uConnectedId,
				connectedTo,
				rNearest,
				uBestDistance);

			// Bottom side
			ResolveNearestConnected(
				iStartX - iOffset,
				iStartY + iDistance,
				uConnectedId,
				connectedTo,
				rNearest,
				uBestDistance);

			// Left side
			ResolveNearestConnected(
				iStartX - iDistance,
				iStartY - iOffset,
				uConnectedId,
				connectedTo,
				rNearest,
				uBestDistance);

			// Advance offset.
			iOffset = iOffset + (iSign * iStep);
			// Increase the step.
			iStep++;
			// Negate the sign.
			iSign = -iSign;
		}
	}

	return (uBestDistance != UIntMax);
}

/**
 * Find the nearest candidate point
 * (based on this query's masks) to
 * start, at a max of uMaxDistance.
 *
 * Pre: start must be on the grid.
 *
 * On success, outputs the found point.
 */
Bool Query::FindNearest(
	QueryState& rState,
	const Position& start,
	UInt32 uMaxDistance,
	Position& rNearest) const
{
	// Enforce preconditions.
	SEOUL_ASSERT(start.m_uX < m_Grid.GetWidth());
	SEOUL_ASSERT(start.m_uY < m_Grid.GetHeight());

	// Nearest binder, also handles < 0.
	QueryNearestBinder binder(*this);

	// Simple case, already passes.
	if (binder(start.m_uX, start.m_uY))
	{
		rNearest = start;
		return true;
	}

	// Iterate in-to-out until the max distance.
	Int32 const iStartX = (Int32)start.m_uX;
	Int32 const iStartY = (Int32)start.m_uY;
	for (Int32 iDistance = 1; iDistance <= (Int32)uMaxDistance; ++iDistance)
	{
		// Zig-zag pattern - e.g. 0, 1, -1, 2, -2, 3
		// We always stop on the last positive, to avoid processing a
		// corner twice (e.g. the corner at -3 of the left side will be handled
		// by +3 of the bottom side).
		Int32 iOffset = 0;
		// Step starts out at +1, and is increased by +1 with each loop.
		Int32 iStep = 1;
		// Start with addition, engate each iteration.
		Int32 iSign = 1;
		Int32 const iIterations = (iDistance * 2);
		for (Int32 iIteration = 0; iIteration < iIterations; ++iIteration)
		{
			// Top side
			{
				Int32 const iX = iStartX + iOffset;
				Int32 const iY = iStartY - iDistance;
				if (binder(iX, iY))
				{
					rNearest = Position((UInt32)iX, (UInt32)iY);
					return true;
				}
			}

			// Right side
			{
				Int32 const iX = iStartX + iDistance;
				Int32 const iY = iStartY + iOffset;
				if (binder(iX, iY))
				{
					rNearest = Position((UInt32)iX, (UInt32)iY);
					return true;
				}
			}

			// Bottom side
			{
				Int32 const iX = iStartX - iOffset;
				Int32 const iY = iStartY + iDistance;
				if (binder(iX, iY))
				{
					rNearest = Position((UInt32)iX, (UInt32)iY);
					return true;
				}
			}

			// Left side
			{
				Int32 const iX = iStartX - iDistance;
				Int32 const iY = iStartY - iOffset;
				if (binder(iX, iY))
				{
					rNearest = Position((UInt32)iX, (UInt32)iY);
					return true;
				}
			}

			// Advance offset.
			iOffset = iOffset + (iSign * iStep);
			// Increase the step.
			iStep++;
			// Negate the sign.
			iSign = -iSign;
		}
	}

	return false;
}

/**
 * Find a path from
 * [start, end] and write the result to rState
 * on success.
 *
 * Pre: start/end must be on the grid.
 *
 * Return true if a path was found, false otherwise.
 * On success, rState will be populated with the
 * resulting path.
 */
Bool Query::FindPath(
	QueryState& rState,
	const Position& start,
	const Position& end) const
{
	// Enforce preconditions.
	SEOUL_ASSERT(start.m_uX < m_Grid.GetWidth());
	SEOUL_ASSERT(start.m_uY < m_Grid.GetHeight());
	SEOUL_ASSERT(end.m_uX < m_Grid.GetWidth());
	SEOUL_ASSERT(end.m_uY < m_Grid.GetHeight());

	// Early out if we have connectivity info and there
	// is no connectivity.
	if (HasConnectivity())
	{
		if (!IsConnected(rState, start, end))
		{
			return false;
		}
	}

	using namespace JumpPointSearch;

	QueryPathBinder binder(*this);

	rState.m_vWaypoints.Clear();
	Bool const bFound = rState.m_Searcher.FindPath(
		binder,
		&rState.m_vWaypoints,
		start,
		end,
		0u);

	return bFound;
}

/**
 * Find a "straight" path from
 * [start, end] and write the result
 * to rState on success.
 *
 * Pre: start/end must be on
 * the grid.
 *
 * A straight path, also sometimes called "string pulling",
 * is the path returned by FindPath() pruned of any
 * unnecessary turns/corners. It is the shortest/most direct
 * version of a FindPath() waypoint set.
 */
Bool Query::FindStraightPath(
	QueryState& rState,
	const Position& start,
	const Position& end) const
{
	// Call FindPath() to compute the initial path.
	// If this fails, we're done.
	if (!FindPath(rState, start, end))
	{
		return false;
	}

	// First prune inner waypoints on completely straight
	// runs. Do this first so we avoid unnecessary ray
	// tests.
	PruneInnerWaypointsStraight(rState);

	// Finally, prune inner waypoints using ray tests
	// (e.g. if no ray hit between a & c, we can prune
	// b).
	PruneInnerWaypointsRayTest(rState);

	return true;
}

/** Return the connectivity info at (x, y). Pre: (x, y) must be on the grid. */
UInt32 Query::GetConnectivityId(UInt32 uX, UInt32 uY) const
{
	SEOUL_ASSERT(uX < m_Grid.GetWidth());
	SEOUL_ASSERT(uY < m_Grid.GetHeight());

	return m_pConnectivity[uY * m_Grid.GetWidth() + uX];
}

/** Return the connectivity info at (x, y). Checked, returns kuNoConnectivityId for out-of-bounds locations. */
UInt32 Query::GetConnectivityIdSafe(Int32 iX, Int32 iY) const
{
	if (iX >= 0 && iY >= 0)
	{
		UInt32 const uX = (UInt32)iX;
		UInt32 const uY = (UInt32)iY;
		if (uX < m_Grid.GetWidth() &&
			uY < m_Grid.GetHeight())
		{
			return m_pConnectivity[uY * m_Grid.GetWidth() + uX];
		}
	}

	return QueryConfig::kuNoConnectivityId;
}

/**
 * If HasConnectivity() is false, this method always returns false.
 *
 * Otherwise, return true if start has a path/connection
 * to end.
 *
 * Pre: start/end must be on the grid.
 *
 * O(1).
 */
Bool Query::IsConnected(
	QueryState& rState,
	const Position& start,
	const Position& end) const
{
	// Early out if no connectivity info.
	if (!HasConnectivity())
	{
		return false;
	}

	// Enforce preconditions.
	SEOUL_ASSERT(start.m_uX < m_Grid.GetWidth());
	SEOUL_ASSERT(start.m_uY < m_Grid.GetHeight());
	SEOUL_ASSERT(end.m_uX < m_Grid.GetWidth());
	SEOUL_ASSERT(end.m_uY < m_Grid.GetHeight());

	// Get connectivity ID for each.
	UInt32 const uStartConnectivityId = GetConnectivityId(start.m_uX, start.m_uY);
	UInt32 const uEndConnectivityId = GetConnectivityId(end.m_uX, end.m_uY);

	// No connectivity if either value is impassable value.
	if (QueryConfig::kuNoConnectivityId == uStartConnectivityId ||
		QueryConfig::kuNoConnectivityId == uEndConnectivityId)
	{
		return false;
	}

	// Connected if both positions have the same connectivity id.
	return (uStartConnectivityId == uEndConnectivityId);
}

/**
 * Return true or false if a particular grid cell is passable.
 *
 * Pre: position must beon the grid.
 */
Bool Query::IsPassable(const Position& position) const
{
	UInt8 const uValue = m_Grid.GetCell(position.m_uX, position.m_uY);
	return IsPassable(uValue);
}

/**
 * Call when a referenced Grid is mutated.
 *
 * Queries may be stale/invalid until this function is called,
 * after a mutation occurs to the underlying grid. It is useful
 * to batch mutations and calls this function less frequently,
 * since it can be expensive.
 *
 * Important: it is the caller's responsibility to synchronize
 * this function call such that it is mutually exclusive from
 * other query calls.
 */
void Query::OnDirty()
{
	ComputeConnectivity();
}

/**
 * Cast a ray against the grid,
 * from [start, end] inclusive.
 *
 * Pre: Start/end must be on the grid.
 *
 * Return true if a hit occurs, false otherwise.
 * On hit, riHitX and riHitY will contain
 * the grid cell of the hit.
 */
Bool Query::RayTest(
	QueryState& rState,
	const Position& start,
	const Position& end,
	Bool bHitStartingCell,
	Position& rHit) const
{
	// Enforce preconditions.
	SEOUL_ASSERT(start.m_uX < m_Grid.GetWidth());
	SEOUL_ASSERT(start.m_uY < m_Grid.GetHeight());
	SEOUL_ASSERT(end.m_uX < m_Grid.GetWidth());
	SEOUL_ASSERT(end.m_uY < m_Grid.GetHeight());

	// Cache local variables.
	Int32 iX = (Int32)start.m_uX;
	Int32 iY = (Int32)start.m_uY;
	Int32 const iEndX = (Int32)end.m_uX;
	Int32 const iEndY = (Int32)end.m_uY;

	// Setup deltas for tracking error to
	// apply direction, and step size
	// (always at least one cell in any direction).
	Int32 const iDeltaX = Abs(iEndX - iX);
	Int32 const iDeltaY = Abs(iEndY - iY);
	Int32 const iStepX = (iX < iEndX ? 1 : -1);
	Int32 const iStepY = (iY < iEndY ? 1 : -1);

	// Error tracking, half the separation.
	Int32 iError0 = (iDeltaX > iDeltaY ? iDeltaX : -iDeltaY) / 2;
	Int32 iError1 = iError0;

	Int32 const iWidth = (Int32)m_Grid.GetWidth();
	Int32 const iHeight = (Int32)m_Grid.GetHeight();
	while (true)
	{
		// No hit if now out of bounds.
		if (iX < 0 || iX >= iWidth ||
			iY < 0 || iY >= iHeight)
		{
			return false;
		}

		// Check for hit.
		if (!IsPassable(m_Grid.GetCell((UInt32)iX, (UInt32)iY)))
		{
			// If bHitStartingCell is false, this hit
			// must be at a location other than the starting
			// cell to count as a hit.
			if (bHitStartingCell || (start.m_uX != (UInt32)iX || start.m_uY != (UInt32)iY))
			{
				rHit.m_uX = (UInt32)iX;
				rHit.m_uY = (UInt32)iY;
				return true;
			}
		}

		// Not hit if start has reached end.
		if (iX == iEndX && iY == iEndY)
		{
			return false;
		}

		// Advance to the next cell (pixel, in the original Bresenham's line drawing algorithm).
		iError1 = iError0;
		if (iError1 > -iDeltaX)
		{
			iError0 -= iDeltaY;
			iX += iStepX;
		}
		if (iError1 < iDeltaY)
		{
			iError0 += iDeltaX;
			iY += iStepY;
		}
	}
}

// Utility used by ComputeConnectivity(), insert a pair
// of connectivity ids found to be connected.
static void InsertPair(
	Vector<UInt32, MemoryBudgets::Navigation>& rv,
	UInt32 u0,
	UInt32 u1)
{
	// Sanity check - should never be called if u0 == u1.
	SEOUL_ASSERT(u0 != u1);

	// u0 should always be lower.
	if (u1 < u0)
	{
		Swap(u1, u0);
	}

	// The end state of InertPair() is to update the table so
	// that chains exist from the highest connectivity id,
	// to the lowest connectivity id. This will then be resolved
	// into a direct lookup from an id to its lowest union.
	UInt32 uCurrentRemap = rv[u1];
	while (uCurrentRemap != QueryConfig::kuNoConnectivityId)
	{
		// If we already remap to u0, we're done.
		if (uCurrentRemap == u0)
		{
			return;
		}

		// If u0 is less than the current, replace the current
		// and remap uCurrentRemap.
		if (u0 < uCurrentRemap)
		{
			rv[u1] = u0;
			u1 = uCurrentRemap;
		}
		// Otherwise, leave the entry alone, but now fixup u0 -> uCurrentRemap.
		else
		{
			u1 = u0;
			u0 = uCurrentRemap;
		}

		uCurrentRemap = rv[u1];
	}

	// If we get here, need to insert a remape from u1 to u0.
	rv[u1] = u0;
}

void Query::ComputeConnectivity()
{
	// Early out.
	if (!HasConnectivity())
	{
		return;
	}

	UInt32 const uWidth = m_Grid.GetWidth();
	UInt32 const uHeight = m_Grid.GetHeight();
	UInt32 const uConnectivitySizeInBytes = (sizeof(UInt32) * uWidth * uHeight);

	// Sanity check.
	SEOUL_STATIC_ASSERT(QueryConfig::kuNoConnectivityId == 0u);
	memset(m_pConnectivity, 0, uConnectivitySizeInBytes);

	// Passable query binder.
	QueryPathBinder binder(*this);

	// During flood filling, we may create 2 groups that are actually connected. This
	// table is used to track connected groups so they can be fixed up in the second pass.
	Vector<UInt32, MemoryBudgets::Navigation> vRemap;

	// Rough heuristic, use the max of width/height for initial capacity.
	vRemap.Reserve(Max(uWidth, uHeight));

	// First pass, check left, left-top, and top - if index != kuNoConnectivityId, use it, otherwise
	// assign the next index.
	UInt32 uCurrentId = 1u;
	vRemap.PushBack(QueryConfig::kuNoConnectivityId);
	for (UInt32 uY = 0u; uY < uHeight; ++uY)
	{
		for (UInt32 uX = 0u; uX < uWidth; ++uX)
		{
			// Early out if not passable.
			if (!IsPassable(m_Grid.GetCell(uX, uY)))
			{
				continue;
			}

			// We have at most 4 adjacencies (left, topleft, top, topright).
			UInt32 auNeighbors[4];
			UInt32 uNeighborCount = 0u;

			// Check left.
			if (uX > 0u)
			{
				ResolveNeighbor(uX - 1u, uY, auNeighbors, uNeighborCount);
			}

			// Check top
			if (uY > 0u)
			{
				ResolveNeighbor(uX, uY - 1u, auNeighbors, uNeighborCount);
			}

			// Check top-left
			if (uX > 0u && uY > 0u)
			{
				// Diagonal connectivity requires at least one of the
				// adjacent blocks to be passable. e.g. (X means impassable, O means passable):
				// 1.   2.   3.
				// A X  A O  A X
				// X A  X A  O A
				//
				// One is not connected, 2 and 3 are connected.
				if (binder(uX - 1u, uY) || binder(uX, uY - 1u))
				{
					ResolveNeighbor(uX - 1u, uY - 1u, auNeighbors, uNeighborCount);
				}
			}

			// Check top-right
			if (uX + 1u < uWidth && uY > 0u)
			{
				// Must check corners, see comment in Check top-left above.
				if (binder(uX + 1u, uY) || binder(uX, uY - 1u))
				{
					ResolveNeighbor(uX + 1u, uY - 1u, auNeighbors, uNeighborCount);
				}
			}

			// Three cases:
			// - no neighbors, new id.
			// - one neighbor, use that id.
			// - two neighbors - use the lower of the two ids and update our resolution
			//   table so we can merge groups in the second pass.
			if (0u == uNeighborCount)
			{
				// No neighbors, new id.
				SetConnectivityId(uX, uY, uCurrentId);
				++uCurrentId;
				vRemap.PushBack(QueryConfig::kuNoConnectivityId);
			}
			else if (1u == uNeighborCount)
			{
				// Use neighbor0 id.
				SetConnectivityId(uX, uY, auNeighbors[0]);
			}
			else
			{
				// Sort the neighbors so minimum is first.
				QuickSort(&auNeighbors[0], &auNeighbors[uNeighborCount]);

				// Use the minimum neighbor for the id.
				SetConnectivityId(uX, uY, auNeighbors[0]);

				// Insert all pairs.
				for (UInt32 i = 1u; i < uNeighborCount; ++i)
				{
					InsertPair(vRemap, auNeighbors[0], auNeighbors[i]);
				}
			}
		}
	}

	// Second pass, resolve the remap table, in lowest to highest
	// order, so that each chain formed in the table becomes length 1
	// (it is a direct mapping from itself to its lowest union.
	//
	// We start at index 3, as no lower index can have a remap
	// chain of greater than length 1.
	for (UInt32 uId = 3u; uId < uCurrentId; ++uId)
	{
		// No remap, continue.
		UInt32 uRemap = vRemap[uId];
		if (uRemap == QueryConfig::kuNoConnectivityId)
		{
			continue;
		}

		// Loop until we don't get a better value.
		while (vRemap[uRemap] != QueryConfig::kuNoConnectivityId)
		{
			uRemap = vRemap[uRemap];
		}

		// Update the remap.
		vRemap[uId] = uRemap;
	}

	// Third pass, apply fixups.
	for (UInt32 uY = 0u; uY < uHeight; ++uY)
	{
		for (UInt32 uX = 0u; uX < uWidth; ++uX)
		{
			UInt32 const uOldId = GetConnectivityId(uX, uY);
			UInt32 const uNewId = vRemap[uOldId];

			// No remap from old to new, skip it.
			if (uNewId == QueryConfig::kuNoConnectivityId)
			{
				continue;
			}

			// Update the id.
			SetConnectivityId(uX, uY, uNewId);
		}
	}
}

/** StraightPath utility, removes inner Waypoints on straight runs. */
void Query::PruneInnerWaypointsStraight(QueryState& rState) const
{
	// Early out if only 2 waypoints.
	if (rState.m_vWaypoints.GetSize() <= 2u)
	{
		return;
	}

	// Iterate and remove any inner waypoint for which
	// the signed delta between previous and current is
	// the same.
	UInt32 const uWaypoints = rState.m_vWaypoints.GetSize();
	UInt32 uInWaypoint = 2u;
	UInt32 uOutWaypoint = 1u;
	Position prev = rState.m_vWaypoints[0u];
	Position cur = rState.m_vWaypoints[1u];
	Int32 iPrevDeltaX = ((Int32)cur.m_uX - (Int32)prev.m_uX);
	Int32 iPrevDeltaY = ((Int32)cur.m_uY - (Int32)prev.m_uY);

	while (uInWaypoint < uWaypoints)
	{
		Position const next = rState.m_vWaypoints[uInWaypoint];
		Int32 const iDeltaX = ((Int32)next.m_uX - (Int32)cur.m_uX);
		Int32 const iDeltaY = ((Int32)next.m_uY - (Int32)cur.m_uY);

		// Delta is different, keep this waypoint.
		if (iDeltaX != iPrevDeltaX || iDeltaY != iPrevDeltaY)
		{
			prev = cur;
			rState.m_vWaypoints[uOutWaypoint++] = cur;
		}

		// Prev direction set to current.
		iPrevDeltaX = iDeltaX;
		iPrevDeltaY = iDeltaY;

		// In all cases, cur = next and advance in.
		cur = next;
		++uInWaypoint;
	}

	// Always append the end point.
	SEOUL_ASSERT(uOutWaypoint < rState.m_vWaypoints.GetSize());
	rState.m_vWaypoints[uOutWaypoint++] = rState.m_vWaypoints.Back();

	// Resize waypoints based on uOutWaypoint
	rState.m_vWaypoints.Resize(uOutWaypoint);
}

/** StraightPath utility, removes inner Waypoints based on ray tests. */
void Query::PruneInnerWaypointsRayTest(QueryState& rState) const
{
	// Early out if only 2 waypoints.
	if (rState.m_vWaypoints.GetSize() <= 2u)
	{
		return;
	}

	// Now smooth the path with the following algorithm
	// O(n*d), where d is the average distance between
	// two adjacent waypoints, the cost of a raytest,
	// and n is the number of waypoints in the path.
	UInt32 const uWaypoints = rState.m_vWaypoints.GetSize();
	UInt32 uInWaypoint = 2u;
	UInt32 uOutWaypoint = 1u;
	Position prev = rState.m_vWaypoints[0u];
	Position cur = rState.m_vWaypoints[1u];

	Position unusedHit;
	while (uInWaypoint < uWaypoints)
	{
		Position const next = rState.m_vWaypoints[uInWaypoint];

		// RayTest between prev and next - if this fails,
		// cur gets commited and prev becomes cur.
		if (RayTest(rState, prev, next, true, unusedHit))
		{
			prev = cur;
			rState.m_vWaypoints[uOutWaypoint++] = cur;
		}

		// In all cases, cur = next and advance in.
		cur = next;
		++uInWaypoint;
	}

	// Always append the end point.
	SEOUL_ASSERT(uOutWaypoint < rState.m_vWaypoints.GetSize());
	rState.m_vWaypoints[uOutWaypoint++] = rState.m_vWaypoints.Back();

	// Resize waypoints based on uOutWaypoint
	rState.m_vWaypoints.Resize(uOutWaypoint);
}

/** FindNearestConnected utility, resolves in a consideration. */
void Query::ResolveNearestConnected(
	Int32 iX,
	Int32 iY,
	UInt32 uConnectedId,
	const Position& connectedTo,
	Position& rNearest,
	UInt32& ruBestDistance) const
{
	if (GetConnectivityIdSafe(iX, iY) == uConnectedId)
	{
		Position const position((UInt32)iX, (UInt32)iY);
		UInt32 const uDistance = JumpPointSearch::Manhattan(connectedTo, position);
		if (uDistance < ruBestDistance)
		{
			rNearest = position;
			ruBestDistance = uDistance;
		}
	}
}

/** ComputeConnectivity utility, manages possible neighbors of an implied cell. */
void Query::ResolveNeighbor(UInt32 uNeighborX, UInt32 uNeighborY, UInt32 auNeighbors[4], UInt32& ruNeighborCount) const
{
	// Get the neighbor's id - if no connectivity, it is impassable,
	// so skip it.
	UInt32 const uNeighborId = GetConnectivityId(uNeighborX, uNeighborY);
	if (uNeighborId == QueryConfig::kuNoConnectivityId)
	{
		return;
	}

	// Iterate over already found neighbor ids - if found, skip it.
	for (UInt32 i = 0u; i < ruNeighborCount; ++i)
	{
		if (auNeighbors[i] == uNeighborId)
		{
			return;
		}
	}

	// Otherwise, add the id as a neighbor.
	auNeighbors[ruNeighborCount++] = uNeighborId;
}

/**
 * Aggressive version of FindStraightPath, meant
 * as a convenience utility/typical path find
 * implementation for most applications.
 *
 * uMaxStartDistance is the maximum distance an alternative
 * start position can be from an impassable start value.
 *
 * Pre: same as FindStraightPath()
 *
 * This method returns partial paths to an impassable end
 * and seeks a passable start from an impassable start within
 * uMaxStartDistance. If both start/end are passable but not
 * connected, this method returns false.
 */
Bool Query::RobustFindStraightPath(
	QueryState& rState,
	const Position& start,
	const Position& end,
	UInt32 uMaxStartDistance,
	UInt32 uMaxEndDistance) const
{
	// Try a standard FindStraightPath first. This will either succeed,
	// fail quickly (if connectivity is enabled), or fail slowly,
	// but with no fallback (since all our recovery options require
	// connectivity).
	Bool const bSuccess = FindStraightPath(rState, start, end);

	// Done if successful.
	if (bSuccess)
	{
		return true;
	}

	// On failure, try to find a new start and end.

	// No new start, overall failure.
	Position newStart;
	if (!FindNearest(rState, start, uMaxStartDistance, newStart))
	{
		return false;
	}

	// No new end, overall failure.
	Position newEnd;
	if (!FindNearestConnected(rState, end, uMaxEndDistance, newStart, newEnd))
	{
		return false;
	}

	// Now issue the final fallback find with the new start and end.
	return FindStraightPath(
		rState,
		newStart,
		newEnd);
}

/** Update connectivity info at (x, y). Pre: (x, y) must be on the grid. */
void Query::SetConnectivityId(UInt32 uX, UInt32 uY, UInt32 uId)
{
	SEOUL_ASSERT(uX < m_Grid.GetWidth());
	SEOUL_ASSERT(uY < m_Grid.GetHeight());

	m_pConnectivity[uY * m_Grid.GetWidth() + uX] = uId;
}

} // namespace Seoul::Navigation

#endif // /#if SEOUL_WITH_NAVIGATION
