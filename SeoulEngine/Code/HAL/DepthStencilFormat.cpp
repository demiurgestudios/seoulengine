/**
 * \file DepthStencilFormat.cpp
 * \brief Enum defining valid pixel formats for depth-stencil formats.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DepthStencilFormat.h"
#include "ReflectionDefine.h"
#include "SeoulHString.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(DepthStencilFormat)
	SEOUL_ENUM_N("Invalid", DepthStencilFormat::kInvalid)
	SEOUL_ENUM_N("D16_LOCKABLE", DepthStencilFormat::kD16_LOCKABLE)
	SEOUL_ENUM_N("D32", DepthStencilFormat::kD32)
	SEOUL_ENUM_N("D15S1", DepthStencilFormat::kD15S1)
	SEOUL_ENUM_N("D24S8", DepthStencilFormat::kD24S8)
	SEOUL_ENUM_N("D24FS8", DepthStencilFormat::kD24FS8)
	SEOUL_ENUM_N("D24X8", DepthStencilFormat::kD24X8)
	SEOUL_ENUM_N("D24X4S4", DepthStencilFormat::kD24X4S4)
	SEOUL_ENUM_N("D16", DepthStencilFormat::kD16)
	SEOUL_ENUM_N("D16S8", DepthStencilFormat::kD16S8)
SEOUL_END_ENUM()

UInt32 DepthStencilFormatBytesPerPixel(DepthStencilFormat eFormat)
{
	switch (eFormat)
	{
	case DepthStencilFormat::kD16_LOCKABLE: return 2u;
	case DepthStencilFormat::kD32: return 4u;
	case DepthStencilFormat::kD15S1: return 2u;
	case DepthStencilFormat::kD24S8: return 4u;
	case DepthStencilFormat::kD24FS8: return 4u;
	case DepthStencilFormat::kD24X8: return 4u;
	case DepthStencilFormat::kD24X4S4: return 4u;
	case DepthStencilFormat::kD16: return 2u;
	case DepthStencilFormat::kD16S8: return 3u;
	default:
		return 0u;
	};
}

/**
 * Convert a string representing a DepthStencilFormat to an equivalent
 * enum value.
 */
DepthStencilFormat HStringToDepthStencilFormat(HString hstring)
{
	static const HString kD16_LOCKABLE("D16_LOCKABLE");
	static const HString kD32("D32");
	static const HString kD15S1("D15S1");
	static const HString kD24S8("D24S8");
	static const HString kD24FS8("D24FS8");
	static const HString kD24X8("D24X8");
	static const HString kD24X4S4("D24X4S4");
	static const HString kD16("D16");
	static const HString kD16S8("D16S8");

	if (kD16_LOCKABLE == hstring) { return DepthStencilFormat::kD16_LOCKABLE; }
	else if (kD32 == hstring) { return DepthStencilFormat::kD32; }
	else if (kD15S1 == hstring) { return DepthStencilFormat::kD15S1; }
	else if (kD24S8 == hstring) { return DepthStencilFormat::kD24S8; }
	else if (kD24FS8 == hstring) { return DepthStencilFormat::kD24FS8; }
	else if (kD24X8 == hstring) { return DepthStencilFormat::kD24X8; }
	else if (kD24X4S4 == hstring) { return DepthStencilFormat::kD24X4S4; }
	else if (kD16 == hstring) { return DepthStencilFormat::kD16; }
	else if (kD16S8 == hstring) { return DepthStencilFormat::kD16S8; }
	else
	{
		return DepthStencilFormat::kInvalid;
	}
}

} // namespace Seoul
