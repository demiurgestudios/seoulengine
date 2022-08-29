/**
 * \file D3DCommonThumbnailUtil.h
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

#pragma once
#ifndef D3D_COMMON_THUBMNAIL_UTIL_H
#define D3D_COMMON_THUBMNAIL_UTIL_H

#include "Geometry.h"
#include "Platform.h"
#include "Prereqs.h"
#include "Vector.h"
namespace Seoul { struct OsWindowRegion; }

#if !SEOUL_SHIP

extern "C"
{

typedef struct tagCHANGEFILTERSTRUCT {
	DWORD cbSize;
	DWORD ExtStatus;
} CHANGEFILTERSTRUCT, *PCHANGEFILTERSTRUCT;
typedef BOOL(__stdcall *ChangeWindowMessageFilterExPtr)(HWND hwnd, UINT message, DWORD action, PCHANGEFILTERSTRUCT pChangeFilterStruct);
typedef HRESULT(__stdcall *DwmSetWindowAttributePtr)(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
typedef HRESULT(__stdcall *DwmInvalidateIconicBitmapsPtr)(HWND hwnd);
typedef HRESULT(__stdcall *DwmSetIconicLivePreviewBitmapPtr)(HWND hwnd, HBITMAP hbmp, POINT *pptClient, DWORD dwSITFlags);
typedef HRESULT(__stdcall *DwmSetIconicThumbnailPtr)(HWND hwnd, HBITMAP hbmp, DWORD dwSITFlags);

} // extern "C"

namespace Seoul
{

class D3DCommonThumbnailUtil SEOUL_SEALED
{
public:
	typedef Vector<OsWindowRegion, MemoryBudgets::Rendering> OsWindowRegions;

	D3DCommonThumbnailUtil(HWND hwnd);
	~D3DCommonThumbnailUtil();

	void InvalidateCachedBitmaps();
	void OnLivePreviewBitmap(const OsWindowRegions& vRegions);
	void OnLiveThumbnail(const OsWindowRegions& vRegions, UInt32 uDstWidth, UInt32 uDstHeight);

private:
	SEOUL_DISABLE_COPY(D3DCommonThumbnailUtil);

	typedef HRESULT (*Setter)(D3DCommonThumbnailUtil const& r, HWND hwnd, HBITMAP hbm, const Point2DInt& srcOrigin);
	void SetBitmap(const OsWindowRegions& vRegions, UInt32 uDstWidth, UInt32 uDstHeight, Setter setter);

	RECT GetRect(const OsWindowRegions& vRegions) const;
	static RECT Convert(const Rectangle2DInt& rect);
	static RECT Merge(const RECT& a, const Rectangle2DInt& b);

	HWND const m_Hwnd;
	HMODULE m_hUser32;
	ChangeWindowMessageFilterExPtr m_pChangeWindowMessageFilterEx;
	HMODULE m_hDwmapi;
	DwmSetWindowAttributePtr m_pDwmSetWindowAttribute;
	DwmInvalidateIconicBitmapsPtr m_pDwmInvalidateIconicBitmaps;
	DwmSetIconicLivePreviewBitmapPtr m_pDwmSetIconicLivePreviewBitmap;
	DwmSetIconicThumbnailPtr m_pDwmSetIconicThumbnail;

	static HRESULT DwmSetIconicLivePreviewBitmapSetter(D3DCommonThumbnailUtil const& r, HWND hwnd, HBITMAP hbmp, const Point2DInt& srcOrigin);
	static HRESULT DwmSetIconicThumbnailSetter(D3DCommonThumbnailUtil const& r, HWND hwnd, HBITMAP hbmp, const Point2DInt& srcOrigin);

	static inline ChangeWindowMessageFilterExPtr GetChangeWindowMessageFilterEx(HMODULE user32);
	static inline DwmSetWindowAttributePtr GetDwmSetWindowAttribute(HMODULE dwmapi);
	static inline DwmInvalidateIconicBitmapsPtr GetDwmInvalidateIconicBitmaps(HMODULE dwmapi);
	static inline DwmSetIconicLivePreviewBitmapPtr GetDwmSetIconicLivePreviewBitmap(HMODULE dwmapi);
	static inline DwmSetIconicThumbnailPtr GetDwmSetIconicThumbnail(HMODULE dwmapi);
};

} // namespace Seoul

#endif // /!SEOUL_SHIP

#endif // include guard
