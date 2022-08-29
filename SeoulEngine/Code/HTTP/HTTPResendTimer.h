/**
 * \file HTTPResendTimer.h
 * \brief Utility used by HTTP::Request to handle request resend scheduling
 * and pacing.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	HTTP_RESEND_TIMER_H
#define HTTP_RESEND_TIMER_H

#include "Prereqs.h"

namespace Seoul::HTTP
{

class ResendTimer SEOUL_SEALED
{
public:
	ResendTimer();
	ResendTimer(const ResendTimer&) = default;
	ResendTimer& operator=(const ResendTimer&) = default;

	// Allows apps to override the built-in defaults
	void UpdateSettings(Double fMinInterval, Double fMaxInterval, Double fBaseMultiplier, Double fRandomMultiplier);

	// Gets the next resend timeout to use, and advances its internal resend counter
	Double NextResendSeconds();
	// Resets NextResendSeconds to its initial value
	void ResetResendSeconds();

private:
	// Always wait at least this long between resends
	Double m_fMinIntervalSeconds;
	// Never wait longer than this between resends
	Double m_fMaxIntervalSeconds;
	// Every resend's base delay is this many times longer than the previous
	Double m_fIntervalBaseMultiplier;
	// Every resend interval is jittered by this factor:
	// interval + random value in range [-interval * multiplier, interval * multiplier)
	Double m_fIntervalRandomMultiplier;

	// Tracks the next delay to be returned from NextResendSeconds
	Double m_fNextIntervalInSeconds;
}; // class HTTP::ResendTimer

} // namespace Seoul::HTTP

#endif // include guard
