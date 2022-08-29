/**
 * \file EditorUIModelScene.h
 * \brief A model implementation that encapsulates
 * state of a scene for editing purposes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDTIOR_MODEL_SCENE_H
#define EDTIOR_MODEL_SCENE_H

#include "EditorUIIModel.h"
#include "EditorUISettings.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
namespace Seoul { namespace EditorUI { class SceneState; } }
namespace Seoul { namespace EditorUI { class SceneStateLoadJob; } }
namespace Seoul { namespace EditorScene { class Container; } }
namespace Seoul { class SceneTicker; }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

class ModelScene SEOUL_SEALED : public IModel
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ModelScene);

	ModelScene(
		const Settings& settings,
		FilePath rootScenePrefabFilePath);
	~ModelScene();

	const EditorScene::Container& GetScene() const { return *m_pScene; }
	      EditorScene::Container& GetScene()       { return *m_pScene; }

	const Settings& GetSettings() const
	{
		return m_Settings;
	}

	Bool IsLoading() const;

	Bool IsOutOfDate() const;

	void MarkUpToDate();

	void Tick(Float fDeltaTimeInSeconds);

private:
	Settings const m_Settings;
	ScopedPtr<EditorScene::Container> m_pScene;

	SEOUL_DISABLE_COPY(ModelScene);
}; // class ModelScene

} // namespace Seoul::EditorUI

#endif // #if SEOUL_WITH_SCENE

#endif // include guard
