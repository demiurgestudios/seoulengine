/**
* \file EditorMain.cpp
* \brief Root singleton that handles startup of non-engine singletons
* for the Editor.
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#include "Animation3DManager.h"
#include "AnimationNetworkDefinitionManager.h"
#include "ContentLoadManager.h"
#include "EditorMain.h"
#include "EditorUIRoot.h"
#include "Engine.h"
#include "FileManager.h"
#include "FxManager.h"
#if SEOUL_WITH_FX_STUDIO
#include "FxStudioManager.h"
#endif // /#if SEOUL_WITH_FX_STUDIO
#include "GamePaths.h"
#include "ReflectionScriptStub.h"
#include "Renderer.h"
#include "ScenePrefabManager.h"
#include "SoundManager.h"

namespace Seoul::Editor
{

static inline EditorUI::Settings GetEditorUISettings()
{
	EditorUI::Settings ret;

	// Fx variations.
	ret.m_aFxEffectFilePaths[(UInt32)EditorUI::ViewportEffectType::kUnlit] = FilePath::CreateContentFilePath(
		"Authored/Effects/World/FxUnlit.fx");
	ret.m_aFxEffectFilePaths[(UInt32)EditorUI::ViewportEffectType::kWireframe] = FilePath::CreateContentFilePath(
		"Authored/Effects/Editor/FxWireframe.fx");
	ret.m_aFxEffectFilePaths[(UInt32)EditorUI::ViewportEffectType::kMips] = FilePath::CreateContentFilePath(
		"Authored/Effects/Editor/FxMips.fx");
	ret.m_aFxEffectFilePaths[(UInt32)EditorUI::ViewportEffectType::kNormals] = FilePath::CreateContentFilePath(
		"Authored/Effects/Editor/FxNormals.fx");
	ret.m_aFxEffectFilePaths[(UInt32)EditorUI::ViewportEffectType::kOverdraw] = FilePath::CreateContentFilePath(
		"Authored/Effects/Editor/FxOverdraw.fx");

	// Mesh variations.
	ret.m_aMeshEffectFilePaths[(UInt32)EditorUI::ViewportEffectType::kUnlit] = FilePath::CreateContentFilePath(
		"Authored/Effects/Editor/MeshUnlit.fx");
	ret.m_aMeshEffectFilePaths[(UInt32)EditorUI::ViewportEffectType::kWireframe] = FilePath::CreateContentFilePath(
		"Authored/Effects/Editor/MeshWireframe.fx");
	ret.m_aMeshEffectFilePaths[(UInt32)EditorUI::ViewportEffectType::kMips] = FilePath::CreateContentFilePath(
		"Authored/Effects/Editor/MeshMips.fx");
	ret.m_aMeshEffectFilePaths[(UInt32)EditorUI::ViewportEffectType::kNormals] = FilePath::CreateContentFilePath(
		"Authored/Effects/Editor/MeshNormals.fx");
	ret.m_aMeshEffectFilePaths[(UInt32)EditorUI::ViewportEffectType::kOverdraw] = FilePath::CreateContentFilePath(
		"Authored/Effects/Editor/MeshOverdraw.fx");

	// Primitive effect.
	ret.m_PrimitiveEffectFilePath = FilePath::CreateContentFilePath(
		"Authored/Effects/Editor/Primitive.fx");

	return ret;
}

Main::Main()
	: m_pAnimationNetworkDefinitionManager(SEOUL_NEW(MemoryBudgets::Animation) Animation::NetworkDefinitionManager)
#if SEOUL_WITH_ANIMATION_3D
	, m_pAnimation3DManager(SEOUL_NEW(MemoryBudgets::Animation3D) Animation3D::Manager)
#endif // /#if SEOUL_WITH_ANIMATION_3D
#if SEOUL_WITH_FX_STUDIO
	, m_pFxManager(SEOUL_NEW(MemoryBudgets::Fx) FxStudio::Manager)
#else
	, m_pFxManager(SEOUL_NEW(MemoryBudgets::Fx) NullFxManager)
#endif
#if SEOUL_WITH_SCENE
	, m_pScenePrefabManager(SEOUL_NEW(MemoryBudgets::Scene) Scene::PrefabManager)
#endif // /#if SEOUL_WITH_SCENE
	, m_pEditorUIRoot(SEOUL_NEW(MemoryBudgets::Editor) EditorUI::Root(GetEditorUISettings()))
{
	// Set the ContentLoadManager to permanent accept in the editor.
	Content::LoadManager::Get()->SetHoadLoadMode(Content::LoadManagerHotLoadMode::kPermanentAccept);

	// Setup the renderer.
	Renderer::Get()->ReadConfiguration(
		FilePath::CreateConfigFilePath(String::Printf("Renderer/Renderer%s.json", GetCurrentPlatformName())),
		HString("DefaultConfig"));
}

Main::~Main()
{
	// Disable network file IO before further processing, we don't want
	// calls to WaitUntilAllLoadsAreFinished() to content manager with
	// network file IO still active.
	FileManager::Get()->DisableNetworkFileIO();

	// Wait for content loads to finish, make sure
	// content references are free before shutdown.
	Content::LoadManager::Get()->WaitUntilAllLoadsAreFinished();

	// Shutdown the renderer.
	Renderer::Get()->ClearConfiguration();

	// Shutdown UI.
	m_pEditorUIRoot.Reset();
}

Bool Main::Tick()
{
	// Return value from possible branches below.
	Bool bTickResult = false;

	// Tick Engine systems.
	if (Engine::Get()->Tick())
	{
		// TODO: Push these steps into Engine::Get()->Tick() or
		// otherwise eliminate this boilerplate.
		Float const fDeltaTimeInSeconds = Engine::Get()->GetSecondsInTick();

		FxManager::Get()->Tick(fDeltaTimeInSeconds);
		Renderer::Get()->Pose(fDeltaTimeInSeconds);
		Sound::Manager::Get()->Tick(fDeltaTimeInSeconds);
		Renderer::Get()->Render(fDeltaTimeInSeconds);

		return true;
	}
	else
	{
		return false;
	}
}

} // namespace Seoul::Editor
