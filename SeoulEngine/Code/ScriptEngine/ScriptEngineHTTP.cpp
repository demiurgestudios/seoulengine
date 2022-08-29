/**
 * \file ScriptEngineHTTP.cpp
 * \brief Binder instance for exposing HTTP functionality into
 * a script VM.
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
#include "HTTPRequestList.h"
#include "ReflectionDefine.h"
#include "ScriptEngineHTTP.h"
#include "ScriptEngineHTTPRequest.h"
#include "ScriptFunctionInterface.h"
#include "ScriptVm.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngineHTTP, TypeFlags::kDisableCopy)
	SEOUL_METHOD(CancelAllRequests)
	SEOUL_METHOD(CreateRequest)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "ScriptEngineHTTPRequest", "string sURL, object oCallback = null, string sMethod = HTTPMethods.m_sMethodGet, bool bResendOnFailure = true")
	SEOUL_METHOD(CreateCachedRequest)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "ScriptEngineHTTPRequest", "string sURL, object oCallback = null, string sMethod = HTTPMethods.m_sMethodGet, bool bResendOnFailure = true")
	SEOUL_METHOD(GetCachedData)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(string, SlimCS.Table)", "string sURL")
	SEOUL_METHOD(OverrideCachedDataBody)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "string sURL, SlimCS.Table tData")
	SEOUL_END_TYPE()

ScriptEngineHTTP::ScriptEngineHTTP()
	: m_pList(SEOUL_NEW(MemoryBudgets::Scripting) HTTP::RequestList)
{
}

ScriptEngineHTTP::~ScriptEngineHTTP()
{
	m_pList->BlockingCancelAll();
}

/** Cancel any and all pending requests created and started via CreateRequest(). */
void ScriptEngineHTTP::CancelAllRequests()
{
	m_pList->BlockingCancelAll();
}

/** Common implementation for CreateRequest() and CreateCachedRequest(). */
static inline void CreateRequestCommon(HTTP::RequestList& rList, Script::FunctionInterface* pInterface, Bool bCached)
{
	String sURL;
	if (!pInterface->GetString(1, sURL))
	{
		pInterface->RaiseError(1, "expected string URL.");
		return;
	}

	// pCallback, bResendOnFailure, and sMethod are optional arguments -
	// only report an error if an argument is specified but
	// not the expected type.
	SharedPtr<Script::VmObject> pCallback;
	HString method = HTTP::Method::ksGet;
	Bool bResendOnFailure = true;

	// Begin of optional arguments.
	if (!pInterface->IsNilOrNone(2))
	{
		// If argument 2 is specified but is not a function,
		// this is an error.
		if (!pInterface->GetFunction(2, pCallback))
		{
			pInterface->RaiseError(2, "expected function HTTP callback");
			return;
		}
	}

	// Optional method argument.
	if (!pInterface->IsNilOrNone(3))
	{
		// If argument 3 is specified but is not a string,
		// this is an error.
		if (!pInterface->GetString(3, method))
		{
			pInterface->RaiseError(3, "expected string sMethod");
			return;
		}
	}

	// Optional bResendOnFailure argument.
	if (!pInterface->IsNilOrNone(4))
	{
		// If argument 4 is specified but is not a boolean,
		// this is an error.
		if (!pInterface->GetBoolean(4, bResendOnFailure))
		{
			pInterface->RaiseError(4, "expected boolean bResendOnFailure");
			return;
		}
	}
	// End Optional argument handling.

	// Create the request object and then construct it.
	ScriptEngineHTTPRequest* pScriptEngineHTTPRequest = pInterface->PushReturnUserData<ScriptEngineHTTPRequest>();
	if (nullptr == pScriptEngineHTTPRequest)
	{
		pInterface->RaiseError(-1, "failed allocating HTTP request.");
		return;
	}

	// Final step, construct.
	if (!pScriptEngineHTTPRequest->Construct(
		rList,
		sURL,
		pCallback,
		method,
		bResendOnFailure,
		bCached))
	{
		pInterface->RaiseError(-1, "failed constructing HTTP request, check for invalid arguments.");
		return;
	}
}

/** Instantiate a Seoul::HTTPRequest script wrapper and return it. */
void ScriptEngineHTTP::CreateRequest(Script::FunctionInterface* pInterface)
{
	CreateRequestCommon(*m_pList, pInterface, false);
}

/** Instantiate a Seoul::HTTPRequest script wrapper (with caching enabled) and return it. */
void ScriptEngineHTTP::CreateCachedRequest(Script::FunctionInterface* pInterface)
{
	CreateRequestCommon(*m_pList, pInterface, true);
}

/** Retrieve data of a previously cached request, if available. */
void ScriptEngineHTTP::GetCachedData(Script::FunctionInterface* pInterface) const
{
	// No URL, error.
	String sURL;
	if (!pInterface->GetString(1, sURL))
	{
		pInterface->RaiseError(1, "expected string URL.");
		return;
	}

	// If we failed to acquire the data, immediately return nil.
	Game::ClientCacheLock lock(sURL);
	if (!lock.HasData())
	{
		pInterface->PushReturnNil();
		return;
	}

	// Success, send the data to script.
	auto pData = lock.GetData();
	pInterface->PushReturnString((Byte const*)pData->GetBody(), (UInt32)pData->GetBodySize());

	ScriptEngineHTTPHeaderTable wrapper;
	wrapper.m_pTable = &pData->GetHeaders();
	pInterface->PushReturnAsTable(wrapper);
}

void ScriptEngineHTTP::OverrideCachedDataBody(Script::FunctionInterface* pInterface)
{
	// No URL, error.
	String sURL;
	if (!pInterface->GetString(1, sURL))
	{
		pInterface->RaiseError(1, "expected string URL.");
		return;
	}

	DataStore bodyTable;
	if (!pInterface->GetTable(2, bodyTable))
	{
		pInterface->RaiseError(1, "expected table body.");
		return;
	}

	Game::Client::Get()->OverrideCachedDataBody(sURL, bodyTable);
}

} // namespace Seoul
