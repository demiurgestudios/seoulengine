/**
 * \file ScriptEngineRenderer.h
 * \brief Binder instance for exposing Renderer functionality into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_RENDERER_H
#define SCRIPT_ENGINE_RENDERER_H

#include "Prereqs.h"
namespace Seoul { namespace Script { class FunctionInterface; } }

namespace Seoul
{

class ScriptEngineRenderer SEOUL_SEALED
{
public:
	ScriptEngineRenderer();
	~ScriptEngineRenderer();

	// Return the back buffer viewport aspect ratio.
	Float GetViewportAspectRatio() const;
	// Return the back buffer viewport dimensions.
	void GetViewportDimensions(Script::FunctionInterface* pInterface) const;
	void PrefetchFx(Script::FunctionInterface* pInterface) const;

private:
	SEOUL_DISABLE_COPY(ScriptEngineRenderer);
}; // class ScriptEngineRenderer

} // namespace Seoul

#endif // include guard
