/**
 * \file Util.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Platform.h"
#include "Prereqs.h"
#include "SeoulString.h"

namespace Seoul
{

// TODO: Move into a core function.
Bool IsReadOnly(const String& sFilename)
{
#if SEOUL_PLATFORM_WINDOWS
	auto const uAttributes = ::GetFileAttributesW(sFilename.WStr());
	return (FILE_ATTRIBUTE_READONLY == (FILE_ATTRIBUTE_READONLY & uAttributes));
#else // !SEOUL_PLATFORM_WINDOWS
	return (0 != access(sFilename.CStr(), W_OK));
#endif // /!SEOUL_PLATFORM_WINDOWS
}

} // namespace Seoul
