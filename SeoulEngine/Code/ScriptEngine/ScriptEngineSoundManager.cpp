/**
 * \file ScriptEngineSoundManager.cpp
 * \brief Binder instance for exposing the Sound::Manager Singleton into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "ScriptEngineSoundManager.h"
#include "SoundManager.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngineSoundManager, TypeFlags::kDisableCopy)
	SEOUL_METHOD(IsCategoryPlaying)
	SEOUL_METHOD(SetCategoryMute)
	SEOUL_METHOD(SetCategoryVolume)
	SEOUL_METHOD(GetCategoryVolume)
SEOUL_END_TYPE()

ScriptEngineSoundManager::ScriptEngineSoundManager()
{
}

ScriptEngineSoundManager::~ScriptEngineSoundManager()
{
}

Bool ScriptEngineSoundManager::IsCategoryPlaying(HString categoryName, Bool bIncludeLoopingSounds) const
{
	return Sound::Manager::Get()->IsCategoryPlaying(categoryName, bIncludeLoopingSounds);
}

Bool ScriptEngineSoundManager::SetCategoryMute(HString hsCategoryName, Bool bMute, Bool bAllowPending, Bool bSuppressLogging)
{
	return Sound::Manager::Get()->SetCategoryMute(hsCategoryName, bMute, bAllowPending, bSuppressLogging);
}

Bool ScriptEngineSoundManager::SetCategoryVolume(HString hsCategoryName, Float fVolume, Float fFadeTimeInSeconds, Bool bAllowPending, Bool bSuppressLogging)
{
	return Sound::Manager::Get()->SetCategoryVolume(hsCategoryName, fVolume, fFadeTimeInSeconds, bAllowPending, bSuppressLogging);
}

Float ScriptEngineSoundManager::GetCategoryVolume(HString hsCategoryName) const
{
	return Sound::Manager::Get()->GetCategoryVolume(hsCategoryName);
}

} // namespace Seoul
