/**
 * \file Pair.h
 * \brief Templated struct utility for combining 2 distinct types
 * into a pair of types. Roughly equivalent to std::pair<>.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PAIR_H
#define PAIR_H

#include "SeoulTypes.h"

namespace Seoul
{

/**
 * Pair structure
 *
 * Container class for a generic pair of objects.  This structure is useful when
 * you want to pass or return two values without having to jump through hoops.
 */
template <typename T1, typename T2>
struct Pair SEOUL_SEALED
{
	T1 First;
	T2 Second;

	Pair()
		: First()
		, Second()
	{
	}

	/**
	 * Constructs a Pair with a given pair of values.
	 *
	 * @param[in] first  First value in pair
	 * @param[in] second Second value in pair
	 */
	Pair(const T1& first, const T2& second)
		: First(first)
		, Second(second)
	{
	}

	Pair(const Pair& b)
		: First(b.First)
		, Second(b.Second)
	{
	}

	Pair(Pair&& b)
		: First(RvalRef(b.First))
		, Second(RvalRef(b.Second))
	{
	}

	template <typename U1, typename U2>
	Pair(const Pair<U1, U2>& b)
		: First(b.First)
		, Second(b.Second)
	{
	}

	Pair& operator=(const Pair& other)
	{
		if (this != &other)
		{
			First = other.First;
			Second = other.Second;
		}

		return *this;
	}

	Pair& operator=(Pair&& other)
	{
		if (this != &other)
		{
			First = RvalRef(other.First);
			Second = RvalRef(other.Second);
		}

		return *this;
	}

	template <typename U1, typename U2>
	Pair& operator=(const Pair<U1, U2>& other)
	{
		if (this != &other)
		{
			First = other.First;
			Second = other.Second;
		}

		return *this;
	}

	/**
	 * Tests this pair for equality with another pair.
	 *
	 * @param[in] b Pair to compare against this pair
	 * @return True if rhs is equal to this, or false if rhs is unequal
	 *         to this
	 */
	Bool operator==(const Pair& b) const
	{
		return (First == b.First && Second == b.Second);
	}

	/**
	 * Tests this pair for inequality with another pair.
	 *
	 * @param[in] b Pair to compare against this pair
	 * @return True if rhs is unequal to this, or false if rhs is equal
	 *         to this
	 */
	Bool operator!=(const Pair& b) const
	{
		return !(*this == b);
	}
}; // struct Pair

/**
 * Helper function for making a Pair structure
 *
 * Helper function for making a Pair structure.  This obviates the need to
 * specify template parameters when constructing a Pair, since the compiler can
 * deduce template parameters for a function but not for a constructor, e.g.

   \code
	Int x = 3;
	String s = "Hello, world";

	Pair<Int, String> p = MakePair(x, s);
	// Instead of:
	// Pair<Int, String> p = Pair<Int, String>(x, s);
   \endcode
 */
template <typename T1, typename T2>
Pair<T1, T2> MakePair(T1 First, T2 Second)
{
	return Pair<T1, T2>(First, Second);
}

template <typename T1, typename T2>
struct DefaultHashTableKeyTraits< Pair<T1, T2> >
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static Pair<T1, T2> GetNullKey()
	{
		return Pair<T1, T2>(DefaultHashTableKeyTraits<T1>::GetNullKey(), DefaultHashTableKeyTraits<T2>::GetNullKey());
	}

	static const Bool kCheckHashBeforeEquals = false;
};

} // namespace Seoul

#endif // include guard
