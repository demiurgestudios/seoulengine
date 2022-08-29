/**
 * \file ScriptNetworkExtrapolator.cpp
 * \brief Binder instance for exposing a NetworkExtrapolator
 * instance to script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "NetworkExtrapolator.h"
#include "ReflectionDefine.h"
#include "ScriptNetworkExtrapolator.h"

#if SEOUL_WITH_NETWORK

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptNetworkExtrapolator, TypeFlags::kDisableCopy)
	SEOUL_METHOD(ExtrapolateAt)
	SEOUL_METHOD(GetSettings)
	SEOUL_METHOD(SetServerTickNow)
	SEOUL_METHOD(SetSettings)
SEOUL_END_TYPE()

ScriptNetworkExtrapolator::ScriptNetworkExtrapolator()
	: m_pNetworkExtrapolator(SEOUL_NEW(MemoryBudgets::Network) Network::Extrapolator)
{
}

ScriptNetworkExtrapolator::~ScriptNetworkExtrapolator()
{
}

/** Get an extrapolated value at specified game time in ticks. */
Int32 ScriptNetworkExtrapolator::ExtrapolateAt(Int64 iAtGameTimeInTicks, const Network::ExtrapolatedValue32& v) const
{
	return m_pNetworkExtrapolator->ExtrapolateAt(iAtGameTimeInTicks, v);
}

/** Current settings of the extrapolator. */
const Network::ExtrapolatorSettings& ScriptNetworkExtrapolator::GetSettings() const
{
	return m_pNetworkExtrapolator->GetSettings();
}

/** Update the server ticks and conversion at the current game time. */
void ScriptNetworkExtrapolator::SetServerTickNow(UInt32 uServerTick, UInt32 uMillisecondsPerServerTick)
{
	Int64 const iGameTimeInTicks = SeoulTime::GetGameTimeInTicks();

	Network::ExtrapolatorSettings settings;
	settings.m_iCorrelatedClientGameTimeInClientTicks = iGameTimeInTicks;
	settings.m_uBaseServerTick = uServerTick;
	settings.m_uMillisecondsPerServerTick = uMillisecondsPerServerTick;
	SetSettings(settings);
}

/** Update the current extrapolator settings. */
void ScriptNetworkExtrapolator::SetSettings(const Network::ExtrapolatorSettings& settings)
{
	m_pNetworkExtrapolator->SetSettings(settings);
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_NETWORK
