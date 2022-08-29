/**
 * \file GameDevUIViewAnimation3DNetworks.h
 * \brief Supports for visualization and debugging of animation
 * networks.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_DEV_UI_VIEW_ANIMATION3D_NETWORKS_H
#define GAME_DEV_UI_VIEW_ANIMATION3D_NETWORKS_H

#include "DevUIView.h"

#if (SEOUL_ENABLE_DEV_UI && SEOUL_WITH_ANIMATION_3D && !SEOUL_SHIP)

namespace Seoul::Game
{

class DevUIViewAnimation3DNetworks SEOUL_SEALED : public DevUI::View
{
public:
	DevUIViewAnimation3DNetworks();
	~DevUIViewAnimation3DNetworks();

	// DevUIView overrides
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		static const HString kId("Animation3D Networks");
		return kId;
	}

private:
	virtual void DoPrePose(DevUI::Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	virtual UInt32 GetFlags() const SEOUL_OVERRIDE;
	virtual Vector2D GetInitialSize() const SEOUL_OVERRIDE
	{
		return Vector2D(400, 600);
	}
	String m_sSelected;
	HString m_Trigger;

	SEOUL_DISABLE_COPY(DevUIViewAnimation3DNetworks);
}; // class Game::DevUIViewAnimation3DNetworks

#endif // /SEOUL_WITH_ANIMATION_3D

} // namespace Seoul::Game

#endif // /(SEOUL_ENABLE_DEV_UI && SEOUL_WITH_ANIMATION_3D && !SEOUL_SHIP)
