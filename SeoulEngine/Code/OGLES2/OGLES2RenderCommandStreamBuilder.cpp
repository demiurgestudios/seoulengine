/**
 * \file OGLES2RenderCommandStreamBuilder.cpp
 * \brief Specialization of RenderCommandStreamBuilder for the Open GL ES2 graphics system.
 * Handles execution of a command buffer of graphics commands with the ES2 api.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ClearFlags.h"
#include "JobsFunction.h"
#include "GLSLFXLiteInternal.h"
#include "OGLES2DepthStencilSurface.h"
#include "OGLES2IndexBuffer.h"
#include "OGLES2RenderCommandStreamBuilder.h"
#include "OGLES2RenderDevice.h"
#include "OGLES2RenderTarget.h"
#include "OGLES2StateManager.h"
#include "OGLES2Texture.h"
#include "OGLES2Util.h"
#include "OGLES2VertexBuffer.h"
#include "OGLES2VertexFormat.h"
#include "SharedPtr.h"
#include "PixelFormat.h"
#include "RenderSurface.h"
#include "ThreadId.h"
#include "UnsafeHandle.h"

namespace Seoul
{

// NOTE: Please cut and paste if needed in new places, this function is very specific to its use case and is not general purpose.

/**
 * Swap the RB channels of an input buffer, in either BGRA or RGBA format.
 */
inline static void InternalStaticSwapR8B8(Byte* pData, UInt32 zWidth, UInt32 zHeight)
{
	for (UInt32 y = 0u; y < zHeight; ++y)
	{
		for (UInt32 x = 0u; x < zWidth; ++x)
		{
			Byte* pColor = (pData + (y * zWidth * 4) + (x * 4));
			Swap(pColor[0], pColor[2]);
		}
	}
}

/**
 * @return A Cell Gcm enum value which corresponds to the Seoul
 * primitive type eType.
 */
inline static UInt8 InlineStaticGetPrimitiveType(PrimitiveType eType)
{
	switch (eType)
	{
	case PrimitiveType::kPointList:
		return GL_POINTS;
	case PrimitiveType::kLineList:
		return GL_LINES;
	case PrimitiveType::kLineStrip:
		return GL_LINE_STRIP;
	case PrimitiveType::kTriangleList:
		return GL_TRIANGLES;

	case PrimitiveType::kNone: // fall-through
	default:
		return 0u;
	};
}

static void OGLES2RenderCommandStreamBuilderCallReadPixel(
	const SharedPtr<IReadPixel>& pReadPixel,
	ColorARGBu8 cColor,
	Bool bSuccess)
{
	pReadPixel->OnReadPixel(cColor, bSuccess);
}

static void OGLES2RenderCommandStreamBuilderCallGrabFrame(
	UInt32 uFrame,
	const SharedPtr<IGrabFrame>& pGrabFrame,
	const SharedPtr<IFrameData>& pFrameData,
	Bool bSuccess)
{
	pGrabFrame->OnGrabFrame(uFrame, pFrameData, bSuccess);
}

OGLES2RenderCommandStreamBuilder::OGLES2RenderCommandStreamBuilder(
	UInt32 zInitialCapacity)
	: RenderCommandStreamBuilder(zInitialCapacity)
	, m_aActiveStreams()
	, m_uActiveMinVertexIndex(0)
	, m_aCommittedStreams()
	, m_pActiveVertexFormat(nullptr)
	, m_pCommitedVertexFormat(nullptr)
	, m_abActiveVertexAttributes(false)
{
}

OGLES2RenderCommandStreamBuilder::~OGLES2RenderCommandStreamBuilder()
{
}

void OGLES2RenderCommandStreamBuilder::ExecuteCommandStream(RenderStats& rStats)
{
	SEOUL_ASSERT(IsRenderThread());

	m_aActiveStreams.Fill(VertexStream());
	m_uActiveMinVertexIndex = 0u;
	m_aCommittedStreams.Fill(VertexStream());
	m_pActiveVertexFormat = nullptr;
	m_pCommitedVertexFormat = nullptr;
	m_pActiveIndexBuffer = nullptr;

	memset(&rStats, 0, sizeof(rStats));

	String s;

	UInt32 const zStartingOffset = m_CommandStream.GetOffset();
	m_CommandStream.SeekToOffset(0u);

	OpCode eOpCode = kUnknown;

	Bool bLastScissorEnabled = false;
	Viewport lastScissorViewport(Viewport::Create(0, 0, 0, 0, 0, 0));

	OGLES2RenderDevice& rDevice = GetOGLES2RenderDevice();
	OGLES2StateManager& rStateManager = rDevice.GetStateManager();

	while (m_CommandStream.GetOffset() < zStartingOffset && Read(eOpCode))
	{
		switch (eOpCode)
		{
		case kApplyDefaultRenderState:
			{
				rStateManager.ApplyDefaultRenderStates();

				// Now that we've unset everything, we need to restore the render target
				// and depth-stencil surface, since middleware typically needs these
				// to be set in order to have surfaces to draw to.
				OGLES2RenderTarget* pRenderTarget = (nullptr == RenderTarget::GetActiveRenderTarget()
					? nullptr
					: ((OGLES2RenderTarget*)RenderTarget::GetActiveRenderTarget()));
				rDevice.SetRenderTarget(pRenderTarget);

				OGLES2DepthStencilSurface* pDepthStencilSurface = (nullptr == DepthStencilSurface::GetActiveDepthStencilSurface()
					? nullptr
					: ((OGLES2DepthStencilSurface*)DepthStencilSurface::GetActiveDepthStencilSurface()));
				rDevice.SetDepthStencilSurface(pDepthStencilSurface);
			}
			break;

		case kBeginEvent:
			SEOUL_VERIFY(m_CommandStream.Read(s));
			rDevice.PushGroupMarker(s);
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

				GLbitfield uClearBits = 0u;
				uClearBits |= ((UInt32)ClearFlags::kColorTarget == ((UInt32)ClearFlags::kColorTarget & uFlags)) ? GL_COLOR_BUFFER_BIT : 0u;
				uClearBits |= ((UInt32)ClearFlags::kDepthTarget == ((UInt32)ClearFlags::kDepthTarget & uFlags)) ? GL_DEPTH_BUFFER_BIT : 0u;
				uClearBits |= ((UInt32)ClearFlags::kStencilTarget == ((UInt32)ClearFlags::kStencilTarget & uFlags)) ? GL_STENCIL_BUFFER_BIT : 0u;

				if (GL_COLOR_BUFFER_BIT == (GL_COLOR_BUFFER_BIT & uClearBits))
				{
					SEOUL_OGLES2_VERIFY(glClearColor(cClearColor.R, cClearColor.G, cClearColor.B, cClearColor.A));

					UInt32 colorWriteBits = 0u;
					RenderStateUtil::SetComponent8<Components8Bit::kColorMaskR>(GL_TRUE, colorWriteBits);
					RenderStateUtil::SetComponent8<Components8Bit::kColorMaskG>(GL_TRUE, colorWriteBits);
					RenderStateUtil::SetComponent8<Components8Bit::kColorMaskB>(GL_TRUE, colorWriteBits);
					RenderStateUtil::SetComponent8<Components8Bit::kColorMaskA>(GL_TRUE, colorWriteBits);

					rStateManager.SetRenderState(
						RenderState::kColorWriteEnable,
						colorWriteBits);

					// If we're clearing the backbuffer, mark that we have a frame to present.
					if (nullptr == RenderTarget::GetActiveRenderTarget())
					{
						rDevice.m_bHasFrameToPresent = true;
					}
				}

				if (GL_DEPTH_BUFFER_BIT == (GL_DEPTH_BUFFER_BIT & uClearBits))
				{
					SEOUL_OGLES2_VERIFY(glClearDepthf(fClearDepth));

					rStateManager.SetRenderState(
						RenderState::kDepthWriteEnable,
						GL_TRUE);
					rStateManager.SetRenderState(
						RenderState::kDepthEnable,
						GL_TRUE);
				}

				if (GL_STENCIL_BUFFER_BIT == (GL_STENCIL_BUFFER_BIT & uClearBits))
				{
					rStateManager.SetRenderState(RenderState::kTwoSidedStencilMode, GL_FALSE);
					rStateManager.SetRenderState(RenderState::kStencilMask, 0x000000FF);
					rStateManager.SetRenderState(RenderState::kStencilWriteMask, 0x000000FF);
					rStateManager.SetRenderState(RenderState::kStencilEnable, GL_TRUE);
					SEOUL_OGLES2_VERIFY(glClearStencil(uClearStencil));
				}

				if (0u != uClearBits)
				{
					// Commit states before a clear issue.
					rStateManager.CommitPendingStates();

					SEOUL_OGLES2_VERIFY(glClear(uClearBits));
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
				UInt nNumPrimitives = 0u;
				SEOUL_VERIFY(Read(eType));
				SEOUL_VERIFY(Read(uOffset));
				SEOUL_VERIFY(Read(nNumPrimitives));

				rStats.m_uTrianglesSubmittedForDraw += nNumPrimitives;
				rStats.m_uDrawsSubmitted++;

				InternalCommitVertexStreams(0u);

				// Commit states before a draw issue.
				rStateManager.CommitPendingStates();

				SEOUL_OGLES2_VERIFY(glDrawArrays(
					InlineStaticGetPrimitiveType(eType),
					uOffset,
					GetNumberOfVertices(eType, nNumPrimitives)));
			}
			break;

		case kDrawIndexedPrimitive:
			{
				PrimitiveType eType = (PrimitiveType)0;
				Int iOffset = 0;
				UInt uMinIndex = 0u;
				UInt nNumVerts = 0u;
				UInt uStartIndex = 0u;
				UInt nNumPrimitives = 0u;
				SEOUL_VERIFY(Read(eType));
				SEOUL_VERIFY(Read(iOffset));
				SEOUL_VERIFY(Read(uMinIndex));
				SEOUL_VERIFY(Read(nNumVerts));
				SEOUL_VERIFY(Read(uStartIndex));
				SEOUL_VERIFY(Read(nNumPrimitives));

				rStats.m_uTrianglesSubmittedForDraw += nNumPrimitives;
				rStats.m_uDrawsSubmitted++;

				InternalCommitVertexStreams(iOffset);

				// Commit states before a draw issue.
				rStateManager.CommitPendingStates();

				// Use the active index buffer's dynamic data
				// for the offset - either it will be nullptr, in which case
				// we have a buffer object and we're specifying offsets,
				// or it will be a pointer and we're specifying pointers into
				// an interleaved buffer.
				SEOUL_OGLES2_VERIFY(glDrawElements(
					InlineStaticGetPrimitiveType(eType),
					GetNumberOfIndices(eType, nNumPrimitives),
					GL_UNSIGNED_SHORT,
					(m_pActiveIndexBuffer->m_pDynamicData + uStartIndex * sizeof(UInt16))));
			}
			break;

		case kEndEvent:
			rDevice.PopGroupMarker();
			break;

		case kLockIndexBuffer:
			{
				UInt32 zDataSizeInBytes = 0u;
				IndexBuffer* pIndexBuffer = nullptr;
				SEOUL_VERIFY(Read(pIndexBuffer));
				SEOUL_VERIFY(Read(zDataSizeInBytes));

				AlignReadOffset();

				OGLES2IndexBuffer* pBuffer = static_cast<OGLES2IndexBuffer*>(pIndexBuffer);

				// If the buffer is a dynamic buffer, just copy the data
				// into the system memory area.
				if (pBuffer->m_pDynamicData != nullptr)
				{
					memcpy(pBuffer->m_pDynamicData, m_CommandStream.GetBuffer() + m_CommandStream.GetOffset(), zDataSizeInBytes);
				}
				// Otherwise, update the OpenGl object.
				else
				{
					SEOUL_OGLES2_VERIFY(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pBuffer->m_IndexBuffer));
					SEOUL_OGLES2_VERIFY(glBufferSubData(
						GL_ELEMENT_ARRAY_BUFFER,
						0,
						zDataSizeInBytes,
						m_CommandStream.GetBuffer() + m_CommandStream.GetOffset()));
					SEOUL_OGLES2_VERIFY(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
				}

				m_CommandStream.SeekToOffset(m_CommandStream.GetOffset() + zDataSizeInBytes);
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

				Byte* pSource = (m_CommandStream.GetBuffer() + m_CommandStream.GetOffset());

				UInt32 const zDataSizeInBytes = UpdateTexture(
					rStateManager,
					pTexture,
					uLevel,
					rectangle,
					pSource);

				m_CommandStream.SeekToOffset(m_CommandStream.GetOffset() + zDataSizeInBytes);
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
					rStateManager,
					pTexture,
					uLevel,
					rectangle,
					(Byte*)pBuffer);
			}
			break;

		case kLockVertexBuffer:
			{
				UInt32 zDataSizeInBytes = 0u;
				VertexBuffer* pVertexBuffer = nullptr;
				SEOUL_VERIFY(Read(pVertexBuffer));
				SEOUL_VERIFY(Read(zDataSizeInBytes));

				AlignReadOffset();

				OGLES2VertexBuffer* pBuffer = static_cast<OGLES2VertexBuffer*>(pVertexBuffer);

				// If the buffer is a dynamic buffer, just copy the data
				// into the system memory area.
				if (pBuffer->m_pDynamicData != nullptr)
				{
					memcpy(pBuffer->m_pDynamicData, m_CommandStream.GetBuffer() + m_CommandStream.GetOffset(), zDataSizeInBytes);
				}
				// Otherwise, update the OpenGl object.
				else
				{
					SEOUL_OGLES2_VERIFY(glBindBuffer(GL_ARRAY_BUFFER, pBuffer->m_VertexBuffer));
					SEOUL_OGLES2_VERIFY(glBufferSubData(
						GL_ARRAY_BUFFER,
						0,
						zDataSizeInBytes,
						m_CommandStream.GetBuffer() + m_CommandStream.GetOffset()));
					SEOUL_OGLES2_VERIFY(glBindBuffer(GL_ARRAY_BUFFER, 0));
				}

				m_CommandStream.SeekToOffset(m_CommandStream.GetOffset() + zDataSizeInBytes);
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
					if (DepthStencilSurface::GetActiveDepthStencilSurface())
					{
						DepthStencilSurface::GetActiveDepthStencilSurface()->Unselect();
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
					p = RenderTarget::GetActiveRenderTarget();
					if (nullptr != p)
					{
						p->Unselect();
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

				GLSLFXLite* e = nullptr;
				UnsafeHandle t;
				SEOUL_VERIFY(ReadEffectParameter(e, t));

				if (e)
				{
					e->BeginTechnique(t);
				}
			}
			break;

		case kEndEffect:
			{
				UnsafeHandle hEffect;
				SEOUL_VERIFY(Read(hEffect));

				StaticCast<GLSLFXLite*>(hEffect)->EndTechnique();
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

				GLSLFXLite* p = StaticCast<GLSLFXLite*>(hEffect);
				p->BeginPassFromIndex(n);
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

				StaticCast<GLSLFXLite*>(hEffect)->Commit();
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

				GLSLFXLite* p = StaticCast<GLSLFXLite*>(hEffect);
				SEOUL_ASSERT(p);
				p->EndPass();
			}
			break;

		case kSetFloatParameter:
			{
				GLSLFXLite* pEffect = nullptr;
				UnsafeHandle hParameter;
				Float fValue = 0.0f;
				SEOUL_VERIFY(ReadEffectParameter(pEffect, hParameter));
				SEOUL_VERIFY(Read(fValue));
				pEffect->SetFloat(hParameter, fValue);
			}
			break;

		case kSetMatrix3x4ArrayParameter:
			{
				GLSLFXLite* pEffect = nullptr;
				UnsafeHandle hParameter;
				UInt32 uCount = 0u;
				SEOUL_VERIFY(ReadEffectParameter(pEffect, hParameter));
				SEOUL_VERIFY(Read(uCount));
				AlignReadOffset();
				pEffect->SetScalarArrayF(
					hParameter,
					(Float const*)(m_CommandStream.GetBuffer() + m_CommandStream.GetOffset()),
					uCount * 12u);

				m_CommandStream.SeekToOffset(m_CommandStream.GetOffset() + uCount * sizeof(Matrix3x4));
			}
			break;

		case kSetMatrix4DParameter:
			{
				GLSLFXLite* pEffect = nullptr;
				UnsafeHandle hParameter;
				Matrix4D mValue = Matrix4D::Zero();
				SEOUL_VERIFY(ReadEffectParameter(pEffect, hParameter));
				SEOUL_VERIFY(Read(mValue));
				pEffect->SetMatrixF4x4(hParameter, (Float const*)mValue.GetData());
			}
			break;

		case kSetTextureParameter:
			{
				GLSLFXLite* pEffect = nullptr;
				UnsafeHandle hParameter;
				BaseTexture* pTexture = nullptr;
				SEOUL_VERIFY(ReadEffectParameter(pEffect, hParameter));
				SEOUL_VERIFY(Read(pTexture));

				pEffect->SetSampler(hParameter, pTexture);
			}
			break;

		case kSetVector4DParameter:
			{
				GLSLFXLite* pEffect = nullptr;
				UnsafeHandle hParameter;
				Vector4D vValue = Vector4D::Zero();
				SEOUL_VERIFY(ReadEffectParameter(pEffect, hParameter));
				SEOUL_VERIFY(Read(vValue));
				pEffect->SetScalarArrayF(hParameter, (Float const*)vValue.GetData(), 4u);
			}
			break;

		case kSetCurrentViewport:
			{
				Viewport viewport;
				SEOUL_VERIFY(Read(viewport));

				// Note that OpenGl (unlike all of our other APIs) uses the
				// lower-left as its origin in all 2D contexts,
				// so we need to invert the Y component of the viewport origin.
				rStateManager.SetViewport(
					viewport.m_iViewportX,
					(viewport.m_iTargetHeight - (viewport.m_iViewportY + viewport.m_iViewportHeight)),
					viewport.m_iViewportWidth,
					viewport.m_iViewportHeight);
			}
			break;

		case kSetScissor:
			{
				Bool bEnabled = false;
				Viewport viewport;
				SEOUL_VERIFY(Read(bEnabled));
				SEOUL_VERIFY(Read(viewport));

				if (bEnabled != bLastScissorEnabled ||
					lastScissorViewport != viewport)
				{
					bLastScissorEnabled = bEnabled;
					lastScissorViewport = viewport;

					if (bEnabled)
					{
						// Construct the scissor rectangle.
						Rectangle2DInt rectangle(
							viewport.m_iViewportX,
							viewport.m_iViewportY,
							viewport.m_iViewportX + viewport.m_iViewportWidth,
							viewport.m_iViewportY + viewport.m_iViewportHeight);

						// Get the current target height.
						Int32 const iTargetHeight = (RenderTarget::GetActiveRenderTarget() != nullptr
							? RenderTarget::GetActiveRenderTarget()->GetHeight()
							: rDevice.m_BackBufferViewport.m_iTargetHeight);

						// OpenGl uses the lower left for its origin in all contexts, so we need to account
						// for that with this rectangle specification.
						rStateManager.SetScissor(
							rectangle.m_iLeft,
							(iTargetHeight - rectangle.m_iBottom),
							(rectangle.m_iRight - rectangle.m_iLeft),
							(rectangle.m_iBottom - rectangle.m_iTop));
					}

					rStateManager.SetRenderState(
						RenderState::kScissor,
						(bEnabled ? GL_TRUE : GL_FALSE));
				}
			}
			break;

		case kSetNullIndices:
			m_pActiveIndexBuffer.Reset();
			SEOUL_OGLES2_VERIFY(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u));
			break;

		case kSetIndices:
			{
				IndexBuffer* pIndexBuffer = nullptr;
				SEOUL_VERIFY(Read(pIndexBuffer));

				CheckedPtr<OGLES2IndexBuffer> pOGLES2IndexBuffer = static_cast<OGLES2IndexBuffer*>(pIndexBuffer);
				if (m_pActiveIndexBuffer != pOGLES2IndexBuffer)
				{
					m_pActiveIndexBuffer = pOGLES2IndexBuffer;

					// If this is not a dynamic index buffer, bind the OpenGl object.
					if (nullptr == pOGLES2IndexBuffer->m_pDynamicData)
					{
						SEOUL_OGLES2_VERIFY(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pOGLES2IndexBuffer->m_IndexBuffer));
					}
					// Otherwise, bind a nullptr object.
					else
					{
						SEOUL_OGLES2_VERIFY(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u));
					}
				}
			}
			break;

		case kSetNullVertices:
			{
				UInt uStreamNumber = 0u;
				SEOUL_VERIFY(Read(uStreamNumber));

				VertexStream& rVertexStream = m_aActiveStreams[uStreamNumber];
				rVertexStream = VertexStream();

				SEOUL_OGLES2_VERIFY(glBindBuffer(GL_ARRAY_BUFFER, 0u));
			}
			break;

		case kSetVertices:
			{
				UInt uStreamNumber = 0u;
				VertexBuffer* pBuffer = nullptr;
				UInt zOffsetInBytes = 0u;
				UInt zStrideInBytes = 0u;
				SEOUL_VERIFY(Read(uStreamNumber));
				SEOUL_VERIFY(Read(pBuffer));
				SEOUL_VERIFY(Read(zOffsetInBytes));
				SEOUL_VERIFY(Read(zStrideInBytes));

				VertexStream& rVertexStream = m_aActiveStreams[uStreamNumber];
				rVertexStream.m_pBuffer = pBuffer;
				rVertexStream.m_zOffsetInBytes = zOffsetInBytes;
				rVertexStream.m_zStrideInBytes = zStrideInBytes;
			}
			break;

		case kUseVertexFormat:
			{
				VertexFormat* p = nullptr;
				SEOUL_VERIFY(Read(p));

				m_pActiveVertexFormat = p;
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
						&OGLES2RenderCommandStreamBuilderCallReadPixel,
						SharedPtr<IReadPixel>(p),
						cColor,
						bSuccess);
				}
			}
			break;

			// TODO: Implement grab back buffer frame
			// in OGLES2.
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

				if (nullptr != p)
				{
					Jobs::AsyncFunction(
						callbackThreadId,
						&OGLES2RenderCommandStreamBuilderCallGrabFrame,
						uFrame,
						SharedPtr<IGrabFrame>(p),
						SharedPtr<IFrameData>(),
						false);
				}
			}
			break;

		case kUpdateOsWindowRegions:
			{
				UInt32 uCount = 0u;
				SEOUL_VERIFY(Read(uCount));
				if (uCount > 0u)
				{
					AlignReadOffset();
					m_CommandStream.SeekToOffset(m_CommandStream.GetOffset() + (uCount * sizeof(OsWindowRegion)));
				}
			}
			break;

		case kUnknown: // fall-through
		default:
			SEOUL_FAIL(__FUNCTION__);
			break;
		};
	}

	SEOUL_ASSERT(zStartingOffset == m_CommandStream.GetOffset());

	// Now disable any attributes which are still enabled.
	for (UInt32 i = 0u; i < m_abActiveVertexAttributes.GetSize(); ++i)
	{
		if (m_abActiveVertexAttributes[i])
		{
			SEOUL_OGLES2_VERIFY(glDisableVertexAttribArray(i));
			m_abActiveVertexAttributes[i] = false;
		}
	}
}

void OGLES2RenderCommandStreamBuilder::InternalCommitVertexStreams(UInt32 uMinVertexIndex, UInt32 uInstance /* = 0u */)
{
	// Initially, assume all attributes are disabled.
	FixedArray<Bool, kVertexAttributeCount> abActiveVertexAttributes(false);

	// Cannot set vertex buffers when there is no vertex format.
	if (nullptr == m_pActiveVertexFormat)
	{
		return;
	}

	const VertexFormat::VertexElements& vElements = m_pActiveVertexFormat->GetVertexElements();
	UInt32 const uElementCount = vElements.GetSize();

	for (UInt32 uStreamNumber = 0u; uStreamNumber < m_aActiveStreams.GetSize(); ++uStreamNumber)
	{
		const VertexStream& stream = m_aActiveStreams[uStreamNumber];
		VertexStream& rCommittedStream = m_aCommittedStreams[uStreamNumber];

		// If setting a buffer, walk the vertex declaration and
		// set each channel as a separate attribute with
		// the correct stride between each element.
		if (nullptr == stream.m_pBuffer)
		{
			continue;
		}

		auto pOGLES2VertexBuffer = static_cast<OGLES2VertexBuffer*>(stream.m_pBuffer);

		// Only commit the stream if it's changed.
		if (stream != rCommittedStream ||
			m_uActiveMinVertexIndex != uMinVertexIndex ||
			m_pActiveVertexFormat != m_pCommitedVertexFormat)
		{
			rCommittedStream = stream;

			// If this is not a dynamic buffer, bind the vertex buffer object.
			if (nullptr == pOGLES2VertexBuffer->m_pDynamicData)
			{
				SEOUL_OGLES2_VERIFY(glBindBuffer(GL_ARRAY_BUFFER, pOGLES2VertexBuffer->m_VertexBuffer));
			}
			// Otherwise, bind the nullptr object.
			else
			{
				SEOUL_OGLES2_VERIFY(glBindBuffer(GL_ARRAY_BUFFER, 0u));
			}

			for (UInt32 i = 0u; i < uElementCount; ++i)
			{
				const VertexElement& element = vElements[i];

				if (uStreamNumber != element.m_Stream)
				{
					continue;
				}

				UInt32 const uIndex = GetVertexDataIndex(element);

				// If the attribute has a valid index, activate it.
				if (uIndex < kVertexAttributeCount)
				{
					// Use the dynamic buffer pointer as the base - either it will be nullptr,
					// in which case we're only specifying offsets, or it will be
					// a valid pointer and we're specifying pointer addresses.
					SEOUL_OGLES2_VERIFY(glVertexAttribPointer(
						uIndex,
						GetVertexElementComponentCount(element),
						GetVertexElementType(element),
						GetVertexElementIsNormalized(element),
						stream.m_zStrideInBytes,
						pOGLES2VertexBuffer->m_pDynamicData + ((uMinVertexIndex * stream.m_zStrideInBytes) + stream.m_zOffsetInBytes + element.m_Offset)));

					abActiveVertexAttributes[uIndex] = true;
				}
			}
		}
		else
		{
			for (UInt32 i = 0u; i < uElementCount; ++i)
			{
				const VertexElement& element = vElements[i];

				if (uStreamNumber != element.m_Stream)
				{
					continue;
				}

				UInt32 const uIndex = GetVertexDataIndex(element);

				// If the attribute has a valid index, activate it.
				if (uIndex < kVertexAttributeCount)
				{
					abActiveVertexAttributes[uIndex] = true;
				}
			}
		}
	}

	// Now the active index.
	m_uActiveMinVertexIndex = uMinVertexIndex;

	// Now the commited vertex format.
	m_pCommitedVertexFormat = m_pActiveVertexFormat;

	// Now, enable/disable any attributes which do not match.
	for (UInt32 i = 0u; i < abActiveVertexAttributes.GetSize(); ++i)
	{
		if (!abActiveVertexAttributes[i] && m_abActiveVertexAttributes[i])
		{
			SEOUL_OGLES2_VERIFY(glDisableVertexAttribArray(i));
			m_abActiveVertexAttributes[i] = false;
		}
		else if (abActiveVertexAttributes[i] && !m_abActiveVertexAttributes[i])
		{
			SEOUL_OGLES2_VERIFY(glEnableVertexAttribArray(i));
			m_abActiveVertexAttributes[i] = true;
		}
	}
}

UInt32 OGLES2RenderCommandStreamBuilder::UpdateTexture(
	OGLES2StateManager& rStateManager,
	BaseTexture* pTexture,
	UInt uLevel,
	const Rectangle2DInt& rectangle,
	Byte* pSource)
{
	OGLES2Texture* pTexture2d = static_cast<OGLES2Texture*>(pTexture);

	Int32 iTextureWidth = pTexture->GetWidth();
	Int32 iTextureHeight = pTexture->GetHeight();
	BaseTexture::AdjustWidthAndHeightForTextureLevel(uLevel, iTextureWidth, iTextureHeight);

	Int32 const iRectangleWidth = (UInt32)(rectangle.m_iRight - rectangle.m_iLeft);
	Int32 const iRectangleHeight = (UInt32)(rectangle.m_iBottom - rectangle.m_iTop);
	PixelFormat const eFormat = pTexture->GetFormat();
	UInt32 const zDataSizeInBytes = GetDataSizeForPixelFormat(
		iRectangleWidth,
		iRectangleHeight,
		eFormat);

	SEOUL_OGLES2_VERIFY(glBindTexture(GL_TEXTURE_2D, pTexture2d->m_Texture));
	SEOUL_OGLES2_VERIFY(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

	if (iTextureWidth == iRectangleWidth &&
		iTextureHeight == iRectangleHeight)
	{
		if (IsCompressedPixelFormat(pTexture2d->GetFormat()))
		{
			SEOUL_OGLES2_VERIFY(GetOGLES2RenderDevice().CompressedTexImage2D(
				GL_TEXTURE_2D,
				uLevel,
				PixelFormatToOpenGlFormat(pTexture2d->GetFormat()),
				iTextureWidth,
				iTextureHeight,
				GL_FALSE,
				zDataSizeInBytes,
				pSource));
		}
		else
		{
			GLenum eOpenGlFormat = GL_INVALID_ENUM;
			GLenum eInternalOpenGlFormat = GL_INVALID_ENUM;
			GLenum eOpenGlType = GL_INVALID_ENUM;

			// Handle devices that do not support BGRA.
			if (PixelFormat::kA8R8G8B8 == pTexture2d->GetFormat() && !GetOGLES2RenderDevice().GetCaps().m_bBGRA)
			{
				eOpenGlFormat = PixelFormatToOpenGlFormat(PixelFormat::kA8B8G8R8);
				eInternalOpenGlFormat = PixelFormatToOpenGlInternalFormat(PixelFormat::kA8B8G8R8);
				eOpenGlType = PixelFormatToOpenGlElementType(PixelFormat::kA8B8G8R8);
				InternalStaticSwapR8B8(pSource, iTextureWidth, iTextureHeight);
			}
			else
			{
				eOpenGlFormat = PixelFormatToOpenGlFormat(pTexture2d->GetFormat());
				eInternalOpenGlFormat = PixelFormatToOpenGlInternalFormat(pTexture2d->GetFormat());
				eOpenGlType = PixelFormatToOpenGlElementType(pTexture2d->GetFormat());
			}

			SEOUL_OGLES2_VERIFY(GetOGLES2RenderDevice().TexImage2D(
				GL_TEXTURE_2D,
				uLevel,
				eInternalOpenGlFormat,
				iTextureWidth,
				iTextureHeight,
				GL_FALSE,
				eOpenGlFormat,
				eOpenGlType,
				pSource));
		}
	}
	else
	{
		if (IsCompressedPixelFormat(pTexture2d->GetFormat()))
		{
			SEOUL_OGLES2_VERIFY(GetOGLES2RenderDevice().CompressedTexSubImage2D(
				GL_TEXTURE_2D,
				uLevel,
				rectangle.m_iLeft,
				rectangle.m_iTop,
				iRectangleWidth,
				iRectangleHeight,
				PixelFormatToOpenGlFormat(pTexture2d->GetFormat()),
				zDataSizeInBytes,
				pSource));
		}
		else
		{
			GLenum eOpenGlFormat = GL_INVALID_ENUM;
			GLenum eOpenGlType = GL_INVALID_ENUM;

			// Handle devices that do not support BGRA.
			if (PixelFormat::kA8R8G8B8 == pTexture2d->GetFormat() && !GetOGLES2RenderDevice().GetCaps().m_bBGRA)
			{
				eOpenGlFormat = PixelFormatToOpenGlFormat(PixelFormat::kA8B8G8R8);
				eOpenGlType = PixelFormatToOpenGlElementType(PixelFormat::kA8B8G8R8);
				InternalStaticSwapR8B8(pSource, iRectangleWidth, iRectangleHeight);
			}
			else
			{
				eOpenGlFormat = PixelFormatToOpenGlFormat(pTexture2d->GetFormat());
				eOpenGlType = PixelFormatToOpenGlElementType(pTexture2d->GetFormat());
			}

			SEOUL_OGLES2_VERIFY(GetOGLES2RenderDevice().TexSubImage2D(
				GL_TEXTURE_2D,
				uLevel,
				rectangle.m_iLeft,
				rectangle.m_iTop,
				iRectangleWidth,
				iRectangleHeight,
				eOpenGlFormat,
				eOpenGlType,
				pSource));
		}
	}

	SEOUL_OGLES2_VERIFY(glPixelStorei(GL_UNPACK_ALIGNMENT, 4));
	SEOUL_OGLES2_VERIFY(glBindTexture(GL_TEXTURE_2D, 0));

	// Make sure the state manager's view of things is in sync once we're done.
	rStateManager.RestoreActiveTextureIfSet(GL_TEXTURE_2D);

	return zDataSizeInBytes;
}

} // namespace Seoul
