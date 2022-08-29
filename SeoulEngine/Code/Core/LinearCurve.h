/**
 * \file LinearCurve.h
 * \brief LinearCurve is constructed from a sorted array of
 * data points. Each data point consists of a t (the x-axis
 * of the curve, typically time) and a value (the y-axis of the curve).
 * t is always a Float a and the value can be any type for which
 * the global Lerp() function is defined.
 *
 * LinearCurve, while typically more accurate than SimpleCurve,
 * is also typically more expensive to evaluate at some value t.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef LINEAR_CURVE_H
#define LINEAR_CURVE_H

#include "Prereqs.h"
#include "SeoulMath.h"
#include "Vector.h"

namespace Seoul
{

template <typename T, Int MEMORY_BUDGETS = MemoryBudgets::TBD>
struct LinearCurve SEOUL_SEALED
{
	typedef Vector<Float, MEMORY_BUDGETS> Times;
	typedef Vector<T, MEMORY_BUDGETS> Values;
	typedef T ValueType;

	/**
	 * @return The first T value defined in the set of points in this LinearCurve. Expected to be the min.
	 */
	Float GetFirstT() const
	{
		return (m_vTimes.IsEmpty() ? 0.0f : m_vTimes.Front());
	}

	/**
	 * @return The highest T value defined in the set of points in this LinearCurve. Expected to be the max.
	 */
	Float GetLastT() const
	{
		return (m_vTimes.IsEmpty() ? 0.0f : m_vTimes.Back());
	}

	/**
	 * Evaluates this LinearCurve, returning the dependent point on the
	 * curve at alpha value fT.
	 *
	 * @param[in] fT is a value to evaluate the curve at. It will be clamped to the min/max values of
	 * the set of points that were used to define this LinearCurve.
	 */
	Bool Evaluate(Float fT, T& r) const
	{
		// Sanity check that samples have been constructed as expected.
		if (m_vTimes.GetSize() != m_vValues.GetSize())
		{
			return false;
		}

		// Early out if we have no samples.
		if (m_vTimes.IsEmpty())
		{
			return false;
		}

		// Cache the previous sample and the number of sample.
		Float fPrevious = m_vTimes[0];
		UInt32 const n = m_vTimes.GetSize();

		// O(n) walk the samples and return an interpolated value between the appropriate samples.
		for (UInt32 i = 1u; i < n; ++i)
		{
			Float fCurrent = m_vTimes[i];

			// If we've hit the sample we fall within, evaluate it and return.
			if (fT <= fCurrent)
			{
				// Lerp the previous and current values.
				T const ret = Lerp(m_vValues[i-1], m_vValues[i], Clamp((fT - fPrevious) / (fCurrent - fPrevious), 0.0f, 1.0f));
				r = ret;
				return true;
			}

			// Iterate.
			fPrevious = fCurrent;
		}

		// If we get here, it means the curve has 1 sample or fT is out of range
		// of the end of the curve, so just return the last value.
		r = m_vValues.Back();
		return true;
	}

	/** Array of times of samples in m_vValues - expected to be in ascending order. */
	Times m_vTimes;

	/** Array of values - this Vector<> must be the same size as m_vTimes. */
	Values m_vValues;
}; // struct LinearCurve

/**
 * Utility used to evaluate a curve object in a repeating fashion
 * (when fT reaches the end of the range of the curve, it wraps
 * back around to the beginning of the range).
 */
template <typename T>
class RepeatingCurveEvaluator SEOUL_SEALED
{
public:
	RepeatingCurveEvaluator(T& r)
		: m_r(r)
		, m_fT(0.0f)
	{
	}

	/**
	 * @return The current T value.
	 */
	Float GetT() const
	{
		return m_fT;
	}

	/**
	 * Set the current T value.
	 */
	void SetT(Float fT)
	{
		Float const fFirstT = m_r.GetFirstT();
		Float const fLastT = m_r.GetLastT();

		if (fFirstT == fLastT)
		{
			m_fT = 0.0f;
		}
		else
		{
			m_fT = Fmod(fT - fFirstT, (fLastT - fFirstT)) + fFirstT;
		}
	}

	/**
	 * @return The value at the current T value.
	 */
	Bool Evaluate(typename T::ValueType& r) const
	{
		return m_r.Evaluate(m_fT, r);
	}

private:
	T& m_r;
	Float m_fT;

	SEOUL_DISABLE_COPY(RepeatingCurveEvaluator);
}; // class RepeatingCurveEvaluator

/**
 * Utility used to evaluate a curve object in a non-repeating fashion
 * (when fT reaches the end of the range of the curve, it rails
 * at the last value in the curve's range).
 */
template <typename T>
class NonRepeatingCurveEvaluator SEOUL_SEALED
{
public:
	NonRepeatingCurveEvaluator(T& r)
		: m_r(r)
		, m_fT(0.0f)
	{
	}

	/**
	 * @return The current T value.
	 */
	Float GetT() const
	{
		return m_fT;
	}

	/**
	 * Set the current T value.
	 */
	void SetT(Float fT)
	{
		Float const fFirstT = m_r.GetFirstT();
		Float const fLastT = m_r.GetLastT();

		if (fFirstT == fLastT)
		{
			m_fT = 0.0f;
		}
		else
		{
			m_fT = fT > fLastT ? fLastT : fT;
		}
	}

	/**
	 * @return The value at the current T value.
	 */
	Bool Evaluate(typename T::ValueType& r) const
	{
		return m_r.Evaluate(m_fT, r);
	}

private:
	T& m_r;
	Float m_fT;

	SEOUL_DISABLE_COPY(NonRepeatingCurveEvaluator);
}; // class NonRepeatingCurveEvaluator

} // namespace Seoul

#endif // include guard
