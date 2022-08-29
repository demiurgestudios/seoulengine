/**
* \file EditorUIViewLog.h
* \brief Log pane for the editor.
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#pragma once
#ifndef EDITOR_UI_VIEW_LOG_H
#define EDITOR_UI_VIEW_LOG_H

#include "DevUIView.h"

namespace Seoul::EditorUI
{

class ViewLog SEOUL_SEALED : public DevUI::View
{
public:
	ViewLog();
	~ViewLog();

	// DevUI::View overrides
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		static const HString kId("Log");
		return kId;
	}
	// /DevUI::View overrides

private:
	// DevUI::View overrides
	virtual void DoPrePose(DevUI::Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	// /DevUI::View overrides

	SEOUL_DISABLE_COPY(ViewLog);

	Atomic32Type m_LastUpdateCount;
}; // class ViewLog

} // namespace Seoul::EditorUI

#endif // include guard
