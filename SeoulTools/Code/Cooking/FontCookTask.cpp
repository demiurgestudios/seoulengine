/**
 * \file FontCookTask.cpp
 * \brief Cooking tasks for cooking .ttf files into runtime
 * SeoulEngine .sff files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CookPriority.h"
#include "BaseCookTask.h"
#include "FalconTypes.h"
#include "FileManager.h"
#include "Logger.h"
#include "Prereqs.h"
#include "ReflectionDefine.h"

namespace Seoul
{

namespace Cooking
{

class FontCookTask SEOUL_SEALED : public BaseCookTask
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(FontCookTask);

	FontCookTask()
	{
	}

	virtual Bool CanCook(FilePath filePath) const SEOUL_OVERRIDE
	{
		return (filePath.GetType() == FileType::kFont);
	}

	virtual Bool CookAllOutOfDateContent(ICookContext& rContext) SEOUL_OVERRIDE
	{
		ContentFiles v;
		if (!DefaultOutOfDateCook(rContext, FileType::kFont, v))
		{
			return false;
		}

		return true;
	}

	virtual Int32 GetPriority() const SEOUL_OVERRIDE
	{
		return CookPriority::kFont;
	}

private:
	SEOUL_DISABLE_COPY(FontCookTask);

	virtual Bool InternalCook(ICookContext& rContext, FilePath filePath) SEOUL_OVERRIDE
	{
		auto const sInputFilename(filePath.GetAbsoluteFilenameInSource());

		void* p = nullptr;
		UInt32 u = 0u;
		if (!FileManager::Get()->ReadAll(sInputFilename, p, u, 0u, MemoryBudgets::Cooking))
		{
			SEOUL_LOG_COOKING("%s: failed reading input font data from disk.", filePath.CStr());
			return false;
		}

		// TrueTypeFontData takes ownership of the buffer we passed in.
		Falcon::TrueTypeFontData data(HString(Path::GetFileName(sInputFilename)), p, u);

		StreamBuffer buffer;
		if (!data.Cook(buffer))
		{
			SEOUL_LOG_COOKING("%s: failed cooking input font data to runtime SDF format.", filePath.CStr());
			return false;
		}

		return AtomicWriteFinalOutput(rContext, buffer.GetBuffer(), buffer.GetTotalDataSizeInBytes(), filePath);
	}

}; // class FontCookTask

} // namespace Cooking

SEOUL_BEGIN_TYPE(Cooking::FontCookTask, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Cooking::BaseCookTask)
SEOUL_END_TYPE()

} // namespace Seoul
