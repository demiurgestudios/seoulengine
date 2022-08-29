/**
 * \file D3D11DeviceHeadless.h
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

#pragma once
#ifndef D3D11_DEVICE_HEADLESS_H
#define D3D11_DEVICE_HEADLESS_H

#include "D3D11Device.h"

namespace Seoul
{

class D3D11DeviceHeadless SEOUL_SEALED : public D3D11Device
{
public:
	static CheckedPtr<D3D11DeviceHeadless> Get()
	{
		if (RenderDevice::Get() && RenderDevice::Get()->GetType() == RenderDeviceType::kD3D11Headless)
		{
			return (D3D11DeviceHeadless*)RenderDevice::Get().Get();
		}
		else
		{
			return CheckedPtr<D3D11DeviceHeadless>();
		}
	}

	static D3DCommonDevice* CreateDeviceD3D11(const D3DCommonDeviceSettings& deviceSettings);
	static Bool IsSupportedD3D11(const D3DCommonDeviceSettings& deviceSettings);

	D3D11DeviceHeadless(const D3DCommonDeviceSettings& settings);
	~D3D11DeviceHeadless();

	virtual RenderDeviceType GetType() const SEOUL_OVERRIDE { return RenderDeviceType::kD3D11Headless; }

private:
	Viewport m_BackBufferViewport;
	ID3D11Texture2D* m_pBackBuffer;
	Int64 m_iPresentTimeStamp;
	Bool m_bHasFrameToPresent;

	// D3D11Device implementations.
	virtual ID3D11Texture2D* AcquireBackBuffer() SEOUL_OVERRIDE;
	virtual Viewport InternalCreateDefaultViewport() const SEOUL_OVERRIDE
	{
		return m_BackBufferViewport;
	}
	virtual void InitializeDirect3DDevice(
		ID3D11Device*& rpD3DDevice,
		ID3D11DeviceContext*& rpD3DDeviceContext) SEOUL_OVERRIDE;
	virtual void DeinitializeDirect3D() SEOUL_OVERRIDE;
	virtual Bool InternalDoCanRender() const SEOUL_OVERRIDE
	{
		return true;
	}
	virtual Bool InternalDoResetDevice() SEOUL_OVERRIDE
	{
		return true;
	}
	virtual RefreshRate InternalGetRefreshRate() const SEOUL_OVERRIDE
	{
		return RefreshRate();
	}
	virtual Bool InternalPresent() SEOUL_OVERRIDE;
	virtual void InternalBeginScenePreResetCheck() SEOUL_OVERRIDE
	{
		// Nop
	}
	virtual void InternalBeginScenePostResetCheck() SEOUL_OVERRIDE
	{
		// Nop
	}
	virtual void OnHasFrameToPresent() SEOUL_OVERRIDE
	{
		m_bHasFrameToPresent = true;
	}
	// /D3D11Device implementations.

	// Begin PCEngine friend functions.
	virtual void PCEngineFriend_CaptureAndResizeClientViewport() SEOUL_OVERRIDE
	{
		// Nop
	}
	virtual void PCEngineFriend_DestroyWindow() SEOUL_OVERRIDE
	{
		// Nop
	}
	virtual void PCEngineFriend_SetActive(Bool bActive) SEOUL_OVERRIDE
	{
		// Nop
	}
	virtual HWND PCEngineFriend_GetMainWindow() const SEOUL_OVERRIDE
	{
		return nullptr;
	}
	virtual Bool PCEngineFriend_ShouldIgnoreActivateEvents() const SEOUL_OVERRIDE
	{
		return false;
	}
	virtual Bool PCEngineFriend_IsLeavingFullscren() const SEOUL_OVERRIDE
	{
		return false;
	}
	virtual void PCEngineFriend_Minimized(Bool bMinimized) SEOUL_OVERRIDE
	{
		// Nop
	}
	virtual void PCEngineFriend_OnLivePreviewBitmap() SEOUL_OVERRIDE
	{
		// Nop
	}
	virtual void PCEngineFriend_OnLiveThumbnail(UInt32 uWidth, UInt32 uHeight) SEOUL_OVERRIDE
	{
		// Nop
	}
	// /End PCEngine friend functions.

	SEOUL_DISABLE_COPY(D3D11DeviceHeadless);
}; // class D3D11DeviceHeadless

D3DDeviceEntry GetD3D11DeviceHeadlessEntry();

} // namespace Seoul

#endif // include guard
