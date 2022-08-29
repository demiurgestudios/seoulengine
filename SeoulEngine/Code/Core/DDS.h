/**
 * \file DDS.h
 * \brief Structures and helper functions for reading DDS (Direct Draw Surface) image
 * files, used as the runtime texture format for OGLES2.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DDS_H
#define DDS_H

#include "PixelFormat.h"
#include "Prereqs.h"

namespace Seoul
{

/** Magic value used as a header - little endian ascii for "DDS ". */
static const UInt32 kDdsMagicValue = 0x20534444;
static const UInt32 kDdsFourCc = 0x00000004; // DDPF_FOURCC
static const UInt32 kDdsRgb = 0x00000040; // DDPF_RGB
static const UInt32 kDdsRgba = 0x00000041; // DDPF_RGB | DDPF_ALPHAPIXELS
static const UInt32 kDdsLuminance = 0x00020000; // DDPF_LUMINANCE
static const UInt32 kDdsAlpha = 0x00000002; // DDPF_ALPHA

static const UInt32 kDdsHeaderFlagsTexture = 0x00001007; // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
static const UInt32 kDdsHeaderFlagsMipmap = 0x00020000; // DDSD_MIPMAPCOUNT
static const UInt32 kDdsHeaderFlagsVolume = 0x00800000; // DDSD_DEPTH
static const UInt32 kDdsHeaderFlagsPitch = 0x00000008; // DDSD_PITCH
static const UInt32 kDdsHeaderFlagsLinearSize = 0x00080000; // DDSD_LINEARSIZE

static const UInt32 kDdsSurfaceFlagsTexture = 0x00001000; // DDSCAPS_TEXTURE
static const UInt32 kDdsSurfaceFlagsMipMap = 0x00400008; // DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
static const UInt32 kDdsSurfaceFlagsCubeMap = 0x00000008; // DDSCAPS_COMPLEX

static const UInt32 kDdsCubeMapPositiveX = 0x00000600; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
static const UInt32 kDdsCubeMapNegativeX = 0x00000a00; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
static const UInt32 kDdsCubeMapPositiveY = 0x00001200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
static const UInt32 kDdsCubeMapNegativeY = 0x00002200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
static const UInt32 kDdsCubeMapPositiveZ = 0x00004200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
static const UInt32 kDdsCubeMapNegativeZ = 0x00008200; // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

static const UInt32 kDdsCubeMapAllFaces = (
	kDdsCubeMapPositiveX | kDdsCubeMapNegativeX |
	kDdsCubeMapPositiveY | kDdsCubeMapNegativeY |
	kDdsCubeMapPositiveZ | kDdsCubeMapNegativeZ);

static const UInt32 kDdsFlagsVolume = 0x00200000; // DDSCAPS2_VOLUME

SEOUL_DEFINE_PACKED_STRUCT(
	struct DdsPixelFormat
	{
		UInt32 m_Size;
		UInt32 m_Flags;
		UInt32 m_FourCC;
		UInt32 m_RGBBitCount;
		UInt32 m_RBitMask;
		UInt32 m_GBitMask;
		UInt32 m_BBitMask;
		UInt32 m_ABitMask;
	});

	inline Bool operator==(const DdsPixelFormat& a, const DdsPixelFormat& b)
	{
		return (0 == memcmp(&a, &b, sizeof(a)));
	}

	inline Bool operator!=(const DdsPixelFormat& a, const DdsPixelFormat& b)
	{
		return !(a == b);
	}

#define SEOUL_MAKEFOURCC(ch0, ch1, ch2, ch3) \
	((UInt32)(UInt8)(ch0) | ((UInt32)(UInt8)(ch1) << 8) |   \
	((UInt32)(UInt8)(ch2) << 16) | ((UInt32)(UInt8)(ch3) << 24 ))

static const DdsPixelFormat kDdsPixelFormatDXT1 = { sizeof(DdsPixelFormat), kDdsFourCc, SEOUL_MAKEFOURCC('D','X','T','1'), 0, 0, 0, 0, 0 };
static const DdsPixelFormat kDdsPixelFormatDXT2 = { sizeof(DdsPixelFormat), kDdsFourCc, SEOUL_MAKEFOURCC('D','X','T','2'), 0, 0, 0, 0, 0 };
static const DdsPixelFormat kDdsPixelFormatDXT3 = { sizeof(DdsPixelFormat), kDdsFourCc, SEOUL_MAKEFOURCC('D','X','T','3'), 0, 0, 0, 0, 0 };
static const DdsPixelFormat kDdsPixelFormatDXT4 = { sizeof(DdsPixelFormat), kDdsFourCc, SEOUL_MAKEFOURCC('D','X','T','4'), 0, 0, 0, 0, 0 };
static const DdsPixelFormat kDdsPixelFormatDXT5 = { sizeof(DdsPixelFormat), kDdsFourCc, SEOUL_MAKEFOURCC('D','X','T','5'), 0, 0, 0, 0, 0 };
static const DdsPixelFormat kDdsPixelFormatPVRTC_RGBA_2BPPV1 = { sizeof(DdsPixelFormat), kDdsFourCc, SEOUL_MAKEFOURCC('P','T','C','2'), 0, 0, 0, 0, 0 };
static const DdsPixelFormat kDdsPixelFormatPVRTC_RGBA_4BPPV1 = { sizeof(DdsPixelFormat), kDdsFourCc, SEOUL_MAKEFOURCC('P','T','C','4'), 0, 0, 0, 0, 0 };
static const DdsPixelFormat kDdsPixelFormatETC1_RGB8 = { sizeof(DdsPixelFormat), kDdsFourCc, SEOUL_MAKEFOURCC('E','T','C','1'), 0, 0, 0, 0, 0 };
static const DdsPixelFormat kDdsPixelFormatA8R8G8B8 = { sizeof(DdsPixelFormat), kDdsRgba, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };
static const DdsPixelFormat kDdsPixelFormatA8B8G8R8 = { sizeof(DdsPixelFormat), kDdsRgba, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 };
static const DdsPixelFormat kDdsPixelFormatA1R5G5B5 = { sizeof(DdsPixelFormat), kDdsRgba, 0, 16, 0x00007c00, 0x000003e0, 0x0000001f, 0x00008000 };
static const DdsPixelFormat kDdsPixelFormatA4R4G4B4 = { sizeof(DdsPixelFormat), kDdsRgba, 0, 16, 0x00000f00, 0x000000f0, 0x0000000f, 0x0000f000 };
static const DdsPixelFormat kDdsPixelFormatR8G8B8 = { sizeof(DdsPixelFormat), kDdsRgb, 0, 24, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 };
static const DdsPixelFormat kDdsPixelFormatR5G6B5 = { sizeof(DdsPixelFormat), kDdsRgb, 0, 16, 0x0000f800, 0x000007e0, 0x0000001f, 0x00000000 };

// Special PixelFormat code that indicates that the DDS is a DX10 format
// DDS and there is additional data in the Dds10 header.
static const DdsPixelFormat kDdsPixelFormatDX10 = { sizeof(DdsPixelFormat), kDdsFourCc, SEOUL_MAKEFOURCC('D','X','1','0'), 0, 0, 0, 0, 0 };

#undef SEOUL_MAKEFOURCC

SEOUL_DEFINE_PACKED_STRUCT(
	struct DdsHeader
	{
		UInt32 m_MagicNumber;
		UInt32 m_Size;
		UInt32 m_HeaderFlags;
		UInt32 m_Height;
		UInt32 m_Width;
		UInt32 m_PitchOrLinearSize;
		UInt32 m_Depth; // only if DDS_HEADER_FLAGS_VOLUME is set in m_HeaderFlags
		UInt32 m_MipMapCount;
		UInt32 m_Reserved1[11];
		DdsPixelFormat m_PixelFormat;
		UInt32 m_SurfaceFlags;
		UInt32 m_CubemapFlags;
		UInt32 m_Reserved2[3];
	});

/**
 * DxgiFormat codes, used if a DX10 style header is present.
 */
enum class DxgiFormat : UInt32
{
	kUNKNOWN                     = 0,
	kR32G32B32A32_TYPELESS       = 1,
	kR32G32B32A32_FLOAT          = 2,
	kR32G32B32A32_UINT           = 3,
	kR32G32B32A32_SINT           = 4,
	kR32G32B32_TYPELESS          = 5,
	kR32G32B32_FLOAT             = 6,
	kR32G32B32_UINT              = 7,
	kR32G32B32_SINT              = 8,
	kR16G16B16A16_TYPELESS       = 9,
	kR16G16B16A16_FLOAT          = 10,
	kR16G16B16A16_UNORM          = 11,
	kR16G16B16A16_UINT           = 12,
	kR16G16B16A16_SNORM          = 13,
	kR16G16B16A16_SINT           = 14,
	kR32G32_TYPELESS             = 15,
	kR32G32_FLOAT                = 16,
	kR32G32_UINT                 = 17,
	kR32G32_SINT                 = 18,
	kR32G8X24_TYPELESS           = 19,
	kD32_FLOAT_S8X24_UINT        = 20,
	kR32_FLOAT_X8X24_TYPELESS    = 21,
	kX32_TYPELESS_G8X24_UINT     = 22,
	kR10G10B10A2_TYPELESS        = 23,
	kR10G10B10A2_UNORM           = 24,
	kR10G10B10A2_UINT            = 25,
	kR11G11B10_FLOAT             = 26,
	kR8G8B8A8_TYPELESS           = 27,
	kR8G8B8A8_UNORM              = 28,
	kR8G8B8A8_UNORM_SRGB         = 29,
	kR8G8B8A8_UINT               = 30,
	kR8G8B8A8_SNORM              = 31,
	kR8G8B8A8_SINT               = 32,
	kR16G16_TYPELESS             = 33,
	kR16G16_FLOAT                = 34,
	kR16G16_UNORM                = 35,
	kR16G16_UINT                 = 36,
	kR16G16_SNORM                = 37,
	kR16G16_SINT                 = 38,
	kR32_TYPELESS                = 39,
	kD32_FLOAT                   = 40,
	kR32_FLOAT                   = 41,
	kR32_UINT                    = 42,
	kR32_SINT                    = 43,
	kR24G8_TYPELESS              = 44,
	kD24_UNORM_S8_UINT           = 45,
	kR24_UNORM_X8_TYPELESS       = 46,
	kX24_TYPELESS_G8_UINT        = 47,
	kR8G8_TYPELESS               = 48,
	kR8G8_UNORM                  = 49,
	kR8G8_UINT                   = 50,
	kR8G8_SNORM                  = 51,
	kR8G8_SINT                   = 52,
	kR16_TYPELESS                = 53,
	kR16_FLOAT                   = 54,
	kD16_UNORM                   = 55,
	kR16_UNORM                   = 56,
	kR16_UINT                    = 57,
	kR16_SNORM                   = 58,
	kR16_SINT                    = 59,
	kR8_TYPELESS                 = 60,
	kR8_UNORM                    = 61,
	kR8_UINT                     = 62,
	kR8_SNORM                    = 63,
	kR8_SINT                     = 64,
	kA8_UNORM                    = 65,
	kR1_UNORM                    = 66,
	kR9G9B9E5_SHAREDEXP          = 67,
	kR8G8_B8G8_UNORM             = 68,
	kG8R8_G8B8_UNORM             = 69,
	kBC1_TYPELESS                = 70,
	kBC1_UNORM                   = 71,
	kBC1_UNORM_SRGB              = 72,
	kBC2_TYPELESS                = 73,
	kBC2_UNORM                   = 74,
	kBC2_UNORM_SRGB              = 75,
	kBC3_TYPELESS                = 76,
	kBC3_UNORM                   = 77,
	kBC3_UNORM_SRGB              = 78,
	kBC4_TYPELESS                = 79,
	kBC4_UNORM                   = 80,
	kBC4_SNORM                   = 81,
	kBC5_TYPELESS                = 82,
	kBC5_UNORM                   = 83,
	kBC5_SNORM                   = 84,
	kB5G6R5_UNORM                = 85,
	kB5G5R5A1_UNORM              = 86,
	kB8G8R8A8_UNORM              = 87,
	kB8G8R8X8_UNORM              = 88,
	kR10G10B10_XR_BIAS_A2_UNORM  = 89,
	kB8G8R8A8_TYPELESS           = 90,
	kB8G8R8A8_UNORM_SRGB         = 91,
	kB8G8R8X8_TYPELESS           = 92,
	kB8G8R8X8_UNORM_SRGB         = 93,
	kBC6H_TYPELESS               = 94,
	kBC6H_UF16                   = 95,
	kBC6H_SF16                   = 96,
	kBC7_TYPELESS                = 97,
	kBC7_UNORM                   = 98,
	kBC7_UNORM_SRGB              = 99,
};

/**
 * Dimensionality codes used if a DX10 style header is present in the DDS.
 */
enum class D3d11ResourceDimension
{
	kUnknown   = 0,
	kBuffer    = 1,
	kTexture1D = 2,
	kTexture2D = 3,
	kTexture3D = 4
};

	/** Extended DX10 header that may or may not be present in the DDS. */
SEOUL_DEFINE_PACKED_STRUCT(
	struct DdsHeaderDX10
	{
		UInt32 m_DxgiFormat;
		UInt32 m_ResourceDimension;
		UInt32 m_MiscFlag;
		UInt32 m_ArraySize;
		UInt32 m_Reserved;
	});

namespace DDS
{

// Extract all relevant data from the DDS file. On success,
// rpOut (and optionally rpOutSecondary) point(s) *inside* pData and *must not* be deallocated
// separately.
Bool Decode(
	void const* pData,
	UInt32 uSizeInBytes,
	UInt32& ruWidth,
	UInt32& ruHeight,
	PixelFormat& reFormat,
	void const*& rpOut,
	void const*& rpOutSecondary);

// Read the PixelFormat from the DDS header data contained in the given stream.
// Can fail if the stream is invalid.
Bool ReadPixelFormat(void const* pData, UInt32 uSizeInBytes, PixelFormat& reFormat);

// Specialized utility - given a DDS in an uncompressed BGRA8888 or RGBA8888 format,
// swaps the RB channels and updates the data's header.
Bool SwapChannelsRB(void* pData, UInt32 uSizeInBytes);

// Convert a SeoulEngine PixelFormat to a DdsPixelFormat.
DdsPixelFormat ToDdsPixelFormat(PixelFormat eFormat);

// Return a SeoulEngine PixelFormat value corresponding to the pixel format
// defined in pixelFormat from a DDS file.
PixelFormat ToPixelFormat(const DdsHeader& header, const DdsHeaderDX10& headerDX10);

// Given a DDS fourcc, return a SeoulEngine PixelFormat.
PixelFormat ToPixelFormat(UInt32 uFourCC);

} // namespace DDS

} // namespace Seoul

#endif // include guard
