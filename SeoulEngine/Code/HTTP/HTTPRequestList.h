/**
 * \file HTTPRequestList.h
 * \brief Specialized doubly linked-list implementation for tracking
 * HTTP::Request instances.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	HTTP_REQUEST_LIST_H
#define HTTP_REQUEST_LIST_H

#include "CheckedPtr.h"
#include "Mutex.h"
#include "PerThreadStorage.h"
#include "Prereqs.h"
namespace Seoul { namespace HTTP { class Request; } }
namespace Seoul { namespace HTTP { class RequestListNode; } }

namespace Seoul::HTTP
{

/** Simplified List<> like utility structure to allow clients to track HTTP::Request instances. */
class RequestList SEOUL_SEALED
{
public:
	RequestList()
		: m_pHead()
		, m_Mutex()
		, m_ActiveRequestCallbacks()
	{
	}

	/** Cancel and wait for all requests in this list to complete. */
	void BlockingCancelAll();

	/** @return True if no entries are contained in this list, false otherwise. */
	Bool IsEmpty() const
	{
		return (!m_pHead);
	}

	/** @return The head entry of this HTTPRequestList. */
	CheckedPtr<RequestListNode> GetHead() const
	{
		return m_pHead;
	}

	// Mutex access for synchronizing access to the list.
	const Mutex& GetMutex() const
	{
		return m_Mutex;
	}

private:
	// Functions to keep m_iActiveRequestCallbacks in sync
	size_t GetHttpCallbackCount();
	void EnterHttpCallback();
	void ExitHttpCallback();

private:
	SEOUL_DISABLE_COPY(RequestList);

	friend class RequestListNode;
	friend class ScopedHTTPRequestListCallbackCount;

	CheckedPtr<RequestListNode> m_pHead;
	Mutex m_Mutex;

	// Tracks the number of active request callbacks; must be 0 when BlockingCancelAll() is called,
	// or the thread will block forever. Use here of PerThreadStorage is intentional - this check
	// is effectively a per-thread reentrancy check. We're checking that we're not inside the dispatch
	// of a callback on a particular thread so that we don't attempt to call BlockingCancelAll()
	// from within that callback, since it can invalidate structures that the callback depends on.
	PerThreadStorage m_ActiveRequestCallbacks;
}; // class HTTP::RequestList

class ScopedHTTPRequestListCallbackCount SEOUL_SEALED
{
public:
	explicit ScopedHTTPRequestListCallbackCount(CheckedPtr<RequestList>);
	~ScopedHTTPRequestListCallbackCount();
private:
	CheckedPtr<RequestList> const m_pRequestList;

	SEOUL_DISABLE_COPY(ScopedHTTPRequestListCallbackCount);
}; // class ScopedHTTPRequestListCallbackCount

/** Internal structured used in HTTP::Request to handle HTTP::RequestList membership. */
class RequestListNode SEOUL_SEALED
{
public:
	RequestListNode(Request* pThis);
	~RequestListNode();

	/** @return The next entry in this Nodes list. */
	CheckedPtr<RequestListNode> GetNext() const
	{
		return m_pNext;
	}

	/** @return The current list owner of this node or not valid if no current owner. */
	CheckedPtr<RequestList> GetOwner() const
	{
		return m_pOwner;
	}

	/** @return The previous entry in this Nodes list. */
	CheckedPtr<RequestListNode> GetPrev() const
	{
		return m_pPrev;
	}

	/** @return The HTTP::Request of this Node. */
	CheckedPtr<Request> GetThis() const
	{
		return m_pThis;
	}

	void Insert(RequestList& rList);
	void Remove();

private:
	void InsideLockRemove();

	CheckedPtr<Request> const m_pThis;
	CheckedPtr<RequestList> m_pOwner;
	CheckedPtr<RequestListNode> m_pNext;
	CheckedPtr<RequestListNode> m_pPrev;

	SEOUL_DISABLE_COPY(RequestListNode);
}; // class HTTP::RequestListNode

} // namespace Seoul::HTTP

#endif // include guard
