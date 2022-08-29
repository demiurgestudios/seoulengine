/**
 * \file ScriptUIMovieMutator.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "ReflectionMethod.h"
#include "ReflectionType.h"
#include "ScriptUIMovie.h"
#include "ScriptUIMovieMutator.h"

namespace Seoul
{

ScriptUIMovieMutator::ScriptUIMovieMutator(ScriptUIMovie& rMovie, Bool bNativeCall /*= false */)
	: m_aMethodArguments()
	, m_rMovie(rMovie)
	, m_bNativeCall(bNativeCall)
{
}

ScriptUIMovieMutator::~ScriptUIMovieMutator()
{
}

void ScriptUIMovieMutator::InternalInvokeMethod(
	HString methodName,
	UInt32 uArgumentCount)
{
	if (!m_rMovie.OnTryInvoke(
		methodName,
		m_aMethodArguments,
		uArgumentCount,
		m_bNativeCall))
	{
		SEOUL_LOG_UI("UIMovie %s is attempting to invoke method %s but invocation failed.\n",
			m_rMovie.GetMovieTypeName().CStr(),
			methodName.CStr());
	}
}

} // namespace Seoul
