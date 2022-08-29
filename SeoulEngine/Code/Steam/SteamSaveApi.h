/**
 * \file SteamSaveApi.h
 * \brief Interface for using Steam cloud saving.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef STEAM_SAVE_API_H
#define STEAM_SAVE_API_H

#include "GenericSaveApi.h"

#if SEOUL_WITH_STEAM

namespace Seoul
{

class SteamSaveApi SEOUL_SEALED : public GenericSaveApi
{
public:
	SteamSaveApi();
	~SteamSaveApi();

	virtual SaveLoadResult::Enum Load(FilePath filePath, StreamBuffer& rData) const SEOUL_OVERRIDE;
	virtual SaveLoadResult::Enum Save(FilePath filePath, const StreamBuffer& data) const SEOUL_OVERRIDE;

private:
	SEOUL_DISABLE_COPY(SteamSaveApi);
}; // class SteamSaveApi

} // namespace Seoul

#endif // /#if SEOUL_WITH_STEAM

#endif // include guard
