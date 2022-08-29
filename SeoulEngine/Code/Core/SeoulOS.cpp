/**
 * \file SeoulOS.cpp
 * \brief Platform-agnostic  around common OS behavior
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "SeoulOS.h"
#include "StringUtil.h"
#include "Platform.h"
#if SEOUL_PLATFORM_LINUX
#include "Logger.h"
#include "unistd.h"
#endif

namespace Seoul
{

/**
 * Get an environment variable by name. Returns "" if unset.
 *
 * @param name The name of the environment variable to return.
 * @return The value of the named environment variable, or an empty string.
 */
String GetEnvironmentVar(const String& s)
{
#if SEOUL_PLATFORM_WINDOWS
	WCHAR aBuffer[MAX_PATH] = { 0 };
	DWORD const uResult = ::GetEnvironmentVariableW(
		s.WStr(),
		aBuffer,
		SEOUL_ARRAY_COUNT(aBuffer));

	// Success if uResult != 0 and uResult < SEOUL_ARRAY_COUNT(aBuffer).
	// A value greater than the buffer size means the value is too big,
	// and a value 0 usually means the variable is not found.
	if (uResult > 0u && uResult < SEOUL_ARRAY_COUNT(aBuffer))
	{
		// Make sure we nullptr terminate the buffer in all cases.
		aBuffer[uResult] = (WCHAR)'\0';
		return WCharTToUTF8(aBuffer);
	}
	// Otherwise, punt.
	else
	{
		return String();
	}
#else
	auto const sValue = getenv(s.CStr());
	if (nullptr == sValue)
	{
		return String();
	}
	else
	{
		return String(sValue);
	}
#endif
}

/**
 * Get the current username. Only implemented in Windows and Linux; otherwise, returns an empty string.
 *
 * @return The current OS username on Windows and Linux, or an empty string.
 */
String GetUsername()
{
	String sUsername;
#if SEOUL_PLATFORM_WINDOWS
	sUsername = GetEnvironmentVar("USERNAME");
#elif SEOUL_PLATFORM_LINUX
	{
		// Support up to 256 characters plus string terminator
		Vector<Byte> vBuffer(257, '\0');
		Int iResult = getlogin_r(vBuffer.Data(), vBuffer.GetSize());
		if (iResult == 0)
		{
			sUsername.Assign(vBuffer.Data());
		}
		else
		{
			SEOUL_WARN("Failed to get username: getlogin_r returned %d", iResult);
			sUsername = String();
		}
	}
#else
	sUsername = String();
#endif

	return sUsername;
}

} // namespace Seoul
