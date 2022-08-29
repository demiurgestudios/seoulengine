/**
 * \file HTTPRequest.h
 * \brief Wraps a single HTTP client request managed via HTTPManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "Delegate.h"
#include "HashTable.h"
#include "HTTPCommon.h"
#include "HTTPExternalDeclare.h"
#include "HTTPRequestList.h"
#include "HTTPResendTimer.h"
#include "HTTPResponse.h"
#include "HTTPStats.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
#include "StreamBuffer.h"
namespace Seoul { namespace HTTP { class RequestCancellationToken; } }
namespace Seoul { namespace HTTP { class ResendTimer; } }
namespace Seoul { class SyncFile; }

namespace Seoul::HTTP
{

class Request SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(Request);

	/** @return Stats about operation over the lifespan of this request (including resends). */
	const Stats& GetStats() const { return m_Stats; }

	/** Update whether the completion callback should be dispatched on the main thread or not. */
	void SetDispatchCallbackOnMainThread(Bool bDispatchCallbackOnMainThread)
	{
		m_bDispatchCallbackOnMainThread = bDispatchCallbackOnMainThread;
	}

	/** Mark this request as ignoring our HTTP system's request throttling; useful for the patcher/downloader */
	void SetIgnoreDomainRequestBudget(Bool bIgnoreDomainRequestBudget)
	{
		m_bIgnoreDomainRequestBudget = bIgnoreDomainRequestBudget;
	}

	/**
	 * Lanes used by this request. 0 schedules this request immediately, otherwise,
	 * it must wait until all its lanes are clear.
	 */
	Atomic32Type GetLanesMask() const { return m_LanesMask; }
	void SetLanesMask(Atomic32Type mask) { m_LanesMask = mask; }

	/** Gets the HTTP method to be used */
	HString GetMethod() const { return m_sMethod; }
	// Sets the HTTP method to be used
	void SetMethod(HString sMethod);
	/** Gets the URL to request */
	const String& GetURL() const { return m_sURL; }
	// Sets the URL to request (it must be properly URL-encoded)
	void SetURL(const String& sURL);
	// Returns true if the request has started (mostly useful for unit tests)
	Bool HasStarted() const { return m_iStartTimeInTicks != 0; }
	// Sets the callback for when the request completes
	void SetCallback(const ResponseDelegate& callback);
	// Sets the callback for reporting request progress
	void SetProgressCallback(const ResponseProgressDelegate& callback);
	// Override the buffer used when accepting the request. By default,
	// a buffer will be dynamically managed to store the request. If a buffer
	// is set via this callback, it will not be resized - it must be large
	// enough to store the maximum size of the request, or the request will
	// be truncated to zBufferSizeInBytes.
	//
	// WARNING: pBuffer must remain allocated until this request completes.
	void SetBodyOutputBuffer(void* pBuffer, UInt32 zBufferSizeInBytes);
	// Sets the absolute filename to which response body data will be written to - if
	// this is set, the body data returned by HTTP::Response::GetBodyData() will
	// be nullptr and the data will be stored in this file.
	void SetBodyDataOutputFile(const String& sAbsoluteFilename, Bool bResume = false);
	// Sets the callback that will be invoked before opening the body data output file
	// after receiving the initial chunk of data. Allows the caller to reject data
	// before the file is open for writing.
	void SetOpenFileValidateCallback(const OpenFileValidateDelegate& callback);
	void SetPrepForResendCallback(const PrepForResendCallback& callback);
	// Sets the connection timeout for the HTTP request.
	void SetConnectionTimeout(Int iTimeoutInSeconds);
	// Sets the transfer timeout for the HTTP request.
	void SetTransferTimeout(Int iTimeoutInSeconds);
	// Return true if this is a resend request, false otherwise.
	Bool IsResendRequest() const { return m_bIsResendRequest; }
	// Adds a string Key=Value pair of POST data (the value will be
	// URL-encoded)
	void AddPostData(const String& sKey, const String& sValue);
	// Alternative to post data pairs, acquire the post body directly
	// and populate it custom.
	StreamBuffer& AcquirePostBody()
	{
		// No post data if manually specified.
		m_tPostData.Clear();
		return m_PostBody;
	}

	// Adds the given "Header: Value" header to the request.  The value MUST
	// be properly encoded for the given header name -- if it might contain
	// any unsafe characters, it should probably be URL-encoded.
	void AddHeader(const String& sKey, const String& sValue);

	/**
	 * Convenience function, adds the partial content "Range" header, for partial file downloads.
	 *
	 * @param[in] uStartOffsetInBytes 0 based offset to the start of the range to request.
	 * @param[in] uEndOffsetInBytes 0 based offset to the last byte of the range to request.
	 *
	 * \example To download the first 500 bytes of a file, uStartOffsetInBytes = 0 and uEndOffsetInBytes = 499
	 */
	void AddRangeHeader(UInt64 uStartOffsetInBytes, UInt64 uEndOffsetInBytes)
	{
		AddHeader(
			"Range",
			String::Printf("bytes=%" PRIu64 "-%" PRIu64, uStartOffsetInBytes, uEndOffsetInBytes));
	}

	// Use to delete an existing header from this HTTPRequest.
	void DeleteHeader(const String& sKey);

	/**
	 * Delete a previously specified "Range" header from this request.
	 */
	void DeleteRangeHeader()
	{
		DeleteHeader("Range");
	}

	// Enqueues the request for start.
	SharedPtr<RequestCancellationToken> Start();

	/**
	 * Update whether this HTTP::Request should be resent
	 * on failure. Defaults to true.
	 */
	void SetResendOnFailure(Bool bResendOnFailure)
	{
		m_bResendOnFailure = bResendOnFailure;
	}

	/**
	 * @return True if this HTTP::Request should verify the certificate chain
	 * of a secure peer before allowing a connection (HTTPS), false otherwise.
	 */
	Bool GetVerifyPeer() const
	{
		return m_bVerifyPeer;
	}

	/**
	 * Set whether this HTTP::Request should verify the certificate chain
	 * of a secure peer before allowing a connection (HTTPS). Only applicable
	 * to platforms where we maintain our own certificate bundle. Defaults to true.
	 */
	void SetVerifyPeer(Bool bVerifyPeer)
	{
		m_bVerifyPeer = bVerifyPeer;
	}

#if SEOUL_WITH_CURL
	// SSL context function.  We return an int instead of CURLcode to avoid
	// pulling the curl headers into this header.  Alas, C++ does not support
	// partial classes (in which case we could just move this declaration into
	// the source file).
	int CurlSSLContextFunction(void *pContext);
#endif // /#if SEOUL_WITH_CURL

	Request* Clone() const;

	/** @return True if interaction with the body data output file has thus far been successful, false otherwise. */
	Bool IsBodyDataOutputFileOk() const
	{
		return m_bBodyDataOutputFileOk;
	}

	/** @return True if the open file validation callback passed when attempting to open the body data output file, false otherwise. */
	Bool DidBodyDataOpenFileValidateCallbackPass() const
	{
		return m_bBodyDataOpenFileValidateCallbackPassed;
	}

	/** @return True if the request is in the process of being cancelled, false otherwise. */
	Bool IsCanceling() const;

#if SEOUL_UNIT_TESTS
	void UnitTestOnly_InitializeResendRequest(Result eStatus, Int64 iNowTicks)
	{
		InitializeResendRequest(eStatus, iNowTicks);
	}

	Int64 UnitTestOnly_GetEarliestSendTimeInTicks()
	{
		return m_iEarliestSendTimeInTicks;
	}

	void UnitTestOnly_SetEarliestSendTimeInTicks(Int64 iTicks)
	{
		m_iEarliestSendTimeInTicks = iTicks;
	}
#endif

private:
	friend class Manager;
	friend class RequestHelper;
	friend class RequestList;

	// Only HTTP::Manager can construct HTTP::Request instances
	Request(const SharedPtr<RequestCancellationToken>& pToken, const ResendTimer& resendTimer);
	~Request();

	// Mark request as resend and (if appropriate for the status) increase resend timer
	void InitializeResendRequest(Result eStatus, Int64 iNowTicks);

	// Called by Start() to complete start processing.
	void InternalStart(CheckedPtr<Manager> pHTTPManager);

	// Actual implementation of Start() for the request. Expected
	// to be called from the ticker thread.
	void TickerThreadPerformStart();

	// Marks the request as completed with the given result
	void Finish(Result eResult);

	// Called to setup m_PostBody - either left unmodified, if already
	// non-zero, or populated with the contents of m_tPostData.
	void FinalizePostData();

	// Helper function called when we receive header data as part of the response
	void OnHeaderReceived(const void *pData, size_t zSize);

	// Helper function called when we receive data as part of the response
	void OnDataReceived(const void *pData, size_t zSize);

#if SEOUL_WITH_CURL
	// Invoked by the CURL thread to actually start the request.
	void CurlStart(CURLSH* pCurlShare, CURLM* pCurlMulti, const String& sUserAgent);

	// Header callback from curl
	static size_t StaticCurlHeaderCallback(char *ptr, size_t size, size_t nmemb, void *pUserData);
	// Data write callback from curl
	static size_t StaticCurlWriteCallback(char *ptr, size_t size, size_t nmemb, void *pUserData);
	// Progress callback from curl
	static int StaticCurlProgressCallback(void *pUserData, double fDownloadTotal, double fDownloadSoFar, double fUploadTotal, double fUploadSoFar);
#elif SEOUL_WITH_URLSESSION
	// Invoke by the API thread to actually start a request.
	SharedPtr<RequestHelper> UrlSessionStart(NSOperationQueue* pQueue);
#endif

private:
	///////////////////////////////////////////////////////////////////////////
	// Start of members that need to be cloned.
	///////////////////////////////////////////////////////////////////////////

	/** URL to request */
	String m_sURL;
	/** True if a failed request should be resent, false otherwise. */
	Bool m_bResendOnFailure;
	/** True if secure connections require certificate verification, false otherwise. */
	Bool m_bVerifyPeer;
	/** True if completion callback is dispatched on the main thread (the default) or immediately from the network thread. */
	Bool m_bDispatchCallbackOnMainThread;
	/** True if we should exempt this request from domain request budgets. If exempt, we don't hold this request for or count it against the domain's HTTP budget. */
	Bool m_bIgnoreDomainRequestBudget;

	/** Tracks the next retry delay. Optionally increased when a retry request is cloned. */
	ResendTimer m_ResendTimer;
	/** The current retry delay. */
	Double m_fResendDelaySeconds;

	/** Body data output file settings */
	String m_sBodyDataOutputFilename;
	Bool m_bBodyDataOutputFileOpenForResume;

	/** Optioanl callback to validate the initial data received before opening the body data output file. */
	OpenFileValidateDelegate m_OpenFileValidateCallback;

	/** Optional callback invoked right before a resend request is sent, to give the client a chance to modify settings on a resend. */
	PrepForResendCallback m_PrepForResendCallback;

	/** HTTP method */
	HString m_sMethod;

	/**
	 * Timeout for the request to connect, in seconds. Not supported on all platforms.
	 * When not supported, the transfer timeout is the only timeout to apply:
	 *
	 * Applies on:
	 * - Android, PC
	 */
	Int m_iConnectionTimeoutInSeconds;

	/**
	 * Timeout for the request to finish transfer operations. On some platforms (iOS),
	 * this is the timeout for the entire operation, including connection.
	 */
	Int m_iTransferTimeoutInSeconds;

	/** Lanes mask of the request. */
	Atomic32Type m_LanesMask;

	/** Entries for HTTP::RequestList membership handling. */
	RequestListNode m_Node;

	/** Callback to call when the request completes */
	ResponseDelegate m_Callback;
	/** Callback to call with progress updates */
	ResponseProgressDelegate m_ProgressCallback;

	/** Table of POST data */
	typedef HashTable<String, String, MemoryBudgets::Network> PostDataTable;
	PostDataTable m_tPostData;

	/**
	 * Alternative post body, directly specified. If not, used to store
	 * collapsed post data.
	 */
	StreamBuffer m_PostBody;

	/** Table of headers */
	typedef HashTable<String, String, MemoryBudgets::Network> HeaderDataTable;
	HeaderDataTable m_tHeaderData;

	/** Flag indicating if we should cancel the request */
	SharedPtr<RequestCancellationToken> m_pCancellationToken;

	/**
	 * Tracked stats throughout the lifespan of the request. This
	 * is the time that HTTP::Request() is invoked. Other timers
	 * form tighter scoping around the underlying HTTP layer, whereas
	 * this timing will capture (e.g.) dispatch and queuing overhead
	 * at the SeoulEngine level.
	 */
	Int64 m_iRequestConstructTimeInTicks;

	/** Marks call to HTTP::Request::Start(). Used to measure request delay time. */
	Int64 m_iRequestStartTimeInTicks;
	Stats m_Stats;

	///////////////////////////////////////////////////////////////////////////
	// End of members that need to be cloned.
	///////////////////////////////////////////////////////////////////////////

	/** Don't begin sending this request until this time (enforces retry delays) */
	Int64 m_iEarliestSendTimeInTicks;

	/** When writing the body directly to a file, this defines current write state. */
	ScopedPtr<SyncFile> m_pBodyDataOutputFile;
	Bool m_bBodyDataOutputFileOk;
	Bool m_bBodyDataOpenFileValidateCallbackPassed;
	Bool m_bBodyDataOutputFileOpened;

	/** Variables used to track round trip time of the request. */
	Int64 m_iStartTimeInTicks;

	/** Variables used to track download progress and handle reporting it to a callback, if defined. */
	Atomic32 m_TotalDownloadSizeInBytes;
	Atomic32 m_DownloadSizeSoFarInBytes;
	Atomic32 m_LastReportedDownloadSizeSoFarInBytes;

	/** Have we completed yet? */
	Atomic32Value<Bool> m_bCompleted;

	/** True if this is a resend request, false otherwise. */
	Bool m_bIsResendRequest;
	/** True if this resend request is due to a network or connection failure. */
	Bool m_bIsNetworkFailureResendRequest;
	/** Result of the request, if completed */
	Atomic32Value<Result> m_eResult;

	/** Response object for this request */
	Response m_Response;

	/** Tracking of start state. */
	Atomic32Value<Bool> m_bApiHasStarted;

	/** Tracking of cancelation state. */
	Atomic32Value<Bool> m_bApiCancelRequested;
	Atomic32Value<Bool> m_bApiCancelRequestCompleted;

#if SEOUL_WITH_CURL
	/** Curl easy handle for this request */
	CURL* m_pCurl;

	/** Linked list of HTTP headers to be sent */
	curl_slist* m_pHeaderList;

#elif SEOUL_WITH_URLSESSION
	/** URL session object */
	NSURLSession* m_pSession;
	NSURLSessionDataTask* m_pTask;
	SeoulURLSessionDelegate* m_pDelegate;
#endif

	SEOUL_DISABLE_COPY(Request);
}; // class HTTP::Request

} // namespace Seoul::HTTP

#endif // include guard
