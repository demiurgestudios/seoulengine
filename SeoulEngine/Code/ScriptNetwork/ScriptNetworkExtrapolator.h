/**
 * \file ScriptNetworkExtrapolator.h
 * \brief Binder instance for exposing a NetworkExtrapolator
 * instance to script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_NETWORK_EXTRAPOLATOR_H
#define SCRIPT_NETWORK_EXTRAPOLATOR_H

#include "Prereqs.h"
namespace Seoul { namespace Network { class Extrapolator; } }
namespace Seoul { namespace Network { struct ExtrapolatorSettings; } }

#if SEOUL_WITH_NETWORK

namespace Seoul
{

class ScriptNetworkExtrapolator SEOUL_SEALED
{
public:
	ScriptNetworkExtrapolator();
	~ScriptNetworkExtrapolator();

	// Extrapolation access.
	Int32 ExtrapolateAt(Int64 iAtGameTimeInTicks, const Network::ExtrapolatedValue32& v) const;

	// Configuration.
	const Network::ExtrapolatorSettings& GetSettings() const;
	void SetServerTickNow(UInt32 uServerTick, UInt32 uMillisecondsPerServerTick);
	void SetSettings(const Network::ExtrapolatorSettings& settings);

private:
	ScopedPtr<Network::Extrapolator> m_pNetworkExtrapolator;

	SEOUL_DISABLE_COPY(ScriptNetworkExtrapolator);
}; // class ScriptNetworkExtrapolator

} // namespace Seoul

#endif // /#if SEOUL_WITH_NETWORK

#endif // include guard
