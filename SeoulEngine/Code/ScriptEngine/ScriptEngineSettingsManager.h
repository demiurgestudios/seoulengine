/**
 * \file ScriptEngineSettingsManager.h
 * \brief Binder instance for exposing the global SettingsManager
 * to script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_SETTINGS_MANAGER_H
#define SCRIPT_ENGINE_SETTINGS_MANAGER_H

#include "Prereqs.h"
namespace Seoul { namespace Script { class FunctionInterface; } }

namespace Seoul
{

/** Binder, wraps a Camera instance and exposes functionality to a script VM. */
class ScriptEngineSettingsManager SEOUL_SEALED
{
public:
	ScriptEngineSettingsManager();
	~ScriptEngineSettingsManager();

	void Exists(Script::FunctionInterface* pInterface) const;
	void GetSettings(Script::FunctionInterface* pInterface) const;
	void GetSettingsAsJsonString(Script::FunctionInterface* pInterface) const;
	void GetSettingsInDirectory(Script::FunctionInterface* pInterface) const;
	void SetSettings(Script::FunctionInterface* pInterface) const;
#if !SEOUL_SHIP
	void ValidateSettings(Script::FunctionInterface* pInterface) const;
#endif // /#if !SEOUL_SHIP

private:
	SEOUL_DISABLE_COPY(ScriptEngineSettingsManager);
}; // struct ScriptEngineSettingsManager

} // namespace Seoul

#endif // include guard
