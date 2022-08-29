/**
 * \file DevUIImGuiRendererSettings.h
 * \brief Configuration of DevUI's "dear imgui" renderer.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEV_UI_IM_GUI_RENDERER_SETTINGS_H
#define DEV_UI_IM_GUI_RENDERER_SETTINGS_H

#include "FilePath.h"

#if SEOUL_ENABLE_DEV_UI

namespace Seoul::DevUI
{

struct ImGuiRendererSettings SEOUL_SEALED
{
	ImGuiRendererSettings()
		: m_uVertexBufferSizeInVertices(43704u)
		, m_EffectFilePath(FilePath::CreateContentFilePath("Authored/Effects/Imgui/Imgui.fx"))
	{
	}

	/** Index buffer size in indices is derived from vertices. */
	UInt32 GetIndexBufferSizeInIndices() const
	{
		return ((m_uVertexBufferSizeInVertices / 4) * 6);
	}

	/** Maximum number of vertices in the DevUI::ImGuiRenderer's scratch buffer. */
	UInt32 m_uVertexBufferSizeInVertices;

	/** Path to the .fx file to use for rendering the developer UI. */
	FilePath m_EffectFilePath;
}; // struct DevUI::ImGuiRendererSettings

} // namespace Seoul::DevUI

#endif // /SEOUL_ENABLE_DEV_UI

#endif // include guard
