/**
* \file GameDevUIViewLocalization.h
* \brief A developer UI view component with localization
* tools for debugging in-game text.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_DEV_UI_VIEW_LOCALIZATION_H
#define GAME_DEV_UI_VIEW_LOCALIZATION_H

#include "CheckedPtr.h"
#include "DevUIView.h"
#include "HashSet.h"

#if (SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)

namespace Seoul { namespace Falcon { class Instance; } }
namespace Seoul { namespace Falcon { class MovieClipInstance; } }
namespace Seoul { namespace Falcon { class EditTextInstance; } }

namespace Seoul::Game
{

class DevUIViewLocalization SEOUL_SEALED : public DevUI::View
{
public:
	DevUIViewLocalization();
	~DevUIViewLocalization();

	// DevUIView overrides
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		static const HString kId("Localization");
		return kId;
	}

	virtual Bool GetInitialPosition(Vector2D& rv) const SEOUL_OVERRIDE
	{
		rv = Vector2D(150.0f, 250.0f);
		return true;
	}
	virtual Vector2D GetInitialSize() const SEOUL_OVERRIDE
	{
		return Vector2D(250.0f, 100.0f);
	}

protected:
	virtual void DoPrePose(DevUI::Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	// /DevUIView overrides

	void PoseInstance(CheckedPtr<Falcon::Instance const> pInstance, HashSet<HString>& foundTokens);
	void PoseMovieClip(CheckedPtr<Falcon::MovieClipInstance const> pInstance, HashSet<HString>& foundTokens);
	void PoseEditText(CheckedPtr<Falcon::EditTextInstance const> pInstance, HashSet<HString>& foundTokens);

	void GetTokens(String const& sString, Vector<HString>& rvTokens) const;

private:
	SEOUL_DISABLE_COPY(DevUIViewLocalization);
}; // class Game::DevUIViewLocalization

} // namespace Seoul::Game

#endif // /(SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)

#endif // include guard
