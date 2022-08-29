/**
 * \file SeoulETC1.h
 * \brief Implementation of ETC texture decompression in software.
 * \sa https://www.khronos.org/registry/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_ETC1_H
#define SEOUL_ETC1_H

#include "Prereqs.h"

namespace Seoul
{

// Decompress ETC1 texture data. Input is expected to be
// a SeoulEngine DDS container (which means, it may actually
// be 2 DDS containers, one after another, where the second
// DDS container is the alpha data for the decompressed data).
//
// On success, rpOut will be allocated and populated with
// a new SeoulEngine DDS container in a 32-bit RGBA format.
Bool ETC1Decompress(
	void const* pIn,
	UInt32 uInputDataSizeInBytes,
	void*& rpOut,
	UInt32& ruOutputDataSizeInBytes,
	MemoryBudgets::Type eType = MemoryBudgets::Rendering,
	UInt32 uAlignmentOfOutputBuffer = 4u);

} // namespace Seoul

#endif // include guard
