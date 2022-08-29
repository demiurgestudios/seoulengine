/**
 * \file DevUIImGuiRenderer.cpp
 * \brief Renderer implementation for "dear imgui" integration into
 * SeoulEngine, as used by the DevUI project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BaseGraphicsObject.h"
#include "ContentLoadManager.h"
#include "DevUIImGui.h"
#include "DevUIImGuiRenderer.h"
#include "DevUIRoot.h"
#include "DevUIUtil.h"
#include "Effect.h"
#include "EffectManager.h"
#include "RenderCommandStreamBuilder.h"
#include "RenderDevice.h"
#include "RenderPass.h"
#include "TextureManager.h"

#if SEOUL_ENABLE_DEV_UI

namespace Seoul::DevUI
{

// Constants used for setting up and submitting ImGui rendering.
static const HString kEffectTechnique("seoul_Render");
static const HString kTextureParameterName("seoul_Texture");
static const HString kViewportDimensionsInPixels("seoul_ViewportDimensionsInPixels");
static const HString kViewProjectionTransform("seoul_ViewProjectionUI");

/** Utility structure used to track texture data requested by Gwen. */
ImGuiRendererTextureData::ImGuiRendererTextureData(const TextureContentHandle& hBase)
	: m_aTextures()
{
	// Indirect, only fill in the last, we will request the
	// others on demand.
	if (hBase.IsIndirect())
	{
		m_aTextures.Back() = hBase;
	}
	// Direct textures (no file path) need
	// to be used at every level.
	else
	{
		m_aTextures.Fill(hBase);
	}
}

ImGuiRendererTextureData::~ImGuiRendererTextureData()
{
}

void ImGuiRendererTextureData::ResolveTexture(
	const Vector2D& vScreenDimensions,
	TextureContentHandle& rh)
{
	// Early out.
	if (!m_aTextures.Back().IsIndirect())
	{
		rh = m_aTextures.Back();
		return;
	}

	auto p(m_aTextures.Back().GetPtr());
	if (!p.IsValid())
	{
		rh = m_aTextures.Back();
		return;
	}

	auto iTarget = (Int32)Ceil(vScreenDimensions.X) * (Int32)Ceil(vScreenDimensions.Y);
	auto iCurrent = (Int32)(p->GetWidth() * p->GetTexcoordsScale().X * p->GetHeight() * p->GetTexcoordsScale().Y);
	auto iIndex = (Int32)m_aTextures.GetSize() - 1;
	while (iIndex > 0 && iCurrent < iTarget)
	{
		--iIndex;
		iCurrent *= 4;
	}

	if (!m_aTextures[iIndex].IsInternalPtrValid())
	{
		FilePath filePath = m_aTextures.Back().GetKey();
		filePath.SetType((FileType)(iIndex + (Int32)FileType::FIRST_TEXTURE_TYPE));
		m_aTextures[iIndex] = TextureManager::Get()->GetTexture(filePath);
	}

	while (iIndex < (Int32)m_aTextures.GetSize() - 1 && !m_aTextures[iIndex].IsPtrValid())
	{
		++iIndex;
	}

	rh = m_aTextures[iIndex];
}

Bool ImGuiRendererTextureData::ResolveDimensions(Vector2D& rv) const
{
	auto p(m_aTextures.Back().GetPtr());
	if (!p.IsValid())
	{
		return false;
	}

	rv.X = (Float)p->GetWidth();
	rv.Y = (Float)p->GetHeight();
	return true;
}

SEOUL_STATIC_ASSERT(sizeof(ImDrawVert) == 20);
static SharedPtr<VertexFormat> InternalStaticCreateDevUIVertexFormat()
{
	static const VertexElement s_kaVertexFormat[] =
	{
		// Position (in stream 0)
		{ 0,								// stream
		  0,								// offset
		  VertexElement::TypeFloat2,		// type
		  VertexElement::MethodDefault,		// method
		  VertexElement::UsagePosition,		// usage
		  0u },								// usage index

		// Texcoords (in stream 0)
		{ 0,								// stream
		  8,								// offset
		  VertexElement::TypeFloat2,		// type
		  VertexElement::MethodDefault,		// method
		  VertexElement::UsageTexcoord,		// usage
		  0u },								// usage index

		// Color (in stream 0)
		{ 0,								// stream
		  16,								// offset
		  VertexElement::TypeColor,			// type
		  VertexElement::MethodDefault,		// method
		  VertexElement::UsageColor,		// usage
		  0u },								// usage index

		VertexElementEnd
	};

	return RenderDevice::Get()->CreateVertexFormat(s_kaVertexFormat);
}
SEOUL_STATIC_ASSERT(sizeof(ImDrawIdx) == sizeof(UInt16));

ImGuiRendererTextureData* InternalStaticCreateFontTexture()
{
	UInt8* pData = nullptr;
	Int32 iWidth = 0;
	Int32 iHeight = 0;
	UInt32 uSize = 0u;

	// Get and copy font data.
	{
		UInt8* pReadOnlyData = nullptr;
		ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pReadOnlyData, &iWidth, &iHeight);
		uSize = (UInt32)(iWidth * iHeight * 4);

		if (nullptr != pReadOnlyData)
		{
			pData = (UInt8*)MemoryManager::Allocate(
				uSize,
				MemoryBudgets::DevUI);
			memcpy(pData, pReadOnlyData, uSize);
		}
	}

	auto eFormat = PixelFormat::kA8R8G8B8;
	TextureData data = TextureData::CreateFromInMemoryBuffer(pData, uSize, eFormat);
	TextureConfig config;
	ImGuiRendererTextureData* pTextureData = SEOUL_NEW(MemoryBudgets::DevUI) ImGuiRendererTextureData(
		TextureContentHandle(RenderDevice::Get()->CreateTexture(
			config,
			data,
			(UInt32)iWidth,
			(UInt32)iHeight,
			eFormat).GetPtr()));
	return pTextureData;
}

ImGuiRenderer::ImGuiRenderer(const ImGuiRendererSettings& settings /*= DevUI::ImGuiRendererSettings()*/)
	: m_Settings(settings)
	, m_pFontTexture()
	, m_Pass()
	, m_pRenderPass()
	, m_pBuilder()
	, m_hEffect(EffectManager::Get()->GetEffect(settings.m_EffectFilePath))
	, m_pAcquiredEffect()
	, m_pIndexBuffer(RenderDevice::Get()->CreateDynamicIndexBuffer(sizeof(UInt16) * settings.GetIndexBufferSizeInIndices(), IndexBufferDataFormat::kIndex16))
	, m_pVertexBuffer(RenderDevice::Get()->CreateDynamicVertexBuffer(sizeof(ImDrawVert) * settings.m_uVertexBufferSizeInVertices, sizeof(ImDrawVert)))
	, m_pVertexFormat(InternalStaticCreateDevUIVertexFormat())
{
	ReInitFontTexture();
}

ImGuiRenderer::~ImGuiRenderer()
{
	// Disassociate font texture.
	ImGui::GetIO().Fonts->TexID = nullptr;

	// Cleanup texture instances.
	SafeDeleteTable(m_tTextureData);
}

Bool ImGuiRenderer::BeginFrame(RenderPass& rPass)
{
	// Can't render if we don't have an effect yet.
	m_pAcquiredEffect = m_hEffect.GetPtr();
	if (!m_pAcquiredEffect.IsValid() || BaseGraphicsObject::kDestroyed == m_pAcquiredEffect->GetState())
	{
		return false;
	}

	m_pRenderPass = &rPass;
	m_pBuilder = rPass.GetRenderCommandStreamBuilder();

	// Compute and set view projection and viewport dimensions.
	{
		// Cache rescale factor.
		auto const fWindowScale = Root::Get()->GetWindowScale();

		Viewport const viewport = m_pBuilder->GetCurrentViewport();
		Vector4D const vViewportDimensionsInPixels(
			(Float)(viewport.m_iViewportWidth * fWindowScale),
			(Float)(viewport.m_iViewportHeight * fWindowScale),
			1.0f / (Float)(viewport.m_iViewportWidth * fWindowScale),
			1.0f / (Float)(viewport.m_iViewportHeight * fWindowScale));
		Vector4D const vViewProjectionTransform(
			(2.0f / (Float)(viewport.m_iViewportWidth * fWindowScale)),
			(-2.0f / (Float)(viewport.m_iViewportHeight * fWindowScale)),
			-1.0f,
			1.0f);

		// Commit non-changing parameters to the render Effect.
		m_pBuilder->SetVector4DParameter(m_pAcquiredEffect, kViewportDimensionsInPixels, vViewportDimensionsInPixels);
		m_pBuilder->SetVector4DParameter(m_pAcquiredEffect, kViewProjectionTransform, vViewProjectionTransform);
	}

	// Setup the shader Effect and buffers for frame rendering.
	m_Pass = m_pBuilder->BeginEffect(m_pAcquiredEffect, kEffectTechnique);
	m_pBuilder->BeginEffectPass(m_pAcquiredEffect, m_Pass);
	m_pBuilder->UseVertexFormat(m_pVertexFormat.GetPtr());
	m_pBuilder->SetIndices(m_pIndexBuffer.GetPtr());
	m_pBuilder->SetVertices(0, m_pVertexBuffer.GetPtr(), 0, sizeof(ImDrawVert));

	return true;
}

void ImGuiRenderer::Render(const ImDrawData& imDrawData, Byte const* sMainFormName)
{
	// Absolute first, update window regions.
	if (Root::Get()->IsVirtualizedDesktop())
	{
		auto const fWindowScale = Root::Get()->GetWindowScale();
		m_vOsWindowRegions.Clear();
		ImGui::GatherAllWindowRects(fWindowScale, m_vOsWindowRegions, sMainFormName);
		QuickSort(m_vOsWindowRegions.Begin(), m_vOsWindowRegions.End());
		m_pBuilder->UpdateOsWindowRegions(m_vOsWindowRegions.Data(), m_vOsWindowRegions.GetSize());
	}
	// Make sure it's unset.
	else
	{
		m_pBuilder->UpdateOsWindowRegions(nullptr, 0u);
	}

	Int32 iCmdList = 0;

	Int32 const iMaxIndices = (Int32)m_Settings.GetIndexBufferSizeInIndices();
	Int32 const iMaxVertices = (Int32)m_Settings.m_uVertexBufferSizeInVertices;

	Bool bRestoreScissor = false;

	// Outer loop supports multiple passes if the total draw
	// count exceeds our buffer sizes.
	Int32 iCmdListsCount = imDrawData.CmdListsCount;
	while (iCmdList < iCmdListsCount)
	{
		Int32 iIndices = 0;
		Int32 iVertices = 0;

		// Count indices and vertices.
		for (Int32 i = iCmdList; i < iCmdListsCount; ++i)
		{
			const ImDrawList& imDrawList = *imDrawData.CmdLists[i];

			// Update iCmdListCount to i if we'd exceed
			// either of our capacities.
			Int32 const iIndexBufferSize = imDrawList.IdxBuffer.size();
			Int32 const iVertexBufferSize = imDrawList.VtxBuffer.size();
			if (iIndexBufferSize + iIndices > iMaxIndices ||
				iVertexBufferSize + iVertices > iMaxVertices)
			{
				iCmdListsCount = i;
				break;
			}

			iIndices += iIndexBufferSize;
			iVertices += iVertexBufferSize;
		}

		// Now lock buffers, populate, and render.

		// Populate indices.
		if (iIndices > 0)
		{
			UInt32 const zSizeInBytes = (UInt32)(iIndices * sizeof(UInt16));
			UInt16* pIndices = (UInt16*)m_pBuilder->LockIndexBuffer(m_pIndexBuffer.GetPtr(), zSizeInBytes);
			if (nullptr == pIndices)
			{
				return;
			}

			for (Int32 i = iCmdList; i < iCmdListsCount; ++i)
			{
				const ImDrawList& imDrawList = *imDrawData.CmdLists[i];
				for (auto const& imDraw : imDrawList.CmdBuffer)
				{
					if (!imDraw.ElemCount)
					{
						continue;
					}

					memcpy(pIndices, &imDrawList.IdxBuffer[imDraw.IdxOffset], imDraw.ElemCount * sizeof(UInt16));
					pIndices += imDraw.ElemCount;
				}
			}

			m_pBuilder->UnlockIndexBuffer(m_pIndexBuffer.GetPtr());
		}

		// Populate vertices.
		if (iVertices > 0)
		{
			UInt32 const zSizeInBytes = (UInt32)(iVertices * sizeof(ImDrawVert));
			ImDrawVert* pVertices = (ImDrawVert*)m_pBuilder->LockVertexBuffer(m_pVertexBuffer.GetPtr(), zSizeInBytes);
			if (nullptr == pVertices)
			{
				return;
			}

			for (Int32 i = iCmdList; i < iCmdListsCount; ++i)
			{
				const ImDrawList& imDrawList = *imDrawData.CmdLists[i];
				int const iSize = imDrawList.VtxBuffer.size();

				// Nothing to do if the vertex buffer is empty.
				if (iSize <= 0)
				{
					continue;
				}

				// Copy the vertices to the output.
				memcpy(pVertices, &imDrawList.VtxBuffer[0], iSize * sizeof(ImDrawVert));

				// Apply the visible rectangle of the texture to the vertex coordinates.
				UInt32 uIndexOffset = 0u;
				for (Int32 iCmd = 0; iCmd < imDrawList.CmdBuffer.size(); ++iCmd)
				{
					const ImDrawCmd& imDrawCmd = imDrawList.CmdBuffer[iCmd];

					auto const vScreenDimensions = Vector2D(
						imDrawCmd.TexScreenDim.x,
						imDrawCmd.TexScreenDim.y);
					TextureContentHandle hTexture;
					if (nullptr != imDrawCmd.TextureId)
					{
						((ImGuiRendererTextureData*)imDrawCmd.TextureId)->ResolveTexture(
							vScreenDimensions,
							hTexture);
					}

					auto pTexture(hTexture.GetPtr());

					// Have a texture.
					if (pTexture.IsValid())
					{
						// Check rescale - if not the identity, apply it to the UV
						// channel of this draw command's vertex buffer range.
						auto const vScale = pTexture->GetTexcoordsScale();
						if (vScale != Vector2D::One())
						{
							// TODO: Annoying to need to do this by index.
							// My longterm goal is to gut imgui and reimplement its
							// rendering backend with Falcon, which will take care
							// of cases like this automatically.

							auto const pBegin = imDrawList.IdxBuffer.Data + uIndexOffset;
							auto const pEnd = pBegin + imDrawCmd.ElemCount;
							for (auto p = pBegin; p < pEnd; ++p)
							{
								auto const& vtx = imDrawList.VtxBuffer[*p];
								auto& outVtx = pVertices[*p];

								outVtx.uv.x = vtx.uv.x * vScale.X;
								outVtx.uv.y = vtx.uv.y * vScale.Y;
							}
						}
					}

					uIndexOffset += imDrawCmd.ElemCount;
				}

				pVertices += iSize;
			}

			m_pBuilder->UnlockVertexBuffer(m_pVertexBuffer.GetPtr());
		}

		// Issue draw calls.
		Int32 iIndexOffset = 0;
		Int32 iVertexOffset = 0;
		for (; iCmdList < iCmdListsCount; ++iCmdList)
		{
			const ImDrawList& imDrawList = *imDrawData.CmdLists[iCmdList];
			for (Int32 iCmd = 0; iCmd < imDrawList.CmdBuffer.size(); ++iCmd)
			{
				const ImDrawCmd& imDrawCmd = imDrawList.CmdBuffer[iCmd];
				if (nullptr != imDrawCmd.UserCallback)
				{
					InternalBeginCustomRender();
					imDrawCmd.UserCallback(&imDrawList, &imDrawCmd, m_pRenderPass.Get());
					InteranlEndCustomRender();
				}
				else if (imDrawCmd.ElemCount > 0)
				{
					// Cache rescale factor.
					auto const fWindowScale = Root::Get()->GetWindowScale();

					auto const cur = m_pBuilder->GetCurrentViewport();
					Viewport scissorViewport(cur);

					// += here is deliberate, ClipRect assumes origin at (0, 0),
					// but scissor needs to respect any letterboxing/pillarboxing.
					scissorViewport.m_iViewportX += (Int32)Floor(imDrawCmd.ClipRect.x / fWindowScale);
					scissorViewport.m_iViewportY += (Int32)Floor(imDrawCmd.ClipRect.y / fWindowScale);
					scissorViewport.m_iViewportWidth = (Int32)Ceil((imDrawCmd.ClipRect.z - imDrawCmd.ClipRect.x) / fWindowScale);
					scissorViewport.m_iViewportHeight = (Int32)Ceil((imDrawCmd.ClipRect.w - imDrawCmd.ClipRect.y) / fWindowScale);

					// Clamp, out-of-bounds can happen and will be rejected by some backends.
					scissorViewport.m_iViewportX = Max(scissorViewport.m_iViewportX, cur.m_iViewportX);
					scissorViewport.m_iViewportY = Max(scissorViewport.m_iViewportY, cur.m_iViewportY);
					scissorViewport.m_iViewportWidth -= Max(scissorViewport.GetViewportRight() - cur.GetViewportRight(), 0);
					scissorViewport.m_iViewportHeight -= Max(scissorViewport.GetViewportBottom() - cur.GetViewportBottom(), 0);

					auto const uPrimitives = (UInt)(imDrawCmd.ElemCount / 3);
					auto const uVertices = (UInt)(imDrawList.VtxBuffer.size());
					auto const vScreenDimensions = Vector2D(
						imDrawCmd.TexScreenDim.x,
						imDrawCmd.TexScreenDim.y);
					TextureContentHandle hTexture;
					if (nullptr != imDrawCmd.TextureId)
					{
						((ImGuiRendererTextureData*)imDrawCmd.TextureId)->ResolveTexture(
							vScreenDimensions,
							hTexture);
					}

					m_pBuilder->SetTextureParameter(m_pAcquiredEffect, kTextureParameterName, hTexture);
					m_pBuilder->SetScissor(true, scissorViewport);
					bRestoreScissor = true;

					m_pBuilder->CommitEffectPass(m_pAcquiredEffect, m_Pass);
					m_pBuilder->DrawIndexedPrimitive(
						PrimitiveType::kTriangleList,
						iVertexOffset + (Int32)imDrawCmd.VtxOffset,
						0,
						uVertices,
						(UInt)iIndexOffset,
						uPrimitives);
				}
				iIndexOffset += imDrawCmd.ElemCount;
			}
			iVertexOffset += imDrawList.VtxBuffer.size();
		}
	}

	// Make sure we restore the scissor rectangle if we changed it.
	if (bRestoreScissor)
	{
		m_pBuilder->SetScissor(true, m_pBuilder->GetCurrentViewport());
	}
}

void ImGuiRenderer::EndFrame()
{
	// Done with the shader effect for the just rendered frame.
	m_pBuilder->CommitEffectPass(m_pAcquiredEffect, m_Pass);
	m_pBuilder->EndEffectPass(m_pAcquiredEffect, m_Pass);
	m_Pass = EffectPass();
	m_pBuilder->EndEffect(m_pAcquiredEffect);
	m_pBuilder = nullptr;
	m_pRenderPass = nullptr;
	m_pAcquiredEffect.Reset();
}

void ImGuiRenderer::ReInitFontTexture()
{
	ImGui::GetIO().Fonts->TexID = nullptr;
	m_pFontTexture.Reset(InternalStaticCreateFontTexture());
	ImGui::GetIO().Fonts->TexID = (void*)m_pFontTexture.Get();
}

ImGuiRendererTextureData* ImGuiRenderer::ResolveTexture(FilePath filePath)
{
	filePath.SetType(FileType::LAST_TEXTURE_TYPE);

	ImGuiRendererTextureData* pData = nullptr;
	if (!m_tTextureData.GetValue(filePath, pData))
	{
		pData = SEOUL_NEW(MemoryBudgets::Editor) ImGuiRendererTextureData(
			TextureManager::Get()->GetTexture(filePath));
		SEOUL_VERIFY(m_tTextureData.Insert(filePath, pData).Second);
	}

	return pData;
}

void ImGuiRenderer::InternalBeginCustomRender()
{
	m_pBuilder->CommitEffectPass(m_pAcquiredEffect, m_Pass);
	m_pBuilder->EndEffectPass(m_pAcquiredEffect, m_Pass);
	m_Pass = EffectPass();
	m_pBuilder->EndEffect(m_pAcquiredEffect);
}

void ImGuiRenderer::InteranlEndCustomRender()
{
	m_Pass = m_pBuilder->BeginEffect(m_pAcquiredEffect, kEffectTechnique);
	m_pBuilder->BeginEffectPass(m_pAcquiredEffect, m_Pass);
	m_pBuilder->UseVertexFormat(m_pVertexFormat.GetPtr());
	m_pBuilder->SetIndices(m_pIndexBuffer.GetPtr());
	m_pBuilder->SetVertices(0, m_pVertexBuffer.GetPtr(), 0, sizeof(ImDrawVert));
}

} // namespace Seoul::DevUI

#endif // /SEOUL_ENABLE_DEV_UI
