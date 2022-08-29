/**
 * \file SceneTicker.cpp
 * \brief Utility that handles update of components and systems
 * in a 3D scene that need per-frame ticking.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "SceneAnimation3DComponent.h"
#include "SceneFxComponent.h"
#include "SceneObject.h"
#include "ScenePrefabComponent.h"
#include "SceneRigidBodyComponent.h"
#include "SceneTicker.h"

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

Ticker::Ticker()
{
}

Ticker::~Ticker()
{
}

void Ticker::Tick(
	Interface& rInterface,
	const Objects& vObjects,
	Float fDeltaTimeInSeconds)
{
	// TODO: Track tickable Objets in a separate list to minimize
	// duration time here.
	// TODO: Explicit enumeration here is brittle and not a longterm
	// solution.

	UInt32 const uObjects = vObjects.GetSize();
	for (UInt32 i = 0u; i < uObjects; ++i)
	{
		Object& object = *vObjects[i];

		{
			auto p(object.GetComponent<Animation3DComponent>());
			if (p.IsValid())
			{
				p->Tick(fDeltaTimeInSeconds);
			}
		}
		{
			auto p(object.GetComponent<FxComponent>());
			if (p.IsValid())
			{
				p->Tick(fDeltaTimeInSeconds);
			}
		}

		// TODO: This will never appear at runtime, so checking for it
		// is needless overhead.

		{
			auto p(object.GetComponent<PrefabComponent>());
			if (p.IsValid())
			{
#if SEOUL_HOT_LOADING
				p->CheckHotLoad();
#endif
				Tick(rInterface, p->GetObjects(), fDeltaTimeInSeconds);
			}
		}
	}
}

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE
