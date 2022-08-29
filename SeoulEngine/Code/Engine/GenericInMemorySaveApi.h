/**
 * \file GenericInMemorySaveApi.h
 * \brief GenericInMemorySaveApi provides save/loading support
 * that is volatile and in-memory only. Useful to provide persistence
 * to save/load backing that does not persist across runs of the app.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GENERIC_IN_MEMORY_SAVE_API_H
#define GENERIC_IN_MEMORY_SAVE_API_H

#include "HashTable.h"
#include "SharedPtr.h"
#include "Mutex.h"
#include "SaveApi.h"
#include "StreamBuffer.h"

namespace Seoul
{

class GenericInMemorySaveApiSharedMemory SEOUL_SEALED
{
public:
	typedef HashTable<FilePath, StreamBuffer*, MemoryBudgets::Saving> Buffers;

	GenericInMemorySaveApiSharedMemory()
		: m_tBuffers()
		, m_Mutex()
	{
	}

	~GenericInMemorySaveApiSharedMemory()
	{
		SafeDeleteTable(m_tBuffers);
	}

	/**
	 * Populate rBuffer from the current state of this internal buffer,
	 * or return false if no data is available.
	 */
	Bool Load(FilePath filePath, StreamBuffer& rBuffer)
	{
		Lock lock(m_Mutex);

		StreamBuffer* pBuffer = nullptr;
		if (m_tBuffers.GetValue(filePath, pBuffer))
		{
			StreamBuffer bufferCopy;
			bufferCopy.CopyFrom(*pBuffer);
			rBuffer.Swap(bufferCopy);
			rBuffer.SeekToOffset(0u);
			return true;
		}

		return false;
	}

	void Save(FilePath filePath, const StreamBuffer& buffer)
	{
		Lock lock(m_Mutex);

		StreamBuffer* pBuffer = nullptr;
		if (!m_tBuffers.GetValue(filePath, pBuffer))
		{
			pBuffer = SEOUL_NEW(MemoryBudgets::Io) StreamBuffer;
			SEOUL_VERIFY(m_tBuffers.Overwrite(filePath, pBuffer).Second);
		}

		StreamBuffer bufferCopy;
		bufferCopy.CopyFrom(buffer);
		pBuffer->Swap(bufferCopy);
		pBuffer->SeekToOffset(0u);
	}

private:
	SEOUL_REFERENCE_COUNTED(GenericInMemorySaveApiSharedMemory);

	Buffers m_tBuffers;
	Mutex m_Mutex;

	SEOUL_DISABLE_COPY(GenericInMemorySaveApiSharedMemory);
}; // class GenericInMemorySaveApiSharedMemory

class GenericInMemorySaveApi SEOUL_SEALED : public SaveApi
{
public:
	GenericInMemorySaveApi(const SharedPtr<GenericInMemorySaveApiSharedMemory>& pSharedMemory);
	~GenericInMemorySaveApi();

	virtual SaveLoadResult Load(FilePath filePath, StreamBuffer& rData) const SEOUL_OVERRIDE;
	virtual SaveLoadResult Save(FilePath filePath, const StreamBuffer& data) const SEOUL_OVERRIDE;

private:
	SharedPtr<GenericInMemorySaveApiSharedMemory> m_pSharedMemory;

	SEOUL_DISABLE_COPY(GenericInMemorySaveApi);
}; // class GenericInMemorySaveApi

} // namespace Seoul

#endif // include guard
