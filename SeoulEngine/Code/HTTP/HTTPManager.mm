/**
 * \file HTTPManager.mm
 * \brief Implementation for HTTP requests using Cocoa in Objective-C++ for iOS
 * (and eventually Mac OS X, maybe)
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "IOSUtil.h"
#include "Logger.h"
#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "HTTPRequestList.h"
#include "HTTPResendTimer.h"
#include "HTTPResponse.h"
#include "Thread.h"
#include "ThreadId.h"

#import <Foundation/Foundation.h>

using namespace Seoul;

@interface SeoulURLSessionDelegate: NSObject<NSURLSessionDataDelegate, NSURLSessionTaskDelegate, NSURLSessionDelegate>
/** Pointer to the Seoul HTTPRequestHelper for this delegate */
@property(assign) HTTPRequestHelper* m_pRequestHelper;

// Initializes this delegate with the given request
- (id)initWithRequest:(HTTPRequest *)pRequest;

// NSURLSessionDelegate methods

// Session has been invalidated; may be called after request completes
- (void)URLSession:(NSURLSession *)session
		didBecomeInvalidWithError:(NSError *)error;

// Lets the session respond to an auth challenge; passes through to default behavior
- (void)URLSession:(NSURLSession *)session
		didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
		completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition, NSURLCredential *credential))completionHandler;

// Background requests completed while we were offline; handle them
// TODO: Not implemented; we don't use background downloading.
- (void)URLSessionDidFinishEventsForBackgroundURLSession:(NSURLSession *)session;

// end NSURLSessionDelegate methods

// NSURLSessionTaskDelegate methods

// Called on request complete
- (void)URLSession:(NSURLSession *)session
		task:(NSURLSessionTask *)task
		didCompleteWithError:(NSError *)error;


// Called when an HTTP redirect is about to happen
- (void)URLSession:(NSURLSession *)session
		task:(NSURLSessionTask *)task
		willPerformHTTPRedirection:(NSHTTPURLResponse *)response
		newRequest:(NSURLRequest *)request
		completionHandler:(void (^)(NSURLRequest *))completionHandler;

// Lets the session respond to an auth challenge; passes through to default behavior
- (void)URLSession:(NSURLSession *)session
		task:(NSURLSessionTask *)task
		didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
		completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition, NSURLCredential *credential))completionHandler;

// Lets the session know data is uploading (noop)
- (void)URLSession:(NSURLSession *)session
		task:(NSURLSessionTask *)task
		didSendBodyData:(int64_t)bytesSent
		totalBytesSent:(int64_t)totalBytesSent
		totalBytesExpectedToSend:(int64_t)totalBytesExpectedToSend;

// (noop)
- (void)URLSession:(NSURLSession *)session
		task:(NSURLSessionTask *)task
		didFinishCollectingMetrics:(NSURLSessionTaskMetrics *)metrics;

// end NSURLSessionTaskDelegate methods

// NSURLSessionDataDelegate methods

// Received initial HTTP headers
- (void)URLSession:(NSURLSession *)session
		dataTask:(NSURLSessionDataTask *)dataTask
		didReceiveResponse:(NSURLResponse *)response
		completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))completionHandler;

// Received body data
- (void)URLSession:(NSURLSession *)session
		dataTask:(NSURLSessionDataTask *)dataTask
		didReceiveData:(NSData *)data;

// Shouldn't be called; we don't request download tasks
- (void)URLSession:(NSURLSession *)session
		dataTask:(NSURLSessionDataTask *)dataTask
		didBecomeDownloadTask:(NSURLSessionDownloadTask *)downloadTask;

// Shouldn't be called; we don't request stream tasks
- (void)URLSession:(NSURLSession *)session
		dataTask:(NSURLSessionDataTask *)dataTask
		didBecomeStreamTask:(NSURLSessionStreamTask *)streamTask;

// end NSURLSessionDataDelegate methods
@end

namespace Seoul
{

void SetHttpApiThreadId(const ThreadId& id);
Bool IsHttpApiThread();

/**
 * Helper class for accessing private methods of HTTPRequest from Objective-C
 */
class HTTPRequestHelper SEOUL_SEALED
{
public:
	HTTPRequestHelper(HTTPRequest* pRequest)
		: m_Mutex()
		, m_pRequest(pRequest)
		, m_pResponse(nil)
		, m_pError(nil)
		, m_bCompleted(false)
	{
	}

	~HTTPRequestHelper()
	{
		Lock lock(m_Mutex);
		if (nil != m_pError)
		{
			[m_pError release];
			m_pError = nil;
		}
		if (nil != m_pResponse)
		{
			[m_pResponse release];
			m_pResponse = nil;
		}
	}

	void Finish(Bool bCanceled)
	{
		Lock lock(m_Mutex);
		if (!m_pRequest) { return; }

		// Cancel if requested.
		if (bCanceled)
		{
			// If we don't have a task, there's nothing to cancel
			if (nil != m_pRequest->m_pTask)
			{
				// Use a call to cancel on the task here instead of session
				// invalidateAndCancel based on:
				// https://stackoverflow.com/questions/20019692/nsurlsession-invalidateandcancel-bad-access
				//
				// After seeing some periodic crashes during
				// our DeviceFarm testing.
				[m_pRequest->m_pTask cancel];
			}
		}

		// Propagate headers and status code.
		if (nil != m_pResponse)
		{
			NSHTTPURLResponse* pResponse = (NSHTTPURLResponse*)m_pResponse;
			m_pRequest->m_Response.m_nStatus = (Int)[pResponse statusCode];
			NSDictionary* pHeaders = [pResponse allHeaderFields];

			// Process headers.
			for (NSString* pKey in pHeaders)
			{
				NSString* pValue = [pHeaders objectForKey:pKey];
				(void)m_pRequest->m_Response.m_tHeaders.AddKeyValue(
					[pKey UTF8String],
					(UInt32)[pKey length],
					[pValue UTF8String],
					(UInt32)[pValue length]);
			}
		}

		// Cleanup the UrlSession parts.
		{
			// Sessions are responsible for creating tasks so they should be cleaned up first.
			if (nil != m_pRequest->m_pTask)
			{
				[m_pRequest->m_pTask release];
				m_pRequest->m_pTask = nil;
			}

			if (nil != m_pRequest->m_pSession)
			{
				[m_pRequest->m_pSession finishTasksAndInvalidate];
				[m_pRequest->m_pSession release];
				m_pRequest->m_pSession = nil;
			}

			if (nil != m_pRequest->m_pDelegate)
			{
				[m_pRequest->m_pDelegate release];
				m_pRequest->m_pDelegate = nil;
			}
		}

		// Handle various codes.
		auto eResult = HTTPResult::kFailure;
		auto pError = m_pError;
		if (bCanceled)
		{
			eResult = HTTPResult::kCanceled;
		}
		else if (nil == pError)
		{
			eResult = HTTPResult::kSuccess;
		}
		else
		{
			auto iUrlSessionCode = pError.code;
			switch (iUrlSessionCode)
			{
			case NSURLErrorCannotFindHost:
			case NSURLErrorCannotConnectToHost:
			case NSURLErrorDNSLookupFailed:
			case NSURLErrorNotConnectedToInternet:
			case NSURLErrorInternationalRoamingOff:
			case NSURLErrorCallIsActive:
			case NSURLErrorSecureConnectionFailed:
				eResult = HTTPResult::kConnectFailure;
				break;
			default:
				eResult = HTTPResult::kFailure;
				break;
			};
		}

		// WARNING: Release our pointer to the request - once
		// Finish is called, it may be destroyed
		// on another thread.
		auto pRequest = m_pRequest;
		m_pRequest.Reset();

		pRequest->Finish(eResult);
	}

	CheckedPtr<HTTPRequest> GetRequest() const
	{
		CheckedPtr<HTTPRequest> p;
		{
			Lock lock(m_Mutex);
			p = m_pRequest;
		}

		return p;
	}

	Bool IsCompleted() const
	{
		return m_bCompleted;
	}

	void OnComplete(NSError* pError)
	{
		Lock lock(m_Mutex);
		if (!m_pRequest) { return; }

		// Keep track of the error instance.
		if (nil == m_pError)
		{
			[pError retain];
			m_pError = pError;
		}

		// Record the round trip time.
		if (!m_bCompleted)
		{
			m_pRequest->m_Stats.m_fTotalRequestSecs = SeoulTime::ConvertTicksToSeconds(
				SeoulTime::GetGameTimeInTicks() - m_pRequest->m_iStartTimeInTicks);
		}

		// IMPORTANT: Must be last, since the API thread is allowed
		// to immediately finish this request once m_bCompleted is true.
		SeoulMemoryBarrier();
		m_bCompleted = true;
		SeoulMemoryBarrier();

		// Wake up the API thread.
		HTTPManager::s_ApiSignal.Activate();
	}

	void OnDataReceived(const void *pData, size_t zSize)
	{
		Lock lock(m_Mutex);
		if (!m_pRequest) { return; }

		m_pRequest->m_DownloadSizeSoFarInBytes += (Atomic32Type)zSize;
		m_pRequest->OnDataReceived(pData, zSize);

		// If the request has a progress callback, wake up the
		// tick thread so it can report the progress.
		if (m_pRequest->m_ProgressCallback)
		{
			HTTPManager::s_TickWorkerSignal.Activate();
		}
	}

	void SetRedirectURL(const String& sURL)
	{
		Lock lock(m_Mutex);
		if (!m_pRequest) { return; }

		m_pRequest->m_Response.m_sRedirectURL = sURL;
	}

	void OnResponse(NSURLResponse* pResponse)
	{
		Lock lock(m_Mutex);
		if (!m_pRequest) { return; }

		// Sanity handling - if somehow we received multiple responses,
		// always use the most recent.
		if (nil != m_pResponse)
		{
			[m_pResponse release];
			m_pResponse = nil;
		}

		// Track the response.
		[pResponse retain];
		m_pResponse = pResponse;

		// Carry through the total download size.
		m_pRequest->m_TotalDownloadSizeInBytes.Set((Atomic32Type)[pResponse expectedContentLength]);
	}

private:
	SEOUL_DISABLE_COPY(HTTPRequestHelper);
	SEOUL_REFERENCE_COUNTED(HTTPRequestHelper);

	Mutex m_Mutex;
	CheckedPtr<HTTPRequest> m_pRequest;
	NSURLResponse* m_pResponse;
	NSError* m_pError;
	Atomic32Value<Bool> m_bCompleted;
}; // class HTTPRequestHelper

static inline Bool operator==(const SharedPtr<HTTPRequestHelper>& a, HTTPRequest* b)
{
	if (a.IsValid())
	{
		return (a->GetRequest() == b);
	}
	else
	{
		return (nullptr == b);
	}
}

} // namespace Seoul

@implementation SeoulURLSessionDelegate

// Property synthesizers
@synthesize m_pRequestHelper;

/**
 * Initializes this delegate with the given request
 */
- (id)initWithRequest:(HTTPRequest *)pRequest
{
	if ((self = [super init]))
	{
		auto pRequestHelper = SEOUL_NEW(MemoryBudgets::Network) HTTPRequestHelper(pRequest);
		SeoulGlobalIncrementReferenceCount(pRequestHelper);
		self.m_pRequestHelper = pRequestHelper;
	}

	return self;
}

/**
 * Cleanup.
 */
- (void)dealloc
{
	auto p = self.m_pRequestHelper;
	self.m_pRequestHelper = nil;

	SeoulGlobalDecrementReferenceCount(p);
	[super dealloc];
}

/**
 * Session has been invalidated; may be called after request completes
 */
- (void)URLSession:(NSURLSession *)session
		didBecomeInvalidWithError:(NSError *)error;
{
	SEOUL_ASSERT(nullptr != self.m_pRequestHelper);
	self.m_pRequestHelper->OnComplete(error);
}

// Lets the session respond to an auth challenge; passes through to default behavior
- (void)URLSession:(NSURLSession *)session
		didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
		completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition, NSURLCredential *credential))completionHandler;
{
	completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
}

// Background requests completed while we were offline; handle them
// TODO: Not implemented; we don't use background downloading.
- (void)URLSessionDidFinishEventsForBackgroundURLSession:(NSURLSession *)session;
{
}

/**
 * Called when a session finishes, passing any client errors
 */
- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)error;
{
	SEOUL_ASSERT(nullptr != self.m_pRequestHelper);
	self.m_pRequestHelper->OnComplete(error);
}

/**
 * The HTTP request is being redirected
 */
- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task willPerformHTTPRedirection:(NSURLResponse *)response newRequest:(NSURLRequest *)request completionHandler:(void (^)(NSURLRequest *))completionHandler;
{
	SEOUL_ASSERT(nullptr != self.m_pRequestHelper);

	String const sURL([[request URL] absoluteString]);
	self.m_pRequestHelper->SetRedirectURL(sURL);

	completionHandler(request);
}

/**
 * Lets the session respond to an auth challenge; passes through to default behavior
 */
- (void)URLSession:(NSURLSession *)session
		task:(NSURLSessionTask *)task
		didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
		completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition, NSURLCredential *credential))completionHandler;
{
	completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
}

/**
 * Noop
 */
- (void)URLSession:(NSURLSession *)session
		task:(NSURLSessionTask *)task
		didSendBodyData:(int64_t)bytesSent
		totalBytesSent:(int64_t)totalBytesSent
		totalBytesExpectedToSend:(int64_t)totalBytesExpectedToSend;
{
}

/**
 * Noop
 */
- (void)URLSession:(NSURLSession *)session
		task:(NSURLSessionTask *)task
		didFinishCollectingMetrics:(NSURLSessionTaskMetrics *)metrics;
{
}

/**
 * Called when the session has received initial HTTP headers from a request.
 * Note that this may be called more than once, in the case of an HTTP load
 * where the content type of the load data is "multipart/x-mixed-replace".
 */
- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask didReceiveResponse:(NSURLResponse *)response completionHandler:(void (^)(NSURLSessionResponseDisposition))completionHandler;
{
	SEOUL_ASSERT(nullptr != self.m_pRequestHelper);
	self.m_pRequestHelper->OnResponse(response);

	completionHandler(NSURLSessionResponseAllow);
}

/**
 * Called as a session loads data incrementally
 */
- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask didReceiveData:(NSData *)data;
{
	SEOUL_ASSERT(nullptr != self.m_pRequestHelper);

	[data enumerateByteRangesUsingBlock:^(const void* bytes, NSRange byteRange, BOOL* stop)
	{
		m_pRequestHelper->OnDataReceived(bytes, byteRange.length);
	}];
}

/**
 * Noop
 */
- (void)URLSession:(NSURLSession *)session
		dataTask:(NSURLSessionDataTask *)dataTask
		didBecomeDownloadTask:(NSURLSessionDownloadTask *)downloadTask;
{
}

/**
 * Noop
 */
- (void)URLSession:(NSURLSession *)session
		dataTask:(NSURLSessionDataTask *)dataTask
		didBecomeStreamTask:(NSURLSessionStreamTask *)streamTask;
{
}

@end  // SeoulURLSessionDelegate implementation

namespace Seoul
{

/** Invoke by the API thread to actually start a request. */
SharedPtr<HTTPRequestHelper> HTTPRequest::UrlSessionStart(NSOperationQueue* pQueue)
{
	SEOUL_ASSERT(IsHttpApiThread()); // Sanity check.

	// Set up the NSURLRequest
	NSURL* pURL = [NSURL URLWithString:m_sURL.ToNSString()];
	NSMutableURLRequest* pRequest = [NSMutableURLRequest requestWithURL:pURL];
	[pRequest setTimeoutInterval:(NSTimeInterval)m_iTransferTimeoutInSeconds];
	[pRequest setHTTPMethod:m_sMethod.ToNSString()];

	// Never cache SeoulEngine requests.
	[pRequest setCachePolicy:NSURLRequestReloadIgnoringLocalCacheData];

	// For each custom header...
	for (auto const& pair : m_tHeaderData)
	{
		// Add it to the list of headers
		[pRequest addValue:pair.Second.ToNSString() forHTTPHeaderField:pair.First.ToNSString()];
	}

	// Set the POST body.
	if (HTTPMethod::ksPost == m_sMethod)
	{
		Byte const* pPostData = (m_PostBody.IsEmpty() ? nullptr : m_PostBody.GetBuffer());
		UInt32 const uPostData = (m_PostBody.GetTotalDataSizeInBytes());

		NSData* pBodyData = [NSData dataWithBytes:pPostData length:uPostData];
		[pRequest setHTTPBody:pBodyData];
	}

	// Session delegate.
	m_pDelegate = [[SeoulURLSessionDelegate alloc] initWithRequest:this];
	SharedPtr<HTTPRequestHelper> pReturn(m_pDelegate.m_pRequestHelper);

	// Session configuration.
	NSURLSessionConfiguration* config = [NSURLSessionConfiguration defaultSessionConfiguration];
	config.requestCachePolicy = NSURLRequestReloadIgnoringLocalCacheData;

	// Session itself.
	SEOUL_ASSERT(m_pSession == nullptr);
	m_pSession = [NSURLSession sessionWithConfiguration: config delegate:m_pDelegate delegateQueue:pQueue];

	// Retain the session - it will be released explicitly on completion.
	[m_pSession retain];

	// Instantiate a new task.
	m_pTask = [m_pSession dataTaskWithRequest: pRequest];

	// Retain the task - it will be released explicitly on completion.
	[m_pTask retain];

	// Start the connection
	[m_pTask resume];

	// Done, success.
	return pReturn;
}

Int HTTPManager::InternalUrlSessionThread(const Thread&)
{
	// We are the API thread.
	SetHttpApiThreadId(Thread::GetThisThreadId());

	// Create a shared operation queue.
	NSOperationQueue* pQueue = [[NSOperationQueue alloc] init];

	// Tracking of active requests.
	Vector< SharedPtr<HTTPRequestHelper>, MemoryBudgets::Network> vActive;

	while (!m_bApiShuttingDown)
	{
		// Scope the loop so we don't "leak" auto-released objects forever.
		// For Job system threads, this is handled, but we must do this
		// explicitly since we are our own thread.
		ScopedAutoRelease autoRelease;

		// Start requests that were queued.
		if (!m_bInBackground && !m_bApiShuttingDown)
		{
			// Process the start queue.
			auto pRequest = m_ApiToStartBuffer.Pop();
			while (nullptr != pRequest)
			{
				// Record the start time for round-trip tracking.
				pRequest->m_iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
				vActive.PushBack(pRequest->UrlSessionStart(pQueue));
				pRequest->m_bApiHasStarted = true;
				pRequest = m_ApiToStartBuffer.Pop();

				// Stop early if we've entered the background.
				if (m_bInBackground)
				{
					break;
				}
			}
		}

			// Process the cancel queue.
		if (!m_bInBackground && !m_bApiShuttingDown)
		{
			// Wake up the tick thread.
			Bool bWakeUpTickThread = false;

			auto pRequest = m_ApiToCancelBuffer.Pop();
			while (nullptr != pRequest)
			{
				// Remove prior to completion or cancelation completion.
				SharedPtr<HTTPRequestHelper> pRequestHelper;
				{
					auto const i = vActive.Find(pRequest);
					if (vActive.End() != i)
					{
						pRequestHelper = *i;
						vActive.Erase(i);
					}
				}

				// Retrieve the helper from the delegate.
				if (!pRequestHelper.IsValid() && nil != pRequest->m_pDelegate)
				{
					pRequestHelper.Reset(pRequest->m_pDelegate.m_pRequestHelper);
				}

				// Completion handling.
				if (!pRequest->m_bCompleted && pRequestHelper.IsValid())
				{
					pRequestHelper->Finish(true);
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
				(void)HTTPManager::s_TickWorkerSignal.Activate();
			}
		}

		// Tick
		if (!m_bInBackground && !m_bApiShuttingDown)
		{
			// Enumerate and process.
			for (auto i = vActive.Begin(); vActive.End() != i; )
			{
				// If the request has completed, finish it.
				if ((*i)->IsCompleted())
				{
					SharedPtr<HTTPRequestHelper> p(*i);
					i = vActive.Erase(i);
					p->Finish(false);
				}
				// Otherwise, advance passed it.
				else
				{
					++i;
				}
			}
		}

		// Wait on the Signal with a timeout, unless we're shutting down.
		if (!m_bApiShuttingDown)
		{
			// Wait on the API signal.
			s_ApiSignal.Wait();
		}
	}

	// Sanity, must have all been cleaned up.
	SEOUL_ASSERT(vActive.IsEmpty());

	// Cleanup the operation queue.
	[pQueue cancelAllOperations];
	[pQueue waitUntilAllOperationsAreFinished];
	[pQueue release];

	// We are no longer the API thread.
	SetHttpApiThreadId(ThreadId());

	return 0;
}

} // namespace Seoul
