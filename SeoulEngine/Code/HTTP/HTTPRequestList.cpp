/**
 * \file HTTPRequestList.cpp
 * \brief Specialized doubly linked-list implementation for tracking
 * HTTP::Request instances.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "HTTPRequestList.h"
#include "JobsManager.h"
#include "Thread.h"

namespace Seoul::HTTP
{

size_t RequestList::GetHttpCallbackCount()
{
	auto pValue = m_ActiveRequestCallbacks.GetPerThreadStorage();
	return reinterpret_cast<size_t>(pValue);
}

void RequestList::EnterHttpCallback()
{
	size_t value = GetHttpCallbackCount();
	value++;
	m_ActiveRequestCallbacks.SetPerThreadStorage(reinterpret_cast<void*>(value));
}

void RequestList::ExitHttpCallback()
{
	size_t value = GetHttpCallbackCount();
	value--;
	m_ActiveRequestCallbacks.SetPerThreadStorage(reinterpret_cast<void*>(value));
}

/** Cancel all requests in this list and wait for all to complete. */
void RequestList::BlockingCancelAll()
{
	SEOUL_ASSERT_MESSAGE(
		GetHttpCallbackCount() == 0,
		"Called HTTPRequestList::BlockingCancelAll while an HTTP response callback was active on the same thread");

	// While requests are pending.
	while (true)
	{
		{
			// Synchronize access to the list.
			Lock lock(m_Mutex);

			// Done if list is empty.
			if (IsEmpty())
			{
				return;
			}

			// Cancel all requests.
			for (CheckedPtr<RequestListNode> p = GetHead(); p.IsValid(); p = p->GetNext())
			{
				CheckedPtr<Request> pRequest(p->GetThis());
				pRequest->m_pCancellationToken->Cancel();
			}
		}

		// If we're the main thread, yield some time to the HTTP manager.
		if (IsMainThread())
		{
			// Tick the HTTP manager to commit the cancellation requests.
			Manager::Get()->Tick();
		}

		// TODO: Need to introduce signaling semantics to Jobs::Manager
		// to eliminate cases like this.

		// Make sure the Jobs::Manager is getting some time while we're spinning in this loop.
		Jobs::Manager::Get()->YieldThreadTime();
	}
}

ScopedHTTPRequestListCallbackCount::ScopedHTTPRequestListCallbackCount(CheckedPtr<RequestList> pList)
	: m_pRequestList(pList)
{
	if (pList.IsValid())
	{
		pList->EnterHttpCallback();
	}
}

ScopedHTTPRequestListCallbackCount::~ScopedHTTPRequestListCallbackCount()
{
	if (m_pRequestList.IsValid())
	{
		m_pRequestList->ExitHttpCallback();
	}
}

RequestListNode::RequestListNode(Request* pThis)
	: m_pThis(pThis)
	, m_pOwner(nullptr)
	, m_pNext(nullptr)
	, m_pPrev(nullptr)
{
}

RequestListNode::~RequestListNode()
{
	// Remove this node from its owning list, if defined.
	Remove();
}

void RequestListNode::Insert(RequestList& rList)
{
	// Synchronize.
	Lock lock(rList.m_Mutex);

	// Remove this node from its current owning list, if defined.
	InsideLockRemove();

	// If the list has a head instance, point its previous pointer
	// at this instance.
	if (rList.m_pHead)
	{
		rList.m_pHead->m_pPrev = this;
	}

	// Our next is the existing head.
	m_pNext = rList.m_pHead;

	// The head is now this instance.
	rList.m_pHead = this;

	// Cache the owner.
	m_pOwner = &rList;
}

void RequestListNode::Remove()
{
	// If we have no owner, nop - must have an owner to be in a list.
	if (!m_pOwner)
	{
		// Sanity check that all our other variables are nullptr.
		SEOUL_ASSERT(!m_pNext.IsValid());
		SEOUL_ASSERT(!m_pPrev.IsValid());
		return;
	}

	// Synchronize.
	Lock lock(m_pOwner->m_Mutex);

	// Now perform the actual remove.
	InsideLockRemove();
}

void RequestListNode::InsideLockRemove()
{
	// If we have no owner, nop - must have an owner to be in a list.
	if (!m_pOwner)
	{
		// Sanity check that all our other variables are nullptr.
		SEOUL_ASSERT(!m_pNext.IsValid());
		SEOUL_ASSERT(!m_pPrev.IsValid());
		return;
	}

	// If we have a next pointer, update its previous pointer.
	if (m_pNext)
	{
		m_pNext->m_pPrev = m_pPrev;
	}

	// If we have a previous pointer, update its next pointer.
	if (m_pPrev)
	{
		m_pPrev->m_pNext = m_pNext;
	}

	// Update our owner's head pointer, if we are currently
	// the head.
	if (this == m_pOwner->m_pHead)
	{
		m_pOwner->m_pHead = m_pNext;
	}

	// Clear our list pointers.
	m_pPrev.Reset();
	m_pNext.Reset();
	m_pOwner.Reset();
}

} // namespace Seoul::HTTP
