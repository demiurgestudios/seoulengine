/**
 * \file VectorUtil.h
 * \brief Vector utilities header file
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef VECTOR_UTIL_H
#define VECTOR_UTIL_H

#include "SeoulMath.h"
#include "Vector.h"

namespace Seoul
{

enum class VectorSelectionType
{
	Lowest,
	LowestIncludeTies,
	Highest,
	HighestIncludeTies
};

/**
 * Given a vector of values of type T, find the index(es) of the
 * largest (or smallest) value, breaking ties by randomly selecting one
 * if Lowest or Highest is specified, or returning all the tied indices
 * if LowestIncludeTies or HighestIncludeTies is specified.
 *
 * @return true on success.
 */
template <typename T, int MEMORY_BUDGETS, int MEMORY_BUDGETS_2>
inline Bool FindValueRandomTiebreaker(
	const Vector<T, MEMORY_BUDGETS>& vData,
	VectorSelectionType eSelectionType,
	Vector<UInt, MEMORY_BUDGETS_2>& vBestIndices)
{
	vBestIndices.Clear();

	// Fail if we are given a length zero vector.
	if (vData.IsEmpty())
	{
		return false;
	}

	// Assume the zero-th entry is the best
	UInt nBestIndex = 0;
	vBestIndices.PushBack(nBestIndex);

	auto const uSize = vData.GetSize();
	for (UInt nIndex = 1; nIndex < uSize; ++nIndex)
	{
		T currValue = vData[nIndex];
		T bestValue = vData[nBestIndex];

		switch (eSelectionType)
		{
		case VectorSelectionType::Lowest:
		case VectorSelectionType::LowestIncludeTies:
		{
			if (currValue < bestValue)
			{
				nBestIndex = nIndex;
				// Strictly better - clear the "best" list
				vBestIndices.Clear();
				vBestIndices.PushBack(nIndex);
			}
			if (currValue == bestValue)
			{
				// Tie. Append to best list.
				vBestIndices.PushBack(nIndex);
			}
			break;
		}
		case VectorSelectionType::Highest:
		case VectorSelectionType::HighestIncludeTies:
		{
			if (currValue > bestValue)
			{
				nBestIndex = nIndex;
				// Strictly better - clear the "best" list
				vBestIndices.Clear();
				vBestIndices.PushBack(nIndex);
			}
			if (currValue == bestValue)
			{
				// Tie. Append to best list.
				vBestIndices.PushBack(nIndex);
			}
			break;
		}
		default:
			SEOUL_FAIL("FindValueRandomTiebreaker: Unknown VectorSelectionType");
			break;
		}
	}

	if (eSelectionType == VectorSelectionType::Lowest
		|| eSelectionType == VectorSelectionType::Highest)
	{
		// Choose one of the winners at random
		nBestIndex = vBestIndices[GlobalRandom::UniformRandomUInt32n(vBestIndices.GetSize())];
		vBestIndices.Clear();
		vBestIndices.PushBack(nBestIndex);
	}

	return true;
}

} // namespace Seoul

#endif // include guard
