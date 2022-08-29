/**
 * \file D3D11DeviceWindow.cpp
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

#include "D3D11DeviceWindow.h"
#include "D3DCommonThumbnailUtil.h"
#include "JobsFunction.h"
#include "LocManager.h"
#include "RenderCommandStreamBuilder.h"
#include "ThreadId.h"

extern "C" { typedef HRESULT (__stdcall *DwmFlushPtr)(); }

namespace Seoul
{

class D3D11VblankUtil SEOUL_SEALED
{
public:
	D3D11VblankUtil()
		: m_hDwmapi(::LoadLibraryW(L"Dwmapi.dll"))
		, m_pDwmFlush(GetDwmFlush(m_hDwmapi))
	{
	}

	~D3D11VblankUtil()
	{
		m_pDwmFlush = nullptr;
		if (nullptr != m_hDwmapi)
		{
			SEOUL_VERIFY(FALSE != ::FreeLibrary(m_hDwmapi));
			m_hDwmapi = nullptr;
		}
	}

	void WaitForVblank()
	{
		if (nullptr != m_pDwmFlush)
		{
			m_pDwmFlush();
		}
	}

private:
	SEOUL_DISABLE_COPY(D3D11VblankUtil);

	HMODULE m_hDwmapi;
	DwmFlushPtr m_pDwmFlush;

	static inline DwmFlushPtr GetDwmFlush(HMODULE dwmapi)
	{
		return (DwmFlushPtr)::GetProcAddress(dwmapi, "DwmFlush");
	}
};

/** Index used for the special "Windowed" render mode. */
static const Int32 kRenderModeWindowedIndex = 0;

/** Index used for the special "Windowed (Fullscreen)" render mode. */
static const Int32 kRenderModeWindowedFullscreenIndex = 1;

/** Number of special render modes. */
static const Int32 kSpecialRenderModeCount = 2;

static inline Float32 GetRefreshRate(const DXGI_MODE_DESC& mode)
{
	return ((Float)mode.RefreshRate.Numerator / (Float)mode.RefreshRate.Denominator);
}

struct D3D11SortByWidthThenHeightThenRefresh
{
	D3D11SortByWidthThenHeightThenRefresh(const DXGI_MODE_DESC& desktopMode)
		: m_DesktopMode(desktopMode)
		, m_fDesktopRefreshRate(GetRefreshRate(desktopMode))
	{
	}

	Bool operator()(const DXGI_MODE_DESC& a, const DXGI_MODE_DESC& b) const
	{
		if (a.Width != b.Width)
		{
			return (a.Width < b.Width);
		}

		if (a.Height != b.Height)
		{
			return (a.Height < b.Height);
		}

		// Prefer centered over stretching.
		if (a.Scaling != b.Scaling)
		{
			return (a.Scaling == DXGI_MODE_SCALING_CENTERED);
		}

		// Prefer pregressive over other modes.
		if (a.ScanlineOrdering != b.ScanlineOrdering)
		{
			return (a.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE);
		}

		// Sort by the mode closest to the desktop's refresh.
		return Abs(GetRefreshRate(a) - m_fDesktopRefreshRate) < Abs(GetRefreshRate(b) - m_fDesktopRefreshRate);
	}

	DXGI_MODE_DESC const m_DesktopMode;
	Float32 const m_fDesktopRefreshRate;
};

/** Consider two modes equal if they have the same width and height */
inline Bool HaveSameWidthAndHeight(const DXGI_MODE_DESC& a, const DXGI_MODE_DESC& b)
{
	return (a.Width == b.Width && a.Height == b.Height);
}

/** Consider two modes equal if they are equal in all fields. */
inline Bool operator==(const DXGI_MODE_DESC& a, const DXGI_MODE_DESC& b)
{
	return (
		a.Format == b.Format &&
		a.RefreshRate.Denominator == b.RefreshRate.Denominator &&
		a.RefreshRate.Numerator == b.RefreshRate.Numerator &&
		a.Scaling == b.Scaling &&
		a.ScanlineOrdering == b.ScanlineOrdering &&
		a.Width == b.Width &&
		a.Height == b.Height);
}

inline Bool operator!=(const DXGI_MODE_DESC& a, const DXGI_MODE_DESC& b)
{
	return !(a == b);
}

/**
 * Prune modes that we do not want to support, as well as modes at refresh rates we don't need.
 *
 * \pre List must have been sorted with SortByWidthThenHeightThenRefresh prior to calling this function.
 */
static void InternalStaticFilterModes(Vector<DXGI_MODE_DESC>& rv)
{
	// We need to maintain the sort of rv, so don't try to use
	// the "swap trick" here to avoid the memory moves.
	DXGI_MODE_DESC prevMode;
	memset(&prevMode, 0, sizeof(prevMode));

	for (auto i = rv.Begin(); rv.End() != i; )
	{
		auto const mode = *i;

		// Only modes that hit our minimum resolution height are allowed.
		if (mode.Height < kMinimumResolutionHeight)
		{
			i = rv.Erase(i);
			prevMode = mode;
			continue;
		}

		// Only the first mode of a particular width + height is allowed.
		// All further modes are filtered (we only want the mode with
		// refresh rate closest to the desktop).
		if (prevMode.Height == mode.Height &&
			prevMode.Width == mode.Width)
		{
			i = rv.Erase(i);
			prevMode = mode;
			continue;
		}

		// If we get here, just advance to next.
		++i;
		prevMode = mode;
	}
}

/**
 * Populates the output vector with the list of valid
 * display modes on the primary D3D11Device display.
 */
static Bool InternalStaticGetDisplayModes(const DXGI_MODE_DESC& desktopMode, Vector<DXGI_MODE_DESC>& rv)
{
	rv.Clear();

	IDXGIFactory* pFactory = nullptr;
	if (FAILED(CreateDXGIFactory(
		__uuidof(IDXGIFactory),
		(void**)&pFactory)))
	{
		return false;
	}

	IDXGIAdapter* pAdapter = nullptr;
	for (UInt32 i = 0u; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters(i, &pAdapter); ++i)
	{
		DXGI_ADAPTER_DESC desc;
		memset(&desc, 0, sizeof(desc));
		if (FAILED(pAdapter->GetDesc(&desc)))
		{
			SafeRelease(pAdapter);
			continue;
		}

		IDXGIOutput* pOutput = nullptr;
		for (UInt32 j = 0u; DXGI_ERROR_NOT_FOUND != pAdapter->EnumOutputs(j, &pOutput); ++j)
		{
			DXGI_OUTPUT_DESC outputDesc;
			memset(&outputDesc, 0, sizeof(outputDesc));
			if (FAILED(pOutput->GetDesc(&outputDesc)))
			{
				SafeRelease(pOutput);
				continue;
			}

			if (outputDesc.AttachedToDesktop)
			{
				UINT uModes = 0;
				if (SUCCEEDED(pOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0u, &uModes, nullptr)) &&
					uModes > 0u)
				{
					rv.Resize(uModes);
					if (SUCCEEDED(pOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0u, &uModes, rv.Data())))
					{
						SafeRelease(pOutput);
						SafeRelease(pAdapter);
						SafeRelease(pFactory);

						// Sort modes.
						D3D11SortByWidthThenHeightThenRefresh sorter(desktopMode);
						QuickSort(rv.Begin(), rv.End(), sorter);

						// Finally, filter. Remove modes we don't want.
						InternalStaticFilterModes(rv);
						return true;
					}
				}
			}

			SafeRelease(pOutput);
		}

		SafeRelease(pAdapter);
	}

	SafeRelease(pFactory);
	return false;
}

static void RenderThreadToggleMaximized()
{
	if (RenderDevice::Get())
	{
		RenderDevice::Get()->ToggleMaximized();
	}
}

static void RenderThreadToggleMinimized()
{
	if (RenderDevice::Get())
	{
		RenderDevice::Get()->ToggleMinimized();
	}
}

#if !SEOUL_SHIP
/**
 * @return The full coordinates of the entire virtual desktop.
 */
static inline Rectangle2DInt ComputeVirtualizedDesktopRect()
{
	SEOUL_ASSERT(IsRenderThread());

	Int32 const iDesktopX = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
	Int32 const iDesktopY = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
	Int32 const iDesktopWidth = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
	Int32 const iDesktopHeight = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);

	return Rectangle2DInt(iDesktopX, iDesktopY, iDesktopX + iDesktopWidth, iDesktopY + iDesktopHeight);
}
#endif

D3DCommonDevice* D3D11DeviceWindow::CreateDeviceD3D11(const D3DCommonDeviceSettings& deviceSettings)
{
	return SEOUL_NEW(MemoryBudgets::Rendering) D3D11DeviceWindow(deviceSettings);
}

Bool D3D11DeviceWindow::IsSupportedD3D11(const D3DCommonDeviceSettings& deviceSettings)
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

D3D11DeviceWindow::D3D11DeviceWindow(const D3DCommonDeviceSettings& settings)
	: D3D11Device(settings)
	, m_pVblankUtil(SEOUL_NEW(MemoryBudgets::Rendering) D3D11VblankUtil)
	, m_hMainWindow(nullptr)
	, m_D3DSwapChainDescription()
	, m_pD3DSwapChain(nullptr)
	, m_vAvailableDisplayModes()
	, m_ActiveMode()
	, m_DesktopMode()
	, m_LastValidMode()
#if !SEOUL_SHIP
	, m_VirtualizedDesktopRect()
#endif
	, m_nActiveRenderModeIndex(-1)
	, m_bIgnoreActivateEvents(false)
	, m_bLeavingFullscreen(false)
	, m_bRefreshFullscreen(false)
	, m_bWantsFullscreen(false)
	, m_bMaximized(false)
	, m_bMinimized(false)
	, m_bPendingShowWindow(false)
	, m_bActive(true)
	, m_bHasFrameToPresent(false)
#if !SEOUL_SHIP
	, m_bWantsVirtualizedDesktop(false)
#endif
	, m_bOsWindowRegionsDirty(false)
{
#if !SEOUL_SHIP
	m_bWantsVirtualizedDesktop = m_GraphicsParameters.m_bVirtualizedDesktop;
	memset(&m_VirtualizedDesktopMainFormWindowRect, 0, sizeof(m_VirtualizedDesktopMainFormWindowRect));
#endif // /!SEOUL_SHIP
	memset(&m_D3DSwapChainDescription, 0, sizeof(m_D3DSwapChainDescription));
	memset(&m_ActiveMode, 0, sizeof(m_ActiveMode));
	memset(&m_DesktopMode, 0, sizeof(m_DesktopMode));
	memset(&m_LastValidMode, 0, sizeof(m_LastValidMode));

	Construct();
}

D3D11DeviceWindow::~D3D11DeviceWindow()
{
	Destruct();
	m_pVblankUtil.Reset();
}

/**
 * @return The current render mode index.
 */
Int D3D11DeviceWindow::GetActiveRenderModeIndex() const
{
	return m_nActiveRenderModeIndex;
}

/** Given an index into the list of render modes, switches to that mode */
Bool D3D11DeviceWindow::SetRenderModeByIndex(Int nRenderMode)
{
	if (m_nActiveRenderModeIndex == nRenderMode ||
		nRenderMode < 0 ||
		(UInt32)nRenderMode >= m_vAvailableRenderModeNames.GetSize())
	{
		return false;
	}

	m_nActiveRenderModeIndex = nRenderMode;
	return true;
}

void D3D11DeviceWindow::SetRenderMode(const DXGI_MODE_DESC& mode)
{
	SEOUL_ASSERT(IsRenderThread());

	if (mode != m_ActiveMode)
	{
		UInt const uWidth = mode.Width;
		UInt const uHeight = mode.Height;

		// Update viewport, present param, and graphics params.
		m_GraphicsParameters.m_iFullscreenWidth = uWidth;
		m_GraphicsParameters.m_iFullscreenHeight = uHeight;

		// Immediately commit the size if we're in full screen mode.
		if (!m_D3DSwapChainDescription.Windowed)
		{
			m_D3DSwapChainDescription.BufferDesc = mode;

			m_bRefreshFullscreen = true;
			SetNeedsReset();
		}

		// Set the active mode.
		m_ActiveMode = mode;
	}
}

/**
 * Returns true if the game is currently in windowed mode, false
 * otherwise.
 *
 * If this function returns false, then the game is in fullscreen
 * mode.
 */
Bool D3D11DeviceWindow::IsWindowed() const
{
	SEOUL_ASSERT(IsRenderThread());

	return (FALSE != m_D3DSwapChainDescription.Windowed);
}

/**
 * Toggles the render window between full screen and windowed
 * mode.
 * This also marks the graphics device as needing a reset.
 */
void D3D11DeviceWindow::ToggleFullscreenMode()
{
	m_bWantsFullscreen = !m_bWantsFullscreen;
	if (!m_bWantsFullscreen)
	{
		m_bLeavingFullscreen = true;
	}
	SetNeedsReset();
}

/** On supported platform, toggle maximization of the main viewport window. */
void D3D11DeviceWindow::ToggleMaximized()
{
	// Early out.
	if (nullptr == m_hMainWindow)
	{
		return;
	}

	// Must happen on the render thread.
	if (!IsRenderThread())
	{
		Jobs::AsyncFunction(GetRenderThreadId(), &RenderThreadToggleMaximized);
		return;
	}

	(void)::ShowWindow(m_hMainWindow, (m_bMaximized ? SW_RESTORE : SW_MAXIMIZE));
}

/** On supported platform, toggle minimization of the main viewport window. */
void D3D11DeviceWindow::ToggleMinimized()
{
	// Early out.
	if (nullptr == m_hMainWindow)
	{
		return;
	}

	// Must happen on the render thread.
	if (!IsRenderThread())
	{
		Jobs::AsyncFunction(GetRenderThreadId(), &RenderThreadToggleMinimized);
		return;
	}

	(void)::ShowWindow(m_hMainWindow, (m_bMinimized ? SW_RESTORE : SW_MINIMIZE));
}

/**
 * Does the initial application window setup. Loads
 * some setup variables from JSON files as well.
 */
void D3D11DeviceWindow::InternalInitializeWindow()
{
	SEOUL_ASSERT(IsRenderThread());

	auto const& settings = GetSettings();

	WNDCLASS windowClass;
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = settings.m_pWndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = settings.m_hInstance;
	windowClass.hIcon = LoadIcon(settings.m_hInstance, MAKEINTRESOURCE(settings.m_iApplicationIcon));
	windowClass.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = "D3D11WndClassName";

	ATOM result = RegisterClass(&windowClass);
	SEOUL_ASSERT_MESSAGE(result, "Unable to register the window class");

	InternalCreateWindow(
		settings.m_sLocalizedAppNameAndVersion + " - D3D11",
		settings.m_hInstance,
		m_GraphicsParameters.m_bStartFullscreen);
}

/**
 * Applies virtualization mode and other sanity to the basic window
 * position and layout.
 */
RECT D3D11DeviceWindow::InternalApplyVirtualizedModeToGraphicsSettings()
{
	Int32 const iDesktopX = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
	Int32 const iDesktopY = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
	Int32 const iDesktopWidth = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
	Int32 const iDesktopHeight = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);

#if !SEOUL_SHIP
	// When using a virtualized desktop, our render area fills the entire virtual desktop.
	if (m_GraphicsParameters.m_bVirtualizedDesktop)
	{
		m_GraphicsParameters.m_iWindowViewportX = iDesktopX;
		m_GraphicsParameters.m_iWindowViewportY = iDesktopY;
		m_GraphicsParameters.m_iWindowViewportWidth = iDesktopWidth;
		m_GraphicsParameters.m_iWindowViewportHeight = iDesktopHeight;
	}
#endif // /!SEOUL_SHIP

	// Initial values.
	RECT rectangle =
	{
		m_GraphicsParameters.m_iWindowViewportX,
		m_GraphicsParameters.m_iWindowViewportY,
		m_GraphicsParameters.m_iWindowViewportX + m_GraphicsParameters.m_iWindowViewportWidth,
		m_GraphicsParameters.m_iWindowViewportY + m_GraphicsParameters.m_iWindowViewportHeight
	};
	AdjustWindowRectEx(&rectangle, GetD3DWindowedModeWindowStyle(), false, GetD3DWindowedModeWindowStyleEx());

#if !SEOUL_SHIP
	// Apply sanity clamping - don't allow sizes to go outside the virtualzed desktop region.
	if (!m_GraphicsParameters.m_bVirtualizedDesktop)
#endif
	{
		// Keep the window from being created with its left border off the left edge of the desktop area.
		if (rectangle.left < iDesktopX)
		{
			LONG adjustment = (iDesktopX - rectangle.left);
			rectangle.left += adjustment;
			rectangle.right += adjustment;
			m_GraphicsParameters.m_iWindowViewportX += adjustment;
		}

		// Keep the window from being created with its top border off the top edge of the desktop area.
		if (rectangle.top < iDesktopY)
		{
			LONG adjustment = (iDesktopY - rectangle.top);
			rectangle.top += adjustment;
			rectangle.bottom += adjustment;
			m_GraphicsParameters.m_iWindowViewportY += adjustment;
		}

		if (rectangle.right > (iDesktopX + iDesktopWidth))
		{
			rectangle.right = (iDesktopX + iDesktopWidth);
		}

		if (rectangle.bottom > (iDesktopY + iDesktopHeight))
		{
			rectangle.bottom = (iDesktopY + iDesktopHeight);
		}
	}

	// Update window parameters.
	m_GraphicsParameters.m_iWindowXOffset = (rectangle.left - m_GraphicsParameters.m_iWindowViewportX);
	m_GraphicsParameters.m_iWindowYOffset = (rectangle.top - m_GraphicsParameters.m_iWindowViewportY);

	return rectangle;
}

/**
 * This function actually creates the application window, making
 * sure it is the required size for whatever client dimensions
 * have been selected.
 */
void D3D11DeviceWindow::InternalCreateWindow(
	const String& sAppNameAndVersion,
	HINSTANCE hInstance,
	Bool bStartFullscreen)
{
	SEOUL_ASSERT(IsRenderThread());

#if !SEOUL_SHIP
	m_VirtualizedDesktopRect = ComputeVirtualizedDesktopRect();
#endif // /!SEOUL_SHIP
	auto const rectangle = InternalApplyVirtualizedModeToGraphicsSettings();

	m_sOsWindowTitle.Assign(sAppNameAndVersion);
	m_hMainWindow =
		CreateWindowExW(
			GetD3DWindowedModeWindowStyleEx(),
			L"D3D11WndClassName",
			sAppNameAndVersion.WStr(),
			GetD3DWindowedModeWindowStyle(),
			rectangle.left,
			rectangle.top,
			(rectangle.right - rectangle.left),
			(rectangle.bottom - rectangle.top),
			0u,
			0u,
			hInstance,
			nullptr);

	SEOUL_ASSERT_MESSAGE(m_hMainWindow != nullptr, "Failed creating client window.");

#if !SEOUL_SHIP
	// Thumbnail management util - only used in non-ship for virtualized desktops.
	m_pThumbnailUtil.Reset(SEOUL_NEW(MemoryBudgets::Rendering) D3DCommonThumbnailUtil(m_hMainWindow));
#endif // /!SEOUL_SHIP

	// After creating the window, potentially fixup in response to the os
	// clamping the window dimensions.
	(void)InternalFixupClientAndWindowInWindowedMode();

	// Notify the render system to trigger a device resest to go fullscreen.
	if (bStartFullscreen)
	{
		m_bWantsFullscreen = true;
		SetNeedsReset();
		m_bPendingShowWindow = false;
	}
	// Otherwise show the window - leave it unshown if we're going to full screen anyway.
	else
	{
		m_bPendingShowWindow = true;
	}

	::UpdateWindow(m_hMainWindow);
}

/**
 * Destroys the application window if it exists.
 */
void D3D11DeviceWindow::InternalDestroyWindow()
{
	SEOUL_ASSERT(IsRenderThread());

	// Always disable the cursor clip once we get here.
	::ClipCursor(nullptr);

#if !SEOUL_SHIP
	// Release thumbnail util.
	m_pThumbnailUtil.Reset();
#endif // /!SEOUL_SHIP

	if (m_hMainWindow)
	{
		::DestroyWindow(m_hMainWindow);
		m_hMainWindow = nullptr;
	}
}

/**
 * Update the cursor clip mode.
 *
 * When going into full screen, the cursor is clipped to
 * the game window to prevent accidentally clicking outside
 * the screen on multi-monitor setups.
 */
void D3D11DeviceWindow::InternalUpdateCursorClip()
{
	SEOUL_ASSERT(IsRenderThread());

	if (m_GraphicsParameters.m_bWindowedFullscreen || m_bMinimized || IsWindowed() || !m_bActive)
	{
		::ClipCursor(nullptr);
	}
	else
	{
		// Clamp the cursor to the full screen region as long as we're in
		// full screen mode.
		RECT clientRectangle;
		clientRectangle.top = 0;
		clientRectangle.left = 0;
		clientRectangle.right = m_D3DSwapChainDescription.BufferDesc.Width;
		clientRectangle.bottom = m_D3DSwapChainDescription.BufferDesc.Height;
		::ClipCursor(&clientRectangle);
	}
}

/**
 * Handles toggling between full screen and windowed mode.
 * A lot of the dance in this function is related to ensuring that
 * the window is configured as neeed by windows, and making sure the
 * window returns to its previous dimensions and position when
 * exiting fullscreen.
 */
void D3D11DeviceWindow::InternalToggleFullscreen()
{
	SEOUL_ASSERT(IsRenderThread());

	GraphicsParameters& parameters = m_GraphicsParameters;

	// Switch to fullscreen mode.
	if (m_D3DSwapChainDescription.Windowed || m_bRefreshFullscreen)
	{
		// If we're in windowed full screen, override the mode to the Desktop mode.
		if (m_GraphicsParameters.m_bWindowedFullscreen)
		{
			m_ActiveMode = m_DesktopMode;
		}

		m_GraphicsParameters.m_iFullscreenWidth = m_ActiveMode.Width;
		m_GraphicsParameters.m_iFullscreenHeight = m_ActiveMode.Height;

		m_D3DSwapChainDescription.BufferDesc = m_ActiveMode;
		m_D3DSwapChainDescription.Windowed = FALSE;

		// If we're entering fullscreen (not windowed fullscreen),
		// hide the window.
		if (!m_GraphicsParameters.m_bWindowedFullscreen)
		{
			m_bIgnoreActivateEvents = true;
			(void)::ShowWindow(m_hMainWindow, SW_HIDE);
			m_bPendingShowWindow = false;
			m_bIgnoreActivateEvents = false;
		}

		// Switch the window to the POPUP style, which is no decorations or border at all.
		SetWindowLongPtr(m_hMainWindow, GWL_STYLE, WS_POPUP);

		// Set the window parameters.
		SetWindowPos(
			m_hMainWindow,
			HWND_NOTOPMOST,
			0, 0,
			m_D3DSwapChainDescription.BufferDesc.Width,
			m_D3DSwapChainDescription.BufferDesc.Height,
			SWP_NOCOPYBITS);

		// see http://msdn.microsoft.com/en-us/library/ms633545(VS.85).aspx for what this does.
		// The first two calls (to SetWindowPos and SetWindowLongPtr) actually configure
		// the window as we need it to be configured, this call ensures that the changes
		// are committed.
		SetWindowPos(
			m_hMainWindow,
			HWND_NOTOPMOST,
			0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
	}
	// Switch to windowed mode.
	else
	{
		// If we're exiting full screen from windowed fullscreen,
		// hide the window.
		if (m_GraphicsParameters.m_bWindowedFullscreen)
		{
			m_bIgnoreActivateEvents = true;
			(void)::ShowWindow(m_hMainWindow, SW_HIDE);
			m_bPendingShowWindow = false;
			m_bIgnoreActivateEvents = false;
		}

		// Update the device parameters from the final calculated client viewport width and height.
		m_D3DSwapChainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_D3DSwapChainDescription.BufferDesc.Width = parameters.m_iWindowViewportWidth;
		m_D3DSwapChainDescription.BufferDesc.Height = parameters.m_iWindowViewportHeight;
		m_D3DSwapChainDescription.Windowed = TRUE;
	}

	m_bRefreshFullscreen = false;
	m_bWantsFullscreen = !m_D3DSwapChainDescription.Windowed;
	SetNeedsReset();
}

/**
* Wrapper around the Win32 function ::SetWindowPos() that executes that
* function with the standard set of parameters to size and position the window
* based on m_GraphicsParameters when running in windowed mode.
*/
void D3D11DeviceWindow::InternalSetWindowPosInWindowedModeBasedOnClientViewport()
{
	SEOUL_ASSERT(IsRenderThread());

	RECT rectangle =
	{
		m_GraphicsParameters.m_iWindowViewportX,
		m_GraphicsParameters.m_iWindowViewportY,
		m_GraphicsParameters.m_iWindowViewportX + m_GraphicsParameters.m_iWindowViewportWidth,
		m_GraphicsParameters.m_iWindowViewportY + m_GraphicsParameters.m_iWindowViewportHeight
	};

	// This call takes a client rectangle as input and returns
	// a rectangle correctly sized for the window that contains
	// the client area.
	SEOUL_VERIFY(FALSE != AdjustWindowRectEx(
		&rectangle,
		GetD3DWindowedModeWindowStyle(),
		FALSE,
		GetD3DWindowedModeWindowStyleEx()));

	// Set the size of the window for windowed mode. Only
	// do this on changes, since calling this redundantly
	// can effectively break maximize/restore behavior.
	RECT actualRectangle;
	memset(&actualRectangle, 0, sizeof(actualRectangle));
	SEOUL_VERIFY(FALSE != ::GetWindowRect(m_hMainWindow, &actualRectangle));

	if (actualRectangle.bottom != rectangle.bottom ||
		actualRectangle.left != rectangle.left ||
		actualRectangle.right != rectangle.right ||
		actualRectangle.top != rectangle.top)
	{
		SEOUL_VERIFY(FALSE != ::SetWindowPos(
			m_hMainWindow,
			HWND_NOTOPMOST,
			rectangle.left,
			rectangle.top,
			(rectangle.right - rectangle.left),
			(rectangle.bottom - rectangle.top),
			SWP_NOCOPYBITS));
	}
}

/**
 * Windows can occasionally clamp the desired window. This function
 * must be called after any window resizing calls in order to catch and potentially
 * fixup differences between the window client viewport and the desired client viewport
 * dimensions.
 *
 * @return True if the client viewport was resized to account for window clamping,
 * false otherwise.
 */
Bool D3D11DeviceWindow::InternalFixupClientAndWindowInWindowedMode()
{
	SEOUL_ASSERT(IsRenderThread());

	// Get the actual client rectangle.
	RECT clientRectangle;
	memset(&clientRectangle, 0, sizeof(clientRectangle));
	SEOUL_VERIFY(FALSE != ::GetClientRect(m_hMainWindow, &clientRectangle));

	// Calculate actual client width and height and differences from the expected width and height.
	Int const iActualClientWidth = (clientRectangle.right - clientRectangle.left);
	Int const iActualClientHeight = (clientRectangle.bottom - clientRectangle.top);
	Int const iWidthDifference = Abs(iActualClientWidth - m_GraphicsParameters.m_iWindowViewportWidth);
	Int const iHeightDifference = Abs(iActualClientHeight - m_GraphicsParameters.m_iWindowViewportHeight);

	// If the actual width and height differ from the expected, adjust the window.
	if (iWidthDifference != 0 ||
		iHeightDifference != 0)
	{
		// Get the target aspect ratio.
		Float const fAspectRatio = (Float)m_GraphicsParameters.m_iWindowViewportWidth / (Float)m_GraphicsParameters.m_iWindowViewportHeight;

		// If the width was clamped more than the height, keep the width and rescale the height
		// based on the aspect ratio.
		if (iWidthDifference > iHeightDifference)
		{
			m_GraphicsParameters.m_iWindowViewportWidth = iActualClientWidth;
			m_GraphicsParameters.m_iWindowViewportHeight = (Int)(iActualClientWidth / fAspectRatio);
		}
		// Otherwise, keep the height and rescale the width.
		else
		{
			m_GraphicsParameters.m_iWindowViewportHeight = iActualClientHeight;
			m_GraphicsParameters.m_iWindowViewportWidth = (Int)(iActualClientHeight * fAspectRatio);
		}

		// Resize the window.
		InternalSetWindowPosInWindowedModeBasedOnClientViewport();

		// Final fail-safe - if the client viewport is still not right, give up, and just update
		// our cached parameters so everything is in-sync.
		SEOUL_VERIFY(FALSE != ::GetClientRect(m_hMainWindow, &clientRectangle));
		m_GraphicsParameters.m_iWindowViewportHeight = (clientRectangle.bottom - clientRectangle.top);
		m_GraphicsParameters.m_iWindowViewportWidth = (clientRectangle.right - clientRectangle.left);

		return true;
	}

	return false;
}

/**
 * Apply the m_nActiveRenderModeIndex member, attempting to configure
 * the device to set the desired mode.
 */
void D3D11DeviceWindow::InternalApplyPendingRenderModeIndex()
{
	SEOUL_ASSERT(IsRenderThread());

	Int const nActiveRenderModeIndex = InternalGetActiveRenderModeIndex();
	if (nActiveRenderModeIndex == m_nActiveRenderModeIndex)
	{
		return;
	}

	// Wants windowed mode.
	if (kRenderModeWindowedIndex == m_nActiveRenderModeIndex)
	{
		if (!IsWindowed())
		{
			m_bLeavingFullscreen = true;
		}
		m_bWantsFullscreen = false;
		SetNeedsReset();
	}
	// Wants windowed full screen.
	else if (kRenderModeWindowedFullscreenIndex == m_nActiveRenderModeIndex)
	{
		// Set the render mode to the desktop.
		SetRenderMode(m_DesktopMode);
		m_bWantsFullscreen = true;
		m_GraphicsParameters.m_bWindowedFullscreen = true;
		if (!IsWindowed())
		{
			m_bRefreshFullscreen = true;
		}
		SetNeedsReset();
	}
	// Wants full screen, specific mode.
	else if (
		m_nActiveRenderModeIndex >= kSpecialRenderModeCount &&
		(UInt32)(m_nActiveRenderModeIndex - kSpecialRenderModeCount) < m_vAvailableDisplayModes.GetSize())
	{
		SetRenderMode(m_vAvailableDisplayModes[m_nActiveRenderModeIndex - kSpecialRenderModeCount]);
		m_bWantsFullscreen = true;
		m_GraphicsParameters.m_bWindowedFullscreen = false;
		SetNeedsReset();
	}
}

/**
 * @return The actual active render mode index, derived from the control
 * flags that configure the device.
 */
Int D3D11DeviceWindow::InternalGetActiveRenderModeIndex() const
{
	SEOUL_ASSERT(IsRenderThread());

	// Windowed mode is render mode kRenderModeWindowedIndex.
	if (IsWindowed())
	{
		return kRenderModeWindowedIndex;
	}
	// Full screen is either render mode kRenderModeWindowedFullscreenIndex (windowed fullscreen) or an index into the available modes + kSpecialRenderModeCount.
	else
	{
		if (m_GraphicsParameters.m_bWindowedFullscreen)
		{
			return kRenderModeWindowedFullscreenIndex;
		}
		else
		{
			UInt32 const nModes = m_vAvailableDisplayModes.GetSize();
			for (UInt32 i = 0u; i < nModes; ++i)
			{
				if (m_vAvailableDisplayModes[i] == m_ActiveMode)
				{
					return (Int)i + kSpecialRenderModeCount;
				}
			}
		}
	}

	// Something horrible has happened.
	return -1;
}

/**
 * Acquire a strong reference to the device's
 * back buffer.
 */
ID3D11Texture2D* D3D11DeviceWindow::AcquireBackBuffer()
{
	ID3D11Texture2D* pBackBuffer = nullptr;
	if (!SUCCEEDED(m_pD3DSwapChain->GetBuffer(
		0u,
		__uuidof(pBackBuffer),
		(void**)(&pBackBuffer))))
	{
		return nullptr;
	}

	return pBackBuffer;
}

/**
 * Helper function, calculates the default viewport
 * that should be used for the backbuffer.
 */
Viewport D3D11DeviceWindow::InternalCreateDefaultViewport() const
{
	Viewport viewport;
	viewport.m_iTargetWidth = (Int32)m_D3DSwapChainDescription.BufferDesc.Width;
	viewport.m_iTargetHeight = (Int32)m_D3DSwapChainDescription.BufferDesc.Height;
	viewport.m_iViewportX = 0;
	viewport.m_iViewportY = 0;
	viewport.m_iViewportWidth = viewport.m_iTargetWidth;
	viewport.m_iViewportHeight = viewport.m_iTargetHeight;
	return viewport;
}

/**
 * Initialize the Direct 3D device interface
 */
void D3D11DeviceWindow::InitializeDirect3DDevice(
	ID3D11Device*& rpD3DDevice,
	ID3D11DeviceContext*& rpD3DDeviceContext)
{
	SEOUL_ASSERT(IsRenderThread());

	// Create the window.
	InternalInitializeWindow();

	// Fill out the DXGI_SWAP_CHAIN_DESC.
	memset(&m_D3DSwapChainDescription, 0, sizeof(m_D3DSwapChainDescription));
	m_D3DSwapChainDescription.BufferCount = 1;
	m_D3DSwapChainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_D3DSwapChainDescription.BufferDesc.Height = m_GraphicsParameters.m_iWindowViewportHeight;
	m_D3DSwapChainDescription.BufferDesc.Width = m_GraphicsParameters.m_iWindowViewportWidth;
	m_D3DSwapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	m_D3DSwapChainDescription.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	m_D3DSwapChainDescription.OutputWindow = (HWND)m_hMainWindow;
	m_D3DSwapChainDescription.SampleDesc.Count = 1;
	m_D3DSwapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	m_D3DSwapChainDescription.Windowed = TRUE;

	// Create the D3D11 Device.
#if SEOUL_DEBUG
	// Try a debug device first if a debug build.
	UINT const uFlags = D3D11_CREATE_DEVICE_DEBUG;
#else
	// Otherwise, no specific flags.
	UINT const uFlags = 0u;
#endif

	auto const eResult = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		uFlags,
		kaFeatureLevels,
		SEOUL_ARRAY_COUNT(kaFeatureLevels),
		D3D11_SDK_VERSION,
		&m_D3DSwapChainDescription,
		&m_pD3DSwapChain,
		&rpD3DDevice,
		nullptr,
		&rpD3DDeviceContext);

	// Create failed, fall back to a standard (not debug) device.
	if (eResult < 0)
	{
		// Try again - this create must succeed.
		SEOUL_D3D11_VERIFY(D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0u, // No flags for fallback creation.
			kaFeatureLevels,
			SEOUL_ARRAY_COUNT(kaFeatureLevels),
			D3D11_SDK_VERSION,
			&m_D3DSwapChainDescription,
			&m_pD3DSwapChain,
			&rpD3DDevice,
			nullptr,
			&rpD3DDeviceContext));
	}

	// Sanity check that all outputs were created.
	SEOUL_ASSERT(nullptr != m_pD3DSwapChain);
	SEOUL_ASSERT(nullptr != rpD3DDevice);
	SEOUL_ASSERT(nullptr != rpD3DDeviceContext);

	// Disable automatic ALT+ENTER handling.
	{
		IDXGIDevice* pDXGIDevice = nullptr;
		SEOUL_D3D11_VERIFY(rpD3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice));

		IDXGIAdapter* pDXGIAdapter = nullptr;
		SEOUL_D3D11_VERIFY(pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&pDXGIAdapter));

		IDXGIFactory* pIDXGIFactory = nullptr;
		SEOUL_D3D11_VERIFY(pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&pIDXGIFactory));

		pIDXGIFactory->MakeWindowAssociation(m_hMainWindow, DXGI_MWA_NO_ALT_ENTER);
		SafeRelease(pIDXGIFactory);
		SafeRelease(pDXGIAdapter);
		SafeRelease(pDXGIDevice);
	}

	// Get the current display mode.
	{
		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		memset(&swapChainDesc, 0, sizeof(swapChainDesc));
		SEOUL_D3D11_VERIFY(m_pD3DSwapChain->GetDesc(&swapChainDesc));

		// Basic mode matches the current in the swap chain.
		m_DesktopMode = swapChainDesc.BufferDesc;

		// Now update with the actual desktop resolutions.
		IDXGIOutput* pOutput = nullptr;
		SEOUL_D3D11_VERIFY(m_pD3DSwapChain->GetContainingOutput(&pOutput));

		DXGI_OUTPUT_DESC outputDesc;
		memset(&outputDesc, 0, sizeof(outputDesc));
		SEOUL_D3D11_VERIFY(pOutput->GetDesc(&outputDesc));
		m_DesktopMode.Width = (outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left);
		m_DesktopMode.Height = (outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top);

		// Finally, match to an actual mode to get the refresh rate.
		if (FAILED(pOutput->FindClosestMatchingMode(&m_DesktopMode, &m_DesktopMode, nullptr)))
		{
			// On failure, use a reasonable fallback for refresh rate.
			m_DesktopMode = swapChainDesc.BufferDesc;
			if (m_DesktopMode.RefreshRate.Denominator == 0 ||
				m_DesktopMode.RefreshRate.Numerator == 0)
			{
				// 60 Hz.
				m_DesktopMode.RefreshRate.Denominator = 1000;
				m_DesktopMode.RefreshRate.Numerator = 60000;
			}
		}

		SafeRelease(pOutput);
	}

	// Gather display modes
	(void)InternalStaticGetDisplayModes(m_DesktopMode, m_vAvailableDisplayModes);

	// Setup render mode labels.
	m_vAvailableRenderModeNames.Clear();

	// The first two entries are special - entry 0 is "Windowed", entry 1 is "Windowed Fullscreen".
	m_vAvailableRenderModeNames.PushBack(LocManager::Get()->Localize(HString("render_mode_windowed")));
	m_vAvailableRenderModeNames.PushBack(LocManager::Get()->Localize(HString("render_mode_windowed_fullscreen")));

	// Cache the base string for fullscreen strings - these have 2 placeholders, %Width and %Height.
	String sFullscreenString(LocManager::Get()->Localize(HString("render_mode_fullscreen")));

	for (UInt32 i = 0u; i < m_vAvailableDisplayModes.GetSize(); ++i)
	{
		String sFullscreen(sFullscreenString
			.ReplaceAll("%Width", String::Printf("%u", m_vAvailableDisplayModes[i].Width))
			.ReplaceAll("%Height", String::Printf("%u", m_vAvailableDisplayModes[i].Height)));
		m_vAvailableRenderModeNames.PushBack(sFullscreen);
	}

	// Initialize the last valid mode to the default.
	m_LastValidMode = m_DesktopMode;

	// If full screen mode is unspecified, use the starting mode.
	if (m_GraphicsParameters.m_iFullscreenHeight < 0 ||
		m_GraphicsParameters.m_iFullscreenWidth < 0)
	{
		SetRenderMode(m_DesktopMode);
	}
	else
	{
		// Get the target aspect ratio.
		Float const fAspectRatio = (Float)m_GraphicsParameters.m_iFullscreenWidth / (Float)m_GraphicsParameters.m_iFullscreenHeight;

		// Find a matching mode that is close to the desired resolution with the same aspect ratio
		auto closestMode = m_vAvailableDisplayModes.End();
		// Keep track of the difference in just the width since they should have the same aspect ratio.
		Float fWidthDiff = FloatMax;
		for (auto iter = m_vAvailableDisplayModes.Begin();
			iter != m_vAvailableDisplayModes.End();
			iter++)
		{
			Float fTestAspectRatio = (Float)iter->Width / (Float)iter->Height;
			// Check if they are relatively equal since the resolution was clamped to integer pixels
			static const Float fAspectRatioEpsilon = 0.001f;
			if (Equals(fAspectRatio, fTestAspectRatio, fAspectRatioEpsilon))
			{
				Float fTestWidthDiff = Abs((Float)m_GraphicsParameters.m_iFullscreenWidth - (Float)iter->Width);
				if (fTestWidthDiff < fWidthDiff)
				{
					closestMode = iter;
					fWidthDiff = fTestWidthDiff;
				}
			}
		}

		// If no resolution with the same aspect ratio was found, use the starting mode
		if (closestMode != m_vAvailableDisplayModes.End())
		{
			SetRenderMode(*closestMode);
		}
		else
		{
			SetRenderMode(m_DesktopMode);
		}
	}
}

void D3D11DeviceWindow::DeinitializeDirect3D()
{
	SafeRelease(m_pD3DSwapChain);
	InternalDestroyWindow();
}

/** Device specific render check. */
Bool D3D11DeviceWindow::InternalDoCanRender() const
{
	// Can't render if we don't have a window.
	if (nullptr == m_hMainWindow)
	{
		return false;
	}

	return true;
}

/** Device specific implementation of reset. */
Bool D3D11DeviceWindow::InternalDoResetDevice()
{
	SEOUL_ASSERT(IsRenderThread());

#if !SEOUL_SHIP
	// Apply virtualized desktop sanitizing.
	if (m_bWantsVirtualizedDesktop != m_GraphicsParameters.m_bVirtualizedDesktop)
	{
		// Capture prior to the change.
		if (m_bWantsVirtualizedDesktop)
		{
			m_VirtualizedDesktopRect = ComputeVirtualizedDesktopRect();
		}
		else
		{
			m_VirtualizedDesktopRect = Rectangle2DInt();
		}

		// Update.
		m_GraphicsParameters.m_bVirtualizedDesktop = m_bWantsVirtualizedDesktop;
		// If we just switched out of a virtualized desktop, reset viewport and apply sanitizing
		// to pick a reasonable new value.
		if (!m_GraphicsParameters.m_bVirtualizedDesktop)
		{
			m_GraphicsParameters.m_iWindowViewportX = m_GraphicsParameters.m_iWindowViewportY = 0;
			m_GraphicsParameters.m_iWindowViewportWidth = m_GraphicsParameters.m_iWindowViewportHeight = 0;
			InternalSanitizeGraphicsSettings();
		}
		// Apply new settings.
		(void)InternalApplyVirtualizedModeToGraphicsSettings();
		// Invalidate bitmap cache.
		if (m_pThumbnailUtil.IsValid()) { m_pThumbnailUtil->InvalidateCachedBitmaps(); }
	}
#endif // /!SEOUL_SHIP

	// Handling for full screen and buffer dimensions.
	if (m_bRefreshFullscreen ||
		(m_bWantsFullscreen == (m_D3DSwapChainDescription.Windowed == TRUE)))
	{
		InternalToggleFullscreen();
	}
	// We only update the back buffer dimensions in windowed mode. Full
	// screen is fixed until we return to windowed mode.
	else if (m_D3DSwapChainDescription.Windowed)
	{
		m_D3DSwapChainDescription.BufferDesc.Width = m_GraphicsParameters.m_iWindowViewportWidth;
		m_D3DSwapChainDescription.BufferDesc.Height = m_GraphicsParameters.m_iWindowViewportHeight;
	}

	// Cache the current graphics parameters - potentially used
	// to perform a fixup after the device reset.
	GraphicsParameters parametersBackup = m_GraphicsParameters;

	// Before resetting in windowed mode, need to reset the window style.
	if (IsWindowed())
	{
		Bool bCommit = false;

		// Check the style - only want to commit the change
		// if the desired flags are not already present.
		DWORD const uStyle = GetD3DWindowedModeWindowStyle();
		DWORD const uCurrent = (DWORD)::GetWindowLongPtrW(m_hMainWindow, GWL_STYLE);
		DWORD const uMasked = (uStyle & uCurrent);
		if (uStyle != uMasked)
		{
			// Backup graphics parameters during this call, as it can trigger
			// messaging side effects that will dirty params.
			auto const params = m_GraphicsParameters;

			// Restore the window to the style we use for windowed mode.
			::SetWindowLongPtrW(
				m_hMainWindow,
				GWL_STYLE,
				uStyle);

			m_GraphicsParameters = params; // Restore;

			// Tracking.
			bCommit = true;
		}

		// Set the window dimensions based on the desired client viewport.
		InternalSetWindowPosInWindowedModeBasedOnClientViewport();

		// see http://msdn.microsoft.com/en-us/library/ms633545(VS.85).aspx for what this does.
		// The first two calls (to SetWindowPos and SetWindowLongPtr) actually configure
		// the window as we need it to be configured, this call ensures that the changes
		// are committed.
		if (bCommit)
		{
			::SetWindowPos(
				m_hMainWindow,
				HWND_NOTOPMOST,
				0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
		}

		// Potentially fixup the window if the os clamped its dimensions.
		(void)InternalFixupClientAndWindowInWindowedMode();

		// Make sure the window is visible on a reset in windowed mode.
		m_bPendingShowWindow = true;

		// Make sure the back buffer width and height are up to date.
		m_D3DSwapChainDescription.BufferDesc.Width = m_GraphicsParameters.m_iWindowViewportWidth;
		m_D3DSwapChainDescription.BufferDesc.Height = m_GraphicsParameters.m_iWindowViewportHeight;
	}

	// Create a local copy of present parameters.
	DXGI_SWAP_CHAIN_DESC desired = m_D3DSwapChainDescription;

	// If windowed fullscreen is enabled, set
	// the mode to windowed just for the reset - this prevents
	// exclusive ownership of the display but otherwise fulfills
	// fullscreen behavior.
	if (m_GraphicsParameters.m_bWindowedFullscreen)
	{
		desired.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desired.Windowed = TRUE;
	}

	Bool bSuccess = true;

	// Actual reset operations.
	{
		// Setup the swap chain.
		DXGI_SWAP_CHAIN_DESC current;
		memset(&current, 0, sizeof(current));
		if (FAILED(m_pD3DSwapChain->GetDesc(&current)))
		{
			bSuccess = false;
		}

		// Update dimensions if necessary.
		if (bSuccess)
		{
			if (current.BufferDesc.Height != desired.BufferDesc.Height ||
				current.BufferDesc.Width != desired.BufferDesc.Width)
			{
				if (FAILED(m_pD3DSwapChain->ResizeTarget(&desired.BufferDesc)))
				{
					bSuccess = false;
				}

				if (bSuccess)
				{
					if (FAILED(m_pD3DSwapChain->ResizeBuffers(
						0u,
						desired.BufferDesc.Width,
						desired.BufferDesc.Height,
						DXGI_FORMAT_UNKNOWN,
						0u)))
					{
						bSuccess = false;
					}
				}

				if (bSuccess)
				{
					current.BufferDesc.Height = desired.BufferDesc.Height;
					current.BufferDesc.Width = desired.BufferDesc.Width;
				}
			}
		}

		// Update full screen state if necessary.
		if (bSuccess)
		{
			if (current.Windowed != desired.Windowed)
			{
				if (FAILED(m_pD3DSwapChain->SetFullscreenState(!desired.Windowed, nullptr)))
				{
					bSuccess = false;
				}

				// See : https://msdn.microsoft.com/en-us/library/windows/desktop/ee417025(v=vs.85).aspx
				// We're supposed to call ResizeTarget() again with the refresh ratem member 0 out?
				if (bSuccess)
				{
					auto modifiedDesired = desired.BufferDesc;
					memset(&modifiedDesired.RefreshRate, 0, sizeof(modifiedDesired.RefreshRate));
					(void)m_pD3DSwapChain->ResizeTarget(&modifiedDesired);
				}
			}
		}
	}

	if (!bSuccess)
	{
		if (!IsWindowed())
		{
			// If we're not in windowed mode and
			// we have a last valid mode that's different
			// from the mode we're attempting, try one more
			// time with the last valid mode.
			if (m_LastValidMode != m_ActiveMode)
			{
				SetRenderMode(m_LastValidMode);
				return false;
			}
			// Otherwise, try switching back to windowed mode.
			else
			{
				ToggleFullscreenMode();
				return false;
			}
		}

		return false;
	}

	// If in Windowed mode, and if we are leaving fullscreen, check if we ended up with dimensions
	// that don't match our desired. If so, try again now that we've reset, since a full screen mode
	// change may have prevented the window size from matching the desired.
	if (IsWindowed() && m_bLeavingFullscreen)
	{
		if (parametersBackup.m_iWindowViewportHeight != m_GraphicsParameters.m_iWindowViewportHeight ||
			parametersBackup.m_iWindowViewportWidth != m_GraphicsParameters.m_iWindowViewportWidth)
		{
			// No longer leaving full screen.
			m_bLeavingFullscreen = false;

			// Restore graphics parameters to previous.
			m_GraphicsParameters = parametersBackup;

			// Still need a reset.
			SetNeedsReset();

			// This reset did not complete successfully, need to try again.
			return false;
		}
	}

	// No longer leaving full screen.
	m_bLeavingFullscreen = false;

	// Update the last valid render mode - if we get here, we've successfully
	// switch to or are running a valid mode.
	m_LastValidMode = m_ActiveMode;

	// Refresh cursor clipping.
	InternalUpdateCursorClip();

	// Once the device has reset, the backbuffer contents are undefined,
	// so we reset our state so that one frame must be rendered before
	// we're ready to present.
	m_bHasFrameToPresent = false;

	// Done, success.
	return true;
}

/**
 * @return The current display refresh rate, as reported by the system device driver.
 */
RefreshRate D3D11DeviceWindow::InternalGetRefreshRate() const
{
	SEOUL_ASSERT(IsRenderThread());

	// Handling for unknown refresh.
	if (m_DesktopMode.RefreshRate.Denominator == 0 ||
		m_DesktopMode.RefreshRate.Numerator == 0)
	{
		return RefreshRate();
	}
	else
	{
		return RefreshRate(
			(UInt32)m_DesktopMode.RefreshRate.Numerator,
			(UInt32)m_DesktopMode.RefreshRate.Denominator);
	}
}

/**
 * Present the back buffer.
 *
 * Depending on how the device was created, this call may or
 * may not block and wait for the vertical refresh.
 */
Bool D3D11DeviceWindow::InternalPresent()
{
	SEOUL_ASSERT(IsRenderThread());

	if (m_bHasFrameToPresent)
	{
		HRESULT hr = S_OK;

		{
			InternalPrePresent();
			ApplyOsWindowRegions();
			hr = m_pD3DSwapChain->Present(m_GraphicsParameters.m_iVsyncInterval, 0);
			InternalPostPresent();
		}

		m_bHasFrameToPresent = false;

		if (FAILED(hr))
		{
			SetNeedsReset();
			return false;
		}

		// Show the window now if pending.
		if (m_bPendingShowWindow)
		{
			(void)::ShowWindow(m_hMainWindow, SW_SHOW);
			m_bPendingShowWindow = false;
		}
	}

	return true;
}

void D3D11DeviceWindow::InternalBeginScenePreResetCheck()
{
	// Apply any render mode changes
	InternalApplyPendingRenderModeIndex();
}

void D3D11DeviceWindow::InternalBeginScenePostResetCheck()
{
	// Update the render mode based on what we actually ended up with.
	m_nActiveRenderModeIndex = InternalGetActiveRenderModeIndex();
}

void D3D11DeviceWindow::UpdateOsWindowRegions(OsWindowRegion const* pRegions, UInt32 uCount)
{
	// Simple case, clear the region.
	SEOUL_ASSERT(IsRenderThread());

	// Edge cases (shutdown).
	if (nullptr == m_hMainWindow)
	{
		return;
	}

	// Early out if already set.
	if (m_vOsWindowRegions.GetSize() == uCount)
	{
		if (0u == uCount)
		{
			return;
		}
		else
		{
			if (0 == memcmp(m_vOsWindowRegions.Data(), pRegions, m_vOsWindowRegions.GetSizeInBytes()))
			{
				return;
			}
		}
	}

	// Populate.
	m_vOsWindowRegions.Assign(pRegions, pRegions + uCount);
	m_bOsWindowRegionsDirty = true;
}

void D3D11DeviceWindow::ApplyOsWindowRegions()
{
	SEOUL_ASSERT(IsRenderThread());

	if (!m_bOsWindowRegionsDirty)
	{
		return;
	}

	// Updated.
	m_bOsWindowRegionsDirty = false;

#if !SEOUL_SHIP
	// Clear initially - may be computed
	// based on current input.
	memset(&m_VirtualizedDesktopMainFormWindowRect, 0, sizeof(m_VirtualizedDesktopMainFormWindowRect));
#endif // /!SEOUL_SHIP

	// Apply the region change.
	auto const uCount = m_vOsWindowRegions.GetSize();
	auto const pRegions = m_vOsWindowRegions.Data();
	if (0 == uCount)
	{
		// Wait for the vblank, so that the region
		// apply and present line up and we
		// don't have tearing artifacts.
		m_pVblankUtil->WaitForVblank();

		SEOUL_VERIFY(FALSE != ::SetWindowRgn(m_hMainWindow, nullptr, FALSE));
	}
	// Otherwise, build the region object.
	else
	{
		// TODO: Really, want to capture input when in the margin instead
		// of just expanding the rectangle. Expanding the rectangle creates a clear visual
		// artifact (solid black fill) and, if the client ever decides to optimize
		// rendering by only updating what's in the client rectangle, will just be broken
		// outright.
		auto rect = pRegions->m_Rect;
		rect.Expand((Int)Ceil(pRegions->m_fInputMargin));

		// See also: https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createrectrgn#remarks
		// "Regions created by the Create<shape>Rgn methods (such as CreateRectRgn and CreatePolygonRgn)
		//  only include the interior of the shape; the shape's outline is excluded from the region.
		//  This means that any point on a line between two sequential vertices is not included in the region.
		//  If you were to call PtInRegion for such a point, it would return zero as the result."
		//
		// Due to our semantics of a pixel rectangle, the right and bottom edge are +1 already (because
		// we want (right - left) = width, so right has to be the rightmost pixel we want to draw + 1),
		// but the left and top are not, so we subtract 1 from them.
		auto hRegion = ::CreateRectRgn(rect.m_iLeft - 1, rect.m_iTop - 1, rect.m_iRight, rect.m_iBottom);

#if !SEOUL_SHIP
		// Track.
		if (pRegions->m_bMainForm)
		{
			m_VirtualizedDesktopMainFormWindowRect.left = m_VirtualizedDesktopRect.m_iLeft + rect.m_iLeft;
			m_VirtualizedDesktopMainFormWindowRect.top = m_VirtualizedDesktopRect.m_iTop + rect.m_iTop;
			m_VirtualizedDesktopMainFormWindowRect.right = m_VirtualizedDesktopRect.m_iLeft + rect.m_iRight;
			m_VirtualizedDesktopMainFormWindowRect.bottom = m_VirtualizedDesktopRect.m_iTop + rect.m_iBottom;
		}
#endif // /!SEOUL_SHIP

		// Accumulate additional regions.
		for (UInt32 i = 1u; i < uCount; ++i)
		{
			auto const& region = pRegions[i];
			rect = region.m_Rect;
			rect.Expand((Int)Ceil(region.m_fInputMargin));

#if !SEOUL_SHIP
			// Track.
			if (region.m_bMainForm)
			{
				m_VirtualizedDesktopMainFormWindowRect.left = m_VirtualizedDesktopRect.m_iLeft + rect.m_iLeft;
				m_VirtualizedDesktopMainFormWindowRect.top = m_VirtualizedDesktopRect.m_iTop + rect.m_iTop;
				m_VirtualizedDesktopMainFormWindowRect.right = m_VirtualizedDesktopRect.m_iLeft + rect.m_iRight;
				m_VirtualizedDesktopMainFormWindowRect.bottom = m_VirtualizedDesktopRect.m_iTop + rect.m_iBottom;
			}
#endif // /!SEOUL_SHIP

			auto hAdditional = ::CreateRectRgn(rect.m_iLeft - 1, rect.m_iTop - 1, rect.m_iRight, rect.m_iBottom);
			(void)::CombineRgn(hRegion, hRegion, hAdditional, RGN_OR);
			SEOUL_VERIFY(FALSE != ::DeleteObject(hAdditional));
		}

		// Wait for the vblank, so that the region
		// apply and present line up and we
		// don't have tearing artifacts.
		m_pVblankUtil->WaitForVblank();

		// NOTE: We don't call DeleteRegion/own the region after this call:
		// See also: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowrgn#remarks
		// "After a successful call to SetWindowRgn, the system owns the region \
		//  specified by the region handle hRgn. The system does not make a copy
		//  of the region. Thus, you should not make any further function calls
		//  with this region handle. In particular, do not delete this region handle.
		//  The system deletes the region handle when it no longer needed."
		SEOUL_VERIFY(FALSE != ::SetWindowRgn(m_hMainWindow, hRegion, FALSE));
	}
}

/**
 * Friend function, only to be called by PCEngine.
 * When called, this function gets the current client area and position
 * and updates internal variables that store this information. It also
 * marks a flag that will cause the render device to reset itself and
 * resize internal buffers as needed.
 *
 * This function does not capture size if the current viewport mode
 * is bFixedViewport. bFixedViewport means that the client viewport should
 * never change.
 */
void D3D11DeviceWindow::PCEngineFriend_CaptureAndResizeClientViewport()
{
	SEOUL_ASSERT(IsRenderThread());

	RECT rect = { 0, 0, 0, 0 };

	// We get the client rectangle to determine the viewport width
	// and height.
	(void)::GetClientRect(m_hMainWindow, &rect);

	// Only update width/height of the window if we allow the user to resize the window
	// and we're not in full screen.
	if (IsWindowed())
	{
		// We need to clamp the width and height to a minimum of 1 or device
		// reset will fail due to an invalid parameter error.
		Int const iNewViewportWidth = Max((rect.right - rect.left), (LONG)1);
		Int const iNewViewportHeight = Max((rect.bottom - rect.top), (LONG)1);

		// Update and trigger a reset if the window resized.
		if (iNewViewportWidth != m_GraphicsParameters.m_iWindowViewportWidth ||
			iNewViewportHeight != m_GraphicsParameters.m_iWindowViewportHeight)
		{
			m_GraphicsParameters.m_iWindowViewportWidth = iNewViewportWidth;
			m_GraphicsParameters.m_iWindowViewportHeight = iNewViewportHeight;

			// Only necessary to trigger a device reset if the window was resized
			SetNeedsReset();
		}
	}

	// We need to derive the client-space upper-left corner from the window
	// rectangle. Only do this if we're in windowed mode and not minimized
	if (!m_bMinimized && IsWindowed())
	{
		(void)::GetWindowRect(m_hMainWindow, &rect);
		m_GraphicsParameters.m_iWindowViewportX = rect.left - m_GraphicsParameters.m_iWindowXOffset;
		m_GraphicsParameters.m_iWindowViewportY = rect.top - m_GraphicsParameters.m_iWindowYOffset;
	}

	// Update maximized state.
	{
		WINDOWPLACEMENT placement;
		memset(&placement, 0, sizeof(placement));

		SEOUL_VERIFY(FALSE != ::GetWindowPlacement(m_hMainWindow, &placement));

		m_bMaximized = (placement.showCmd == SW_MAXIMIZE);
	}
}

/**
 * Destroy the application window if it exists.
 */
void D3D11DeviceWindow::PCEngineFriend_DestroyWindow()
{
	SEOUL_ASSERT(IsRenderThread());

	InternalDestroyWindow();
}

/**
 * Marks the application window as either active or inactive.
 *
 * If the window becomes inactive in full-screen mode,
 * this function will also mark the graphics device as needing
 * to be reset.
 */
void D3D11DeviceWindow::PCEngineFriend_SetActive(Bool bActive)
{
	SEOUL_ASSERT(IsRenderThread());

	if (bActive != m_bActive)
	{
		m_bActive = bActive;
		if (!IsWindowed() && !m_GraphicsParameters.m_bWindowedFullscreen)
		{
			SetNeedsReset();
		}
		InternalUpdateCursorClip();
	}
}

/**
 * Marks the application window as either minimized or
 * not.
 *
 * If the window becomes minimized while in full-screen mode,
 * this function will also mark the graphics device as needing
 * to be reset.
 */
void D3D11DeviceWindow::PCEngineFriend_Minimized(Bool bMinimized)
{
	SEOUL_ASSERT(IsRenderThread());

	if (bMinimized != m_bMinimized)
	{
		m_bMinimized = bMinimized;
		if (!IsWindowed() && !m_GraphicsParameters.m_bWindowedFullscreen)
		{
			SetNeedsReset();
		}
		InternalUpdateCursorClip();
	}
}

void D3D11DeviceWindow::PCEngineFriend_OnLivePreviewBitmap()
{
#if !SEOUL_SHIP
	if (m_pThumbnailUtil.IsValid())
	{
		m_pThumbnailUtil->OnLivePreviewBitmap(m_vOsWindowRegions);
	}
#endif // /!SEOUL_SHIP
}

void D3D11DeviceWindow::PCEngineFriend_OnLiveThumbnail(UInt32 uWidth, UInt32 uHeight)
{
#if !SEOUL_SHIP
	if (m_pThumbnailUtil.IsValid())
	{
		m_pThumbnailUtil->OnLiveThumbnail(m_vOsWindowRegions, uWidth, uHeight);
	}
#endif // /!SEOUL_SHIP
}

D3DDeviceEntry GetD3D11DeviceWindowEntry()
{
	D3DDeviceEntry ret;
	ret.m_pCreateD3DDevice = D3D11DeviceWindow::CreateDeviceD3D11;
	ret.m_pIsSupported = D3D11DeviceWindow::IsSupportedD3D11;
	return ret;
}

} // namespace Seoul
