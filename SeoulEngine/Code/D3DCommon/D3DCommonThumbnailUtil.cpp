/**
 * \file D3DCommonThumbnailUtil.cpp
 * \brief Utility used to override the live preview/snapshot thumbnail
 * displayed in ALT+TAB/taskbar contexts under DWM (Desktop Window Manager).
 *
 * Starting with Windows 7. Currently developer only, as this is only
 * needed when "virtualized desktop" is available.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "D3DCommonThumbnailUtil.h"
#include "Logger.h"
#include "RenderCommandStreamBuilder.h"
#include "RenderDevice.h"
#include "ScopedAction.h"

#if !SEOUL_SHIP

extern "C"
{
#	ifndef MSGFLT_RESET
#		define MSGFLT_RESET 0
#	endif

#	ifndef MSGFLT_ALLOW
#		define MSGFLT_ALLOW 1
#	endif

#	ifndef MSGFLT_DISALLOW
#		define MSGFLT_DISALLOW 2
#	endif

	// Window attributes
	enum DWMWINDOWATTRIBUTE
	{
		DWMWA_NCRENDERING_ENABLED = 1,      // [get] Is non-client rendering enabled/disabled
		DWMWA_NCRENDERING_POLICY,           // [set] Non-client rendering policy
		DWMWA_TRANSITIONS_FORCEDISABLED,    // [set] Potentially enable/forcibly disable transitions
		DWMWA_ALLOW_NCPAINT,                // [set] Allow contents rendered in the non-client area to be visible on the DWM-drawn frame.
		DWMWA_CAPTION_BUTTON_BOUNDS,        // [get] Bounds of the caption button area in window-relative space.
		DWMWA_NONCLIENT_RTL_LAYOUT,         // [set] Is non-client content RTL mirrored
		DWMWA_FORCE_ICONIC_REPRESENTATION,  // [set] Force this window to display iconic thumbnails.
		DWMWA_FLIP3D_POLICY,                // [set] Designates how Flip3D will treat the window.
		DWMWA_EXTENDED_FRAME_BOUNDS,        // [get] Gets the extended frame bounds rectangle in screen space
		DWMWA_HAS_ICONIC_BITMAP,            // [set] Indicates an available bitmap when there is no better thumbnail representation.
		DWMWA_DISALLOW_PEEK,                // [set] Don't invoke Peek on the window.
		DWMWA_EXCLUDED_FROM_PEEK,           // [set] LivePreview exclusion information
		DWMWA_CLOAK,                        // [set] Cloak or uncloak the window
		DWMWA_CLOAKED,                      // [get] Gets the cloaked state of the window
		DWMWA_FREEZE_REPRESENTATION,        // [set] Force this window to freeze the thumbnail without live update
		DWMWA_LAST
	};

#	ifndef SRCCOPY
#		define SRCCOPY (DWORD)0x00CC0020 /* dest = source */
#	endif

#	ifndef DWM_SIT_DISPLAYFRAME
#		define DWM_SIT_DISPLAYFRAME (0x00000001)
#	endif
}

namespace Seoul
{

D3DCommonThumbnailUtil::D3DCommonThumbnailUtil(HWND hwnd)
	: m_Hwnd(hwnd)
	, m_hUser32(::LoadLibraryW(L"User32.dll"))
	, m_pChangeWindowMessageFilterEx(GetChangeWindowMessageFilterEx(m_hUser32))
	, m_hDwmapi(::LoadLibraryW(L"Dwmapi.dll"))
	, m_pDwmSetWindowAttribute(GetDwmSetWindowAttribute(m_hDwmapi))
	, m_pDwmInvalidateIconicBitmaps(GetDwmInvalidateIconicBitmaps(m_hDwmapi))
	, m_pDwmSetIconicLivePreviewBitmap(GetDwmSetIconicLivePreviewBitmap(m_hDwmapi))
	, m_pDwmSetIconicThumbnail(GetDwmSetIconicThumbnail(m_hDwmapi))
{
	// Adding handling for thumbnail messages.
	if (nullptr != m_pChangeWindowMessageFilterEx)
	{
		SEOUL_VERIFY(FALSE != m_pChangeWindowMessageFilterEx(m_Hwnd, WM_DWMSENDICONICLIVEPREVIEWBITMAP, MSGFLT_ALLOW, nullptr));
		SEOUL_VERIFY(FALSE != m_pChangeWindowMessageFilterEx(m_Hwnd, WM_DWMSENDICONICTHUMBNAIL, MSGFLT_ALLOW, nullptr));
	}

	// Static only.
	if (nullptr != m_pDwmSetWindowAttribute)
	{
		BOOL const val = TRUE;
		SEOUL_VERIFY(SUCCEEDED(m_pDwmSetWindowAttribute(m_Hwnd, DWMWA_FORCE_ICONIC_REPRESENTATION, &val, sizeof(val))));
		SEOUL_VERIFY(SUCCEEDED(m_pDwmSetWindowAttribute(m_Hwnd, DWMWA_HAS_ICONIC_BITMAP, &val, sizeof(val))));
	}
}

D3DCommonThumbnailUtil::~D3DCommonThumbnailUtil()
{
	m_pDwmSetIconicThumbnail = nullptr;
	m_pDwmSetIconicLivePreviewBitmap = nullptr;
	m_pDwmInvalidateIconicBitmaps = nullptr;
	m_pDwmSetWindowAttribute = nullptr;
	m_pChangeWindowMessageFilterEx = nullptr;
	if (nullptr != m_hUser32)
	{
		SEOUL_VERIFY(FALSE != ::FreeLibrary(m_hUser32));
		m_hUser32 = nullptr;
	}
}

void D3DCommonThumbnailUtil::InvalidateCachedBitmaps()
{
	SEOUL_ASSERT(IsRenderThread());

	if (nullptr == m_pDwmInvalidateIconicBitmaps)
	{
		return;
	}

	m_pDwmInvalidateIconicBitmaps(m_Hwnd);
}

void D3DCommonThumbnailUtil::OnLivePreviewBitmap(const OsWindowRegions& vRegions)
{
	SEOUL_ASSERT(IsRenderThread());

	if (nullptr == m_pDwmSetIconicLivePreviewBitmap)
	{
		return;
	}

	// Perform the set.
	SetBitmap(vRegions, 0u, 0u, DwmSetIconicLivePreviewBitmapSetter);
}

void D3DCommonThumbnailUtil::OnLiveThumbnail(const OsWindowRegions& vRegions, UInt32 uDstWidth, UInt32 uDstHeight)
{
	SEOUL_ASSERT(IsRenderThread());

	if (nullptr == m_pDwmSetIconicThumbnail)
	{
		return;
	}

	// Perform the set.
	SetBitmap(vRegions, uDstWidth, uDstHeight, DwmSetIconicThumbnailSetter);
}

void D3DCommonThumbnailUtil::SetBitmap(const OsWindowRegions& vRegions, UInt32 uDstWidth, UInt32 uDstHeight, Setter setter)
{
	// Settings.
	auto const capture = GetRect(vRegions);
	auto const srcWidth = (int)(capture.right - capture.left);
	auto const srcHeight = (int)(capture.bottom - capture.top);

	// Handling.
	if (0u == uDstWidth) { uDstWidth = (UInt32)srcWidth; }
	if (0u == uDstHeight) { uDstHeight = (UInt32)srcHeight; }

	// Early out on insanity.
	if (0 == uDstWidth || 0 == uDstHeight ||
		0 == srcWidth || 0 == srcHeight)
	{
		return;
	}

	// Access.
	auto const hdcWindow = ::GetDC(m_Hwnd);
	auto const deferWindow = MakeDeferredAction([&]() { ::ReleaseDC(m_Hwnd, hdcWindow); });

	auto const hdc = ::CreateCompatibleDC(hdcWindow);
	auto const deferHdc = MakeDeferredAction([&]() { ::DeleteObject(hdc); });

	// Bitmap for output.
	BITMAPINFO info;
	memset(&info, 0, sizeof(info));
	auto& hdr = info.bmiHeader;
	hdr.biSize = sizeof(hdr);
	hdr.biWidth = uDstWidth;
	hdr.biHeight = -((Int)uDstHeight); // top-down
	hdr.biPlanes = 1u;
	hdr.biBitCount = 32u;

	// Need to initialize the bits prior to copy unless we're filling
	// the entire bitmap.
	void* pBits = nullptr;
	auto const hbmp = ::CreateDIBSection(
		hdc,
		&info,
		DIB_RGB_COLORS,
		&pBits,
		nullptr,
		0u);
	auto const deferHbmp = MakeDeferredAction([&]() { ::DeleteObject(hbmp); });

	// Prep to blt.
	::SelectObject(hdc, hbmp);

	// Perform the blt.
	if (uDstWidth == (UInt32)srcWidth && uDstHeight == (UInt32)srcHeight)
	{
		SEOUL_VERIFY(FALSE != BitBlt(
			hdc,
			0, 0, uDstWidth, uDstHeight,
			hdcWindow,
			capture.left, capture.top,
			SRCCOPY));
	}
	else
	{
		Int iDstX = 0;
		Int iDstY = 0;
		Int iDstH = (Int)uDstHeight;
		Int iDstW = (Int)uDstWidth;
		if (iDstH != srcHeight || iDstW != srcWidth)
		{
			auto const iNewHeight = (Int)Ceil(((Float)((Int)iDstW) / (Float)srcWidth) * (Float)srcHeight);
			if (iNewHeight > 0 && iNewHeight <= iDstH)
			{
				// Clear to zero, since we won't be filling the entire bmp.
				if (nullptr != pBits && iNewHeight < iDstH)
				{
					memset(pBits, 0, 4 * uDstWidth * uDstHeight);
				}

				iDstY = (iDstH - iNewHeight) / 2;
				iDstH = iNewHeight;
			}
			else
			{
				auto const iNewWidth = (Int)Ceil(((Float)((Int)iDstH) / (Float)srcHeight) * (Float)srcWidth);
				if (iNewWidth > 0 && iNewWidth <= iDstW)
				{
					// Clear to zero, since we won't be filling the entire bmp.
					if (nullptr != pBits && iNewWidth < iDstW)
					{
						memset(pBits, 0, 4 * uDstWidth * uDstHeight);
					}

					iDstX = (iDstW - iNewWidth) / 2;
					iDstW = iNewWidth;
				}
			}
		}

		auto const prev = ::SetStretchBltMode(hdc, HALFTONE);
		auto const deferBltMode = MakeDeferredAction([&]() { ::SetStretchBltMode(hdc, prev); });
		SEOUL_VERIFY(FALSE != StretchBlt(
			hdc,
			iDstX, iDstY, iDstW, iDstH,
			hdcWindow,
			capture.left, capture.top, srcWidth, srcHeight,
			SRCCOPY));
	}

	// Commit.
	auto const hr = setter(*this, m_Hwnd, hbmp, Point2DInt(capture.left, capture.top));
	if (!SUCCEEDED(hr))
	{
		SEOUL_LOG_RENDER("D3DCommonThumbnailUtil::SetBitmap failed: %d", hr);
	}
}

RECT D3DCommonThumbnailUtil::GetRect(const OsWindowRegions& vRegions) const
{
	RECT capture;
	memset(&capture, 0, sizeof(capture));
	if (vRegions.IsEmpty())
	{
		SEOUL_VERIFY(FALSE != ::GetClientRect(m_Hwnd, &capture));
	}
	else
	{
		capture = Convert(vRegions[0].m_Rect);
		for (UInt32 i = 1u; i < vRegions.GetSize(); ++i)
		{
			// If we find a main form, just use that immediately.
			if (vRegions[i].m_bMainForm)
			{
				capture = Convert(vRegions[i].m_Rect);
				break;
			}

			capture = Merge(capture, vRegions[i].m_Rect);
		}
	}

	return capture;
}

RECT D3DCommonThumbnailUtil::Convert(const Rectangle2DInt& rect)
{
	RECT ret;
	ret.bottom = rect.m_iBottom;
	ret.left = rect.m_iLeft;
	ret.right = rect.m_iRight;
	ret.top = rect.m_iTop;
	return ret;
}

RECT D3DCommonThumbnailUtil::Merge(const RECT& a, const Rectangle2DInt& b)
{
	RECT ret;
	ret.bottom = Max(a.bottom, (LONG)b.m_iBottom);
	ret.left = Min(a.left, (LONG)b.m_iLeft);
	ret.right = Max(a.right, (LONG)b.m_iRight);
	ret.top = Min(a.top, (LONG)b.m_iTop);
	return ret;
}

HRESULT D3DCommonThumbnailUtil::DwmSetIconicLivePreviewBitmapSetter(D3DCommonThumbnailUtil const& r, HWND hwnd, HBITMAP hbmp, const Point2DInt& srcOrigin)
{
	SEOUL_ASSERT(nullptr != r.m_pDwmSetIconicLivePreviewBitmap);
	auto const uOptions = (RenderDevice::Get()->IsVirtualizedDesktop() ? 0u : DWM_SIT_DISPLAYFRAME);
	POINT org;
	org.x = srcOrigin.X;
	org.y = srcOrigin.Y;
	return r.m_pDwmSetIconicLivePreviewBitmap(hwnd, hbmp, &org, uOptions);
}

HRESULT D3DCommonThumbnailUtil::DwmSetIconicThumbnailSetter(D3DCommonThumbnailUtil const& r, HWND hwnd, HBITMAP hbmp, const Point2DInt& srcOrigin)
{
	SEOUL_ASSERT(nullptr != r.m_pDwmSetIconicThumbnail);
	return r.m_pDwmSetIconicThumbnail(hwnd, hbmp, 0u); // DWM_SIT_DISPLAYFRAME always looks incorrect in the icon thumbnail.
}

ChangeWindowMessageFilterExPtr D3DCommonThumbnailUtil::GetChangeWindowMessageFilterEx(HMODULE user32)
{
	return (ChangeWindowMessageFilterExPtr)::GetProcAddress(user32, "ChangeWindowMessageFilterEx");
}

DwmSetWindowAttributePtr D3DCommonThumbnailUtil::GetDwmSetWindowAttribute(HMODULE dwmapi)
{
	return (DwmSetWindowAttributePtr)::GetProcAddress(dwmapi, "DwmSetWindowAttribute");
}

DwmInvalidateIconicBitmapsPtr D3DCommonThumbnailUtil::GetDwmInvalidateIconicBitmaps(HMODULE dwmapi)
{
	return (DwmInvalidateIconicBitmapsPtr)::GetProcAddress(dwmapi, "DwmInvalidateIconicBitmaps");
}

DwmSetIconicLivePreviewBitmapPtr D3DCommonThumbnailUtil::GetDwmSetIconicLivePreviewBitmap(HMODULE dwmapi)
{
	return (DwmSetIconicLivePreviewBitmapPtr)::GetProcAddress(dwmapi, "DwmSetIconicLivePreviewBitmap");
}

DwmSetIconicThumbnailPtr D3DCommonThumbnailUtil::GetDwmSetIconicThumbnail(HMODULE dwmapi)
{
	return (DwmSetIconicThumbnailPtr)::GetProcAddress(dwmapi, "DwmSetIconicThumbnail");
}

} // namespace Seoul

#endif // /!SEOUL_SHIP
