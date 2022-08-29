/**
 * \file ScriptEngineHTTPRequest.h
 * \brief Binder instance for exposing the Seoul::HTTPRequest
 * into script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_HTTP_REQUEST_H
#define SCRIPT_ENGINE_HTTP_REQUEST_H

#include "Delegate.h"
#include "HTTPCommon.h"
#include "SharedPtr.h"
#include "SeoulHString.h"
namespace Seoul { namespace HTTP { class HeaderTable; } }
namespace Seoul { namespace HTTP { class RequestCancellationToken; } }
namespace Seoul { namespace HTTP { class RequestList; } }
namespace Seoul { struct ScriptEngineHTTPRequestCallback; }
namespace Seoul { namespace Script { class VmObject; } }

namespace Seoul
{

/** Binds Seoul::HTTPRequest into script. */
class ScriptEngineHTTPRequest SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(ScriptEngineHTTPRequest);

	ScriptEngineHTTPRequest();
	Bool Construct(
		HTTP::RequestList& rList,
		const String& sURL,
		const SharedPtr<Script::VmObject>& pCallback,
		HString method,
		Bool bResendOnFailure,
		Bool bCached);
	~ScriptEngineHTTPRequest();

	// Script accessible methods
	void AddHeader(const String& sKey, const String& sValue);
	void AddPostData(const String& sKey, const String& sValue);
	void SetLanesMask(UInt32 uMask);
	void Start();
	void StartWithPlatformSignInIdToken();
	void Cancel();
	// /Script accessible methods

	void OnCallback();

private:
	HTTP::Request* m_pRequest;
	ScriptEngineHTTPRequestCallback* m_pCallbackBinder;
	SharedPtr<HTTP::RequestCancellationToken> m_pCancellationToken;

	static HTTP::CallbackResult ResponseCallback(void* pUserData, HTTP::Result eResult, HTTP::Response* pResponse);

	SEOUL_DISABLE_COPY(ScriptEngineHTTPRequest);
}; // class AppScriptingFalconUIMovie

/** Wrapper to efficiently pass the HTTP headers table into Lua. */
struct ScriptEngineHTTPHeaderTable
{
	ScriptEngineHTTPHeaderTable()
		: m_pTable(nullptr)
	{
	}

	HTTP::HeaderTable const* m_pTable;
}; // class ScriptEngineHTTPHeaderTable

template <>
struct DataNodeHandler<ScriptEngineHTTPHeaderTable, false>
{
	static const Bool Value = true;

	static Bool FromDataNode(Reflection::SerializeContext&, const DataStore& dataStore, const DataNode& dataNode, ScriptEngineHTTPHeaderTable& rmValue);
	static Bool ToArray(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& array, UInt32 uIndex, const ScriptEngineHTTPHeaderTable& mValue);
	static Bool ToTable(Reflection::SerializeContext&, DataStore& rDataStore, const DataNode& table, HString key, const ScriptEngineHTTPHeaderTable& mValue);
	static void FromScript(lua_State* pVm, Int iOffset, ScriptEngineHTTPHeaderTable& r);
	static void ToScript(lua_State* pVm, const ScriptEngineHTTPHeaderTable& v);
}; // struct DataNodeHandler<ScriptEngineHTTPHeaderTable, false>

} // namespace Seoul

#endif // include guard
