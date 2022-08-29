/**
 * \file GameSceneMovie.h
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

#pragma once
#ifndef GAME_SCENE_MOVIE_H
#define GAME_SCENE_MOVIE_H

#include "Delegate.h"
#include "ScopedPtr.h"
#include "UIMovie.h"
namespace Seoul { class ScriptScene; }

#if SEOUL_WITH_SCENE

namespace Seoul::Game
{

/**
 * UI::Movie that owns a scriptable 3D scene.
 *
 * Game::SceneMovie has no UI elements (it is a non-Falcon
 * UI::Movie). It acts as a state to control when
 * and where a Scene instance exists. It also
 * provides the interface (via OnTryBroadcastEvent())
 * that allows the UI system to communicate with
 * the 3D scene.
 */
class SceneMovie SEOUL_SEALED : public UI::Movie
{
public:
	SEOUL_DELEGATE_TARGET(SceneMovie);
	SEOUL_REFLECTION_POLYMORPHIC(SceneMovie);

	SceneMovie();
	~SceneMovie();

private:
	SEOUL_REFLECTION_FRIENDSHIP(SceneMovie);

	void InternalRender(RenderPass& rPass, RenderCommandStreamBuilder& rBuilder);

	// TODO: Eliminate, OnHitTest is only necessary
	// to work around the fact that this type of movie
	// has no FCN file.
	virtual UI::MovieHitTestResult OnHitTest(
		UInt8 uMask,
		const Point2DInt& mousePosition,
		UI::Movie*& rpHitMovie,
		SharedPtr<Falcon::MovieClipInstance>& rpHitInstance,
		SharedPtr<Falcon::Instance>& rpLeafInstance,
		Vector<UI::Movie*>* rvPassthroughInputs) const SEOUL_OVERRIDE;
	virtual void OnMouseButtonPressed(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance,
		Bool bInInstance) SEOUL_OVERRIDE;
	virtual void OnMouseButtonReleased(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance,
		Bool bInInstance,
		UInt8 uInputCaptureHitTestMask) SEOUL_OVERRIDE;
	virtual void OnLoad() SEOUL_OVERRIDE;
	virtual void OnPose(RenderPass& rPass, UI::Renderer& rRenderer) SEOUL_OVERRIDE;
	virtual void OnTick(RenderPass& rPass, Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;

	// Script hook - serves as the UI VM -> Combat Game VM
	// bridge.
	virtual Bool OnTryBroadcastEvent(
		HString eventName,
		const Reflection::MethodArguments& aMethodArguments,
		Int iArgumentCount) SEOUL_OVERRIDE;

	FilePath m_FxEffectFilePath;
	FilePath m_MeshEffectFilePath;
	FilePath m_RootScenePrefabFilePath;
	String m_sScriptMainRelativeFilename;
	ScopedPtr<ScriptScene> m_pScriptScene;

	SEOUL_DISABLE_COPY(SceneMovie);
}; // class Game::SceneMovie

} // namespace Seoul::Game

#endif // /SEOUL_WITH_SCENE

#endif // include guard
