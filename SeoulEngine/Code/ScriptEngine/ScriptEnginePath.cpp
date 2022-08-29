/**
 * \file ScriptEnginePath.cpp
 * \brief Binder instance for exposing Core
 * functionality into script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Path.h"
#include "ReflectionDefine.h"
#include "ScriptEnginePath.h"
#include "ScriptFunctionInterface.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEnginePath, TypeFlags::kDisableCopy)
	SEOUL_METHOD(Combine)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "params string[] asArgs")
	SEOUL_METHOD(CombineAndSimplify)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "params string[] asArgs")
	SEOUL_METHOD(GetDirectoryName)
	SEOUL_METHOD(GetExactPathName)
	SEOUL_METHOD(GetExtension)
	SEOUL_METHOD(GetFileName)
	SEOUL_METHOD(GetFileNameWithoutExtension)
	SEOUL_METHOD(GetPathWithoutExtension)
	SEOUL_METHOD(GetTempFileAbsoluteFilename)
	SEOUL_METHOD(Normalize)
SEOUL_END_TYPE()

ScriptEnginePath::ScriptEnginePath()
{
}

ScriptEnginePath::~ScriptEnginePath()
{
}

void ScriptEnginePath::Combine(Script::FunctionInterface* pInterface) const
{
	Int32 const iArgs = pInterface->GetArgumentCount();

	// Argument 0 is 'self', so it's 1 + the number of arguments we expect.
	if (iArgs < 3)
	{
		pInterface->RaiseError(-1, "expected at least 2 string arguments to combine.");
		return;
	}

	String sReturn;
	if (!pInterface->GetString(1, sReturn))
	{
		pInterface->RaiseError(1, "expected string argument.");
		return;
	}

	// Argument 0 is self.
	for (Int32 i = 2; i < iArgs; ++i)
	{
		String s;
		if (!pInterface->GetString(i, s))
		{
			pInterface->RaiseError(i, "expected string argument.");
			return;
		}

		sReturn = Path::Combine(sReturn, s);
	}

	pInterface->PushReturnString(sReturn);
}

void ScriptEnginePath::CombineAndSimplify(Script::FunctionInterface* pInterface) const
{
	Int32 const iArgs = pInterface->GetArgumentCount();

	// Argument 0 is 'self', so it's 1 + the number of arguments we expect.
	if (iArgs < 3)
	{
		pInterface->RaiseError(-1, "expected at least 2 string arguments to combine.");
		return;
	}

	String sReturn;
	if (!pInterface->GetString(1, sReturn))
	{
		pInterface->RaiseError(1, "expected string argument.");
		return;
	}

	// Argument 0 is self.
	for (Int32 i = 2; i < iArgs; ++i)
	{
		String s;
		if (!pInterface->GetString(i, s))
		{
			pInterface->RaiseError(i, "expected string argument.");
			return;
		}

		sReturn = Path::Combine(sReturn, s);
	}

	if (!Path::CombineAndSimplify(
		String(),
		sReturn,
		sReturn))
	{
		pInterface->RaiseError(-1, "combine failed, likely, invalid characters in path segment.");
		return;
	}

	pInterface->PushReturnString(sReturn);
}

String ScriptEnginePath::GetDirectoryName(const String& sPath) const
{
	return Path::GetDirectoryName(sPath);
}

String ScriptEnginePath::GetExactPathName(const String& sPath) const
{
	return Path::GetExactPathName(sPath);
}

String ScriptEnginePath::GetExtension(const String& sPath) const
{
	return Path::GetExtension(sPath);
}

String ScriptEnginePath::GetFileName(const String& sPath) const
{
	return Path::GetFileName(sPath);
}

String ScriptEnginePath::GetFileNameWithoutExtension(const String& sPath) const
{
	return Path::GetFileNameWithoutExtension(sPath);
}

String ScriptEnginePath::GetPathWithoutExtension(const String& sPath) const
{
	return Path::GetPathWithoutExtension(sPath);
}

String ScriptEnginePath::GetTempFileAbsoluteFilename() const
{
	return Path::GetTempFileAbsoluteFilename();
}

String ScriptEnginePath::Normalize(const String& sPath) const
{
	return Path::Normalize(sPath);
}

} // namespace Seoul
