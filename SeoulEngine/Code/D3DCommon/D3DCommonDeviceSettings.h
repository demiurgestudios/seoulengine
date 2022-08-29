/**
 * \file D3DCommonDeviceSettings.h
 * \brief Configuration settings for a D3D9Device or D3D11Device.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef D3D_COMMON_DEVICE_SETTINGS_H
#define D3D_COMMON_DEVICE_SETTINGS_H

#include "FixedArray.h"
#include "MouseCursor.h"
#include "Platform.h"
#include "Prereqs.h"
#include "SeoulString.h"
#include "Vector.h"
namespace Seoul { class D3DCommonDevice; }
namespace Seoul { struct D3DCommonDeviceSettings; }

// Forward declarations
struct IUnknown;

namespace Seoul
{

/**
 * "vtable" for device creation and display queries prior to device creation.
 */
struct D3DDeviceEntry SEOUL_SEALED
{
	D3DDeviceEntry()
		: m_pCreateD3DDevice(nullptr)
		, m_pIsSupported(nullptr)
	{
	}

	D3DCommonDevice* (*m_pCreateD3DDevice)(const D3DCommonDeviceSettings& deviceSettings);
	Bool (*m_pIsSupported)(const D3DCommonDeviceSettings& deviceSettings);
}; // struct D3DDeviceEntry

/**
 * User settings specific to graphics/video (resolution, vsync, etc.).
 */
struct D3DCommonUserGraphicsSettings
{
	D3DCommonUserGraphicsSettings()
		: m_iWindowX(-1)
		, m_iWindowY(-1)
		, m_iWindowWidth(-1)
		, m_iWindowHeight(-1)
		, m_iFullscreenWidth(-1)
		, m_iFullscreenHeight(-1)
		, m_bFullscreenEnabled(false)
		, m_bVsyncEnabled(true)
		, m_bWindowedFullscreenEnabled(false)
	{
	}

	Int m_iWindowX;
	Int m_iWindowY;
	Int m_iWindowWidth;
	Int m_iWindowHeight;
	Int m_iFullscreenWidth;
	Int m_iFullscreenHeight;
	Bool m_bFullscreenEnabled;
	Bool m_bVsyncEnabled;
	Bool m_bWindowedFullscreenEnabled;
};

/**
 * Settings used to configure a D3D9Device or D3D11Device.
 */
struct D3DCommonDeviceSettings SEOUL_SEALED
{
	typedef Vector<D3DDeviceEntry, MemoryBudgets::Rendering> Entries;

	D3DCommonDeviceSettings()
		: m_vEntries()
		, m_hInstance(nullptr)
		, m_pWndProc(nullptr)
		, m_iApplicationIcon(-1)
		, m_aMouseCursors()
		, m_sLocalizedAppNameAndVersion()
		, m_uMinimumPixelShaderVersion(3u)
		, m_uMinimumVertexShaderVersion(3u)
		, m_sPreferredBackend()
		, m_iPreferredViewportWidth(-1)
		, m_iPreferredViewportHeight(-1)
	{
	}

	/** Ordered array of devices to attempt to create. First successful creation wins. */
	Entries m_vEntries;

	/** Module identifier of the current application. */
	HINSTANCE m_hInstance;

	/** Allows for the specification of a custom app message procedure - typically, set to nullptr to use the default PCEngine procedure. */
	WNDPROC m_pWndProc;

	/** HRESOURCE handle to the application icon. */
	Int m_iApplicationIcon;

	/** Custom mouse cursor handle to use for the application. */
	FixedArray<HCURSOR, (UInt32)MouseCursor::COUNT> m_aMouseCursors;

	/** App name and version - generated automatically if empty in PCEngine constructor */
	String m_sLocalizedAppNameAndVersion;

	/** Minimum capability for D3D*Device - minimum major pixel shader version required. */
	UInt32 m_uMinimumPixelShaderVersion;

	/** Minimum capability for D3D*Device - minimum major vertex shader version required. */
	UInt32 m_uMinimumVertexShaderVersion;

	/** Preferred backend name - may be left blank, in which case the first valid backend is chosen. */
	String m_sPreferredBackend;

	/** (Optional) Relevant to headless backend, preferred width of the back buffer. */
	Int32 m_iPreferredViewportWidth;

	/** (Optional) Relevant to headless backend, preferred height of the back buffer. */
	Int32 m_iPreferredViewportHeight;

	/** Saved user settings to initialize the backend. */
	D3DCommonUserGraphicsSettings m_UserSettings;
}; // struct D3DCommonDeviceSettings

} // namespace Seoul

#endif // include guard
