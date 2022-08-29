/**
 * \file D3D11DeviceHeadless.cpp
 * \brief Specialization of D3D11Device that
 * uses no window. Useful for automated
 * tests that require a graphical rendering
 * context.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "D3D11DeviceHeadless.h"
#include "Thread.h"

namespace Seoul
{

D3DCommonDevice* D3D11DeviceHeadless::CreateDeviceD3D11(const D3DCommonDeviceSettings& deviceSettings)
{
	return SEOUL_NEW(MemoryBudgets::Rendering) D3D11DeviceHeadless(deviceSettings);
}

Bool D3D11DeviceHeadless::IsSupportedD3D11(const D3DCommonDeviceSettings& deviceSettings)
{
	if (!deviceSettings.m_sPreferredBackend.IsEmpty() &&
		deviceSettings.m_sPreferredBackend != "D3D11")
	{
		return false;
	}

	return SUCCEEDED(D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0u,
		kaFeatureLevels,
		SEOUL_ARRAY_COUNT(kaFeatureLevels),
		D3D11_SDK_VERSION,
		nullptr,
		nullptr,
		nullptr));
}

D3D11DeviceHeadless::D3D11DeviceHeadless(const D3DCommonDeviceSettings& settings)
	: D3D11Device(settings)
	, m_BackBufferViewport()
	, m_pBackBuffer(nullptr)
	, m_iPresentTimeStamp(-1)
	, m_bHasFrameToPresent(false)
{
	auto const iWidth = settings.m_iPreferredViewportWidth > 0
		? settings.m_iPreferredViewportWidth
		: m_GraphicsParameters.m_iWindowViewportWidth;
	auto const iHeight = settings.m_iPreferredViewportHeight > 0
		? settings.m_iPreferredViewportHeight
		: m_GraphicsParameters.m_iWindowViewportHeight;

	m_BackBufferViewport = Viewport::Create(
		iWidth,
		iHeight,
		0,
		0,
		iWidth,
		iHeight);

	Construct();
}

D3D11DeviceHeadless::~D3D11DeviceHeadless()
{
	Destruct();
}

ID3D11Texture2D* D3D11DeviceHeadless::AcquireBackBuffer()
{
	m_pBackBuffer->AddRef();
	return m_pBackBuffer;
}

void D3D11DeviceHeadless::InitializeDirect3DDevice(
	ID3D11Device*& rpD3DDevice,
	ID3D11DeviceContext*& rpD3DDeviceContext)
{
	// Create the D3D11 Device.
#if SEOUL_DEBUG
	// Try a debug device first if a debug build.
	UINT const uFlags = D3D11_CREATE_DEVICE_DEBUG;
#else
	// Otherwise, no specific flags.
	UINT const uFlags = 0u;
#endif

	auto const eResult = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		uFlags,
		kaFeatureLevels,
		SEOUL_ARRAY_COUNT(kaFeatureLevels),
		D3D11_SDK_VERSION,
		&rpD3DDevice,
		nullptr,
		&rpD3DDeviceContext);

	// Create failed, fall back to a standard (not debug) device.
	if (eResult < 0)
	{
		// Try again - this create must succeed.
		SEOUL_D3D11_VERIFY(D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0u, // No flags for fallback creation.
			kaFeatureLevels,
			SEOUL_ARRAY_COUNT(kaFeatureLevels),
			D3D11_SDK_VERSION,
			&rpD3DDevice,
			nullptr,
			&rpD3DDeviceContext));
	}

	// Sanity check that all outputs were created.
	SEOUL_ASSERT(nullptr != rpD3DDevice);
	SEOUL_ASSERT(nullptr != rpD3DDeviceContext);

	// Create a back buffer.
	D3D11_TEXTURE2D_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.ArraySize = 1;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.Format = PixelFormatToD3D(PixelFormat::kA8B8G8R8);
	desc.Height = m_BackBufferViewport.m_iTargetHeight;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Width = m_BackBufferViewport.m_iTargetWidth;
	SEOUL_D3D11_VERIFY(rpD3DDevice->CreateTexture2D(
		&desc,
		nullptr,
		&m_pBackBuffer));
}

void D3D11DeviceHeadless::DeinitializeDirect3D()
{
	SafeRelease(m_pBackBuffer);
}

Bool D3D11DeviceHeadless::InternalPresent()
{
	SEOUL_ASSERT(IsRenderThread());

	if (m_bHasFrameToPresent)
	{
		InternalPrePresent();
		
		auto iTimeStamp = SeoulTime::GetGameTimeInTicks();

		// Simulate vsync.
		if (m_GraphicsParameters.m_iVsyncInterval > 0)
		{
			if (m_iPresentTimeStamp >= 0)
			{
				auto const fFrameTimeMs = SeoulTime::ConvertTicksToMilliseconds(iTimeStamp - m_iPresentTimeStamp);
				auto const fTargetMs = (1000.0 / GetDisplayRefreshRate().ToHz()) * (Double)m_GraphicsParameters.m_iVsyncInterval;
				auto const fRemainingMs = (fTargetMs - fFrameTimeMs);
				if (fRemainingMs > 5.0) // Arbitrary sleep constant on windows to attempt to avoid overshoot.
				{
					auto const uSleepMs = (UInt32)((Int32)Floor(fRemainingMs - 2.0));
					Thread::Sleep(uSleepMs);
				}

				iTimeStamp = SeoulTime::GetGameTimeInTicks();
				while (SeoulTime::ConvertTicksToMilliseconds(iTimeStamp - m_iPresentTimeStamp) < fTargetMs)
				{
					Thread::YieldToAnotherThread();
					iTimeStamp = SeoulTime::GetGameTimeInTicks();
				}
			}
		}

		m_iPresentTimeStamp = iTimeStamp;
		InternalPostPresent();
		m_bHasFrameToPresent = false;
	}

	return true;
}

D3DDeviceEntry GetD3D11DeviceHeadlessEntry()
{
	D3DDeviceEntry ret;
	ret.m_pCreateD3DDevice = D3D11DeviceHeadless::CreateDeviceD3D11;
	ret.m_pIsSupported = D3D11DeviceHeadless::IsSupportedD3D11;
	return ret;
}

} // namespace Seoul
