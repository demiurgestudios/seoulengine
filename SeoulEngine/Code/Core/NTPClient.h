/**
 * \file NTPClient.h
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

#pragma once
#ifndef NTP_CLIENT_H
#define NTP_CLIENT_H

#include "ScopedPtr.h"
#include "SeoulString.h"
#include "SeoulTime.h"
namespace Seoul { class Socket; }

namespace Seoul
{

struct NTPClientSettings SEOUL_SEALED
{
	/** NTP server to connect to. */
	String m_sHostname;

	/**
	 * Timeout of query operations - highly recommended to set this to a non-zero value.
	 * Infinite timeouts may never return.
	 */
	TimeInterval m_Timeout = TimeInterval::FromSecondsInt64(1);
}; // struct NTPClientSettings

class NTPClient SEOUL_SEALED
{
public:
	NTPClient(const NTPClientSettings& settings);
	~NTPClient();

	// Issue an NTP time query. Can fail. Synchronous - blocks
	// until the network operation completes.
	Bool SyncQueryTime(WorldTime& rWorldTime);

private:
	SEOUL_DISABLE_COPY(NTPClient);

	NTPClientSettings const m_Settings;
	ScopedPtr<Socket> m_pSocket;
}; // class NTPClient

} // namespace Seoul

#endif // include guard
