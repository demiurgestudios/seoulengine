/**
 * \file DevUIImGuiRenderer.h
 * \brief Renderer implementation for "dear imgui" integration into
 * SeoulEngine, as used by the DevUI project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEV_UI_IM_GUI_RENDERER_H
#define DEV_UI_IM_GUI_RENDERER_H

#include "Color.h"
#include "DevUIImGuiRendererSettings.h"
#include "Effect.h"
#include "EffectPass.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
#include "Texture.h"
namespace Seoul { class IndexBuffer; }
namespace Seoul { class RenderCommandStreamBuilder; }
namespace Seoul { class RenderPass; }
namespace Seoul { class VertexBuffer; }
namespace Seoul { class VertexFormat; }
struct ImDrawData;
struct ImGuiIO;

#if SEOUL_ENABLE_DEV_UI

namespace Seoul::DevUI
{

/** Utility structure used to track texture data requested by ImGui. */
class ImGuiRendererTextureData SEOUL_SEALED
{
public:
	ImGuiRendererTextureData(const TextureContentHandle& hBase);
	~ImGuiRendererTextureData();

	void ResolveTexture(
		const Vector2D& vScreenDimensions,
		TextureContentHandle& rh);
	Bool ResolveDimensions(Vector2D& rv) const;

private:
	FixedArray<TextureContentHandle, (UInt32)FileType::LAST_TEXTURE_TYPE - (UInt32)FileType::FIRST_TEXTURE_TYPE + 1u> m_aTextures;

	SEOUL_DISABLE_COPY(ImGuiRendererTextureData);
}; // class DevUI::ImGuiRendererTextureData

class ImGuiRenderer SEOUL_SEALED
{
public:
	typedef Vector<OsWindowRegion, MemoryBudgets::DevUI> OsWindowRegions;

	ImGuiRenderer(const ImGuiRendererSettings& settings = ImGuiRendererSettings());
	~ImGuiRenderer();

	Bool BeginFrame(RenderPass& rPass);
	void Render(const ImDrawData& imDrawData, Byte const* sMainFormName);
	void EndFrame();

	void ReInitFontTexture();
	ImGuiRendererTextureData* ResolveTexture(FilePath filePath);

private:
	OsWindowRegions m_vOsWindowRegions;
	ImGuiRendererSettings const m_Settings;
	ScopedPtr<ImGuiRendererTextureData> m_pFontTexture;
	EffectPass m_Pass;
	CheckedPtr<RenderPass> m_pRenderPass;
	CheckedPtr<RenderCommandStreamBuilder> m_pBuilder;
	EffectContentHandle m_hEffect;
	SharedPtr<Effect> m_pAcquiredEffect;
	SharedPtr<IndexBuffer> m_pIndexBuffer;
	SharedPtr<VertexBuffer> m_pVertexBuffer;
	SharedPtr<VertexFormat> m_pVertexFormat;

	typedef HashTable<FilePath, ImGuiRendererTextureData*, MemoryBudgets::Editor> TextureData;
	TextureData m_tTextureData;

	void InternalBeginCustomRender();
	void InteranlEndCustomRender();

	SEOUL_DISABLE_COPY(ImGuiRenderer);
}; // class DevUI::ImGuiRenderer

} // namespace Seoul::DevUI

#endif // /SEOUL_ENABLE_DEV_UI

#endif // include guard
