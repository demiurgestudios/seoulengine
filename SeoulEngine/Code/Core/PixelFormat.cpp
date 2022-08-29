/**
 * \file PixelFormat.cpp
 * \brief Enum defining valid pixel formats for color targets and textures.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "PixelFormat.h"
#include "SeoulHString.h"
#include "SeoulMath.h"

namespace Seoul
{

/**
 * Convert a string representing a PixelFormat to an equivalent enum
 * value.
 */
PixelFormat HStringToPixelFormat(HString hstring)
{
	static const HString kR8G8B8("R8G8B8");
	static const HString kA8R8G8B8("A8R8G8B8");
	static const HString kA8R8G8B8sRGB("A8R8G8B8sRGB");
	static const HString kX8R8G8B8("X8R8G8B8");
	static const HString kR5G6B5("R5G6B5");
	static const HString kX1R5G5B5("X1R5G5B5");
	static const HString kA1R5G5B5("A1R5G5B5");
	static const HString kA4R4G4B4("A4R4G4B4");
	static const HString kR3G3B2("R3G3B2");
	static const HString kA8("A8");
	static const HString kA8L8("A8L8");
	static const HString kP8("P8");
	static const HString kX4R4G4B4("X4R4G4B4");
	static const HString kA2B10G10R10("A2B10G10R10");
	static const HString kA8B8G8R8("A8B8G8R8");
	static const HString kX8B8G8R8("X8B8G8R8");
	static const HString kG16R16("G16R16");
	static const HString kA2R10G10B10("A2R10G10B10");
	static const HString kA2B10G10R10F("A2B10G10R10F");
	static const HString kA16B16G16R16("A16B16G16R16");
	static const HString kA16B16G16R16_SIGNED("A16B16G16R16_SIGNED");
	static const HString kA16B16G16R16_SIGNED_EXP1("A16B16G16R16_SIGNED_EXP1");
	static const HString kA16B16G16R16_UNSIGNED_EXP1("A16B16G16R16_UNSIGNED_EXP1");
	static const HString kA16B16G16R16_SIGNED_EXP2("A16B16G16R16_SIGNED_EXP2");
	static const HString kA16B16G16R16_UNSIGNED_EXP2("A16B16G16R16_UNSIGNED_EXP2");
	static const HString kA16B16G16R16_SIGNED_EXP3("A16B16G16R16_SIGNED_EXP3");
	static const HString kA16B16G16R16_UNSIGNED_EXP3("A16B16G16R16_UNSIGNED_EXP3");
	static const HString kR16F("R16F");
	static const HString kD16I("D16I");
	static const HString kG16R16F("G16R16F");
	static const HString kA16B16G16R16F("A16B16G16R16F");
	static const HString kR32F("R32F");
	static const HString kR32F_AS_R16F_EXPAND("R32F_AS_R16F_EXPAND");
	static const HString kG32R32F("G32R32F");
	static const HString kG32R32F_AS_G16R16("G32R32F_AS_G16R16");
	static const HString kA32B32G32R32F("A32B32G32R32F");
	static const HString kA32B32G32R32("A32B32G32R32");
	static const HString kDXT1("DXT1");
	static const HString kDXT2("DXT2");
	static const HString kDXT3("DXT3");
	static const HString kDXT4("DXT4");
	static const HString kDXT5("DXT5");
	static const HString kPVRTC_RGB_2BPPV1("PVRTC_RGB_2BPPV1");
	static const HString kPVRTC_RGB_4BPPV1("PVRTC_RGB_4BPPV1");
	static const HString kPVRTC_RGBA_2BPPV1("PVRTC_RGBA_2BPPV1");
	static const HString kPVRTC_RGBA_4BPPV1("PVRTC_RGBA_4BPPV1");
	static const HString kETC1_RGB8("ETC1_RGB8");

	if (kR8G8B8 == hstring) { return PixelFormat::kR8G8B8; }
	else if (kA8R8G8B8 == hstring) { return PixelFormat::kA8R8G8B8; }
	else if (kA8R8G8B8sRGB == hstring) { return PixelFormat::kA8R8G8B8sRGB; }
	else if (kX8R8G8B8 == hstring) { return PixelFormat::kX8R8G8B8; }
	else if (kR5G6B5 == hstring) { return PixelFormat::kR5G6B5; }
	else if (kX1R5G5B5 == hstring) { return PixelFormat::kX1R5G5B5; }
	else if (kA1R5G5B5 == hstring) { return PixelFormat::kA1R5G5B5; }
	else if (kA4R4G4B4 == hstring) { return PixelFormat::kA4R4G4B4; }
	else if (kR3G3B2 == hstring) { return PixelFormat::kR3G3B2; }
	else if (kA8 == hstring) { return PixelFormat::kA8; }
	else if (kA8L8 == hstring) { return PixelFormat::kA8L8; }
	else if (kP8 == hstring) { return PixelFormat::kP8; }
	else if (kX4R4G4B4 == hstring) { return PixelFormat::kX4R4G4B4; }
	else if (kA2B10G10R10 == hstring) { return PixelFormat::kA2B10G10R10; }
	else if (kA8B8G8R8 == hstring) { return PixelFormat::kA8B8G8R8; }
	else if (kX8B8G8R8 == hstring) { return PixelFormat::kX8B8G8R8; }
	else if (kG16R16 == hstring) { return PixelFormat::kG16R16; }
	else if (kA2R10G10B10 == hstring) { return PixelFormat::kA2R10G10B10; }
	else if (kA2B10G10R10F == hstring) { return PixelFormat::kA2B10G10R10F; }
	else if (kA16B16G16R16 == hstring) { return PixelFormat::kA16B16G16R16; }
	else if (kA16B16G16R16_SIGNED == hstring) { return PixelFormat::kA16B16G16R16_SIGNED; }
	else if (kA16B16G16R16_SIGNED_EXP1 == hstring) { return PixelFormat::kA16B16G16R16_SIGNED_EXP1; }
	else if (kA16B16G16R16_UNSIGNED_EXP1 == hstring) { return PixelFormat::kA16B16G16R16_UNSIGNED_EXP1; }
	else if (kA16B16G16R16_SIGNED_EXP2 == hstring) { return PixelFormat::kA16B16G16R16_SIGNED_EXP2; }
	else if (kA16B16G16R16_UNSIGNED_EXP2 == hstring) { return PixelFormat::kA16B16G16R16_UNSIGNED_EXP2; }
	else if (kA16B16G16R16_SIGNED_EXP3 == hstring) { return PixelFormat::kA16B16G16R16_SIGNED_EXP3; }
	else if (kA16B16G16R16_UNSIGNED_EXP3 == hstring) { return PixelFormat::kA16B16G16R16_UNSIGNED_EXP3; }
	else if (kD16I == hstring) { return PixelFormat::kD16I; }
	else if (kR16F == hstring) { return PixelFormat::kR16F; }
	else if (kG16R16F == hstring) { return PixelFormat::kG16R16F; }
	else if (kA16B16G16R16F == hstring) { return PixelFormat::kA16B16G16R16F; }
	else if (kR32F == hstring) { return PixelFormat::kR32F; }
	else if (kR32F_AS_R16F_EXPAND == hstring) { return PixelFormat::kR32F_AS_R16F_EXPAND; }
	else if (kG32R32F == hstring) { return PixelFormat::kG32R32F; }
	else if (kG32R32F_AS_G16R16 == hstring) { return PixelFormat::kG32R32F_AS_G16R16; }
	else if (kA32B32G32R32F == hstring) { return PixelFormat::kA32B32G32R32F; }
	else if (kA32B32G32R32 == hstring) { return PixelFormat::kA32B32G32R32; }
	else if (kDXT1 == hstring) { return PixelFormat::kDXT1; }
	else if (kDXT2 == hstring) { return PixelFormat::kDXT2; }
	else if (kDXT3 == hstring) { return PixelFormat::kDXT3; }
	else if (kDXT4 == hstring) { return PixelFormat::kDXT4; }
	else if (kDXT5 == hstring) { return PixelFormat::kDXT5; }
	else if (kPVRTC_RGB_2BPPV1 == hstring) { return PixelFormat::kPVRTC_RGB_2BPPV1; }
	else if (kPVRTC_RGB_4BPPV1 == hstring) { return PixelFormat::kPVRTC_RGB_4BPPV1; }
	else if (kPVRTC_RGBA_2BPPV1 == hstring) { return PixelFormat::kPVRTC_RGBA_2BPPV1; }
	else if (kPVRTC_RGBA_4BPPV1 == hstring) { return PixelFormat::kPVRTC_RGBA_4BPPV1; }
	else if (kETC1_RGB8 == hstring) { return PixelFormat::kETC1_RGB8; }
	else
	{
		return PixelFormat::kInvalid;
	}
}

/**
 * Given a pixel format enum value, returns the number of bytes
 * in one pixel of that format.
 */
inline static Int32 InternalStaticPixelFormatBytesPerPixel(PixelFormat eFormat)
{
	switch (eFormat)
	{
	case PixelFormat::kR8G8B8: return 3;
	case PixelFormat::kA8R8G8B8: return 4;
	case PixelFormat::kA8R8G8B8sRGB: return 4;
	case PixelFormat::kX8R8G8B8: return 4;
	case PixelFormat::kR5G6B5: return 2;
	case PixelFormat::kX1R5G5B5: return 2;
	case PixelFormat::kA1R5G5B5: return 2;
	case PixelFormat::kA4R4G4B4: return 2;
	case PixelFormat::kR3G3B2: return 1;
	case PixelFormat::kA8: return 1;
	case PixelFormat::kA8L8: return 2;
	case PixelFormat::kP8: return 1;
	case PixelFormat::kX4R4G4B4: return 2;
	case PixelFormat::kA2B10G10R10: return 4;
	case PixelFormat::kA8B8G8R8: return 4;
	case PixelFormat::kX8B8G8R8: return 4;
	case PixelFormat::kG16R16: return 4;
	case PixelFormat::kA2R10G10B10: return 4;
	case PixelFormat::kA2B10G10R10F: return 4;
	case PixelFormat::kA16B16G16R16: return 8;
	case PixelFormat::kA16B16G16R16_SIGNED: return 8;
	case PixelFormat::kA16B16G16R16_SIGNED_EXP1: return 8;
	case PixelFormat::kA16B16G16R16_UNSIGNED_EXP1: return 8;
	case PixelFormat::kA16B16G16R16_SIGNED_EXP2: return 8;
	case PixelFormat::kA16B16G16R16_UNSIGNED_EXP2: return 8;
	case PixelFormat::kA16B16G16R16_SIGNED_EXP3: return 8;
	case PixelFormat::kA16B16G16R16_UNSIGNED_EXP3: return 8;
	case PixelFormat::kR16F: return 2;
	case PixelFormat::kD16I: return 2;
	case PixelFormat::kG16R16F: return 4;
	case PixelFormat::kA16B16G16R16F: return 8;
	case PixelFormat::kR32F: return 4;
	case PixelFormat::kR32F_AS_R16F_EXPAND: return 2;
	case PixelFormat::kG32R32F: return 8;
	case PixelFormat::kG32R32F_AS_G16R16: return 4;
	case PixelFormat::kA32B32G32R32F: return 16;
	case PixelFormat::kA32B32G32R32: return 16;
	case PixelFormat::kInvalid: // fall-through
	default:
		SEOUL_FAIL("Invalid PixelFormat enum value.");
		return 0;
	};
}

Bool PixelFormatBytesPerPixel(PixelFormat eFormat, UInt& ruBytesPerPixel)
{
	switch (eFormat)
	{
	case PixelFormat::kDXT1: // fall-through
	case PixelFormat::kDXT2: // fall-through
	case PixelFormat::kDXT3: // fall-through
	case PixelFormat::kDXT4: // fall-through
	case PixelFormat::kDXT5: // fall-through
	case PixelFormat::kPVRTC_RGB_2BPPV1: // fall-through
	case PixelFormat::kPVRTC_RGB_4BPPV1: // fall-through
	case PixelFormat::kPVRTC_RGBA_2BPPV1: // fall-through
	case PixelFormat::kPVRTC_RGBA_4BPPV1: // fall-through
	case PixelFormat::kETC1_RGB8:
		return false;
	default:
		ruBytesPerPixel = (UInt)InternalStaticPixelFormatBytesPerPixel(eFormat);
		return true;
	};
}

Bool GetPitchForPixelFormat(Int32 iWidth, PixelFormat eFormat, UInt& ruPitch)
{
	switch (eFormat)
	{
	case PixelFormat::kDXT1: // fall-through
	case PixelFormat::kDXT2: // fall-through
	case PixelFormat::kDXT3: // fall-through
	case PixelFormat::kDXT4: // fall-through
	case PixelFormat::kDXT5: // fall-through
	case PixelFormat::kPVRTC_RGB_2BPPV1: // fall-through
	case PixelFormat::kPVRTC_RGB_4BPPV1: // fall-through
	case PixelFormat::kPVRTC_RGBA_2BPPV1: // fall-through
	case PixelFormat::kPVRTC_RGBA_4BPPV1: // fall-through
	case PixelFormat::kETC1_RGB8:
		return false;
	default:
		ruPitch = (UInt)(iWidth * InternalStaticPixelFormatBytesPerPixel(eFormat));
		return true;
	};
}

UInt GetDataSizeForPixelFormat(Int32 iWidth, Int32 iHeight, PixelFormat eFormat)
{
	switch (eFormat)
	{
	case PixelFormat::kDXT1:
		return (UInt)(Max(1, (iWidth + 3) / 4) * Max(1, (iHeight + 3) / 4) * 8);
	case PixelFormat::kDXT2: // fall-through
	case PixelFormat::kDXT3: // fall-through
	case PixelFormat::kDXT4: // fall-through
	case PixelFormat::kDXT5:
		return (UInt)(Max(1, (iWidth + 3) / 4) * Max(1, (iHeight + 3) / 4) * 16);

	case PixelFormat::kETC1_RGB8:
		return (UInt)(Max(1, iWidth / 4) * Max(1, iHeight / 4) * 8);

	case PixelFormat::kPVRTC_RGB_4BPPV1: // fall-through
	case PixelFormat::kPVRTC_RGBA_4BPPV1:
		return (UInt)(Max(2, iWidth / 4) * Max(2, iHeight / 4) * 8);

	case PixelFormat::kPVRTC_RGB_2BPPV1: // fall-through
	case PixelFormat::kPVRTC_RGBA_2BPPV1:
		return (UInt)(Max(2, iWidth / 8) * Max(2, iHeight / 4) * 8);

	default:
		return (UInt)(iWidth * iHeight * InternalStaticPixelFormatBytesPerPixel(eFormat));
	};
}

} // namespace Seoul
