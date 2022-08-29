/**
 * \file ScriptEnginePlatformSignInManager.cpp
 * \brief Binder instance for exposing the PlatformSignInManager Singleton into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "PlatformSignInManager.h"
#include "ReflectionDefine.h"
#include "ScriptEnginePlatformSignInManager.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEnginePlatformSignInManager, TypeFlags::kDisableCopy)
	SEOUL_METHOD(GetStateChangeCount)
	SEOUL_METHOD(IsSignedIn)
	SEOUL_METHOD(IsSigningIn)
	SEOUL_METHOD(IsSignInSupported)
	SEOUL_METHOD(SignIn)
	SEOUL_METHOD(SignOut)
SEOUL_END_TYPE()

ScriptEnginePlatformSignInManager::ScriptEnginePlatformSignInManager()
{
}

ScriptEnginePlatformSignInManager::~ScriptEnginePlatformSignInManager()
{
}

Int32 ScriptEnginePlatformSignInManager::GetStateChangeCount() const
{
	return PlatformSignInManager::Get()->GetStateChangeCount();
}

Bool ScriptEnginePlatformSignInManager::IsSignedIn() const
{
	return PlatformSignInManager::Get()->IsSignedIn();
}

Bool ScriptEnginePlatformSignInManager::IsSigningIn() const
{
	return PlatformSignInManager::Get()->IsSigningIn();
}

Bool ScriptEnginePlatformSignInManager::IsSignInSupported() const
{
	return PlatformSignInManager::Get()->IsSignInSupported();
}

void ScriptEnginePlatformSignInManager::SignIn()
{
	return PlatformSignInManager::Get()->SignIn();
}

void ScriptEnginePlatformSignInManager::SignOut()
{
	return PlatformSignInManager::Get()->SignOut();
}

} // namespace Seoul
