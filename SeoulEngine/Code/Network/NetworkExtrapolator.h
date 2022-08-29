/**
 * \file NetworkExtrapolator.h
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

#pragma once
#ifndef NETWORK_EXTRAPOLATOR_H
#define NETWORK_EXTRAPOLATOR_H

#include "Vector.h"

#if SEOUL_WITH_NETWORK

namespace Seoul
{

namespace Network
{

/** All config values that fully define an extrapolator state. */
struct ExtrapolatorSettings SEOUL_SEALED
{
	ExtrapolatorSettings()
		: m_iCorrelatedClientGameTimeInClientTicks(0)
		, m_uBaseServerTick(0u)
		, m_uMillisecondsPerServerTick(0u)
	{
	}

	/** Seoul::GetGameTimeInTicks() that correlates to the specified base server tick. */
	Int64 m_iCorrelatedClientGameTimeInClientTicks;

	/** Server tick that corresponds to the specified correlated game time. */
	UInt32 m_uBaseServerTick;

	/** Conversion from server tick to a time in milliseconds. */
	UInt32 m_uMillisecondsPerServerTick;
}; // struct ExtrapolatorSettings

/** Single value with sufficient data to extrapolate to future values. */
struct ExtrapolatedSample32 SEOUL_SEALED
{
	ExtrapolatedSample32()
		: m_iValue(0)
		, m_uTick(0u)
	{
	}

	Int32 m_iValue;
	UInt32 m_uTick;
}; // struct ExtrapolatedSample32

} // namespace Network

template <> struct CanMemCpy<Network::ExtrapolatedSample32> { static const Bool Value = true; };
template <> struct CanZeroInit<Network::ExtrapolatedSample32> { static const Bool Value = true; };

namespace Network
{

struct ExtrapolatedValue32 SEOUL_SEALED
{
	typedef Vector<ExtrapolatedSample32, MemoryBudgets::Network> Samples;
	Samples m_vSamples;
}; // struct ExtrapolatedValue32

/** Implements network synchronized value extrapolation. */
class Extrapolator SEOUL_SEALED
{
public:
	Extrapolator();
	Extrapolator(const ExtrapolatorSettings& settings);
	~Extrapolator();

	// Compute an extrapolation of v at the specified game time.
	Int32 ExtrapolateAt(Int64 iAtGameTimeInTicks, const ExtrapolatedValue32& v) const;

	/** @return The current extrapolator settings. */
	const ExtrapolatorSettings& GetSettings() const
	{
		return m_Settings;
	}

	/** Update the extrapolator configuration. */
	void SetSettings(const ExtrapolatorSettings& settings)
	{
		m_Settings = settings;
	}

private:
	ExtrapolatorSettings m_Settings;

	UInt32 ConvertClientTicksToServerTick(Int64 iGameTimeInTicks) const;
	Int64 ConvertServerTickToClientTicks(UInt32 uServerTick) const;

	SEOUL_DISABLE_COPY(Extrapolator);
}; // class Extrapolator

} // namespace Network

} // namespace Seoul

#endif // /#if SEOUL_WITH_NETWORK

#endif // include guard
