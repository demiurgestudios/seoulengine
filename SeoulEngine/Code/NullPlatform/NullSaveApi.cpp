/**
 * \file NullSaveApi.cpp
 * \brief Specialization of SaveApi for platform independent
 * contexts (headless mode and tools).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "NullSaveApi.h"

namespace Seoul
{

NullSaveApi::NullSaveApi()
{
}

NullSaveApi::~NullSaveApi()
{
}

SaveLoadResult NullSaveApi::Load(FilePath filePath, StreamBuffer& rData) const
{
	return SaveLoadResult::kErrorFileNotFound;
}

SaveLoadResult NullSaveApi::Save(FilePath filePath, const StreamBuffer& data) const
{
	return SaveLoadResult::kSuccess;
}

} // namespace Seoul
