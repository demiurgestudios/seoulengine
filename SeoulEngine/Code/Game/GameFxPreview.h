/**
 * \file GameFxPreview.h
 * \brief UI::Movie subclass that implements preview rendering and some logic
 * for visual fx.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_FX_PREVIEW_H
#define GAME_FX_PREVIEW_H

#include "GameFxPreview.h"
#include "UIMovie.h"
namespace Seoul { class Camera; }
namespace Seoul { namespace UI { class FxRenderer; } }

namespace Seoul::Game
{

/**
 * Subclass of UI::Movie, handles rendering the Fx
 * preview effect.
 */
class FxPreview SEOUL_SEALED : public UI::Movie
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(FxPreview);

	FxPreview();
	~FxPreview();

#if SEOUL_HOT_LOADING
	/**
	 * The Game::FxPreview does not hot reload.
	 */
	virtual Bool IsPartOfHotReload() const SEOUL_OVERRIDE
	{
		return false;
	}
#endif // /#if SEOUL_HOT_LOADING

private:
	SEOUL_REFLECTION_FRIENDSHIP(FxPreview);

	// UI::Movie overrides.
	virtual Viewport GetViewport() const SEOUL_OVERRIDE;
	virtual void OnLoad() SEOUL_OVERRIDE;
	virtual void OnPose(RenderPass& rPass, UI::Renderer& rRenderer) SEOUL_OVERRIDE;
	virtual void OnTick(RenderPass& rPass, Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;
	// /UIMovie overrides.

	ScopedPtr<UI::FxRenderer> m_pRenderer;
	Vector3D m_vInitalPreviewPosition;

	SEOUL_DISABLE_COPY(FxPreview);
}; // class Game::FxPreview

} // namespace Seoul::Game

#endif // include guard
