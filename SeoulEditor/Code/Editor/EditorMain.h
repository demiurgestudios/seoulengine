/**
 * \file EditorMain.h
 * \brief Root singleton that handles startup of non-engine singletons
 * for the Editor.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_MAIN_H
#define EDITOR_MAIN_H

#include "Prereqs.h"
#include "ScopedPtr.h"
#include "Singleton.h"
namespace Seoul { namespace EditorUI { class Root; } }
namespace Seoul { class FxManager; }
namespace Seoul { namespace Animation { class NetworkDefinitionManager; } }
namespace Seoul { namespace Animation3D { class Manager; } }
namespace Seoul { namespace Scene { class PrefabManager; } }

namespace Seoul::Editor
{

class Main SEOUL_SEALED : public Singleton<Main>
{
public:
	Main();
	~Main();

	// Call to run 1 frame of the game loop on the main thread. Returns
	// true if the game has not been shutdown, false otherwise.
	Bool Tick();

	/**
	 * \brief Convenience function for platforms that use a traditional game poll
	 * loop.
	 */
	void Run()
	{
		while (Tick());
	}

private:
	ScopedPtr<Animation::NetworkDefinitionManager> m_pAnimationNetworkDefinitionManager;
#if SEOUL_WITH_ANIMATION_3D
	ScopedPtr<Animation3D::Manager> m_pAnimation3DManager;
#endif // /#if SEOUL_WITH_ANIMATION_3D
	ScopedPtr<FxManager> m_pFxManager;
#if SEOUL_WITH_SCENE
	ScopedPtr<Scene::PrefabManager> m_pScenePrefabManager;
#endif // /#if SEOUL_WITH_SCENE
	ScopedPtr<EditorUI::Root> m_pEditorUIRoot;

	SEOUL_DISABLE_COPY(Main);
}; // class Main

} // namespace Seoul::Editor

#endif // include guard
