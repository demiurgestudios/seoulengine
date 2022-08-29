/**
* \file HTTPServer.cpp
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

#include "DiskFileSystem.h"
#include "FileManager.h"
#include "HTTPServer.h"
#include "Logger.h"
#include "ToString.h"
#include "Vector.h"
#include <mongoose-mit.h>

namespace Seoul::HTTP
{

static const HString kContentLength("content-length");

static void SendSimpleResponse(mg_connection* c, Bool bOk)
{
	mg_printf(
		c,
		"HTTP/1.1 %s\r\n"
		"Connection: close\r\n"
		"Content-Type: text/html; charset=utf-8\r\n\r\n",
		(bOk ? "200 OK" : "500 Internal Server Error"));
}

ServerResponseWriter::ServerResponseWriter(mg_connection* connection)
	: m_pConnection(connection)
	, m_bWroteResponse(false)
{}

void ServerResponseWriter::WriteStatusResponse(Int status, const HeaderTable& headers, const String& body)
{
	m_bWroteResponse = true;

	mg_printf(
		m_pConnection,
		"HTTP/1.1 %d STATUS\r\n"
		"Connection: close\r\n"
		"Content-Type: text/html; charset=utf-8\r\n",
		status);

	for (auto header : headers.GetKeyValues())
	{
		auto key = header.First;
		auto value = header.Second;
		mg_printf(m_pConnection, "%s: %s\r\n", key.CStr(), value.m_sValue);
	}

	mg_printf(m_pConnection, "\r\n%s", body.CStr());
}

Bool ServerResponseWriter::WroteResponse()
{
	return m_bWroteResponse;
}

Server::Server(const ServerSettings& settings)
	: m_Settings(settings)
	, m_pCallbacks(SEOUL_NEW(MemoryBudgets::Network) mg_callbacks)
	, m_pContext()
	, m_ReceivedRequestCount(0)
{
	memset(m_pCallbacks.Get(), 0, sizeof(*m_pCallbacks));
	m_pCallbacks->begin_request = OnBeginRequestCallback;
	m_pCallbacks->log_message = OnLogMessage;
	m_pCallbacks->open_file = OnOpenFileCallback;

	typedef Vector<Byte const*, MemoryBudgets::Network> Options;
	Options vOptions;

	if (!m_Settings.m_sRootDirectory.IsEmpty())
	{
		vOptions.PushBack("document_root");
		vOptions.PushBack(m_Settings.m_sRootDirectory.CStr());
	}

	String sRewritePatterns;
	if (!m_Settings.m_vRewritePatterns.IsEmpty())
	{
		for (auto i = m_Settings.m_vRewritePatterns.Begin(); m_Settings.m_vRewritePatterns.End() != i; ++i)
		{
			if (!sRewritePatterns.IsEmpty())
			{
				sRewritePatterns.Append(',');
			}
			sRewritePatterns.Append(i->m_sFrom);
			sRewritePatterns.Append('=');
			sRewritePatterns.Append(i->m_sTo);
		}

		vOptions.PushBack("url_rewrite_patterns");
		vOptions.PushBack(sRewritePatterns.CStr());
	}

	// Override default mapping for .json.
	vOptions.PushBack("extra_mime_types");
	vOptions.PushBack(".json=application/json");

	String const m_sPort(ToString(m_Settings.m_iPort));
	vOptions.PushBack("listening_ports");
	vOptions.PushBack(m_sPort.CStr());

	String const m_sThreadCount(ToString(m_Settings.m_iThreadCount));
	vOptions.PushBack("num_threads");
	vOptions.PushBack(m_sThreadCount.CStr());

	vOptions.PushBack(nullptr);
	m_pContext = mg_start(
		m_pCallbacks.Get(),
		this,
		vOptions.Data());

	SEOUL_ASSERT(m_pContext.IsValid());
}

Server::~Server()
{
	mg_stop(m_pContext);
	m_pContext.Reset();

	m_pCallbacks.Reset();

	// Cleanup file data.
	{
		Lock lock(m_FilesMutex);
		for (auto const& e : m_tFiles)
		{
			MemoryManager::Deallocate(e.Second.First);
		}
		m_tFiles.Clear();
	}
}

Int Server::OnBeginRequest(mg_connection* c)
{
	// Increment the received request count right before return.
	ScopedRequest scoped(*this);

	if (m_Settings.m_Handler)
	{
		ServerRequestInfo info;
		{
			auto p = mg_get_request_info(c);
			info.m_sMethod.Assign(p->request_method);
			info.m_sUri.Assign(p->uri);
			for (Int32 i = 0; i < p->num_headers; ++i)
			{
				info.m_Headers.AddKeyValue(
					p->http_headers[i].name,
					StrLen(p->http_headers[i].name),
					p->http_headers[i].value,
					StrLen(p->http_headers[i].value));
			}

			String sLength;
			UInt32 uLength = 0u;
			if (info.m_Headers.GetValue(kContentLength, sLength) &&
				FromString(sLength, uLength))
			{
				ServerRequestInfo::Body vBody(uLength);
				if (uLength > 0u)
				{
					if ((Int32)uLength == mg_read(c, vBody.Data(), (size_t)uLength))
					{
						info.m_vBody.Swap(vBody);
					}
				}
			}
		}

		ServerResponseWriter responseWriter(c);

		Bool bHandled = m_Settings.m_Handler(responseWriter, info);
		if (responseWriter.WroteResponse())
		{
			// Request was handled and a response was already written
			return 1;
		}
		else if (bHandled)
		{
			// Request was handled, but no response was written
			SendSimpleResponse(c, true);
			return 1;
		}
	}

	// Request remains unhandled.
	return 0;
}

Byte const* Server::OnOpenFile(
	const mg_connection*,
	Byte const* sPath,
	size_t* pzOutLength)
{
	String s(sPath);

	// Acquire cached data.
	{
		Lock lock(m_FilesMutex);
		Pair<void*, UInt32> data;
		if (m_tFiles.GetValue(s, data))
		{
			if (nullptr != pzOutLength) { *pzOutLength = (size_t)data.Second; }
			return (Byte const*)data.First;
		}
	}

	// Read the data.
	void* p = nullptr;
	UInt32 u = 0u;
	if (!FileManager::Get())
	{
		(void)DiskSyncFile::ReadAll(
			sPath,
			p,
			u,
			0u,
			MemoryBudgets::Network);
	}
	else
	{
		FileManager::Get()->ReadAll(
			sPath,
			p,
			u,
			0u,
			MemoryBudgets::Network);
	}

	// Cache if successful.
	if (nullptr != p)
	{
		Lock lock(m_FilesMutex);
		auto const e = m_tFiles.Insert(s, MakePair(p, u));
		if (!e.Second)
		{
			MemoryManager::Deallocate(p);
			p = e.First->Second.First;
			u = e.First->Second.Second;
		}
	}

	// Return results, on success, also populate length.
	if (nullptr != p && nullptr != pzOutLength)
	{
		*pzOutLength = (size_t)u;
	}

	return (Byte const*)p;
}

Int Server::OnBeginRequestCallback(mg_connection* c)
{
	Server& r = *((Server*)mg_get_request_info(c)->user_data);
	return r.OnBeginRequest(c);
}

Int Server::OnLogMessage(const mg_connection*, const char *message)
{
	SEOUL_WARN("%s", message);
	return 0;
}

Byte const* Server::OnOpenFileCallback(
	const mg_connection* c,
	Byte const* sPath,
	size_t* pzOutLength)
{
	Server& r = *((Server*)mg_get_request_info((mg_connection*)c)->user_data);
	return r.OnOpenFile(c, sPath, pzOutLength);
}

} // namespace Seoul::HTTP
