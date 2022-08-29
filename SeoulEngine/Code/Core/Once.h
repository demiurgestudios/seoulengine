/**
 * \file Once.h
 * \brief Implements a C++ class equivalent to (e.g.) pthread_once().
 * Call with a delegate and that delegate will be called once
 * and only once (Once::Call() will only invoke once ever even if
 * invoked by multiple threads).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ONCE_H
#define ONCE_H

#include "Atomic32.h"
#include "Prereqs.h"

namespace Seoul
{

class Once SEOUL_SEALED
{
public:
	Once()
		: m_b(false)
	{
	}

	~Once()
	{
	}

	template <typename FUNC>
	void Call(FUNC f)
	{
		// If we fail to set false to true, don't call.
		if (false != m_b.CompareAndSet(true, false))
		{
			return;
		}

		// Otherwise, we're the once - invoke.
		f();
	}

private:
	SEOUL_DISABLE_COPY(Once);

	Atomic32Value<Bool> m_b;
}; // class Once;

} // namespace Seoul

#endif // include guard
