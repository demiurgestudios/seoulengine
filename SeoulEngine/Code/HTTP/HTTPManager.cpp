/**
 * \file HTTPManager.cpp
 * \brief Singleton class for platform-independent HTTP requests.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

// TODO: Refactor this class into an abstract base class with platform-specific
// subclasses for implementation details.  This is starting to get rather ugly
// with all of the platforms' code mixed together.

#include "BuildChangelistPublic.h"
#include "BuildDistro.h"
#include "BuildVersion.h"
#include "CoreVirtuals.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "HTTPExternalDefine.h"
#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "IOSUtil.h"
#include "JobsManager.h"
#include "Logger.h"
#include "MemoryBarrier.h"
#include "Path.h"
#include "SecureRandom.h"
#include "SeoulSocket.h"
#include "SeoulTime.h"
#include "SeoulUUID.h"
#include "StringUtil.h"

namespace Seoul::HTTP
{

/** Default max size of the pending request list. */
static const Int32 kDefaultMaxPendingListSize = 1024;

static const String ksAmazonTraceIdHeader("x-amzn-trace-id");

static const String ksDemiurgeBuildChangelistHeader("x-demiurge-build-changelist");
static const String ksDemiurgeBuildVersionHeader("x-demiurge-build-version");
static const String ksDemiurgeClientPlatformHeader("x-demiurge-client-platform");
static const String ksDemiurgeRequestIdHeader("x-demiurge-request-id");
static const String ksDemiurgeRetryTokenHeader("x-demiurge-retry-token");
static const String ksDemiurgeDeviceTokenHashHeader("x-demiurge-device-token-hash");

static String GetLoadShedPlatformUuidHash()
{
	// Salt the platform UUID with the current date, so players get different load
	// shedding luck every day (rather than always shedding the same players first)
	Int32 const iDayNumber = (Int32)((Int64)WorldTime::GetUTCTime().GetSeconds() / (Int64)WorldTime::kDaysToSeconds);
	auto const sSaltedUuid = String::Printf("%d%s%d", iDayNumber, g_pCoreVirtuals->GetPlatformUUID().CStr(), iDayNumber);

	// Return hash as a padded string, from 00 to 99
	auto const uHash = GetHash(sSaltedUuid) % 100u;
	return String::Printf("%02u", uHash);
}

#if SEOUL_WITH_CURL
/**
 * Default, min, and max signal wait time of the curl thread, in milliseconds.
 *
 * We clamp the curl timeout values, since they're intended to be used when we're
 * waiting on curl's file descriptor handles in addition to a timeout, which means
 * they can be significantly too long if we just wait on the timeout only.
 *
 * IMPORTANT: We need to avoid calling curl_multi_perform too frequently, as
 * it can flood an internal timeout buffer, effectively resulting in a memory leak.
 */
static const long kiDefaultCurlSignalWaitInMilliseconds = 25;
static const long kiMinCurlSignalWaitInMilliseconds = 1;
static const long kiMaxCurlSignalWaitInMilliseconds = 50;

/** Handling from our special hook - prune connections in the connection cache after they have been idle for longer than this time. */
static const long kiCurlIdleCachedConnectionTimeoutMilliseconds = 29000; // 29 seconds.

static inline void MillisecondsToTimeval(long iMilliseconds, timeval& rTimeout)
{
	rTimeout.tv_sec = (iMilliseconds / 1000);
	rTimeout.tv_usec = (iMilliseconds % 1000) * 1000;
}

#endif // /#if SEOUL_WITH_CURL

/** Global signal used to wake up the tick worker thread. */
Signal Manager::s_TickWorkerSignal{};

/** Global signal used to wake up the API worker thread. */
Signal Manager::s_ApiSignal{};

/** Global id used to track API thread ownership, mainly for debugging purposes. */
static ThreadId s_ApiThreadId;

/**
 * Update the current thread id.
 */
void SetHttpApiThreadId(const ThreadId& id)
{
	SEOUL_ASSERT(!id.IsValid() || !s_ApiThreadId.IsValid());
	s_ApiThreadId = id;
}

/**
 * @return True if the current thread is the API worker thread, false otherwise.
 */
Bool IsHttpApiThread()
{
	return (s_ApiThreadId == Thread::GetThisThreadId());
}

#if SEOUL_WITH_CURL
/**
 * malloc() override for libcurl
 */
static void* CurlMalloc(size_t zSize)
{
	return MemoryManager::Allocate(zSize, MemoryBudgets::Network);
}

/**
 * calloc() override for libcurl
 */
static void* CurlCalloc(size_t nMemb, size_t zSize)
{
	// Check for overflow
	if (nMemb != 0 && zSize > SIZE_MAX / nMemb)
	{
		return nullptr;
	}

	void *pMemory = CurlMalloc(nMemb* zSize);
	if (pMemory != nullptr)
	{
		memset(pMemory, 0, nMemb * zSize);
	}

	return pMemory;
}

/**
 * realloc() override for libcurl
 */
static void* CurlRealloc(void* pPtr, size_t zSize)
{
	return MemoryManager::Reallocate(pPtr, zSize, MemoryBudgets::Network);
}

/**
 * free() override for libcurl
 */
static void CurlFree(void* pPtr)
{
	MemoryManager::Deallocate(pPtr);
}

/**
 * strdup() override for libcurl
 */
static Byte* CurlStrdup(Byte const* pStr)
{
	size_t zLength = strlen(pStr);

	Byte* pNewStr = (Byte*)CurlMalloc(zLength + 1);
	if (pNewStr != nullptr)
	{
		memcpy(pNewStr, pStr, zLength + 1);
	}

	return pNewStr;
}
#endif // /#if SEOUL_WITH_CURL

String Manager::ParseURLDomain(String const& sURL)
{
	UInt indexAfterDoubleSlash = sURL.Find("//");
	if (indexAfterDoubleSlash == String::NPos)
	{
		indexAfterDoubleSlash = 0;
	}
	else
	{
		indexAfterDoubleSlash += 2;
	}

	UInt indexOfFirstSlashOrColon = sURL.FindFirstOf(":/", indexAfterDoubleSlash);
	if (indexOfFirstSlashOrColon == String::NPos)
	{
		return sURL.Substring(indexAfterDoubleSlash);
	}

	return sURL.Substring(indexAfterDoubleSlash, indexOfFirstSlashOrColon - indexAfterDoubleSlash);
}

Manager::Manager(const ManagerSettings& settings)
	: m_Lanes(0)
	, m_ResendTimer()
	, m_NetworkFailureActiveResendRequests(0)
	, m_nMaxPendingListSize(kDefaultMaxPendingListSize)
	, m_nPendingListSize(0)
	, m_bShuttingDown(false)
	, m_Settings(settings)
	, m_bTickWorkerShuttingDown(false)
	, m_pTickWorkerThread(nullptr)
	, m_bTickThreadDoCancelAllRequests(false)
	, m_bPendingCancelledRequests(false)
	, m_iDomainRequestBudgetInitial(20)
	, m_DomainRequestBudgetIncreaseInterval(1, 0)
	, m_tDomainRequestBudgets()
	, m_MainThreadFinishedBuffer()
	, m_MainThreadNeedsResendCallbackBuffer()
	, m_pApiWorkerThread(nullptr)
	, m_ApiToStartBuffer()
	, m_ApiToCancelBuffer()
	, m_bApiShuttingDown(false)
#if SEOUL_WITH_CURL
	, m_pCertificateChain(nullptr)
	, m_bVerboseHttp2LogsEnabled(false)
#endif
	, m_iLastBackgroundGameTimeInTicks(0)
	, m_bInBackground(false)
{
	m_ResendTimer.ResetResendSeconds();

	// API thread.
	m_pApiWorkerThread.Reset(SEOUL_NEW(MemoryBudgets::Network) Thread(
		SEOUL_BIND_DELEGATE(&Manager::InternalApiThread, this),
		false));
	m_pApiWorkerThread->Start("HTTP API Thread");
	m_pApiWorkerThread->SetPriority(Thread::kLow);

	// Ticker thread for dispatching.
	m_pTickWorkerThread.Reset(SEOUL_NEW(MemoryBudgets::Network) Thread(
		SEOUL_BIND_DELEGATE(&Manager::InternalTickWorkerThread, this),
		false));
	m_pTickWorkerThread->Start("HTTP Tick Thread");
	m_pTickWorkerThread->SetPriority(Thread::kLow);
}

Manager::~Manager()
{
	// Not in the background on shutdown.
	m_bInBackground = false;

	m_bShuttingDown = true;
	CancelAllRequestsForDestruction();

	// Wake up the tick worker and shut it down.
	m_bTickWorkerShuttingDown = true;
	(void)s_TickWorkerSignal.Activate();
	m_pTickWorkerThread.Reset();

	// Wake up the API worker and shut it down.
	m_bApiShuttingDown = true;
	s_ApiSignal.Activate();
	m_pApiWorkerThread.Reset();

	{
		Lock lock(m_PendingRequestsMutex);
		SEOUL_ASSERT(m_lPendingRequests.IsEmpty());
	}
	SEOUL_ASSERT(m_lActiveRequests.IsEmpty());
}

void Manager::OnEnterBackground()
{
	// Now in the background.
	if (false == m_bInBackground.CompareAndSet(true, false))
	{
		// Log for testing and debug tracking.
		SEOUL_LOG_HTTP("HTTP::Manager::OnEnterBackground()");

		// Tracking to suppress certain warnings.
		m_iLastBackgroundGameTimeInTicks = SeoulTime::GetGameTimeInTicks();
	}
}

void Manager::OnLeaveBackground()
{
	if (true == m_bInBackground.CompareAndSet(false, true))
	{
		// Log for testing and debug tracking.
		SEOUL_LOG_HTTP("HTTP::Manager::OnLeaveBackground()");

		// Wake up the API worker.
		(void)s_ApiSignal.Activate();

		// Wake up the tick worker.
		(void)s_TickWorkerSignal.Activate();
	}
}

/**
 * Creates a new HTTP request.  The caller is responsible for setting up all of
 * the applicable arguments and then calling Start() on the request to actually
 * start it.
 */
Request& Manager::CreateRequest(RequestList* pClientList /*= nullptr*/)
{
	// It's a bug for code to try to start a new request during shutdown
	SEOUL_ASSERT(!m_bShuttingDown);

	Request *pRequest = SEOUL_NEW(MemoryBudgets::Network) Request(
		SharedPtr<RequestCancellationToken>(SEOUL_NEW(MemoryBudgets::Network) RequestCancellationToken),
		m_ResendTimer);

	// Add the request to the client list, if defined.
	if (nullptr != pClientList)
	{
		pRequest->m_Node.Insert(*pClientList);
	}

	AddSeoulEngineHeaders(pRequest, false);

	return *pRequest;
}

/**
 * TODO: Fix - this is a bad API design.
 *
 * Rare use - only valid if a request can be started but then never
 * sent via Start().
 */
void Manager::DestroyUnusedRequest(Request*& rp) const
{
	auto p = rp;
	rp = nullptr;

	if (p)
	{
		// Sanity - must be 0 or this was an active request.
		SEOUL_ASSERT(0 == p->m_iRequestStartTimeInTicks);

		// Make sure the node is removed from any list it may
		// still be a part of *before* entering the destructor.
		p->m_Node.Remove();
	}

	// No adjustment of m_RequestsInProgressCount, since
	// this request was never started.

	SEOUL_DELETE p;
}

Request* Manager::CloneRequest(const Request& request)
{
	// It's a bug for code to try to start a new request during shutdown
	SEOUL_ASSERT(!m_bShuttingDown);
	auto clone = request.Clone();

	// Cloned requests still get a unique request ID, to disambiguate logs
	AddSeoulEngineHeaders(clone, true);
	return clone;
}

static inline String MakeRandom128BitToken()
{
	const UInt32 knBytes = 16;
	UInt8 bytes[knBytes];
	SecureRandom::GetBytes(bytes, knBytes);

	return HexDump(bytes, knBytes, false);
}

void Manager::AddSeoulEngineHeaders(Request* pRequest, Bool bResend)
{
	// Most headers are not refreshed on resend - request ID is,
	// so a new one is always "added" (it will overwrite the existing value).
	if (!bResend)
	{
		pRequest->AddHeader(ksDemiurgeBuildChangelistHeader, g_sBuildChangelistStr);
		pRequest->AddHeader(ksDemiurgeBuildVersionHeader, BUILD_VERSION_STR);
		AddSeoulEnginePlatformHeader(pRequest);
		pRequest->AddHeader(ksDemiurgeRetryTokenHeader, MakeRandom128BitToken());
		pRequest->AddHeader(ksDemiurgeDeviceTokenHashHeader, GetLoadShedPlatformUuidHash());
	}

	const auto sTraceId = UUID::GenerateV4().ToString();
	pRequest->m_Stats.m_sRequestTraceId = sTraceId;
	// ELB trace IDs follow the format: [version]-[epoch time base16]-[trace id]
	pRequest->AddHeader(ksAmazonTraceIdHeader, String::Printf("Root=1-00000000-%s", sTraceId.CStr()));
	pRequest->AddHeader(ksDemiurgeRequestIdHeader, sTraceId);
}

void Manager::AddSeoulEnginePlatformHeader(Request* pRequest)
{
	if (!m_Settings.m_sSubPlatform.IsEmpty())
	{
		pRequest->AddHeader(
			ksDemiurgeClientPlatformHeader,
			String::Printf("%s.%s", GetCurrentPlatformName(), m_Settings.m_sSubPlatform.CStr()));
	}
	else
	{
		pRequest->AddHeader(
			ksDemiurgeClientPlatformHeader,
			GetCurrentPlatformName());
	}
}

/**
 * Runs any pending callbacks for completed requests
 */
void Manager::Tick()
{
	// Make sure the HTTP system does not think it's in the background when Tick() is called.
	OnLeaveBackground();

	// Track whether we need to wake up the tick worker after processing.
	Bool bTriggerTickWorker = false;

	{
		Request* pRequest = m_MainThreadFinishedBuffer.Pop();
		while (nullptr != pRequest)
		{
			CallRequestCallback(pRequest);

			// Remove the request's lanes contribution now that
			// it is fully complete.
			Atomic32Type const lanes = pRequest->GetLanesMask();
			m_Lanes &= ~lanes;

			// If lanes was non-zero, it means applying it to m_Lanes changed
			// its state, so trigger the ticker thread again since it may
			// now have work to do.
			if (0 != lanes)
			{
				bTriggerTickWorker = true;
			}

			// Make sure the node is removed from any list it may
			// still be a part of *before* entering the destructor.
			pRequest->m_Node.Remove();

			SEOUL_DELETE pRequest;
			pRequest = m_MainThreadFinishedBuffer.Pop();
		}
	}

	// Activate if requested.
	if (bTriggerTickWorker)
	{
		s_TickWorkerSignal.Activate();
	}
}

void Manager::CallRequestCallback(Request* pRequest)
{
	// We consider the request complete unless a resend
	// occurs below.
	SEOUL_ASSERT(m_RequestsInProgressCount > 0);
	--m_RequestsInProgressCount;

	if (!pRequest->m_Callback)
	{
		return;
	}

	// Track the request callback to make sure we don't try to cancel its own request list in the callback.
	// That causes a deadlock, and removing it from the request list before the callback means resend requests
	// aren't tracked against any request list.
	CallbackResult eCallbackResult;
	{
		ScopedHTTPRequestListCallbackCount counter(pRequest->m_Node.GetOwner());

		// Filtering - if m_bCancelling is true (an explicit cancel was requested at any point),
		// we always report the request as cancelled.
		if (pRequest->IsCanceling()) { pRequest->m_eResult = Result::kCanceled; }
		eCallbackResult = pRequest->m_Callback(pRequest->m_eResult, &pRequest->m_Response);
	}

	if (pRequest->m_bResendOnFailure && eCallbackResult == CallbackResult::kNeedsResend)
	{
		// Restore the request on successful resend.
		if (QueueResendRequest(pRequest))
		{
			++m_RequestsInProgressCount;
		}
	}
	// Merge stats on success.
	else if (eCallbackResult == CallbackResult::kSuccess)
	{
		m_MaxRequestStats.Merge(
			pRequest->m_sURL,
			pRequest->m_Response.m_Stats);
	}
}

/**
 * Cancels all requests which are currently in progress.  Callbacks will be
 * called immediately.
 */
void Manager::CancelAllRequestsForDestruction()
{
	// Make sure the HTTP system does not think it's in the background when cancelling.
	OnLeaveBackground();

	// Tell the tick thread to cancel all requests.
	m_bTickThreadDoCancelAllRequests = true;

	// Keep looping until we've cancelled all requests.
	while (m_bTickThreadDoCancelAllRequests)
	{
		// Wake up the API worker.
		(void)s_ApiSignal.Activate();

		// Wake up the tick worker.
		(void)s_TickWorkerSignal.Activate();

		// Tick to run callbacks
		Tick();
	}

	// Tick one last time.
	Tick();
}

Bool Manager::HasRequests() const
{
	return (0 != m_RequestsInProgressCount);
}

#if SEOUL_WITH_CURL
/**
 * Parses our root SSL certificate into OpenSSL's internal format
 */
void Manager::ParseSSLCertificate()
{
	SEOUL_ASSERT(IsHttpApiThread());

	if (m_Settings.m_sSSLCertificates.IsEmpty())
	{
		return;
	}

	// Create a memory BIO (basic I/O) object to read from our raw certificate
	// data
	BIO *pBio = BIO_new_mem_buf(const_cast<char *>(m_Settings.m_sSSLCertificates.CStr()),
								(int)m_Settings.m_sSSLCertificates.GetSize());

	// Read the X509 info
	m_pCertificateChain = PEM_X509_INFO_read_bio(pBio, nullptr, nullptr, nullptr);

	BIO_free(pBio);

	if (m_pCertificateChain == nullptr)
	{
		SEOUL_WARN("HTTP::Manager::ParseSSLCertificate: Failed to read X509 info");
	}
}
#endif // /#if SEOUL_WITH_CURL

void Manager::StartHTTPRequest(Request* pRequest)
{
	// Put the request on the start queue.
	m_ApiToStartBuffer.Push(pRequest);

	// Wake up the API thread, so it starts the request right away.
	s_ApiSignal.Activate();
}

void Manager::SetDomainRequestBudgetSettings(Int iInitialBudget, Int iSecondsPerIncrease)
{
	m_iDomainRequestBudgetInitial = iInitialBudget;
	m_DomainRequestBudgetIncreaseInterval = TimeInterval::FromSecondsInt64(iSecondsPerIncrease);
}

/**
 * URL-encodes the given string according to RFC 1738 section 2.2
 * (http://tools.ietf.org/html/rfc1738#section-2.2)
 *
 * Characters which are not printable in ASCII, as well as characters which are
 * considered unsafe or reserved are percent-encoded using their hexadecimal
 * values.  Additionally, all line endings are converted to CRLF line endings
 * ("%0D%0A").
 *
 * See also http://www.w3.org/TR/html401/interact/forms.html#h-17.13.4.1 for
 * a description of the application/x-www-form-urlencoded content type.
 */
String Manager::URLEncode(const String& sStr)
{
	// To try to avoid reallocations, we assume that the output is going to
	// be no more than 10% larger than the input.
	String sResult;
	sResult.Reserve(sStr.GetSize() * 11 / 10);

	for (UInt i = 0; i < sStr.GetSize(); i++)
	{
		Byte c = sStr[i];
		if ((c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9') ||
			c == '$' ||
			c == '-' ||
			c == '_' ||
			c == '.' ||
			c == '!' ||
			c == '*' ||
			c == '\'' ||
			c == '(' ||
			c == ')' ||
			c == ',')
		{
			// Encode safe characters directly
			sResult.Append(c);
		}
		else if (c == '\n' && (i == 0 || sStr[i - 1] != '\r'))
		{
			// Convert raw newlines to CRLFs
			sResult.Append("%0D%0A");
		}
		else
		{
			// Percent-encode all other characters
			static const char kaHexDigits[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
			UByte uCodePoint = (UByte)c;
			sResult.Append('%');
			sResult.Append(kaHexDigits[uCodePoint >> 4]);
			sResult.Append(kaHexDigits[uCodePoint & 0x0F]);
		}
	}

	return sResult;
}

void Manager::EnableVerboseHttp2Logging(Bool bEnabled)
{
#if SEOUL_WITH_CURL
	m_bVerboseHttp2LogsEnabled = bEnabled;
#endif
}

Int Manager::InternalTickWorkerThread(const Thread&)
{
	// Time to sleep in ms when in a typical processing loop.
	static const UInt32 kuSleepTimeInMilliseconds = 25;

	// Flag used to track whether this thread did any work this pass.
	Bool bDidWork = true;

	// Placeholder Mutex so simplify some logic.
	Mutex nopMutex;

	// Keep looping until the end.
	while (!m_bTickWorkerShuttingDown)
	{
		// Scope the loop so we don't "leak" auto-released objects forever.
		// For Job system threads, this is handled, but we must do this
		// explicitly since we are our own thread.
		ScopedAutoRelease autoRelease;

		// If in the background or if there are no active requests,
		// wait to be told to wake up.
		Bool bPendingRequestsEmpty = false;
		{
			Lock lock(m_PendingRequestsMutex);
			bPendingRequestsEmpty = m_lPendingRequests.IsEmpty();
		}
		if (bPendingRequestsEmpty)
		{
			s_TickWorkerSignal.Wait();
		}
		// Otherwise, yield some thread time if we didn't do any work the last time around.
		else if (!bDidWork)
		{
			s_TickWorkerSignal.Wait(kuSleepTimeInMilliseconds);
		}

		bDidWork = false;

		// If we're shutting down, exiting immediately (we check
		// again here since the signal wait above gives plenty
		// of time for the state of shutting down to change).
		if (m_bTickWorkerShuttingDown)
		{
			break;
		}

		UpdateDomainRequestBudgets();

		{
			// Add any newly created requests to the active list and clear the
			// pending list
			Lock lock(m_PendingRequestsMutex);

			// Only start requests that are scheduled to run now (or sooner)
			auto iNowTicks = SeoulTime::GetGameTimeInTicks();

			// Cache the current lanes mask here - we don't want it to change during
			// one pass of the requests lists (this is fine, as the only way it could
			// change mid iteration is if the main thread clears bits in the mask,
			// making it possible for a request to be issued, which means our local
			// copy is more restrictive, not less).
			Atomic32 lanesCopy = m_Lanes;
			for (auto reqIter = m_lPendingRequests.Begin(); reqIter != m_lPendingRequests.End(); )
			{
				Request* const pRequest = *reqIter;

				// Make sure we have room in the HTTP request budget to start this request.
				// Ignore HTTP budget if we're canceling all requests.
				Bool bIgnoreRequestBudget = m_bTickThreadDoCancelAllRequests || pRequest->m_bIgnoreDomainRequestBudget || pRequest->IsCanceling();
				Bool bHasRequestBudget = bIgnoreRequestBudget || !IsDomainRequestBudgetZero(*pRequest);
				Bool bBeforeEarliestSendTime = iNowTicks < pRequest->m_iEarliestSendTimeInTicks;

				// Make sure the required lanes of this request are free.
				Atomic32Type const lanes = pRequest->GetLanesMask();
				Bool bLaneIsFree = 0 == (lanesCopy & lanes);

				if (bHasRequestBudget && bLaneIsFree && !bBeforeEarliestSendTime)
				{
					// Occupy lanes.
					m_Lanes |= lanes;
					lanesCopy |= lanes;

					if (!bIgnoreRequestBudget)
					{
						DecrementDomainRequestBudget(*pRequest);
					}

					// Push the request to the active list.
					m_lActiveRequests.PushBack(pRequest);
					reqIter = m_lPendingRequests.Erase(reqIter);
					--m_nPendingListSize;
					SEOUL_ASSERT(m_nPendingListSize >= 0);

					// Actually start the request.
					pRequest->TickerThreadPerformStart();
				}
				// Advance to the next item.
				else
				{
					++reqIter;
				}
			}
		}

		// If cancelling all requests, perform the cancellation now.
		if (m_bTickThreadDoCancelAllRequests)
		{
			// Cancel all active requests
			for (auto& pRequest : m_lActiveRequests)
			{
				pRequest->m_pCancellationToken->Cancel();
			}
		}

		// Flag used to track whether resends are in progress (either already in the active queue,
		// or added to the queue this tick).
		Bool bResending = false;

		// Shared function for checking and reporting
		// progress.
		auto const reportProgress = [&](Request* pRequest)
		{
			// Check and report.
			if (pRequest->m_ProgressCallback)
			{
				if (pRequest->m_DownloadSizeSoFarInBytes != pRequest->m_LastReportedDownloadSizeSoFarInBytes)
				{
					pRequest->m_ProgressCallback(pRequest, pRequest->m_TotalDownloadSizeInBytes, pRequest->m_DownloadSizeSoFarInBytes);
					pRequest->m_LastReportedDownloadSizeSoFarInBytes = pRequest->m_DownloadSizeSoFarInBytes;
				}
			}
		};

		// Track if any requests have been cancelled.
		Bool bHasCancelledRequests = false;
		for (auto reqIter = m_lActiveRequests.Begin(); reqIter != m_lActiveRequests.End(); )
		{
			// Run the callback for each completed request and remove it from the
			// list.  If any callback starts a new request, it gets added to the
			// pending list and so doesn't screw with our iteration.
			auto pRequest = *reqIter;

			// Check if progress must be reported now.
			reportProgress(pRequest);

			// Track that we have a cancelled request.
			if (pRequest->IsCanceling())
			{
				// Propagate if needed.
				if (pRequest->m_bApiHasStarted &&
					!pRequest->m_bApiCancelRequested &&
					!pRequest->m_bCompleted)
				{
					pRequest->m_bApiCancelRequested = true;
					m_ApiToCancelBuffer.Push(pRequest);

					// Wake up the API thread to process the request.
					s_ApiSignal.Activate();
				}

				bHasCancelledRequests = true;
			}

			Bool bWaitingOnCancelRequest = (pRequest->m_bApiCancelRequested && !pRequest->m_bApiCancelRequestCompleted);
			if (!pRequest->m_bCompleted || bWaitingOnCancelRequest)
			{
				// Record that we're actively resending if this entry is a resend request.
				bResending = bResending || pRequest->IsResendRequest();

				// Don't erase this request yet
				++reqIter;
				continue;
			}

			// One way or another we're about to finish (or resend) the request,
			// so update progress again. This handles race conditions where progress
			// changed between the previous call above and here.
			reportProgress(pRequest);

			// TODO: Note that this lock depends
			// on our Mutex class being re-entrant per thread on all
			// platforms (a lock of a Mutex already locked on the current
			// thread succeeds by incrementing a reference count - the
			// lock is released once the reference count hits 0).

			// If the request has a membership list, we want this entire
			// block to be synchronized, incase BlockAndCancelAll() has
			// been called on the list. Otherwise, a race condition
			// can occur, where we're setting up a resend request in-between
			// cancellation attempts, resulting in some requests still
			// being scheduled for resend (even when they've been
			// cancelled).
			Lock lock(pRequest->m_Node.GetOwner().IsValid()
				? pRequest->m_Node.GetOwner()->GetMutex()
				: nopMutex);

			bDidWork = true;

			// By default, a completed entry is removed from the queue.
			Bool bDeleteEntry = true;

			// Resend if:
			// - the HTTP system is not shutting down
			// - this is not a tick loop specifically to clear out requests (tick for completion)
			// - the request specifies resend-on-failure
			// - the request was not cancelled (this is to ensure we don't attempt a resend on a cancel even if the request completed successfully).
			// - the result of the request is HTTP::Result::kFailure or HTTP::Result::kConnectFailure
			// - there is sufficient space to enqueue a resend request.
			Bool bResultIsFailure = (
				Result::kFailure == pRequest->m_eResult ||
				Result::kConnectFailure == pRequest->m_eResult);
			if (!m_bShuttingDown &&
				!m_bTickThreadDoCancelAllRequests &&
				pRequest->m_bResendOnFailure &&
				!pRequest->IsCanceling() &&
				bResultIsFailure &&
				m_nPendingListSize < m_nMaxPendingListSize)
			{
				QueueResendRequest(pRequest);
				bResending = true;
			}
			// Otherwise, dispatch and cleanup the request.
			else
			{
				// Track if we've abandoned a retry because of the max list size.
#if SEOUL_LOGGING_ENABLED
				if (!m_bShuttingDown &&
					!m_bTickThreadDoCancelAllRequests &&
					pRequest->m_bResendOnFailure &&
					!pRequest->IsCanceling() &&
					bResultIsFailure &&
					m_nPendingListSize >= m_nMaxPendingListSize)
				{
					SEOUL_WARN("Not retrying HTTP request to %s due to pending list size of %u which is greater than the max of %u",
						pRequest->GetURL().CStr(),
						m_nPendingListSize,
						m_nMaxPendingListSize);
				}
#endif // /#if SEOUL_LOGGING_ENABLED

				// If a main thread callback, queue it for dispatch.
				if (pRequest->m_bDispatchCallbackOnMainThread)
				{
					// Don't delete, we'll remove it here and
					// pass it to the main thread.
					bDeleteEntry = false;

					// Enque
					m_MainThreadFinishedBuffer.Push(pRequest);

					// Remove it from the list but don't delete.
					reqIter = m_lActiveRequests.Erase(reqIter);
				}
				// Call immediately.
				else
				{
					CallRequestCallback(pRequest);
				}
			}

			// If specified, delete the entry.
			if (bDeleteEntry)
			{
				// Cache lanes mask for update then
				// remove the entry from the list.
				Atomic32Type const lanes = (*reqIter)->GetLanesMask();
				reqIter = m_lActiveRequests.Erase(reqIter);

				// Remove the request's lanes contribution now that
				// it is fully complete.
				m_Lanes &= ~lanes;

				// If lanes was non-zero, it means it changed the state of m_Lanes,
				// so trigger the ticker thread again.
				if (0 != lanes)
				{
					s_TickWorkerSignal.Activate();
				}

				// Make sure the node is removed from any list it may
				// still be a part of *before* entering the destructor.
				if (nullptr != pRequest)
				{
					pRequest->m_Node.Remove();
				}

				// Destroy the entry - can't use SafeDelete
				// here because destructor is private/need
				// friend access to destruct it.
				SEOUL_DELETE pRequest;
				pRequest = nullptr;
			}
		}

		// Set whether we have any cancelling requests or not.
		m_bPendingCancelledRequests = bHasCancelledRequests;

		// If we're cancelling, check if we're done.
		if (m_bTickThreadDoCancelAllRequests)
		{
			Bool bDone = m_lActiveRequests.IsEmpty();

			// Another thread may have queued a pending request, if so,
			// mark as not done and loop.
			{
				Lock lock(m_PendingRequestsMutex);

				if (!m_lPendingRequests.IsEmpty())
				{
					bDone = false;
				}
			}

			if (bDone)
			{
				m_bTickThreadDoCancelAllRequests = false;
			}
		}
	}

	return 0;
}

Manager::DomainRequestBudget::DomainRequestBudget(Int initialBudget)
	: m_LastIncreaseUptime(g_pCoreVirtuals->GetUptime())
	, m_BudgetRemaining(initialBudget)
{
}

Manager::DomainRequestBudget::~DomainRequestBudget() = default;

void Manager::GetMaxRequestStats(String& rsURL, Stats& rStats) const
{
	m_MaxRequestStats.Get(rsURL, rStats);
}

void Manager::UpdateDomainRequestBudgets()
{
	Vector<String> vToErase;
	for (auto iElement : m_tDomainRequestBudgets)
	{
		auto uptime = g_pCoreVirtuals->GetUptime();
		auto timeSinceLastIncrease = uptime - iElement.Second.m_LastIncreaseUptime;
		if (timeSinceLastIncrease > m_DomainRequestBudgetIncreaseInterval)
		{
			Int64 iIncrease = timeSinceLastIncrease.GetMicroseconds() / m_DomainRequestBudgetIncreaseInterval.GetMicroseconds();
			Int64 iAmount = iElement.Second.m_BudgetRemaining + iIncrease;
			if (iAmount >= m_iDomainRequestBudgetInitial)
			{
				vToErase.PushBack(iElement.First);
			}
			else
			{
				iElement.Second.m_BudgetRemaining = (Int)iAmount;
				iElement.Second.m_LastIncreaseUptime = uptime;
			}
		}
	}

	for (auto const& iKey : vToErase)
	{
		m_tDomainRequestBudgets.Erase(iKey);
	}
}

void Manager::DecrementDomainRequestBudget(const Request& request)
{
	String sDomain = Manager::ParseURLDomain(request.GetURL());
	auto pBudget = m_tDomainRequestBudgets.Find(sDomain);
	if (nullptr == pBudget)
	{
		m_tDomainRequestBudgets.Insert(sDomain, DomainRequestBudget(m_iDomainRequestBudgetInitial - 1));
	}
	else
	{
		pBudget->m_BudgetRemaining = Max(0, pBudget->m_BudgetRemaining - 1);
	}
}

Bool Manager::IsDomainRequestBudgetZero(const Request& request) const
{
	String sDomain = Manager::ParseURLDomain(request.GetURL());
	auto pBudget = m_tDomainRequestBudgets.Find(sDomain);
	if (nullptr == pBudget)
	{
		return false;
	}
	else
	{
		return pBudget->m_BudgetRemaining <= 0;
	}
}

Bool Manager::QueueResendRequest(Request* pRequest)
{
	// Don't resend if we're shutting down, canceling, or if the request isn't supposed to resend
	if (m_bShuttingDown ||
		m_bTickThreadDoCancelAllRequests ||
		!pRequest->m_bResendOnFailure ||
		pRequest->IsCanceling())
	{
		return false;
	}

	auto pClonedRequest = CloneRequest(*pRequest);

	// TODO: Ugly - I don't want to do this in CloneRequest(), since
	// it's a "move" operation, not a "copy" operation and as such may not
	// be valid in all future use cases of CloneRequest().
	//
	// However, doing it here just increases the likelihood that it will be
	// missed as a necessary step if any code changes occur around request
	// retry. *Not* performing this buffer update will result in a resend
	// failing to write output to the appropriate output buffer.
	//
	// If pRequest is using a body buffer that it does not own, transfer
	// that buffer to the clone.
	if (!pRequest->m_Response.m_bOwnsBody)
	{
		// Cache the bits we need.
		auto pBuffer = pRequest->m_Response.m_pBody;
		auto uBufferSizeInBytes = pRequest->m_Response.m_zBodyCapacity;

		// Reset fields of pRequest.
		pRequest->m_Response.m_pBody = nullptr;
		pRequest->m_Response.m_zBodySize = 0u;
		pRequest->m_Response.m_zBodyCapacity = 0u;
		pRequest->m_Response.m_bOwnsBody = true;
		pRequest->m_Response.m_bBodyDataTruncated = false;

		// Now set the buffer to the clone.
		pClonedRequest->SetBodyOutputBuffer(pBuffer, uBufferSizeInBytes);
	}

	// Increases the request's resend delay if appropriate
	pClonedRequest->InitializeResendRequest(pRequest->m_eResult, SeoulTime::GetGameTimeInTicks());

	// If a prep for resend call was defined, invoke it now.
	if (pClonedRequest->m_PrepForResendCallback)
	{
		pClonedRequest->m_PrepForResendCallback(&pRequest->m_Response, pRequest, pClonedRequest);
	}

	// Begin - called InternalStart() so this request is treated
	// as a resend, not a new request.
	pClonedRequest->InternalStart(this);
	return true;
}

/** General binder for API specific worker thread body. */
Int Manager::InternalApiThread(const Thread& thread)
{
#if SEOUL_WITH_CURL
	return InternalCurlThread(thread);
#elif SEOUL_WITH_URLSESSION
	return InternalUrlSessionThread(thread);
#else
#	error "Define for this platform."
#endif
}

#if SEOUL_WITH_CURL
void Manager::InternalCurlFinishRequest(void* pMulti, Request* pRequest, Int32 iResult)
{
	SEOUL_ASSERT(IsHttpApiThread()); // Sanity check.
	SEOUL_ASSERT(nullptr != pRequest); // Sanity check.
	SEOUL_ASSERT(nullptr != pRequest->m_pCurl); // Sanity check.

	CURLM* pCurlMulti = (CURLM*)pMulti;
	CURLcode eResult = (CURLcode)iResult;

	// Cleanup curl data structures and store the results in the response.
	{
		// Set response data from curl
		long lStatus = 0;
		SEOUL_VERIFY_CURLE(curl_easy_getinfo(pRequest->m_pCurl, CURLINFO_RESPONSE_CODE, &lStatus));
		pRequest->m_Response.m_nStatus = (Int32)lStatus;

		// Set round trip time.
		SEOUL_VERIFY_CURLE(curl_easy_getinfo(pRequest->m_pCurl, CURLINFO_NAMELOOKUP_TIME, &pRequest->m_Stats.m_fLookupSecs));
		SEOUL_VERIFY_CURLE(curl_easy_getinfo(pRequest->m_pCurl, CURLINFO_CONNECT_TIME, &pRequest->m_Stats.m_fConnectSecs));
		SEOUL_VERIFY_CURLE(curl_easy_getinfo(pRequest->m_pCurl, CURLINFO_APPCONNECT_TIME, &pRequest->m_Stats.m_fAppConnectSecs));
		SEOUL_VERIFY_CURLE(curl_easy_getinfo(pRequest->m_pCurl, CURLINFO_PRETRANSFER_TIME, &pRequest->m_Stats.m_fPreTransferSecs));
		SEOUL_VERIFY_CURLE(curl_easy_getinfo(pRequest->m_pCurl, CURLINFO_REDIRECT_TIME, &pRequest->m_Stats.m_fRedirectSecs));
		SEOUL_VERIFY_CURLE(curl_easy_getinfo(pRequest->m_pCurl, CURLINFO_STARTTRANSFER_TIME, &pRequest->m_Stats.m_fStartTransferSecs));
		SEOUL_VERIFY_CURLE(curl_easy_getinfo(pRequest->m_pCurl, CURLINFO_TOTAL_TIME, &pRequest->m_Stats.m_fTotalRequestSecs));
		SEOUL_VERIFY_CURLE(curl_easy_getinfo(pRequest->m_pCurl, CURLINFO_SPEED_DOWNLOAD, &pRequest->m_Stats.m_fAverageDownloadSpeedBytesPerSec));
		SEOUL_VERIFY_CURLE(curl_easy_getinfo(pRequest->m_pCurl, CURLINFO_SPEED_UPLOAD, &pRequest->m_Stats.m_fAverageUploadSpeedBytesPerSec));

		long lRedirectCount = 0;
		char *sRedirectURL = nullptr;
		if (curl_easy_getinfo(pRequest->m_pCurl, CURLINFO_REDIRECT_COUNT, &lRedirectCount) == CURLE_OK &&
			lRedirectCount > 0 &&
			curl_easy_getinfo(pRequest->m_pCurl, CURLINFO_EFFECTIVE_URL, &sRedirectURL) == CURLE_OK &&
			sRedirectURL != nullptr)
		{
			pRequest->m_Response.m_sRedirectURL = sRedirectURL;
		}

		// Cleanup Curl data structures.
		// Clean up the curl handle
		SEOUL_VERIFY_CURLM(curl_multi_remove_handle(pCurlMulti, pRequest->m_pCurl));
		curl_easy_cleanup(pRequest->m_pCurl);
		pRequest->m_pCurl = nullptr;

		// Cleanup the header list.
		curl_slist_free_all(pRequest->m_pHeaderList);
		pRequest->m_pHeaderList = nullptr;
	}

	// Warning - pRequest can be destroyed by the main thread
	// or tick thread at the end of Finish(), do not access it
	// here after the call.
	if (eResult == CURLE_OK)
	{
		pRequest->Finish(Result::kSuccess);
	}
	else if (pRequest->IsCanceling())
	{
		pRequest->Finish(Result::kCanceled);
	}
	else if (eResult == CURLE_FAILED_INIT ||
		eResult == CURLE_COULDNT_RESOLVE_PROXY ||
		eResult == CURLE_COULDNT_RESOLVE_HOST ||
		eResult == CURLE_COULDNT_CONNECT ||
		eResult == CURLE_SSL_CONNECT_ERROR)
	{
		pRequest->Finish(Result::kConnectFailure);
	}
	else
	{
		SEOUL_LOG_HTTP("curl request failed: %s\n", curl_easy_strerror(eResult));
		pRequest->Finish(Result::kFailure);
	}
}

namespace
{

class CurlShare SEOUL_SEALED
{
public:
	CurlShare() = default;

	static void Lock(CURL* pHandle, curl_lock_data eData, curl_lock_access eAccess, void* pUser)
	{
		SEOUL_ASSERT(nullptr != pUser);
		reinterpret_cast<CurlShare*>(pUser)->InternalLock(pHandle, eData, eAccess);
	}

	static void Unlock(CURL* pHandle, curl_lock_data eData, void* pUser)
	{
		SEOUL_ASSERT(nullptr != pUser);
		reinterpret_cast<CurlShare*>(pUser)->InternalUnlock(pHandle, eData);
	}

private:
	SEOUL_DISABLE_COPY(CurlShare);

	FixedArray<Mutex, CURL_LOCK_DATA_LAST> m_aMutexes;

	void InternalLock(CURL* /*pHandle*/, curl_lock_data eData, curl_lock_access /*eAccess*/)
	{
		// Don't consider share - if we ever implement a RW vs. R mutex,
		// then CURL_LOCK_ACCESS_SHARED should be read access while
		// CURL_LOCK_ACCESS_SINGLE should be write access.
		m_aMutexes[eData].Lock();
	}

	void InternalUnlock(CURL* /*pHandle*/, curl_lock_data eData)
	{
		m_aMutexes[eData].Unlock();
	}
}; // class CurlShare

} // namespace anonymous

Int Manager::InternalCurlThread(const Thread&)
{
	// We are the API thread.
	SetHttpApiThreadId(Thread::GetThisThreadId());

	// Allocate these on the heap because they
	// can be very large on some platforms.
	ScopedPtr<fd_set> pFdExcep(SEOUL_NEW(MemoryBudgets::Network) fd_set);
	ScopedPtr<fd_set> pFdRead(SEOUL_NEW(MemoryBudgets::Network) fd_set);
	ScopedPtr<fd_set> pFdWrite(SEOUL_NEW(MemoryBudgets::Network) fd_set);

	CURLM *pCurlMulti = nullptr;
	String sUserAgent;
	Int nLastRunningHandles = 0;

	CurlShare share;
	CURLSH* pCurlShare = nullptr;

	// Startup
	{
		SEOUL_VERIFY_CURLE(curl_global_init_mem(
			CURL_GLOBAL_DEFAULT,
			&CurlMalloc,
			&CurlFree,
			&CurlRealloc,
			&CurlStrdup,
			&CurlCalloc));

		// Share handle for shared caching.
		pCurlShare = curl_share_init();
		SEOUL_ASSERT(pCurlShare != nullptr);
		SEOUL_VERIFY(CURLSHE_OK == curl_share_setopt(pCurlShare, CURLSHOPT_LOCKFUNC, CurlShare::Lock));
		SEOUL_VERIFY(CURLSHE_OK == curl_share_setopt(pCurlShare, CURLSHOPT_UNLOCKFUNC, CurlShare::Unlock));
		SEOUL_VERIFY(CURLSHE_OK == curl_share_setopt(pCurlShare, CURLSHOPT_USERDATA, &share));
		SEOUL_VERIFY(CURLSHE_OK == curl_share_setopt(pCurlShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT));
		SEOUL_VERIFY(CURLSHE_OK == curl_share_setopt(pCurlShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS));
		SEOUL_VERIFY(CURLSHE_OK == curl_share_setopt(pCurlShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION));

		// Multi for all operations.
		pCurlMulti = curl_multi_init();
		SEOUL_ASSERT(pCurlMulti != nullptr);

		// Enable multiplex.
		SEOUL_VERIFY_CURLM(curl_multi_setopt(pCurlMulti, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX));

		sUserAgent = curl_version();

		// Parse SSL certificate into OpenSSL's internal format
		ParseSSLCertificate();
	}

	// NOTE: Multiple if blocks below account for the (potentially large) time
	// each block can take, to recheck volatile values that are changed
	// by other threads.
	while (!m_bApiShuttingDown)
	{
		// Go to sleep on the API signal indefinitely if we're
		// in the background.
		if (m_bInBackground)
		{
			// Wait on the API signal.
			s_ApiSignal.Wait();
			continue; // Restart loop immediately, recheck background state and shutting down state.
		}

		// Start requests that were queued.
		if (!m_bInBackground && !m_bApiShuttingDown)
		{
			// Process the start queue.
			auto pRequest = m_ApiToStartBuffer.Pop();
			while (nullptr != pRequest)
			{
				// Record the start time for round-trip tracking.
				pRequest->m_iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();

				pRequest->CurlStart(pCurlShare, pCurlMulti, sUserAgent);
				pRequest->m_bApiHasStarted = true;
				pRequest = m_ApiToStartBuffer.Pop();

				// Stop early if we've entered the background.
				if (m_bInBackground)
				{
					break;
				}
			}
		}

		// Cancel requests that were queued.
		if (!m_bInBackground && !m_bApiShuttingDown)
		{
			// Wake up the tick thread.
			Bool bWakeUpTickThread = false;

			// Process the cancel queue.
			auto pRequest = m_ApiToCancelBuffer.Pop();
			while (nullptr != pRequest)
			{
				if (!pRequest->m_bCompleted)
				{
					InternalCurlFinishRequest(pCurlMulti, pRequest, CURLE_FAILED_INIT);
				}
				// Do not access pRequest after this line. This acts as a latch on another thread
				// to prevent the request from being destroyed.
				pRequest->m_bApiCancelRequestCompleted = true;
				pRequest = m_ApiToCancelBuffer.Pop();
				bWakeUpTickThread = true;

				// Stop early if we've entered the background.
				if (m_bInBackground)
				{
					break;
				}
			}

			if (bWakeUpTickThread)
			{
				(void)Manager::s_TickWorkerSignal.Activate();
			}
		}

		// Tick
		if (!m_bInBackground && !m_bApiShuttingDown)
		{
			// Before running multi, use our custom hook in curl to
			// apply connection cache idle timeouts.
			{
				CurlShare::Lock(nullptr, CURL_LOCK_DATA_CONNECT, CURL_LOCK_ACCESS_SINGLE, &share);
				Curl_demiurge_share_inside_lock_prune_idle_connections(pCurlShare, kiCurlIdleCachedConnectionTimeoutMilliseconds);
				CurlShare::Unlock(nullptr, CURL_LOCK_DATA_CONNECT, &share);
			}

			Int nRunningHandles = 0;
			SEOUL_VERIFY_CURLM(curl_multi_perform(pCurlMulti, &nRunningHandles));

			// Check for messages on 0 (curl doesn't mark a task as running until
			// perform is called once) or on running handle changes.
			//
			// We also need to hit curl_multi_info_read if any requests
			// have been tagged as cancelled, to make sure their progress
			// callback is processed and invoke.
			if (0 == nRunningHandles ||
				(nLastRunningHandles != nRunningHandles) ||
				m_bPendingCancelledRequests)
			{
				// See how the transfers went
				CURLMsg *pMessage = nullptr;
				int nMessagesLeft = 0;
				while ((pMessage = curl_multi_info_read(pCurlMulti, &nMessagesLeft)) != nullptr)
				{
					if (pMessage->msg == CURLMSG_DONE)
					{
						// Find out which request finished
						char *pRequestAsChar = nullptr;
						curl_easy_getinfo(pMessage->easy_handle, CURLINFO_PRIVATE, &pRequestAsChar);
						Request *pRequest = (Request *)pRequestAsChar;
						SEOUL_ASSERT(nullptr != pRequest);

						InternalCurlFinishRequest(pCurlMulti, pRequest, pMessage->data.result);
					}
				}
			}

			// Update running handles tracking.
			nLastRunningHandles = nRunningHandles;
		}

		// Wait on the Signal with a timeout, unless we're shutting down,
		// in the background, or there are pending actions in the start and/or
		// cancel buffers.
		if (!m_bInBackground &&
			!m_bApiShuttingDown &&
			m_ApiToCancelBuffer.IsEmpty() &&
			m_ApiToStartBuffer.IsEmpty())
		{
			// Get the timeout time from curl.
			long iTimeoutInMilliseconds = kiDefaultCurlSignalWaitInMilliseconds;
			SEOUL_VERIFY_CURLM(curl_multi_timeout(pCurlMulti, &iTimeoutInMilliseconds));

			// Negative value means no timeout, so just use a default, based on curl documentation.
			if (iTimeoutInMilliseconds < 0)
			{
				iTimeoutInMilliseconds = kiDefaultCurlSignalWaitInMilliseconds;
			}

			// 0 value means try again immediately - otherwise, we use the timeout value.
			if (0 != iTimeoutInMilliseconds)
			{
				// Sanitize the timeout.
				iTimeoutInMilliseconds = Clamp(
					iTimeoutInMilliseconds,
					kiMinCurlSignalWaitInMilliseconds,
					kiMaxCurlSignalWaitInMilliseconds);

				// Wait on sockets if possible.
				{
					memset(pFdRead.Get(), 0, sizeof(*pFdRead));
					memset(pFdWrite.Get(), 0, sizeof(*pFdWrite));
					memset(pFdExcep.Get(), 0, sizeof(*pFdExcep));
					Int iMaxFd = -1;

					SEOUL_VERIFY_CURLM(curl_multi_fdset(pCurlMulti, pFdRead.Get(), pFdWrite.Get(), pFdExcep.Get(), &iMaxFd));

					// -1 indicates no active sockets, so we wait on the curl signal instead.
					if (-1 == iMaxFd)
					{
						// Wait on the curl signal - wait indefinitely if curl returned no sockets
						// and has no active handles.
						if (0 == nLastRunningHandles)
						{
							s_ApiSignal.Wait();
						}
						else
						{
							// TODO: This case is annoying - basically, it is extremely
							// likely that curl will very quickly need attention again, because
							// this case indicates it has active handles but no sockets yet
							// (in the lookup phase?) Unfortunately there are no events from
							// curl to indicate this, so our choices are:
							// - use a longer wait time here and risk artificially delaying requests.
							// - use a shorter wait time here and risk starving other threads as the
							//   worst case limit is the curl thread entering a busy loop.
							//
							// Wait for the default interval unless explicitly signalled.
							s_ApiSignal.Wait((UInt32)Min(kiDefaultCurlSignalWaitInMilliseconds, iTimeoutInMilliseconds));
						}
					}
					// Otherwise, wait on the curl sockets.
					else
					{
						// Wait on sockets.
						timeval timevalTimeout;
						memset(&timevalTimeout, 0, sizeof(timevalTimeout));
						MillisecondsToTimeval(iTimeoutInMilliseconds, timevalTimeout);
						auto const iSelectResult = select(iMaxFd + 1, pFdRead.Get(), pFdWrite.Get(), pFdExcep.Get(), &timevalTimeout);
						(void)iSelectResult;

#if !SEOUL_ASSERTIONS_DISABLED
						// Report error.
						if (iSelectResult < 0)
						{
							auto const iError = Socket::GetLastSocketError();

							// Interruption is occasionally expected and can be safely ignored.
							static const Int32 kiInterrupt = 4; /* EINTR */
							if (kiInterrupt != iError)
							{
								Socket::LogError(__FUNCTION__, iError);
								SEOUL_FAIL(String::Printf("Unexpected return value from select: %d", iError).CStr());
							}
						}
#endif // /#if !SEOUL_ASSERTIONS_DISABLED
					}
				}
			}
		}
	}

	// Shutdown
	{
		if (m_pCertificateChain != nullptr)
		{
			sk_X509_INFO_pop_free(m_pCertificateChain, X509_INFO_free);
			m_pCertificateChain = nullptr;
		}

		// Release the multi handle.
		curl_multi_cleanup(pCurlMulti);
		pCurlMulti = nullptr;

		// Relase the share object.
		curl_share_cleanup(pCurlShare);
		pCurlShare = nullptr;

		curl_global_cleanup();

		// This is a Demiurge specific function added to OpenSSL,
		// to eliminate a shutdown "leak" that obfuscates our
		// memory leak tracking.
		SSL_COMP_free_compression_methods();

		// Stock OpenSSL shutdown/cleanup functions - order is based
		// on samples, but it's unclear if there is a correct/prefered
		// order.
		CONF_modules_free();
		EVP_cleanup();
		CRYPTO_cleanup_all_ex_data();
	}

	// We are no longer the API thread.
	SetHttpApiThreadId(ThreadId());

	return 0;
}
#endif // if SEOUL_WITH_CURL

#if SEOUL_UNIT_TESTS
void Manager::LogHttpState()
{
	SEOUL_LOG_HTTP("API thread vars:");
	SEOUL_LOG_HTTP("m_ApiToStartBuffer.IsEmpty:  %s", (m_ApiToStartBuffer.IsEmpty()) ? "true" : "false");
	SEOUL_LOG_HTTP("m_bApiShuttingDown:          %s", (m_bApiShuttingDown) ? "true" : "false");
	SEOUL_LOG_HTTP("m_bPendingCancelledRequests: %s", (m_bPendingCancelledRequests) ? "true" : "false");
	SEOUL_LOG_HTTP("m_ApiToCancelBuffer:         %u", (UInt32)m_ApiToCancelBuffer.GetCount());

	SEOUL_LOG_HTTP("HTTP thread vars:");
	SEOUL_LOG_HTTP("m_bShuttingDown:                      %s", (m_bShuttingDown) ? "true" : "false");
	SEOUL_LOG_HTTP("m_bTickWorkerShuttingDown:            %s", (m_bTickWorkerShuttingDown) ? "true" : "false");
	SEOUL_LOG_HTTP("m_bTickThreadDoCancelAllRequests:     %s", (m_bTickThreadDoCancelAllRequests) ? "true" : "false");
	SEOUL_LOG_HTTP("now in ticks:                         %" PRId64, SeoulTime::GetGameTimeInTicks());
	SEOUL_LOG_HTTP("m_NetworkFailureActiveResendRequests: %d", (Int32)m_NetworkFailureActiveResendRequests);
	SEOUL_LOG_HTTP("m_nPendingListSize:                   %d", m_nPendingListSize);
	SEOUL_LOG_HTTP("m_nMaxPendingListSize:                %d", m_nMaxPendingListSize);
	SEOUL_LOG_HTTP("m_lActiveRequests:                    %u", m_lActiveRequests.GetSize());
	SEOUL_LOG_HTTP("m_lPendingRequests:                   %u", m_lPendingRequests.GetSize());
	SEOUL_LOG_HTTP("m_Lanes:                              %u", (UInt32)m_Lanes);
	SEOUL_LOG_HTTP("m_MainThreadFinishedBuffer:           %u", (UInt32)m_MainThreadFinishedBuffer.GetCount());

	{
		TryLock test(m_PendingRequestsMutex);
		SEOUL_LOG_HTTP("m_PendingRequestsMutex available:    %s", (test.IsLocked()) ? "true" : "false");
	}

	SEOUL_LOG_HTTP("uptime:                              %" PRId64, g_pCoreVirtuals->GetUptime().GetMicroseconds());
	SEOUL_LOG_HTTP("m_tDomainRequestBudgets:             %u", m_tDomainRequestBudgets.GetSize());
	for (auto const& iter : m_tDomainRequestBudgets)
	{
		SEOUL_LOG_HTTP("\t%s: %d at %" PRId64, iter.First.CStr(), iter.Second.m_BudgetRemaining, iter.Second.m_LastIncreaseUptime.GetMicroseconds());
	}
}
#endif // if SEOUL_UNIT_TESTS

void Manager::MaxRequestStats::Get(String& rsURL, Stats& rStats) const
{
	Lock lock(m_Mutex);
	rsURL = m_sURL;
	rStats = m_Stats;
}

void Manager::MaxRequestStats::Merge(const String& sURL, const Stats& stats)
{
	Lock lock(m_Mutex);
	if (stats.m_fOverallSecs > m_Stats.m_fOverallSecs)
	{
		m_Stats = stats;
		m_sURL = sURL;
	}
}

} // namespace Seoul::HTTP
