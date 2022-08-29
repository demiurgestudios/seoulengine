/**
 * \file D3DCommonDevice.h
 * \brief Shared base class of D3D*Device specializations for the
 * various flavors of D3D.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef D3D_COMMON_DEVICE_H
#define D3D_COMMON_DEVICE_H

#include "Platform.h"
#include "RenderDevice.h"

namespace Seoul
{

// Forward declarations
struct D3DCommonDeviceSettings;
struct D3DCommonUserGraphicsSettings;

class D3DCommonDevice SEOUL_ABSTRACT : public RenderDevice
{
public:
	static void CheckAndConfigureSettings(D3DCommonUserGraphicsSettings& rSettings);

	static CheckedPtr<D3DCommonDevice> Get()
	{
		if (RenderDevice::Get() &&
			(RenderDevice::Get()->GetType() == RenderDeviceType::kD3D9 ||
			RenderDevice::Get()->GetType() == RenderDeviceType::kD3D11Headless ||
			RenderDevice::Get()->GetType() == RenderDeviceType::kD3D11Window))
		{
			return (D3DCommonDevice*)RenderDevice::Get().Get();
		}
		else
		{
			return CheckedPtr<D3DCommonDevice>();
		}
	}

	// Shared entry points. Implementations must be provided by function pointers
	// in deviceSettings.
	static D3DCommonDevice* CreateD3DDevice(
		const D3DCommonDeviceSettings& deviceSettings);

	D3DCommonDevice(const D3DCommonDeviceSettings& settings);
	virtual ~D3DCommonDevice();

	virtual Bool GetMaximumWorkAreaForRectangle(const Rectangle2DInt& input, Rectangle2DInt& output) const SEOUL_OVERRIDE;
	virtual Bool GetMaximumWorkAreaOnPrimary(Rectangle2DInt& output) const SEOUL_OVERRIDE;

	void MergeUserGraphicsSettings(D3DCommonUserGraphicsSettings& settings);

	/** If supported, bring the hardware window into the foreground of other windows. */
	virtual Bool ForegroundOsWindow() SEOUL_OVERRIDE;

	virtual Bool GetOsWindowRegion(Point2DInt& rPos, Point2DInt& rSize) const SEOUL_OVERRIDE;
	virtual void SetOsWindowRegion(const Point2DInt& pos, const Point2DInt& size) SEOUL_OVERRIDE;

	// Not supported on all platforms. When supported, returns the human readable
	// title string of the application's main window.
	virtual const String& GetOsWindowTitle() const SEOUL_OVERRIDE
	{
		return m_sOsWindowTitle;
	}

	// Whether we're in between the BeginScene()/EndScene() calls.
	Bool IsInScene() const { return m_bInScene; }

protected:
	friend class PCEngine;

	Atomic32Value<Bool> m_bInScene;
	String m_sOsWindowTitle;

	DWORD GetD3DWindowedModeWindowStyle() const;
	DWORD GetD3DWindowedModeWindowStyleEx() const;

	virtual void PCEngineFriend_CaptureAndResizeClientViewport() = 0;
	virtual void PCEngineFriend_DestroyWindow() = 0;
	virtual const GraphicsParameters& PCEngineFriend_GetGraphicsParameters() const = 0;
	virtual HWND PCEngineFriend_GetMainWindow() const = 0;
	virtual const D3DCommonDeviceSettings& PCEngineFriend_GetSettings() const = 0;
	virtual void PCEngineFriend_SetActive(Bool bActive) = 0;
	virtual Bool PCEngineFriend_ShouldIgnoreActivateEvents() const = 0;
	virtual Bool PCEngineFriend_IsLeavingFullscren() const = 0;
	virtual void PCEngineFriend_Minimized(Bool bMinimized) = 0;
	virtual void PCEngineFriend_OnLivePreviewBitmap() = 0;
	virtual void PCEngineFriend_OnLiveThumbnail(UInt32 uWidth, UInt32 uHeight) = 0;

	void InternalSanitizeGraphicsSettings();

private:
	void InternalUpdateGraphicsParametersFromUserSettings(const D3DCommonDeviceSettings& deviceSettings);

	SEOUL_DISABLE_COPY(D3DCommonDevice);
}; // class D3DCommonDevice

} // namespace Seoul

#endif // include guard
