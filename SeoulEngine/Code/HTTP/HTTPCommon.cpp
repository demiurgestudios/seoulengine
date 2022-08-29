/**
 * \file HTTPCommon.cpp
 * \brief Shared enums of the HTTP singleton.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HTTPCommon.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(HTTP::Result)
	SEOUL_ENUM_N("Success", HTTP::Result::kSuccess)
	SEOUL_ENUM_N("Failure", HTTP::Result::kFailure)
	SEOUL_ENUM_N("Canceled", HTTP::Result::kCanceled)
	SEOUL_ENUM_N("ConnectFailure", HTTP::Result::kConnectFailure)
SEOUL_END_ENUM()

SEOUL_BEGIN_ENUM(HTTP::Status)
	SEOUL_ENUM_N("OK", HTTP::Status::kOK)
	SEOUL_ENUM_N("PartialContent", HTTP::Status::kPartialContent)
	SEOUL_ENUM_N("BadRequest", HTTP::Status::kBadRequest)
	SEOUL_ENUM_N("NotFound", HTTP::Status::kNotFound)
SEOUL_END_ENUM()

namespace HTTP
{

/**
 * HTTP methods.  See http://tools.ietf.org/html/rfc2616#section-5.1.1 for more
 * detailed descriptions.
 */
namespace Method
{

const HString ksConnect("CONNECT");
const HString ksDelete("DELETE");
const HString ksGet("GET");
const HString ksHead("HEAD");
const HString ksPost("POST");
const HString ksPut("PUT");
const HString ksOptions("OPTIONS");
const HString ksTrace("TRACE");

} // namespace HTTP::Method

} // namespace HTTP

} // namespace Seoul
