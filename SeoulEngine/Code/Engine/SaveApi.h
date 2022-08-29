/**
 * \file SaveApi.h
 * \brief SaveApi encapsulates platform dependent storage for save-game
 * style files.
 *
 * SaveApi can wrap a standard disk write, or it can wrap proprietary platform
 * storage.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SAVE_API_H
#define SAVE_API_H

#include "FilePath.h"
#include "SaveLoadResult.h"
namespace Seoul { class StreamBuffer; }

namespace Seoul
{

/**
 * Intended to be a thread-safe, global interface. Implementations must
 * either store no state, or store the state such that Load() and Save()
 * can be called from multiple threads simultaneously.
 */
class SaveApi SEOUL_ABSTRACT
{
public:
	virtual ~SaveApi()
	{
	}

	virtual SaveLoadResult Load(FilePath filePath, StreamBuffer& rData) const = 0;
	virtual SaveLoadResult Save(FilePath filePath, const StreamBuffer& data) const = 0;

protected:
	SaveApi()
	{
	}

private:
	SEOUL_DISABLE_COPY(SaveApi);
}; // class SaveApi

} // namespace Seoul

#endif // include guard
