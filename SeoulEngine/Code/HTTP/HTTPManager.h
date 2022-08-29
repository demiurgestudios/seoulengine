/**
 * \file HTTPManager.h
 * \brief Singleton class for platform-independent HTTP requests.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	HTTP_MANAGER_H
#define HTTP_MANAGER_H

#include "AtomicRingBuffer.h"
#include "Delegate.h"
#include "HashTable.h"
#include "HTTPCommon.h"
#include "HTTPExternalDeclare.h"
#include "HTTPHeaderTable.h"
#include "HTTPResendTimer.h"
#include "HTTPStats.h"
#include "List.h"
#include "Mutex.h"
#include "PerThreadStorage.h"
#include "ScopedPtr.h"
#include "SeoulSignal.h"
#include "SeoulString.h"
#include "SeoulTime.h"
#include "Singleton.h"
#include "StreamBuffer.h"
#include "Vector.h"
namespace Seoul { namespace HTTP { class Request; } }
namespace Seoul { namespace HTTP { class RequestCancellationToken; } }
namespace Seoul { namespace HTTP { class RequestHelper; } }
namespace Seoul { namespace HTTP { class RequestList; } }
namespace Seoul { namespace HTTP { class RequestListNode; } }
namespace Seoul { namespace HTTP { class Response; } }
namespace Seoul { class Thread; }

namespace Seoul::HTTP
{

struct ManagerSettings SEOUL_SEALED
{
	/** Collection of SSL certificates to use with the server. Not required on all platforms. */
	String m_sSSLCertificates;
	/** Sub-platform name for request header. (e.g. Amazon) */
	String m_sSubPlatform;
}; // struct HTTP::ManagerSettings

class Manager SEOUL_SEALED : public Singleton<Manager>
{
public:
	/** Parses the domain part of an URL */
	static String ParseURLDomain(String const&);

	SEOUL_DELEGATE_TARGET(Manager);

	Manager(const ManagerSettings& settings);
	~Manager();

	// Called by the application to indicate that the app has entered the background. Use
	// to suspend the HTTP worker thread. OnEnterBackground() must be followed by OnLeaveBackground()
	// at the appropriate time.
	void OnEnterBackground();
	void OnLeaveBackground();

	// Creates a new HTTP request.  The caller is responsible for setting up
	// all of the applicable arguments and then calling Start() on the request
	// to actually start it. Include an (optional) HTTP::RequestList to allow
	// for client tracking of requests.
	//
	// TODO: Fix - this is a bad API design. Instead, CreateRequest
	// should take a full settings structure that completely defines the
	// request and start it implicitly, returning only a cancellation token
	// instance.
	//
	// IMPORTANT: HTTP::Request is returned by reference because it *must* not
	// be stored. In fact, no access of HTTP::Request can be safely made
	// after the call to start.
	Request& CreateRequest(RequestList* pClientList = nullptr);

	// TODO: Fix - this is a bad API design.
	//
	// Rare use - only valid if a request can be started but then never
	// sent via Start().
	void DestroyUnusedRequest(Request*& rp) const;

	// Runs any pending callbacks for completed requests. If
	// bTickForCompletion is true, requests that are marked
	// as ResendOnFailure will not be resent and instead will
	// be dispatched via their callbacks with a kResultFailure
	// code.
	void Tick();

	// Return true if there are outstanding requests, false otherwise.
	Bool HasRequests() const;

	ResendTimer CopyHttpResendTimer() const
	{
		return m_ResendTimer;
	}

	/** Update the values used for resend rate limiting */
	void SetResendSettings(Double fMinInterval, Double fMaxInterval, Double fBaseMultiplier, Double fRandomMultiplier)
	{
		m_ResendTimer.UpdateSettings(fMinInterval, fMaxInterval, fBaseMultiplier, fRandomMultiplier);
	}

	void SetDomainRequestBudgetSettings(Int iInitialBudget, Int iSecondsPerIncrease);

	/**
	 * Return the number of currently active resend requests due
	 * to network or connection failures (not completed requests
	 * that return HTTP error status codes.
	 */
	Atomic32Type GetNetworkFailureActiveResendRequests() const
	{
		return m_NetworkFailureActiveResendRequests;
	}

	// URL-encodes the given string according to RFC 1738 section 2.2
	// To decode an URL encoded string, see Seoul::URLDecode() in StringUtil.h.
	static String URLEncode(const String& sStr);

	// Turns on some debug logging of curl HTTP/2 errors
	void EnableVerboseHttp2Logging(Bool bEnabled);

	// Global across all requests, retrieve stats of the request
	// that has (so far) been the longest overall request time.
	void GetMaxRequestStats(String& rsURL, Stats& rStats) const;

	// Public for unit tests
	/** Updates request budgets; called at every iteration of InternalTickWorkerThread, immediately before pulling pending requests */
	void UpdateDomainRequestBudgets();
	/** Spend one request from the request domain's budget */
	void DecrementDomainRequestBudget(const Request&);
	/** Checks if the request budget is at zero; if true, no new requests will be started (unless the request is exempt from the budget) */
	Bool IsDomainRequestBudgetZero(const Request&) const;

#if SEOUL_UNIT_TESTS
	/** Prints internal state for test debugging */
	void LogHttpState();
#endif

private:
	friend class Request;
	friend class RequestCancellationToken;
	friend class RequestHelper;

	// Cancels all requests which are currently in progress.  Callbacks will
	// be called immediately.
	void CancelAllRequestsForDestruction();

	// Call a request callback (if it has one)
	void CallRequestCallback(Request* pRequest);

	// Add a resend of pRequest to the pending request list
	Bool QueueResendRequest(Request* pRequest);

	// Clones an HTTP request, but generates and applies a new request ID header
	Request *CloneRequest(const Request& request);

	// Set a unique, random request ID header to help us correlate end-to-end log lines
	void AddSeoulEngineHeaders(Request* pRequest, Bool bResend);

	// Adds the Platform and SubPlatform headers.
	void AddSeoulEnginePlatformHeader(Request* pRequest);

#if SEOUL_WITH_CURL
	// Parses our root SSL certificate into OpenSSL's internal format
	void ParseSSLCertificate();
#endif // /#if SEOUL_WITH_CURL

	// Starts an HTTP request on the API thread.
	void StartHTTPRequest(Request* pRequest);

private:
	Atomic32 m_RequestsInProgressCount;

	class MaxRequestStats SEOUL_SEALED
	{
	public:
		MaxRequestStats() = default;

		void Get(String& rsURL, Stats& rStats) const;
		void Merge(const String& sURL, const Stats& stats);

	private:
		SEOUL_DISABLE_COPY(MaxRequestStats);

		Stats m_Stats;
		String m_sURL;
		Mutex m_Mutex;
	}; // struct MaxRequestStats
	MaxRequestStats m_MaxRequestStats;

	/** State of the various HTTP lanes. */
	Atomic32 m_Lanes;

	/** Control variables for resend behavior. */
	ResendTimer m_ResendTimer;

	Atomic32 m_NetworkFailureActiveResendRequests;
	Int32 m_nMaxPendingListSize;
	Int32 m_nPendingListSize;

	typedef List<Request*, MemoryBudgets::Network> RequestsList;
	/** List of requests which are currently in progress */
	RequestsList m_lActiveRequests;

	/**
	 * New requests created in the last tick.  We keep two separate lists to
	 * avoid annoying multithreading problems and so that request callbacks can
	 * add new requests without screwing up our iteration etc.
	 */
	RequestsList m_lPendingRequests;
	/** Mutex to protect m_lPendingRequests */
	Mutex m_PendingRequestsMutex;
	/** Flag indicating if we're shutting down */
	Bool m_bShuttingDown;

	/** Settings used to create HTTP. */
	ManagerSettings const m_Settings;

	/** tick worker thread body. */
	Int InternalTickWorkerThread(const Thread&);

	/**
	 * Signal to use to trigger interactions with the tick worker thread.
	 *
	 * Static to simplify interactions between multiple thread and a
	 * race condition during shutdown where a Finish()ed request can
	 * attempt to signal the tick worker after the HTTP system has
	 * already been destroyed.
	 */
	static Signal s_TickWorkerSignal;
	/** Boolean used to tell the tick worker thread that the HTTP system is shutting down. */
	Atomic32Value<Bool> m_bTickWorkerShuttingDown;
	/** tick worker thread. */
	ScopedPtr<Thread> m_pTickWorkerThread;
	/** flag indicating that the tick thread should be cancelling requests. */
	Atomic32Value<Bool> m_bTickThreadDoCancelAllRequests;
	/** flag set by the tick thread to indicate that some requests are pending a cancellation. */
	Atomic32Value<Bool> m_bPendingCancelledRequests;

	struct DomainRequestBudget SEOUL_SEALED
	{
		DomainRequestBudget(Int initialBudget);
		~DomainRequestBudget();

		TimeInterval m_LastIncreaseUptime;
		Int m_BudgetRemaining;
	};
	Int m_iDomainRequestBudgetInitial;
	TimeInterval m_DomainRequestBudgetIncreaseInterval;
	HashTable<String, DomainRequestBudget> m_tDomainRequestBudgets;

	/** Thread-safe buffers used to pass requests between threads */
	typedef AtomicRingBuffer<Request*, MemoryBudgets::Network> RequestBuffer;

	/** Pass completed tasks from the tick worker thread to the main thread. */
	RequestBuffer m_MainThreadFinishedBuffer;
	RequestBuffer m_MainThreadNeedsResendCallbackBuffer;

	/** API worker thread. */
	ScopedPtr<Thread> m_pApiWorkerThread;

	/**
	 * Signal used to tell the API worker thread to wake up.
	 *
	 * Static to simplify interactions between multiple thread and a
	 * race condition during shutdown where a Finish()ed request can
	 * attempt to signal the tick worker after the HTTP system has
	 * already been destroyed.
	 */
	static Signal s_ApiSignal;

	/** Buffers used to pass requests to the API thread. */
	RequestBuffer m_ApiToStartBuffer;
	RequestBuffer m_ApiToCancelBuffer;

	/** Flag indicating if we're shutting down */
	Atomic32Value<Bool> m_bApiShuttingDown;

	/** General binder for API specific worker thread body. */
	Int InternalApiThread(const Thread& thread);

#if SEOUL_WITH_CURL
	/** utility function. */
	static void InternalCurlFinishRequest(void* pMulti, Request* pRequest, Int32 iResult);

	/** Parsed SSL certificates */
	stack_st_X509_INFO *m_pCertificateChain;

	/** curl worker thread body. */
	Int InternalCurlThread(const Thread&);

	Bool m_bVerboseHttp2LogsEnabled;

#elif SEOUL_WITH_URLSESSION
	/** urlsession worker thread body. */
	Int InternalUrlSessionThread(const Thread&);
#endif

	Int64 m_iLastBackgroundGameTimeInTicks;
	Atomic32Value<Bool> m_bInBackground;

	SEOUL_DISABLE_COPY(Manager);
}; // class HTTP::Manager

} // namespace Seoul::HTTP

#endif // include guard
