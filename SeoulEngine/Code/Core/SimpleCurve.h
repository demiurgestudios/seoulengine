/**
 * \file SimpleCurve.h
 * \brief SimpleCurve uses a Piecewise Linear (PWL) approximation to
 * a source curve, with a compile-time known number of equal sized
 * linear pieces.
 *
 * As a result, evaluating a SimpleCurve is O(1) and the
 * implementation of SimpleCurve can be heavily inlined and optimized
 * by the compiler.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SIMPLE_CURVE_H
#define SIMPLE_CURVE_H

#include "Prereqs.h"
#include "SeoulMath.h"

namespace Seoul
{

/**
 * A SimpleCurve can be used to approximate almost any linear
 * or non-linear curve. It uses a PWL approximation to the source curve,
 * with a fixed number of linear segment of equal size. As a result,
 * a SimpleCurve::Evaluate() call is O(1).
 */
template <typename T, UInt32 SAMPLE_COUNT = 32u>
class SimpleCurve SEOUL_SEALED
{
public:
	SimpleCurve()
#if SEOUL_DEBUG
		: m_bHasBeenSet(false)
#endif
	{
		memset(m_aSamples, 0, SAMPLE_COUNT * sizeof(T));
	}

	SimpleCurve(const SimpleCurve& b)
#if SEOUL_DEBUG
		: m_bHasBeenSet(b.m_bHasBeenSet)
#endif
	{
		memcpy(m_aSamples, b.m_aSamples, SAMPLE_COUNT * sizeof(T));
	}

	SimpleCurve& operator=(const SimpleCurve& b)
	{
		memcpy(m_aSamples, b.m_aSamples, SAMPLE_COUNT * sizeof(T));
#if SEOUL_DEBUG
		m_bHasBeenSet = b.m_bHasBeenSet;
#endif
		return *this;
	}

	/**
	 * Populate this SimpleCurve using the delegate populateDelegate
	 * and the source curve dataSource.
	 */
	template <typename U>
	void Set(T (*populateDelegate)(const U&, Float fT), const U& dataSource)
	{
		SEOUL_ASSERT(populateDelegate);

		for (UInt32 i = 0u; i < SAMPLE_COUNT; ++i)
		{
			const Float fT = ((Float)i / (SAMPLE_COUNT - 1u));
			m_aSamples[i] = populateDelegate(dataSource, fT);
		}

#if SEOUL_DEBUG
		m_bHasBeenSet = true;
#endif
	}

	/** @return Const iterator over the curve samples. */
	T const* Begin() const { return m_aSamples; }
	T const* End() const { return ((T const*)m_aSamples) + SAMPLE_COUNT; }

	/** ranged for support. */
	T const* begin() const { return m_aSamples; }
	T const* end() const { return ((T const*)m_aSamples) + SAMPLE_COUNT; }

	/**
	 * Gets the first sample of this SimpleCurve.
	 *
	 * If SimpleCurve::Set() has not been called on this SimpleCurve,
	 * the return value is undefined.
	 */
	T GetFirst() const
	{
		return m_aSamples[0u];
	}

	/**
	 * Gets the last sample of this SimpleCurve.
	 *
	 * If SimpleCurve::Set() has not been called on this SimpleCurve,
	 * the return value is undefined.
	 */
	T GetLast() const
	{
		return m_aSamples[SAMPLE_COUNT - 1u];
	}

	/**
	 * Evaluates this SimpleCurve, returning the dependent point on the
	 * curve at alpha value fT.
	 *
	 * @param[in] fT Value on [0.0, 1.0] to evaluate the curve at.
	 *
	 * \pre fT is >= 0.0f and <= 1.0f.
	 *
	 * If SimpleCurve::Set() has not been called on this SimpleCurve,
	 * the results of this evaluation are undefined.
	 */
	T Evaluate(Float fT) const
	{
		SEOUL_ASSERT_DEBUG(m_bHasBeenSet);
		SEOUL_ASSERT(fT >= 0.0f && fT <= 1.0f);

		const Float fIndex = (fT * (SAMPLE_COUNT - 1u));
		const Int i0 = (Int)FastFloor(fIndex);
		const Int i1 = (Int)FastCeil(fIndex);

		return Lerp(m_aSamples[i0], m_aSamples[i1], fIndex - (Float)i0);
	}

private:
	T m_aSamples[SAMPLE_COUNT];

#if SEOUL_DEBUG
	Bool m_bHasBeenSet;
#endif
}; // class SimpleCurve

} // namespace Seoul

#endif // include guard
