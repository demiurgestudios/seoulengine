/**
 * \file NTPClient.cpp
 * \brief Utility for communicating with an NTP (time) server.
 *
 * \sa https://www.cisco.com/c/en/us/about/press/internet-protocol-journal/back-issues/table-contents-58/154-ntp.html
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "NTPClient.h"
#include "SeoulMath.h"
#include "SeoulSocket.h"

namespace Seoul
{

// See also: https://lettier.github.io/posts/2016-04-26-lets-make-a-ntp-client-in-c.html
static const UInt16 kuNtpPort = 123u;
static const UInt32 kuTimestampDelta = 2208988800u;

// If rx and tx time differ by greater than this amount, we reject the NTP value.
static const UInt64 kuMaxNtpErrorMicroseconds = 2000000u; // 2 seconds.

namespace
{

SEOUL_DEFINE_PACKED_STRUCT(struct NtpPacket
{
	// li:   2 bits. Leap indicator (we use 0 - no leap seconds adjustment).
	// vn:   3 bits. Version number of the protocol (we use version 3).
	// mode: 3 bits. Mode settings - always mode 3 (client).
	UInt8 m_uLiVnMode;

	UInt8 m_uStratum;   // Stratum level of the local clock.
	UInt8 m_uPoll;      // Maximum interval between successive messages.
	UInt8 m_uPrecision; // Precision of the local clock.

	UInt32 m_uRootDelay;      // Total round trip delay time.
	UInt32 m_uRootDispersion; // Max error allowed from primary clock source.
	UInt32 m_uRefId;          // Reference clock identifier.

	UInt32 m_uRefTimeSecs;     // Reference time-stamp seconds.
	UInt32 m_uRefTimeFraction; // Reference time-stamp fraction of a second.

	UInt32 m_uOrigTimeSecs;     // Originate time-stamp seconds.
	UInt32 m_uOrigTimeFraction; // Originate time-stamp fraction of a second.

	UInt32 m_uRxTimeSecs;     // Received time-stamp seconds.
	UInt32 m_uRxTimeFraction; // Received time-stamp fraction of a second.

	UInt32 m_uTxTimeSecs;     // Transmit time-stamp seconds.
	UInt32 m_uTxTimeFraction; // Transmit time-stamp fraction of a second.
});

static void EndianSwap(NtpPacket& r)
{
	r.m_uRootDelay = EndianSwap32(r.m_uRootDelay);
	r.m_uRootDispersion = EndianSwap32(r.m_uRootDispersion);
	r.m_uRefId = EndianSwap32(r.m_uRefId);
	r.m_uRefTimeSecs = EndianSwap32(r.m_uRefTimeSecs);
	r.m_uRefTimeFraction = EndianSwap32(r.m_uRefTimeFraction);
	r.m_uOrigTimeSecs = EndianSwap32(r.m_uOrigTimeSecs);
	r.m_uOrigTimeFraction = EndianSwap32(r.m_uOrigTimeFraction);
	r.m_uRxTimeSecs = EndianSwap32(r.m_uRxTimeSecs);
	r.m_uRxTimeFraction = EndianSwap32(r.m_uRxTimeFraction);
	r.m_uTxTimeSecs = EndianSwap32(r.m_uTxTimeSecs);
	r.m_uTxTimeFraction = EndianSwap32(r.m_uTxTimeFraction);
}

static inline UInt64 GetMicroseconds(UInt32 uSeconds, UInt32 uFraction)
{
	// Sanity, catch bad values.
	if (uSeconds < kuTimestampDelta)
	{
		return 0;
	}

	auto const uSecondsUnixUtc = (uSeconds - kuTimestampDelta);

	UInt64 const uMicroseconds =
		((UInt64)uSecondsUnixUtc * (UInt64)1000000u) +
		(((UInt64)uFraction * (UInt64)1000000u) >> (UInt64)32u);

	return uMicroseconds;
}

static inline UInt64 GetRxMicroseconds(const NtpPacket& packet)
{
	return GetMicroseconds(packet.m_uRxTimeSecs, packet.m_uRxTimeFraction);
}

static inline UInt64 GetTxMicroseconds(const NtpPacket& packet)
{
	return GetMicroseconds(packet.m_uTxTimeSecs, packet.m_uTxTimeFraction);
}

static inline void SetTimeInMicroseconds(UInt64 uMicroseconds, UInt32& ruSeconds, UInt32& ruFraction)
{
	ruSeconds = (UInt32)((uMicroseconds / (UInt64)1000000u) + (UInt64)kuTimestampDelta);
	ruFraction = (UInt32)((uMicroseconds << (UInt64)32u) / (UInt64)1000000u);
}

static inline void SetOrigTimeInMicroseconds(UInt64 uMicroseconds, NtpPacket& packet)
{
	SetTimeInMicroseconds(uMicroseconds, packet.m_uOrigTimeSecs, packet.m_uOrigTimeFraction);
}

static void InitializeClientPackaet(UInt64 uOrigTimeInMicroseconds, NtpPacket& r)
{
	// Zero out.
	memset(&r, 0, sizeof(r));

	// Set li = 0 (2 bits), vn = 3 (3 bits), and mode = 3 (3 bits).
	r.m_uLiVnMode = 0b00011011;

	// Set originator time.
	SetOrigTimeInMicroseconds(uOrigTimeInMicroseconds, r);

	// Done for client.
}

} // namespace anonymous

static inline SocketSettings ToSocketSettings(const NTPClientSettings& settings)
{
	SocketSettings ret;
	ret.m_ReceiveTimeout = settings.m_Timeout;
	return ret;
}

NTPClient::NTPClient(const NTPClientSettings& settings)
	: m_Settings(settings)
	, m_pSocket(SEOUL_NEW(MemoryBudgets::Network) Socket(ToSocketSettings(settings)))
{
	(void)m_pSocket->Connect(Socket::kUDP, m_Settings.m_sHostname, kuNtpPort);
}

NTPClient::~NTPClient()
{
}

/**
 * Issue an NTP time query. Can fail. Synchronous - blocks
 * until the network operation completes.
 */
Bool NTPClient::SyncQueryTime(WorldTime& rWorldTime)
{
	// Attempt to reconnect if not connected.
	if (!m_pSocket->IsConnected())
	{
		if (!m_pSocket->Connect(Socket::kUDP, m_Settings.m_sHostname, kuNtpPort))
		{
			return false;
		}
	}

	// Start time.
	auto const uStartTimeInMicroseconds = (UInt64)(SeoulTime::ConvertTicksToMilliseconds(SeoulTime::GetGameTimeInTicks()) * 1000.0);

	// Initialize.
	NtpPacket packet;
	InitializeClientPackaet(uStartTimeInMicroseconds, packet);

	// Send the data.
	auto zSize = m_pSocket->SendAll(&packet, sizeof(packet));
	if (zSize != sizeof(packet))
	{
		return false;
	}

	// Receive response.
	zSize = m_pSocket->ReceiveAll(&packet, sizeof(packet));
	if (zSize != sizeof(packet))
	{
		return false;
	}

	// End time.
	auto const uEndTimeInMicroseconds = (UInt64)(SeoulTime::ConvertTicksToMilliseconds(SeoulTime::GetGameTimeInTicks()) * 1000.0);

	// Endian swap fields - network order is big endian.
#if SEOUL_LITTLE_ENDIAN
	EndianSwap(packet);
#endif

	// Get server time values.
	auto const uRxMicroseconds = GetRxMicroseconds(packet);
	auto const uTxMicroseconds = GetTxMicroseconds(packet);
	// Sanity, catch bad values.
	if (0 == uRxMicroseconds || 0 == uTxMicroseconds)
	{
		return false;
	}
	// Sanity, transmit must always be >= receive or something weird happened.
	if (uTxMicroseconds < uRxMicroseconds)
	{
		return false;
	}
	// Sanity, catch too wide an interval (TODO: Do some digging for how wide this
	// should really be and when can it happen - this is filtering for cases where the NTP
	// value is completely wrong).
	if (uTxMicroseconds - uRxMicroseconds > kuMaxNtpErrorMicroseconds)
	{
		return false;
	}

	// Adjust based on transmission delay.
	auto const iAdjustment = Max(
		(Int64)0,
		((Int64)uEndTimeInMicroseconds - (Int64)uStartTimeInMicroseconds) - ((Int64)uTxMicroseconds - (Int64)uRxMicroseconds));
	
	// The estimation between tx time and current time is half the delay.
	rWorldTime = WorldTime::FromMicroseconds(uTxMicroseconds + (UInt64)(iAdjustment / (Int64)2));
	return true;
}

} // namespace Seoul
