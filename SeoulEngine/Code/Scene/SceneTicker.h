/**
 * \file SceneTicker.h
 * \brief Utility that handles update of components and systems
 * in a 3D scene that need per-frame ticking.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_TICKER_H
#define SCENE_TICKER_H

#include "SharedPtr.h"
#include "Prereqs.h"
#include "Vector.h"
namespace Seoul { namespace Scene { class Interface; } }
namespace Seoul { namespace Scene { class Object; } }

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

class Ticker SEOUL_SEALED
{
public:
	typedef Vector<SharedPtr<Object>, MemoryBudgets::SceneObject> Objects;

	Ticker();
	~Ticker();

	void Tick(
		Interface& rInterface,
		const Objects& vObjects,
		Float fDeltaTimeInSeconds);

private:
	SEOUL_DISABLE_COPY(Ticker);
}; // class Ticker

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
