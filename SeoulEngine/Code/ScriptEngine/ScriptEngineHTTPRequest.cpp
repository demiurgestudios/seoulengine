/**
 * \file ScriptEngineHTTPRequest.cpp
 * \brief Binder instance for exposing the Seoul::HTTPRequest
 * into script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

// TODO: Fix this up reference, ScriptEngine should not be dependent on the Game project.
#include "GameClient.h"
#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "Logger.h"
#include "PlatformSignInManager.h"
#include "ReflectionDefine.h"
#include "ScriptEngineHTTPRequest.h"
#include "ScriptFunctionInvoker.h"
#include "ScriptVm.h"
#include "ThreadId.h"

namespace Seoul
{

/**
 * Utility, wraps a script function
 * to allow it to be bound as an
 * HTTP request callback.
 */
struct ScriptEngineHTTPRequestCallback SEOUL_SEALED
{
	ScriptEngineHTTPRequestCallback()
		: m_pRequest(nullptr)
	{
	}

	SharedPtr<Script::VmObject> m_pCallback;
	ScriptEngineHTTPRequest* m_pRequest;
}; // struct ScriptEngineHTTPRequestCallback

ScriptEngineHTTPRequest::ScriptEngineHTTPRequest()
	: m_pRequest(nullptr)
	, m_pCallbackBinder(nullptr)
	, m_pCancellationToken()
{
}

ScriptEngineHTTPRequest::~ScriptEngineHTTPRequest()
{
	if (nullptr != m_pCallbackBinder)
	{
		m_pCallbackBinder->m_pRequest = nullptr;
	}

	// If m_pRequest is still not null, it means
	// it was never started which means we need
	// to destroy it manually.
	HTTP::Manager::Get()->DestroyUnusedRequest(m_pRequest);
}

/** Must be called immediately after ScriptEngineHTTPRequest(). */
Bool ScriptEngineHTTPRequest::Construct(
	HTTP::RequestList& rList,
	const String& sURL,
	const SharedPtr<Script::VmObject>& pCallback,
	HString method,
	Bool bResendOnFailure,
	Bool bCached)
{
	// If a script callback was specified, setup a binder for it.
	m_pCallbackBinder = SEOUL_NEW(MemoryBudgets::Scripting) ScriptEngineHTTPRequestCallback;
	m_pCallbackBinder->m_pCallback = pCallback;
	m_pCallbackBinder->m_pRequest = this;
	HTTP::ResponseDelegate callback = SEOUL_BIND_DELEGATE(&ResponseCallback, m_pCallbackBinder);

	// Instantiate the request and configure the request.
	auto& r = HTTP::Manager::Get()->CreateRequest(&rList);

	// TODO: Fix this up reference, ScriptEngine should not be dependent on the Game project.
	if (bCached)
	{
		// Now wrap the request if specified. Don't want to do this until we're sure we'll issue the request.
		callback = Game::Client::Get()->WrapCallbackForCache(callback, sURL);
	}

	r.SetMethod(method);
	r.SetURL(sURL);
	r.SetCallback(callback);
	r.SetResendOnFailure(bResendOnFailure);

	// Setup the request for main thread dispatch (although the script VM is thread-safe, it
	// doesn't benefit us to access it off the main thread).
	r.SetDispatchCallbackOnMainThread(true);

	// TODO: Fix this up reference, ScriptEngine should not be dependent on the Game project.
	Game::Client::Get()->PrepareRequest(r, false);

	// Done, cache for further API access.
	m_pRequest = &r;
	return true;
}

/** Script wrapper around HTTP::Request::AddHeader(). */
void ScriptEngineHTTPRequest::AddHeader(const String& sKey, const String& sValue)
{
	if (nullptr != m_pRequest)
	{
		m_pRequest->AddHeader(sKey, sValue);
	}
}

/** Script wrapper around HTTP::Request::AddPostData(). */
void ScriptEngineHTTPRequest::AddPostData(const String& sKey, const String& sValue)
{
	if (nullptr != m_pRequest)
	{
		m_pRequest->AddPostData(sKey, sValue);
	}
}

/**
 * Script wrapper around HTTP::Request::SetLanesMask().
 */
void ScriptEngineHTTPRequest::SetLanesMask(UInt32 uMask)
{
	if (nullptr != m_pRequest)
	{
		m_pRequest->SetLanesMask(uMask);
	}
}

/**
 * Script wrapper around HTTP::Request::Start(). Once called, further calls
 * to ScriptEngineHTTPRequest are effectively a nop.
 */
void ScriptEngineHTTPRequest::Start()
{
	if (nullptr != m_pRequest)
	{
		m_pCancellationToken = m_pRequest->Start();
		m_pRequest = nullptr;
	}
}

/** The same as Start(), but routes through PlatformSignInManager so the platform id token can be asynchronously added to the request. */
void ScriptEngineHTTPRequest::StartWithPlatformSignInIdToken()
{
	if (nullptr != m_pRequest)
	{
		auto pRequest = m_pRequest;
		m_pRequest = nullptr;

		PlatformSignInManager::Get()->StartWithIdToken(*pRequest);
	}
}

void ScriptEngineHTTPRequest::Cancel()
{
	if (m_pCancellationToken.IsValid())
	{
		m_pCancellationToken->Cancel();
	}
}

void ScriptEngineHTTPRequest::OnCallback()
{
	m_pCallbackBinder = nullptr;
}

/** Static handler for HTTP::Request callbacks to script functions. */
HTTP::CallbackResult ScriptEngineHTTPRequest::ResponseCallback(void* pUserData, HTTP::Result eResult, HTTP::Response* pResponse)
{
	SEOUL_ASSERT(IsMainThread());

	ScriptEngineHTTPRequestCallback* pBinder = ((ScriptEngineHTTPRequestCallback*)pUserData);

	if(pBinder->m_pRequest)
	{
		pBinder->m_pRequest->OnCallback();
		pBinder->m_pRequest = nullptr;
	}

	// TODO: Fix this up reference, ScriptEngine should not be dependent on the Game project.

	// If the response includes server time headers, pass them on
	Game::Client::Get()->UpdateCurrentServerTimeFromResponse(pResponse);

	// Invoke the script callback, if defined.
	if (pBinder->m_pCallback.IsValid())
	{
		Script::FunctionInvoker invoker(pBinder->m_pCallback);
		if (invoker.IsValid())
		{
			// Arguments included in all cases.
			invoker.PushInteger((Int32)eResult);
			invoker.PushInteger(pResponse->GetStatus());
			invoker.PushString((Byte const*)pResponse->GetBody(), pResponse->GetBodySize());

			ScriptEngineHTTPHeaderTable wrapper;
			wrapper.m_pTable = &pResponse->GetHeaders();
			invoker.PushAsTable(wrapper);

			// TODO: Get the resend status here; if nil, default to (status == 500)
			(void)invoker.TryInvoke();
		}
	}

	// In all cases, make sure we free the binder data after the callback.
	SafeDelete(pBinder);

	return HTTP::CallbackResult::kSuccess;
}

} // namespace Seoul
