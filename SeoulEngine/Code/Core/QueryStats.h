/**
 * \file QueryStats.h
 * \brief Helper struct used to track spatial query data for profiling
 * purposes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef QUERY_STATS_H
#define QUERY_STATS_H

#include "Prereqs.h"

namespace Seoul
{

/**
 * Helper structure, used for tracking various stats for profiling
 *  purposes while querying a scene.
 */
struct QueryStats SEOUL_SEALED
{
	static QueryStats Create()
	{
		QueryStats ret;
		ret.m_nTotalQueriesIssued = 0u;
		ret.m_nTotalObjectsTouched = 0u;
		ret.m_nTotalNodesTouched = 0u;
		ret.m_nTotalObjectsPassed = 0u;
		return ret;
	}

	QueryStats operator+(const QueryStats& b) const
	{
		QueryStats ret;

		ret.m_nTotalQueriesIssued = m_nTotalQueriesIssued + b.m_nTotalQueriesIssued;
		ret.m_nTotalObjectsTouched = m_nTotalObjectsTouched + b.m_nTotalObjectsTouched;
		ret.m_nTotalNodesTouched = m_nTotalNodesTouched + b.m_nTotalNodesTouched;
		ret.m_nTotalObjectsPassed = m_nTotalObjectsPassed + b.m_nTotalObjectsPassed;
		return ret;
	}

	QueryStats& operator+=(const QueryStats& b)
	{
		m_nTotalQueriesIssued += b.m_nTotalQueriesIssued;
		m_nTotalObjectsTouched += b.m_nTotalObjectsTouched;
		m_nTotalNodesTouched +=  b.m_nTotalNodesTouched;
		m_nTotalObjectsPassed += b.m_nTotalObjectsPassed;
		return *this;
	}

	/** Used to accumulate queries issued. It is expected that each
	 *  call to a Query() method which returns a QueryStats structure will
	 *  initialize this value to 1, so accumulating QueryStats structures
	 *  will accumulate the total query count.
	 */
	UInt32 m_nTotalQueriesIssued;

	/** Total objects checked against a query shape. Will be >=
	 *  number of objects that passed query checks.
	 */
	UInt32 m_nTotalObjectsTouched;

	/** Number of nodes whose objects were checked against a query shape.
	 *  Nodes correspond to nodes in tree structures or other subdivision elements
	 *  in non-tree structures.
	 */
	UInt32 m_nTotalNodesTouched;

	/** Number of objects that passed the test against a query
	 *  shape.
	 */
	UInt32 m_nTotalObjectsPassed;
}; // struct QueryStats

} // namespace Seoul

#endif // include guard
