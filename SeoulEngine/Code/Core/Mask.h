/**
 * \file Mask.h
 * \brief Represents a bit flag that can be used to find intersection
 * between subtypes of a type and to represent sets of subtypes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MASK_H
#define MASK_H

#include "HashTable.h"
#include "Prereqs.h"

#include <limits>

namespace Seoul
{

/**
 * Mask<>  is a helper structure to map HString sub-type
 * identifiers to bit flags.
 *
 * All HString identifiers *must* be registered with RegisterGroup()
 * before they can be used when instantiating or manipulating Mask<> specialization
 * objects. This is to allow Mask<> to be thread-safe.
 *
 * \warning It is NOT safe to global Mask objects. Mask depends on other complex
 * types and static globals and can instantiate incorrectly if used as
 * a global or const global.
 */
template <typename T>
struct Mask SEOUL_SEALED
{
public:
	typedef UInt32 InternalType;

	static const Mask kAllGroups;
	static const Mask kNoGroups;

	/**
	 * Adds a group to the static global table of group names that
	 * can be used in Mask::Add and Mask::Remove methods.
	 *
	 * This function should be called at game start with the
	 * group names that will be used for the lifetime of the game and then
	 * the Mask table should not be modified.
	 */
	static void RegisterGroup(HString groupName)
	{
		InternalType bit = 0u;
		if (!s_tGroups.GetValue(groupName, bit))
		{
			bit = s_tGroups.GetSize();
			if (bit < (InternalType)std::numeric_limits<InternalType>::digits)
			{
				s_tGroups.Insert(groupName, bit);
			}
			else
			{
				SEOUL_FAIL(String::Printf("Too many Mask groups, failed when "
					"trying to add (%s)", groupName.CStr()).CStr());
			}
		}
	}

	Mask()
		: m_Mask(0u)
	{}

	Mask(const Mask& mask)
		: m_Mask(mask.m_Mask)
	{}

	explicit Mask(HString groupName)
		: m_Mask(0u)
	{
		Add(groupName);
	}

	Mask(HString groupName0, HString groupName1)
		: m_Mask(0u)
	{
		Add(groupName0);
		Add(groupName1);
	}

	Mask(HString groupName0, HString groupName1, HString groupName2)
		: m_Mask(0u)
	{
		Add(groupName0);
		Add(groupName1);
		Add(groupName2);
	}

	Mask(
		HString groupName0,
		HString groupName1,
		HString groupName2,
		HString groupName3)
		: m_Mask(0u)
	{
		Add(groupName0);
		Add(groupName1);
		Add(groupName2);
		Add(groupName3);
	}

	Mask& operator=(const Mask& mask)
	{
		m_Mask = mask.m_Mask;
		return *this;
	}

	/**
	 * Adds a poseable group to this mask.
	 *
	 * Equivalent to mask |= (1 << groupBit).
	 *
	 * \warning This will fail with an assertion in non-ship builds
	 * if you attempt to use a groupName that has not been registered.
	 * This behavior is necessary to keep member functions of Mask
	 * thread-safe. In ship builds, this method will silently fail
	 * to add the groupName to the mask if it has not been previously registered.
	 *
	 * \warning groupName is case sensitive.
	 */
	void Add(HString groupName)
	{
		InternalType bit = 0u;
		if (!s_tGroups.GetValue(groupName, bit))
		{
			SEOUL_FAIL(String::Printf("Mask group (%s) was not registered, "
				"you must call Mask::RegisterGroup() on an HString "
				"group name before attempting to use it with Mask.",
				groupName.CStr()).CStr());
		}
		else
		{
			m_Mask |= (1 << bit);
		}
	}

	/**
	 * Removes a group from this mask.
	 * Equivalent to (mask &= ~(1 << groupBit)).
	 */
	void Remove(HString groupName)
	{
		InternalType bit = 0u;
		if (s_tGroups.GetValue(groupName, bit))
		{
			m_Mask &= ~(1 << bit);
		}
	}

	/**
	 * Add the groups defined in mask to this
	 * Mask.
	 */
	void Add(const Mask& mask)
	{
		m_Mask |= mask.GetInternalMask();
	}

	/**
	 * Remove the poseable defined in mask from this
	 *  Mask.
	 */
	void Remove(const Mask& mask)
	{
		m_Mask &= ~mask.GetInternalMask();
	}

	/**
	 * Get this object's bit vector as a UInt32 bit vector.
	 */
	InternalType GetInternalMask() const
	{
		return m_Mask;
	}

	Bool operator==(Mask b) const
	{
		return m_Mask == b.m_Mask;
	}

	Bool operator!=(Mask b) const
	{
		return m_Mask != b.m_Mask;
	}

	/**
	 * True if a and b are completely disjoint, false otherwise.
	 */
	static Bool Disjoint(Mask a, Mask b)
	{
		return ((a.m_Mask & b.m_Mask) == 0u);
	}

	/**
	 * True if a and b share at least one bit, false otherwise.
	 */
	static Bool Intersect(Mask a, Mask b)
	{
		return ((a.m_Mask & b.m_Mask) != 0u);
	}

	/**
	 * Returns true if a contains more groups than the
	 * groups in b.
	 */
	static Bool ContainsMoreThan(Mask a, Mask b)
	{
		return ((a.m_Mask & ~b.m_Mask) != 0u);
	}

private:
	Mask(InternalType mask)
		: m_Mask(mask)
	{}

	InternalType m_Mask;

private:
	typedef HashTable<HString, InternalType> Groups;
	static Groups s_tGroups;
}; // struct Mask

} // namespace Seoul

#endif // include guard
