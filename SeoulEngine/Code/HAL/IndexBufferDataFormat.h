/**
 * \file IndexBufferDataFormat.h
 * \brief Defines the per component element type that can be used
 * when constructing index buffers.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef INDEX_BUFFER_DATA_FORMAT_H
#define INDEX_BUFFER_DATA_FORMAT_H

namespace Seoul
{

enum class IndexBufferDataFormat
{
	kIndex16,
	kIndex32
};

} // namespace Seoul

#endif // include guard
