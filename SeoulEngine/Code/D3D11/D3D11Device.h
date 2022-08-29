/**
 * \file D3D11Device.h
 * \brief D3D11 specialization of RenderDevice.
 * D3D11Device is the root of D3D11 specific low-level graphics functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef D3D11_DEVICE_H
#define D3D11_DEVICE_H

#include "AtomicRingBuffer.h"
#include "D3DCommonDevice.h"
#include "D3DCommonDeviceSettings.h"
#include "D3D11Util.h"
#include "Engine.h"
#include "HashTable.h"
#include "InputKeys.h"
#include "RenderDevice.h"
#include "SeoulString.h"
#include "StandardPlatformIncludes.h"
namespace Seoul { class D3D11EventQuery; }
namespace Seoul { class D3D11FrameData; }
namespace Seoul { class D3D11RenderTarget; }
namespace Seoul { class D3D11VertexFormat; }
namespace Seoul { struct D3DCommonUserGraphicsSettings; }
namespace Seoul { class IFrameData; }
namespace Seoul { struct OsWindowRegion; }
struct ID3D11DepthStencilView;
struct ID3D11RenderTargetView;

namespace Seoul
{

// JSON file graphics parameters used in several different places.
static const char* kVideoJsonFileSection = "Video";
static const char* kWindowWidthJsonParameter = "WindowWidth";
static const char* kWindowHeightJsonParameter = "WindowHeight";
static const char* kFullscreenWidthJsonParameter = "FullscreenWidth";
static const char* kFullscreenHeightJsonParameter = "FullscreenHeight";

/** Set of feature levels that we query for and require. */
static D3D_FEATURE_LEVEL const kaFeatureLevels[] =
{
	D3D_FEATURE_LEVEL_11_0,
};

/**
 * D3D11Device is a Seoul engine wrapper around IDirect3DDevice9.
 * D3D11Device handles initializatin and teardown of the D3D11 device and object.
 * It also handles window management and the creation and lifespan of most
 * graphics objects.
 */
class D3D11Device SEOUL_ABSTRACT : public D3DCommonDevice
{
public:
	static CheckedPtr<D3D11Device> Get()
	{
		if (RenderDevice::Get() &&
			(RenderDevice::Get()->GetType() == RenderDeviceType::kD3D11Headless ||
			RenderDevice::Get()->GetType() == RenderDeviceType::kD3D11Window))
		{
			return (D3D11Device*)RenderDevice::Get().Get();
		}
		else
		{
			return CheckedPtr<D3D11Device>();
		}
	}

	D3D11Device(const D3DCommonDeviceSettings& settings);
	~D3D11Device();

	// RenderDevice overrides
	virtual RenderCommandStreamBuilder* CreateRenderCommandStreamBuilder(UInt32 zInitialCapacity = 0u) const SEOUL_OVERRIDE;

	// Generic graphics object create method
	template <typename T>
	SharedPtr<T> Create(MemoryBudgets::Type eType)
	{
		SharedPtr<T> pReturn(SEOUL_NEW(eType) T);
		InternalAddObject(pReturn);
		return pReturn;
	}

	// Scene management
	virtual Bool BeginScene() SEOUL_OVERRIDE;
	virtual void EndScene() SEOUL_OVERRIDE;

	// Viewport control
	virtual const Viewport& GetBackBufferViewport() const SEOUL_OVERRIDE;

	// Get the screen refresh rate in hertz.
	virtual RefreshRate GetDisplayRefreshRate() const SEOUL_OVERRIDE;

	// Vertex formats
	virtual SharedPtr<VertexFormat> CreateVertexFormat(const VertexElement* InElements) SEOUL_OVERRIDE;
	// Surfaces
	virtual SharedPtr<DepthStencilSurface> CreateDepthStencilSurface(const DataStoreTableUtil& configSettings) SEOUL_OVERRIDE;
	virtual SharedPtr<RenderTarget> CreateRenderTarget(const DataStoreTableUtil& configSettings) SEOUL_OVERRIDE;

	// Index and Vertex buffers
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

	/**
	* @return True if the current render device supports immediate texture creation
	* off the render thread, false otherwise.
	*/
	virtual Bool SupportsAsyncCreateTexture() const SEOUL_OVERRIDE
	{
		return m_bAsyncCreate;
	}

	// Textures
	virtual SharedPtr<BaseTexture> AsyncCreateTexture(
		const TextureConfig& config,
		const TextureData& data,
		UInt uWidth,
		UInt uHeight,
		PixelFormat eFormat) SEOUL_OVERRIDE;
	virtual SharedPtr<BaseTexture> CreateTexture(
		const TextureConfig& config,
		const TextureData& data,
		UInt uWidth,
		UInt uHeight,
		PixelFormat eFormat) SEOUL_OVERRIDE;

	// Effects
	virtual SharedPtr<Effect> CreateEffectFromFileInMemory(
		FilePath filePath,
		void* pRawEffectFileData,
		UInt32 zFileSizeInBytes) SEOUL_OVERRIDE;

	ID3D11DeviceContext* GetD3DDeviceContext() const { return m_pD3DDeviceContext; }
	ID3D11Device* GetD3DDevice() const { return m_pD3DDevice; }

	virtual void SetDesiredVsyncInterval(Int iInterval) SEOUL_OVERRIDE
	{
		// Valid values in D3D11 are 0-4.
		iInterval = Clamp(iInterval, 0, 4);
		D3DCommonDevice::SetDesiredVsyncInterval(iInterval);
		m_GraphicsParameters.m_iVsyncInterval = iInterval;
	}

protected:
	// D3D11 specific initialization hooks. Vtable workaround.
	// Must be called in the constructor of the
	// child and destructor of the child.
	void Construct();
	void Destruct();

	virtual ID3D11Texture2D* AcquireBackBuffer() = 0;
	virtual Viewport InternalCreateDefaultViewport() const = 0;
	virtual void InitializeDirect3DDevice(
		ID3D11Device*& rpD3DDevice,
		ID3D11DeviceContext*& rpD3DDeviceContext) = 0;
	virtual void DeinitializeDirect3D() = 0;
	virtual Bool InternalDoCanRender() const = 0;
	virtual Bool InternalDoResetDevice() = 0;
	virtual RefreshRate InternalGetRefreshRate() const = 0;
	virtual Bool InternalPresent() = 0;
	virtual void InternalBeginScenePreResetCheck() = 0;
	virtual void InternalBeginScenePostResetCheck() = 0;
	const D3DCommonDeviceSettings& GetSettings() const { return m_Settings; }
	void SetNeedsReset() { m_bNeedsReset = true; }
	virtual void OnHasFrameToPresent() = 0;

	virtual void UpdateOsWindowRegions(OsWindowRegion const* pRegions, UInt32 uCount)
	{
		// Nop by default, can be specialized when a window is present.
	}

	// Functions and member variables in this private: block
	// are all related to window management.
private:
	// Also accesible to D3D11DepthStencilSurface, D3D11RenderTarget,
	// and D3D11RenderCommandStreamBuilder
	friend class DepthStencilSurface;
	friend class RenderTarget;
	friend class D3D11DepthStencilSurface;
	friend class D3D11RenderTarget;
	friend class D3D11RenderCommandStreamBuilder;

	// Set pSurface as the depth-stencil surface that will be used for
	// rendering. Must only be called by D3D11DepthStencilSurface.
	void SetDepthStencilSurface(D3D11DepthStencilSurface const* pSurface);

	// Set pTarget as the color surface that will be used for rendering.
	// D3D11RenderTarget.
	void SetRenderTarget(D3D11RenderTarget const* pTarget);

	// Apply any changes to the active depth-stencil and color targets.
	void CommitRenderSurface();

	// Clear render state back to the default.
	void ClearState();

	D3DCommonDeviceSettings m_Settings;
	Bool m_bAsyncCreate;
	Bool m_bNeedsReset;
	Bool m_bLost;

	void InternalLostDevice();
	Bool InternalResetDevice();
	void InternalUpdateCursorClip();

	// Handles checking and resetting the graphics device.
	Bool InternalDoReset();

	// Checks conditions of the device and window state for reset.
	Bool InternalCanRender();

	// Call each time a graphics object is created, to add it to the render device's
	// tracking of graphics objects.
	void InternalAddObject(const SharedPtr<BaseGraphicsObject>& pObject);

	// Called each frame to do object cleanup and setup.
	Bool InternalPerFrameMaintenance();

	// Called in the destructor, loops until the object count
	// does not change or until the graphics object count is
	// 0. Returns true if the graphics object count is 0, false
	// otherwise.
	Bool InternalDestructorMaintenance();

	// Begin PCEngine friend functions.
	virtual const GraphicsParameters& PCEngineFriend_GetGraphicsParameters() const SEOUL_OVERRIDE
	{
		return m_GraphicsParameters;
	}
	virtual const D3DCommonDeviceSettings& PCEngineFriend_GetSettings() const SEOUL_OVERRIDE
	{
		return m_Settings;
	}
	// /End PCEngine friend functions.

	typedef Vector< SharedPtr<BaseGraphicsObject>, MemoryBudgets::Rendering> GraphicsObjects;
	GraphicsObjects m_vGraphicsObjects;

	typedef AtomicRingBuffer<BaseGraphicsObject*> PendingGraphicsObjects;
	PendingGraphicsObjects m_PendingGraphicsObjects;

	Viewport m_BackBufferViewport;

	struct Surface SEOUL_SEALED
	{
		Surface()
			: m_pDepthStencil(nullptr)
			, m_pRenderTarget(nullptr)
		{
		}

		ID3D11DepthStencilView* m_pDepthStencil;
		ID3D11RenderTargetView* m_pRenderTarget;
	}; // struct Surface
	Surface m_CurrentRenderSurface;
	Bool m_bCurrentRenderSurfaceIsDirty;

	RefreshRate m_RefreshRate;
	ID3D11Device* m_pD3DDevice;
	ID3D11RenderTargetView* m_pD3DBackBufferRenderTargetView;
	ID3D11DeviceContext* m_pD3DDeviceContext;

	/**
	 * Per frame query that is used to prevent the CPU from
	 * becomming too far ahead of the GPU.
	 */
	ScopedPtr<D3D11EventQuery> m_pGpuSyncQuery;

	// Vertex format cache
	UInt m_iMaxVertexFormatSize;
	Vector< SharedPtr<D3D11VertexFormat>, MemoryBudgets::Rendering> m_vVertexFormats;
	Mutex m_VertexFormatsMutex;

	// Clears that use a scissor or viewport rectangle. Need to implement these
	// as a quad render.
	void ClearWithQuadRender(
		UInt32 uFlags,
		const Color4& cClearColor,
		Float fClearDepth,
		UInt8 uClearStencil);
	void CreateClearResources();
	void DestroyClearResources();
	ID3D11BlendState* m_pClearColorBS;
	ID3D11BlendState* m_pClearNoColorBS;
	ID3D11DepthStencilState* m_pClearColorOnly;
	ID3D11DepthStencilState* m_pClearDepth;
	ID3D11DepthStencilState* m_pClearDepthStencil;
	ID3D11DepthStencilState* m_pClearStencil;
	ID3D11RasterizerState* m_pClearRS;
	ID3D11PixelShader* m_pClearPS;
	ID3D11Buffer* m_pClearPSCB;
	ID3D11VertexShader* m_pClearVS;
	ID3D11Buffer* m_pClearVSCB;

	Bool GrabBackBufferFrame(const Rectangle2DInt& rect, SharedPtr<IFrameData>& rpFrameData);
	Bool ReadBackBufferPixel(Int32 iX, Int32 iY, ColorARGBu8& rcColor);
	CheckedPtr<ID3D11Texture2D> m_pOnePixelTextureSystem;

	void InternalApplyPendingRenderModeIndex();
	Int InternalGetActiveRenderModeIndex() const;

	ID3D11Texture2D* CreateStagingTexture(UInt32 uWidth, UInt32 uHeight, PixelFormat eFormat) const;
	void AcquireFrameData(UInt32 uWidth, UInt32 uHeight, SharedPtr<D3D11FrameData>& pFrameData);
	typedef Vector< SharedPtr<D3D11FrameData>, MemoryBudgets::Rendering > FrameData;
	FrameData m_vFrameData;
}; // class D3D11Device

/**
 * @return The global singleton reference to the current D3D11Device.
 *
 * \pre D3D11Device must have been created by initializing Engine.
 */
inline D3D11Device& GetD3D11Device()
{
	return *D3D11Device::Get();
}

} // namespace Seoul

#endif // include guard
