/**
 * \file UILetterbox.cpp
 * \brief Rendering logic for displaying letterbox or pillarbox
 * framing of the entire viewport.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EffectManager.h"
#include "ReflectionDefine.h"
#include "RenderCommandStreamBuilder.h"
#include "RenderDevice.h"
#include "RenderPass.h"
#include "UIContext.h"
#include "UILetterbox.h"
#include "UIManager.h"
#include "TextureManager.h"
#include "VertexBuffer.h"
#include "VertexFormat.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(UI::LetterboxSettings)
	SEOUL_PROPERTY_N("LetterboxingEnabledOnPC", m_bLetterboxingEnabledOnPC)
	SEOUL_PROPERTY_N("LetterboxingEffect", m_EffectFilePath)
	SEOUL_PROPERTY_N("LetterboxingBaseTop", m_LetterFilePath)
	SEOUL_PROPERTY_N("PillarboxingBaseLeft", m_PillarFilePath)
SEOUL_END_TYPE()

namespace UI
{

// We need an anonymous namespace to keep the linker
// from thinking that this Vertex struct is not the
// other vertex struct. Making it anonymous
// effectively makes it static.
namespace
{

// C++ packed data structure used to populate the vertex buffer
// used by UI::GameBoardTiles for drawing.
SEOUL_DEFINE_PACKED_STRUCT(
	struct Vertex
{
	Float m_fPositionX;
	Float m_fPositionY;
	Float m_fTexcoordU;
	Float m_fTexcoordV;
});

static Vertex PopulateVertex(Float x, Float y, Float u, Float v)
{
	Vertex vert;
	vert.m_fPositionX = x;
	vert.m_fPositionY = y;
	vert.m_fTexcoordU = u;
	vert.m_fTexcoordV = v;
	return vert;
}

} // namespace anonymous

static const HString kEffectTechnique("seoul_Render");
static const HString kColorTextureParameterSemantic("seoul_LetterboxTexture");
static const UInt32 kPrimitivesPerQuad = 2u; // 2 triangels per quad.
static const UInt32 kVertexCount = 4u; // 4 vertices per quad.
static const UInt32 kNumQuads = 2u;
static const UInt32 kVertexCountForBothQuads = kNumQuads * kVertexCount; // two boxes, 4 verticies per quad ("box")

static SharedPtr<VertexFormat> CreateUILetterboxVertexFormat()
{
	static const VertexElement s_kaVertexFormat[] =
	{
		// Position (in stream 0)
		{ 0u,                         // stream
		0u,                           // offset
		VertexElement::TypeFloat2,    // type
		VertexElement::MethodDefault, // method
		VertexElement::UsagePosition, // usage
		0u },                         // usage index

		// Texcoords (in stream 0)
		{ 0u,                         // stream
		8u,                           // offset
		VertexElement::TypeFloat2,    // type
		VertexElement::MethodDefault, // method
		VertexElement::UsageTexcoord, // usage
		0u },                         // usage index

		VertexElementEnd
	};

	return RenderDevice::Get()->CreateVertexFormat(s_kaVertexFormat);
}

static SharedPtr<IndexBuffer> CreateUILetterboxIndexBuffer(UInt32 uMaxInstances)
{
	// Indices to draw a quad with 2 triangles.
	static const UInt16 kaIndicesForOneInstance[] =
	{
		0u, 1u, 2u,
		2u, 3u, 0u
	};

	// Total index buffer size is the max number of instances times the size of a single quad's index set.
	UInt32 const zIndexBufferSizeInBytes =
		uMaxInstances * sizeof(UInt16) * SEOUL_ARRAY_COUNT(kaIndicesForOneInstance);

	// Create a heap allocated buffer to define the indices.
	UInt16* pIdx = (UInt16*)MemoryManager::AllocateAligned(
		zIndexBufferSizeInBytes,
		MemoryBudgets::Rendering,
		SEOUL_ALIGN_OF(UInt16));

	// For each instance...
	for (UInt32 j = 0u; j < uMaxInstances; ++j)
	{
		// Copy the default indices for one quad, offset by the current instance.
		for (UInt32 i = 0u; i < SEOUL_ARRAY_COUNT(kaIndicesForOneInstance); ++i)
		{
			pIdx[SEOUL_ARRAY_COUNT(kaIndicesForOneInstance) * j + i] = kVertexCount * j + kaIndicesForOneInstance[i];
		}
	}

	return RenderDevice::Get()->CreateIndexBuffer(
		pIdx,
		zIndexBufferSizeInBytes,
		zIndexBufferSizeInBytes,
		IndexBufferDataFormat::kIndex16);
}

static SharedPtr<VertexBuffer> CreateUILetterboxVertexBuffer(UInt32 uMaxInstances)
{
	// Size of a the entire vertex buffer, size of a single vertex times the number
	// of vertices per quad.
	UInt32 const zVertexBufferSizeInBytes =
		uMaxInstances * sizeof(Vertex) * kVertexCount;

	SharedPtr<VertexBuffer> pReturn(RenderDevice::Get()->CreateDynamicVertexBuffer(
		zVertexBufferSizeInBytes,
		sizeof(Vertex)));

	return pReturn;
}

Letterbox::Letterbox(const LetterboxSettings& settings)
{
	m_bLetterboxingEnabled = true;
#if SEOUL_PLATFORM_WINDOWS
	m_bLetterboxingEnabled = settings.m_bLetterboxingEnabledOnPC;
#endif
	m_hEffect = EffectManager::Get()->GetEffect(settings.m_EffectFilePath);
	m_hLetterTexture = TextureManager::Get()->GetTexture(settings.m_LetterFilePath);
	m_hPillarTexture = TextureManager::Get()->GetTexture(settings.m_PillarFilePath);

	m_pIndices = CreateUILetterboxIndexBuffer(kNumQuads);
	m_pQuads = CreateUILetterboxVertexBuffer(kNumQuads);
	m_pVertexFormat = CreateUILetterboxVertexFormat();
}

Letterbox::~Letterbox()
{
}

void Letterbox::Draw(RenderCommandStreamBuilder& rBuilder, RenderPass& rPass)
{
	if (!m_bLetterboxingEnabled)
	{
		return;
	}

	SharedPtr<Effect> pEffect(m_hEffect.GetPtr());
	if (!pEffect.IsValid() || pEffect->GetState() == BaseGraphicsObject::kDestroyed)
	{
		return;
	}

	auto pUIManager(Manager::Get());
	if (!pUIManager)
	{
		return;
	}

	const auto viewportOriginal = pUIManager->ComputeViewport(); // Includes (possible) fixed aspect ratio.
	auto viewportModified = g_UIContext.GetRootViewport(); // Original without fixed aspect ratio applied.

	if (viewportOriginal == viewportModified)
	{
		// Perfect fit! Do nothing.
		return;
	}

	if (viewportModified.m_iViewportWidth == 0
		|| viewportModified.m_iViewportHeight == 0)
	{
		// This will cause problems, and there's nothing to do. Bail.
		return;
	}

	// Should we be doing letterboxing or pillarboxing?
	Bool bDoLetterbox = true;
	if (viewportOriginal.m_iViewportWidth == viewportModified.m_iViewportWidth)
	{
		bDoLetterbox = true;
	}
	else if (viewportOriginal.m_iViewportHeight == viewportModified.m_iViewportHeight)
	{
		bDoLetterbox = false;
	}
	else
	{
		// Neither the width nor the height matches the target
		// What is going on?
		return;
	}

	rBuilder.SetCurrentViewport(viewportModified);
	rBuilder.SetScissor(true, viewportModified);

	// Assume the draw effect is only 1 pass.
	auto pass = rBuilder.BeginEffect(pEffect, kEffectTechnique);
	if (pass.IsValid())
	{
		// If the pass succeeds, setup draw properties and draw.
		if (rBuilder.BeginEffectPass(pEffect, pass))
		{
			// Select the format and indices.
			rBuilder.UseVertexFormat(m_pVertexFormat.GetPtr());
			rBuilder.SetIndices(m_pIndices.GetPtr());

			// LockVertexBuffer is working with raw bytes, don't be scared
			auto pLock = (Vertex*)rBuilder.LockVertexBuffer(m_pQuads.GetPtr(),
				sizeof(Vertex) * kVertexCountForBothQuads);

			if (bDoLetterbox)
			{
				CalcLetterbox(rBuilder, pEffect, pLock, viewportOriginal);
			}
			else
			{
				CalcPillarbox(rBuilder, pEffect, pLock, viewportOriginal);
			}

			rBuilder.UnlockVertexBuffer(m_pQuads.GetPtr());
			rBuilder.SetVertices(0, m_pQuads.GetPtr(), 0, sizeof(Vertex));

			// Commit changes to the pass.
			rBuilder.CommitEffectPass(pEffect, pass);

			// Issue the draw call.
			rBuilder.DrawIndexedPrimitive(
				PrimitiveType::kTriangleList,
				0,
				0,
				kVertexCountForBothQuads,
				0,
				kPrimitivesPerQuad * kNumQuads);

			// Done with the pass.
			rBuilder.EndEffectPass(pEffect, pass);
		}

		// Done with the effect.
		rBuilder.EndEffect(pEffect);
	}

	// Restore the viewport
	rBuilder.SetCurrentViewport(viewportOriginal);
	rBuilder.SetScissor(true, viewportOriginal);
}

void Letterbox::CalcLetterbox(
	RenderCommandStreamBuilder& rBuilder,
	const SharedPtr<Effect>& pEffect,
	void* pLock,
	const Viewport& origViewp)
{
	auto const modViewp = rBuilder.GetCurrentViewport();
	auto pVertex = (Vertex*)pLock;

	auto pTexture = m_hLetterTexture.GetPtr();
	if (!pTexture.IsValid())
	{
		// Texture is not ready yet.  Bail and draw nothing.
		memset(pLock, 0, kVertexCountForBothQuads * sizeof(Vertex));
		return;
	}

	rBuilder.SetTextureParameter(pEffect, kColorTextureParameterSemantic, m_hLetterTexture);

	// Texture is padded to fit a power-of-two, so get the actual extent of the texture.
	auto vUV = pTexture->GetTexcoordsScale();

	// Top and bottom inner boundaries of the letterboxes,
	// and top and bottom texture coordinate boundaries of the letterboxes.
	Float const fTopLetterBottomBorder = (Float)Clamp(origViewp.m_iViewportY - modViewp.m_iViewportY, 0, (modViewp.m_iViewportY + modViewp.m_iViewportHeight));
	Float const fBottomLetterTopBorder = (Float)Clamp(origViewp.m_iViewportY + origViewp.m_iViewportHeight - modViewp.m_iViewportY, 0, (modViewp.m_iViewportY + modViewp.m_iViewportHeight));

	Float const fWidth = (Float)pTexture->GetWidth() * vUV.X;
	Float const fHeight = (Float)pTexture->GetHeight() * vUV.Y;
	Float const fScaledWidth = (Float)origViewp.m_iViewportWidth;
	Float const fScaledHeight = fHeight * (fScaledWidth / fWidth);

	Float const fTopLetterTopBorder = (fTopLetterBottomBorder - fScaledHeight);

	// Transform into normalized [0, 1] coordinates
	// This is safe because we checked for the width/height not being zero
	Float fHeightScalar = 1.0f / (Float)modViewp.m_iViewportHeight;

	pVertex[0] = PopulateVertex(0.0f, fTopLetterTopBorder * fHeightScalar, 0.0f, 0.0f);
	pVertex[1] = PopulateVertex(0.0f, fTopLetterBottomBorder * fHeightScalar, 0.0f, vUV.Y);
	pVertex[2] = PopulateVertex(1.0f, fTopLetterBottomBorder * fHeightScalar, vUV.X, vUV.Y);
	pVertex[3] = PopulateVertex(1.0f, fTopLetterTopBorder * fHeightScalar, vUV.X, 0.0f);

	Float const fBottomLetterBottomBorder = fBottomLetterTopBorder + fScaledHeight;

	// Flipped version - switch the UVs around
	pVertex[4] = PopulateVertex(0.0f, fBottomLetterTopBorder * fHeightScalar, 0.0f, vUV.Y);
	pVertex[5] = PopulateVertex(0.0f, fBottomLetterBottomBorder * fHeightScalar, 0.0f, 0.0f);
	pVertex[6] = PopulateVertex(1.0f, fBottomLetterBottomBorder * fHeightScalar, vUV.X, 0.0f);
	pVertex[7] = PopulateVertex(1.0f, fBottomLetterTopBorder * fHeightScalar, vUV.X, vUV.Y);
}

void Letterbox::CalcPillarbox(
	RenderCommandStreamBuilder& rBuilder,
	const SharedPtr<Effect>& pEffect,
	void* pLock,
	const Viewport& origViewp)
{
	auto const modViewp = rBuilder.GetCurrentViewport();
	auto pVertex = (Vertex*)pLock;

	auto pTexture = m_hPillarTexture.GetPtr();
	if (!pTexture.IsValid())
	{
		// Texture is not ready yet.  Bail and draw nothing.
		memset(pLock, 0, kVertexCountForBothQuads * sizeof(Vertex));
		return;
	}

	rBuilder.SetTextureParameter(pEffect, kColorTextureParameterSemantic, m_hPillarTexture);

	// Texture is padded to fit a power-of-two, so get the actual extent of the texture.
	auto vUV = pTexture->GetTexcoordsScale();

	// Left and right inner boundaries of the pillars,
	// and left and right texture coordinate boundaries of the pillars.
	Float const fLeftPillarRightBorder = (Float)Clamp(origViewp.m_iViewportX - modViewp.m_iViewportX, 0, (modViewp.m_iViewportX + modViewp.m_iViewportWidth));
	Float const fRightPillarLeftBorder = (Float)Clamp(origViewp.m_iViewportX + origViewp.m_iViewportWidth - modViewp.m_iViewportX, 0, (modViewp.m_iViewportX + modViewp.m_iViewportWidth));

	Float const fWidth = (Float)pTexture->GetWidth() * vUV.X;
	Float const fHeight = (Float)pTexture->GetHeight() * vUV.Y;
	Float const fScaledHeight = (Float)origViewp.m_iViewportHeight;
	Float const fScaledWidth = fWidth * (fScaledHeight / fHeight);

	Float const fLeftPillarLeftBorder = (fLeftPillarRightBorder - fScaledWidth);

	// Transform into normalized [0, 1] coordinates
	// This is safe because we checked for the target width/height not being zero
	Float fWidthScalar = 1.0f / (Float)modViewp.m_iViewportWidth;

	pVertex[0] = PopulateVertex(fLeftPillarLeftBorder * fWidthScalar, 0.0f, 0.0f, 0.0f);
	pVertex[1] = PopulateVertex(fLeftPillarLeftBorder * fWidthScalar, 1.0f, 0.0f, vUV.Y);
	pVertex[2] = PopulateVertex(fLeftPillarRightBorder * fWidthScalar, 1.0f, vUV.X, vUV.Y);
	pVertex[3] = PopulateVertex(fLeftPillarRightBorder * fWidthScalar, 0.0f, vUV.X, 0.0f);

	Float const fRightPillarRightBorder = fRightPillarLeftBorder + fScaledWidth;

	// Flipped version - switch the UVs around
	pVertex[4] = PopulateVertex(fRightPillarLeftBorder * fWidthScalar, 0.0f, vUV.X, 0.0f);
	pVertex[5] = PopulateVertex(fRightPillarLeftBorder * fWidthScalar, 1.0f, vUV.X, vUV.Y);
	pVertex[6] = PopulateVertex(fRightPillarRightBorder * fWidthScalar, 1.0f, 0.0f, vUV.Y);
	pVertex[7] = PopulateVertex(fRightPillarRightBorder * fWidthScalar, 0.0f, 0.0f, 0.0f);
}

} // namespace UI

} // namespace Seoul
