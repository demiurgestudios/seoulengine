/**
 * \file D3DUtil.h
 * \brief Wrapper around d3dcompiler.h functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef D3D_UTIL_H
#define D3D_UTIL_H

#include <d3dcompiler.h>

/* Originally from d3d9types.h. */
typedef enum _D3DSHADEMODE {
    D3DSHADE_FLAT = 1,
    D3DSHADE_GOURAUD = 2,
    D3DSHADE_PHONG = 3,
    D3DSHADE_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */
} D3DSHADEMODE;

typedef enum _D3DFILLMODE {
    D3DFILL_POINT = 1,
    D3DFILL_WIREFRAME = 2,
    D3DFILL_SOLID = 3,
    D3DFILL_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */
} D3DFILLMODE;

typedef enum _D3DBLEND {
    D3DBLEND_ZERO = 1,
    D3DBLEND_ONE = 2,
    D3DBLEND_SRCCOLOR = 3,
    D3DBLEND_INVSRCCOLOR = 4,
    D3DBLEND_SRCALPHA = 5,
    D3DBLEND_INVSRCALPHA = 6,
    D3DBLEND_DESTALPHA = 7,
    D3DBLEND_INVDESTALPHA = 8,
    D3DBLEND_DESTCOLOR = 9,
    D3DBLEND_INVDESTCOLOR = 10,
    D3DBLEND_SRCALPHASAT = 11,
    D3DBLEND_BOTHSRCALPHA = 12,
    D3DBLEND_BOTHINVSRCALPHA = 13,
    D3DBLEND_BLENDFACTOR = 14, /* Only supported if D3DPBLENDCAPS_BLENDFACTOR is on */
    D3DBLEND_INVBLENDFACTOR = 15, /* Only supported if D3DPBLENDCAPS_BLENDFACTOR is on */
    D3DBLEND_SRCCOLOR2 = 16,
    D3DBLEND_INVSRCCOLOR2 = 17,
    D3DBLEND_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */
} D3DBLEND;

typedef enum _D3DBLENDOP {
    D3DBLENDOP_ADD = 1,
    D3DBLENDOP_SUBTRACT = 2,
    D3DBLENDOP_REVSUBTRACT = 3,
    D3DBLENDOP_MIN = 4,
    D3DBLENDOP_MAX = 5,
    D3DBLENDOP_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */
} D3DBLENDOP;

typedef enum _D3DCULL {
    D3DCULL_NONE = 1,
    D3DCULL_CW = 2,
    D3DCULL_CCW = 3,
    D3DCULL_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */
} D3DCULL;

typedef enum _D3DCMPFUNC {
    D3DCMP_NEVER = 1,
    D3DCMP_LESS = 2,
    D3DCMP_EQUAL = 3,
    D3DCMP_LESSEQUAL = 4,
    D3DCMP_GREATER = 5,
    D3DCMP_NOTEQUAL = 6,
    D3DCMP_GREATEREQUAL = 7,
    D3DCMP_ALWAYS = 8,
    D3DCMP_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */
} D3DCMPFUNC;

typedef enum _D3DSTENCILOP {
    D3DSTENCILOP_KEEP = 1,
    D3DSTENCILOP_ZERO = 2,
    D3DSTENCILOP_REPLACE = 3,
    D3DSTENCILOP_INCRSAT = 4,
    D3DSTENCILOP_DECRSAT = 5,
    D3DSTENCILOP_INVERT = 6,
    D3DSTENCILOP_INCR = 7,
    D3DSTENCILOP_DECR = 8,
    D3DSTENCILOP_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */
} D3DSTENCILOP;

#define D3DCOLORWRITEENABLE_RED     (1L<<0)
#define D3DCOLORWRITEENABLE_GREEN   (1L<<1)
#define D3DCOLORWRITEENABLE_BLUE    (1L<<2)
#define D3DCOLORWRITEENABLE_ALPHA   (1L<<3)

#endif // include guard
