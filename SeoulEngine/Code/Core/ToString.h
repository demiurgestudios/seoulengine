/**
 * \file ToString.h
 * \brief Global functions for converting various types to Seoul::Strings.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef TO_STRING_H
#define TO_STRING_H

#include "Prereqs.h"
#include "SeoulHString.h"
namespace Seoul { class String; }

namespace Seoul
{

// Convenience for ToString(Vector)
static inline String ToString(HString h) { return String(h); }
static inline const String& ToString(const String& s) { return s; }

// ToString overloads.
String ToString(Bool b);
String ToString(Int8 i);
String ToString(Int16 i);
String ToString(Int32 i);
String ToString(Int64 i);
String ToString(UInt8 u);
String ToString(UInt16 u);
String ToString(UInt32 u);
String ToString(UInt64 u);
String ToString(Float32 f);
String ToString(Float64 f);

template <typename T, int MEMORY_BUDGETS>
static String ToString(const Vector<T, MEMORY_BUDGETS>& v, Byte const* sSep = ", ")
{
	String s;
	Bool b = true;
	for (auto const& e : v)
	{
		if (!b) { s.Append(sSep); }
		b = false;
		s.Append(ToString(e));
	}
	
	return s;
}

} // namespace Seoul

#endif  // include guard
