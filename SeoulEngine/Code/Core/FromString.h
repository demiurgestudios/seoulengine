/**
 * \file FromString.h
 * \brief Global functions for parsing various types from Seoul::Strings
 * into their concrete types.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FROM_STRING_H
#define FROM_STRING_H

#include "Prereqs.h"
#include "SeoulString.h"

namespace Seoul
{

// FromString overloads.
Bool FromString(Byte const* s, Bool& rb);
Bool FromString(Byte const* s, Int8& ri);
Bool FromString(Byte const* s, Int16& ri);
Bool FromString(Byte const* s, Int32& ri);
Bool FromString(Byte const* s, Int64& ri);
Bool FromString(Byte const* s, UInt8& ru);
Bool FromString(Byte const* s, UInt16& ru);
Bool FromString(Byte const* s, UInt32& ru);
Bool FromString(Byte const* s, UInt64& ru);
Bool FromString(Byte const* s, Float32& rf);
Bool FromString(Byte const* s, Float64& rf);

inline static Bool FromString(const String& s, Bool& rb) { return FromString(s.CStr(), rb); }
inline static Bool FromString(const String& s, Int8& ri) { return FromString(s.CStr(), ri); }
inline static Bool FromString(const String& s, Int16& ri) {	return FromString(s.CStr(), ri); }
inline static Bool FromString(const String& s, Int32& ri) {	return FromString(s.CStr(), ri); }
inline static Bool FromString(const String& s, Int64& ri) {	return FromString(s.CStr(), ri); }
inline static Bool FromString(const String& s, UInt8& ru) {	return FromString(s.CStr(), ru); }
inline static Bool FromString(const String& s, UInt16& ru) { return FromString(s.CStr(), ru); }
inline static Bool FromString(const String& s, UInt32& ru) { return FromString(s.CStr(), ru); }
inline static Bool FromString(const String& s, UInt64& ru) { return FromString(s.CStr(), ru); }
inline static Bool FromString(const String& s, Float32& rf) { return FromString(s.CStr(), rf); }
inline static Bool FromString(const String& s, Float64& rf) { return FromString(s.CStr(), rf); }

} // namespace Seoul

#endif  // include guard
