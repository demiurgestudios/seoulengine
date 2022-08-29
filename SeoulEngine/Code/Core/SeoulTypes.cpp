/**
 * \file SeoulTypes.cpp
 * \brief Portable types for use in Seoul.
 *
 * Here we do compile-time checks on the sizes of our types.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "SeoulTypes.h"
#include "SeoulAssert.h"

namespace Seoul
{

// Compile-time checks of the sizes of types

SEOUL_STATIC_ASSERT(sizeof(UInt8) == 1);
SEOUL_STATIC_ASSERT(sizeof(Int8) == 1);

SEOUL_STATIC_ASSERT(sizeof(UInt16) == 2);
SEOUL_STATIC_ASSERT(sizeof(Int16) == 2);

SEOUL_STATIC_ASSERT(sizeof(UInt32) == 4);
SEOUL_STATIC_ASSERT(sizeof(Int32) == 4);

SEOUL_STATIC_ASSERT(sizeof(UInt64) == 8);
SEOUL_STATIC_ASSERT(sizeof(Int64) == 8);

SEOUL_STATIC_ASSERT(sizeof(Float32) == 4);
SEOUL_STATIC_ASSERT(sizeof(Float64) == 8);

SEOUL_STATIC_ASSERT(sizeof(UByte) == 1);
SEOUL_STATIC_ASSERT(sizeof(Byte) == 1);

SEOUL_STATIC_ASSERT(sizeof(UniChar) == 4);

SEOUL_STATIC_ASSERT(sizeof(UShort) == 2);
SEOUL_STATIC_ASSERT(sizeof(Short) == 2);

SEOUL_STATIC_ASSERT(sizeof(UInt) == 4);
SEOUL_STATIC_ASSERT(sizeof(Int) == 4);

SEOUL_STATIC_ASSERT(sizeof(ULongInt) == 8);
SEOUL_STATIC_ASSERT(sizeof(LongInt) == 8);

SEOUL_STATIC_ASSERT(sizeof(Float) == 4);
SEOUL_STATIC_ASSERT(sizeof(Double) == 8);

} // namespace Seoul
