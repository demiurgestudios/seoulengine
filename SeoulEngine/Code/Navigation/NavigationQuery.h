/**
 * \file NavigationQuery.h
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

#pragma once
#ifndef NAVIGATION_QUERY_H
#define NAVIGATION_QUERY_H

#include "Prereqs.h"
namespace Seoul { namespace Navigation { class Grid; } }
namespace Seoul { namespace Navigation { class Path; } }
namespace Seoul { namespace Navigation { struct Position; } }
namespace Seoul { namespace Navigation { class QueryState; } }

#if SEOUL_WITH_NAVIGATION

namespace Seoul::Navigation
{

namespace QueryConfig
{

/** Special index that means "no connectivity". Blocker cells. */
static const UInt32 kuNoConnectivityId = 0u;

/** Query control flags - keep in-sync with values in Navlib.h. */
enum Enum
{
	/** No special options. */
	kNone = 0u,

	/**
		* By default, a Query builds a connectivity
		* graph based on its masks. This requires (width * height * 4)
		* bytes of memory and is an O(n) operation on mutations
		* (query construction and all calls to OnDirty()).
		*
		* This flag disables connectivity. Doing so avoids the costs
		* mentioned above, by causes all calls to IsConnected() and
		* FindNearestConnected() to return false. FindPath()
		* will also be more expensive (when connectivity is available,
		* FindPath() uses the connectivity info to avoid a search
		* when requested points are not connected).
		*/
	kDisableConnectivity = (1 << 0u),
};

} // namespace QueryConfig

/**
 * Utility class to issue queries against a Grid.
 *
 * Note on thread-safety:
 * - FindPath(), RayTest(), and const methods (in general) are
 *   thread safe, if the referenced Grid is not mutated
 *   while they are called.
 * - As a result, it is the client's responsibility to ensure
 *   mutations to a referenced navigation grid, and calls to
 *   Query::OnDirty(), are synchronized and exclusive
 *   from Query query calls.
 *
 * Rough usage outline:
 * - One shared Grid instance.
 * - Several Query instances, one for each mask and optimization
 *   settings.
 * - Many QueryState instances, likely, one for each entity
 *   that will use a Query instance.
 *
 * Mutable grids:
 * - mutations to a Grid must be synchronized/exclusive from
 *   Query query methods.
 * - it is the client's responsibility to call Query::OnDirty()
 *   after mutations to the grid, prior to further Query queries.
 *   - calling queries prior to OnDirty() will not crash, but results may
 *     be stale or otherwise not reflect mutations to the grid. As a result,
 *     it can be useful to batch mutations to minimize the frequency of
 *     OnDirty() calls.
 */
class Query SEOUL_SEALED
{
public:
	/**
	 * A Query describes a single mask against a Grid.
	 * It is the client's responsibility to keep the referenced Grid
	 * in memory for the lifespan of this Query.
	 *
	 * Mutable grids, it is the responsibility of the client to call OnDirty()
	 * whenever the referenced grid is mutated. It is also necessary to synchronize
	 * mutations of the grid and calls to OnDirty() so they are mutually exclusive
	 * from query calls (e.g. RayTest). See class notes on thread-safety.
	 *
	 * @param grid Grid against which queries are issued.
	 * @param uFlags Control flags, enable/disable optimization modes or assumptions
	 *               that can be made against the grid.
	 * @param uBlockerMask A grid cell is considered passable if the cell's value &
	 *                     this mask == 0u. This can be used to treat individual cell
	 *                     bits as conditional blockers (e.g. bit 0 blocks pathing for
	 *                     characters, bit 1 blocks pathing for vehicles, and bit 2
	 *                     blocks line-of-sight for all entities).
	 * @param uForcePassableMask For a grid cell, (uForcePassableMask & uCellValu) != 0u
	 *                           means that cell is always considered passable. This can
	 *                           be used to explicitly override passability that would
	 *                           otherwise be considered blocking (e.g. bit 0 defines
	 *                           passability as determined by 3D terrain height, and bit 1
	 *                           overrides passability based on designer specified constraints).
	 */
	Query(
		const Grid& grid,
		UInt16 uFlags = QueryConfig::kNone,
		UInt8 uBlockerMask = 0xFF,
		UInt8 uForcePassableMask = 0u);
	~Query();

	// Find the nearest candidate point
	// (based on this query's masks) to
	// start, at a max of uMaxDistance,
	// which is connected to connectedTo.
	//
	// Fails if connectivity information does
	// not exist for this query.
	//
	// On success, outputs the found point.
	Bool FindNearestConnected(
		QueryState& rState,
		const Position& start,
		UInt32 uMaxDistance,
		const Position& connectedTo,
		Position& rNearest) const;

	// Find the nearest candidate point
	// (based on this query's masks) to
	// start, at a max of uMaxDistance.
	//
	// On success, outputs the found point.
	Bool FindNearest(
		QueryState& rState,
		const Position& start,
		UInt32 uMaxDistance,
		Position& rNearest) const;

	// Find a path from
	// [start, end] and write the
	// result to rState on success.
	// Pre: start/end must be on
	// the grid.
	Bool FindPath(
		QueryState& rState,
		const Position& start,
		const Position& end) const;

	// Find a "straight" path from
	// [start, end] and write the result
	// to rState on success.
	// Pre: start/end must be on
	// the grid.
	Bool FindStraightPath(
		QueryState& rState,
		const Position& start,
		const Position& end) const;

	/** Get the grid this query is attached to. */
	const Grid& GetGrid() const
	{
		return m_Grid;
	}

	/** Return true if connectivity is enabled for this query (the default), false otherwise. */
	Bool HasConnectivity() const { return (0u == (m_uFlags & QueryConfig::kDisableConnectivity)); }

	/**
	 * If HasConnectivity() is false, this method always returns false.
	 *
	 * Otherwise, return true if start has a path/connection
	 * to end.
	 */
	Bool IsConnected(
		QueryState& rState,
		const Position& start,
		const Position& end) const;

	/**
	 * Test used in all cases to determine if a grid cell
	 * is walkable/non-colliding/non-line-of-sight-blocking,
	 * etc.
	 */
	Bool IsPassable(UInt8 uValue) const
	{
		return
			(0u == (m_uBlockerMask & uValue)) ||
			(0u != (m_uForcePassableMask & uValue));
	}

	// Return true or false if a particular grid cell is passable.
	Bool IsPassable(const Position& position) const;

	/** Call when a referenced Grid is mutated. */
	void OnDirty();

	// Cast a ray against the grid,
	// from [start, end]
	// inclusive. Pre: start/end must
	// be on the grid.
	Bool RayTest(
		QueryState& rState,
		const Position& start,
		const Position& end,
		Bool bHitStartingCell,
		Position& rHit) const;

	// Aggressive version of FindStraightPath, meant
	// as a convenience utility/typical path find
	// implementation for most applications. Behaves
	// as follows:
	// - an impassable start will result in a
	//   FindNearest() call within uMaxStartDistance,
	//   to find an alternative starting position.
	// - partial paths will be returned.
	Bool RobustFindStraightPath(
		QueryState& rState,
		const Position& start,
		const Position& end,
		UInt32 uMaxStartDistance,
		UInt32 uMaxEndDistance) const;

private:
	const Grid& m_Grid;
	UInt16 const m_uFlags;
	UInt8 const m_uBlockerMask;
	UInt8 const m_uForcePassableMask;
	UInt32* const m_pConnectivity;

	void ComputeConnectivity();

	// Return the connectivity info at (x, y). Pre: (x, y) must be on the grid.
	UInt32 GetConnectivityId(UInt32 uX, UInt32 uY) const;

	// Return the connectivity info at (x, y). Checked, returns kuNoConnectivityId for out-of-bounds locations.
	UInt32 GetConnectivityIdSafe(Int32 iX, Int32 iY) const;

	// StraightPath utility, removes inner Waypoints on straight runs.
	void PruneInnerWaypointsStraight(QueryState& rState) const;

	// StraightPath utility, removes inner Waypoints based on ray tests.
	void PruneInnerWaypointsRayTest(QueryState& rState) const;

	// FindNearestConnected utility, resolves in a consideration.
	void ResolveNearestConnected(
		Int32 iX,
		Int32 iY,
		UInt32 uConnectedId,
		const Position& connectedTo,
		Position& rNearest,
		UInt32& ruBestDistance) const;

	// ComputeConnectivity utility, manages possible neighbors of an implied cell.
	void ResolveNeighbor(UInt32 uNeighborX, UInt32 uNeighborY, UInt32 auNeighbors[4], UInt32& ruNeighborCount) const;

	// Update the connectivity info at (x, y). Pre: (x, y) must be on the grid.
	void SetConnectivityId(UInt32 uX, UInt32 uY, UInt32 uId);

	SEOUL_DISABLE_COPY(Query);
}; // class Query

} // namespace Seoul::Navigation

#endif // /#if SEOUL_WITH_NAVIGATION

#endif // include guard
