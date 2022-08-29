/**
 * \file DevUIViewEngineStats.h
 * \brief A developer UI view component that displays
 * miscellaneous Engine stats (like object draw count).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEV_UI_VIEW_ENGINE_STATS_H
#define DEV_UI_VIEW_ENGINE_STATS_H

#include "DevUIView.h"

#if (SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)

namespace Seoul::DevUI
{

class ViewEngineStats SEOUL_SEALED : public View
{
public:
	ViewEngineStats();
	~ViewEngineStats();

	// DevUI::View overrides
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		static const HString kId("Engine Stats");
		return kId;
	}

protected:
	virtual void DoPrePose(Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	// /DevUIView overrides

private:
	SEOUL_DISABLE_COPY(ViewEngineStats);
}; // class DevUI::ViewEngineStats

} // namespace Seoul::DevUI

#endif // /(SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)

#endif // include guard
