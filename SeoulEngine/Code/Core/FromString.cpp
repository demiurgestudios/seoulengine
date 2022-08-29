/**
 * \file FromString.cpp
 * \brief Global functions for parsing various types from Seoul::Strings
 * into their concrete types.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FromString.h"
#include "SeoulMath.h"
#include "StringUtil.h"

namespace Seoul
{

Bool FromString(Byte const* s, Bool& rb)
{
	if (0 == STRCMP_CASE_INSENSITIVE("true", s))
	{
		rb = true;
		return true;
	}
	else if (0 == STRCMP_CASE_INSENSITIVE("false", s))
	{
		rb = false;
		return true;
	}

	return false;
}

Bool FromString(Byte const* s, UInt8& ru)
{
	UInt64 u = 0u;
	if (FromString(s, u) && u <= (UInt64)UInt8Max)
	{
		ru = (UInt8)u;
		return true;
	}

	return false;
}

Bool FromString(Byte const* s, UInt16& ru)
{
	UInt64 u = 0u;
	if (FromString(s, u) && u <= (UInt64)UInt16Max)
	{
		ru = (UInt16)u;
		return true;
	}

	return false;
}

Bool FromString(Byte const* s, UInt32& ru)
{
	UInt64 u = 0u;
	if (FromString(s, u) && u <= (UInt64)UIntMax)
	{
		ru = (UInt32)u;
		return true;
	}

	return false;
}

Bool FromString(Byte const* s, UInt64& ru)
{
	if (!s || !s[0])
	{
		return false;
	}

	// Check for a leading minus sign.
	for (auto ss = s; *ss; ++ss)
	{
		Byte const c = *ss;
		if (c == '-')
		{
			return false;
		}
		else if (!IsSpace(c))
		{
			break;
		}
	}

	Byte* sEndPtr = nullptr;
	UInt64 const u = STRTOUINT64(s, &sEndPtr, 0);
	if (sEndPtr && !(*sEndPtr))
	{
		ru = u;
		return true;
	}
	else
	{
		return false;
	}
}

Bool FromString(Byte const* s, Int8& ri)
{
	Int64 i = 0;
	if (FromString(s, i) &&
		i >= (Int64)Int8Min &&
		i <= (Int64)Int8Max)
	{
		ri = (Int8)i;
		return true;
	}

	return false;
}

Bool FromString(Byte const* s, Int16& ri)
{
	Int64 i = 0;
	if (FromString(s, i) &&
		i >= (Int64)Int16Min &&
		i <= (Int64)Int16Max)
	{
		ri = (Int16)i;
		return true;
	}

	return false;
}

Bool FromString(Byte const* s, Int32& ri)
{
	Int64 i = 0;
	if (FromString(s, i) &&
		i >= (Int64)IntMin &&
		i <= (Int64)IntMax)
	{
		ri = (Int32)i;
		return true;
	}

	return false;
}

Bool FromString(Byte const* s, Int64& ri)
{
	if (!s || !s[0])
	{
		return false;
	}

	Byte* sEndPtr = nullptr;
	Int64 const i = STRTOINT64(s, &sEndPtr, 0);
	if (sEndPtr && !(*sEndPtr))
	{
		ri = i;
		return true;
	}
	else
	{
		return false;
	}
}

Bool FromString(Byte const* s, Float32& rf)
{
	if (!s || !s[0])
	{
		return false;
	}

	Byte* sEndPtr = nullptr;
	Double const f = STRTOD(s, &sEndPtr);
	if (sEndPtr && !(*sEndPtr))
	{
		rf = (Float32)f;
		return true;
	}
	else
	{
		return false;
	}
}

Bool FromString(Byte const* s, Float64& rf)
{
	if (!s || !s[0])
	{
		return false;
	}

	Byte* sEndPtr = nullptr;
	Double const f = STRTOD(s, &sEndPtr);
	if (sEndPtr && !(*sEndPtr))
	{
		rf = (Float64)f;
		return true;
	}
	else
	{
		return false;
	}
}

} // namespace Seoul
