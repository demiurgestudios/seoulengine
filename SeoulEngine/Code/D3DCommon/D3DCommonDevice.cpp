/**
 * \file D3DCommonDevice.cpp
 * \brief Shared base class of D3D*Device specializations for the
 * various flavors of D3D.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "D3DCommonDevice.h"
#include "D3DCommonDeviceSettings.h"
#include "JobsFunction.h"
#include "ReflectionDefine.h"

namespace Seoul
{

static const DWORD kD3DWindowStyleOSFeatures = (WS_OVERLAPPEDWINDOW);
static const DWORD kD3DWindowStyleOSFeaturesEx = 0u;

#if !SEOUL_SHIP
// WS_MINIMIZEBOX is needed to tell the OS to trigger minimize when single clicking the app icon on the task bar.
static const DWORD kD3DWindowStyleNoOSFeatures = (WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_MINIMIZEBOX);
static const DWORD kD3DWindowStyleNoOSFeaturesEx = (WS_EX_WINDOWEDGE | WS_EX_APPWINDOW);
#endif // /!SEOUL_SHIP

SEOUL_BEGIN_TYPE(D3DCommonUserGraphicsSettings)
	SEOUL_ATTRIBUTE(DisableReflectionCheck)
	SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("WindowPositionX", m_iWindowX)
		SEOUL_ATTRIBUTE(Description,
			"user_setting_WindowPositionX_comment")
	SEOUL_PROPERTY_N("WindowPositionY", m_iWindowY)
		SEOUL_ATTRIBUTE(Description,
			"user_setting_WindowPositionY_comment")
	SEOUL_PROPERTY_N("WindowDimensionWidth", m_iWindowWidth)
		SEOUL_ATTRIBUTE(Description,
			"user_setting_WindowDimensionWidth_comment")
	SEOUL_PROPERTY_N("WindowDimensionHeight", m_iWindowHeight)
		SEOUL_ATTRIBUTE(Description,
			"user_setting_WindowDimensionHeight_comment")
	SEOUL_PROPERTY_N("FullscreenWidth", m_iFullscreenWidth)
		SEOUL_ATTRIBUTE(Description,
			"user_setting_FullscreenWidth_comment")
	SEOUL_PROPERTY_N("FullscreenHeight", m_iFullscreenHeight)
		SEOUL_ATTRIBUTE(Description,
			"user_setting_FullscreenHeight_comment")
	SEOUL_PROPERTY_N("FullscreenEnabled", m_bFullscreenEnabled)
		SEOUL_ATTRIBUTE(Description,
			"user_setting_FullscreenEnabled_comment")
	SEOUL_PROPERTY_N("VsyncEnabled", m_bVsyncEnabled)
		SEOUL_ATTRIBUTE(Description,
			"user_setting_VsyncEnabled_comment")
	SEOUL_PROPERTY_N("WindowedFullscreenEnabled", m_bWindowedFullscreenEnabled)
		SEOUL_ATTRIBUTE(Description,
			"user_setting_WindowedFullscreenEnabled_comment")
SEOUL_END_TYPE()

/**
 * Called by PCEngine immediately after deserializing the user's current
 * user settings. Gives D3DCommonDevice a chance to modify settings to be valid
 * or to pick first-run values.
 */
void D3DCommonDevice::CheckAndConfigureSettings(D3DCommonUserGraphicsSettings& rSettings)
{
	RECT workArea;
	memset(&workArea, 0, sizeof(workArea));
	Bool const bHasWorkArea = (FALSE != ::SystemParametersInfoW(
		SPI_GETWORKAREA,
		0,
		&workArea,
		0) &&
		(workArea.right - workArea.left) >= kMinimumResolutionWidth &&
		(workArea.bottom - workArea.top) >= kMinimumResolutionHeight);

	// If we have a work area, make sure the window is at least partially
	// within it, to account for events like a dual monitor setup
	// being switched to a single monitor setup.
	if (bHasWorkArea)
	{
		// If the right edge is off the right edge of the work area, nudge it back.
		if (rSettings.m_iWindowX + rSettings.m_iWindowWidth > workArea.right)
		{
			rSettings.m_iWindowX += (workArea.right - (rSettings.m_iWindowX + rSettings.m_iWindowWidth));
		}

		// If the bottom edge is off the bottom edge of the work area, nudge it back.
		if (rSettings.m_iWindowY + rSettings.m_iWindowHeight > workArea.bottom)
		{
			rSettings.m_iWindowY += (workArea.bottom - (rSettings.m_iWindowY + rSettings.m_iWindowHeight));
		}

		// If the left edge is off the left edge of the work area, nudge it back.
		if (rSettings.m_iWindowX < workArea.left)
		{
			rSettings.m_iWindowX += (workArea.left - rSettings.m_iWindowX);
		}

		// If the top edge is off the top edge of the work area, nudge it back.
		if (rSettings.m_iWindowY < workArea.top)
		{
			rSettings.m_iWindowY += (workArea.top - rSettings.m_iWindowY);
		}
	}

	// If the existing window width or height has not been set,
	// pick a default window configuration.
	if (rSettings.m_iWindowWidth <= 0 ||
		rSettings.m_iWindowHeight <= 0)
	{
		// Try to use the work area - if this false for some reason, or gives us
		// ugly data, just the SM_C*FULLSCREEN constants.
		if (bHasWorkArea)
		{
			rSettings.m_iWindowX = workArea.left;
			rSettings.m_iWindowY = workArea.top;
			rSettings.m_iWindowWidth = (workArea.right - workArea.left);
			rSettings.m_iWindowHeight = (workArea.bottom - workArea.top);
		}
		else
		{
			rSettings.m_iWindowX = 0;
			rSettings.m_iWindowY = 0;
			rSettings.m_iWindowWidth = ::GetSystemMetrics(SM_CXFULLSCREEN);
			rSettings.m_iWindowHeight = ::GetSystemMetrics(SM_CYFULLSCREEN);
		}
	}
	// Otherwise, evaluate the window's configuration.
	else
	{
		RECT rectangle =
		{
			0,
			0,
			kMinimumResolutionWidth,
			kMinimumResolutionHeight
		};
		AdjustWindowRect(
			&rectangle,
			kD3DWindowStyleOSFeatures,
			false);

		Int const iMinimumWindowWidth = (Int)(rectangle.right - rectangle.left);
		Int const iMinimumWindowHeight = (Int)(rectangle.bottom - rectangle.top);

		// If the defined window width or height is too small, clamp it.
		if (rSettings.m_iWindowWidth < iMinimumWindowWidth)
		{
			rSettings.m_iWindowWidth += (iMinimumWindowWidth - rSettings.m_iWindowWidth);
		}

		if (rSettings.m_iWindowHeight < iMinimumWindowHeight)
		{
			rSettings.m_iWindowHeight += (iMinimumWindowHeight - rSettings.m_iWindowHeight);
		}
	}
}

D3DCommonDevice::D3DCommonDevice(const D3DCommonDeviceSettings& settings)
	: m_bInScene(false)
{
	InternalUpdateGraphicsParametersFromUserSettings(settings);
	InternalSanitizeGraphicsSettings();
}

D3DCommonDevice::~D3DCommonDevice()
{
}

Bool D3DCommonDevice::GetMaximumWorkAreaForRectangle(const Rectangle2DInt& input, Rectangle2DInt& output) const
{
	// Adjustments applied if virtualized - we assume input and output values
	// are in the virtualized space.
	Int iOffsetX = 0;
	Int iOffsetY = 0;
#if !SEOUL_SHIP
	if (IsVirtualizedDesktop())
	{
		auto const vrect = GetVirtualizedDesktopRect();
		iOffsetX = vrect.m_iLeft;
		iOffsetY = vrect.m_iTop;
	}
#endif // /!SEOUL_SHIP

	// Try matching the rectangle.
	HMONITOR hmonitor = nullptr;
	{
		RECT rect;
		rect.left = input.m_iLeft + iOffsetX;
		rect.top = input.m_iTop + iOffsetY;
		rect.right = input.m_iRight + iOffsetX;
		rect.bottom = input.m_iBottom + iOffsetY;
		hmonitor = ::MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
	}

	// If we failed resolving the monitor, use the primary.
	RECT ret;
	if (nullptr == hmonitor)
	{
		memset(&ret, 0, sizeof(ret));
		if (FALSE == ::SystemParametersInfoW(SPI_GETWORKAREA, 0, &ret, 0))
		{
			return false;
		}
	}
	// Otherwise, lookup information on the specific monitor.
	else
	{
		MONITORINFO monitorInfo;
		memset(&monitorInfo, 0, sizeof(monitorInfo));
		monitorInfo.cbSize = sizeof(monitorInfo);
		if (FALSE == ::GetMonitorInfoW(hmonitor, &monitorInfo))
		{
			return false;
		}

		ret = monitorInfo.rcWork;
	}

	// Done - apply adjustment and output on success.
	output.m_iLeft = ret.left - iOffsetX;
	output.m_iTop = ret.top - iOffsetY;
	output.m_iRight = ret.right - iOffsetX;
	output.m_iBottom = ret.bottom - iOffsetY;
	return true;
}

Bool D3DCommonDevice::GetMaximumWorkAreaOnPrimary(Rectangle2DInt& output) const
{
	// Adjustments applied if virtualized - we assume input and output values
	// are in the virtualized space.
	Int iOffsetX = 0;
	Int iOffsetY = 0;
#if !SEOUL_SHIP
	if (IsVirtualizedDesktop())
	{
		auto const vrect = GetVirtualizedDesktopRect();
		iOffsetX = vrect.m_iLeft;
		iOffsetY = vrect.m_iTop;
	}
#endif // /!SEOUL_SHIP

	// Get primary work area.
	RECT ret;
	memset(&ret, 0, sizeof(ret));
	if (FALSE == ::SystemParametersInfoW(SPI_GETWORKAREA, 0, &ret, 0))
	{
		return false;
	}

	// Done - apply adjustment and output on success.
	output.m_iLeft = ret.left - iOffsetX;
	output.m_iTop = ret.top - iOffsetY;
	output.m_iRight = ret.right - iOffsetX;
	output.m_iBottom = ret.bottom - iOffsetY;
	return true;
}

/** If supported, bring the hardware window into the foreground of other windows. */
Bool D3DCommonDevice::ForegroundOsWindow()
{
	auto hWnd = PCEngineFriend_GetMainWindow();
	if (hWnd != nullptr && INVALID_HANDLE_VALUE != hWnd)
	{
		return (FALSE != ::SetForegroundWindow(hWnd));
	}
	else
	{
		return false;
	}
}

Bool D3DCommonDevice::GetOsWindowRegion(Point2DInt& rPos, Point2DInt& rSize) const
{
	RECT rect;
	memset(&rect, 0, sizeof(rect));
	SEOUL_VERIFY(FALSE != ::GetWindowRect(PCEngineFriend_GetMainWindow(), &rect));

	rPos.X = rect.left;
	rPos.Y = rect.top;
	rSize.X = (rect.right - rect.left);
	rSize.Y = (rect.bottom - rect.top);
	return true;
}

static void RenderThreadSetOsWindowRegion(
	HWND hWnd,
	const Point2DInt& pos,
	const Point2DInt& size)
{
	// Set the size of the window for windowed mode. Only
	// do this on changes, since calling this redundantly
	// can effectively break maximize/restore behavior.
	RECT actualRectangle;
	memset(&actualRectangle, 0, sizeof(actualRectangle));
	SEOUL_VERIFY(FALSE != ::GetWindowRect(hWnd, &actualRectangle));

	if (actualRectangle.bottom != pos.Y + size.Y ||
		actualRectangle.left != pos.X ||
		actualRectangle.right != pos.X + size.X ||
		actualRectangle.top != pos.Y)
	{
		// Make sure the window is normal on movement.
		WINDOWPLACEMENT placement;
		memset(&placement, 0, sizeof(placement));
		SEOUL_VERIFY(FALSE != ::GetWindowPlacement(hWnd, &placement));

		if (placement.showCmd == SW_MAXIMIZE)
		{
			placement.flags = 0;
			placement.showCmd = SW_RESTORE;
			SEOUL_VERIFY(FALSE != ::SetWindowPlacement(hWnd, &placement));
		}

		SEOUL_VERIFY(FALSE != ::SetWindowPos(
			hWnd,
			nullptr,
			pos.X,
			pos.Y,
			size.X,
			size.Y,
			SWP_NOZORDER | SWP_NOACTIVATE));
	}
}

void D3DCommonDevice::SetOsWindowRegion(const Point2DInt& pos, const Point2DInt& size)
{
	// Basic sanitizing of inputs.
	if (size.X < 1 || size.Y < 1)
	{
		return;
	}

	Jobs::AsyncFunction(
		GetRenderThreadId(),
		&RenderThreadSetOsWindowRegion,
		PCEngineFriend_GetMainWindow(),
		pos,
		size);
}

DWORD D3DCommonDevice::GetD3DWindowedModeWindowStyle() const
{
#if !SEOUL_SHIP
	return (m_GraphicsParameters.m_bVirtualizedDesktop ? kD3DWindowStyleNoOSFeatures : kD3DWindowStyleOSFeatures);
#else
	return kD3DWindowStyleOSFeatures;
#endif
}

DWORD D3DCommonDevice::GetD3DWindowedModeWindowStyleEx() const
{
#if !SEOUL_SHIP
	return (m_GraphicsParameters.m_bVirtualizedDesktop ? kD3DWindowStyleNoOSFeaturesEx : kD3DWindowStyleOSFeaturesEx);
#else
	return kD3DWindowStyleOSFeaturesEx;
#endif
}

void D3DCommonDevice::InternalUpdateGraphicsParametersFromUserSettings(const D3DCommonDeviceSettings& deviceSettings)
{
	// Initialize m_GraphicsParameters from user settings.
	auto const& settings = deviceSettings.m_UserSettings;
	m_GraphicsParameters.m_bStartFullscreen = settings.m_bFullscreenEnabled;
	m_GraphicsParameters.m_iVsyncInterval = (settings.m_bVsyncEnabled ? 1 : 0);
	m_GraphicsParameters.m_iFullscreenHeight = settings.m_iFullscreenHeight;
	m_GraphicsParameters.m_iFullscreenWidth = settings.m_iFullscreenWidth;
	m_GraphicsParameters.m_bWindowedFullscreen = settings.m_bWindowedFullscreenEnabled;

	{
		// Construct a rectangle containing the current window size.
		RECT rectangle;
		rectangle.left = settings.m_iWindowX;
		rectangle.top = settings.m_iWindowY;
		rectangle.right = (settings.m_iWindowX + settings.m_iWindowWidth);
		rectangle.bottom = (settings.m_iWindowY + settings.m_iWindowHeight);

		// Cache the original rectangle and adjust it using AdjustWindoRect - this converts
		// a client window to a window rectangle, so we apply this function and then subtract
		// the difference from the original rectangle to convert a window rectangle to
		// a client rectangle.
		RECT originalRectangle = rectangle;
		AdjustWindowRect(&rectangle, kD3DWindowStyleOSFeatures, false);
		originalRectangle.left -= (rectangle.left - originalRectangle.left);
		originalRectangle.top -= (rectangle.top - originalRectangle.top);
		originalRectangle.right -= (rectangle.right - originalRectangle.right);
		originalRectangle.bottom -= (rectangle.bottom - originalRectangle.bottom);

		// Set the adjust rectangle to graphics parameters.
		m_GraphicsParameters.m_iWindowViewportX = originalRectangle.left;
		m_GraphicsParameters.m_iWindowViewportY = originalRectangle.top;
		m_GraphicsParameters.m_iWindowViewportWidth = (originalRectangle.right - originalRectangle.left);
		m_GraphicsParameters.m_iWindowViewportHeight = (originalRectangle.bottom - originalRectangle.top);
	}
}

void D3DCommonDevice::InternalSanitizeGraphicsSettings()
{
	// Early out if explicitly specified viewport.
	if (m_GraphicsParameters.m_iWindowViewportWidth > 0 &&
		m_GraphicsParameters.m_iWindowViewportHeight > 0)
	{
		return;
	}

	RECT workArea;
	memset(&workArea, 0, sizeof(workArea));
	SEOUL_VERIFY(FALSE != ::SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0));

	// Apply AdjustWindowRect to a zero sized rectangle to
	// compute the adjustment to convert a window rectangle to a client rectangle.
	RECT empty;
	memset(&empty, 0, sizeof(empty));
	SEOUL_VERIFY(FALSE != ::AdjustWindowRectEx(&empty, GetD3DWindowedModeWindowStyle(), FALSE, GetD3DWindowedModeWindowStyleEx()));

	// Compute client rectangle.
	workArea.left -= empty.left;
	workArea.top -= empty.top;
	workArea.right -= empty.right;
	workArea.bottom -= empty.bottom;

	// Commit.
	m_GraphicsParameters.m_iWindowViewportX = workArea.left;
	m_GraphicsParameters.m_iWindowViewportY = workArea.top;
	m_GraphicsParameters.m_iWindowViewportWidth = (workArea.right - workArea.left);
	m_GraphicsParameters.m_iWindowViewportHeight = (workArea.bottom - workArea.top);
}

D3DCommonDevice* D3DCommonDevice::CreateD3DDevice(const D3DCommonDeviceSettings& deviceSettings)
{
	for (auto i = deviceSettings.m_vEntries.Begin(); deviceSettings.m_vEntries.End() != i; ++i)
	{
		if (i->m_pIsSupported(deviceSettings))
		{
			return i->m_pCreateD3DDevice(deviceSettings);
		}
	}

	// If we get here, fallback to the last backend.
	if (!deviceSettings.m_vEntries.IsEmpty())
	{
		return deviceSettings.m_vEntries.Back().m_pCreateD3DDevice(deviceSettings);
	}

	return nullptr;
}

/**
 * When called, overwrites any graphics parameters that are defined
 * in the PCEngineUserGraphicsSettings structure with their corresponding value
 * in the GraphicsParameters structure.
 */
void D3DCommonDevice::MergeUserGraphicsSettings(D3DCommonUserGraphicsSettings& settings)
{
	// Update user settings from current graphics parameters.
	settings.m_bWindowedFullscreenEnabled = m_GraphicsParameters.m_bWindowedFullscreen;
	settings.m_bFullscreenEnabled = !IsWindowed();
	settings.m_bVsyncEnabled = (m_GraphicsParameters.m_iVsyncInterval != 0);
	settings.m_iFullscreenHeight = m_GraphicsParameters.m_iFullscreenHeight;
	settings.m_iFullscreenWidth = m_GraphicsParameters.m_iFullscreenWidth;

	// Convert the current client viewport to a window viewport and set it to user settings.
	RECT rectangle;
	rectangle.left = m_GraphicsParameters.m_iWindowViewportX;
	rectangle.top = m_GraphicsParameters.m_iWindowViewportY;
	rectangle.right = (m_GraphicsParameters.m_iWindowViewportX + m_GraphicsParameters.m_iWindowViewportWidth);
	rectangle.bottom = (m_GraphicsParameters.m_iWindowViewportY + m_GraphicsParameters.m_iWindowViewportHeight);
	AdjustWindowRect(&rectangle, GetD3DWindowedModeWindowStyle(), false);
	settings.m_iWindowX = rectangle.left;
	settings.m_iWindowY = rectangle.top;
	settings.m_iWindowWidth = (rectangle.right - rectangle.left);
	settings.m_iWindowHeight = (rectangle.bottom - rectangle.top);
}

} // namespace Seoul
