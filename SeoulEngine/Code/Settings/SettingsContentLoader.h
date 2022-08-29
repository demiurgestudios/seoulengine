/**
 * \file SettingsContentLoader.h
 * \brief Handles loading of DataStores from JSON files
 * into SettingsManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SETTINGS_CONTENT_LOADER_H
#define SETTINGS_CONTENT_LOADER_H

#include "ContentLoaderBase.h"
#include "ContentHandle.h"
#include "Settings.h"

namespace Seoul
{

class SettingsContentLoader SEOUL_SEALED : public Content::LoaderBase
{
public:
	// Entry point for synchronous load, special case for WaitOnContent() cases.
	static Content::LoadState SyncLoad(FilePath filePath, const SettingsContentHandle& hEntry);

	SettingsContentLoader(FilePath filePath, const SettingsContentHandle& hEntry);
	virtual ~SettingsContentLoader();

private:
	virtual Content::LoadState InternalExecuteContentLoadOp() SEOUL_OVERRIDE;

	SettingsContentHandle m_hEntry;

	SEOUL_DISABLE_COPY(SettingsContentLoader);
}; // class SettingsContentLoader

} // namespace Seoul

#endif // include guard
