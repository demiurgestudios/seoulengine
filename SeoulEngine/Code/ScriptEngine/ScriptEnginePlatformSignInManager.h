/**
 * \file ScriptEnginePlatformSignInManager.h
 * \brief Binder instance for exposing the PlatformSignInManager Singleton into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_PLATFORM_SIGN_IN_MANAGER_H
#define SCRIPT_ENGINE_PLATFORM_SIGN_IN_MANAGER_H

#include "Prereqs.h"

namespace Seoul
{

/** Binder, wraps a Camera instance and exposes functionality to a script VM. */
class ScriptEnginePlatformSignInManager SEOUL_SEALED
{
public:
	ScriptEnginePlatformSignInManager();
	~ScriptEnginePlatformSignInManager();

	Int32 GetStateChangeCount() const;
	Bool IsSignedIn() const;
	Bool IsSigningIn() const;
	Bool IsSignInSupported() const;
	void SignIn();
	void SignOut();

private:
	SEOUL_DISABLE_COPY(ScriptEnginePlatformSignInManager);
}; // class ScriptEnginePlatformSignInManager

} // namespace Seoul

#endif // include guard
