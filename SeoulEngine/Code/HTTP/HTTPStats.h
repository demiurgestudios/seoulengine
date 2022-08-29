/**
 * \file HTTPStats.h
 * \brief Stats used for tracking HTTP request behavior.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	HTTP_STATS_H
#define HTTP_STATS_H

#include "Prereqs.h"
#include "SeoulString.h"

namespace Seoul::HTTP
{

struct Stats SEOUL_SEALED
{
	Double m_fApiDelaySecs{};
	Double m_fLookupSecs{};
	Double m_fConnectSecs{};
	Double m_fAppConnectSecs{};
	Double m_fPreTransferSecs{};
	Double m_fRedirectSecs{};
	Double m_fStartTransferSecs{};
	Double m_fTotalRequestSecs{};
	Double m_fOverallSecs{};
	Double m_fAverageDownloadSpeedBytesPerSec{};
	Double m_fAverageUploadSpeedBytesPerSec{};
	UInt32 m_uResends{};
	UInt32 m_uNetworkFailures{};
	UInt32 m_uHttpFailures{};
	String m_sRequestTraceId{};
};

} // namespace Seoul::HTTP

#endif // include guard
