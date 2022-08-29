/**
 * \file HTTPTest.h
 * \brief Tests of the HTTP library client functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef HTTP_TEST_H
#define HTTP_TEST_H

#include "Delegate.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
namespace Seoul { namespace HTTP { class Request; } }
namespace Seoul { namespace HTTP { class Response; } }
namespace Seoul { namespace HTTP { class Server; } }
namespace Seoul { namespace HTTP { struct ServerRequestInfo; } }
namespace Seoul { namespace HTTP { class ServerResponseWriter; } }
namespace Seoul { class UnitTestsEngineHelper; }

namespace Seoul
{

#if SEOUL_UNIT_TESTS

/**
 * Text fixture class for threads.
 */
class HTTPTest SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(HTTPTest);

	HTTPTest();
	~HTTPTest();

	void TestBasic();
	void TestBasicOffMainThread();
	void TestBodyFileBasic();
	void TestBodyFileBasicFail();
	void TestBodyFileResume();
	void TestBodyOutputBufferExact();
	void TestBodyOutputBufferTooSmall();
	void TestBodyOutputBufferResend();
	void TestLanesSimple();
	void TestLanesSimpleOffMainThread();
	void TestManyRequestsShortTimeout();
	void TestResponseHeaders();
	void TestStress();
	void TestStressOffMainThread();
	void TestStressBackground();
	void TestStressBackgroundOffMainThread();
	void TestNeedsResendCallback();
	void TestNeedsResendCallbackOffMainThread();
	void TestExponentialBackoff();
	void TestEnforceEarliestSendTime();
	void TestParseURLDomain();
	void TestRequestRateLimiting();
	void TestCancelRegression();
	void TestBlockingCancelAll();
	void TestShutdownInBackground();
	void TestTickInBackground();
	void TestHTTPSAttempt();
	void TestNoCallback();
	void TestProgressCallback();
	void TestMiscApi();
	void TestRangeReset();
	void TestURLEncode();
	void TestRedirect();
	void TestBadMethod();

private:
	ScopedPtr<UnitTestsEngineHelper> m_pHelper;
	ScopedPtr<HTTP::Server> m_pServer;

	typedef Delegate<Bool(HTTP::ServerResponseWriter& responseWriter, const HTTP::ServerRequestInfo& info)> HandlerDelegate;
	void CreateServer(Int iThreadCount = 1, const HandlerDelegate& responseHandler = HandlerDelegate());

	// Used by some tests - when a "prep for resend", we create
	// the server if it does not already exist. Used for retry tests.
	void CreateServerOnResendPrep(HTTP::Response*, HTTP::Request*, HTTP::Request*)
	{
		if (!m_pServer.IsValid())
		{
			CreateServer();
		}
	}

	SEOUL_DISABLE_COPY(HTTPTest);
}; // class HTTPTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
