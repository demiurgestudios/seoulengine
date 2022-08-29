/**
 * \file ScopedAction.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCOPED_ACTION_H
#define SCOPED_ACTION_H

#include "Prereqs.h"

namespace Seoul
{

/**
 * Simple helper class that allows a pair of functions to
 * be registered such that one is executed on construction and the
 * other on destruction of this instance - implements exception safe,
 * return safe lock/unlock semantics.
 *
 * Both template arguments T and U must be functor types that implement
 * operator Bool(), such that when this operator returns true, the functor
 * can be invoked.
 */
template <typename T, typename U>
class ScopedAction SEOUL_SEALED
{
public:
	ScopedAction(T onConstruct, U onDestruct)
		: m_OnConstruct(onConstruct)
		, m_OnDestruct(onDestruct)
	{
		m_OnConstruct();
	}

	ScopedAction(ScopedAction&& b) = default;
	ScopedAction& operator=(ScopedAction&& b) = default;

	~ScopedAction()
	{
		m_OnDestruct();
	}

private:
	T m_OnConstruct;
	U m_OnDestruct;

	SEOUL_DISABLE_COPY(ScopedAction);
}; // class ScopedAction

namespace ScopedActionDetail
{

static inline void Nop()
{
}

} // namespace ScopedActionDetail

template <typename T, typename U>
ScopedAction<T, U> MakeScopedAction(const T& onConstruct, const U& onDestruct)
{
	return ScopedAction<T, U>(onConstruct, onDestruct);
}

/**
 * Convenience on ScopedAction. Roughly equivalent to the Go 'defer' keyword,
 * except that the deferred action respects scopes (whereas the Go 'defer' keyword
 * is always scoped to the enclosing function).
 *
 * Example: auto const deferred(MakeDeferredAction([]() { DoAThing(); }));
 */
template <typename T>
ScopedAction<void (*)(), T> MakeDeferredAction(const T& onDefer)
{
	return ScopedAction<void(*)(), T>(ScopedActionDetail::Nop, onDefer);
}

} // namespace Seoul

#endif // include guard
