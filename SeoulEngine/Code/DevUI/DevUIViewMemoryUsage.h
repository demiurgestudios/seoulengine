/**
 * \file DevUIViewMemoryUsage.h
 * \brief A developer UI view component that displays
 * the runtime's current memory usage info.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEV_UI_VIEW_MEMORY_USAGE_H
#define DEV_UI_VIEW_MEMORY_USAGE_H

#include "DevUIView.h"

#if (SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)

namespace Seoul::DevUI
{

class ViewMemoryUsage SEOUL_SEALED : public View
{
public:
	ViewMemoryUsage();
	~ViewMemoryUsage();

	// DevUI::View overrides
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		static const HString kId("Memory Usage");
		return kId;
	}

protected:
	virtual void DoPrePose(Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	virtual Vector2D GetInitialSize() const SEOUL_OVERRIDE
	{
		return Vector2D(300, 600);
	}
	// /DevUIView overrides

private:
	SEOUL_DISABLE_COPY(ViewMemoryUsage);
}; // class DevUI::ViewMemoryUsage

} // namespace Seoul::DevUI

#endif // /(SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)

#endif // include guard
