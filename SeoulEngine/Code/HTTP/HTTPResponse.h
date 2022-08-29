/**
 * \file HTTPResponse.h
 * \brief Class that encapsulates the results of an HTTP::Request client request.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "HTTPHeaderTable.h"
#include "HTTPStats.h"
#include "Prereqs.h"
#include "SeoulTime.h"

namespace Seoul::HTTP
{

class Response SEOUL_SEALED
{
public:
	/** Gets the HTTP status code */
	Int GetStatus() const { return m_nStatus; }
	/** Gets the raw response body */
	const void* GetBody() const { return m_pBody; }
	/** Gets the response size */
	UInt32 GetBodySize() const { return m_zBodySize; }
	/** True if received body data was truncated due to a fix size buffer, data should be assumed incorrect. */
	Bool BodyDataWasTruncated() const { return m_bBodyDataTruncated; }
	/** True if file writing was enabled and written successfully, false otherwise. */
	Bool WasBodyFileWrittenSuccessfully() const { return m_bBodyFileWrittenSuccessfully; }
	/** An empty string if no redirection occured, or the final redirect URL of the request. */
	const String& GetRedirectURL() const { return m_sRedirectURL; }
	/** @return The time in seconds of the total request network transfer time. */
	Double GetRoundTripTimeInSeconds() const { return m_Stats.m_fTotalRequestSecs; }
	/** @return The engine GetUptime() value when the response was received. */
	TimeInterval GetUptimeValueAtReceive() const { return m_UptimeValueAtReceive; }

	/** @return The table of headers returned with the response. */
	const HeaderTable& GetHeaders() const
	{
		return m_tHeaders;
	}

	/** @return The stats tracked through to the end of the completed request. */
	const Stats& GetStats() const { return m_Stats; }

private:
	friend class Manager;
	friend class Request;
	friend class RequestHelper;

	// Only HTTP::Request can construct HTTP::Response instances
	Response();
	~Response();

	/** Table of parsed headers returned with the response. */
	HeaderTable m_tHeaders;

	/** Final stats of the entire request sequence. */
	Stats m_Stats;

	TimeInterval m_UptimeValueAtReceive;
	/** HTTP status code */
	Atomic32Value<Int32> m_nStatus;
	/** Empty string or the final redirect URL of a request */
	String m_sRedirectURL;
	Bool m_bBodyFileWrittenSuccessfully;

	/** Raw response body */
	void DeallocateBody();
	Bool AppendData(void const* pData, size_t zInDataSizeInBytes);
	void SetBodyOutputBuffer(void* pBuffer, UInt32 uBufferSizeInBytes);
	void* m_pBody;
	UInt32 m_zBodySize;
	UInt32 m_zBodyCapacity;
	Bool m_bOwnsBody;
	Bool m_bBodyDataTruncated;

	SEOUL_DISABLE_COPY(Response);
}; // class HTTP::Response

} // namespace Seoul::HTTP

#endif // include guard
