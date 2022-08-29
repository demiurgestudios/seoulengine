/**
 * \file ScopedIncrement.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCOPED_INCREMENT_H
#define SCOPED_INCREMENT_H

#include "Prereqs.h"

namespace Seoul
{

/**
 * Helper class that will increment the given number when it's constructed
 * and then decrement it when this class goes out of scope.
 **/
template <typename T>
class ScopedIncrement SEOUL_SEALED
{
public:
	explicit ScopedIncrement(T& ru)
		: m_ru(ru)
	{
		++m_ru;
	}

	~ScopedIncrement()
	{
		--m_ru;
	}

private:
	T& m_ru;
};

} // namespace Seoul

#endif // include guard
