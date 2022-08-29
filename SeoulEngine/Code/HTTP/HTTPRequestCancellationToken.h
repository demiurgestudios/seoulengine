/**
 * \file HTTPRequestCancellationToken.h
 * \brief Wrapper handle that allows a request to be cancelled before completion.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	HTTP_REQUEST_CANCELLATION_TOKEN_H
#define HTTP_REQUEST_CANCELLATION_TOKEN_H

#include "Prereqs.h"
#include "SharedPtr.h"

namespace Seoul::HTTP
{

/**
 * Reference counted shared instance. Can be used
 * to cancel a pending HTTP request.
 *
 * Token used to cancel in-progress HTTP request. An
 * HTTP::RequestCancellationToken will be maintained through
 * resends (a cancellation token instance is valid until the
 * final callback of a request is invoked, at which point it
 * becomes a nop).
 */
class RequestCancellationToken SEOUL_SEALED
{
public:
	void Cancel();

	Bool IsCancelled() const { return m_bCancelled; }

private:
	// Private construction, can only be instantiated
	// by HTTP::Request or HTTP.
	RequestCancellationToken()
		: m_bCancelled(false)
	{
	}
	friend class Manager;
	friend class Request;

	SEOUL_DISABLE_COPY(RequestCancellationToken);
	SEOUL_REFERENCE_COUNTED(RequestCancellationToken);

	Atomic32Value<Bool> m_bCancelled;
}; // class HTTP::RequestCancellationToken

} // namespace Seoul::HTTP

#endif // include guard
