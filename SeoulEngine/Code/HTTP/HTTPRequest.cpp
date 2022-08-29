/**
 * \file HTTPRequest.cpp
 * \brief Wraps a single HTTP client request managed via HTTPManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CoreVirtuals.h"
#include "FileManager.h"
#include "HTTPExternalDefine.h"
#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "Logger.h"
#include "Thread.h"

namespace Seoul::HTTP
{

Bool IsHttpApiThread();

#if SEOUL_WITH_CURL

/**
 * Curl SSL context function to provide custom SSL behavior
 */
static CURLcode CurlSSLContextFunction(CURL* pCurl, void* pContext, void* pParam)
{
	return (CURLcode)((Request*)pParam)->CurlSSLContextFunction(pContext);
}

#if SEOUL_LOGGING_ENABLED
static Byte const* CurlInfoTypeToString(curl_infotype eType)
{
	switch (eType)
	{
	case CURLINFO_TEXT: return "Text";
	case CURLINFO_HEADER_IN: return "HeaderIn";
	case CURLINFO_HEADER_OUT: return "HeaderOut";
	case CURLINFO_DATA_IN: return "DataIn";
	case CURLINFO_DATA_OUT: return "DataOut";
	case CURLINFO_SSL_DATA_IN: return "SslDataIn";
	case CURLINFO_SSL_DATA_OUT: return "SslDataOut";
	default: return "Unknown";
	};
};

static Int CurlVerboseLogFunction(
	CURL* pHandle,
	curl_infotype eType,
	char* pData,
	size_t zSize,
	void*)
{
	switch (eType)
	{
	case CURLINFO_HEADER_IN: // fall-through
	case CURLINFO_HEADER_OUT: // fall-through
	case CURLINFO_TEXT:
		SEOUL_LOG_HTTP("CURLV(%s): %s", CurlInfoTypeToString(eType), String((Byte const*)pData, (UInt32)zSize).CStr());
		break;

	default:
		SEOUL_LOG_HTTP("CURLV(%s): %u bytes", CurlInfoTypeToString(eType), (UInt32)zSize);
		break;
	};

	return 0;
}
#endif // /#if SEOUL_LOGGING_ENABLED

#endif // /#if SEOUL_WITH_CURL

/**
 * Default timeout, in seconds, for the request to connect. Applies
 * only on curl based platforms (Android, PC).
 */
static const Int kiDefaultConnectionTimeoutInSeconds = 15;

/**
 * Default timeout, in seconds, for the transfer portion of all HTTP requests.
 * Note that depending on the platform, this may be either the total timeout
 * for the entire HTTP request (iOS), or it may be the separate timeout value
 * for just the transfer portion of the request but not the connection (Android, PC).
 */
static const Int kiDefaultTransferTimeoutInSeconds = 15;

Request::Request(const SharedPtr<RequestCancellationToken>& pToken, const ResendTimer& resendTimer)
	// Start of the members that need to be cloned.
	: m_sURL()
	, m_bResendOnFailure(true)
	, m_bVerifyPeer(true)
	, m_bDispatchCallbackOnMainThread(true)
	, m_bIgnoreDomainRequestBudget(false)
	, m_ResendTimer(resendTimer)
	, m_fResendDelaySeconds(0)
	, m_sBodyDataOutputFilename()
	, m_bBodyDataOutputFileOpenForResume(false)
	, m_OpenFileValidateCallback()
	, m_PrepForResendCallback()
	, m_sMethod(Method::ksGet)
	, m_iConnectionTimeoutInSeconds(kiDefaultConnectionTimeoutInSeconds)
	, m_iTransferTimeoutInSeconds(kiDefaultTransferTimeoutInSeconds)
	, m_LanesMask(0)
	// Disable 'use of this' warning in constructor
#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable:4355)
#endif
	, m_Node(this)
	// Re-enable 'use of this' warning in constructor
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif
	, m_Callback()
	, m_ProgressCallback()
	, m_tPostData()
	, m_PostBody()
	, m_tHeaderData()
	, m_pCancellationToken(pToken)
	, m_iRequestConstructTimeInTicks(SeoulTime::GetGameTimeInTicks())
	, m_iRequestStartTimeInTicks(0)
	, m_Stats()
	// End of the members that need to be cloned.

	, m_iEarliestSendTimeInTicks(0)
	, m_pBodyDataOutputFile()
	, m_bBodyDataOutputFileOk(false)
	, m_bBodyDataOpenFileValidateCallbackPassed(false)
	, m_bBodyDataOutputFileOpened(false)
	, m_iStartTimeInTicks(0)
	, m_TotalDownloadSizeInBytes(0)
	, m_DownloadSizeSoFarInBytes(0)
	, m_LastReportedDownloadSizeSoFarInBytes(0)
	, m_bCompleted(false)
	, m_bIsResendRequest(false)
	, m_bIsNetworkFailureResendRequest(false)
	, m_eResult(Result::kFailure)
	, m_Response()
	, m_bApiCancelRequested(false)
	, m_bApiCancelRequestCompleted(false)
#if SEOUL_WITH_CURL
	, m_pCurl(nullptr)
	, m_pHeaderList(nullptr)
#elif SEOUL_WITH_URLSESSION
	, m_pSession(nullptr)
	, m_pTask(nullptr)
	, m_pDelegate(nullptr)
#endif
{
}

Request::~Request()
{
	// Must be ensured by code paths
	// that destroy HTTP::Request to avoid
	// problems with (e.g.) HTTP::Request
	// being partially destructed yet
	// still being accessible via a request list.
	SEOUL_ASSERT(nullptr == m_Node.GetNext());
	SEOUL_ASSERT(nullptr == m_Node.GetOwner());
	SEOUL_ASSERT(nullptr == m_Node.GetPrev());

	// Untrack.
	if (m_bIsNetworkFailureResendRequest)
	{
		--Manager::Get()->m_NetworkFailureActiveResendRequests;
	}

#if SEOUL_WITH_CURL
	SEOUL_ASSERT(m_pCurl == nullptr);
	SEOUL_ASSERT(m_pHeaderList == nullptr);
#elif SEOUL_WITH_URLSESSION
	SEOUL_ASSERT(m_pSession == nullptr);
	SEOUL_ASSERT(m_pTask == nullptr);
	SEOUL_ASSERT(m_pDelegate == nullptr);
#endif
}

/**
 * Override the buffer used when accepting the request. By default,
 * a buffer will be dynamically managed to store the request. If a buffer
 * is set via this callback, it will not be resized - it must be large
 * enough to store the maximum size of the request, or the request will
 * be truncated to zBufferSizeInBytes.
 *
 * \pre pBuffer must remain allocated until this request completes.
 */
void Request::SetBodyOutputBuffer(void* pBuffer, UInt32 zBufferSizeInBytes)
{
	// Pass through.
	m_Response.SetBodyOutputBuffer(pBuffer, zBufferSizeInBytes);
}

void Request::SetBodyDataOutputFile(const String& sAbsoluteFilename, Bool bResume /*= false*/)
{
	m_sBodyDataOutputFilename = sAbsoluteFilename;
	m_bBodyDataOutputFileOpenForResume = bResume;
}

/**
 * Sets the HTTP method to be used
 */
void Request::SetMethod(HString sMethod)
{
	m_sMethod = sMethod;
}

/**
 * Sets the URL to request, which must be properly URL-encoded.
 */
void Request::SetURL(const String& sURL)
{
	m_sURL = sURL;
}


/**
 * Sets the callback for when the request completes
 */
void Request::SetCallback(const ResponseDelegate& callback)
{
	m_Callback = callback;
}

/**
 * Sets the callback that will be invoked to report the progress of an HTTP
 * response.
 */
void Request::SetProgressCallback(const ResponseProgressDelegate& callback)
{
	m_ProgressCallback = callback;
}

/**
 * Sets the callback which will be invoked to validate the data initially received
 * from the server when writing to a body file. This allows the client to verify
 * the data before the file is opened or written to.
 */
void Request::SetOpenFileValidateCallback(const OpenFileValidateDelegate& callback)
{
	m_OpenFileValidateCallback = callback;
}

/**
 * Sets the callback that will be invoked right before a resend is started.
 * The HTTP::Request argument to this callback will be the resend request.
 */
void Request::SetPrepForResendCallback(const PrepForResendCallback& callback)
{
	m_PrepForResendCallback = callback;
}

/**
 * Sets the timeout for the connection stage of the HTTP request, in seconds
 */
void Request::SetConnectionTimeout(Int iTimeoutInSeconds)
{
	m_iConnectionTimeoutInSeconds = iTimeoutInSeconds;
}

/**
* Sets the timeout for the transfer stage of the HTTP request, in seconds
*/
void Request::SetTransferTimeout(Int iTimeoutInSeconds)
{
	m_iTransferTimeoutInSeconds = iTimeoutInSeconds;
}

/**
 * Adds a string Key=Value pair of POST data.  The value will be URL-encoded by
 * this method.
 */
void Request::AddPostData(const String& sKey, const String& sValue)
{
	(void)m_tPostData.Overwrite(sKey, Manager::URLEncode(sValue));
	m_PostBody.Clear();
}

/**
 * Adds a string header named sKey with the value sValue.  The header value
 * MUST be properly encoded for the given header name -- if it might contain
 * any unsafe characters, it should probably be URL-encoded.
 */
void Request::AddHeader(const String& sKey, const String& sValue)
{
	m_tHeaderData.Overwrite(sKey, sValue);
}

/**
 * Remove a header from this HTTP::Request with key sKey.
 */
void Request::DeleteHeader(const String& sKey)
{
	(void)m_tHeaderData.Erase(sKey);
}

/** Public API increments m_RequestsInProgressCount. */
SharedPtr<RequestCancellationToken> Request::Start()
{
	// Cache cancellation token on the stack before continuing,
	// since as soon as the request is placed in the pending list,
	// it can be released and destroyed, invalidating the 'this'
	// pointer of this context.
	auto pCancellationToken(m_pCancellationToken);

	auto pHTTP = Manager::Get();
	if (pHTTP)
	{
		// Increment total requests in progress.
		++pHTTP->m_RequestsInProgressCount;

		// WARNING: DO NOT access 'this' after
		// the call to InternalStart. Once
		// the request has been placed in the start queue,
		// it can be immediately destroyed by another thread.

		// Finish start.
		InternalStart(pHTTP);
	}

	return pCancellationToken;
}

/**
 * Starts the HTTP request - private API does
 * not treat this as a new request but instead
 * a resend of another request (we only
 * increment m_RequestsInProgressCount in the public
 * Start() call.
 */
void Request::InternalStart(CheckedPtr<Manager> pHTTPManager)
{
	// Mark call to Start().
	m_iRequestStartTimeInTicks = SeoulTime::GetGameTimeInTicks();

	// Finalize post requests into the buffer now.
	FinalizePostData();

	// As soon as we leave this scope,
	// it is no longer safe to access this.
	{
		Lock lock(pHTTPManager->m_PendingRequestsMutex);

		// A resend request with defined lanes must be prioritized
		// to the front of the pending list.
		if (IsResendRequest() && 0 != GetLanesMask())
		{
			pHTTPManager->m_lPendingRequests.PushFront(this);
		}
		else
		{
			pHTTPManager->m_lPendingRequests.PushBack(this);
		}
		pHTTPManager->m_nPendingListSize++;

		// WARNING: DO NOT ACCESS 'this' after this line,
		// since it may have already been destroyed
		// by the HTTP::Manager worker thread (as soon as we release
		// the lock on m_PendingRequestsMutex).
	}

	// Activate the Tick worker so it processes the request.
	(void)Manager::s_TickWorkerSignal.Activate();
}

/**
 * @return A HTTP::Request instance configured to match this HTTP::Request instance,
 * without being started.
 *
 * This is equivalent to instantiating a new HTTP::Request
 * and then manually configuring it to match this HTTP::Request using public mutators.
 */
Request* Request::Clone() const
{
	Request* pReturn = SEOUL_NEW(MemoryBudgets::Network) Request(m_pCancellationToken, m_ResendTimer);
	pReturn->m_sURL = m_sURL;
	pReturn->m_bResendOnFailure = m_bResendOnFailure;
	pReturn->m_bVerifyPeer = m_bVerifyPeer;
	pReturn->m_bDispatchCallbackOnMainThread = m_bDispatchCallbackOnMainThread;
	pReturn->m_bIgnoreDomainRequestBudget = m_bIgnoreDomainRequestBudget;
	pReturn->m_sBodyDataOutputFilename = m_sBodyDataOutputFilename;
	pReturn->m_bBodyDataOutputFileOpenForResume = m_bBodyDataOutputFileOpenForResume;
	pReturn->m_OpenFileValidateCallback = m_OpenFileValidateCallback;
	pReturn->m_PrepForResendCallback = m_PrepForResendCallback;
	pReturn->m_sMethod = m_sMethod;
	pReturn->m_iConnectionTimeoutInSeconds = m_iConnectionTimeoutInSeconds;
	pReturn->m_iTransferTimeoutInSeconds = m_iTransferTimeoutInSeconds;
	pReturn->m_LanesMask = m_LanesMask;
	if (m_Node.GetOwner().IsValid())
	{
		pReturn->m_Node.Insert(*m_Node.GetOwner());
	}
	pReturn->m_Callback = m_Callback;
	pReturn->m_ProgressCallback = m_ProgressCallback;
	pReturn->m_tPostData = m_tPostData;
	pReturn->m_PostBody.CopyFrom(m_PostBody);
	pReturn->m_tHeaderData = m_tHeaderData;
	pReturn->m_fResendDelaySeconds = m_fResendDelaySeconds;
	pReturn->m_iRequestConstructTimeInTicks = m_iRequestConstructTimeInTicks;
	pReturn->m_Stats = m_Stats;

	return pReturn;
}

Bool Request::IsCanceling() const
{
	return m_pCancellationToken->IsCancelled();
}

void Request::InitializeResendRequest(Result eStatus, Int64 iNowTicks)
{
	if (!m_bIsResendRequest)
	{
		// Only track resend requests for networking related failures.
		if (Result::kSuccess != eStatus)
		{
			m_bIsNetworkFailureResendRequest = true;
			++Manager::Get()->m_NetworkFailureActiveResendRequests;
		}
	}

	m_bIsResendRequest = true;

	// Only increase resend interval if the request connected successfully.
	// We assume server load problems only if the API load balancer sends a
	// bad HTTP response.
	if (eStatus == Result::kSuccess || m_fResendDelaySeconds <= fEpsilon)
	{
		m_fResendDelaySeconds = m_ResendTimer.NextResendSeconds();
	}

	// TODO: Intentional constraint - this means certain stats will be
	// technically incorrect if there is no resend enabled (e.g. a network
	// failure will not be reported on a failed request that has no resend).
	//
	// Increment stats.
	++m_Stats.m_uResends;
	m_Stats.m_uNetworkFailures += (eStatus != Result::kSuccess ? 1u : 0u);
	m_Stats.m_uHttpFailures += (eStatus == Result::kSuccess ? 1u : 0u);

	m_iEarliestSendTimeInTicks = iNowTicks + SeoulTime::ConvertSecondsToTicks(m_fResendDelaySeconds);
}

/**
 * Actual implementation of start behavior. Expected to be called
 * from the ticker thread.
 */
void Request::TickerThreadPerformStart()
{
	// Accumulate delay now
	if (0 != m_iRequestStartTimeInTicks)
	{
		m_Stats.m_fApiDelaySecs += SeoulTime::ConvertTicksToSeconds(
			SeoulTime::GetGameTimeInTicks() - m_iRequestStartTimeInTicks);
		m_iRequestStartTimeInTicks = 0;
	}

	SEOUL_LOG_HTTP(
		"HTTPRequest::DoStart: resend=%s method=%s url=%s trace=%s\n",
		m_bResendOnFailure ? "true" : "false",
		m_sMethod.CStr(),
		m_sURL.CStr(),
		m_Stats.m_sRequestTraceId.CStr());

	Manager::Get()->StartHTTPRequest(this);
}

/**
 * Marks the request as completed with the given result
 */
void Request::Finish(Result eResult)
{
	m_eResult = eResult;

	// Tie up progress on results where we expect to receive
	// the entire body (sometimes we don't get the final progress
	// callback from curl).
	if (Result::kSuccess == eResult)
	{
		m_DownloadSizeSoFarInBytes = m_TotalDownloadSizeInBytes;
	}

	// Pass through file state and close the file, if it is opened.
	m_Response.m_bBodyFileWrittenSuccessfully = m_bBodyDataOutputFileOk;
	m_pBodyDataOutputFile.Reset();

	// Apply current stats to the response.
	m_Stats.m_fOverallSecs = SeoulTime::ConvertTicksToSeconds(
		SeoulTime::GetGameTimeInTicks() - m_iRequestConstructTimeInTicks);
	m_Response.m_Stats = m_Stats;

	SEOUL_LOG_HTTP("HTTPRequest::Finish(%.2f ms): resend=%s result=%s status=%d url=%s trace=%s\n",
		(m_Response.m_Stats.m_fTotalRequestSecs * 1000.0),
		m_bResendOnFailure ? "true" : "false",
		(eResult == Result::kSuccess ? "Success" :
		(eResult == Result::kCanceled ? "Canceled" :
			"Failure")),
			(Int)m_Response.m_nStatus,
		m_sURL.CStr(),
		m_Stats.m_sRequestTraceId.CStr());
	SEOUL_LOG_HTTP("HTTPRequest::Finish(stats): delay: %2.f ms, lookup: %.2f ms, connect: %2.f ms, appconnect: %2.f ms, pretransfer: %2.f ms, redirect: %.2f ms, starttransfer: %2.f ms, totalrequest: %.2f ms, overall: %.2f ms, %.2f B/s down, %2.f B/s up, trace: %s, fail-http: %u, fail-net: %u, resends: %u",
		(m_Stats.m_fApiDelaySecs * 1000.0),
		(m_Stats.m_fLookupSecs * 1000.0),
		(m_Stats.m_fConnectSecs * 1000.0),
		(m_Stats.m_fAppConnectSecs * 1000.0),
		(m_Stats.m_fPreTransferSecs * 1000.0),
		(m_Stats.m_fRedirectSecs * 1000.0),
		(m_Stats.m_fStartTransferSecs * 1000.0),
		(m_Stats.m_fTotalRequestSecs * 1000.0),
		(m_Stats.m_fOverallSecs * 1000.0),
		m_Stats.m_fAverageDownloadSpeedBytesPerSec,
		m_Stats.m_fAverageUploadSpeedBytesPerSec,
		m_Stats.m_sRequestTraceId.CStr(),
		m_Stats.m_uHttpFailures,
		m_Stats.m_uNetworkFailures,
		m_Stats.m_uResends);

	// Warning: as soon as m_bCompleted is set to true, the main thread
	// is allowed to destroy the request. Do not access it after this line.
	SeoulMemoryBarrier();
	m_bCompleted = true;
	SeoulMemoryBarrier();

	// Activate the Tick worker so it processes the request.
	(void)Manager::s_TickWorkerSignal.Activate();
}

/**
 * Called to setup m_PostBody - either left unmodified, if already
 * non-zero, or populated with the contents of m_tPostData.
 */
void Request::FinalizePostData()
{
	// If we have no direct post body, build it from
	// the table.
	if (m_PostBody.IsEmpty())
	{
		String sPostBody;

		{
			auto const iBegin = m_tPostData.Begin();
			auto const iEnd = m_tPostData.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				if (iBegin != i)
				{
					sPostBody.Append('&');
				}

				sPostBody.Append(i->First);
				sPostBody.Append('=');
				sPostBody.Append(i->Second);
			}
		}

		if (!sPostBody.IsEmpty())
		{
			void* p = nullptr;
			UInt32 u = 0u;
			sPostBody.RelinquishBuffer(p, u);
			m_PostBody.TakeOwnership((Byte*)p, u);
		}
	}

	// One way or another, done with the post table, flush it now.
	{
		PostDataTable emptyTable;
		m_tPostData.Swap(emptyTable);
	}
}

/**
 * Helper function called when we receive header data as part of the response
 */
void Request::OnHeaderReceived(const void *pData, size_t zSize)
{
	(void)m_Response.m_tHeaders.ParseAndAddHeader((Byte const*)pData, (UInt32)zSize);
}

/**
 * Helper function called when we receive data as part of the response
 */
void Request::OnDataReceived(const void *pData, size_t zSize)
{
	// Set uptime if not yet set.
	if (TimeInterval() == m_Response.m_UptimeValueAtReceive)
	{
		m_Response.m_UptimeValueAtReceive = g_pCoreVirtuals->GetUptime();
	}

	// If no output filename was specified, use the in memory buffer.
	if (m_sBodyDataOutputFilename.IsEmpty())
	{
		// Record that the data was truncated.
		if (!m_Response.AppendData(pData, zSize))
		{
			m_Response.m_bBodyDataTruncated = true;
		}
	}
	// Otherwise, write data to the file.
	else
	{
		// If we haven't tried to open the file, do so now.
		if (!m_bBodyDataOutputFileOpened)
		{
			// Before opening the file, allow the registered validation callback to check
			// the initial data. This allows responses to be checked before we modify the
			// file at all.
			m_bBodyDataOpenFileValidateCallbackPassed = (!m_OpenFileValidateCallback || m_OpenFileValidateCallback(pData, zSize));
			if (m_bBodyDataOpenFileValidateCallbackPassed)
			{
				FileManager::Get()->OpenFile(
					m_sBodyDataOutputFilename,
					(m_bBodyDataOutputFileOpenForResume ? File::kWriteAppend : File::kWriteTruncate),
					m_pBodyDataOutputFile);
			}

			m_bBodyDataOutputFileOpened = true;

			// The file is ok if it was opened successfully and supports writing.
			m_bBodyDataOutputFileOk = (
				m_pBodyDataOutputFile.IsValid() &&
				m_pBodyDataOutputFile->CanWrite());
		}

		// If the file is ready, write data.
		if (m_bBodyDataOutputFileOk)
		{
			UInt32 const zWriteSize = (UInt32)zSize;
			m_bBodyDataOutputFileOk = (zWriteSize == m_pBodyDataOutputFile->WriteRawData(pData, zWriteSize));
		}
	}
}

#if SEOUL_WITH_CURL
/**
 * Curl SSL context function to provide custom SSL behavior
 */
int Request::CurlSSLContextFunction(void *pContext)
{
	SSL_CTX *pSSLContext = (SSL_CTX *)pContext;

	STACK_OF(X509_INFO) *pCertificateChain = Manager::Get()->m_pCertificateChain;

	if (pCertificateChain != nullptr)
	{
		// Add our root certificate to the X509 certificate store
		X509_STORE *pCertStore = SSL_CTX_get_cert_store(pSSLContext);

		// For each certificate and CRL in our chain, add it to the certificate
		// store
		for (int i = 0; i < sk_X509_INFO_num(pCertificateChain); i++)
		{
			X509_INFO *pInfo = sk_X509_INFO_value(pCertificateChain, i);

			if (pInfo->x509 != nullptr)
			{
				X509_STORE_add_cert(pCertStore, pInfo->x509);
			}

			if (pInfo->crl)
			{
				X509_STORE_add_crl(pCertStore, pInfo->crl);
			}
		}
	}

	return CURLE_OK;
}

void Request::CurlStart(CURLSH* pCurlShare, CURLM* pCurlMulti, const String& sUserAgent)
{
	SEOUL_ASSERT(IsHttpApiThread());

	// Setup the curl request
	SEOUL_ASSERT(m_pCurl == nullptr);
	m_pCurl = curl_easy_init();
	SEOUL_ASSERT(m_pCurl != nullptr);

	// Essentially configuration.
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_URL, m_sURL.CStr()));
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_SHARE, pCurlShare));

	if (m_sMethod == Method::ksHead)
	{
		SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 1L));
	}
	else if (m_sMethod == Method::ksPost)
	{
		SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_POST, 1L));

		// Don't send the "Expect: 100-Continue" header
		m_pHeaderList = curl_slist_append(m_pHeaderList, "Expect:");

		// Set the POST body.  Note that m_PostBody MUST NOT be modified until
		// the request completes, since curl does not copy the data.
		void* pPostData = (m_PostBody.IsEmpty() ? nullptr : (void*)m_PostBody.GetBuffer());
		curl_off_t uPostData = (curl_off_t)(m_PostBody.GetTotalDataSizeInBytes());

		SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, pPostData));
		SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDSIZE_LARGE, uPostData));
	}

	// For each custom header...
	{
		String sHeader;
		auto const iBegin = m_tHeaderData.Begin();
		auto const iEnd = m_tHeaderData.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			// Add it to the list of curl headers
			sHeader = i->First;
			sHeader.Append(':');
			sHeader.Append(' ');
			sHeader.Append(i->Second);
			m_pHeaderList = curl_slist_append(m_pHeaderList, sHeader.CStr());
		}
	}

	// Enable HTTP/2.
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0));

	// Set timeout options
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, (long)m_iConnectionTimeoutInSeconds));
	// Abort request after m_iTimeoutInSeconds of 512 b/s transfer speed.
	// See: http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTLOWSPEEDLIMIT
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_LOW_SPEED_LIMIT, 512L));
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_LOW_SPEED_TIME, (long)m_iTransferTimeoutInSeconds));

	// Set SSL options
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1));
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, (m_bVerifyPeer ? 1L : 0L)));

	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_SSL_CTX_FUNCTION, &::Seoul::HTTP::CurlSSLContextFunction));
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_SSL_CTX_DATA, this));

	// Set User-Agent string
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_USERAGENT, sUserAgent.CStr()));

	// Set HTTP headers
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, m_pHeaderList));

	// Set out header callback.
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_HEADERFUNCTION, &Request::StaticCurlHeaderCallback));
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_HEADERDATA, this));

	// Set our data callback
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &Request::StaticCurlWriteCallback));
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, this));

	// Set our progress callback to enable canceling
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_NOPROGRESS, 0L));
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_PROGRESSFUNCTION, &Request::StaticCurlProgressCallback));
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_PROGRESSDATA, this));

	// Enable redirect following (302 response, etc.)
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_FOLLOWLOCATION, 1L));

	// Set pointer to ourselves for later lookup
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_PRIVATE, this));

	// Disable signaling - by default, this is used to interrupt
	// DNS operations. Although not strictly necessary as long as our dns queries
	// are threaded, we never want this to happen (causes crashes in a multithreaded
	// environment). See http://horstr.blogspot.com/2008/04/on-libcurl-openssl-and-thread-safety.html
	// among other discussion.
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_NOSIGNAL, 1L));

	// Passing "" tells curl to use all supported encodings:
	// http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTACCEPTENCODING
	SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_ACCEPT_ENCODING, ""));

#if SEOUL_LOGGING_ENABLED
	if (Manager::Get()->m_bVerboseHttp2LogsEnabled)
	{
		SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_DEBUGFUNCTION, CurlVerboseLogFunction));
		SEOUL_VERIFY_CURLE(curl_easy_setopt(m_pCurl, CURLOPT_VERBOSE, true));
	}
#endif // /#if SEOUL_LOGGING_ENABLED

	// Enqueue the request.
	SEOUL_VERIFY_CURLM(curl_multi_add_handle(pCurlMulti, m_pCurl));
}

/**
 * Callback called from curl for each header we receive of the response
 */
size_t Request::StaticCurlHeaderCallback(char *ptr, size_t size, size_t nitems, void *pUserData)
{
	// Watch out for overflow
	SEOUL_ASSERT(nitems == 0 || size <= SIZE_MAX / nitems);

	Request *pRequest = (Request *)pUserData;
	pRequest->OnHeaderReceived(ptr, size * nitems);

	return nitems * size;
}

/**
 * Callback called from curl for each piece of data we receive of the response
 */
size_t Request::StaticCurlWriteCallback(char *ptr, size_t size, size_t nmemb, void *pUserData)
{
	// Watch out for overflow
	SEOUL_ASSERT(nmemb == 0 || size <= SIZE_MAX / nmemb);

	Request *pRequest = (Request *)pUserData;
	pRequest->OnDataReceived(ptr, size * nmemb);

	return nmemb * size;
}

/**
 * Callback called from curl to inform us of upload and download progress
 *
 * @return 0 to continue the request, or non-0 to abort it
 */
int Request::StaticCurlProgressCallback(void *pUserData, double fDownloadTotal, double fDownloadSoFar, double fUploadTotal, double fUploadSoFar)
{
	Request *pRequest = (Request *)pUserData;

	// Update progress.
	pRequest->m_TotalDownloadSizeInBytes.Set((Atomic32Type)((Int32)Floor(fDownloadTotal)));
	pRequest->m_DownloadSizeSoFarInBytes.Set((Atomic32Type)((Int32)Floor(fDownloadSoFar)));

	// If the request has a progress callback, wake up the
	// tick thread so it can report the progress.
	if (pRequest->m_ProgressCallback)
	{
		Manager::s_TickWorkerSignal.Activate();
	}

	// If we're trying to cancel the request, inform curl thusly
	return pRequest->IsCanceling() ? 1 : 0;
}
#endif // /#if SEOUL_WITH_CURL

} // namespace Seoul::HTTP
