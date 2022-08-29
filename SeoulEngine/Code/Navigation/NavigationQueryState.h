/**
 * \file NavigationQueryState.h
 * \brief A query state is the specific mutable data structure
 * that is necessary to query a navigation grid. Each querying entity
 * must have its own QueryState instance, which will reference
 * a (shared) Query instance, which finally references a (shared)
 * Grid instance. Query and Grid are immutable and thread-safe,
 * whereas a QueryState contains mutation data and is not thread-safe.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NAVIGATION_QUERY_STATE_H
#define NAVIGATION_QUERY_STATE_H

#include "NavigationGrid.h"
#include "NavigationJumpPointSearch.h"
#include "NavigationPosition.h"
#include "NavigationQuery.h"
#include "Prereqs.h"

#if SEOUL_WITH_NAVIGATION

namespace Seoul::Navigation
{

class QueryPathBinder
{
public:
	QueryPathBinder(const Query& query)
		: m_Grid(query.GetGrid())
		, m_Query(query)
		, m_uWidth(m_Grid.GetWidth())
		, m_uHeight(m_Grid.GetHeight())
	{
	}

	Bool operator()(UInt32 uX, UInt32 uY) const
	{
		if (uX < m_uWidth && uY < m_uHeight)
		{
			UInt8 const uValue = m_Grid.GetCell(uX, uY);
			return m_Query.IsPassable(uValue);
		}

		return false;
	}

private:
	const Grid& m_Grid;
	const Query& m_Query;
	UInt32 const m_uWidth;
	UInt32 const m_uHeight;

	SEOUL_DISABLE_COPY(QueryPathBinder);
}; // class QueryPathBinder

class QueryState SEOUL_SEALED
{
public:
	QueryState();
	~QueryState();

	JumpPointSearch::Searcher<QueryPathBinder> m_Searcher;
	Positions m_vWaypoints;

private:
	SEOUL_DISABLE_COPY(QueryState);
}; // class QueryState

} // namespace Seoul::Navigation

#endif // /#if SEOUL_WITH_NAVIGATION

#endif // include guard
