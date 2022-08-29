/**
 * \file Settings.h
 * \brief Specialization of Content::Traits<> for DataStore, so it can be used
 * in SettingsManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SETTINGS_H
#define SETTINGS_H

#include "ContentHandle.h"
#include "ContentTraits.h"
#include "DataStore.h"
#include "FilePath.h"

namespace Seoul
{

namespace Content
{

template <>
struct Traits<DataStore>
{
	typedef FilePath KeyType;
	static const Bool CanSyncLoad = true;

	static SharedPtr<DataStore> GetPlaceholder(FilePath filePath);
	static Bool FileChange(FilePath filePath, const Handle<DataStore>& pEntry);
	static void Load(FilePath filePath, const Handle<DataStore>& pEntry);
	static Bool PrepareDelete(FilePath filePath, Entry<DataStore, KeyType>& entry);
	static void SyncLoad(FilePath filePath, const Handle<DataStore>& hEntry);
	static UInt32 GetMemoryUsage(const SharedPtr<DataStore>& p);
}; // struct Content::Traits<DataStore>

} // namespace Content

typedef Content::Handle<DataStore> SettingsContentHandle;

} // namespace Seoul

#endif // include guard
