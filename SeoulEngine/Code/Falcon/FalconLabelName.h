/**
 * \file FalconLabelName.h
 * \brief Abstracts the target name of a GotoAndPlayByLabel
 * or GotoAndStopByLabel
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_LABEL_NAME_H
#define FALCON_LABEL_NAME_H

#include "SeoulHString.h"

namespace Seoul
{

namespace Falcon
{

class LabelName SEOUL_SEALED
{
public:
	static const Bool kbLabelIsCaseInsensitive = true;

	         LabelName()                               : m_h() {}

	         LabelName(const CStringLiteral& sLiteral) : m_h(sLiteral, kbLabelIsCaseInsensitive) {}
	explicit LabelName(Byte const* sString)            : m_h(sString,  kbLabelIsCaseInsensitive) {}
	         LabelName(Byte const* s, UInt32 u)        : m_h(s, u, kbLabelIsCaseInsensitive) {}
	explicit LabelName(const String& sString)          : m_h(sString, kbLabelIsCaseInsensitive) {}

	         LabelName(const LabelName& b)             : m_h(b.m_h) {}

	~LabelName() {}

	LabelName& operator=(LabelName b)
	{
		m_h = b.m_h;
		return *this;
	}

	UInt32      GetHash() const        { return m_h.GetHash(); }
	UInt32      GetSizeInBytes() const { return m_h.GetSizeInBytes(); }
	const Byte* CStr() const           { return m_h.CStr(); }
	Bool        IsEmpty() const        { return m_h.IsEmpty(); }

	Bool        operator==(LabelName b) const { return m_h == b.m_h; }
	Bool        operator!=(LabelName b) const { return m_h != b.m_h; }

	Bool        operator==(Byte const* s) const { return m_h == s; }
	Bool        operator!=(Byte const* s) const { return m_h != s; }

	Bool        operator<(LabelName b) const  { return m_h < b.m_h; }
	Bool        operator>(LabelName b) const  { return m_h > b.m_h; }

private:
	HString m_h;
}; // class LabelName

inline Bool operator==(Byte const* a, LabelName b)
{
	return (b == a);
}

inline Bool operator!=(Byte const* a, LabelName b)
{
	return (b != a);
}

static inline UInt32 GetHash(const LabelName& label)
{
	return label.GetHash();
}

} // namespace Falcon

template <> struct CanMemCpy<Falcon::LabelName> { static const Bool Value = true; };
template <> struct CanZeroInit<Falcon::LabelName> { static const Bool Value = true; };

template <>
struct DefaultHashTableKeyTraits<Falcon::LabelName>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static Falcon::LabelName GetNullKey()
	{
		return Falcon::LabelName();
	}

	static const Bool kCheckHashBeforeEquals = false;
};

} // namespace Seoul

#endif // include guard
