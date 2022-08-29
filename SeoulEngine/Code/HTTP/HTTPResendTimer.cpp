/**
 * \file HTTPResendTimer.cpp
 * \brief Utility used by HTTP::Request to handle request resend scheduling
 * and pacing.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HTTPResendTimer.h"
#include "SeoulMath.h"

namespace Seoul::HTTP
{

/** Default min and max time between resend attempts. */
static const Double kfDefaultMinResendIntervalInSeconds = 0.5;
static const Double kfDefaultMaxResendIntervalInSeconds = 15;

static const Double kfDefaultResendIntervalBaseMultiplier = 1.5;
static const Double kfDefaultResendIntervalRandomMultiplier = 0.5;

ResendTimer::ResendTimer()
	: m_fMinIntervalSeconds(kfDefaultMinResendIntervalInSeconds)
	, m_fMaxIntervalSeconds(kfDefaultMaxResendIntervalInSeconds)
	, m_fIntervalBaseMultiplier(kfDefaultResendIntervalBaseMultiplier)
	, m_fIntervalRandomMultiplier(kfDefaultResendIntervalRandomMultiplier)
	, m_fNextIntervalInSeconds(kfDefaultMinResendIntervalInSeconds)
{
	// Call UpdateSettings to clamp values
	UpdateSettings(
		m_fMinIntervalSeconds,
		m_fMaxIntervalSeconds,
		m_fIntervalBaseMultiplier,
		m_fIntervalRandomMultiplier);
}

void ResendTimer::UpdateSettings(Double fMinInterval, Double fMaxInterval, Double fBaseMultiplier, Double fRandomMultiplier)
{
	m_fMinIntervalSeconds = Max(0.0, fMinInterval);
	m_fMaxIntervalSeconds = Max(0.0, fMaxInterval);
	m_fIntervalBaseMultiplier = Max(1.0, fBaseMultiplier);
	m_fIntervalRandomMultiplier = Max(0.0, fRandomMultiplier);

	ResetResendSeconds();
}

Double ResendTimer::NextResendSeconds()
{
	// Multiply the resend interval by a random value in the range [-multiplier, multiplier)
	Double fRandomDeltaInSeconds = m_fNextIntervalInSeconds * m_fIntervalRandomMultiplier;
	Double fRandMin = m_fNextIntervalInSeconds - fRandomDeltaInSeconds;
	Double fRandMax = m_fNextIntervalInSeconds + fRandomDeltaInSeconds;

	Double fRand = GlobalRandom::UniformRandomFloat64();
	Double fResult = Clamp(fRandMin + (fRand * (fRandMax - fRandMin)), m_fMinIntervalSeconds, m_fMaxIntervalSeconds);

	m_fNextIntervalInSeconds = Clamp(m_fNextIntervalInSeconds * m_fIntervalBaseMultiplier, m_fMinIntervalSeconds, m_fMaxIntervalSeconds);

	return fResult;
}

void ResendTimer::ResetResendSeconds()
{
	m_fNextIntervalInSeconds = m_fMinIntervalSeconds;
}

} // namespace Seoul::HTTP
