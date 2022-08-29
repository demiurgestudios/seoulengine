/**
 * \file D3D11DeviceWindow.h
 * \brief Specialization of D3D11Device that
 * uses a hardware window. Device used
 * for standard game engine rendering.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef D3D11_DEVICE_WINDOW_H
#define D3D11_DEVICE_WINDOW_H

#include "D3D11Device.h"

namespace Seoul
{

class D3D11DeviceWindow SEOUL_SEALED : public D3D11Device
{
public:
	static CheckedPtr<D3D11DeviceWindow> Get()
	{
		if (RenderDevice::Get() && RenderDevice::Get()->GetType() == RenderDeviceType::kD3D11Window)
		{
			return (D3D11DeviceWindow*)RenderDevice::Get().Get();
		}
		else
		{
			return CheckedPtr<D3D11DeviceWindow>();
		}
	}

	static D3DCommonDevice* CreateDeviceD3D11(const D3DCommonDeviceSettings& deviceSettings);
	static Bool IsSupportedD3D11(const D3DCommonDeviceSettings& deviceSettings);

	D3D11DeviceWindow(const D3DCommonDeviceSettings& settings);
	~D3D11DeviceWindow();

	virtual RenderDeviceType GetType() const SEOUL_OVERRIDE { return RenderDeviceType::kD3D11Window; }

	// Render mode controls.
	virtual Int GetActiveRenderModeIndex() const SEOUL_OVERRIDE;
	virtual Bool SetRenderModeByIndex(Int nRenderMode) SEOUL_OVERRIDE;
	void SetRenderMode(const DXGI_MODE_DESC& mode);

	// State checking.
	virtual Bool IsActive() const SEOUL_OVERRIDE { return m_bActive; }
	virtual Bool IsMaximized() const SEOUL_OVERRIDE { return m_bMaximized; }
	virtual Bool IsMinimized() const SEOUL_OVERRIDE { return m_bMinimized; }
	virtual Bool IsWindowed(void) const SEOUL_OVERRIDE;

	// Toggle the game between full screen and windowed mode, if supported
	// on the current platform.
	virtual void ToggleFullscreenMode() SEOUL_OVERRIDE;

	// On supported platform, toggle maximization of the main viewport window.
	virtual void ToggleMaximized() SEOUL_OVERRIDE;

	// On supported platform, toggle minimization of the main viewport window.
	virtual void ToggleMinimized() SEOUL_OVERRIDE;

#if !SEOUL_SHIP
	/**
	 * Valid only if IsVirtualizedDesktop() is true. Returns the main monitor
	 * relative coordinates of the virtualized desktop. This can be used
	 * to (e.g.) adjust internal game render coordinates after a virtualization
	 * toggle to avoid growing/shifting content.
	 */
	virtual Rectangle2DInt GetVirtualizedDesktopRect() const SEOUL_OVERRIDE
	{
		return m_VirtualizedDesktopRect;
	}

	/** @return true if active. */
	virtual Bool IsVirtualizedDesktop() const SEOUL_OVERRIDE
	{
		return m_GraphicsParameters.m_bVirtualizedDesktop;
	}

	/** Update the desired virtualized desktop mode. Applied on reset. */
	virtual void SetVirtualizedDesktop(Bool bVirtualized) SEOUL_OVERRIDE
	{
		if (bVirtualized != m_bWantsVirtualizedDesktop)
		{
			m_bWantsVirtualizedDesktop = bVirtualized;
			SetNeedsReset();
		}
	}

	/** We support virtualization desktop mode. */
	virtual Bool SupportsVirtualizedDesktop() const SEOUL_OVERRIDE
	{
		return true;
	}
#endif // /!SEOUL_SHIP

private:
	ScopedPtr<class D3D11VblankUtil> m_pVblankUtil;
	HWND m_hMainWindow;
#if !SEOUL_SHIP
	ScopedPtr<class D3DCommonThumbnailUtil> m_pThumbnailUtil;
#endif // /!SEOUL_SHIP
	DXGI_SWAP_CHAIN_DESC m_D3DSwapChainDescription;
	IDXGISwapChain* m_pD3DSwapChain;
	RefreshRate m_RefreshRate;
	typedef Vector<DXGI_MODE_DESC> AvailableDisplayModes;
	AvailableDisplayModes m_vAvailableDisplayModes;
	DXGI_MODE_DESC m_ActiveMode;
	DXGI_MODE_DESC m_DesktopMode;
	DXGI_MODE_DESC m_LastValidMode;
	typedef Vector<OsWindowRegion, MemoryBudgets::Rendering> OsWindowRegions;
	OsWindowRegions m_vOsWindowRegions;
#if !SEOUL_SHIP
	Rectangle2DInt m_VirtualizedDesktopRect;
	RECT m_VirtualizedDesktopMainFormWindowRect;
#endif // /!SEOUL_SHIP
	Atomic32Value<Int> m_nActiveRenderModeIndex;
	Bool m_bIgnoreActivateEvents;
	Bool m_bLeavingFullscreen;
	Bool m_bRefreshFullscreen;
	Bool m_bWantsFullscreen;
	Bool m_bMaximized;
	Bool m_bMinimized;
	Bool m_bPendingShowWindow;
	Bool m_bActive;
	Bool m_bHasFrameToPresent;
#if !SEOUL_SHIP
	Bool m_bWantsVirtualizedDesktop;
#endif // /!SEOUL_SHIP
	Bool m_bOsWindowRegionsDirty;

	void InternalInitializeWindow();
	RECT InternalApplyVirtualizedModeToGraphicsSettings();
	void InternalCreateWindow(
		const String& sAppNameAndVersion,
		HINSTANCE hInstance,
		Bool bStartFullscreen);
	void InternalDestroyWindow();

	void InternalUpdateCursorClip();
	void InternalToggleFullscreen();
	void InternalSetWindowPosInWindowedModeBasedOnClientViewport();
	Bool InternalFixupClientAndWindowInWindowedMode();
	void InternalApplyPendingRenderModeIndex();
	Int InternalGetActiveRenderModeIndex() const;

	// D3D11Device implementations.
	virtual ID3D11Texture2D* AcquireBackBuffer() SEOUL_OVERRIDE;
	virtual Viewport InternalCreateDefaultViewport() const SEOUL_OVERRIDE;
	virtual void InitializeDirect3DDevice(
		ID3D11Device*& rpD3DDevice,
		ID3D11DeviceContext*& rpD3DDeviceContext) SEOUL_OVERRIDE;
	virtual void DeinitializeDirect3D() SEOUL_OVERRIDE;
	virtual Bool InternalDoCanRender() const SEOUL_OVERRIDE;
	virtual Bool InternalDoResetDevice() SEOUL_OVERRIDE;
	virtual RefreshRate InternalGetRefreshRate() const SEOUL_OVERRIDE;
	virtual Bool InternalPresent() SEOUL_OVERRIDE;
	virtual void InternalBeginScenePreResetCheck() SEOUL_OVERRIDE;
	virtual void InternalBeginScenePostResetCheck() SEOUL_OVERRIDE;
	virtual void OnHasFrameToPresent() SEOUL_OVERRIDE
	{
		m_bHasFrameToPresent = true;
	}
	virtual void UpdateOsWindowRegions(OsWindowRegion const* pRegions, UInt32 uCount) SEOUL_OVERRIDE;
	// /D3D11Device implementations.

	void ApplyOsWindowRegions();

	// Begin PCEngine friend functions.
	virtual void PCEngineFriend_CaptureAndResizeClientViewport() SEOUL_OVERRIDE;
	virtual void PCEngineFriend_DestroyWindow() SEOUL_OVERRIDE;
	virtual void PCEngineFriend_SetActive(Bool bActive) SEOUL_OVERRIDE;
	virtual HWND PCEngineFriend_GetMainWindow() const SEOUL_OVERRIDE
	{
		return (HWND)m_hMainWindow;
	}
	virtual Bool PCEngineFriend_ShouldIgnoreActivateEvents() const SEOUL_OVERRIDE
	{
		return m_bIgnoreActivateEvents;
	}
	virtual Bool PCEngineFriend_IsLeavingFullscren() const SEOUL_OVERRIDE
	{
		return m_bLeavingFullscreen;
	}
	virtual void PCEngineFriend_Minimized(Bool bMinimized) SEOUL_OVERRIDE;
	virtual void PCEngineFriend_OnLivePreviewBitmap() SEOUL_OVERRIDE;
	virtual void PCEngineFriend_OnLiveThumbnail(UInt32 uWidth, UInt32 uHeight) SEOUL_OVERRIDE;
	// /End PCEngine friend functions.

	SEOUL_DISABLE_COPY(D3D11DeviceWindow);
}; // class D3D11DeviceWindow

D3DDeviceEntry GetD3D11DeviceWindowEntry();

} // namespace Seoul

#endif // include guard
