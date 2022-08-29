/**
 * \file GenericInMemorySaveApi.cpp
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

#include "GenericInMemorySaveApi.h"
#include "JobsFunction.h"

namespace Seoul
{

GenericInMemorySaveApi::GenericInMemorySaveApi(
	const SharedPtr<GenericInMemorySaveApiSharedMemory>& pSharedMemory)
	: m_pSharedMemory(pSharedMemory)
{
}

GenericInMemorySaveApi::~GenericInMemorySaveApi()
{
}

/**
 * Perform the actual data load.
 */
SaveLoadResult GenericInMemorySaveApi::Load(FilePath filePath, StreamBuffer& rData) const
{
	// Do not have data to return, set kErrorFileNotFound
	if (!m_pSharedMemory->Load(filePath, rData))
	{
		return SaveLoadResult::kErrorFileNotFound;
	}
	else
	{
		return SaveLoadResult::kSuccess;
	}
}

/**
 * Perform the actual data save.
 */
SaveLoadResult GenericInMemorySaveApi::Save(FilePath filePath, const StreamBuffer& data) const
{
	// Save the data - this always succeeds.
	m_pSharedMemory->Save(filePath, data);

	// Result is always kSuccess.
	return SaveLoadResult::kSuccess;
}

} // namespace Seoul
