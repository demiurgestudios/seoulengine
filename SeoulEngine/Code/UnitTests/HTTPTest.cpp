/**
 * \file HTTPTest.cpp
 * \brief Tests of the HTTP library client functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Atomic64.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "HTTPServer.h"
#include "HTTPTest.h"
#include "Logger.h"
#include "MemoryBarrier.h"
#include "ReflectionDefine.h"
#include "Thread.h"
#include "ThreadId.h"
#include "UnitTesting.h"
#include "UnitTestsEngineHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(HTTPTest, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(UnitTest, Attributes::UnitTest::kInstantiateForEach) // Want Engine and other resources to be recreated for each test.
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestBasicOffMainThread)
	SEOUL_METHOD(TestBodyFileBasic)
	SEOUL_METHOD(TestBodyFileBasicFail)
	SEOUL_METHOD(TestBodyFileResume)
	SEOUL_METHOD(TestBodyOutputBufferExact)
	SEOUL_METHOD(TestBodyOutputBufferTooSmall)
	SEOUL_METHOD(TestBodyOutputBufferResend)
	SEOUL_METHOD(TestLanesSimple)
	SEOUL_METHOD(TestLanesSimpleOffMainThread)
	SEOUL_METHOD(TestManyRequestsShortTimeout)
	SEOUL_METHOD(TestResponseHeaders)
	SEOUL_METHOD(TestStress)
	SEOUL_METHOD(TestStressOffMainThread)
	SEOUL_METHOD(TestStressBackground)
	SEOUL_METHOD(TestStressBackgroundOffMainThread)
	SEOUL_METHOD(TestNeedsResendCallback)
	SEOUL_METHOD(TestNeedsResendCallbackOffMainThread)
	SEOUL_METHOD(TestExponentialBackoff)
	SEOUL_METHOD(TestEnforceEarliestSendTime)
	SEOUL_METHOD(TestParseURLDomain)
	SEOUL_METHOD(TestRequestRateLimiting)
	SEOUL_METHOD(TestCancelRegression)
	SEOUL_METHOD(TestBlockingCancelAll)
	SEOUL_METHOD(TestShutdownInBackground)
	SEOUL_METHOD(TestTickInBackground)
	SEOUL_METHOD(TestHTTPSAttempt)
	SEOUL_METHOD(TestNoCallback)
	SEOUL_METHOD(TestProgressCallback)
	SEOUL_METHOD(TestMiscApi)
	SEOUL_METHOD(TestRangeReset)
	SEOUL_METHOD(TestURLEncode)
	SEOUL_METHOD(TestRedirect)
	SEOUL_METHOD(TestBadMethod)
SEOUL_END_TYPE()

struct HTTPTestUtility SEOUL_SEALED
{
	SEOUL_DELEGATE_TARGET(HTTPTestUtility);

	HTTPTestUtility(Atomic32* pRequestCompletion = nullptr)
		: m_pRequestCompletion(pRequestCompletion)
		, m_RequestCompletionOrder(0)
		, m_eResult(HTTP::Result::kCanceled)
		, m_iStatus(0)
		, m_iRequiredStatus(-1)
		, m_vBody()
		, m_ResponseCallbackThreadId()
		, m_bBodyDataWasTruncated(false)
		, m_LastRequestTickTime(0)
		, m_bComplete(false)
	{
	}

	HTTP::CallbackResult OnComplete(HTTP::Result eResult, HTTP::Response* pResponse)
	{
		m_eResult = eResult;
		m_iStatus = pResponse->GetStatus();
		m_vBody.Resize(pResponse->GetBodySize());
		m_bBodyDataWasTruncated = pResponse->BodyDataWasTruncated();
		m_LastRequestTickTime = SeoulTime::GetGameTimeInTicks();
		m_fRoundTrip = pResponse->GetRoundTripTimeInSeconds();
		m_UpTime = pResponse->GetUptimeValueAtReceive();

		if (!m_vBody.IsEmpty())
		{
			memcpy(m_vBody.Data(), pResponse->GetBody(), m_vBody.GetSize());
		}
		m_ResponseCallbackThreadId = Thread::GetThisThreadId();

		if (m_iRequiredStatus > 0 && m_iStatus != m_iRequiredStatus)
		{
			return HTTP::CallbackResult::kNeedsResend;
		}

		// If we're tracking request completion, increment now and mark our place.
		if (nullptr != m_pRequestCompletion)
		{
			m_RequestCompletionOrder = (++(*m_pRequestCompletion)) - 1;
		}

		SeoulMemoryBarrier();
		m_bComplete = true;

		return HTTP::CallbackResult::kSuccess;
	}

	void OnProgress(HTTP::Request const*, UInt64 zDownloadSizeInBytes, UInt64 zDownloadSoFarInBytes)
	{
		++m_ProgressCalls;
		m_ProgressDownloadSizeInBytes.Set((Atomic32Type)zDownloadSizeInBytes);
		m_ProgressDownloadSoFarInBytes.Set((Atomic32Type)zDownloadSoFarInBytes);
	}

	void SetRequiredHttpStatus(Int iRequiredStatus)
	{
		m_iRequiredStatus = iRequiredStatus;
	}

	void WaitForCompletion(Float fTimeoutInSeconds)
	{
		auto const iStartTicks = SeoulTime::GetGameTimeInTicks();
		while (!m_bComplete)
		{
			// Call to hit the entry but we can't rely on it, since the requests
			// complete in a threaded manner.
			(void)HTTP::Manager::Get()->HasRequests();

			SEOUL_UNITTESTING_ASSERT(
				SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTicks) < fTimeoutInSeconds);
		}
	}

	void WaitForMainThreadCompletion(UnitTestsEngineHelper& rHelper, Float fTimeoutInSeconds)
	{
		auto const iStartTicks = SeoulTime::GetGameTimeInTicks();
		while (!m_bComplete)
		{
			// Main thread not complete means there must be some requests.
			SEOUL_UNITTESTING_ASSERT(HTTP::Manager::Get()->HasRequests());

			rHelper.Tick();

			SEOUL_UNITTESTING_ASSERT(
				SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTicks) < fTimeoutInSeconds);
		}
	}

	Atomic32* m_pRequestCompletion;
	Atomic32Type m_RequestCompletionOrder;
	HTTP::Result m_eResult;
	Int m_iStatus;
	Int m_iRequiredStatus;
	Vector<Byte, MemoryBudgets::Network> m_vBody;
	ThreadId m_ResponseCallbackThreadId;
	Bool m_bBodyDataWasTruncated;
	Atomic64Value<Int64> m_LastRequestTickTime;
	Atomic32Value<Bool> m_bComplete;
	Atomic32 m_ProgressCalls;
	Atomic32 m_ProgressDownloadSizeInBytes;
	Atomic32 m_ProgressDownloadSoFarInBytes;
	Double m_fRoundTrip{};
	TimeInterval m_UpTime{};
}; // struct HTTPTestUtility

struct HTTPTestResponseUtility SEOUL_SEALED
{
	SEOUL_DELEGATE_TARGET(HTTPTestResponseUtility);

	HTTPTestResponseUtility()
		: m_bComplete(false)
	{
	}

	HTTP::CallbackResult OnComplete(HTTP::Result eResult, HTTP::Response* pResponse)
	{
		static const String ksExpectedBody("<html><body></body></html>");

		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, pResponse->GetStatus());
		SEOUL_UNITTESTING_ASSERT_EQUAL(pResponse->GetBodySize(), ksExpectedBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ksExpectedBody, String((Byte const*)pResponse->GetBody(), pResponse->GetBodySize()));

		auto const& tHeaders = pResponse->GetHeaders();
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, tHeaders.GetKeyValues().GetSize());

		String s;
		SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("accept-ranges"), s));
		SEOUL_UNITTESTING_ASSERT_EQUAL("bytes", s);
		SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("connection"), s));
		SEOUL_UNITTESTING_ASSERT_EQUAL("close", s);
		SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("content-length"), s));
		SEOUL_UNITTESTING_ASSERT_EQUAL("26", s);
		SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("content-type"), s));
		SEOUL_UNITTESTING_ASSERT_EQUAL("text/html", s);
		SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("date"), s));
		SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("etag"), s));
		SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("last-modified"), s));

		SeoulMemoryBarrier();
		m_bComplete = true;

		return HTTP::CallbackResult::kSuccess;
	}

	void WaitForCompletion(Float fTimeoutInSeconds)
	{
		auto const iStartTicks = SeoulTime::GetGameTimeInTicks();
		while (!m_bComplete)
		{
			SEOUL_UNITTESTING_ASSERT(
				SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTicks) < fTimeoutInSeconds);
		}
	}

	void WaitForMainThreadCompletion(UnitTestsEngineHelper& rHelper, Float fTimeoutInSeconds)
	{
		auto const iStartTicks = SeoulTime::GetGameTimeInTicks();
		while (!m_bComplete)
		{
			// Main thread so we need to tick the helper while waiting.
			rHelper.Tick();

			SEOUL_UNITTESTING_ASSERT(
				SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTicks) < fTimeoutInSeconds);
		}
	}

	Atomic32Value<Bool> m_bComplete;
}; // struct HTTPTestResponseUtility

HTTPTest::HTTPTest()
	: m_pHelper()
	, m_pServer()
{
	m_pHelper.Reset(SEOUL_NEW(MemoryBudgets::Developer) UnitTestsEngineHelper);
	HTTP::Manager::Get()->EnableVerboseHttp2Logging(true);
	CreateServer();
}

HTTPTest::~HTTPTest()
{
	if (HTTP::Manager::Get())
	{
		SEOUL_UNITTESTING_ASSERT(0 == HTTP::Manager::Get()->GetNetworkFailureActiveResendRequests());
	}

	m_pServer.Reset();
	m_pHelper.Reset();
}

void HTTPTest::TestBasic()
{
	// File0.
	{
		HTTPTestUtility utility;
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
		r.SetURL("http://localhost:8057/file0.html");
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		// Wait.
		utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

		// Verify.
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);

		String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("<html><body></body></html>", sActual);
	}

	// File1.
	{
		HTTPTestUtility utility;
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
		r.SetURL("http://localhost:8057/file1.txt");
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		// Wait.
		utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

		// Verify.
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);

		String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("This is a test file.", sActual);
	}

	// No file.
	{
		HTTPTestUtility utility;
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
		r.SetURL("http://localhost:8057/this-file-does-not-exist.txt");
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		// Wait.
		utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

		// Verify.
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kNotFound, utility.m_iStatus);
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);

		String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("Error 404: Not Found\nFile not found", sActual);
	}
	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

/**
 * Same as TestBasic, except requests are issued so that callbacks do
 * not need to be delivered on the main thread. As a result, we should get
 * callbacks without ticking HTTP::Manager.
 */
void HTTPTest::TestBasicOffMainThread()
{
	// File0.
	{
		HTTPTestUtility utility;
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
		r.SetDispatchCallbackOnMainThread(false);
		r.SetURL("http://localhost:8057/file0.html");
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		// Wait.
		utility.WaitForCompletion(10.0f);

		// Verify.
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);

		String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("<html><body></body></html>", sActual);
	}

	// File1.
	{
		HTTPTestUtility utility;
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
		r.SetDispatchCallbackOnMainThread(false);
		r.SetURL("http://localhost:8057/file1.txt");
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		// Wait.
		utility.WaitForCompletion(10.0f);

		// Verify.
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);

		String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("This is a test file.", sActual);
	}

	// No file.
	{
		HTTPTestUtility utility;
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
		r.SetDispatchCallbackOnMainThread(false);
		r.SetURL("http://localhost:8057/this-file-does-not-exist.txt");
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		// Wait.
		utility.WaitForCompletion(10.0f);

		// Verify.
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kNotFound, utility.m_iStatus);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);

		String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("Error 404: Not Found\nFile not found", sActual);
	}
	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

namespace
{
	Bool OpenFileValidateFail(void const* pFirstReceiedData, size_t zDataSizeInBytes)
	{
		return false;
	}

	Bool OpenBigFileValidate(void const* pFirstReceiedData, size_t zDataSizeInBytes)
	{
		SEOUL_UNITTESTING_ASSERT(zDataSizeInBytes > 0u);
		SEOUL_UNITTESTING_ASSERT_EQUAL(52, *((Byte const*)pFirstReceiedData));
		return true;
	}
}

void HTTPTest::TestBodyFileBasic()
{
	String const sOutputFile(Path::GetTempFileAbsoluteFilename());

	HTTPTestUtility utility;
	auto& r = HTTP::Manager::Get()->CreateRequest();
	r.SetURL("http://localhost:8057/filebig.txt");
	r.SetBodyDataOutputFile(sOutputFile, false);
	r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
	r.SetIgnoreDomainRequestBudget(true);
	r.SetOpenFileValidateCallback(SEOUL_BIND_DELEGATE(OpenBigFileValidate));
	r.Start();

	utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

	SEOUL_UNITTESTING_ASSERT(utility.m_bComplete);
	SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
	SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(
		Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests", "HTTP", "filebig.txt"),
		sOutputFile));

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestBodyFileBasicFail()
{
	String const sOutputFile(Path::GetTempFileAbsoluteFilename());

	HTTPTestUtility utility;
	auto& r = HTTP::Manager::Get()->CreateRequest();
	r.SetURL("http://localhost:8057/filebig.txt");
	r.SetBodyDataOutputFile(sOutputFile, false);
	r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
	r.SetIgnoreDomainRequestBudget(true);
	r.SetOpenFileValidateCallback(SEOUL_BIND_DELEGATE(OpenFileValidateFail));
	r.SetResendOnFailure(false);
	r.Start();

	utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

	SEOUL_UNITTESTING_ASSERT(utility.m_bComplete);
	SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
	SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
	SEOUL_UNITTESTING_ASSERT(!FileManager::Get()->Exists(sOutputFile)); // Not created.

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestBodyFileResume()
{
	static const String ksOrigFilename(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests", "HTTP", "filebig.txt"));
	String const sOutputFile(Path::GetTempFileAbsoluteFilename());

	// Write out the first 100 bytes of the file to the output file.
	UInt32 uFileSize = 0u;
	{
		Vector<Byte, MemoryBudgets::Developer> vBuffer;
		vBuffer.Resize(100);
		{
			ScopedPtr<SyncFile> pFile;
			SEOUL_UNITTESTING_ASSERT(FileManager::Get()->OpenFile(ksOrigFilename, File::kRead, pFile));
			SEOUL_UNITTESTING_ASSERT_EQUAL(100, pFile->ReadRawData(vBuffer.Data(), 100));
			uFileSize = (UInt32)pFile->GetSize();
		}
		{
			ScopedPtr<SyncFile> pFile;
			SEOUL_UNITTESTING_ASSERT(FileManager::Get()->OpenFile(sOutputFile, File::kWriteTruncate, pFile));
			SEOUL_UNITTESTING_ASSERT_EQUAL(100, pFile->WriteRawData(vBuffer.Data(), 100));
		}
	}

	// Sanity check.
	SEOUL_UNITTESTING_ASSERT_LESS_THAN(0u, uFileSize);

	// Setup the request as a resume.
	HTTPTestUtility utility;
	auto& r = HTTP::Manager::Get()->CreateRequest();
	r.AddRangeHeader(100u, uFileSize - 1u);
	r.SetURL("http://localhost:8057/filebig.txt");
	r.SetBodyDataOutputFile(sOutputFile, true);
	r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
	r.SetIgnoreDomainRequestBudget(true);
	r.Start();

	utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

	SEOUL_UNITTESTING_ASSERT(utility.m_bComplete);
	SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kPartialContent, utility.m_iStatus);
	SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(
		ksOrigFilename,
		sOutputFile));

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestBodyOutputBufferExact()
{
	static const String ksFile0("<html><body></body></html>");
	static const String ksFile1("This is a test file.");
	static const String ksNoFile("Error 404: Not Found\nFile not found");

	// File0.
	{
		Vector<Byte, MemoryBudgets::Developer> vBuffer;
		vBuffer.Resize(ksFile0.GetSize());

		HTTPTestUtility utility;
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetBodyOutputBuffer(vBuffer.Data(), vBuffer.GetSizeInBytes());
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
		r.SetURL("http://localhost:8057/file0.html");
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		// Wait.
		utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

		// Verify.
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
		SEOUL_UNITTESTING_ASSERT(!utility.m_bBodyDataWasTruncated);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vBuffer.GetSize(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(vBuffer.Data(), utility.m_vBody.Data(), utility.m_vBody.GetSize()));

		String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ksFile0, sActual);
	}

	// File1.
	{
		Vector<Byte, MemoryBudgets::Developer> vBuffer;
		vBuffer.Resize(ksFile1.GetSize());

		HTTPTestUtility utility;
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetBodyOutputBuffer(vBuffer.Data(), vBuffer.GetSizeInBytes());
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
		r.SetURL("http://localhost:8057/file1.txt");
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		// Wait.
		utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

		// Verify.
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
		SEOUL_UNITTESTING_ASSERT(!utility.m_bBodyDataWasTruncated);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vBuffer.GetSize(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(vBuffer.Data(), utility.m_vBody.Data(), utility.m_vBody.GetSize()));

		String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ksFile1, sActual);
	}

	// No file.
	{
		Vector<Byte, MemoryBudgets::Developer> vBuffer;
		vBuffer.Resize(ksNoFile.GetSize());

		HTTPTestUtility utility;
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetBodyOutputBuffer(vBuffer.Data(), vBuffer.GetSizeInBytes());
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
		r.SetURL("http://localhost:8057/this-file-does-not-exist.txt");
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		// Wait.
		utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

		// Verify.
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kNotFound, utility.m_iStatus);
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
		SEOUL_UNITTESTING_ASSERT(!utility.m_bBodyDataWasTruncated);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vBuffer.GetSize(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(vBuffer.Data(), utility.m_vBody.Data(), utility.m_vBody.GetSize()));

		String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ksNoFile, sActual);
	}

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestBodyOutputBufferTooSmall()
{
	static const String ksTruncatedFile0("<html><bo");
	static const String ksTruncatedFile1("This is a");
	static const String ksTruncatedNoFile("Error 404: Not Found\nFile ");

	// File0.
	{
		Vector<Byte, MemoryBudgets::Developer> vBuffer;
		vBuffer.Resize(ksTruncatedFile0.GetSize());

		HTTPTestUtility utility;
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetBodyOutputBuffer(vBuffer.Data(), vBuffer.GetSizeInBytes());
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
		r.SetURL("http://localhost:8057/file0.html");
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		// Wait.
		utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

		// Verify.
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
		SEOUL_UNITTESTING_ASSERT(utility.m_bBodyDataWasTruncated);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vBuffer.GetSize(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(vBuffer.Data(), utility.m_vBody.Data(), utility.m_vBody.GetSize()));

		String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ksTruncatedFile0, sActual);
	}

	// File1.
	{
		Vector<Byte, MemoryBudgets::Developer> vBuffer;
		vBuffer.Resize(ksTruncatedFile1.GetSize());

		HTTPTestUtility utility;
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetBodyOutputBuffer(vBuffer.Data(), vBuffer.GetSizeInBytes());
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
		r.SetURL("http://localhost:8057/file1.txt");
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		// Wait.
		utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

		// Verify.
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
		SEOUL_UNITTESTING_ASSERT(utility.m_bBodyDataWasTruncated);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vBuffer.GetSize(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(vBuffer.Data(), utility.m_vBody.Data(), utility.m_vBody.GetSize()));

		String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ksTruncatedFile1, sActual);
	}

	// No file.
	{
		Vector<Byte, MemoryBudgets::Developer> vBuffer;
		vBuffer.Resize(ksTruncatedNoFile.GetSize());

		HTTPTestUtility utility;
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetBodyOutputBuffer(vBuffer.Data(), vBuffer.GetSizeInBytes());
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
		r.SetURL("http://localhost:8057/this-file-does-not-exist.txt");
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		// Wait.
		utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

		// Verify.
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kNotFound, utility.m_iStatus);
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
		SEOUL_UNITTESTING_ASSERT(utility.m_bBodyDataWasTruncated);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vBuffer.GetSize(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(vBuffer.Data(), utility.m_vBody.Data(), utility.m_vBody.GetSize()));

		String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ksTruncatedNoFile, sActual);
	}

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

// Regression for a bug where, if a custom
// buffer was specified to a request and the request
// was resent, that buffer would be lost in the
// resend requests and left unpopulated.
void HTTPTest::TestBodyOutputBufferResend()
{
	static const String ksFile0("<html><body></body></html>");
	static const String ksFile1("This is a test file.");
	static const String ksNoFile("Error 404: Not Found\nFile not found");

	// Reduce resend intervals to avoid edge cases
	// where several successive failures increase resend
	// and result in running out of time for the test.
	HTTP::Manager::Get()->SetResendSettings(0.1, 1, 1.5, 0.5);

	// Prior to starting request, destroy the server.
	m_pServer.Reset();

	// File0.
	{
		Vector<Byte, MemoryBudgets::Developer> vBuffer;
		vBuffer.Resize(ksFile0.GetSize());

		HTTPTestUtility utility;
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetBodyOutputBuffer(vBuffer.Data(), vBuffer.GetSizeInBytes());
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
		r.SetURL("http://localhost:8057/file0.html");
		r.SetIgnoreDomainRequestBudget(true);
		r.SetPrepForResendCallback(SEOUL_BIND_DELEGATE(&HTTPTest::CreateServerOnResendPrep, this));
		r.Start();

		// Wait.
		utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

		// Verify.
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
		SEOUL_UNITTESTING_ASSERT(!utility.m_bBodyDataWasTruncated);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vBuffer.GetSize(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(vBuffer.Data(), utility.m_vBody.Data(), utility.m_vBody.GetSize()));

		String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ksFile0, sActual);
	}

	// Prior to starting request, destroy the server.
	m_pServer.Reset();

	// File1.
	{
		Vector<Byte, MemoryBudgets::Developer> vBuffer;
		vBuffer.Resize(ksFile1.GetSize());

		HTTPTestUtility utility;
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetBodyOutputBuffer(vBuffer.Data(), vBuffer.GetSizeInBytes());
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
		r.SetURL("http://localhost:8057/file1.txt");
		r.SetIgnoreDomainRequestBudget(true);
		r.SetPrepForResendCallback(SEOUL_BIND_DELEGATE(&HTTPTest::CreateServerOnResendPrep, this));
		r.Start();

		// Wait.
		utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

		// Verify.
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
		SEOUL_UNITTESTING_ASSERT(!utility.m_bBodyDataWasTruncated);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vBuffer.GetSize(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(vBuffer.Data(), utility.m_vBody.Data(), utility.m_vBody.GetSize()));

		String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ksFile1, sActual);
	}

	// Prior to starting request, destroy the server.
	m_pServer.Reset();

	// No file.
	{
		Vector<Byte, MemoryBudgets::Developer> vBuffer;
		vBuffer.Resize(ksNoFile.GetSize());

		HTTPTestUtility utility;
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetBodyOutputBuffer(vBuffer.Data(), vBuffer.GetSizeInBytes());
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
		r.SetURL("http://localhost:8057/this-file-does-not-exist.txt");
		r.SetIgnoreDomainRequestBudget(true);
		r.SetPrepForResendCallback(SEOUL_BIND_DELEGATE(&HTTPTest::CreateServerOnResendPrep, this));
		r.Start();

		// Wait.
		utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

		// Verify.
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kNotFound, utility.m_iStatus);
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
		SEOUL_UNITTESTING_ASSERT(!utility.m_bBodyDataWasTruncated);
		SEOUL_UNITTESTING_ASSERT_EQUAL(vBuffer.GetSize(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(vBuffer.Data(), utility.m_vBody.Data(), utility.m_vBody.GetSize()));

		String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ksNoFile, sActual);
	}

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestLanesSimple()
{
	static const UInt32 kuRequestCount = 32;

	static const Byte* kasUrls[] =
	{
		"http://localhost:8057/file0.html",
		"http://localhost:8057/file1.txt",
		"http://localhost:8057/filebig.txt",
		"http://localhost:8057/not-a-file.txt",
	};

	Atomic32 tracker;
	Vector<HTTPTestUtility*, MemoryBudgets::Developer> vUtilities;
	vUtilities.Reserve(kuRequestCount);
	for (UInt32 i = 0u; i < kuRequestCount; ++i)
	{
		auto pUtility = SEOUL_NEW(MemoryBudgets::Developer) HTTPTestUtility(&tracker);
		vUtilities.PushBack(pUtility);
	}

	// Now setup requests.
	UInt32 uUrl = 0u;
	for (auto i = vUtilities.Begin(); vUtilities.End() != i; ++i)
	{
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetURL(kasUrls[uUrl]);
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, *i));
		r.SetIgnoreDomainRequestBudget(true);
		r.SetLanesMask(1 << 0);
		r.Start();

		uUrl = (uUrl + 1) % SEOUL_ARRAY_COUNT(kasUrls);
	}

	// Wait for completion.
	for (auto i = vUtilities.Begin(); vUtilities.End() != i; ++i)
	{
		(*i)->WaitForMainThreadCompletion(*m_pHelper, 10.0f);
	}

	// Verify that the order is what we expected.
	for (UInt32 u = 0u; u < vUtilities.GetSize(); ++u)
	{
		auto& utility = *vUtilities[u];
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
		SEOUL_UNITTESTING_ASSERT_EQUAL(u, utility.m_RequestCompletionOrder);

		UInt32 const uURL = (u % SEOUL_ARRAY_COUNT(kasUrls));
		if (uURL != 3u)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kNotFound, utility.m_iStatus);
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(tracker, kuRequestCount);

	// Cleanup
	SafeDeleteVector(vUtilities);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

/**
 * Same as TestLanesSimple, except requests are issued so that callbacks do
 * not need to be delivered on the main thread. As a result, we should get
 * callbacks without ticking HTTP::Manager.
 */
void HTTPTest::TestLanesSimpleOffMainThread()
{
	static const UInt32 kuRequestCount = 32;

	static const Byte* kasUrls[] =
	{
		"http://localhost:8057/file0.html",
		"http://localhost:8057/file1.txt",
		"http://localhost:8057/filebig.txt",
		"http://localhost:8057/not-a-file.txt",
	};

	Atomic32 tracker;
	Vector<HTTPTestUtility*, MemoryBudgets::Developer> vUtilities;
	vUtilities.Reserve(kuRequestCount);
	for (UInt32 i = 0u; i < kuRequestCount; ++i)
	{
		auto pUtility = SEOUL_NEW(MemoryBudgets::Developer) HTTPTestUtility(&tracker);
		vUtilities.PushBack(pUtility);
	}

	// Now setup requests.
	UInt32 uUrl = 0u;
	for (auto i = vUtilities.Begin(); vUtilities.End() != i; ++i)
	{
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetURL(kasUrls[uUrl]);
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, *i));
		r.SetDispatchCallbackOnMainThread(false);
		r.SetIgnoreDomainRequestBudget(true);
		r.SetLanesMask(1 << 0);
		r.Start();

		uUrl = (uUrl + 1) % SEOUL_ARRAY_COUNT(kasUrls);
	}

	// Wait for completion.
	for (auto i = vUtilities.Begin(); vUtilities.End() != i; ++i)
	{
		(*i)->WaitForCompletion(10.0f);
	}

	// Verify that the order is what we expected.
	for (UInt32 u = 0u; u < vUtilities.GetSize(); ++u)
	{
		auto& utility = *vUtilities[u];
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
		SEOUL_UNITTESTING_ASSERT_EQUAL(u, utility.m_RequestCompletionOrder);

		UInt32 const uURL = (u % SEOUL_ARRAY_COUNT(kasUrls));
		if (uURL != 3u)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kNotFound, utility.m_iStatus);
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(tracker, kuRequestCount);

	// Cleanup
	SafeDeleteVector(vUtilities);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

// This is a regression - at one point, libcurl had a bug when pipelining was
// enabled (pipelining tells libcurl to attempt to reuse a connection for
// multiple requests if all are to the same endpoint and pipelining is supported
// by the server, to reduce handshake overhead).
//
// This bug was caused by the connection timeout of a pipeline of requests starting
// at the start time of the first request. As a result, the last request in
// a chain would timeout always, because eventually the timeout would be reached before
// the pipeline was released.
void HTTPTest::TestManyRequestsShortTimeout()
{
	static const UInt32 kuRequest = 128u;

	// We need to give the server at least 2 threads, or our
	// blocking receive will block all connection receives.
	m_pServer.Reset();
	CreateServer(2);

	Vector<HTTPTestUtility*, MemoryBudgets::Developer> v;
	v.Reserve(kuRequest);

	for (UInt32 i = 0u; i < kuRequest; ++i)
	{
		v.PushBack(SEOUL_NEW(MemoryBudgets::Developer) HTTPTestUtility);
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, v.Back()));
		r.SetConnectionTimeout(1);
		r.SetTransferTimeout(3);
		r.SetURL("http://localhost:8057/file0.html");
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();
	}

	// Wait for requests to complete.
	for (auto p : v)
	{
		p->WaitForMainThreadCompletion(*m_pHelper, 10.0);
	}

	// Verify.
	for (auto p : v)
	{
		auto const& utility = *p;
		SEOUL_UNITTESTING_ASSERT(utility.m_bComplete);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);

		String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("<html><body></body></html>", sActual);
	}

	// Cleanup.
	SafeDeleteVector(v);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestResponseHeaders()
{
	HTTPTestResponseUtility utility;
	auto& r = HTTP::Manager::Get()->CreateRequest();
	r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestResponseUtility::OnComplete, &utility));
	r.SetURL("http://localhost:8057/file0.html");
	r.SetIgnoreDomainRequestBudget(true);
	r.Start();

	// Wait.
	utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

	// Done - validation is done as part of the callbac.
	SEOUL_UNITTESTING_ASSERT(utility.m_bComplete);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestStress()
{
	static const UInt32 kuRequestCount = 32;
	static const Double kfServerToggleTimeSeconds = 1.0;
	static const Double kfMaxTestTimeSeconds = 30.0;

	static const Byte* kasUrls[] =
	{
		"http://localhost:8057/file0.html",
		"http://localhost:8057/file1.txt",
		"http://localhost:8057/filebig.txt",
		"http://localhost:8057/not-a-file.txt",
	};

	// Reduce resend intervals to avoid edge cases
	// where several successive failures increase resend
	// and result in running out of time for the test.
	HTTP::Manager::Get()->SetResendSettings(0.1, 1, 1.5, 0.5);

	Atomic32 tracker;
	Vector<HTTPTestUtility*, MemoryBudgets::Developer> vUtilities;
	vUtilities.Reserve(kuRequestCount);
	for (UInt32 i = 0u; i < kuRequestCount; ++i)
	{
		auto pUtility = SEOUL_NEW(MemoryBudgets::Developer) HTTPTestUtility(&tracker);
		vUtilities.PushBack(pUtility);
	}

	// Prior to starting requests, destroy the server.
	m_pServer.Reset();

	// Now setup requests.
	UInt32 uUrl = 0u;
	for (auto i = vUtilities.Begin(); vUtilities.End() != i; ++i)
	{
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetURL(kasUrls[uUrl]);
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, *i));
		r.SetResendOnFailure(true);
		if (uUrl != 3u)
		{
			(*i)->SetRequiredHttpStatus((Int)HTTP::Status::kOK);
		}
		else
		{
			(*i)->SetRequiredHttpStatus((Int)HTTP::Status::kNotFound);
		}
		r.SetLanesMask(1 << 0);
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		uUrl = (uUrl + 1) % SEOUL_ARRAY_COUNT(kasUrls);
	}

	// Loop until the last utility completes, or until times runs out.
	// Every n iterations, either destroy or create the server.
	{
		Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
		Int64 iLastTimeInTicks = iStartTimeInTicks;
		while (true)
		{
			// Check for completion.
			Bool bComplete = true;
			for (auto i = vUtilities.Begin(); vUtilities.End() != i; ++i)
			{
				if (!(*i)->m_bComplete)
				{
					bComplete = false;
					break;
				}
			}

			if (bComplete)
			{
				break;
			}

			// Simulate a 60 FPS frame so we're not starving devices with not many cores.
			auto const iBegin = SeoulTime::GetGameTimeInTicks();

			Int64 iTimeInTicks = SeoulTime::GetGameTimeInTicks();
			if (SeoulTime::ConvertTicksToSeconds(iTimeInTicks - iLastTimeInTicks) >= kfServerToggleTimeSeconds)
			{
				iLastTimeInTicks = iTimeInTicks;
				if (m_pServer.IsValid())
				{
					m_pServer.Reset();
				}
				else
				{
					CreateServer();
				}
			}

			m_pHelper->Tick();

			if (SeoulTime::ConvertTicksToSeconds(iTimeInTicks - iStartTimeInTicks) > kfMaxTestTimeSeconds)
			{
				for (UInt32 i = 0; i < vUtilities.GetSize(); ++i)
				{
					auto const& util = vUtilities[i];
					if (!util->m_bComplete)
					{
						String const sBody(util->m_vBody.Data(), util->m_vBody.GetSize());
						SEOUL_LOG("Request %d incomplete (result: %d, status: %d, ever responded: %s, body: %s)",
							i, (int)util->m_eResult, util->m_iStatus, (util->m_ResponseCallbackThreadId.IsValid()) ? "true" : "false", sBody.CStr());
					}
				}

				HTTP::Manager::Get()->LogHttpState();
				SEOUL_UNITTESTING_FAIL("Timed out after %fs waiting for stress test to complete", (float)kfMaxTestTimeSeconds);
			}

			// Simulate a 60 FPS frame so we're not starving devices with not many cores.
			auto const iEnd = SeoulTime::GetGameTimeInTicks();
			auto const uSleep = (UInt32)Floor(Clamp(SeoulTime::ConvertTicksToMilliseconds(iEnd - iBegin), 0.0, 17.0));
			Thread::Sleep(uSleep);
		}
	}

	// Verify that the order is what we expected.
	for (UInt32 u = 0u; u < vUtilities.GetSize(); ++u)
	{
		auto& utility = *vUtilities[u];
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);

		UInt32 const uURL = (u % SEOUL_ARRAY_COUNT(kasUrls));
		if (uURL != 3u)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kNotFound, utility.m_iStatus);
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(tracker, kuRequestCount);

	// Cleanup
	SafeDeleteVector(vUtilities);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestStressOffMainThread()
{
	static const UInt32 kuRequestCount = 32;
	static const Double kfServerToggleTimeSeconds = 1.0;
	static const Double kfMaxTestTimeSeconds = 30.0;

	static const Byte* kasUrls[] =
	{
		"http://localhost:8057/file0.html",
		"http://localhost:8057/file1.txt",
		"http://localhost:8057/filebig.txt",
		"http://localhost:8057/not-a-file.txt",
	};

	// Reduce resend intervals to avoid edge cases
	// where several successive failures increase resend
	// and result in running out of time for the test.
	HTTP::Manager::Get()->SetResendSettings(0.1, 1, 1.5, 0.5);

	Atomic32 tracker;
	Vector<HTTPTestUtility*, MemoryBudgets::Developer> vUtilities;
	vUtilities.Reserve(kuRequestCount);
	for (UInt32 i = 0u; i < kuRequestCount; ++i)
	{
		auto pUtility = SEOUL_NEW(MemoryBudgets::Developer) HTTPTestUtility(&tracker);
		vUtilities.PushBack(pUtility);
	}

	// Prior to starting requests, destroy the server.
	m_pServer.Reset();

	// Now setup requests.
	UInt32 uUrl = 0u;
	for (auto i = vUtilities.Begin(); vUtilities.End() != i; ++i)
	{
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetURL(kasUrls[uUrl]);
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, *i));
		r.SetDispatchCallbackOnMainThread(false);
		r.SetResendOnFailure(true);
		if (uUrl != 3u)
		{
			(*i)->SetRequiredHttpStatus((Int)HTTP::Status::kOK);
		}
		else
		{
			(*i)->SetRequiredHttpStatus((Int)HTTP::Status::kNotFound);
		}
		r.SetLanesMask(1 << 0);
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		uUrl = (uUrl + 1) % SEOUL_ARRAY_COUNT(kasUrls);
	}

	// Loop until the last utility completes, or until times runs out.
	// Every n iterations, either destroy or create the server.
	{
		Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
		Int64 iLastToggleTimeInTicks = iStartTimeInTicks;
		while (true)
		{
			// Check for completion.
			Bool bComplete = true;
			for (auto i = vUtilities.Begin(); vUtilities.End() != i; ++i)
			{
				if (!(*i)->m_bComplete)
				{
					bComplete = false;
					break;
				}
			}

			if (bComplete)
			{
				break;
			}

			Int64 iTimeInTicks = SeoulTime::GetGameTimeInTicks();
			if (SeoulTime::ConvertTicksToSeconds(iTimeInTicks - iLastToggleTimeInTicks) >= kfServerToggleTimeSeconds)
			{
				iLastToggleTimeInTicks = iTimeInTicks;
				if (m_pServer.IsValid())
				{
					m_pServer.Reset();
				}
				else
				{
					CreateServer();
				}
			}

			if (SeoulTime::ConvertTicksToSeconds(iTimeInTicks - iStartTimeInTicks) > kfMaxTestTimeSeconds)
			{
				for (UInt32 i = 0; i < vUtilities.GetSize(); ++i)
				{
					auto const& util = vUtilities[i];
					if (!util->m_bComplete)
					{
						String const sBody(util->m_vBody.Data(), util->m_vBody.GetSize());
						SEOUL_LOG("Request %d incomplete (result: %d, status: %d, ever responded: %s, body: %s)",
							i, (int)util->m_eResult, util->m_iStatus, (util->m_ResponseCallbackThreadId.IsValid()) ? "true" : "false", sBody.CStr());
					}
				}

				HTTP::Manager::Get()->LogHttpState();
				SEOUL_UNITTESTING_FAIL("Timed out after %fs waiting for stress test to complete", (float)kfMaxTestTimeSeconds);
			}
		}
	}

	// Verify that the order is what we expected.
	for (UInt32 u = 0u; u < vUtilities.GetSize(); ++u)
	{
		auto& utility = *vUtilities[u];
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);

		UInt32 const uURL = (u % SEOUL_ARRAY_COUNT(kasUrls));
		if (uURL != 3u)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kNotFound, utility.m_iStatus);
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(tracker, kuRequestCount);

	// Cleanup
	SafeDeleteVector(vUtilities);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestStressBackground()
{
	static const UInt32 kuRequestCount = 32;
	static const Double kfServerToggleTimeSeconds = 1.0;
	static const Double kfMaxTestTimeSeconds = 30.0;

	static const Byte* kasUrls[] =
	{
		"http://localhost:8057/file0.html",
		"http://localhost:8057/file1.txt",
		"http://localhost:8057/filebig.txt",
		"http://localhost:8057/not-a-file.txt",
	};

	// Reduce resend intervals to avoid edge cases
	// where several successive failures increase resend
	// and result in running out of time for the test.
	HTTP::Manager::Get()->SetResendSettings(0.1, 1, 1.5, 0.5);

	Bool bInBackground = false;

	Atomic32 tracker;
	Vector<HTTPTestUtility*, MemoryBudgets::Developer> vUtilities;
	vUtilities.Reserve(kuRequestCount);
	for (UInt32 i = 0u; i < kuRequestCount; ++i)
	{
		auto pUtility = SEOUL_NEW(MemoryBudgets::Developer) HTTPTestUtility(&tracker);
		vUtilities.PushBack(pUtility);
	}

	// Now setup requests.
	UInt32 uUrl = 0u;
	for (auto i = vUtilities.Begin(); vUtilities.End() != i; ++i)
	{
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetURL(kasUrls[uUrl]);
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, *i));
		r.SetResendOnFailure(true);
		if (uUrl != 3u)
		{
			(*i)->SetRequiredHttpStatus((Int)HTTP::Status::kOK);
		}
		else
		{
			(*i)->SetRequiredHttpStatus((Int)HTTP::Status::kNotFound);
		}
		r.SetLanesMask(1 << 0);
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		uUrl = (uUrl + 1) % SEOUL_ARRAY_COUNT(kasUrls);
	}

	// Loop until the last utility completes, or until times runs out.
	// Every n iterations, either enter or leave the background.
	{
		Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
		Int64 iLastTimeInTicks = iStartTimeInTicks;
		while (true)
		{
			// Check for completion.
			Bool bComplete = true;
			for (auto i = vUtilities.Begin(); vUtilities.End() != i; ++i)
			{
				if (!(*i)->m_bComplete)
				{
					bComplete = false;
					break;
				}
			}

			if (bComplete)
			{
				break;
			}

			// Simulate a 60 FPS frame so we're not starving devices with not many cores.
			auto const iBegin = SeoulTime::GetGameTimeInTicks();

			Int64 iTimeInTicks = SeoulTime::GetGameTimeInTicks();
			if (SeoulTime::ConvertTicksToSeconds(iTimeInTicks - iLastTimeInTicks) >= kfServerToggleTimeSeconds)
			{
				iLastTimeInTicks = iTimeInTicks;
				if (bInBackground)
				{
					HTTP::Manager::Get()->OnLeaveBackground();
					bInBackground = false;
				}
				else
				{
					HTTP::Manager::Get()->OnEnterBackground();
					bInBackground = true;
				}
			}

			m_pHelper->Tick();

			SEOUL_UNITTESTING_ASSERT(
				SeoulTime::ConvertTicksToSeconds(iTimeInTicks - iStartTimeInTicks) <= kfMaxTestTimeSeconds);

			// Simulate a 60 FPS frame so we're not starving devices with not many cores.
			auto const iEnd = SeoulTime::GetGameTimeInTicks();
			auto const uSleep = (UInt32)Floor(Clamp(SeoulTime::ConvertTicksToMilliseconds(iEnd - iBegin), 0.0, 17.0));
			Thread::Sleep(uSleep);
		}
	}

	// Verify that the order is what we expected.
	for (UInt32 u = 0u; u < vUtilities.GetSize(); ++u)
	{
		auto& utility = *vUtilities[u];
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);

		UInt32 const uURL = (u % SEOUL_ARRAY_COUNT(kasUrls));
		if (uURL != 3u)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kNotFound, utility.m_iStatus);
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(tracker, kuRequestCount);

	// Cleanup
	SafeDeleteVector(vUtilities);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestStressBackgroundOffMainThread()
{
	static const UInt32 kuRequestCount = 32;
	static const Double kfServerToggleTimeSeconds = 1.0;
	static const Double kfMaxTestTimeSeconds = 30.0;

	static const Byte* kasUrls[] =
	{
		"http://localhost:8057/file0.html",
		"http://localhost:8057/file1.txt",
		"http://localhost:8057/filebig.txt",
		"http://localhost:8057/not-a-file.txt",
	};

	// Reduce resend intervals to avoid edge cases
	// where several successive failures increase resend
	// and result in running out of time for the test.
	HTTP::Manager::Get()->SetResendSettings(0.1, 1, 1.5, 0.5);

	Bool bInBackground = false;

	Atomic32 tracker;
	Vector<HTTPTestUtility*, MemoryBudgets::Developer> vUtilities;
	vUtilities.Reserve(kuRequestCount);
	for (UInt32 i = 0u; i < kuRequestCount; ++i)
	{
		auto pUtility = SEOUL_NEW(MemoryBudgets::Developer) HTTPTestUtility(&tracker);
		vUtilities.PushBack(pUtility);
	}

	// Now setup requests.
	UInt32 uUrl = 0u;
	for (auto i = vUtilities.Begin(); vUtilities.End() != i; ++i)
	{
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetURL(kasUrls[uUrl]);
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, *i));
		r.SetDispatchCallbackOnMainThread(false);
		r.SetResendOnFailure(true);
		if (uUrl != 3u)
		{
			(*i)->SetRequiredHttpStatus((Int)HTTP::Status::kOK);
		}
		else
		{
			(*i)->SetRequiredHttpStatus((Int)HTTP::Status::kNotFound);
		}
		r.SetLanesMask(1 << 0);
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		uUrl = (uUrl + 1) % SEOUL_ARRAY_COUNT(kasUrls);
	}

	// Loop until the last utility completes, or until times runs out.
	// Every n iterations, either enter or leave the background.
	{
		Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
		Int64 iLastTimeInTicks = iStartTimeInTicks;
		while (true)
		{
			// Check for completion.
			Bool bComplete = true;
			for (auto i = vUtilities.Begin(); vUtilities.End() != i; ++i)
			{
				if (!(*i)->m_bComplete)
				{
					bComplete = false;
					break;
				}
			}

			if (bComplete)
			{
				break;
			}

			Int64 iTimeInTicks = SeoulTime::GetGameTimeInTicks();
			if (SeoulTime::ConvertTicksToSeconds(iTimeInTicks - iLastTimeInTicks) >= kfServerToggleTimeSeconds)
			{
				iLastTimeInTicks = iTimeInTicks;
				if (bInBackground)
				{
					HTTP::Manager::Get()->OnLeaveBackground();
					bInBackground = false;
				}
				else
				{
					HTTP::Manager::Get()->OnEnterBackground();
					bInBackground = true;
				}
			}

			SEOUL_UNITTESTING_ASSERT(
				SeoulTime::ConvertTicksToSeconds(iTimeInTicks - iStartTimeInTicks) <= kfMaxTestTimeSeconds);
		}
	}

	// Verify that the order is what we expected.
	for (UInt32 u = 0u; u < vUtilities.GetSize(); ++u)
	{
		auto& utility = *vUtilities[u];
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);

		UInt32 const uURL = (u % SEOUL_ARRAY_COUNT(kasUrls));
		if (uURL != 3u)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kNotFound, utility.m_iStatus);
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(tracker, kuRequestCount);

	// Cleanup
	SafeDeleteVector(vUtilities);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

struct ResendTestData
{
	Int m_iResponseCode;
	Int m_iRetriesRemainingBeforeStatusOK;
};

HTTP::CallbackResult CallbackRequireStatus200(HTTP::Result eResult, HTTP::Response* pResponse)
{
	if (eResult != HTTP::Result::kSuccess || pResponse->GetStatus() != 200) {
		return HTTP::CallbackResult::kNeedsResend;
	}

	return HTTP::CallbackResult::kSuccess;
}

Bool TestNeedsResendCallbackHandler(void* pUserData, HTTP::ServerResponseWriter& responseWriter, const HTTP::ServerRequestInfo& info)
{
	ResendTestData* pTestData = (ResendTestData*)pUserData;
	pTestData->m_iRetriesRemainingBeforeStatusOK--;
	if (pTestData->m_iRetriesRemainingBeforeStatusOK <= 0)
	{
		pTestData->m_iResponseCode = 200;
	}

	responseWriter.WriteStatusResponse(pTestData->m_iResponseCode, HTTP::HeaderTable(), "");
	return true;
}

void HTTPTest::TestNeedsResendCallback()
{
	HTTP::Manager::Get()->SetResendSettings(0.1, 1, 1.5, 0.5);

	ResendTestData testData;
	testData.m_iResponseCode = 500;
	testData.m_iRetriesRemainingBeforeStatusOK = 5;

	HTTP::ServerSettings httpSettings;
	httpSettings.m_iPort = 8056;
	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestNeedsResendCallbackHandler, (void*)&testData);
	httpSettings.m_iThreadCount = 1;
	HTTP::Server eventServer(httpSettings);

	HTTPTestUtility utility;
	auto& r = HTTP::Manager::Get()->CreateRequest();
	r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
	r.SetResendOnFailure(true);
	utility.SetRequiredHttpStatus((Int)HTTP::Status::kOK);
	r.SetURL("http://localhost:8056/not-a-file.txt");
	r.SetIgnoreDomainRequestBudget(true);
	r.Start();

	// Wait.
	utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

	// Verify.
	SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, testData.m_iRetriesRemainingBeforeStatusOK);
	SEOUL_UNITTESTING_ASSERT_EQUAL(200, utility.m_iStatus);

	SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestNeedsResendCallbackOffMainThread()
{
	HTTP::Manager::Get()->SetResendSettings(0.1, 1, 1.5, 0.5);

	ResendTestData testData = {};
	testData.m_iResponseCode = 500;
	testData.m_iRetriesRemainingBeforeStatusOK = 5;

	HTTP::ServerSettings httpSettings;
	httpSettings.m_iPort = 8056;
	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestNeedsResendCallbackHandler, (void*)&testData);
	httpSettings.m_iThreadCount = 1;
	HTTP::Server eventServer(httpSettings);

	HTTPTestUtility utility;
	auto& r = HTTP::Manager::Get()->CreateRequest();
	r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
	r.SetResendOnFailure(true);
	utility.SetRequiredHttpStatus((Int)HTTP::Status::kOK);
	r.SetURL("http://localhost:8056/file1.txt");
	r.SetDispatchCallbackOnMainThread(false);
	r.SetIgnoreDomainRequestBudget(true);
	r.Start();

	// Wait.
	utility.WaitForCompletion(10.0f);

	// Verify.
	SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, testData.m_iRetriesRemainingBeforeStatusOK);
	SEOUL_UNITTESTING_ASSERT_EQUAL(200, utility.m_iStatus);
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

struct ExponentialBackoffTestData
{
	Float* m_ExpectedDelays{};
	Int m_iExpectedDelay{};
	Int m_iRetryCount{};
	Int64 m_iLastAttemptTickTime{};
};

Bool TestExponentialBackoffCallbackHandler(void* pUserData, HTTP::ServerResponseWriter& responseWriter, const HTTP::ServerRequestInfo& info)
{
	ExponentialBackoffTestData* pTestData = (ExponentialBackoffTestData*)pUserData;

	auto iTickTime = SeoulTime::GetGameTimeInTicks();
	pTestData->m_iRetryCount++;

	Double fExpected = pTestData->m_ExpectedDelays[pTestData->m_iExpectedDelay];

	// Give a bit more wiggle to device runs
#if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS
	Double fEpsilon = 0.4 + Max(0.0, fExpected * 0.5);
#else
	Double fEpsilon = 0.2 + Max(0.0, fExpected * 0.5);
#endif

	if (fExpected >= 0)
	{
		Double fTimeSinceLastTry = SeoulTime::ConvertTicksToSeconds(iTickTime - pTestData->m_iLastAttemptTickTime);
		SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(fExpected, fTimeSinceLastTry, fEpsilon);

		pTestData->m_iExpectedDelay++;
	}

	pTestData->m_iLastAttemptTickTime = iTickTime;

	responseWriter.WriteStatusResponse(500, HTTP::HeaderTable(), "");
	return true;
}

void HTTPTest::TestExponentialBackoff()
{
	HTTP::Manager::Get()->SetResendSettings(0.1, 1, 1.5, 0);
	auto& r = HTTP::Manager::Get()->CreateRequest();

	SEOUL_UNITTESTING_ASSERT(0 == r.UnitTestOnly_GetEarliestSendTimeInTicks());

	const Int64 kAllowedTickDelta = SeoulTime::ConvertSecondsToTicks(0.01f);
	const Int64 iNowTicks = SeoulTime::GetGameTimeInTicks();

	// Non-success status doesn't increase the delay
	for (int i = 0; i < 10; ++i)
	{
		auto iExpectedTicks = iNowTicks + SeoulTime::ConvertSecondsToTicks(0.1f);

		r.UnitTestOnly_InitializeResendRequest(HTTP::Result::kCanceled, iNowTicks);
		auto iActualTicks = r.UnitTestOnly_GetEarliestSendTimeInTicks();
		auto iDelta = Abs(iActualTicks - iExpectedTicks);
		SEOUL_UNITTESTING_ASSERT_MESSAGE(
			iDelta < kAllowedTickDelta,
			"Expected: %f ms. Actual: %f ms: not within %f ms (i=%d)",
			SeoulTime::ConvertTicksToMilliseconds(iExpectedTicks),
			SeoulTime::ConvertTicksToMilliseconds(iActualTicks),
			SeoulTime::ConvertTicksToMilliseconds(kAllowedTickDelta), i);
	}

	Float expectedDelays[] = { 0.15f, 0.225f, 0.3375f };
	for (auto fExpectedDelaySec : expectedDelays)
	{
		auto iExpectedTicks = iNowTicks + SeoulTime::ConvertSecondsToTicks(fExpectedDelaySec);
		r.UnitTestOnly_InitializeResendRequest(HTTP::Result::kSuccess, iNowTicks);
		auto iActualTicks = r.UnitTestOnly_GetEarliestSendTimeInTicks();
		auto iDelta = Abs(iActualTicks - iExpectedTicks);
		SEOUL_UNITTESTING_ASSERT_MESSAGE(
			iDelta < kAllowedTickDelta,
			"Expected: %f ms. Actual: %f ms: not within %f ms",
			SeoulTime::ConvertTicksToMilliseconds(iExpectedTicks),
			SeoulTime::ConvertTicksToMilliseconds(iActualTicks),
			SeoulTime::ConvertTicksToMilliseconds(kAllowedTickDelta));
	}

	auto rp = &r;
	HTTP::Manager::Get()->DestroyUnusedRequest(rp);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestEnforceEarliestSendTime()
{
	const Int64 iEarliestSend = SeoulTime::GetGameTimeInTicks() + SeoulTime::ConvertSecondsToTicks(1.0f);

	HTTPTestUtility utility;
	auto& r = HTTP::Manager::Get()->CreateRequest();
	r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
	r.SetURL("http://localhost:8057/file0.html");
	r.SetIgnoreDomainRequestBudget(true);
	r.UnitTestOnly_SetEarliestSendTimeInTicks(iEarliestSend);
	r.Start();

	// Wait.
	utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

	// Verify.
	SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
	SEOUL_UNITTESTING_ASSERT(utility.m_LastRequestTickTime > iEarliestSend);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestParseURLDomain()
{
	auto tests = {
		"http://api.some-domain.com/v1", "api.some-domain.com",
		"https://api.some-domain.com:8081/v1", "api.some-domain.com",
		"http://127.0.0.1:8081/v1", "127.0.0.1",
		"ftp://127.0.0.1:8081/v1", "127.0.0.1",
		"api-qa.some-domain.com/v1", "api-qa.some-domain.com",
		"api-qa.some-domain.com", "api-qa.some-domain.com",
	};

	auto p = tests.begin();
	for (int i = 0; (i + 1) < tests.size(); i += 2)
	{
		auto input = p[i];
		auto expected = p[i + 1];
		auto sActual = HTTP::Manager::ParseURLDomain(input);

		//SEOUL_LOG("Input: '%s' -- Expected: '%s' -- Actual: '%s'", input, expected, sActual.CStr());
		SEOUL_UNITTESTING_ASSERT_EQUAL(expected, sActual);
	}
}

void HTTPTest::TestRequestRateLimiting()
{
	Int iInitialBudget = 5;
	Int iIncreaseDelaySeconds = 5;
	HTTP::Manager::Get()->SetDomainRequestBudgetSettings(iInitialBudget, iIncreaseDelaySeconds);

	auto& domain1Request1 = HTTP::Manager::Get()->CreateRequest();
	HTTPTestUtility utilD1R1;
	domain1Request1.SetURL("http://localhost:8057/domain1-request1.txt");
	domain1Request1.SetResendOnFailure(false);
	domain1Request1.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utilD1R1));

	auto& domain1Request2 = HTTP::Manager::Get()->CreateRequest();
	HTTPTestUtility utilD1R2;
	domain1Request2.SetURL("http://localhost:8057/domain1-request2.txt");
	domain1Request2.SetResendOnFailure(false);
	domain1Request2.SetIgnoreDomainRequestBudget(true);
	domain1Request2.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utilD1R2));

	auto& domain1Request3 = HTTP::Manager::Get()->CreateRequest();
	HTTPTestUtility utilD1R3;
	domain1Request3.SetURL("http://localhost:8057/domain1-request3.txt");
	domain1Request3.SetResendOnFailure(false);
	domain1Request3.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utilD1R3));

	auto& domain2Request1 = HTTP::Manager::Get()->CreateRequest();
	HTTPTestUtility utilD2R1;
	domain2Request1.SetURL("http://127.0.0.1:8057/domain2-request1.txt");
	domain2Request1.SetResendOnFailure(false);
	domain2Request1.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utilD2R1));

	// Spend all but one of our budget
	auto const iBudgetStartTicks = SeoulTime::GetGameTimeInTicks();
	for (int i = 0; i < iInitialBudget - 1; ++i)
	{
		HTTP::Manager::Get()->DecrementDomainRequestBudget(domain1Request1);
	}

	domain1Request1.Start();
	domain1Request2.Start();
	domain1Request3.Start();
	domain2Request1.Start();

	Double fTimeoutInSeconds = 5.0f;
	auto iStartTicks = SeoulTime::GetGameTimeInTicks();
	while (
		!domain1Request1.HasStarted() ||
		!domain1Request2.HasStarted() ||
		!domain2Request1.HasStarted())
	{
		// Requests 1:1, 1:2 and 2:1 should start before 1:3
		SEOUL_UNITTESTING_ASSERT(!domain1Request3.HasStarted());
		SEOUL_UNITTESTING_ASSERT(
			SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTicks) < fTimeoutInSeconds);
	};

	// Until the budget increases, 1:3 should still not start
	while (SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iBudgetStartTicks) < (Double(iIncreaseDelaySeconds) - 0.1f))
	{
		SEOUL_UNITTESTING_ASSERT(!domain1Request3.HasStarted());
	}

	// Request 1-3 should start soon after the budget increase
	iStartTicks = SeoulTime::GetGameTimeInTicks();
	while (!domain1Request3.HasStarted())
	{
		SEOUL_UNITTESTING_ASSERT(
			SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTicks) < fTimeoutInSeconds);
	}

	// If D1R3 doesn't complete before the test exits, it'll try to call the completion delegate on the now-gone utility
	// from this test's stack (that's bad)
	utilD1R3.WaitForMainThreadCompletion(*m_pHelper, 5.0f);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

/**
 * This is a regression for a memory scribble. Fundamentally, an HTTP::Request
 * pointer *must* not be stored - it can be destroyed at any time (e.g. on
 * a resend). This was forgotten and encouraged due to (e.g.) the HTTP::Request::Cancel()
 * API.
 */
void HTTPTest::TestCancelRegression()
{
	// Issue a request that will never complete to
	// an endpoint we don't expect to exist - give it enough
	// time to retry, then cancel it - this will crash
	// as pRequest is invalid.

	// Kill the server.
	m_pServer.Reset();

	// Update resend for testing.
	HTTP::Manager::Get()->SetResendSettings(0.1, 0.1, 1, 0.0);

	// Utility for tracking.
	HTTPTestUtility utility;

	// Create the request object.
	SharedPtr<HTTP::RequestCancellationToken> pToken;
	{
		auto& r = HTTP::Manager::Get()->CreateRequest();
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
		r.SetResendOnFailure(true);
		r.SetURL("http://localhost:8057/file0.html"); // We expect this to fail.
		pToken = r.Start();
	}

	// Wait a few seconds for a retry to trigger.
	auto const iStartTicks = SeoulTime::GetGameTimeInTicks();
	while (SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTicks) < 2.0)
	{
		// Main thread so we need to tick the helper while waiting.

		// Simulate a 60 FPS frame so we're not starving devices with not many cores.
		auto const iBegin = SeoulTime::GetGameTimeInTicks();
		m_pHelper->Tick();
		auto const iEnd = SeoulTime::GetGameTimeInTicks();
		auto const uSleep = (UInt32)Floor(Clamp(SeoulTime::ConvertTicksToMilliseconds(iEnd - iBegin), 0.0, 17.0));
		Thread::Sleep(uSleep);
	}

	// Now cancel the request - in old versions of the API,
	// this would scribble and crash.
	pToken->Cancel();

	// Wait for completion.
	utility.WaitForMainThreadCompletion(*m_pHelper, 2.0f);

	// Expected code.
	SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kCanceled, utility.m_eResult);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::CreateServer(
	Int iThreadCount /*= 1*/,
	const HandlerDelegate& responseHandler /*= HandlerDelegate()*/)
{
	HTTP::ServerSettings settings;
	settings.m_Handler = responseHandler;
	settings.m_sRootDirectory = Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/HTTP");
	settings.m_iPort = 8057;
	settings.m_iThreadCount = iThreadCount;
	m_pServer.Reset(SEOUL_NEW(MemoryBudgets::Developer) HTTP::Server(settings));
}

void HTTPTest::TestBlockingCancelAll()
{
	static const UInt32 kuRequestCount = 32;

	static const Byte* kasUrls[] =
	{
		"http://localhost:8057/file0.html",
		"http://localhost:8057/file1.txt",
		"http://localhost:8057/filebig.txt",
		"http://localhost:8057/not-a-file.txt",
	};

	// Reduce resend intervals to avoid edge cases
	// where several successive failures increase resend
	// and result in running out of time for the test.
	HTTP::Manager::Get()->SetResendSettings(0.1, 1, 1.5, 0.5);

	Atomic32 tracker;
	Vector<HTTPTestUtility*, MemoryBudgets::Developer> vUtilities;
	vUtilities.Reserve(kuRequestCount);
	for (UInt32 i = 0u; i < kuRequestCount; ++i)
	{
		auto pUtility = SEOUL_NEW(MemoryBudgets::Developer) HTTPTestUtility(&tracker);
		vUtilities.PushBack(pUtility);
	}

	// Now setup requests.
	HTTP::RequestList list;
	UInt32 uUrl = 0u;
	for (auto i = vUtilities.Begin(); vUtilities.End() != i; ++i)
	{
		auto& r = HTTP::Manager::Get()->CreateRequest(&list);
		r.SetURL(kasUrls[uUrl]);
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, *i));
		r.SetResendOnFailure(true);
		(*i)->SetRequiredHttpStatus(0);
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		uUrl = (uUrl + 1) % SEOUL_ARRAY_COUNT(kasUrls);
	}

	// Now cancel all requests.
	list.BlockingCancelAll();

	// Verify results.
	for (auto const& e : vUtilities)
	{
		SEOUL_UNITTESTING_ASSERT(e->m_bComplete);
		SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kCanceled, e->m_eResult);
	}

	// Cleanup
	SafeDeleteVector(vUtilities);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestShutdownInBackground()
{
	static const UInt32 kuRequestCount = 32;

	static const Byte* kasUrls[] =
	{
		"http://localhost:8057/file0.html",
		"http://localhost:8057/file1.txt",
		"http://localhost:8057/filebig.txt",
		"http://localhost:8057/not-a-file.txt",
	};

	// Reduce resend intervals to avoid edge cases
	// where several successive failures increase resend
	// and result in running out of time for the test.
	HTTP::Manager::Get()->SetResendSettings(0.1, 1, 1.5, 0.5);

	Atomic32 tracker;
	Vector<HTTPTestUtility*, MemoryBudgets::Developer> vUtilities;
	vUtilities.Reserve(kuRequestCount);
	for (UInt32 i = 0u; i < kuRequestCount; ++i)
	{
		auto pUtility = SEOUL_NEW(MemoryBudgets::Developer) HTTPTestUtility(&tracker);
		vUtilities.PushBack(pUtility);
	}

	// Enter the background.
	HTTP::Manager::Get()->OnEnterBackground();

	// Now setup requests.
	HTTP::RequestList list;
	UInt32 uUrl = 0u;
	for (auto i = vUtilities.Begin(); vUtilities.End() != i; ++i)
	{
		auto& r = HTTP::Manager::Get()->CreateRequest(&list);
		r.SetURL(kasUrls[uUrl]);
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, *i));
		r.SetResendOnFailure(true);
		(*i)->SetRequiredHttpStatus(0);
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		uUrl = (uUrl + 1) % SEOUL_ARRAY_COUNT(kasUrls);
	}

	// Make sure none of the requests have completed.
	for (auto const& e : vUtilities)
	{
		SEOUL_UNITTESTING_ASSERT(!e->m_bComplete);
	}

	// We're going to terminate HTTP::Manager with request still
	// active and in the background.
	m_pServer.Reset();
	m_pHelper.Reset();

	// Cleanup
	SafeDeleteVector(vUtilities);
}

void HTTPTest::TestTickInBackground()
{
	static const UInt32 kuRequestCount = 32;

	static const Byte* kasUrls[] =
	{
		"https://localhost:8057/file0.html",
		"https://localhost:8057/file1.txt",
		"https://localhost:8057/filebig.txt",
		"https://localhost:8057/not-a-file.txt",
	};

	// Reduce resend intervals to avoid edge cases
	// where several successive failures increase resend
	// and result in running out of time for the test.
	HTTP::Manager::Get()->SetResendSettings(0.1, 1, 1.5, 0.5);

	Atomic32 tracker;
	Vector<HTTPTestUtility*, MemoryBudgets::Developer> vUtilities;
	vUtilities.Reserve(kuRequestCount);
	for (UInt32 i = 0u; i < kuRequestCount; ++i)
	{
		auto pUtility = SEOUL_NEW(MemoryBudgets::Developer) HTTPTestUtility(&tracker);
		vUtilities.PushBack(pUtility);
	}

	// Enter the background.
	HTTP::Manager::Get()->OnEnterBackground();

	// Now setup requests.
	HTTP::RequestList list;
	UInt32 uUrl = 0u;
	for (auto i = vUtilities.Begin(); vUtilities.End() != i; ++i)
	{
		auto& r = HTTP::Manager::Get()->CreateRequest(&list);
		r.SetURL(kasUrls[uUrl]);
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, *i));
		r.SetResendOnFailure(true);
		(*i)->SetRequiredHttpStatus(0);
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		uUrl = (uUrl + 1) % SEOUL_ARRAY_COUNT(kasUrls);
	}

	// Yield for a bit.
	{
		auto const iStart = SeoulTime::GetGameTimeInTicks();
		while (SeoulTime::ConvertTicksToMilliseconds(SeoulTime::GetGameTimeInTicks() - iStart) < 500.0)
		{
			// Simulate a 60 FPS frame so we're not starving devices with not many cores.
			auto const iBegin = SeoulTime::GetGameTimeInTicks();
			m_pHelper->Tick();
			auto const iEnd = SeoulTime::GetGameTimeInTicks();
			auto const uSleep = (UInt32)Floor(Clamp(SeoulTime::ConvertTicksToMilliseconds(iEnd - iBegin), 0.0, 17.0));
			Thread::Sleep(uSleep);
		}
	}

	// We're going to terminate HTTP::Manager with request still
	// active.
	m_pServer.Reset();
	m_pHelper.Reset();

	// Make sure all of the requests have completed.
	// Some will have finished, others will have cancelled,
	// so we don't check status or result.
	for (auto const& e : vUtilities)
	{
		SEOUL_UNITTESTING_ASSERT(e->m_bComplete);
	}

	// Cleanup
	SafeDeleteVector(vUtilities);
}

void HTTPTest::TestHTTPSAttempt()
{
	static const UInt32 kuRequestCount = 32;

	static const Byte* kasUrls[] =
	{
		"https://localhost:8057/file0.html",
		"https://localhost:8057/file1.txt",
		"https://localhost:8057/filebig.txt",
		"https://localhost:8057/not-a-file.txt",
	};

	// Reduce resend intervals to avoid edge cases
	// where several successive failures increase resend
	// and result in running out of time for the test.
	HTTP::Manager::Get()->SetResendSettings(0.1, 1, 1.5, 0.5);

	Atomic32 tracker;
	Vector<HTTPTestUtility*, MemoryBudgets::Developer> vUtilities;
	vUtilities.Reserve(kuRequestCount);
	for (UInt32 i = 0u; i < kuRequestCount; ++i)
	{
		auto pUtility = SEOUL_NEW(MemoryBudgets::Developer) HTTPTestUtility(&tracker);
		vUtilities.PushBack(pUtility);
	}

	// Now setup requests.
	HTTP::RequestList list;
	UInt32 uUrl = 0u;
	for (auto i = vUtilities.Begin(); vUtilities.End() != i; ++i)
	{
		auto& r = HTTP::Manager::Get()->CreateRequest(&list);
		r.SetURL(kasUrls[uUrl]);
		r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, *i));
		r.SetResendOnFailure(true);
		(*i)->SetRequiredHttpStatus(0);
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		uUrl = (uUrl + 1) % SEOUL_ARRAY_COUNT(kasUrls);
	}

	// We're going to terminate HTTP::Manager with request still
	// active.
	m_pServer.Reset();
	m_pHelper.Reset();

	// Make sure all of the requests have completed.
	for (auto const& e : vUtilities)
	{
		SEOUL_UNITTESTING_ASSERT(e->m_bComplete);
	}

	// Cleanup
	SafeDeleteVector(vUtilities);
}

void HTTPTest::TestNoCallback()
{
	static const UInt32 kuRequestCount = 32;

	static const Byte* kasUrls[] =
	{
		"https://localhost:8057/file0.html",
		"https://localhost:8057/file1.txt",
		"https://localhost:8057/filebig.txt",
		"https://localhost:8057/not-a-file.txt",
	};

	// Reduce resend intervals to avoid edge cases
	// where several successive failures increase resend
	// and result in running out of time for the test.
	HTTP::Manager::Get()->SetResendSettings(0.1, 1, 1.5, 0.5);

	// Now setup requests.
	HTTP::RequestList list;
	UInt32 uUrl = 0u;
	for (auto i = 0u; i < kuRequestCount; ++i)
	{
		auto& r = HTTP::Manager::Get()->CreateRequest(&list);
		r.SetURL(kasUrls[uUrl]);
		r.SetResendOnFailure(true);
		r.SetIgnoreDomainRequestBudget(true);
		r.Start();

		uUrl = (uUrl + 1) % SEOUL_ARRAY_COUNT(kasUrls);
	}

	// We're going to terminate HTTP::Manager with request still
	// active.
	m_pServer.Reset();
	m_pHelper.Reset();
}

void HTTPTest::TestProgressCallback()
{
	String const sOutputFile(Path::GetTempFileAbsoluteFilename());

	HTTPTestUtility utility;
	auto& r = HTTP::Manager::Get()->CreateRequest();
	r.SetURL("http://localhost:8057/filebig.txt");
	r.SetBodyDataOutputFile(sOutputFile, false);
	r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
	r.SetProgressCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnProgress, &utility));
	r.SetIgnoreDomainRequestBudget(true);
	r.Start();

	utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

	// Get expected file size - can change based on line ending changes via source control.
	auto const uExpectedSize = (UInt32)FileManager::Get()->GetFileSize(
		Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests", "HTTP", "filebig.txt"));

	SEOUL_UNITTESTING_ASSERT(utility.m_bComplete);
	SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
	SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(
		Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests", "HTTP", "filebig.txt"),
		sOutputFile));
	SEOUL_UNITTESTING_ASSERT_LESS_THAN(0, utility.m_ProgressCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedSize, utility.m_ProgressDownloadSizeInBytes);
	SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedSize, utility.m_ProgressDownloadSoFarInBytes);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestMiscApi()
{
	String const sOutputFile(Path::GetTempFileAbsoluteFilename());
	auto timer = HTTP::Manager::Get()->CopyHttpResendTimer();
	(void)timer;

	HTTPTestUtility utility;
	auto& r = HTTP::Manager::Get()->CreateRequest();
	SEOUL_UNITTESTING_ASSERT(r.GetVerifyPeer());
	r.SetVerifyPeer(false);
	SEOUL_UNITTESTING_ASSERT(!r.GetVerifyPeer());
	SEOUL_UNITTESTING_ASSERT(!r.IsBodyDataOutputFileOk());

	r.SetURL("http://localhost:8057/filebig.txt");
	r.SetBodyDataOutputFile(sOutputFile, false);
	r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
	r.SetProgressCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnProgress, &utility));
	r.SetIgnoreDomainRequestBudget(true);
	r.Start();

	utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

	// Get expected file size - can change based on line ending changes via source control.
	auto const uExpectedSize = (UInt32)FileManager::Get()->GetFileSize(
		Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests", "HTTP", "filebig.txt"));

	SEOUL_UNITTESTING_ASSERT(utility.m_bComplete);
	SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
	SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(
		Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests", "HTTP", "filebig.txt"),
		sOutputFile));
	SEOUL_UNITTESTING_ASSERT_LESS_THAN(0, utility.m_ProgressCalls);
	SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedSize, utility.m_ProgressDownloadSizeInBytes);
	SEOUL_UNITTESTING_ASSERT_EQUAL(uExpectedSize, utility.m_ProgressDownloadSoFarInBytes);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestRangeReset()
{
	String const sOutputFile(Path::GetTempFileAbsoluteFilename());

	HTTPTestUtility utility;
	auto& r = HTTP::Manager::Get()->CreateRequest();
	r.SetURL("http://localhost:8057/filebig.txt");
	r.SetBodyDataOutputFile(sOutputFile, false);
	r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
	r.SetIgnoreDomainRequestBudget(true);

	// Configure a range.
	r.AddRangeHeader(100u, 101u);

	// Now remove it.
	r.DeleteRangeHeader();

	r.Start();

	utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

	SEOUL_UNITTESTING_ASSERT(utility.m_bComplete);
	SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
	SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);
	SEOUL_UNITTESTING_ASSERT(FilesAreEqual(
		Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests", "HTTP", "filebig.txt"),
		sOutputFile));

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

void HTTPTest::TestURLEncode()
{
	SEOUL_UNITTESTING_ASSERT_EQUAL("a.b.com%2F%0D%0Aasdf%20-%20%7Ebb%2F23%2Fv8%2F082", HTTP::Manager::URLEncode("a.b.com/\nasdf - ~bb/23/v8/082"));
}

namespace
{
	Bool RedirectHandler(HTTP::ServerResponseWriter& responseWriter, const HTTP::ServerRequestInfo& info)
	{
		if (info.m_sUri == "/a")
		{
			HTTP::HeaderTable t;
			String const sKey("Location");
			String const sValue("http://localhost:8057/file0.html");
			t.AddKeyValue(sKey.CStr(), sKey.GetSize(), sValue.CStr(), sValue.GetSize());
			responseWriter.WriteStatusResponse(303, t, String());
			return true;
		}
		else
		{
			return false;
		}
	}
}

void HTTPTest::TestRedirect()
{
	// Recreate the server with a redirect handler.
	m_pServer.Reset();
	CreateServer(1, SEOUL_BIND_DELEGATE(RedirectHandler));

	HTTPTestUtility utility;
	auto& r = HTTP::Manager::Get()->CreateRequest();
	r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
	r.SetURL("http://localhost:8057/a");
	r.SetIgnoreDomainRequestBudget(true);
	r.Start();

	// Wait.
	utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

	// Verify.
	SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kOK, utility.m_iStatus);
	SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);

	String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL("<html><body></body></html>", sActual);
}

void HTTPTest::TestBadMethod()
{
	HTTPTestUtility utility;
	auto& r = HTTP::Manager::Get()->CreateRequest();
	r.SetCallback(SEOUL_BIND_DELEGATE(&HTTPTestUtility::OnComplete, &utility));
	r.SetURL("http://localhost:8057/this-file-does-not-exist.txt");
	r.SetIgnoreDomainRequestBudget(true);
	r.Start();

	// Wait.
	utility.WaitForMainThreadCompletion(*m_pHelper, 10.0f);

	// Verify.
	SEOUL_UNITTESTING_ASSERT_EQUAL(HTTP::Result::kSuccess, utility.m_eResult);
	SEOUL_UNITTESTING_ASSERT_EQUAL((Int)HTTP::Status::kNotFound, utility.m_iStatus);
	SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), utility.m_ResponseCallbackThreadId);

	String const sActual(utility.m_vBody.Data(), utility.m_vBody.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL("Error 404: Not Found\nFile not found", sActual);

	SEOUL_UNITTESTING_ASSERT(!HTTP::Manager::Get()->HasRequests());
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
