/**
 * \file D3D11RenderCommandStreamBuilder.cpp
 * \brief Specialization of RenderCommandStreamBuilder for the D3D11 graphics system.
 * Handles execution of a command buffer of graphics commands with the D3D11 API.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ClearFlags.h"
#include "D3D11DepthStencilSurface.h"
#include "D3D11Device.h"
#include "D3D11IndexBuffer.h"
#include "D3D11RenderCommandStreamBuilder.h"
#include "D3D11RenderTarget.h"
#include "D3D11Texture.h"
#include "D3D11Util.h"
#include "D3D11VertexBuffer.h"
#include "D3D11VertexFormat.h"
#include "JobsFunction.h"
#include "SharedPtr.h"
#include "PixelFormat.h"
#include "RenderSurface.h"
#include "ThreadId.h"

#if !SEOUL_SHIP
#if SEOUL_PLATFORM_WINDOWS
// TODO: D3D11+ equivalent.
// #include <d3d9.h>
#endif // /#if SEOUL_PLATFORM_WINDOWS
#endif // /#if !SEOUL_SHIP

namespace Seoul
{

/**
 * Utility to determine if we need a reduce rectangle clip
 */
inline static Bool InternalStaticGetNeedsScissorClear(ID3D11DeviceContext* pContext)
{
	UINT uScissors = 1u;
	D3D11_RECT scissor;
	memset(&scissor, 0, sizeof(scissor));
	pContext->RSGetScissorRects(&uScissors, &scissor);

	Int32 iWidth = 0;
	Int32 iHeight = 0;
	RenderSurface2D::RenderThread_GetActiveSurfaceDimensions(iWidth, iHeight);

	if (scissor.left > 0 ||
		scissor.top > 0 ||
		scissor.right < iWidth ||
		scissor.bottom < iHeight)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * A device reset event can cause the back buffer dimensions to change
 * between the creation of a render command stream and its execution. This
 * function detects this case and adjust the viewport properties to account
 * for the change.
 */
inline static void InternalStaticAdjustViewportToCurrentTarget(Viewport& rViewport)
{
	// Sanity check that a resize did not occur - if it did, rescale the viewport.
	Int32 iCurrentTargetWidth = 0;
	Int32 iCurrentTargetHeight = 0;
	RenderSurface2D::RenderThread_GetActiveSurfaceDimensions(iCurrentTargetWidth, iCurrentTargetHeight);

	// If the target width change, rescale the viewportX and width values.
	if (rViewport.m_iTargetWidth != iCurrentTargetWidth)
	{
		rViewport.m_iViewportX = (Int32)(((Float)rViewport.m_iViewportX / (Float)rViewport.m_iTargetWidth) * (Float)iCurrentTargetWidth);
		rViewport.m_iViewportWidth = (Int32)(((Float)rViewport.m_iViewportWidth / (Float)rViewport.m_iTargetWidth) * (Float)iCurrentTargetWidth);
	}

	// If the target height changed, rescale the viewportY and height values.
	if (rViewport.m_iTargetHeight != iCurrentTargetHeight)
	{
		rViewport.m_iViewportY = (Int32)(((Float)rViewport.m_iViewportY / (Float)rViewport.m_iTargetHeight) * (Float)iCurrentTargetHeight);
		rViewport.m_iViewportHeight = (Int32)(((Float)rViewport.m_iViewportHeight / (Float)rViewport.m_iTargetHeight) * (Float)iCurrentTargetHeight);
	}
}

static void D3D11RenderCommandStreamBuilderCallReadPixel(
	const SharedPtr<IReadPixel>& pReadPixel,
	ColorARGBu8 cColor,
	Bool bSuccess)
{
	pReadPixel->OnReadPixel(cColor, bSuccess);
}

static void D3D11RenderCommandStreamBuilderCallGrabFrame(
	UInt32 uFrame,
	const SharedPtr<IGrabFrame>& pGrabFrame,
	const SharedPtr<IFrameData>& pFrameData,
	Bool bSuccess)
{
	pGrabFrame->OnGrabFrame(uFrame, pFrameData, bSuccess);
}

D3D11RenderCommandStreamBuilder::D3D11RenderCommandStreamBuilder(
	UInt32 zInitialCapacity)
	: RenderCommandStreamBuilder(zInitialCapacity)
{
}

D3D11RenderCommandStreamBuilder::~D3D11RenderCommandStreamBuilder()
{
}

void D3D11RenderCommandStreamBuilder::ExecuteCommandStream(RenderStats& rStats)
{
	SEOUL_ASSERT(IsRenderThread());

	memset(&rStats, 0, sizeof(rStats));

	String s;

	UInt32 const zStartingOffset = m_CommandStream.GetOffset();
	m_CommandStream.SeekToOffset(0u);

	OpCode eOpCode = kUnknown;

	D3D11Device& rDevice = GetD3D11Device();
	ID3D11Device* pDevice = rDevice.GetD3DDevice();
	ID3D11DeviceContext* pContext = rDevice.GetD3DDeviceContext();

	OpCode eLastOpCode = kUnknown;
	while (Read(eOpCode))
	{
		switch (eOpCode)
		{
		case kApplyDefaultRenderState:
			{
				// Clear state.
				rDevice.ClearState();

				// Now that we've unset everything, we need to restore the render target
				// and depth-stencil surface, since middleware typically needs these
				// to be set in order to have surfaces to draw to.
				RenderTarget* p = RenderTarget::GetActiveRenderTarget();
				if (nullptr != p)
				{
					((D3D11RenderTarget*)p)->Reselect();
				}

				DepthStencilSurface* pDepthStencil = DepthStencilSurface::GetActiveDepthStencilSurface();
				if (nullptr != pDepthStencil)
				{
					((D3D11DepthStencilSurface*)pDepthStencil)->Reselect();
				}

				// Commit any surface changes immediately.
				rDevice.CommitRenderSurface();
			}
			break;

		case kBeginEvent:
			SEOUL_VERIFY(m_CommandStream.Read(s));
#if !SEOUL_SHIP
#if SEOUL_PLATFORM_WINDOWS
			// TODO: D3D11+ equivalent.
			// (void)D3DPERF_BeginEvent(D3DCOLOR_RGBA(0, 0, 0, 0), s.WStr());
#endif // /#if SEOUL_PLATFORM_WINDOWS
#endif // /#if !SEOUL_SHIP
			break;

		case kClear:
			{
				UInt32 uFlags = 0u;
				Color4 cClearColor = Color4::Black();
				Float fClearDepth = 0.0f;
				UInt8 uClearStencil = 0u;

				SEOUL_VERIFY(Read(uFlags));
				SEOUL_VERIFY(Read(cClearColor));
				SEOUL_VERIFY(Read(fClearDepth));
				SEOUL_VERIFY(Read(uClearStencil));

				auto const bNeedsScissorClear = InternalStaticGetNeedsScissorClear(pContext);

				// If a scissor clear is necessary, do this by rendering a quad
				// with a custom shader.
				if (bNeedsScissorClear)
				{
					rDevice.ClearWithQuadRender(
						uFlags,
						cClearColor,
						fClearDepth,
						uClearStencil);
				}
				// Otherwise, clear with standard target clears.
				else
				{
					// Convert ClearFlags into D3DCLEAR
					if ((uFlags & (UInt32)ClearFlags::kColorTarget) == (UInt32)ClearFlags::kColorTarget)
					{
						// If we're clearing the backbuffer, mark that we have a frame to present.
						if (nullptr == RenderTarget::GetActiveRenderTarget())
						{
							rDevice.OnHasFrameToPresent();
						}

						auto pView = rDevice.m_CurrentRenderSurface.m_pRenderTarget;
						if (nullptr != pView)
						{
							pContext->ClearRenderTargetView(
								pView,
								cClearColor.GetData());
						}
					}

					DWORD uDepthClearFlags = 0u;
					if ((uFlags & (UInt32)ClearFlags::kDepthTarget) == (UInt32)ClearFlags::kDepthTarget)
					{
						uDepthClearFlags |= D3D11_CLEAR_DEPTH;
					}

					if ((uFlags & (UInt32)ClearFlags::kStencilTarget) == (UInt32)ClearFlags::kStencilTarget)
					{
						uDepthClearFlags |= D3D11_CLEAR_STENCIL;
					}

					if (0u != uDepthClearFlags)
					{
						auto pView = rDevice.m_CurrentRenderSurface.m_pDepthStencil;
						if (nullptr != pView)
						{
							pContext->ClearDepthStencilView(
								pView,
								uDepthClearFlags,
								fClearDepth,
								uClearStencil);
						}
					}
				}
			}
			break;

		case kPostPass:
			{
				UInt32 uClearFlags = 0u;
				SEOUL_VERIFY(Read(uClearFlags));
				(void)uClearFlags;

				// Nop
			}
			break;

		case kDrawPrimitive:
			{
				PrimitiveType eType = (PrimitiveType)0;
				UInt uOffset = 0u;
				UInt uNumPrimitives = 0u;
				SEOUL_VERIFY(Read(eType));
				SEOUL_VERIFY(Read(uOffset));
				SEOUL_VERIFY(Read(uNumPrimitives));

				rStats.m_uTrianglesSubmittedForDraw += uNumPrimitives;
				rStats.m_uDrawsSubmitted++;

				pContext->IASetPrimitiveTopology(PrimitiveTypeToD3D11Type(eType));
				pContext->Draw(
					GetNumberOfVertices(eType, uNumPrimitives),
					uOffset);
			}
			break;

		case kDrawIndexedPrimitive:
			{
				PrimitiveType eType = (PrimitiveType)0;
				Int iOffset = 0;
				UInt uMinIndex = 0u;
				UInt uNumVerts = 0u;
				UInt uStartIndex = 0u;
				UInt uNumPrimitives = 0u;
				SEOUL_VERIFY(Read(eType));
				SEOUL_VERIFY(Read(iOffset));
				SEOUL_VERIFY(Read(uMinIndex));
				SEOUL_VERIFY(Read(uNumVerts));
				SEOUL_VERIFY(Read(uStartIndex));
				SEOUL_VERIFY(Read(uNumPrimitives));

				rStats.m_uTrianglesSubmittedForDraw += uNumPrimitives;
				rStats.m_uDrawsSubmitted++;

				pContext->IASetPrimitiveTopology(PrimitiveTypeToD3D11Type(eType));
				pContext->DrawIndexed(
					GetNumberOfIndices(eType, uNumPrimitives),
					uStartIndex,
					iOffset);
			}
			break;

		case kEndEvent:
#if !SEOUL_SHIP
#if SEOUL_PLATFORM_WINDOWS
			// TODO: D3D11+ equivalent.
			// (void)D3DPERF_EndEvent();
#endif // /#if SEOUL_PLATFORM_WINDOWS
#endif // /#if !SEOUL_SHIP
			break;

		case kLockIndexBuffer:
			{
				UInt32 uDataSizeInBytes = 0u;
				IndexBuffer* pIndexBuffer = nullptr;
				SEOUL_VERIFY(Read(pIndexBuffer));
				SEOUL_VERIFY(Read(uDataSizeInBytes));

				AlignReadOffset();

				D3D11IndexBuffer* pBuffer = static_cast<D3D11IndexBuffer*>(pIndexBuffer);
				CheckedPtr<ID3D11Buffer> p = pBuffer->m_pIndexBuffer;

				D3D11_MAPPED_SUBRESOURCE map;
				memset(&map, 0, sizeof(map));
				SEOUL_D3D11_VERIFY(pContext->Map(
					p,
					0u,
					(pBuffer->m_bDynamic ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE),
					0u,
					&map));
				memcpy(map.pData, m_CommandStream.GetBuffer() + m_CommandStream.GetOffset(), uDataSizeInBytes);
				pContext->Unmap(p, 0u);

				m_CommandStream.SeekToOffset(m_CommandStream.GetOffset() + uDataSizeInBytes);
			}
			break;

		case kUnlockIndexBuffer:
			{
				IndexBuffer* pIndexBuffer = nullptr;
				SEOUL_VERIFY(Read(pIndexBuffer));
				(void)pIndexBuffer;

				// Nop
			}
			break;

		case kLockTexture:
			{
				BaseTexture* pTexture = nullptr;
				UInt uLevel = 0u;
				Rectangle2DInt rectangle;
				memset(&rectangle, 0, sizeof(rectangle));

				SEOUL_VERIFY(Read(pTexture));
				SEOUL_VERIFY(Read(uLevel));
				SEOUL_VERIFY(Read(rectangle));

				AlignReadOffset();

				Byte* const pSource = (m_CommandStream.GetBuffer() + m_CommandStream.GetOffset());

				UInt32 const uDataSizeInBytes = UpdateTexture(
					pContext,
					pTexture,
					uLevel,
					rectangle,
					pSource);

				m_CommandStream.SeekToOffset(m_CommandStream.GetOffset() + uDataSizeInBytes);
			}
			break;

		case kUnlockTexture:
			{
				BaseTexture* pTexture = nullptr;
				UInt uLevel = 0u;

				SEOUL_VERIFY(Read(pTexture));
				SEOUL_VERIFY(Read(uLevel));
				(void)pTexture;
				(void)uLevel;

				// Nop
			}
			break;

		case kUpdateTexture:
			{
				BaseTexture* pTexture = nullptr;
				UInt uLevel = 0u;
				Rectangle2DInt rectangle;
				void* pBuffer = nullptr;
				memset(&rectangle, 0, sizeof(rectangle));

				SEOUL_VERIFY(Read(pTexture));
				SEOUL_VERIFY(Read(uLevel));
				SEOUL_VERIFY(Read(rectangle));
				SEOUL_VERIFY(Read(pBuffer));

				(void)UpdateTexture(
					pContext,
					pTexture,
					uLevel,
					rectangle,
					(Byte const*)pBuffer);
			}
			break;

		case kLockVertexBuffer:
			{
				UInt32 uDataSizeInBytes = 0u;
				VertexBuffer* pVertexBuffer = nullptr;
				SEOUL_VERIFY(Read(pVertexBuffer));
				SEOUL_VERIFY(Read(uDataSizeInBytes));

				AlignReadOffset();

				D3D11VertexBuffer* pBuffer = static_cast<D3D11VertexBuffer*>(pVertexBuffer);
				CheckedPtr<ID3D11Buffer> p = pBuffer->m_pVertexBuffer;

				D3D11_MAPPED_SUBRESOURCE map;
				memset(&map, 0, sizeof(map));
				SEOUL_D3D11_VERIFY(pContext->Map(
					p,
					0u,
					(pBuffer->m_bDynamic ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE),
					0u,
					&map));
				memcpy(map.pData, m_CommandStream.GetBuffer() + m_CommandStream.GetOffset(), uDataSizeInBytes);
				pContext->Unmap(p, 0u);

				m_CommandStream.SeekToOffset(m_CommandStream.GetOffset() + uDataSizeInBytes);
			}
			break;

		case kUnlockVertexBuffer:
			{
				VertexBuffer* pVertexBuffer = nullptr;
				SEOUL_VERIFY(Read(pVertexBuffer));
				(void)pVertexBuffer;

				// Nop
			}
			break;

		case kResolveDepthStencilSurface:
			{
				DepthStencilSurface* p = nullptr;
				SEOUL_VERIFY(Read(p));
				p->Resolve();
			}
			break;

		case kSelectDepthStencilSurface:
			{
				DepthStencilSurface* p = nullptr;
				SEOUL_VERIFY(Read(p));
				if (nullptr != p)
				{
					p->Select();
				}
				else
				{
					DepthStencilSurface* pDepthStencilSurface = DepthStencilSurface::GetActiveDepthStencilSurface();
					if (nullptr != pDepthStencilSurface)
					{
						pDepthStencilSurface->Unselect();
					}
				}
			}
			break;

		case kResolveRenderTarget:
			{
				RenderTarget* p = nullptr;
				SEOUL_VERIFY(Read(p));
				p->Resolve();
			}
			break;

		case kSelectRenderTarget:
			{
				RenderTarget* p = nullptr;
				SEOUL_VERIFY(Read(p));
				if (nullptr != p)
				{
					p->Select();
				}
				else
				{
					RenderTarget* pRenderTarget = RenderTarget::GetActiveRenderTarget();
					if (nullptr != pRenderTarget)
					{
						pRenderTarget->Unselect();
					}
				}
			}
			break;

		case kCommitRenderSurface:
			rDevice.CommitRenderSurface();
			break;

		case kBeginEffect:
			{
				rStats.m_uEffectBegins++;

				ID3DX11Effect* e = nullptr;
				ID3DX11EffectTechnique* pEffectTechnique = nullptr;
				SEOUL_VERIFY(ReadEffectPair(e, pEffectTechnique));

				if (nullptr != e)
				{
					e->SetActiveEffectTechnique(pEffectTechnique);
				}
			}
			break;

		case kEndEffect:
			{
				UnsafeHandle hEffect;
				SEOUL_VERIFY(Read(hEffect));
				ID3DX11Effect* e = StaticCast<ID3DX11Effect*>(hEffect);

				e->SetActiveEffectTechnique(nullptr);
			}
			break;

		case kBeginEffectPass:
			{
				UnsafeHandle hEffect;
				UInt16 n = 0u;
				UInt16 count = 0u;
				SEOUL_VERIFY(Read(hEffect));
				SEOUL_VERIFY(Read(n));
				SEOUL_VERIFY(Read(count));

				auto pEffect = StaticCast<ID3DX11Effect*>(hEffect);
				auto pTechnique = pEffect->GetActiveEffectTechnique();
				auto pPass = pTechnique->GetPassByIndex(n);

				// TODO: This needs to be removed once we've moved away from D3D9 style effects
				// and reduce effects down to an Apply() call in the public API
				SEOUL_D3D11_VERIFY(pPass->Apply(0u, pContext));
			}
			break;

		case kCommitEffectPass:
			{
				UnsafeHandle hEffect;
				UInt16 n = 0u;
				UInt16 count = 0u;
				SEOUL_VERIFY(Read(hEffect));
				SEOUL_VERIFY(Read(n));
				SEOUL_VERIFY(Read(count));

				auto pEffect = StaticCast<ID3DX11Effect*>(hEffect);
				auto pTechnique = pEffect->GetActiveEffectTechnique();
				auto pPass = pTechnique->GetPassByIndex(n);

				D3DX11_PASS_DESC passDescription;
				memset(&passDescription, 0, sizeof(passDescription));
				SEOUL_D3D11_VERIFY(pPass->GetDesc(&passDescription));

				SEOUL_D3D11_VERIFY(pPass->Apply(0u, pContext));
			}
			break;

		case kEndEffectPass:
			{
				UnsafeHandle hEffect;
				UInt16 n = 0u;
				UInt16 count = 0u;
				SEOUL_VERIFY(Read(hEffect));
				SEOUL_VERIFY(Read(n));
				SEOUL_VERIFY(Read(count));

				auto pEffect = StaticCast<ID3DX11Effect*>(hEffect);
				auto pTechnique = pEffect->GetActiveEffectTechnique();
				auto pPass = pTechnique->GetPassByIndex(n);
			}
			break;

		case kSetFloatParameter:
			{
				ID3DX11Effect* e = nullptr;
				ID3DX11EffectVariable* p = nullptr;
				Float fValue = 0.0f;
				SEOUL_VERIFY(ReadEffectPair(e, p));
				SEOUL_VERIFY(Read(fValue));
				SEOUL_D3D11_VERIFY(p->AsScalar()->SetFloat(fValue));
			}
			break;

		case kSetMatrix3x4ArrayParameter:
			{
				ID3DX11Effect* e = nullptr;
				ID3DX11EffectVariable* p = nullptr;
				UInt32 uCount = 0u;
				SEOUL_VERIFY(ReadEffectPair(e, p));
				SEOUL_VERIFY(Read(uCount));
				AlignReadOffset();
				SEOUL_D3D11_VERIFY(p->SetRawValue(
					m_CommandStream.GetBuffer() + m_CommandStream.GetOffset(),
					0u,
					uCount * sizeof(Matrix3x4)));

				m_CommandStream.SeekToOffset(m_CommandStream.GetOffset() + uCount * sizeof(Matrix3x4));
			}
			break;

		case kSetMatrix4DParameter:
			{
				ID3DX11Effect* e = nullptr;
				ID3DX11EffectVariable* p = nullptr;
				Matrix4D mValue = Matrix4D::Zero();
				SEOUL_VERIFY(ReadEffectPair(e, p));
				SEOUL_VERIFY(Read(mValue));
				SEOUL_D3D11_VERIFY(p->AsMatrix()->SetMatrix(mValue.GetData()));
			}
			break;

		case kSetTextureParameter:
			{
				ID3DX11Effect* e = nullptr;
				ID3DX11EffectVariable* p = nullptr;
				BaseTexture* pTexture = nullptr;
				SEOUL_VERIFY(ReadEffectPair(e, p));
				SEOUL_VERIFY(Read(pTexture));

				SEOUL_D3D11_VERIFY(p->AsShaderResource()->SetResource((nullptr != pTexture)
					? StaticCast<ID3D11ShaderResourceView*>(pTexture->GetTextureHandle())
					: nullptr));
			}
			break;

		case kSetVector4DParameter:
			{
				ID3DX11Effect* e = nullptr;
				ID3DX11EffectVariable* p = nullptr;
				Vector4D vValue = Vector4D::Zero();
				SEOUL_VERIFY(ReadEffectPair(e, p));
				SEOUL_VERIFY(Read(vValue));
				SEOUL_D3D11_VERIFY(p->AsVector()->SetFloatVector(vValue.GetData()));
			}
			break;

		case kSetCurrentViewport:
			{
				Viewport viewport;
				SEOUL_VERIFY(Read(viewport));

				// Rescale the viewport if needed (due to a window resize event).
				InternalStaticAdjustViewportToCurrentTarget(viewport);
				D3D11_VIEWPORT d3d11Viepwort = Convert(viewport);
				pContext->RSSetViewports(1u, &d3d11Viepwort);
			}
			break;

		case kSetScissor:
			{
				Bool bEnabled = false;
				Viewport viewport;
				SEOUL_VERIFY(Read(bEnabled));
				SEOUL_VERIFY(Read(viewport));

				if (bEnabled)
				{
					// Rescale the viewport if needed (due to a window resize event).
					InternalStaticAdjustViewportToCurrentTarget(viewport);

					// Compute the scissor rectangle.
					D3D11_RECT d3d11Rectangle = Convert(Rectangle2DInt(
						viewport.m_iViewportX,
						viewport.m_iViewportY,
						viewport.m_iViewportX + viewport.m_iViewportWidth,
						viewport.m_iViewportY + viewport.m_iViewportHeight));
					pContext->RSSetScissorRects(1u, &d3d11Rectangle);
				}
				else
				{
					// Sanity check that a resize did not occur - if it did, rescale the viewport.
					Int32 iCurrentTargetWidth = 0;
					Int32 iCurrentTargetHeight = 0;
					RenderSurface2D::RenderThread_GetActiveSurfaceDimensions(iCurrentTargetWidth, iCurrentTargetHeight);

					D3D11_RECT d3d11Rectangle = Convert(Rectangle2DInt(
						0,
						0,
						iCurrentTargetWidth,
						iCurrentTargetHeight));
					pContext->RSSetScissorRects(1u, &d3d11Rectangle);
				}
			}
			break;

		case kSetNullIndices:
			// Nop
			break;

		case kSetIndices:
			{
				IndexBuffer* pIndexBuffer = nullptr;
				SEOUL_VERIFY(Read(pIndexBuffer));
				pContext->IASetIndexBuffer(
					((D3D11IndexBuffer*)pIndexBuffer)->m_pIndexBuffer,
					(((D3D11IndexBuffer*)pIndexBuffer)->m_eFormat == IndexBufferDataFormat::kIndex32) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT,
					0u);
			}
			break;

		case kSetNullVertices:
			{
				UInt nStreamNumber = 0u;
				SEOUL_VERIFY(Read(nStreamNumber));

				// Nop
			}
			break;

		case kSetVertices:
			{
				UInt nStreamNumber = 0u;
				VertexBuffer* pBuffer = nullptr;
				UInt zOffsetInBytes = 0u;
				UInt zStrideInBytes = 0u;
				SEOUL_VERIFY(Read(nStreamNumber));
				SEOUL_VERIFY(Read(pBuffer));
				SEOUL_VERIFY(Read(zOffsetInBytes));
				SEOUL_VERIFY(Read(zStrideInBytes));

				D3D11VertexBuffer const* pD3DBuffer = static_cast<D3D11VertexBuffer const*>(pBuffer);

				ID3D11Buffer* pD3D11Buffer = pD3DBuffer->m_pVertexBuffer.Get();
				pContext->IASetVertexBuffers(
					nStreamNumber,
					1u,
					&pD3D11Buffer,
					&zStrideInBytes,
					&zOffsetInBytes);
			}
			break;

		case kUseVertexFormat:
			{
				VertexFormat* p = nullptr;
				SEOUL_VERIFY(Read(p));

				if (p)
				{
					D3D11VertexFormat* pFormat = static_cast<D3D11VertexFormat*>(p);
					pContext->IASetInputLayout(pFormat->m_pInputLayout);
				}
				else
				{
					pContext->IASetInputLayout(nullptr);
				}
			}
			break;

		case kReadBackBufferPixel:
			{
				Int32 iX = 0;
				Int32 iY = 0;
				IReadPixel* p = nullptr;
				ThreadId callbackThreadId;
				SEOUL_VERIFY(Read(iX));
				SEOUL_VERIFY(Read(iY));
				SEOUL_VERIFY(Read(p));
				SEOUL_VERIFY(Read(callbackThreadId));

				ColorARGBu8 cColor;
				Bool const bSuccess = rDevice.ReadBackBufferPixel(iX, iY, cColor);

				if (nullptr != p)
				{
					Jobs::AsyncFunction(
						callbackThreadId,
						&D3D11RenderCommandStreamBuilderCallReadPixel,
						SharedPtr<IReadPixel>(p),
						cColor,
						bSuccess);
				}
			}
			break;

		case kGrabBackBufferFrame:
			{
				UInt32 uFrame = 0u;
				Rectangle2DInt rect;
				IGrabFrame* p = nullptr;
				ThreadId callbackThreadId;
				SEOUL_VERIFY(Read(uFrame));
				SEOUL_VERIFY(Read(rect));
				SEOUL_VERIFY(Read(p));
				SEOUL_VERIFY(Read(callbackThreadId));

				SharedPtr<IFrameData> pFrame;
				Bool const bSuccess = rDevice.GrabBackBufferFrame(rect, pFrame);

				if (nullptr != p)
				{
					Jobs::AsyncFunction(
						callbackThreadId,
						&D3D11RenderCommandStreamBuilderCallGrabFrame,
						uFrame,
						SharedPtr<IGrabFrame>(p),
						pFrame,
						bSuccess);
				}
			}
			break;

		case kUpdateOsWindowRegions:
		{
			UInt32 uCount = 0u;
			OsWindowRegion const* pRegions = nullptr;
			SEOUL_VERIFY(Read(uCount));
			if (uCount > 0u)
			{
				AlignReadOffset();
				pRegions = (OsWindowRegion const*)(m_CommandStream.GetBuffer() + m_CommandStream.GetOffset());
				m_CommandStream.SeekToOffset(m_CommandStream.GetOffset() + (uCount * sizeof(*pRegions)));
			}

			rDevice.UpdateOsWindowRegions(pRegions, uCount);
		}
		break;

		case kUnknown: // fall-through
		default:
			SEOUL_FAIL(__FUNCTION__ ": Unknown OpCode");
			break;
		};

		eLastOpCode = eOpCode;
	}

	SEOUL_ASSERT(zStartingOffset == m_CommandStream.GetOffset());
}

UInt32 D3D11RenderCommandStreamBuilder::UpdateTexture(
	ID3D11DeviceContext* pContext,
	BaseTexture* pTexture,
	UInt uLevel,
	const Rectangle2DInt& rectangle,
	Byte const* pSource)
{
	D3D11Texture* pTexture2d = static_cast<D3D11Texture*>(pTexture);
	CheckedPtr<ID3D11Resource> p = pTexture2d->m_pTexture;

	UInt32 const uRectangleWidth = (UInt32)(rectangle.m_iRight - rectangle.m_iLeft);
	UInt32 const uRectangleHeight = (UInt32)(rectangle.m_iBottom - rectangle.m_iTop);
	PixelFormat const eFormat = pTexture->GetFormat();
	UInt32 uBytesPerPixel = 0u;
	SEOUL_VERIFY(PixelFormatBytesPerPixel(eFormat, uBytesPerPixel));
	UInt32 const uDataSizeInBytes = GetDataSizeForPixelFormat(
		uRectangleWidth,
		uRectangleHeight,
		eFormat);
	UInt32 const zDataPitchInBytes = (uBytesPerPixel * uRectangleWidth);

	D3D11_MAPPED_SUBRESOURCE lockedRectangle;
	memset(&lockedRectangle, 0, sizeof(lockedRectangle));

	if (pTexture2d->m_bDynamic)
	{
		SEOUL_D3D11_VERIFY(pContext->Map(p, uLevel, D3D11_MAP_WRITE_DISCARD, 0u, &lockedRectangle));

		Byte* pDestination = (Byte*)lockedRectangle.pData + (lockedRectangle.RowPitch * rectangle.m_iTop) + (rectangle.m_iLeft);

		for (UInt32 i = 0u; i < uRectangleHeight; ++i)
		{
			// lockedRectangle.pData is pointing at the top left corner of the locked rectangle.
			memcpy(
				pDestination + (i * lockedRectangle.RowPitch),
				pSource + (i * zDataPitchInBytes),
				zDataPitchInBytes);
		}
		pContext->Unmap(p, uLevel);
	}
	else
	{
		// Initialize the D3D11_BOX to the destination rectangle.
		D3D11_BOX destinationBox;
		memset(&destinationBox, 0, sizeof(destinationBox));
		destinationBox.back = 1;
		destinationBox.front = 0;
		destinationBox.bottom = rectangle.m_iBottom;
		destinationBox.left = rectangle.m_iLeft;
		destinationBox.right = rectangle.m_iRight;
		destinationBox.top = rectangle.m_iTop;

		// Update the destination rectangle in the sub resource.
		pContext->UpdateSubresource(
			p,
			uLevel,
			&destinationBox,
			pSource,
			zDataPitchInBytes,
			0u);
	}

	return uDataSizeInBytes;
}

} // namespace Seoul
