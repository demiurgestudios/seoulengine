/**
 * \file ScriptEngineHTTP.h
 * \brief Binder instance for exposing HTTP functionality into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_HTTP_H
#define SCRIPT_ENGINE_HTTP_H

#include "Prereqs.h"
#include "ScopedPtr.h"
namespace Seoul { namespace HTTP { class RequestList; } }
namespace Seoul { namespace Script { class FunctionInterface; } }

namespace Seoul
{

/** Script binding for Seoul::HTTP into a script VM. */
class ScriptEngineHTTP SEOUL_SEALED
{
public:
	ScriptEngineHTTP();
	~ScriptEngineHTTP();

	// Cancel all pending requests created via ScriptEngineHTTP.
	void CancelAllRequests();

	// Instantiate a Seoul::HTTPRequest script wrapper and return it.
	void CreateRequest(Script::FunctionInterface* pInterface);

	// Issue a cached request. Identical to CreateRequest,
	// except the returned results (on success, code 200)
	// will be cached, and the lastest version of the cached
	// data can be accessed via GetCacheData() with the request
	// URL.
	void CreateCachedRequest(Script::FunctionInterface* pInterface);

	// Retrieve data of a previously cached request, if available.
	void GetCachedData(Script::FunctionInterface* pInterface) const;

	// Update the cached HTTP request body; use with care. Does not update
	// cached headers, and has no effect if there's no cached result for sURL.
	void OverrideCachedDataBody(Script::FunctionInterface* pInterface);

private:
	ScopedPtr<HTTP::RequestList> m_pList;

	SEOUL_DISABLE_COPY(ScriptEngineHTTP);
}; // class ScriptEngineHTTP

} // namespace Seoul

#endif // include guard
