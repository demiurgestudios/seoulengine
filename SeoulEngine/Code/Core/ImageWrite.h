/**
 * \file ImageWrite.h
 * \brief Utility functions for writing image data of various formats
 * (e.g. PNG, TGA, etc.).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IMAGE_WRITE_H
#define IMAGE_WRITE_H

#include "Prereqs.h"
#include "SeoulFile.h"
namespace Seoul { class SyncFile; }

namespace Seoul
{

Bool ImageWritePng(
	Int32 iWidth, Int32 iHeight, Int32 iComponents, void const* pData, Int32 iStrideInBytes, SyncFile& rFile);
Bool ImageResizeAndWritePng(
	Int32 iWidth, Int32 iHeight, Int32 iComponents, void const* pData, Int32 iStrideInBytes, Int32 iOutWidth, Int32 iOutHeight, SyncFile& rFile);

} // namespace Seoul

#endif // include guard
