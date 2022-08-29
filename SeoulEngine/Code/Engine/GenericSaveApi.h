/**
 * \file GenericSaveApi.h
 * \brief GenericSaveApi is a concrete implementation of SaveApi
 * that writes files to disk using atomic semantics.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GENERIC_SAVE_API_H
#define GENERIC_SAVE_API_H

#include "SaveApi.h"

namespace Seoul
{

class GenericSaveApi : public SaveApi
{
public:
	GenericSaveApi()
	{
	}

	virtual ~GenericSaveApi()
	{
	}

	virtual SaveLoadResult Load(FilePath filePath, StreamBuffer& rData) const SEOUL_OVERRIDE;
	virtual SaveLoadResult Save(FilePath filePath, const StreamBuffer& data) const SEOUL_OVERRIDE;

private:
	static void InternalCreateSaveDirectories(const String& sPath);

	SEOUL_DISABLE_COPY(GenericSaveApi);
}; // class GenericSaveApi

} // namespace Seoul

#endif // include guard
