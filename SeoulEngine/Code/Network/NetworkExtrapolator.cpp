/**
 * \file NetworkExtrapolator.cpp
 * \brief Network::Extrapolator implements value extrapolation
 * based on a synchronized client/server tick.
 *
 * Network::ExtrapolatedValue is applied to an extrapolator
 * to determine the current value at any given time.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "NetworkExtrapolator.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "SeoulTime.h"

#if SEOUL_WITH_NETWORK

namespace Seoul
{

SEOUL_SPEC_TEMPLATE_TYPE(Vector<Network::ExtrapolatedSample32, 27>)
SEOUL_BEGIN_TYPE(Network::ExtrapolatorSettings)
	SEOUL_PROPERTY_N("CorrelatedClientGameTimeInClientTicks", m_iCorrelatedClientGameTimeInClientTicks)
	SEOUL_PROPERTY_N("BaseServerTick", m_uBaseServerTick)
	SEOUL_PROPERTY_N("MillisecondsPerServerTick", m_uMillisecondsPerServerTick)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Network::ExtrapolatedSample32)
	SEOUL_PROPERTY_N("value", m_iValue)
	SEOUL_PROPERTY_N("tick", m_uTick)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Network::ExtrapolatedValue32)
	SEOUL_PROPERTY_N("samples", m_vSamples)
SEOUL_END_TYPE()

namespace Network
{

Extrapolator::Extrapolator()
	: m_Settings()
{
}

Extrapolator::Extrapolator(const ExtrapolatorSettings& settings)
	: m_Settings(settings)
{
}

Extrapolator::~Extrapolator()
{
}

/**
 * Compute an extrapolation of v at iAtGameTimeInTicks.
 *
 * @return The extrapolated value or 0 if v has no samples.
 */
Int32 Extrapolator::ExtrapolateAt(
	Int64 iAtGameTimeInTicks,
	const ExtrapolatedValue32& v) const
{
	// Get samples
	const ExtrapolatedValue32::Samples& vSamples = v.m_vSamples;

	// No points.
	if (vSamples.IsEmpty())
	{
		return 0;
	}

	// Convert the client time to a server tick.
	UInt32 const uAtServerTick = ConvertClientTicksToServerTick(iAtGameTimeInTicks);

	// Before or after, use front or back.
	if (uAtServerTick <= vSamples.Front().m_uTick)
	{
		// First value, prior to first sample.
		return vSamples.Front().m_iValue;
	}
	else if (uAtServerTick >= vSamples.Back().m_uTick)
	{
		// Last value, after the last sample.
		return vSamples.Back().m_iValue;
	}

	// Find endpoints and interpolate.
	ExtrapolatedSample32 a = vSamples.Front();
	ExtrapolatedSample32 b = a;

	// Linear search through samples, pick the first pair
	// that surround the target tick.
	UInt32 const uSize = vSamples.GetSize();
	for (UInt32 i = 1u; i < uSize; ++i)
	{
		if (uAtServerTick <= vSamples[i].m_uTick)
		{
			// Final sample is b.
			b = vSamples[i];
			break;
		}
		else
		{
			// First sample is at i.
			a = vSamples[i];
		}
	}

	// Apply interpolation between endpoints.
	{
		// Start and end time in client ticks.
		Int64 const iTicksA = ConvertServerTickToClientTicks(a.m_uTick);
		Int64 const iTicksB = ConvertServerTickToClientTicks(b.m_uTick);

		// Numerator is offset target client value, denominator is
		// delta in client ticks.
		Int64 const iNumerator = (iAtGameTimeInTicks - iTicksA);
		Int64 const iDenominator = (iTicksB - iTicksA);

		// Compute the return value, interpolation between
		// a and b based on factor between the two.
		Int32 const iReturn = (Int32)Lerp(
			(Double)a.m_iValue,
			(Double)b.m_iValue,
			Clamp((Double)iNumerator / (Double)iDenominator, 0.0, 1.0));

		return iReturn;
	}
}

/** Given a client time in ticks, convert it to a server tick based on current extrapolator settings. */
UInt32 Extrapolator::ConvertClientTicksToServerTick(Int64 iGameTimeInTicks) const
{
	// Rebase the client ticks.
	Int64 const iDeltaTimeInTicks = (iGameTimeInTicks - m_Settings.m_iCorrelatedClientGameTimeInClientTicks);

	// Convert the rebased values to milliseconds.
	Double const fDeltaTimeInMilliseconds = SeoulTime::ConvertTicksToMilliseconds(iDeltaTimeInTicks);

	// Offset and base to server tick, rescale and round to server tick scale based on value.
	UInt32 const uCurrentServerTick = (UInt32)((
		(fDeltaTimeInMilliseconds / (Double)m_Settings.m_uMillisecondsPerServerTick) +
		(Double)m_Settings.m_uBaseServerTick) + 0.5);

	return uCurrentServerTick;
}

/** Given a server time in ticks, convert it to a client tick based on current extrapolator settings. */
Int64 Extrapolator::ConvertServerTickToClientTicks(UInt32 uServerTick) const
{
	// Rebase and rescale to client ticks.
	return
		m_Settings.m_iCorrelatedClientGameTimeInClientTicks +
		SeoulTime::ConvertMillisecondsToTicks(((Double)uServerTick - (Double)m_Settings.m_uBaseServerTick) * (Double)m_Settings.m_uMillisecondsPerServerTick);
}

} // namespace Network

} // namespace Seoul

#endif // /#if SEOUL_WITH_NETWORK
