/**
 * \file FilePathRelativeFilename.h
 * \brief Wraps the relative filename portion of a FilePath,
 * enforces case insensitivity.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FILE_PATH_RELATIVE_FILENAME_H
#define FILE_PATH_RELATIVE_FILENAME_H

#include "SeoulHString.h"

namespace Seoul
{

/**
 * IMPORTANT: This class deliberately does not accept an HString for construction,
 * since it exists to enforce the case insensitivity of filenames.
 */
class FilePathRelativeFilename SEOUL_SEALED
{
public:
	static const Bool kbIsCaseInsensitive = true;

	         FilePathRelativeFilename()                               : m_h() {}

	         FilePathRelativeFilename(const CStringLiteral& sLiteral) : m_h(sLiteral, kbIsCaseInsensitive) {}
	explicit FilePathRelativeFilename(Byte const* sString)            : m_h(sString,  kbIsCaseInsensitive) {}
	         FilePathRelativeFilename(Byte const* s, UInt32 u)        : m_h(s, u, kbIsCaseInsensitive) {}
	explicit FilePathRelativeFilename(const String& sString)          : m_h(sString, kbIsCaseInsensitive) {}

	         FilePathRelativeFilename(const FilePathRelativeFilename& b)             : m_h(b.m_h) {}

	~FilePathRelativeFilename() {}

	FilePathRelativeFilename& operator=(FilePathRelativeFilename b)
	{
		m_h = b.m_h;
		return *this;
	}

	UInt32      GetHash() const        { return m_h.GetHash(); }
	UInt32      GetSizeInBytes() const { return m_h.GetSizeInBytes(); }
	const Byte* CStr() const           { return m_h.CStr(); }
	Bool        IsEmpty() const        { return m_h.IsEmpty(); }
	HString     ToHString() const      { return m_h; }
	String      ToString() const       { return String(m_h); }

	HStringData::InternalIndexType GetHandleValue() const                               { return m_h.GetHandleValue(); }
	void                           SetHandleValue(HStringData::InternalIndexType value) { m_h.SetHandleValue(value); }

	Bool        operator==(FilePathRelativeFilename b) const { return m_h == b.m_h; }
	Bool        operator!=(FilePathRelativeFilename b) const { return m_h != b.m_h; }

	Bool        operator==(Byte const* s) const { return m_h == s; }
	Bool        operator!=(Byte const* s) const { return m_h != s; }

	Bool        operator<(FilePathRelativeFilename b) const  { return m_h < b.m_h; }
	Bool        operator>(FilePathRelativeFilename b) const  { return m_h > b.m_h; }

private:
	HString m_h;
}; // class FilePathRelativeFilename

inline Bool operator==(Byte const* a, FilePathRelativeFilename b)
{
	return (b == a);
}

inline Bool operator!=(Byte const* a, FilePathRelativeFilename b)
{
	return (b != a);
}

static inline UInt32 GetHash(const FilePathRelativeFilename& label)
{
	return label.GetHash();
}

template <> struct CanMemCpy<FilePathRelativeFilename> { static const Bool Value = true; };
template <> struct CanZeroInit<FilePathRelativeFilename> { static const Bool Value = true; };

template <>
struct DefaultHashTableKeyTraits<FilePathRelativeFilename>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static FilePathRelativeFilename GetNullKey()
	{
		return FilePathRelativeFilename();
	}

	static const Bool kCheckHashBeforeEquals = false;
};

} // namespace Seoul

#endif // include guard
