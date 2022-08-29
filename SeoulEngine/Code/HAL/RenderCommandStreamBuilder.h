/**
 * \file RenderCommandStreamBuilder.h
 * \brief RenderCommandStreamBuilder fulfills the same role as the GPU command buffer,
 * allowing Seoul encapsulation of render commands to be queued for later fulfillment
 * on the render thread.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef RENDER_COMMAND_STREAM_BUILDER_H
#define RENDER_COMMAND_STREAM_BUILDER_H

#include "CheckedPtr.h"
#include "ContentLoadManager.h"
#include "Delegate.h"
#include "DepthStencilSurface.h"
#include "Effect.h"
#include "EffectPass.h"
#include "Geometry.h"
#include "HashSet.h"
#include "IndexBuffer.h"
#include "IndexBufferDataFormat.h"
#include "Matrix3x4.h"
#include "Matrix4D.h"
#include "PixelFormat.h"
#include "Prereqs.h"
#include "PrimitiveType.h"
#include "RenderTarget.h"
#include "StreamBuffer.h"
#include "Texture.h"
#include "ThreadId.h"
#include "Vector2D.h"
#include "Vector3D.h"
#include "Vector4D.h"
#include "VertexBuffer.h"
#include "VertexElement.h"
#include "VertexFormat.h"
#include "Viewport.h"

namespace Seoul
{

// Forward declarations
struct Color4;
class Material;
class RenderPass;
class RenderSurface2D;
typedef Vector<VertexElement, MemoryBudgets::Rendering> VertexElements;

struct RenderStats SEOUL_SEALED
{
	static RenderStats Create()
	{
		RenderStats ret;
		ret.Clear();
		return ret;
	}

	void BeginFrame()
	{
		m_uMaxDrawsSubmitted = Max(m_uMaxDrawsSubmitted, m_uDrawsSubmitted);
		m_uDrawsSubmitted = 0u;
		m_uMaxTrianglesSubmittedForDraw = Max(m_uMaxTrianglesSubmittedForDraw, m_uTrianglesSubmittedForDraw);
		m_uTrianglesSubmittedForDraw = 0u;
		m_uMaxEffectBegins = Max(m_uMaxEffectBegins, m_uEffectBegins);
		m_uEffectBegins = 0u;
	}

	void Clear()
	{
		m_uDrawsSubmitted = 0u;
		m_uMaxDrawsSubmitted = 0u;
		m_uTrianglesSubmittedForDraw = 0u;
		m_uMaxTrianglesSubmittedForDraw = 0u;
		m_uEffectBegins = 0u;
		m_uMaxEffectBegins = 0u;
	}

	RenderStats& operator+=(const RenderStats& b)
	{
		m_uDrawsSubmitted += b.m_uDrawsSubmitted;
		m_uTrianglesSubmittedForDraw += b.m_uTrianglesSubmittedForDraw;
		m_uEffectBegins += b.m_uEffectBegins;

		return *this;
	}

	UInt32 m_uDrawsSubmitted;
	UInt32 m_uMaxDrawsSubmitted;
	UInt32 m_uTrianglesSubmittedForDraw;
	UInt32 m_uMaxTrianglesSubmittedForDraw;
	UInt32 m_uEffectBegins;
	UInt32 m_uMaxEffectBegins;
}; // struct RenderStats

inline RenderStats operator+(const RenderStats& a, const RenderStats& b)
{
	RenderStats ret = a;
	ret += b;
	return ret;
}

template <typename T>
struct RenderCommandStreamReadWrite
{
	static inline Bool Read(StreamBuffer& rBuffer, T& rValue)
	{
		return rBuffer.Read(&rValue, sizeof(rValue));
	}

	static inline void Write(StreamBuffer& rBuffer, const T& value)
	{
		rBuffer.Write(&value, sizeof(value));
	}
};

class IFrameData SEOUL_ABSTRACT
{
public:
	virtual ~IFrameData()
	{
	}

	virtual void const* GetData() const = 0;
	virtual UInt32 GetFrameHeight() const = 0;
	virtual UInt32 GetFrameWidth() const = 0;
	virtual UInt32 GetPitch() const = 0;
	virtual PixelFormat GetPixelFormat() const = 0;

protected:
	SEOUL_REFERENCE_COUNTED(IFrameData);

	IFrameData()
	{
	}

private:
	SEOUL_DISABLE_COPY(IFrameData);
}; // class IFrameData

class IGrabFrame SEOUL_ABSTRACT
{
public:
	virtual ~IGrabFrame()
	{
	}

	virtual void OnGrabFrame(
		UInt32 uFrame,
		const SharedPtr<IFrameData>& pFrameData,
		Bool bSuccess) = 0;

protected:
	SEOUL_REFERENCE_COUNTED(IGrabFrame);

	IGrabFrame()
	{
	}

private:
	SEOUL_DISABLE_COPY(IGrabFrame);
}; // class IGrabFrame

class IReadPixel SEOUL_ABSTRACT
{
public:
	virtual ~IReadPixel()
	{
	}

	virtual void OnReadPixel(ColorARGBu8 cPixel, Bool bSuccess) = 0;

protected:
	SEOUL_REFERENCE_COUNTED(IReadPixel);

	IReadPixel()
	{
	}

private:
	SEOUL_DISABLE_COPY(IReadPixel);
}; // class IReadPixel

/** Convenience utility for representing OS window regions. */
struct OsWindowRegion SEOUL_SEALED
{
	/**
	 * A rectangle in which the window should render and receive
	 * input.
	 */
	Rectangle2DInt m_Rect;

	/**
	 * Area outside of the rect that should disable rendering
	 * to the window but still capture input.
	 */
	Float m_fInputMargin = 0.0f;

	/**
	 * Identifies the rectangle as the effective main form
	 * of all rectangles. Used for thumbnail/snapshot generation
	 * on Windows.
	 */
	Bool m_bMainForm = false;

	Bool operator<(const OsWindowRegion& b) const
	{
		if (m_Rect == b.m_Rect)
		{
			return (m_fInputMargin < b.m_fInputMargin);
		}
		else
		{
			return (m_Rect < b.m_Rect);
		}
	}
};

class RenderCommandStreamBuilder SEOUL_ABSTRACT
{
public:
	virtual ~RenderCommandStreamBuilder();

	virtual void ExecuteCommandStream(RenderStats& rStats) = 0;

	/**
	 * @return True if this RenderCommandStreamBuilder has no commands, false otherwise.
	 */
	Bool IsEmpty() const
	{
		return m_CommandStream.IsEmpty();
	}

	/**
	 * Reset this RenderCommandStreamBuilder so it is empty and has no commands.
	 */
	void ResetCommandStream()
	{
		SEOUL_ASSERT(IsRenderThread());

		InternalResetCommandStream();
	}

	// Buffer clearing support
	void Clear(
		UInt32 uFlags,
		const Color4& cClearColor,
		Float fClearDepth,
		UInt8 uClearStencil)
	{
		Write(kClear);
		Write(uFlags);
		Write(cClearColor);
		Write(fClearDepth);
		Write(uClearStencil);
	}

	void PostPass(UInt32 uClearFlags)
	{
		Write(kPostPass);
		Write(uClearFlags);
	}

	// State management
	void ApplyDefaultRenderState()
	{
		Write(kApplyDefaultRenderState);
	}

	/**
	 * Set the current scissor rectangle and mode that will be used.
	 *
	 * Viewport is used so that the device backend can adjust the
	 * scissor rectangle on resize events, if needed.
	 */
	void SetScissor(Bool bEnabled, const Viewport& viewport = Viewport())
	{
		Write(kSetScissor);
		Write(bEnabled);
		Write(viewport);
	}

	// Viewport control
	const Viewport& GetCurrentViewport() const
	{
		return m_CurrentViewport;
	}

	void SetCurrentViewport(const Viewport& viewport)
	{
		m_CurrentViewport = viewport;

		Write(kSetCurrentViewport);
		Write(viewport);
	}

	// Vertex formats
	void UseVertexFormat(VertexFormat* pFormat)
	{
		Write(kUseVertexFormat);
		if (nullptr != pFormat)
		{
			m_References.Insert(SharedPtr<VertexFormat>(pFormat));
		}
		Write(pFormat);
	}

	// Mesh data
	void SetIndices(IndexBuffer* pBuffer)
	{
		if (pBuffer)
		{
			Write(kSetIndices);
			m_References.Insert(SharedPtr<IndexBuffer>(pBuffer));
			Write(pBuffer);
		}
		else
		{
			Write(kSetNullIndices);
		}
	}

	void* LockIndexBuffer(IndexBuffer* pIndexBuffer, UInt32 zLockSizeInBytes)
	{
		SEOUL_ASSERT(!m_bBufferLocked);
		SEOUL_ASSERT(nullptr != pIndexBuffer);

		zLockSizeInBytes = Min(zLockSizeInBytes, pIndexBuffer->m_zTotalSizeInBytes);

		Write(kLockIndexBuffer);
		m_References.Insert(SharedPtr<IndexBuffer>(pIndexBuffer));
		Write(pIndexBuffer);
		Write(zLockSizeInBytes);

		AlignWriteOffset();

		UInt32 const zOffset = m_CommandStream.GetOffset();
		m_CommandStream.PadTo(m_CommandStream.GetOffset() + zLockSizeInBytes, false);
		void* pReturn = (m_CommandStream.GetBuffer() + zOffset);

		// Can only lock one vertex buffer at a time - use this boolean to ensure correct usage.
		m_bBufferLocked = true;

		return pReturn;
	}

	void UnlockIndexBuffer(IndexBuffer* pIndexBuffer)
	{
		SEOUL_ASSERT(m_bBufferLocked);
		SEOUL_ASSERT(nullptr != pIndexBuffer);

		Write(kUnlockIndexBuffer);
		m_References.Insert(SharedPtr<IndexBuffer>(pIndexBuffer));
		Write(pIndexBuffer);

		m_bBufferLocked = false;
	}

	void* LockTexture(BaseTexture* pTexture, UInt uLevel, const Rectangle2DInt& rectangle)
	{
		SEOUL_ASSERT(!m_bBufferLocked);
		SEOUL_ASSERT(nullptr != pTexture);

		Int32 iHeight = pTexture->GetHeight();
		Int32 iWidth = pTexture->GetWidth();
		BaseTexture::AdjustWidthAndHeightForTextureLevel(uLevel, iWidth, iHeight);

		SEOUL_ASSERT(rectangle.m_iLeft >= 0 && rectangle.m_iRight <= iWidth &&
			rectangle.m_iTop >= 0 && rectangle.m_iBottom <= iHeight &&
			(rectangle.m_iRight - rectangle.m_iLeft > 0) &&
			(rectangle.m_iBottom - rectangle.m_iTop > 0));

		UInt32 const zLockSizeInBytes = GetDataSizeForPixelFormat(
			(UInt32)(rectangle.m_iRight - rectangle.m_iLeft),
			(UInt32)(rectangle.m_iBottom - rectangle.m_iTop),
			pTexture->GetFormat());

		Write(kLockTexture);
		m_References.Insert(SharedPtr<BaseTexture>(pTexture));
		Write(pTexture);
		Write(uLevel);
		Write(rectangle);

		AlignWriteOffset();

		UInt32 const zOffset = m_CommandStream.GetOffset();
		m_CommandStream.PadTo(m_CommandStream.GetOffset() + zLockSizeInBytes, false);
		void* pReturn = (m_CommandStream.GetBuffer() + zOffset);

		// Can only lock one vertex buffer at a time - use this boolean to ensure correct usage.
		m_bBufferLocked = true;

		return pReturn;
	}

	void UnlockTexture(BaseTexture* pTexture, UInt uLevel)
	{
		SEOUL_ASSERT(m_bBufferLocked);
		SEOUL_ASSERT(nullptr != pTexture);

		Write(kUnlockTexture);
		m_References.Insert(SharedPtr<BaseTexture>(pTexture));
		Write(pTexture);
		Write(uLevel);

		m_bBufferLocked = false;
	}

	void UpdateTexture(
		BaseTexture* pTexture,
		UInt uLevel,
		const Rectangle2DInt& rectangle,
		void* pBuffer,
		UInt32 zBufferSizeInBytes,
		Bool bTakeOwnershipOfBuffer)
	{
		SEOUL_ASSERT(!m_bBufferLocked);
		SEOUL_ASSERT(nullptr != pTexture);

		Int32 iHeight = pTexture->GetHeight();
		Int32 iWidth = pTexture->GetWidth();
		BaseTexture::AdjustWidthAndHeightForTextureLevel(uLevel, iWidth, iHeight);

		SEOUL_ASSERT(rectangle.m_iLeft >= 0 && rectangle.m_iRight <= iWidth &&
			rectangle.m_iTop >= 0 && rectangle.m_iBottom <= iHeight &&
			(rectangle.m_iRight - rectangle.m_iLeft > 0) &&
			(rectangle.m_iBottom - rectangle.m_iTop > 0));

		UInt32 const zExpectedSizeInBytes = GetDataSizeForPixelFormat(
			(UInt32)(rectangle.m_iRight - rectangle.m_iLeft),
			(UInt32)(rectangle.m_iBottom - rectangle.m_iTop),
			pTexture->GetFormat());

		SEOUL_ASSERT(zExpectedSizeInBytes == zBufferSizeInBytes);

		Write(kUpdateTexture);
		m_References.Insert(SharedPtr<BaseTexture>(pTexture));
		Write(pTexture);
		Write(uLevel);
		Write(rectangle);

		if (!bTakeOwnershipOfBuffer)
		{
			void* pBufferCopy = MemoryManager::Allocate(
				zExpectedSizeInBytes,
				MemoryBudgets::Rendering);
			memcpy(pBufferCopy, pBuffer, zExpectedSizeInBytes);
			pBuffer = pBufferCopy;
		}

		m_vBuffers.PushBack(pBuffer);

		Write(pBuffer);
	}

	void* LockVertexBuffer(VertexBuffer* pVertexBuffer, UInt32 zLockSizeInBytes)
	{
		SEOUL_ASSERT(!m_bBufferLocked);
		SEOUL_ASSERT(nullptr != pVertexBuffer);

		zLockSizeInBytes = Min(zLockSizeInBytes, pVertexBuffer->m_zTotalSizeInBytes);

		Write(kLockVertexBuffer);
		m_References.Insert(SharedPtr<VertexBuffer>(pVertexBuffer));
		Write(pVertexBuffer);
		Write(zLockSizeInBytes);

		AlignWriteOffset();

		UInt32 const zOffset = m_CommandStream.GetOffset();
		m_CommandStream.PadTo(m_CommandStream.GetOffset() + zLockSizeInBytes, false);
		void* pReturn = (m_CommandStream.GetBuffer() + zOffset);

		// Can only lock one vertex buffer at a time - use this boolean to ensure correct usage.
		m_bBufferLocked = true;

		return pReturn;
	}

	void UnlockVertexBuffer(VertexBuffer* pVertexBuffer)
	{
		SEOUL_ASSERT(m_bBufferLocked);
		SEOUL_ASSERT(nullptr != pVertexBuffer);

		Write(kUnlockVertexBuffer);
		m_References.Insert(SharedPtr<VertexBuffer>(pVertexBuffer));
		Write(pVertexBuffer);

		m_bBufferLocked = false;
	}

	void SetVertices(
		UInt nStreamNumber,
		VertexBuffer* pBuffer,
		UInt zOffsetInBytes,
		UInt zStrideInBytes)
	{
		if (pBuffer)
		{
			Write(kSetVertices);
			Write(nStreamNumber);
			m_References.Insert(SharedPtr<VertexBuffer>(pBuffer));
			Write(pBuffer);
			Write(zOffsetInBytes);
			Write(zStrideInBytes);
		}
		else
		{
			Write(kSetNullVertices);
			Write(nStreamNumber);
		}
	}

	// Drawing
	void DrawPrimitive(
		PrimitiveType eType,
		UInt uOffset,
		UInt nNumPrimitives)
	{
		Write(kDrawPrimitive);
		Write(eType);
		Write(uOffset);
		Write(nNumPrimitives);
	}

	void DrawIndexedPrimitive(
		PrimitiveType eType,
		Int iOffset,
		UInt uMinIndex,
		UInt nNumVerts,
		UInt uStartIndex,
		UInt nNumPrimitives)
	{
		Write(kDrawIndexedPrimitive);
		Write(eType);
		Write(iOffset);
		Write(uMinIndex);
		Write(nNumVerts);
		Write(uStartIndex);
		Write(nNumPrimitives);
	}

	// Render surfaces
	void ResolveDepthStencilSurface(DepthStencilSurface* pDepthStencilSurface)
	{
		SEOUL_ASSERT(nullptr != pDepthStencilSurface);
		Write(kResolveDepthStencilSurface);
		m_References.Insert(SharedPtr<DepthStencilSurface>(pDepthStencilSurface));
		Write(pDepthStencilSurface);
	}

	void SelectDepthStencilSurface(DepthStencilSurface* pDepthStencilSurface)
	{
		Write(kSelectDepthStencilSurface);
		if (nullptr != pDepthStencilSurface)
		{
			m_References.Insert(SharedPtr<DepthStencilSurface>(pDepthStencilSurface));
		}
		Write(pDepthStencilSurface);
	}

	void ResolveRenderTarget(RenderTarget* pTarget)
	{
		SEOUL_ASSERT(nullptr != pTarget);

		Write(kResolveRenderTarget);
		m_References.Insert(SharedPtr<RenderTarget>(pTarget));
		Write(pTarget);
	}

	void SelectRenderTarget(RenderTarget* pTarget)
	{
		Write(kSelectRenderTarget);
		if (nullptr != pTarget)
		{
			m_References.Insert(SharedPtr<RenderTarget>(pTarget));
		}
		Write(pTarget);
	}

	void CommitRenderSurface()
	{
		Write(kCommitRenderSurface);
	}

	// Debugging support
	void BeginEvent(const String& sEventName)
	{
		Write(kBeginEvent);
		m_CommandStream.Write(sEventName);
	}

	void EndEvent()
	{
		Write(kEndEvent);
	}

	// Effects
	EffectPass BeginEffect(const SharedPtr<Effect>& pEffect, HString technique)
	{
		if (!pEffect.IsValid() || !pEffect->m_hHandle.IsValid() || pEffect->GetState() == BaseGraphicsObject::kDestroyed)
		{
			return EffectPass();
		}

		Effect::TechniqueEntry entry;
		if (!pEffect->m_tTechniquesByName.GetValue(technique, entry))
		{
			return EffectPass();
		}

		Write(kBeginEffect);
		Write(pEffect->m_hHandle);
		m_References.Insert(pEffect);
		Write(entry.m_hHandle);

		return EffectPass(0u, entry.m_uPassCount);
	}

	void EndEffect(const SharedPtr<Effect>& pEffect)
	{
		// Sanity check - never called outside of a BeginEffect() block, which sould
		// have verified this.
		SEOUL_ASSERT(pEffect.IsValid() && pEffect->m_hHandle.IsValid());

		Write(kEndEffect);
		Write(pEffect->m_hHandle);
		m_References.Insert(pEffect);
	}

	Bool BeginEffectPass(const SharedPtr<Effect>& pEffect, const EffectPass& pass)
	{
		// Sanity check - never called outside of a BeginEffect() block, which sould
		// have verified this.
		SEOUL_ASSERT(pEffect.IsValid() && pEffect->m_hHandle.IsValid());

		// Check that the pass is valid - if not, return failure.
		if (!pass.IsValid())
		{
			return false;
		}

		Write(kBeginEffectPass);
		Write(pEffect->m_hHandle);
		m_References.Insert(pEffect);
		Write(pass.m_uPassIndex);
		Write(pass.m_uPassCount);
		return true;
	}

	void CommitEffectPass(const SharedPtr<Effect>& pEffect, const EffectPass& pass)
	{
		// Sanity check - never called outside of a BeginEffect() block, which sould
		// have verified this.
		SEOUL_ASSERT(pEffect.IsValid() && pEffect->m_hHandle.IsValid());

		Write(kCommitEffectPass);
		Write(pEffect->m_hHandle);
		m_References.Insert(pEffect);
		Write(pass.m_uPassIndex);
		Write(pass.m_uPassCount);
	}

	void EndEffectPass(const SharedPtr<Effect>& pEffect, const EffectPass& pass)
	{
		// Sanity check - never called outside of a BeginEffect() block, which sould
		// have verified this.
		SEOUL_ASSERT(pEffect.IsValid() && pEffect->m_hHandle.IsValid());

		Write(kEndEffectPass);
		Write(pEffect->m_hHandle);
		m_References.Insert(pEffect);
		Write(pass.m_uPassIndex);
		Write(pass.m_uPassCount);
	}

	void GrabBackBufferFrame(
		UInt32 uFrame,
		const Rectangle2DInt& rect,
		const SharedPtr<IGrabFrame>& pCallback,
		const ThreadId& callbackThreadId = ThreadId())
	{
		Write(kGrabBackBufferFrame);
		Write(uFrame);
		Write(rect);
		if (pCallback.IsValid())
		{
			m_vGrabFrameCallbacks.PushBack(pCallback);
		}
		Write(pCallback.GetPtr());
		Write(callbackThreadId);
	}

	void ReadBackBufferPixel(
		Int32 iX,
		Int32 iY,
		const SharedPtr<IReadPixel>& pCallback,
		const ThreadId& callbackThreadId = ThreadId())
	{
		Write(kReadBackBufferPixel);
		Write(iX);
		Write(iY);
		if (pCallback.IsValid())
		{
			m_vReadPixelCallbacks.PushBack(pCallback);
		}
		Write(pCallback.GetPtr());
		Write(callbackThreadId);
	}

	// Effect parameters
	void SetFloatParameter(const SharedPtr<Effect>& pEffect, HString parameterSemantic, Float f)
	{
		// Early out.
		if (!pEffect.IsValid() || !pEffect->m_hHandle.IsValid())
		{
			return;
		}

		Effect::ParameterEntry entry;
		if (!pEffect->m_tParametersBySemantic.GetValue(parameterSemantic, entry))
		{
			return;
		}

		if (entry.m_eType != EffectParameterType::kFloat)
		{
			return;
		}

		Write(kSetFloatParameter);
		Write(pEffect->m_hHandle);
		m_References.Insert(pEffect);
		Write(entry.m_hHandle);
		Write(f);
	}

	void SetMatrix3x4ArrayParameter(
		const SharedPtr<Effect>& pEffect,
		HString parameterSemantic,
		Matrix3x4 const* p,
		UInt32 uNumberOfMatrix3x4)
	{
		// Early out.
		if (!pEffect.IsValid() || !pEffect->m_hHandle.IsValid())
		{
			return;
		}

		Effect::ParameterEntry entry;
		if (!pEffect->m_tParametersBySemantic.GetValue(parameterSemantic, entry))
		{
			return;
		}

		if (entry.m_eType != EffectParameterType::kArray)
		{
			return;
		}
		Write(kSetMatrix3x4ArrayParameter);
		Write(pEffect->m_hHandle);
		m_References.Insert(pEffect);
		Write(entry.m_hHandle);
		WriteAlignedArray(p, uNumberOfMatrix3x4);
	}

	void SetMatrix4DParameter(const SharedPtr<Effect>& pEffect, HString parameterSemantic, const Matrix4D& m)
	{
		// Early out.
		if (!pEffect.IsValid() || !pEffect->m_hHandle.IsValid())
		{
			return;
		}

		Effect::ParameterEntry entry;
		if (!pEffect->m_tParametersBySemantic.GetValue(parameterSemantic, entry))
		{
			return;
		}

		if (entry.m_eType != EffectParameterType::kMatrix4D)
		{
			return;
		}

		Write(kSetMatrix4DParameter);
		Write(pEffect->m_hHandle);
		m_References.Insert(pEffect);
		Write(entry.m_hHandle);
		Write(m);
	}

	void SetTextureParameter(const SharedPtr<Effect>& pEffect, HString parameterSemantic, const TextureContentHandle& h)
	{
		// Early out.
		if (!pEffect.IsValid() || !pEffect->m_hHandle.IsValid())
		{
			return;
		}

		Effect::ParameterEntry entry;
		if (!pEffect->m_tParametersBySemantic.GetValue(parameterSemantic, entry))
		{
			return;
		}

		if (entry.m_eType != EffectParameterType::kTexture)
		{
			return;
		}

		Write(kSetTextureParameter);
		Write(pEffect->m_hHandle);
		m_References.Insert(pEffect);
		Write(entry.m_hHandle);

		SharedPtr<BaseTexture> pTexture(h.GetPtr());
		if (pTexture.IsValid())
		{
			m_References.Insert(pTexture);
		}
		Write(pTexture.GetPtr());
	}

	void SetVector4DParameter(const SharedPtr<Effect>& pEffect, HString parameterSemantic, const Vector4D& v)
	{
		// Early out.
		if (!pEffect.IsValid() || !pEffect->m_hHandle.IsValid())
		{
			return;
		}

		Effect::ParameterEntry entry;
		if (!pEffect->m_tParametersBySemantic.GetValue(parameterSemantic, entry))
		{
			return;
		}

		if (entry.m_eType != EffectParameterType::kVector4D)
		{
			return;
		}

		Write(kSetVector4DParameter);
		Write(pEffect->m_hHandle);
		m_References.Insert(pEffect);
		Write(entry.m_hHandle);
		Write(v);
	}

	/**
	 * For render backends which support it, apply a set of inclusive or
	 * rectangles that describe the subsets of the OS window that will
	 * be visible and receive input. Other areas of the window are expected
	 * to be not drawn and allow click through.
	 */
	void UpdateOsWindowRegions(OsWindowRegion const* pRegions, UInt32 uCount)
	{
		Write(kUpdateOsWindowRegions);
		Write(uCount);
		if (uCount > 0u)
		{
			AlignWriteOffset();
			m_CommandStream.Write((void const*)pRegions, sizeof(*pRegions) * uCount);
		}
	}

protected:
	RenderCommandStreamBuilder(UInt32 zInitialCapacity);

	typedef Vector< SharedPtr<IGrabFrame>, MemoryBudgets::RenderCommandStream > GrabFrameCallbacks;
	GrabFrameCallbacks m_vGrabFrameCallbacks;

	typedef Vector< SharedPtr<IReadPixel>, MemoryBudgets::RenderCommandStream > ReadPixelCallbacks;
	ReadPixelCallbacks m_vReadPixelCallbacks;

	typedef HashSet< SharedPtr<BaseGraphicsObject>, MemoryBudgets::RenderCommandStream > References;
	References m_References;

	typedef Vector< void*, MemoryBudgets::RenderCommandStream > Buffers;
	Buffers m_vBuffers;

	StreamBuffer m_CommandStream;
	Viewport m_CurrentViewport;

	enum OpCode
	{
		kUnknown,
		kApplyDefaultRenderState,
		kBeginEvent,
		kClear,
		kPostPass,
		kDrawPrimitive,
		kDrawIndexedPrimitive,
		kEndEvent,
		kLockIndexBuffer,
		kUnlockIndexBuffer,
		kLockTexture,
		kUnlockTexture,
		kUpdateTexture,
		kLockVertexBuffer,
		kUnlockVertexBuffer,
		kResolveDepthStencilSurface,
		kSelectDepthStencilSurface,
		kResolveRenderTarget,
		kSelectRenderTarget,
		kCommitRenderSurface,

		kBeginEffect,
		kEndEffect,
		kBeginEffectPass,
		kCommitEffectPass,
		kEndEffectPass,

		kSetFloatParameter,
		kSetMatrix3x4ArrayParameter,
		kSetMatrix4DParameter,
		kSetTextureParameter,
		kSetVector4DParameter,

		kSetCurrentViewport,
		kSetScissor,
		kSetNullIndices,
		kSetIndices,
		kSetNullVertices,
		kSetVertices,
		kUseVertexFormat,

		kReadBackBufferPixel,
		kGrabBackBufferFrame,

		kUpdateOsWindowRegions,
	};

	template <typename T>
	Bool Read(T& rValue)
	{
		return RenderCommandStreamReadWrite<T>::Read(m_CommandStream, rValue);
	}

	template <typename T>
	void Write(const T& value)
	{
		RenderCommandStreamReadWrite<T>::Write(m_CommandStream, value);
	}

	void AlignReadOffset()
	{
		StreamBuffer::SizeType nAlignedOffset = (StreamBuffer::SizeType)RoundUpToAlignment(m_CommandStream.GetOffset(), 16u);
		m_CommandStream.SeekToOffset(nAlignedOffset);
		SEOUL_ASSERT(m_CommandStream.GetOffset() == nAlignedOffset);
	}

	void AlignWriteOffset()
	{
		StreamBuffer::SizeType nAlignedOffset = (StreamBuffer::SizeType)RoundUpToAlignment(m_CommandStream.GetOffset(), 16u);
		m_CommandStream.PadTo(nAlignedOffset, false);
		SEOUL_ASSERT(m_CommandStream.GetOffset() == nAlignedOffset);
	}

	template <typename T>
	void WriteAlignedArray(T const* p, UInt32 nCount)
	{
		Write(nCount);
		AlignWriteOffset();
		m_CommandStream.Write(p, nCount * sizeof(T));
	}

private:
	void InternalResetCommandStream();

	Bool m_bBufferLocked;

	SEOUL_DISABLE_COPY(RenderCommandStreamBuilder);
}; // class RenderCommandStreamBuilder

// Helper macros for marking GPU events.
#if (!SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD))
#define SEOUL_BEGIN_GFX_EVENT(builder, format, ...) \
	(builder).BeginEvent(String::Printf((format), ##__VA_ARGS__))

#define SEOUL_END_GFX_EVENT(builder) \
	(builder).EndEvent()
#else
#define SEOUL_BEGIN_GFX_EVENT(builder, format, ...) ((void)0)
#define SEOUL_END_GFX_EVENT(builder) ((void)0)
#endif

} // namespace Seoul

#endif // include guard
