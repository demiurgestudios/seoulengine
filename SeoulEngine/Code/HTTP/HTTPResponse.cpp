/**
 * \file HTTPResponse.cpp
 * \brief Class that encapsulates the results of an HTTP::Request client request.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HTTPResponse.h"

namespace Seoul::HTTP
{

Response::Response()
	: m_UptimeValueAtReceive(0)
	, m_nStatus(-1)
	, m_bBodyFileWrittenSuccessfully(false)
	, m_pBody(nullptr)
	, m_zBodySize(0u)
	, m_zBodyCapacity(0u)
	, m_bOwnsBody(true)
	, m_bBodyDataTruncated(false)
{
}

Response::~Response()
{
	DeallocateBody();
}

void Response::DeallocateBody()
{
	// Zero out size.
	m_zBodySize = 0u;

	// No longer truncated.
	m_bBodyDataTruncated = false;

	// Deallocate the data if we own it.
	if (m_bOwnsBody)
	{
		// Zero out capacity.
		m_zBodyCapacity = 0u;

		MemoryManager::Deallocate(m_pBody);
		m_pBody = nullptr;
	}
	// If we don't own the data, leave capacity
	// and body pointer alone.
}

/**
 * Add pData to the buffer and return true, or return false if the data
 * was truncated to the maximum size of the buffer, if the buffer
 * used by this HTTP::Response is not owned.
 */
Bool Response::AppendData(void const* pData, size_t zInDataSizeInBytes)
{
	// Truncate to a UInt32
	UInt32 zDataSizeInBytes = (UInt32)zInDataSizeInBytes;

	// Whether the entire data was consumed or not.
	Bool bReturn = true;

	// Increase capacity.
	if (m_zBodySize + zDataSizeInBytes > m_zBodyCapacity)
	{
		// Truncate the data if we don't own the buffer and don't have enough room for all of it.
		if (!m_bOwnsBody)
		{
			// Sanity check.
			SEOUL_ASSERT(m_zBodyCapacity >= m_zBodySize);

			// Use whatever space is remaining.
			zDataSizeInBytes = (m_zBodyCapacity - m_zBodySize);

			// Data was truncated.
			bReturn = false;
		}
		// Otherwise, increase the capacity, using roughly the same algorithm as Vector<> (double the current capacity or enough to contain pData)
		else
		{
			m_zBodyCapacity = Max(m_zBodySize + zDataSizeInBytes, 2u * m_zBodyCapacity);
			m_pBody = MemoryManager::Reallocate(m_pBody, m_zBodyCapacity, MemoryBudgets::Network);
		}
	}

	// If we have data to copy, do so now.
	if (zDataSizeInBytes > 0u)
	{
		memcpy((Byte*)m_pBody + m_zBodySize, pData, zDataSizeInBytes);
		m_zBodySize += zDataSizeInBytes;
	}

	return bReturn;
}

void Response::SetBodyOutputBuffer(void* pBuffer, UInt32 uBufferSizeInBytes)
{
	// Release existing state.
	DeallocateBody();

	// Sanitize.
	if (nullptr == pBuffer) { uBufferSizeInBytes = 0u; }

	// Populate the response with the new data, and mark that
	// it no longer owns the buffer.
	m_pBody = pBuffer;
	m_zBodySize = 0u;
	m_zBodyCapacity = uBufferSizeInBytes;
	m_bOwnsBody = false;
}

} // namespace Seoul::HTTP
