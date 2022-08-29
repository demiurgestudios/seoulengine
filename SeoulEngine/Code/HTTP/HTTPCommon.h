/**
 * \file HTTPCommon.h
 * \brief Shared enums of the HTTP singleton.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	HTTP_COMMON_H
#define HTTP_COMMON_H

#include "Prereqs.h"
#include "SeoulHString.h"
namespace Seoul { template <typename F> class Delegate; }
namespace Seoul { namespace HTTP { class Request; } }
namespace Seoul { namespace HTTP { class Response; } }

namespace Seoul::HTTP
{

/**
 * Result codes for HTTP requests, indicating whether or not the request
 * completed successfully or not.  These are not to be confused with HTTP
 * status codes (e.g. "200 OK" or "404 Not Found")
 */
enum class Result
{
	/** Request succeeded */
	kSuccess,
	/** Unspecified failure */
	kFailure,
	/** Request was canceled */
	kCanceled,
	/** Request failed to connect */
	kConnectFailure,
};

/**
 * HTTP status codes.  See http://tools.ietf.org/html/rfc2616#section-10
 * for more detailed descriptions.
 */
enum class Status : Int
{
	/** OK */
	kOK = 200,

	/** Partial Content returned on Range-Header requests in place of status OK. */
	kPartialContent = 206,

	/** Client Errors */
	kBadRequest = 400,
	kNotFound = 404,

	/* Server error */
	kInternalServerError = 500,
};

enum class CallbackResult
{
	kSuccess,
	kNeedsResend,
};

/** Callback invoked to verify data received before opening an output file for writing. */
typedef Delegate<Bool (void const* pFirstReceiedData, size_t zDataSizeInBytes)> OpenFileValidateDelegate;

/**
 * Callback invoked for resend requests, to give client code an opportunity to modify the parameters
 * of the request. They are an exact copy of the parameters used to configure the original request,
 * by default.
 */
typedef Delegate<void (Response*, Request*, Request*)> PrepForResendCallback;

/** Callback type for HTTP responses */
typedef Delegate<CallbackResult (Result, Response*)> ResponseDelegate;

/** Callback type for HTTP progress callbacks */
typedef Delegate<void (Request const*, UInt64 zDownloadSizeInBytes, UInt64 zDownloadSoFarInBytes)> ResponseProgressDelegate;

namespace Method
{

// HTTP methods.  See RFC 2616 section 5.1.1.
extern const HString ksConnect;
extern const HString ksDelete;
extern const HString ksGet;
extern const HString ksHead;
extern const HString ksPost;
extern const HString ksPut;
extern const HString ksOptions;
extern const HString ksTrace;

} // namespace HTTP::Method

} // namespace Seoul::HTTP

#endif  // include guard
