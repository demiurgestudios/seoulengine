/**
 * \file ICookContext.h
 * \brief API through which cook tasks can accessed shared utilities.
 * Not thread safe.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ICOOK_CONTENT_H
#define ICOOK_CONTENT_H

#include "Prereqs.h"
namespace Seoul { namespace Scc { class IClient; } }
namespace Seoul { namespace Scc { struct FileTypeOptions; } }
namespace Seoul { class CookDatabase; }
namespace Seoul { namespace Cooking { class PackageCookConfig; } }

namespace Seoul::Cooking
{

class ICookContext SEOUL_ABSTRACT
{
public:
	typedef Vector<FilePath, MemoryBudgets::Cooking> FilePaths;

	virtual ~ICookContext()
	{
	}

	virtual void AdvanceProgress(HString type, Float fTimeInSeconds, Float fPercentage, UInt32 uActiveTasks, UInt32 uTotalTasks) = 0;
	virtual Bool AmendSourceFiles(String const* iBegin, String const* iEnd) = 0;
	virtual void CompleteProgress(HString type, Float fTimeInSeconds, Bool bSuccess) = 0;
	virtual Bool GetCookDebugOnly() const = 0;
	virtual Bool GetForceCompressionDictGeneration() const = 0;
	virtual CookDatabase& GetDatabase() = 0;
	virtual CheckedPtr<PackageCookConfig> GetPackageCookConfig() const = 0;
	virtual Platform GetPlatform() const = 0;
	virtual Scc::IClient& GetSourceControlClient() = 0;
	virtual const Scc::FileTypeOptions& GetSourceControlFileTypeOptions(Bool bNeedsExclusiveLock = true, Bool bLongLife = false) const = 0;
	virtual const FilePaths& GetSourceFilesOfType(FileType eType) const = 0;
	virtual const String& GetToolsDirectory() const = 0;
	virtual Bool RemoveSourceFiles(String const* iBegin, String const* iEnd) = 0;

protected:
	ICookContext()
	{
	}

private:
	SEOUL_DISABLE_COPY(ICookContext);
}; // class ICookContext

} // namespace Seoul::Cooking

#endif // include guard
