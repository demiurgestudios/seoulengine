/**
 * \file NavigationQueryState.cpp
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

#include "NavigationQueryState.h"

#if SEOUL_WITH_NAVIGATION

namespace Seoul::Navigation
{

QueryState::QueryState()
{
}

QueryState::~QueryState()
{
}

} // namespace Seoul::Navigation

#endif // /#if SEOUL_WITH_NAVIGATION
