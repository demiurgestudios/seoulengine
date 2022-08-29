/**
 * \file ScriptEnginePath.h
 * \brief Binder instance for exposing Core
 * functionality into script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_PATH_H
#define SCRIPT_ENGINE_PATH_H

#include "Prereqs.h"
namespace Seoul { namespace Script { class FunctionInterface; } }
namespace Seoul { class String; }

namespace Seoul
{

class ScriptEnginePath SEOUL_SEALED
{
public:
	ScriptEnginePath();
	~ScriptEnginePath();

	void Combine(Script::FunctionInterface* pInterface) const;
	void CombineAndSimplify(Script::FunctionInterface* pInterface) const;
	String GetDirectoryName(const String& sPath) const;
	String GetExactPathName(const String& sPath) const;
	String GetExtension(const String& sPath) const;
	String GetFileName(const String& sPath) const;
	String GetFileNameWithoutExtension(const String& sPath) const;
	String GetPathWithoutExtension(const String& sPath) const;
	String GetTempFileAbsoluteFilename() const;
	String Normalize(const String& sPath) const;

private:
	SEOUL_DISABLE_COPY(ScriptEnginePath);
}; // class ScriptEnginePath

} // namespace Seoul

#endif // include guard
