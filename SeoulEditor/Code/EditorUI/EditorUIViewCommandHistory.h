/**
 * \file EditorUIViewCommandHistory.h
 * \brief An editor view that displays
 * a tool for working with hierarchical data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_VIEW_COMMAND_HISTORY_H
#define EDITOR_UI_VIEW_COMMAND_HISTORY_H

#include "DevUIView.h"

namespace Seoul::EditorUI
{

class ViewCommandHistory SEOUL_SEALED : public DevUI::View
{
public:
	ViewCommandHistory();
	~ViewCommandHistory();

	// DevUI::View overrides
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		static const HString kId("History");
		return kId;
	}
	// /DevUI::View overrides

private:
	// DevUI::View overrides
	virtual void DoPrePose(DevUI::Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	// /DevUI::View overrides

	SEOUL_DISABLE_COPY(ViewCommandHistory);
}; // class ViewCommandHistory

} // namespace Seoul::EditorUI

#endif // include guard
