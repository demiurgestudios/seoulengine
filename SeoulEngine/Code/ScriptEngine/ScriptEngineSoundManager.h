/**
 * \file ScriptEngineSoundManager.h
 * \brief Binder instance for exposing the Sound::Manager Singleton into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_SOUND_MANAGER_H
#define SCRIPT_ENGINE_SOUND_MANAGER_H

#include "SeoulHString.h"

namespace Seoul
{

/** Binder, wraps a Camera instance and exposes functionality to a script VM. */
class ScriptEngineSoundManager SEOUL_SEALED
{
public:
	ScriptEngineSoundManager();
	~ScriptEngineSoundManager();

	Bool IsCategoryPlaying(HString categoryName, Bool bIncludeLoopingSounds) const;
	Bool SetCategoryMute(HString hsCategoryName, Bool bMute, Bool bAllowPending, Bool bSuppressLogging);

	Bool SetCategoryVolume(HString hsCategoryName, Float fVolume, Float fFadeTimeInSeconds, Bool bAllowPending, Bool bSuppressLogging);
	Float GetCategoryVolume(HString hsCategoryName) const;

private:
	SEOUL_DISABLE_COPY(ScriptEngineSoundManager);
}; // class ScriptEngineSoundManager

} // namespace Seoul

#endif // include guard
