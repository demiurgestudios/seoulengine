/**
 * \file GameSceneMovie.cpp
 * \brief UI::Movie that owns a ScriptScene which binds
 * a 3D scene into the UI state machine.
 *
 * Game::SceneMovie is a non-Falcon UI::Movie which exists as a state to
 * control the lifespan of a 3D scene.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconMovieClipInstance.h"
#include "GameSceneMovie.h"
#include "GameMain.h"
#include "ReflectionDefine.h"
#include "ScriptScene.h"
#include "UIHitShapeInstance.h"
#include "UIManager.h"
#include "UIRenderer.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(Game::SceneMovie, TypeFlags::kDisableCopy)
	SEOUL_PARENT(UI::Movie)

	SEOUL_PROPERTY_N("FxEffectFilePath", m_FxEffectFilePath)
	SEOUL_PROPERTY_N("MeshEffectFilePath", m_MeshEffectFilePath)
	SEOUL_PROPERTY_N("RootScenePrefabFilePath", m_RootScenePrefabFilePath)
	SEOUL_PROPERTY_N("ScriptMainRelativeFilename", m_sScriptMainRelativeFilename)
SEOUL_END_TYPE()

namespace Game
{

/** Events dispatched through SendGameEvent for mouse events that fall through. */
static const HString kOnMouseButtonPressed("OnMouseButtonPressed");
static const HString kOnMouseButtonReleased("OnMouseButtonReleased");

/** Special event name that we capture, and forward to the scene. */
static const HString kSendGameEvent("SendGameEvent");

SceneMovie::SceneMovie()
	: UI::Movie()
	, m_FxEffectFilePath()
	, m_MeshEffectFilePath()
	, m_RootScenePrefabFilePath()
	, m_sScriptMainRelativeFilename()
	, m_pScriptScene()
{
}

SceneMovie::~SceneMovie()
{
}

void SceneMovie::InternalRender(RenderPass& rPass, RenderCommandStreamBuilder& rBuilder)
{
	if (m_pScriptScene.IsValid())
	{
		m_pScriptScene->Render(rPass, rBuilder);
	}
}

// TODO: Eliminate, OnHitTest is only necessary
// to work around the fact that this type of movie
// has no FCN file.
UI::MovieHitTestResult SceneMovie::OnHitTest(
	UInt8 uMask,
	const Point2DInt& mousePosition,
	UI::Movie*& rpHitMovie,
	SharedPtr<Falcon::MovieClipInstance>& rpHitInstance,
	SharedPtr<Falcon::Instance>& rpLeafInstance,
	Vector<UI::Movie*>* rvPassthroughInputs) const
{
	if (!m_pScriptScene.IsValid())
	{
		return UI::MovieHitTestResult::kNoHit;
	}

	SharedPtr<Falcon::MovieClipInstance> pRoot;
	SEOUL_VERIFY(GetRootMovieClip(pRoot));

	SharedPtr<Falcon::Instance> p;
	SEOUL_VERIFY(pRoot->GetChildAt(0, p));

	rpHitMovie = const_cast<SceneMovie*>(this);
	rpHitInstance = pRoot;
	rpLeafInstance = p;
	return UI::MovieHitTestResult::kHit;
}

void SceneMovie::OnMouseButtonPressed(
	const Point2DInt& mousePosition,
	const SharedPtr<Falcon::MovieClipInstance>& pInstance,
	Bool bInInstance)
{
	if (m_pScriptScene.IsValid())
	{
		Reflection::MethodArguments aArguments;
		aArguments[0] = kOnMouseButtonPressed;
		aArguments[1] = mousePosition.X;
		aArguments[2] = mousePosition.Y;
		aArguments[3] = bInInstance;
		m_pScriptScene->SendEvent(aArguments, 4);
	}
}

void SceneMovie::OnMouseButtonReleased(
	const Point2DInt& mousePosition,
	const SharedPtr<Falcon::MovieClipInstance>& pInstance,
	Bool bInInstance,
	UInt8 uInputCaptureHitTestMask)
{
	if (m_pScriptScene.IsValid())
	{
		Reflection::MethodArguments aArguments;
		aArguments[0] = kOnMouseButtonReleased;
		aArguments[1] = mousePosition.X;
		aArguments[2] = mousePosition.Y;
		aArguments[3] = bInInstance;
		m_pScriptScene->SendEvent(aArguments, 4);
	}
}

/** Custom hook for starting async load of the combat scene. */
void SceneMovie::OnLoad()
{
	UI::Movie::OnLoad();

	// Setup a hit area so that scene's get input if the user doesn't tap
	// on anything else.
	{
		SharedPtr<Falcon::MovieClipInstance> pRoot;
		SEOUL_VERIFY(GetRootMovieClip(pRoot));

		auto const viewport = GetViewport();
		Falcon::Rectangle const stageBounds(Falcon::Rectangle::Create(0, (Float32)viewport.m_iViewportWidth, 0, (Float32)viewport.m_iViewportHeight));
		SharedPtr<UI::HitShapeInstance> pHit(SEOUL_NEW(MemoryBudgets::UIRuntime) UI::HitShapeInstance(stageBounds));
		pRoot->SetChildAtDepth(*this, 1u, pHit);

		pRoot->SetHitTestChildrenMask(0);
		pRoot->SetHitTestSelfMask(0xFF);
	}

	// Destroy any existing scene.
	m_pScriptScene.Reset();

	// Populate settings.
	ScriptSceneSettings settings;
	settings.m_FxEffectFilePath = m_FxEffectFilePath;
	settings.m_MeshEffectFilePath = m_MeshEffectFilePath;
	settings.m_RootScenePrefabFilePath = m_RootScenePrefabFilePath;
	settings.m_sScriptMainRelativeFilename = m_sScriptMainRelativeFilename;
	settings.m_ScriptErrorHandler = Main::Get()->GetSettings().m_ScriptErrorHandler;
#if SEOUL_HOT_LOADING
	settings.m_CustomHotLoadHandler = SEOUL_BIND_DELEGATE(&UI::Manager::HotReload, (UI::Manager*)UI::Manager::Get());
#endif // /SEOUL_HOT_LOADING

	// Instantiate the ScriptScene instance.
	m_pScriptScene.Reset(SEOUL_NEW(MemoryBudgets::Scene) ScriptScene(settings));
}

/** Custom render hook for the combat scene. */
void SceneMovie::OnPose(
	RenderPass& rPass,
	UI::Renderer& rRenderer)
{
	// Always use the full viewport.
	auto const viewport = GetViewport();
	rRenderer.PushViewport(viewport);

	// Deliberately don't call UI::Movie::OnRender() here, we have
	// completely custom render behavior.

	// Start rendering this movie.
	Falcon::Rectangle const stageBounds(Falcon::Rectangle::Create(0, (Float32)viewport.m_iViewportWidth, 0, (Float32)viewport.m_iViewportHeight));

	// Start this movie rendering in the renderer.
	rRenderer.BeginMovie(this, stageBounds);

	// Enqueue custom renderer context to handle ImGui rendering during
	// buffer generation.
	rRenderer.PoseCustomDraw(SEOUL_BIND_DELEGATE(&SceneMovie::InternalRender, this));

	// Done with movie.
	rRenderer.EndMovie();

	// Restore the viewport.
	rRenderer.PopViewport();
}

/** Custom update hook for the combat scene. */
void SceneMovie::OnTick(RenderPass& rPass, Float fDeltaTimeInSeconds)
{
	UI::Movie::OnTick(rPass, fDeltaTimeInSeconds);

	if (m_pScriptScene.IsValid())
	{
		m_pScriptScene->Tick(fDeltaTimeInSeconds);
	}
}

/**
 * Bridge from UI VM to Game VM.
 *
 * Hook to allow the UI VM to communicate with the Game VM.
 * Usage in script (outside of the Game::SceneMovie class) is:
 *
 * uiManager:BroadcastEventTo('GameSceneMovie', 'SendGameEvent', '<string_event_name>', ...<optional_arguments>)
 */
Bool SceneMovie::OnTryBroadcastEvent(
	HString eventName,
	const Reflection::MethodArguments& aMethodArguments,
	Int iArgumentCount)
{
	// If eventName == "SendGameEvent", handle it specially.
	if (eventName == kSendGameEvent)
	{
		if (m_pScriptScene.IsValid())
		{
			m_pScriptScene->SendEvent(
				aMethodArguments,
				iArgumentCount);
			return true;
		}

		// Can't handle right now.
		return false;
	}

	// Otherwise, use base class handling.
	return UI::Movie::OnTryBroadcastEvent(
		eventName,
		aMethodArguments,
		iArgumentCount);
}

} // namespace Game

} // namespace Seoul

#endif // /SEOUL_WITH_SCENE
