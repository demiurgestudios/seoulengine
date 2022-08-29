/**
 * \file EditorUIModelScene.cpp
 * \brief A model implementation that encapsulates
 * state of a scene for editing purposes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorSceneContainer.h"
#include "EditorUIModelScene.h"
#include "ReflectionDefine.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(EditorUI::ModelScene, TypeFlags::kDisableNew)
	SEOUL_PARENT(EditorUI::IModel)
SEOUL_END_TYPE()

namespace EditorUI
{

static inline EditorScene::Settings ToEditorSceneSettings(
	const Settings& in,
	FilePath rootScenePrefabFilePath)
{
	EditorScene::Settings ret;
	ret.m_RootScenePrefabFilePath = rootScenePrefabFilePath;
	return ret;
}

ModelScene::ModelScene(
	const Settings& settings,
	FilePath rootScenePrefabFilePath)
	: m_Settings(settings)
	, m_pScene(SEOUL_NEW(MemoryBudgets::Scene) EditorScene::Container(ToEditorSceneSettings(settings, rootScenePrefabFilePath)))
{
}

ModelScene::~ModelScene()
{
}

Bool ModelScene::IsLoading() const
{
	return m_pScene->IsLoading();
}

Bool ModelScene::IsOutOfDate() const
{
	return m_pScene->IsOutOfDate();
}

void ModelScene::MarkUpToDate()
{
	m_pScene->MarkUpToDate();
}

void ModelScene::Tick(Float fDeltaTimeInSeconds)
{
	m_pScene->Tick(fDeltaTimeInSeconds);
}

} // namespace EditorUI

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
