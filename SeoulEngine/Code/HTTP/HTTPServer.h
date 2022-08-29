/**
 * \file HTTPServer.h
 * \brief Seoul API around the 2013 fork of Mongoose (the MIT
 * licensed version of Mongoose, prior to a licensing changing in
 * 2013 to GPLv2).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	HTTP_SERVER_H
#define HTTP_SERVER_H

#include "Atomic32.h"
#include "CheckedPtr.h"
#include "Delegate.h"
#include "HashTable.h"
#include "HTTPHeaderTable.h"
#include "Mutex.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
#include "SeoulString.h"
#include "Vector.h"
extern "C" { struct mg_callbacks; }
extern "C" { struct mg_connection; }
extern "C" { struct mg_context; }

namespace Seoul::HTTP
{

struct ServerRewritePattern SEOUL_SEALED
{
	String m_sFrom;
	String m_sTo;
};

struct ServerRequestInfo SEOUL_SEALED
{
	typedef Vector<Byte, MemoryBudgets::Network> Body;

	String m_sMethod;
	String m_sUri;
	HeaderTable m_Headers;
	Body m_vBody;
};

class ServerResponseWriter SEOUL_SEALED
{
public:
	ServerResponseWriter(mg_connection* connection);
	void WriteStatusResponse(Int status, const HeaderTable& headers, const String& body);

	// Has the writer been used?
	Bool WroteResponse();

private:
	mg_connection* const m_pConnection;
	Bool m_bWroteResponse;

	SEOUL_DISABLE_COPY(ServerResponseWriter);
};

struct ServerSettings SEOUL_SEALED
{
	typedef Vector<ServerRewritePattern, MemoryBudgets::Network> RewritePatterns;

	ServerSettings()
		: m_Handler()
		, m_sRootDirectory()
		, m_iPort(8057)
		, m_iThreadCount(50)
	{
	}

	/**
	 * (Optional) If specified, requests are routed through this delegate. Used for custom handling.
	 *
	 * This callback should return true if it has handled the request, in which case the
	 * server will return status code 200 with no further processing.
	 */
	Delegate<Bool(ServerResponseWriter& responseWriter, const ServerRequestInfo& info)> m_Handler;

	/** Absolute path to the root of the server's document directory. */
	String m_sRootDirectory;

	/** Bind port of the server. */
	Int32 m_iPort;

	/** Number of threads the server will create to handle requests. */
	Int32 m_iThreadCount;

	/** Optional list of URL rewrites patterns. */
	RewritePatterns m_vRewritePatterns;
}; // struct HTTP::ServerSettings

class Server SEOUL_SEALED
{
public:
	Server(const ServerSettings& settings);
	~Server();

	/** @return The total number of requests received by the server. */
	Atomic32Type GetReceivedRequestCount() const
	{
		return m_ReceivedRequestCount;
	}

private:
	ServerSettings const m_Settings;
	ScopedPtr<mg_callbacks> m_pCallbacks;
	CheckedPtr<mg_context> m_pContext;
	Atomic32 m_ReceivedRequestCount;

	Mutex m_FilesMutex;
	typedef HashTable<String, Pair<void*, UInt32>, MemoryBudgets::Network> Files;
	Files m_tFiles;

	Int OnBeginRequest(mg_connection* c);
	Byte const* OnOpenFile(const mg_connection*, Byte const* sPath, size_t* pzOutLength);
	static Int OnBeginRequestCallback(mg_connection* c);
	static Int OnLogMessage(const mg_connection*, const char *message);
	static Byte const* OnOpenFileCallback(const mg_connection*, Byte const* sPath, size_t* pzOutLength);

	class ScopedRequest SEOUL_SEALED
	{
	public:
		ScopedRequest(Server& r)
			: m_r(r)
		{
		}

		~ScopedRequest()
		{
			++m_r.m_ReceivedRequestCount;
		}

	private:
		Server& m_r;

		SEOUL_DISABLE_COPY(ScopedRequest);
	}; // class ScopedRequest

	SEOUL_DISABLE_COPY(Server);
}; // class HTTP::Server

} // namespace Seoul::HTTP

#endif // include guard
