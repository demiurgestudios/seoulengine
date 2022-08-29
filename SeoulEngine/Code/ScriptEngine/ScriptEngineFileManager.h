/**
 * \file ScriptEngineFileManager.h
 * \brief Binder instance for exposing the FileManager Singleton into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_FILE_MANAGER_H
#define SCRIPT_ENGINE_FILE_MANAGER_H

#include "Prereqs.h"
#include "Vector.h"
namespace Seoul { namespace Script { class FunctionInterface; } }
namespace Seoul { class String; }

namespace Seoul
{

/** Binder, wraps a Camera instance and exposes functionality to a script VM. */
class ScriptEngineFileManager SEOUL_SEALED
{
public:
	ScriptEngineFileManager();
	~ScriptEngineFileManager();

	Bool Delete(const String& sFileName);
	void FileExists(Script::FunctionInterface* pInterface) const;
	Vector<String> GetDirectoryListing(const String& sPath, Bool bRecursive, const String& sFileExtension) const;

	// TODO: Move into ScriptEngineGamePaths?
	String GetSourceDir() const;
	String GetVideosDir() const;

	Bool RenameFile(const String& sFrom, const String& sTo);

private:
	SEOUL_DISABLE_COPY(ScriptEngineFileManager);
}; // class ScriptEngineFileManager

} // namespace Seoul

#endif // include guard
