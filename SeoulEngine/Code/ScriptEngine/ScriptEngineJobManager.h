/**
 * \file ScriptEngineJobManager.h
 * \brief Binder instance for exposing the Jobs::Manager Singleton into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_JOB_MANAGER_H
#define SCRIPT_ENGINE_JOB_MANAGER_H

#include "Prereqs.h"
#include "Vector.h"
namespace Seoul { namespace Script { class FunctionInterface; } }
namespace Seoul { class String; }

namespace Seoul
{

/** Binder, wraps a Camera instance and exposes functionality to a script VM. */
class ScriptEngineJobManager SEOUL_SEALED
{
public:
	ScriptEngineJobManager() = default;
	~ScriptEngineJobManager() = default;

	Bool YieldThreadTime();

private:
	SEOUL_DISABLE_COPY(ScriptEngineJobManager);
}; // class ScriptEngineJobManager

} // namespace Seoul

#endif // include guard
