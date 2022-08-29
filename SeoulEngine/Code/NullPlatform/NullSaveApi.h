/**
 * \file NullSaveApi.h
 * \brief Specialization of SaveApi for platform independent
 * contexts (headless mode and tools).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NULL_SAVE_API_H
#define NULL_SAVE_API_H

#include "SaveApi.h"

namespace Seoul
{

class NullSaveApi SEOUL_SEALED : public SaveApi
{
public:
	NullSaveApi();
	~NullSaveApi();

	virtual SaveLoadResult Load(FilePath filePath, StreamBuffer& rData) const SEOUL_OVERRIDE;
	virtual SaveLoadResult Save(FilePath filePath, const StreamBuffer& data) const SEOUL_OVERRIDE;

private:
	SEOUL_DISABLE_COPY(NullSaveApi);
}; // class NullSaveApi

} // namespace Seoul

#endif // include guard
