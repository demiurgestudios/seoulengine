/**
 * \file D3D11Device.cpp
 * \brief D3D11 specialization of RenderDevice.
 * D3D11Device is the root of D3D11 specific low-level graphics functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "D3D11DepthStencilSurface.h"
#include "D3D11Device.h"
#include "D3D11Effect.h"
#include "D3D11IndexBuffer.h"
#include "D3D11RenderCommandStreamBuilder.h"
#include "D3D11RenderTarget.h"
#include "D3D11Texture.h"
#include "D3D11VertexBuffer.h"
#include "D3D11VertexFormat.h"
#include "D3DCommonEffect.h"
#include "JobsManager.h"
#include "Thread.h"

// Must come last.
#include "D3D11ClearPS.h"
#include "D3D11ClearVS.h"

namespace Seoul
{

/**
 * Simple wrapper around the D3D11 query, used
 * to issue and wait for a single event.
 */
class D3D11EventQuery SEOUL_SEALED
{
public:
	D3D11EventQuery(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
		: m_pContext(pContext)
		, m_pQuery(nullptr)
		, m_bIssued(false)
	{
		SEOUL_ASSERT(IsRenderThread());

		m_pContext->AddRef();

		D3D11_QUERY_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.Query = D3D11_QUERY_EVENT;
		desc.MiscFlags = 0u;
		SEOUL_D3D11_VERIFY(pDevice->CreateQuery(&desc, &m_pQuery));
	}

	~D3D11EventQuery()
	{
		SEOUL_ASSERT(IsRenderThread());

		SafeRelease(m_pQuery);
		SafeRelease(m_pContext);
	}

	/**
	 * Issue the GPU event query - if it has already been issued, this becomes a nop.
	 */
	void Submit()
	{
		SEOUL_ASSERT(IsRenderThread());

		if (m_pQuery && !m_bIssued)
		{
			m_pContext->End(m_pQuery);
			m_bIssued = true;
		}
	}

	/**
	 * Wait for a previously issued query - if a query has not been issued, this method becomes
	 * a nop.
	 */
	void Wait()
	{
		SEOUL_ASSERT(IsRenderThread());

		if (m_pQuery && m_bIssued)
		{
			BOOL bIsComplete = TRUE;
			HRESULT hr = m_pContext->GetData(m_pQuery, &bIsComplete, sizeof(bIsComplete), 0u);
			while (S_FALSE == hr || (S_OK == hr && FALSE == bIsComplete))
			{
				Thread::YieldToAnotherThread();

				bIsComplete = TRUE;
				hr = m_pContext->GetData(m_pQuery, &bIsComplete, sizeof(bIsComplete), 0u);
			}

			m_bIssued = false;
		}
	}

private:
	ID3D11DeviceContext* m_pContext;
	ID3D11Query* m_pQuery;
	Bool m_bIssued;
};

/**
 * Constructor for the D3D device class
 *
 * @param[in] hInstance Handle to the instance of this application.
 * Passed in from the windows main.
 * @param[in] hWindow   Handle to the main window the this application
 */
D3D11Device::D3D11Device(const D3DCommonDeviceSettings& settings)
	: D3DCommonDevice(settings)
	, m_Settings(settings)
	, m_CurrentRenderSurface()
	, m_bCurrentRenderSurfaceIsDirty(false)
	, m_bAsyncCreate(false)
	, m_bNeedsReset(true)
	, m_bLost(true)
	, m_vGraphicsObjects()
	, m_PendingGraphicsObjects()
	, m_BackBufferViewport()
	, m_pD3DDevice(nullptr)
	, m_RefreshRate()
	, m_pGpuSyncQuery(nullptr)
	, m_pD3DBackBufferRenderTargetView(nullptr)
	, m_iMaxVertexFormatSize(0)
	, m_vVertexFormats()
	, m_VertexFormatsMutex()
	, m_pClearColorBS(nullptr)
	, m_pClearNoColorBS(nullptr)
	, m_pClearColorOnly(nullptr)
	, m_pClearDepth(nullptr)
	, m_pClearDepthStencil(nullptr)
	, m_pClearStencil(nullptr)
	, m_pClearRS(nullptr)
	, m_pClearPS(nullptr)
	, m_pClearPSCB(nullptr)
	, m_pClearVS(nullptr)
	, m_pClearVSCB(nullptr)
	, m_pOnePixelTextureSystem(nullptr)
	, m_pD3DDeviceContext(nullptr)
{
	SEOUL_ASSERT(IsRenderThread());

	// Setup features.
	m_Caps.m_bBlendMinMax = true;
	m_Caps.m_bBGRA = true;
	m_Caps.m_bETC1 = false;
	m_Caps.m_bIncompleteMipChain = true;

	// Initialize back buffer formats
	m_eBackBufferDepthStencilFormat = DepthStencilFormat::kD24S8;
	m_eBackBufferPixelFormat = PixelFormat::kA8R8G8B8;
	m_Caps.m_bBackBufferWithAlpha = PixelFormatHasAlpha(m_eBackBufferPixelFormat);
}

/**
 * Destructor for the D3D interface class.
 * Releases D3D resources created by the init calls.
 */
D3D11Device::~D3D11Device()
{
}

void D3D11Device::Construct()
{
	// Initialize the device based on the specific specialization.
	InitializeDirect3DDevice(m_pD3DDevice, m_pD3DDeviceContext);

	// Query whether async creates are supported.
	m_bAsyncCreate = false;
	{
		D3D11_FEATURE_DATA_THREADING data;
		memset(&data, 0, sizeof(data));
		if (SUCCEEDED(m_pD3DDevice->CheckFeatureSupport(
			D3D11_FEATURE_THREADING,
			&data,
			sizeof(data))))
		{
			m_bAsyncCreate = (FALSE != data.DriverConcurrentCreates);
		}
	}

	// Initialize the backbuffer viewport.
	m_BackBufferViewport = InternalCreateDefaultViewport();

	// Initialize the display refresh rate.
	m_RefreshRate = InternalGetRefreshRate();
}

void D3D11Device::Destruct()
{
	SEOUL_ASSERT(IsRenderThread());

	// Enter the lost state on shutdown.
	InternalLostDevice();

	m_vVertexFormats.Clear();

	if (RenderTarget::GetActiveRenderTarget())
	{
		RenderTarget::GetActiveRenderTarget()->Unselect();
	}
	if (DepthStencilSurface::GetActiveDepthStencilSurface())
	{
		DepthStencilSurface::GetActiveDepthStencilSurface()->Unselect();
	}

	// Teardown all our persistent objects.
	SEOUL_VERIFY(InternalDestructorMaintenance());

	// Release our submission context.
	SafeRelease(m_pD3DDeviceContext);

	// Destroy the D3D device.
	SafeRelease(m_pD3DDevice);
	DeinitializeDirect3D();
}

RenderCommandStreamBuilder* D3D11Device::CreateRenderCommandStreamBuilder(UInt32 zInitialCapacity) const
{
	return SEOUL_NEW(MemoryBudgets::Rendering) D3D11RenderCommandStreamBuilder(zInitialCapacity);
}

/**
 * Marks the beginning of one frame of rendering.
 *
 * Should be called once and only once at the start
 * of a frame, and be followed by a call to RenderDevice::EndScene()
 * at the end of the frame.
 */
Bool D3D11Device::BeginScene()
{
	SEOUL_ASSERT(IsRenderThread());

	// Device specific hook.
	InternalBeginScenePreResetCheck();

	// Check if we can render - if not, immediately attempt a reset.
	if (m_bNeedsReset || !InternalCanRender())
	{
		if (!InternalDoReset())
		{
			return false;
		}
	}

	// End specific hook.
	InternalBeginScenePostResetCheck();

	// Handle object reclaimation and promotion.
	if (!InternalPerFrameMaintenance())
	{
		return false;
	}

	m_bInScene = true;

	// Restore the active viewport to the default.
	D3D11_VIEWPORT d3d11Viewport = Convert(m_BackBufferViewport);
	m_pD3DDeviceContext->RSSetViewports(1u, &d3d11Viewport);

	return true;
}

/**
 * Marks the end of one frame of rendering.
 *
 * Should be called once and only once at the end
 * of a frame, and be preceeded by a call to RenderDevice::BeginScene()
 * at the beginning of the frame.
 */
void D3D11Device::EndScene()
{
	SEOUL_ASSERT(IsRenderThread());

	// Make sure we flush the context prior to queueing up a present.
	// Can get tearing (even with vsync turned on) without this.
	m_pD3DDeviceContext->Flush();

	if (InternalPresent())
	{
		// Wait for the previous query and then issue another for the next frame -
		// this allows us to control synchronization with the GPU, instead of
		// just overwhelming the driver and forcing it to stall for us.
		m_pGpuSyncQuery->Wait();
		m_pGpuSyncQuery->Submit();
	}

	m_bInScene = false;
}

/**
 * Returns the viewport that should be used for the backbuffer.
 */
const Viewport& D3D11Device::GetBackBufferViewport() const
{
	return m_BackBufferViewport;
}

// Get the screen refresh rate in hertz.
RefreshRate D3D11Device::GetDisplayRefreshRate() const
{
	return m_RefreshRate;
}

/**
 * Creates a new VertexFormat from the VertexElement array.
 *
 * @param[in] pElements Array of vertex elements, terminated with
 * a VertexElementEnd terminator.
 *
 * @return A GPU vertex format matching the description specified
 * by pElements.
 */
SharedPtr<VertexFormat> D3D11Device::CreateVertexFormat(VertexElement const* pElements)
{
	Lock lock(m_VertexFormatsMutex);

	UInt32 size = 0u;

	// Count the elements, stop at the terminator.
	while (VertexElementEnd != pElements[size])
	{
		size++;
	}

	// Add one to include the terminator.
	size++;

	// Get the number of currently active formats
	const UInt32 nFormatCount = m_vVertexFormats.GetSize();

	// For each format
	for (UInt32 i = 0u; i < nFormatCount; ++i)
	{
		// Get the vertex elements of the ith format.
		SEOUL_ASSERT(m_vVertexFormats[i].IsValid());
		const D3D11VertexFormat::VertexElements& vVertexElements =
			m_vVertexFormats[i]->GetVertexElements();

		// If the size of elements is not equal to the size of elements
		// that we are attempting to add, we're done - these formats
		// cannot be the same.
		if (size != vVertexElements.GetSize())
		{
			continue;
		}

		// Otherwise, we need to walk the elements and compare all
		// of the members of each.

		Bool bEquivalent = true;
		for (UInt32 j = 0u; j < size; ++j)
		{
			const VertexElement& element = vVertexElements[j];

			if ((pElements[j].m_Stream != element.m_Stream) ||
				(pElements[j].m_Offset != element.m_Offset) ||
				(pElements[j].m_Type != element.m_Type) ||
				(pElements[j].m_Method != element.m_Method) ||
				(pElements[j].m_Usage != element.m_Usage) ||
				(pElements[j].m_UsageIndex != element.m_UsageIndex))
			{
				bEquivalent = false;
				break;
			}
		}

		// If an existing format has the same elements as the
		// elements we're adding, return that format.
		if (bEquivalent)
		{
			return m_vVertexFormats[i];
		}
	}

	SharedPtr<D3D11VertexFormat> pFormat(SEOUL_NEW(MemoryBudgets::Rendering) D3D11VertexFormat(
		pElements));
	InternalAddObject(pFormat);
	m_vVertexFormats.PushBack(pFormat);
	return pFormat;
}


SharedPtr<DepthStencilSurface> D3D11Device::CreateDepthStencilSurface(const DataStoreTableUtil& configSettings)
{
	SharedPtr<DepthStencilSurface> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) D3D11DepthStencilSurface(configSettings));
	InternalAddObject(pReturn);
	return pReturn;
}

SharedPtr<RenderTarget> D3D11Device::CreateRenderTarget(const DataStoreTableUtil& configSettings)
{
	SharedPtr<RenderTarget> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) D3D11RenderTarget(configSettings));
	InternalAddObject(pReturn);
	return pReturn;
}

/**
 * Creates a new IndexBuffer
 *
 * @param[in]	Size	Size of the buffer in bytes
 * @param[in]	Usage	IndexBuffer usage
 * @param[in]	Format	Type of data stored in buffer (either Index16 or Index32)
 * @return 	The new IndexBuffer
 */
SharedPtr<IndexBuffer> D3D11Device::CreateIndexBuffer(
	void const* pInitialData,
	UInt32 zInitialDataSizeInBytes,
	UInt32 zTotalSizeInBytes,
	IndexBufferDataFormat eFormat)
{
	SharedPtr<IndexBuffer> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) D3D11IndexBuffer(
		pInitialData,
		zInitialDataSizeInBytes,
		zTotalSizeInBytes,
		eFormat,
		false));
	InternalAddObject(pReturn);
	return pReturn;
}

/**
 * Creates a new IndexBuffer that, for platforms on which the
 * distinction matters, is setup to be most efficient in situations
 * where data will be changed multiple times per frame.
 */
SharedPtr<IndexBuffer> D3D11Device::CreateDynamicIndexBuffer(
	UInt32 zTotalSizeInBytes,
	IndexBufferDataFormat eFormat)
{
	SharedPtr<IndexBuffer> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) D3D11IndexBuffer(
		nullptr,
		0u,
		zTotalSizeInBytes,
		eFormat,
		true));
	InternalAddObject(pReturn);
	return pReturn;
}

/**
 * Creates a new VertexBuffer
 *
 * @param[in]	Size	Size of the buffer in bytes
 * @param[in]	Usage	VertexBuffer usage
 * @return 	The new VertexBuffer
 */
SharedPtr<VertexBuffer> D3D11Device::CreateVertexBuffer(
	void const* pInitialData,
	UInt32 zInitialDataSizeInBytes,
	UInt zTotalSizeInBytes,
	UInt zStrideInBytes)
{
	SharedPtr<VertexBuffer> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) D3D11VertexBuffer(
		pInitialData,
		zInitialDataSizeInBytes,
		zTotalSizeInBytes,
		zStrideInBytes,
		false));
	InternalAddObject(pReturn);
	return pReturn;
}

/**
 * D3D11 specific function, creates a vertex buffer with
 * a system memory backup. Necessary for buffers that will
 * be modified at runtime.
 */
SharedPtr<VertexBuffer> D3D11Device::CreateDynamicVertexBuffer(
	UInt zTotalSizeInBytes,
	UInt zStrideInBytes)
{
	SharedPtr<VertexBuffer> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) D3D11VertexBuffer(
		nullptr,
		0u,
		zTotalSizeInBytes,
		zStrideInBytes,
		true));
	InternalAddObject(pReturn);
	return pReturn;
}

/**
 * Create a texture from a worker thread.
 */
SharedPtr<BaseTexture> D3D11Device::AsyncCreateTexture(
	const TextureConfig& config,
	const TextureData& data,
	UInt uWidth,
	UInt uHeight,
	PixelFormat eFormat)
{
	SharedPtr<BaseTexture> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) D3D11Texture(
		data,
		uWidth,
		uHeight,
		eFormat,
		GetDataSizeForPixelFormat(uWidth, uHeight, eFormat),
		false,
		true));
	InternalAddObject(pReturn);
	return pReturn;
}

/**
 * Create a texture not tied to a file with the given parameters.
 */
SharedPtr<BaseTexture> D3D11Device::CreateTexture(
	const TextureConfig& config,
	const TextureData& data,
	UInt uWidth,
	UInt uHeight,
	PixelFormat eFormat)
{
	SharedPtr<BaseTexture> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) D3D11Texture(
		data,
		uWidth,
		uHeight,
		eFormat,
		GetDataSizeForPixelFormat(uWidth, uHeight, eFormat),
		false,
		false));
	InternalAddObject(pReturn);
	return pReturn;
}

/**
 * Instantiate a new Effect instance from raw effect data.
 */
SharedPtr<Effect> D3D11Device::CreateEffectFromFileInMemory(
	FilePath filePath,
	void* pRawEffectFileData,
	UInt32 zFileSizeInBytes)
{
	// Validate the data first.
	if (!ValidateEffectData(true, (Byte const*)pRawEffectFileData, zFileSizeInBytes))
	{
		return SharedPtr<Effect>();
	}

	SharedPtr<Effect> pReturn(SEOUL_NEW(MemoryBudgets::Rendering) D3D11Effect(filePath, pRawEffectFileData, zFileSizeInBytes));
	InternalAddObject(pReturn);
	return pReturn;
}

namespace
{

SEOUL_DEFINE_PACKED_STRUCT(struct Float4
{
	union { Float32 m_fR; Float32 m_fX; };
	union { Float32 m_fG; Float32 m_fY; };
	union { Float32 m_fB; Float32 m_fZ; };
	union { Float32 m_fA; Float32 m_fW; };
});
SEOUL_STATIC_ASSERT(sizeof(Float4) == 16);

} // namespace

/**
 * Perform a clear (color, depth, stencil) with a full screen quad.
 * Necessary in D3D11 with a non-full screen viewport, as
*  the standard clear operations do not respect the scissor or viewport.
 */
void D3D11Device::ClearWithQuadRender(
	UInt32 uFlags,
	const Color4& cClearColor,
	Float fClearDepth,
	UInt8 uClearStencil)
{
	// Backup current values.
	ID3D11BlendState* pOldBlendState = nullptr;
	Float afOldBlendFactors[4] = { 0, 0, 0, 0 };
	UInt32 uOldSampleMask = 0u;
	m_pD3DDeviceContext->OMGetBlendState(&pOldBlendState, afOldBlendFactors, &uOldSampleMask);

	ID3D11DepthStencilState* pOldDepthStencilState = nullptr;
	UInt32 uOldStencilRef = 0u;
	m_pD3DDeviceContext->OMGetDepthStencilState(&pOldDepthStencilState, &uOldStencilRef);

	ID3D11RasterizerState* pOldRasterizerState = nullptr;
	m_pD3DDeviceContext->RSGetState(&pOldRasterizerState);

	ID3D11InputLayout* pOldLayout = nullptr;
	m_pD3DDeviceContext->IAGetInputLayout(&pOldLayout);

	ID3D11PixelShader* pOldPixelShader = nullptr;
	m_pD3DDeviceContext->PSGetShader(&pOldPixelShader, nullptr, nullptr);

	ID3D11VertexShader* pOldVertexShader = nullptr;
	m_pD3DDeviceContext->VSGetShader(&pOldVertexShader, nullptr, nullptr);

	D3D11_PRIMITIVE_TOPOLOGY eOldTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_pD3DDeviceContext->IAGetPrimitiveTopology(&eOldTopology);

	// Set the clear depth.
	{
		D3D11_MAPPED_SUBRESOURCE map;
		memset(&map, 0, sizeof(map));
		m_pD3DDeviceContext->Map(m_pClearVSCB, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &map);

		Float4 depth;
		memset(&depth, 0, sizeof(depth));
		depth.m_fX = fClearDepth;
		memcpy(map.pData, &depth, sizeof(depth));

		m_pD3DDeviceContext->Unmap(m_pClearVSCB, 0u);
	}

	// Set the clear color.
	{
		D3D11_MAPPED_SUBRESOURCE map;
		memset(&map, 0, sizeof(map));
		m_pD3DDeviceContext->Map(m_pClearPSCB, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &map);

		Float4 color;
		color.m_fR = cClearColor.R;
		color.m_fG = cClearColor.G;
		color.m_fB = cClearColor.B;
		color.m_fA = cClearColor.A;
		memcpy(map.pData, &color, sizeof(color));

		m_pD3DDeviceContext->Unmap(m_pClearPSCB, 0u);
	}

	// Select the BS state we need to use based on flags.
	ID3D11BlendState* pBS = nullptr;
	if ((UInt32)ClearFlags::kColorTarget == ((UInt32)ClearFlags::kColorTarget & uFlags))
	{
		pBS = m_pClearColorBS;
	}
	else
	{
		pBS = m_pClearNoColorBS;
	}

	// Select the DS state we need to use based on flags.
	ID3D11DepthStencilState* pDS = nullptr;
	switch ((uFlags & (UInt32)(ClearFlags::kDepthTarget | ClearFlags::kStencilTarget)))
	{
	case (UInt32)(ClearFlags::kDepthTarget | ClearFlags::kStencilTarget): pDS = m_pClearDepthStencil; break; // clear both.
	case (UInt32)(ClearFlags::kDepthTarget): pDS = m_pClearDepth; break; // clear depth only.
	case (UInt32)(ClearFlags::kStencilTarget): pDS = m_pClearStencil; break; // clear stencil only.
	default: // no depth or stencil clear.
		pDS = m_pClearColorOnly;
		break;
	};

	// Set our desired values.
	m_pD3DDeviceContext->OMSetBlendState(pBS, Vector4D::Zero().GetData(), 0xFFFFFFFF);
	m_pD3DDeviceContext->OMSetDepthStencilState(pDS, uClearStencil);
	m_pD3DDeviceContext->RSSetState(m_pClearRS);
	m_pD3DDeviceContext->IASetInputLayout(nullptr);
	m_pD3DDeviceContext->PSSetShader(m_pClearPS, nullptr, 0u);
	m_pD3DDeviceContext->PSSetConstantBuffers(0u, 1u, &m_pClearPSCB);
	m_pD3DDeviceContext->VSSetShader(m_pClearVS, nullptr, 0u);
	m_pD3DDeviceContext->VSSetConstantBuffers(0u, 1u, &m_pClearVSCB);

	// Render.
	m_pD3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_pD3DDeviceContext->Draw(4u, 0u);

	// Restore old values.
	m_pD3DDeviceContext->IASetPrimitiveTopology(eOldTopology);
	m_pD3DDeviceContext->VSSetShader(pOldVertexShader, nullptr, 0u);
	m_pD3DDeviceContext->PSSetShader(pOldPixelShader, nullptr, 0u);
	m_pD3DDeviceContext->IASetInputLayout(pOldLayout);
	m_pD3DDeviceContext->RSSetState(pOldRasterizerState);
	m_pD3DDeviceContext->OMSetDepthStencilState(pOldDepthStencilState, uOldStencilRef);
	m_pD3DDeviceContext->OMSetBlendState(pOldBlendState, afOldBlendFactors, uOldSampleMask);

	// Release.
	SafeRelease(pOldVertexShader);
	SafeRelease(pOldPixelShader);
	SafeRelease(pOldLayout);
	SafeRelease(pOldRasterizerState);
	SafeRelease(pOldDepthStencilState);
	SafeRelease(pOldBlendState);
}

void D3D11Device::CreateClearResources()
{
	// Blend states.
	{
		// Color writes enabled.
		{
			D3D11_BLEND_DESC desc;
			memset(&desc, 0, sizeof(desc));
			desc.AlphaToCoverageEnable = FALSE;
			desc.IndependentBlendEnable = FALSE;
			desc.RenderTarget[0].BlendEnable = FALSE;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			SEOUL_D3D11_VERIFY(m_pD3DDevice->CreateBlendState(&desc, &m_pClearColorBS));
		}

		// Color writes disabled.
		{
			D3D11_BLEND_DESC desc;
			memset(&desc, 0, sizeof(desc));
			desc.AlphaToCoverageEnable = FALSE;
			desc.IndependentBlendEnable = FALSE;
			desc.RenderTarget[0].BlendEnable = FALSE;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].RenderTargetWriteMask = 0;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			SEOUL_D3D11_VERIFY(m_pD3DDevice->CreateBlendState(&desc, &m_pClearNoColorBS));
		}
	}

	// Depth-stencil states.
	{
		// Both clear.
		{
			D3D11_DEPTH_STENCIL_DESC desc;
			memset(&desc, 0, sizeof(desc));
			desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.DepthEnable = TRUE;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.StencilEnable = TRUE;
			desc.StencilReadMask = 0u;
			desc.StencilWriteMask = 0xFF;
			SEOUL_D3D11_VERIFY(m_pD3DDevice->CreateDepthStencilState(&desc, &m_pClearDepthStencil));
		}

		// Clear depth only.
		{
			D3D11_DEPTH_STENCIL_DESC desc;
			memset(&desc, 0, sizeof(desc));
			desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			desc.DepthEnable = TRUE;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			desc.StencilEnable = FALSE;
			desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
			desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
			SEOUL_D3D11_VERIFY(m_pD3DDevice->CreateDepthStencilState(&desc, &m_pClearDepth));
		}

		// Clear stencil only.
		{
			D3D11_DEPTH_STENCIL_DESC desc;
			memset(&desc, 0, sizeof(desc));
			desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.DepthEnable = FALSE;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.StencilEnable = TRUE;
			desc.StencilReadMask = 0u;
			desc.StencilWriteMask = 0xFF;
			SEOUL_D3D11_VERIFY(m_pD3DDevice->CreateDepthStencilState(&desc, &m_pClearStencil));
		}

		// Clear only color, no depth or stencil clear.
		{
			D3D11_DEPTH_STENCIL_DESC desc;
			memset(&desc, 0, sizeof(desc));
			desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			desc.DepthEnable = FALSE;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			desc.StencilEnable = FALSE;
			desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
			desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
			SEOUL_D3D11_VERIFY(m_pD3DDevice->CreateDepthStencilState(&desc, &m_pClearColorOnly));
		}
	}

	// Rasterizer state.
	{
		D3D11_RASTERIZER_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.AntialiasedLineEnable = FALSE;
		desc.CullMode = D3D11_CULL_BACK;
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0.0f;
		desc.DepthClipEnable = TRUE;
		desc.FillMode = D3D11_FILL_SOLID;
		desc.FrontCounterClockwise = FALSE;
		desc.MultisampleEnable = FALSE;
		desc.ScissorEnable = TRUE; // TODO: This works because we always enable the scissor otherwise, but will break if that changes.
		desc.SlopeScaledDepthBias = 0.0f;
		SEOUL_D3D11_VERIFY(m_pD3DDevice->CreateRasterizerState(&desc, &m_pClearRS));
	}

	// Pixel shader.
	{
		SEOUL_D3D11_VERIFY(m_pD3DDevice->CreatePixelShader(g_D3D11ClearPS, sizeof(g_D3D11ClearPS), nullptr, &m_pClearPS));
	}

	// Pixel shader constant buffer.
	{
		D3D11_BUFFER_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.ByteWidth = sizeof(Float4);
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0u;
		desc.StructureByteStride = 0u;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		SEOUL_D3D11_VERIFY(m_pD3DDevice->CreateBuffer(&desc, nullptr, &m_pClearPSCB));
	}

	// Vertex shader.
	{
		SEOUL_D3D11_VERIFY(m_pD3DDevice->CreateVertexShader(g_D3D11ClearVS, sizeof(g_D3D11ClearVS), nullptr, &m_pClearVS));
	}

	// Vertex shader constant buffer.
	{
		D3D11_BUFFER_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.ByteWidth = sizeof(Float4);
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0u;
		desc.StructureByteStride = 0u;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		SEOUL_D3D11_VERIFY(m_pD3DDevice->CreateBuffer(&desc, nullptr, &m_pClearVSCB));
	}
}

void D3D11Device::DestroyClearResources()
{
	SafeRelease(m_pClearVSCB);
	SafeRelease(m_pClearVS);
	SafeRelease(m_pClearPSCB);
	SafeRelease(m_pClearPS);
	SafeRelease(m_pClearRS);
	SafeRelease(m_pClearStencil);
	SafeRelease(m_pClearDepthStencil);
	SafeRelease(m_pClearDepth);
	SafeRelease(m_pClearColorOnly);
	SafeRelease(m_pClearNoColorBS);
	SafeRelease(m_pClearColorBS);
}

/** Utility for which we known the layout of the structure. */
namespace
{

SEOUL_DEFINE_PACKED_STRUCT(struct BackBufferPixel
{
	UInt8 m_B;
	UInt8 m_G;
	UInt8 m_R;
	UInt8 m_A;
});
SEOUL_STATIC_ASSERT(sizeof(BackBufferPixel) == 4);

} // namespace anonymous

class D3D11FrameData SEOUL_SEALED : public IFrameData
{
public:
	D3D11FrameData(UInt32 uWidth, UInt32 uHeight, PixelFormat eFormat, ID3D11Texture2D* pTexture)
		: m_uWidth(uWidth)
		, m_uHeight(uHeight)
		, m_eFormat(eFormat)
		, m_pTexture(pTexture)
		, m_Mapped()
		, m_uGrabWidth(0u)
		, m_uGrabHeight(0u)
	{
		memset(&m_Mapped, 0, sizeof(m_Mapped));
	}

	~D3D11FrameData()
	{
		SEOUL_ASSERT(!IsMapped());
		SafeRelease(m_pTexture);
	}

	virtual void const* GetData() const SEOUL_OVERRIDE { return m_Mapped.pData; }
	virtual UInt32 GetFrameHeight() const SEOUL_OVERRIDE { return m_uGrabHeight; }
	virtual UInt32 GetFrameWidth() const SEOUL_OVERRIDE { return m_uGrabWidth; }
	virtual UInt32 GetPitch() const SEOUL_OVERRIDE { return m_Mapped.RowPitch; }
	virtual PixelFormat GetPixelFormat() const SEOUL_OVERRIDE { return m_eFormat; }

	UInt32 GetHeight() const { return m_uHeight; }
	UInt32 GetWidth() const { return m_uWidth; }
	Bool IsMapped() const { return (nullptr != m_Mapped.pData); }

	void Grab(ID3D11DeviceContext* p, const Rectangle2DInt& rect, ID3D11Texture2D* pTexture)
	{
		SEOUL_ASSERT(IsRenderThread());

		D3D11_BOX src;
		memset(&src, 0, sizeof(src));
		src.back = 1;
		src.bottom = rect.m_iBottom;
		src.front = 0;
		src.left = rect.m_iLeft;
		src.right = rect.m_iRight;
		src.top = rect.m_iTop;

		// Make sure we don't have a pending map.
		Unmap(p);

		// Cache sizes.
		m_uGrabWidth = (UInt32)(src.right - src.left);
		m_uGrabHeight = (UInt32)(src.bottom - src.top);

		// Copy the resource.
		p->CopySubresourceRegion(
			m_pTexture,
			0,
			0, 0, 0,
			pTexture,
			0,
			&src);

		// TODO: Allow this to fail and retry later on the render thread.
		SEOUL_VERIFY(TryMap(p));
	}

	Bool TryMap(ID3D11DeviceContext* p)
	{
		SEOUL_ASSERT(IsRenderThread());

		// Don't remap.
		if (IsMapped())
		{
			return false;
		}

		auto const e = p->Map(m_pTexture, 0u, D3D11_MAP_READ, 0u/* TODO: D3D11_MAP_FLAG_DO_NOT_WAIT*/, &m_Mapped);
		if (!SUCCEEDED(e))
		{
			memset(&m_Mapped, 0, sizeof(m_Mapped));
			return false;
		}

		return true;
	}

	void Unmap(ID3D11DeviceContext* p)
	{
		SEOUL_ASSERT(IsRenderThread());

		// No need to unmap.
		if (!IsMapped())
		{
			return;
		}

		memset(&m_Mapped, 0, sizeof(m_Mapped));
		p->Unmap(m_pTexture, 0u);
	}

private:
	UInt32 const m_uWidth;
	UInt32 const m_uHeight;
	PixelFormat const m_eFormat;
	ID3D11Texture2D* m_pTexture;
	D3D11_MAPPED_SUBRESOURCE m_Mapped;
	UInt32 m_uGrabWidth;
	UInt32 m_uGrabHeight;

	SEOUL_REFERENCE_COUNTED_SUBCLASS(D3D11FrameData);
}; // class D3D11FrameData

Bool D3D11Device::GrabBackBufferFrame(const Rectangle2DInt& rect, SharedPtr<IFrameData>& rpFrameData)
{
	SEOUL_ASSERT(IsRenderThread());

	if (!InternalCanRender())
	{
		return false;
	}

	// Get a reference to the back buffer resource.
	auto pBackBuffer = AcquireBackBuffer();
	if (nullptr == pBackBuffer)
	{
		return false;
	}

	// Get the frame data.
	SharedPtr<D3D11FrameData> pData;
	AcquireFrameData(
		(UInt32)(rect.m_iRight - rect.m_iLeft),
		(UInt32)(rect.m_iBottom - rect.m_iTop),
		pData);

	// Copy.
	pData->Grab(m_pD3DDeviceContext, rect, pBackBuffer);

	// Release the back buffer reference we acquired.
	SafeRelease(pBackBuffer);

	rpFrameData = pData;
	return true;
}

/**
 * Access to the back buffer color, single pixel access.
 * Must be called from the render thread.
 */
Bool D3D11Device::ReadBackBufferPixel(Int32 iX, Int32 iY, ColorARGBu8& rcColor)
{
	SEOUL_ASSERT(IsRenderThread());

	if (!InternalCanRender())
	{
		return false;
	}

	// Get a reference to the back buffer resource.
	auto pBackBuffer = AcquireBackBuffer();
	if (nullptr == pBackBuffer)
	{
		return false;
	}

	D3D11_BOX src;
	memset(&src, 0, sizeof(src));
	src.back = 1;
	src.bottom = (iY + 1);
	src.front = 0;
	src.left = iX;
	src.right = (iX + 1);
	src.top = iY;
	m_pD3DDeviceContext->CopySubresourceRegion(
		m_pOnePixelTextureSystem,
		0,
		0, 0, 0,
		pBackBuffer,
		0,
		&src);

	// Release the back buffer reference we acquired.
	SafeRelease(pBackBuffer);

	D3D11_MAPPED_SUBRESOURCE mapped;
	memset(&mapped, 0, sizeof(mapped));
	if (!SUCCEEDED(m_pD3DDeviceContext->Map(
		m_pOnePixelTextureSystem,
		0,
		D3D11_MAP_READ,
		0,
		&mapped)))
	{
		return false;
	}

	BackBufferPixel pixel;
	memcpy(&pixel, mapped.pData, sizeof(pixel));
	m_pD3DDeviceContext->Unmap(m_pOnePixelTextureSystem, 0);

	// Swap if an unexpected format.
	rcColor.m_R = pixel.m_R;
	rcColor.m_G = pixel.m_G;
	rcColor.m_B = pixel.m_B;
	rcColor.m_A = pixel.m_A;
	if (PixelFormat::kA8B8G8R8 == m_eBackBufferPixelFormat)
	{
		Swap(rcColor.m_R, rcColor.m_B);
	}

	return true;
}

/**
 * Causes graphics objects (not the device) to be placed in a lost
 * state. Some objects
 * are released, others have their OnLost() member function called.
 */
void D3D11Device::InternalLostDevice()
{
	SEOUL_ASSERT(IsRenderThread());

	m_bLost = true;

	// Release state.
	ClearState();

	// Unselect the depth-stencil surface.
	if (DepthStencilSurface::GetActiveDepthStencilSurface())
	{
		DepthStencilSurface::GetActiveDepthStencilSurface()->Unselect();
	}

	// Destroy object used for read pixel.
	SafeRelease(m_pOnePixelTextureSystem);

	// Destroy clear resources.
	DestroyClearResources();

	// Destroy the GPU sync object.
	m_pGpuSyncQuery.Reset();

	// Lose graphics objects.
	Int const nCount = (Int)m_vGraphicsObjects.GetSize();
	for (Int i = (nCount - 1); i >= 0; --i)
	{
		if (BaseGraphicsObject::kReset == m_vGraphicsObjects[i]->GetState())
		{
			m_vGraphicsObjects[i]->OnLost();
		}
	}

	// Release the back buffer render target view.
	SafeRelease(m_pD3DBackBufferRenderTargetView);
}

/**
 * Handles reset of the device. If device reset fails in
 * an expected manner, returns false. If device reset fails in
 * an unexpected manner, returns false and fails with an assert
 * in non-ship builds. Otherwise, reset the device, calls
 * OnReset() on any graphics objects, and returns true.
 */
Bool D3D11Device::InternalResetDevice()
{
	SEOUL_ASSERT(IsRenderThread());

	// Specialization specific device reset.
	if (!InternalDoResetDevice())
	{
		return false;
	}

	// Initialize the back buffer render target view
	{
		ID3D11Texture2D* pBackBuffer = AcquireBackBuffer();
		if (nullptr == pBackBuffer)
		{
			return false;
		}

		D3D11_TEXTURE2D_DESC textureDesc;
		memset(&textureDesc, 0, sizeof(textureDesc));
		pBackBuffer->GetDesc(&textureDesc);

		D3D11_RENDER_TARGET_VIEW_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.Format = textureDesc.Format;
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		SEOUL_D3D11_VERIFY(GetD3DDevice()->CreateRenderTargetView(
			pBackBuffer,
			&desc,
			&m_pD3DBackBufferRenderTargetView));

		m_eBackBufferPixelFormat = D3DToPixelFormat(desc.Format);
		m_Caps.m_bBackBufferWithAlpha = PixelFormatHasAlpha(m_eBackBufferPixelFormat);

		// Release the back buffer reference we acquired.
		SafeRelease(pBackBuffer);
	}

	// Reset surface state.
	m_CurrentRenderSurface.m_pRenderTarget = m_pD3DBackBufferRenderTargetView;
	m_CurrentRenderSurface.m_pDepthStencil = nullptr;

	// Initialize the backbuffer viewport.
	m_BackBufferViewport = InternalCreateDefaultViewport();

	// Initialize the display refresh rate.
	m_RefreshRate = InternalGetRefreshRate();

	// Reset graphics objects.
	UInt32 const nCount = m_vGraphicsObjects.GetSize();
	for (UInt32 i = 0u; i < nCount; ++i)
	{
		if (BaseGraphicsObject::kDestroyed == m_vGraphicsObjects[i]->GetState())
		{
			if (!m_vGraphicsObjects[i]->OnCreate())
			{
				continue;
			}
		}

		if (BaseGraphicsObject::kCreated == m_vGraphicsObjects[i]->GetState())
		{
			m_vGraphicsObjects[i]->OnReset();
		}
	}

	// Recreate the sync query.
	m_pGpuSyncQuery.Reset(SEOUL_NEW(MemoryBudgets::Rendering) D3D11EventQuery(m_pD3DDevice, m_pD3DDeviceContext));

	// Create clear resources.
	CreateClearResources();

	// Create texture used for read pixel.
	m_pOnePixelTextureSystem.Reset(CreateStagingTexture(1u, 1u, m_eBackBufferPixelFormat));

	m_bLost = false;

	return true;
}

/**
 * Set pSurface as the depth-stencil surface that will be used
 * for rendering. Must only be called by D3D11DepthStencilSurface.
 */
void D3D11Device::SetDepthStencilSurface(D3D11DepthStencilSurface const* pSurface)
{
	SEOUL_ASSERT(IsRenderThread());

	m_bCurrentRenderSurfaceIsDirty = true;

	// If setting a nullptr surface, disable the depth-stencil surface.
	if (!pSurface)
	{
		m_CurrentRenderSurface.m_pDepthStencil = nullptr;
	}
	else
	{
		m_CurrentRenderSurface.m_pDepthStencil = pSurface->m_pDepthStencilView;
	}
}

/**
 * Set pTarget as the color surface that will be used
 * for rendering. Must only be called by D3D11RenderTarget2D.
 */
void D3D11Device::SetRenderTarget(D3D11RenderTarget const* pRenderTarget)
{
	SEOUL_ASSERT(IsRenderThread());

	m_bCurrentRenderSurfaceIsDirty = true;

	if (pRenderTarget)
	{
		m_CurrentRenderSurface.m_pRenderTarget = pRenderTarget->m_pRenderTargetViewA;
	}
	else
	{
		m_CurrentRenderSurface.m_pRenderTarget = m_pD3DBackBufferRenderTargetView;
	}
}

/**
 * Commits any changes to the depth-stencil targets
 * and color targets to the OpenGL API.
 */
void D3D11Device::CommitRenderSurface()
{
	SEOUL_ASSERT(IsRenderThread());

	// If the render surface has changes since the last
	// commit, update those changes.
	if (m_bCurrentRenderSurfaceIsDirty)
	{
		m_pD3DDeviceContext->OMSetRenderTargets(
			1u,
			&m_CurrentRenderSurface.m_pRenderTarget,
			m_CurrentRenderSurface.m_pDepthStencil);

		m_bCurrentRenderSurfaceIsDirty = false;
	}
}

/**
 * Clear render state back to the default.
 */
void D3D11Device::ClearState()
{
	SEOUL_ASSERT(IsRenderThread());

	// Clear all render state.
	m_pD3DDeviceContext->ClearState();

	// Reset surface state.
	m_CurrentRenderSurface.m_pRenderTarget = m_pD3DBackBufferRenderTargetView;
	m_CurrentRenderSurface.m_pDepthStencil = nullptr;

	// Mark the surface as dirty.
	m_bCurrentRenderSurfaceIsDirty = true;
}

/**
 * Resets the device if necessary. Returns false if the reset
 * fails in a valid way. In non-ship builds, asserts with
 * a failure if the reset fails in an invalid way.
 */
Bool D3D11Device::InternalDoReset()
{
	SEOUL_ASSERT(IsRenderThread());

	if (m_bNeedsReset)
	{
		// Report device lost to all graphics objects.
		InternalLostDevice();

		// Now attempt to reset.
		if (!InternalResetDevice())
		{
			return false;
		}

		m_bNeedsReset = false;
	}

	return true;
}

/**
 * @return True if the device and window fulfills conditions for reset, false otherwise.
 */
Bool D3D11Device::InternalCanRender()
{
	// If the internal D3D11Device state is lost, we can't render.
	if (m_bLost)
	{
		return false;
	}

	return InternalDoCanRender();
}

/**
 * Called on new graphics objects so they end up in the graphics list - this
 * can only be performed on the render thread, so this function may insert the
 * object into a thread-safe queue for later processing on the render thread.
 */
void D3D11Device::InternalAddObject(const SharedPtr<BaseGraphicsObject>& pObject)
{
	if (IsRenderThread())
	{
		if (!m_bNeedsReset)
		{
			if (pObject->OnCreate())
			{
				pObject->OnReset();
			}
		}

		m_vGraphicsObjects.PushBack(pObject);
	}
	else
	{
		SeoulGlobalIncrementReferenceCount(pObject.GetPtr());
		m_PendingGraphicsObjects.Push(pObject.GetPtr());
	}
}

/**
 * Called once per frame to do per-frame object cleanup and maintenance operations.
 */
Bool D3D11Device::InternalPerFrameMaintenance()
{
	// Cleanup existing objects
	Int nCount = (Int)m_vGraphicsObjects.GetSize();
	for (Int i = 0; i < nCount; ++i)
	{
		// If we have a unique reference, the object is no longer in use, so it can be destroyed.
		if (m_vGraphicsObjects[i].IsUnique())
		{
			m_vGraphicsObjects[i].Swap(m_vGraphicsObjects.Back());
			if (BaseGraphicsObject::kReset == m_vGraphicsObjects.Back()->GetState())
			{
				m_vGraphicsObjects.Back()->OnLost();
			}

			m_vGraphicsObjects.PopBack();
			--i;
			--nCount;
			continue;
		}

		// If an object is in the destroyed state, create it.
		if (BaseGraphicsObject::kDestroyed == m_vGraphicsObjects[i]->GetState())
		{
			// If we fail creating it, nothing more we can do.
			if (!m_vGraphicsObjects[i]->OnCreate())
			{
				return false;
			}
		}

		// If an object is in the lost state, reset it.
		if (BaseGraphicsObject::kCreated == m_vGraphicsObjects[i]->GetState())
		{
			m_vGraphicsObjects[i]->OnReset();
		}
	}

	// Handle pending objects in the queue.
	SharedPtr<BaseGraphicsObject> pObject(m_PendingGraphicsObjects.Pop());
	while (pObject.IsValid())
	{
		// Need to decrement the reference count once - it was incremented before inserting into the queue.
		SeoulGlobalDecrementReferenceCount(pObject.GetPtr());

		InternalAddObject(pObject);
		pObject.Reset(m_PendingGraphicsObjects.Pop());
	}

	// Also unmap any frame data objects that are now unique.
	for (auto const& p : m_vFrameData)
	{
		if (p.IsUnique())
		{
			p->Unmap(m_pD3DDeviceContext);
		}
	}

	return true;
}

/**
 * Called in the destructor, loops until the object count
 * does not change or until the graphics object count is
 * 0. Returns true if the graphics object count is 0, false
 * otherwise.
 */
Bool D3D11Device::InternalDestructorMaintenance()
{
	// Propagate pending objects.
	{
		Atomic32Type pendingObjectCount = m_PendingGraphicsObjects.GetCount();
		while (pendingObjectCount != 0)
		{
			if (!InternalPerFrameMaintenance())
			{
				return false;
			}

			Atomic32Type newPendingObjectCount = m_PendingGraphicsObjects.GetCount();
			if (newPendingObjectCount == pendingObjectCount)
			{
				break;
			}

			pendingObjectCount = newPendingObjectCount;
		}
	}

	// Now cleanup objects.
	{
		UInt32 uObjectCount = m_vGraphicsObjects.GetSize();
		while (uObjectCount != 0u)
		{
			if (!InternalPerFrameMaintenance())
			{
				return false;
			}

			UInt32 nNewObjectCount = m_vGraphicsObjects.GetSize();
			if (uObjectCount == nNewObjectCount)
			{
				return (0u == nNewObjectCount);
			}

			uObjectCount = nNewObjectCount;
		}
	}

	// Cleanup frame data.
	Int64 const kiMaxWaitTimeInTicks = SeoulTime::ConvertMillisecondsToTicks(500.0);
	Int64 iWaitTimeInTicks = 0;
	while (true)
	{
		Int32 iSize = (Int32)m_vFrameData.GetSize();
		Int32 iPrevSize = iSize;
		for (Int32 i = 0; i < iSize; ++i)
		{
			auto const& p = m_vFrameData[i];
			if (p.IsUnique())
			{
				p->Unmap(m_pD3DDeviceContext);
				Swap(m_vFrameData[i], m_vFrameData[iSize - 1]);
				--i;
				--iSize;
			}
		}
		m_vFrameData.Resize(iSize);

		if (iSize > 0 && iSize == iPrevSize)
		{
			// Give threaded jobs a chance to dispatch. We wait
			// a maximum of kuMilliseconds.
			auto const iStart = SeoulTime::GetGameTimeInTicks();
			Jobs::Manager::Get()->YieldThreadTime();
			iWaitTimeInTicks += (SeoulTime::GetGameTimeInTicks() - iStart);

			if (iWaitTimeInTicks > kiMaxWaitTimeInTicks)
			{
				return false;
			}

			continue;
		}

		// Done, success.
		break;
	}

	// Sanity checks.
	SEOUL_ASSERT(m_PendingGraphicsObjects.GetCount() == 0);
	SEOUL_ASSERT(m_vGraphicsObjects.IsEmpty());
	SEOUL_ASSERT(m_vFrameData.IsEmpty());

	return true;
}

ID3D11Texture2D* D3D11Device::CreateStagingTexture(UInt32 uWidth, UInt32 uHeight, PixelFormat eFormat) const
{
	SEOUL_ASSERT(IsRenderThread());

	D3D11_TEXTURE2D_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.ArraySize = 1;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Format = PixelFormatToD3D(eFormat);
	desc.Height = uHeight;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.Width = uWidth;

	ID3D11Texture2D* pTexture = nullptr;
	if (SUCCEEDED(m_pD3DDevice->CreateTexture2D(
		&desc,
		nullptr,
		&pTexture)))
	{
		return pTexture;
	}
	else
	{
		return nullptr;
	}
}

void D3D11Device::AcquireFrameData(UInt32 uWidth, UInt32 uHeight, SharedPtr<D3D11FrameData>& rpFrameData)
{
	SEOUL_ASSERT(IsRenderThread());

	// Find the first existing frame data that has a unique reference count
	// and has dimensions >= uWidth and uHeight.
	for (auto const& p : m_vFrameData)
	{
		if (p.IsUnique())
		{
			if (p->GetHeight() >= uHeight &&
				p->GetWidth() >= uWidth)
			{
				rpFrameData = p;
				return;
			}
		}
	}

	// Insert a new entry.
	rpFrameData.Reset(SEOUL_NEW(MemoryBudgets::Rendering) D3D11FrameData(uWidth, uHeight, m_eBackBufferPixelFormat, CreateStagingTexture(uWidth, uHeight, m_eBackBufferPixelFormat)));
	m_vFrameData.PushBack(rpFrameData);
}

} // namespace Seoul
