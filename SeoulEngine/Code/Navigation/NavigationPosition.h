/**
 * \file NavigationPosition.h
 * \brief A single 2D point on a navigation grid.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NAVIGATION_POSITION_H
#define NAVIGATION_POSITION_H

#include "HashFunctions.h"
#include "Prereqs.h"
#include "SeoulMath.h"
#include "Vector.h"

#if SEOUL_WITH_NAVIGATION

namespace Seoul
{

namespace Navigation
{

struct Position SEOUL_SEALED
{
	static Position Invalid()
	{
		return Position(UIntMax, UIntMax);
	}

	UInt32 m_uX;
	UInt32 m_uY;

	Position(UInt32 uX = 0u, UInt32 uY = 0u)
		: m_uX(uX)
		, m_uY(uY)
	{
	}

	Bool operator==(const Position& b) const
	{
		return (m_uX == b.m_uX && m_uY == b.m_uY);
	}

	Bool operator!=(const Position& b) const
	{
		return !(*this == b);
	}

	Bool operator<(const Position& b) const
	{
		return m_uY < b.m_uY || (m_uY == b.m_uY && m_uX < b.m_uX);
	}

	UInt32 GetHash() const
	{
		UInt32 uHash = 0u;
		IncrementalHash(uHash, Seoul::GetHash(m_uX));
		IncrementalHash(uHash, Seoul::GetHash(m_uY));
		return uHash;
	}

	Bool IsValid() const
	{
		return (UIntMax != m_uX);
	}
}; // struct Position

} // namespace Navigation

template <> struct CanMemCpy<Navigation::Position> { static const Bool Value = true; };
template <> struct CanZeroInit<Navigation::Position> { static const Bool Value = true; };

namespace Navigation
{

static inline UInt32 GetHash(const Position& position)
{
	return position.GetHash();
}

} // namespace Navigation

template <>
struct DefaultHashTableKeyTraits<Navigation::Position>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static Navigation::Position GetNullKey()
	{
		return Navigation::Position::Invalid();
	}

	static const Bool kCheckHashBeforeEquals = false;
};

namespace Navigation
{

typedef Vector<Position, MemoryBudgets::Navigation> Positions;

} // namespace Navigation

} // namespace Seoul

#endif // /#if SEOUL_WITH_NAVIGATION

#endif // include guard
