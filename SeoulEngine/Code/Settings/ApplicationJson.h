/**
 * \file ApplicationJson.h
 * \brief Helper method for pulling keys out of application.json
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef APPLICATION_JSON_H
#define APPLICATION_JSON_H

#include "DataStore.h"
#include "GamePaths.h"
#include "Logger.h"
#include "ReflectionDataStoreTableUtil.h"
#include "SeoulHString.h"
#include "SettingsManager.h"
#include "SharedPtr.h"

namespace Seoul
{

	/** Utility to get a particular key-value pair from application.json. */
template <typename T>
inline static Bool GetApplicationJsonValue(HString sName, T& rValue)
{
	static const HString ksApplication("Application");

	SharedPtr<DataStore> pDataStore(SettingsManager::Get()->WaitForSettings(
		GamePaths::Get()->GetApplicationJsonFilePath()));
	if (pDataStore.IsValid())
	{
		DataStoreTableUtil applicationSection(*pDataStore, ksApplication);
		return applicationSection.GetValue(sName, rValue);
	}

	SEOUL_WARN(
		"Could not load %s",
		GamePaths::Get()->GetApplicationJsonFilePath().GetAbsoluteFilenameInSource().CStr());
	return false;
}

} // namespace Seoul


#endif  // include guard
