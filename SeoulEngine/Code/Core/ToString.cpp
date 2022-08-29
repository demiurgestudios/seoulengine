/**
 * \file ToString.cpp
 * \brief Global functions for converting various types to Seoul::Strings.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "SeoulString.h"
#include "ToString.h"

namespace Seoul
{

/**
 * Convert a Bool to a string
 */
String ToString(Bool b)
{
	static const String sTrue("true");
	static const String sFalse("false");

	return b ? sTrue : sFalse;
}

/**
 * Convert a signed 8-bit integer to a string
 */
String ToString(Int8 i)
{
	return String::Printf("%d", (Int32)i);
}

/**
 * Convert a signed 16-bit integer to a string
 */
String ToString(Int16 i)
{
	return String::Printf("%d", (Int32)i);
}

/**
 * Convert a signed 32-bit integer to a string
 */
String ToString(Int32 i)
{
	return String::Printf("%d", i);
}

/**
 * Convert a signed 64-bit integer to a string
 */
String ToString(Int64 i)
{
	return String::Printf("%" PRId64, i);
}

/**
 * Convert an unsigned 8-bit integer to a string
 */
String ToString(UInt8 u)
{
	return String::Printf("%u", (UInt32)u);
}

/**
 * Convert an unsigned 16-bit integer to a string
 */
String ToString(UInt16 u)
{
	return String::Printf("%u", (UInt32)u);
}

/**
 * Convert an unsigned 32-bit integer to a string
 */
String ToString(UInt32 u)
{
	return String::Printf("%u", u);
}

/**
 * Convert an unsigned 64-bit integer to a string
 */
String ToString(UInt64 u)
{
	return String::Printf("%" PRIu64, u);
}

/**
 * Convert a 32-bit floating point value to a string
 */
String ToString(Float32 f)
{
	String const s(String::Printf("%g", f));
	return s;
}

/**
 * Convert a 64-bit floating point value to a string
 */
String ToString(Float64 f)
{
	String const s(String::Printf("%g", f));
	return s;
}

} // namespace Seoul
