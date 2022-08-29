/**
 * \file NullGraphicsDevice.h
 * \brief Nop implementation of a GraphicsDevice for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NULL_GRAPHICS_DEVICE_H
#define NULL_GRAPHICS_DEVICE_H

#include "AtomicRingBuffer.h"
#include "Mutex.h"
#include "RenderDevice.h"
namespace Seoul { class NullGraphicsVertexFormat; }

namespace Seoul
{

class NullGraphicsDevice SEOUL_SEALED : public RenderDevice
{
public:
	static CheckedPtr<NullGraphicsDevice> Get()
	{
		if (RenderDevice::Get() && RenderDevice::Get()->GetType() == RenderDeviceType::kNull)
		{
			return (NullGraphicsDevice*)RenderDevice::Get().Get();
		}
		else
		{
			return CheckedPtr<NullGraphicsDevice>();
		}
	}

	NullGraphicsDevice(Int32 iNullViewportWidth = 800, Int32 iNullViewportHeight = 600);
	~NullGraphicsDevice();

	virtual RenderDeviceType GetType() const SEOUL_OVERRIDE { return RenderDeviceType::kNull; }

	virtual RenderCommandStreamBuilder* CreateRenderCommandStreamBuilder(UInt32 zInitialCapacity = 0u) const SEOUL_OVERRIDE;

	virtual Bool BeginScene() SEOUL_OVERRIDE;
	virtual void EndScene() SEOUL_OVERRIDE;

	virtual const Viewport& GetBackBufferViewport() const SEOUL_OVERRIDE;
	virtual RefreshRate GetDisplayRefreshRate() const SEOUL_OVERRIDE;

	virtual SharedPtr<VertexFormat> CreateVertexFormat(const VertexElement* InElements) SEOUL_OVERRIDE;
	virtual SharedPtr<DepthStencilSurface> CreateDepthStencilSurface(const DataStoreTableUtil& configSettings) SEOUL_OVERRIDE;
	virtual SharedPtr<RenderTarget> CreateRenderTarget(const DataStoreTableUtil& configSettings) SEOUL_OVERRIDE;

	virtual SharedPtr<IndexBuffer> CreateIndexBuffer(
		void const* pInitialData,
		UInt32 zInitialDataSizeInBytes,
		UInt32 zTotalSizeInBytes,
		IndexBufferDataFormat eFormat) SEOUL_OVERRIDE;
	virtual SharedPtr<IndexBuffer> CreateDynamicIndexBuffer(
		UInt32 zTotalSizeInBytes,
		IndexBufferDataFormat eFormat) SEOUL_OVERRIDE;
	virtual SharedPtr<VertexBuffer> CreateVertexBuffer(
		void const* pInitialData,
		UInt32 zInitialDataSizeInBytes,
		UInt zTotalSizeInBytes,
		UInt zStrideInBytes) SEOUL_OVERRIDE;
	virtual SharedPtr<VertexBuffer> CreateDynamicVertexBuffer(
		UInt zTotalSizeInBytes,
		UInt zStrideInBytes) SEOUL_OVERRIDE;

	virtual SharedPtr<BaseTexture> CreateTexture(
		const TextureConfig& config,
		const TextureData& data,
		UInt uWidth,
		UInt uHeight,
		PixelFormat eFormat) SEOUL_OVERRIDE;

	virtual SharedPtr<Effect> CreateEffectFromFileInMemory(
		FilePath filePath,
		void* pRawEffectFileData,
		UInt32 zFileSizeInBytes) SEOUL_OVERRIDE;

private:
	Viewport InternalCreateDefaultViewport() const;
	void InternalAddObject(const SharedPtr<BaseGraphicsObject>& pObject);
	Bool InternalPerFrameMaintenance();
	Bool InternalDestructorMaintenance();

	Int32 const m_iNullViewportWidth;
	Int32 const m_iNullViewportHeight;
	Viewport m_BackBufferViewport;

	Vector< SharedPtr<NullGraphicsVertexFormat>, MemoryBudgets::Rendering> m_vpVertexFormats;
	Mutex m_VertexFormatsMutex;

	typedef Vector< SharedPtr<BaseGraphicsObject>, MemoryBudgets::Rendering> GraphicsObjects;
	GraphicsObjects m_vGraphicsObjects;

	typedef AtomicRingBuffer<BaseGraphicsObject*> PendingGraphicsObjects;
	PendingGraphicsObjects m_PendingGraphicsObjects;

	SEOUL_DISABLE_COPY(NullGraphicsDevice);
}; // class NullGraphicsDevice

} // namespace Seoul

#endif // include guard
