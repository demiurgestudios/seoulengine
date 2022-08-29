/**
 * \file UILetterbox.h
 * \brief Rendering logic for displaying letterbox or pillarbox
 * framing of the entire viewport.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_LETTERBOX_H
#define UI_LETTERBOX_H

#include "Effect.h"
#include "SharedPtr.h"
#include "Texture.h"
namespace Seoul { class RenderPass; }

namespace Seoul::UI
{

struct LetterboxSettings SEOUL_SEALED
{
	Bool m_bLetterboxingEnabledOnPC{};
	FilePath m_EffectFilePath;
	FilePath m_LetterFilePath;
	FilePath m_PillarFilePath;
};

class Letterbox SEOUL_SEALED
{
public:
	Letterbox(const LetterboxSettings& settings);
	~Letterbox();

	void Draw(RenderCommandStreamBuilder& rBuilder, RenderPass& rPass);

private:
	void CalcLetterbox(
		RenderCommandStreamBuilder& rBuilder, 
		const SharedPtr<Effect>& pEffect,
		void* pLock, 
		const Viewport& viewp);
	void CalcPillarbox(
		RenderCommandStreamBuilder& rBuilder,
		const SharedPtr<Effect>& pEffect,
		void* pLock, 
		const Viewport& viewp);

	Bool m_bLetterboxingEnabled;

	EffectContentHandle m_hEffect;
	TextureContentHandle m_hLetterTexture;
	TextureContentHandle m_hPillarTexture;
	SharedPtr<IndexBuffer> m_pIndices;
	SharedPtr<VertexBuffer> m_pQuads;
	SharedPtr<VertexFormat> m_pVertexFormat;

	SEOUL_DISABLE_COPY(Letterbox);
}; // class UI::State;

} // namespace Seoul::UI

#endif // include guard
