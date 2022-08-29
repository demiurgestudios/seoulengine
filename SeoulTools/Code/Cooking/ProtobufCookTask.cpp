/**
 * \file ProtobufCookTask.cpp
 * \brief Cooking tasks for cooking Google Protocol Buffer .proto files into runtime
 * SeoulEngine .pb files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "CookPriority.h"
#include "BaseCookTask.h"
#include "FileManager.h"
#include "ICookContext.h"
#include "Logger.h"
#include "Path.h"
#include "Prereqs.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"

namespace Seoul
{

namespace Cooking
{

/** Relative path from tools directory for protoc. */
#if SEOUL_PLATFORM_WINDOWS
static Byte const* ksProtoc = "protobuf\\protoc.exe";
#else
static Byte const* ksProtoc = "protobuf/protoc";
#endif

class ProtobufCookTask SEOUL_SEALED : public BaseCookTask
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ProtobufCookTask);

	ProtobufCookTask()
	{
	}

	virtual Bool CanCook(FilePath filePath) const SEOUL_OVERRIDE
	{
		return (filePath.GetType() == FileType::kProtobuf);
	}

	virtual Bool CookAllOutOfDateContent(ICookContext& rContext) SEOUL_OVERRIDE
	{
		ContentFiles v;
		if (!DefaultOutOfDateCook(rContext, FileType::kProtobuf, v))
		{
			return false;
		}

		return true;
	}

	virtual Int32 GetPriority() const SEOUL_OVERRIDE
	{
		return CookPriority::kProtobuf;
	}

private:
	SEOUL_DISABLE_COPY(ProtobufCookTask);

	ProcessArguments GetArguments(ICookContext& rContext, FilePath filePath, const String& sOutFilename) const
	{
		auto const sInputFilename(filePath.GetAbsoluteFilenameInSource());

		ProcessArguments vs;
		vs.PushBack("-I" + Path::GetDirectoryName(sInputFilename));
		vs.PushBack("-o" + sOutFilename);
		vs.PushBack(sInputFilename);

		return vs;
	}

	virtual Bool InternalCook(ICookContext& rContext, FilePath filePath) SEOUL_OVERRIDE
	{
		auto const sProtoc(Path::Combine(rContext.GetToolsDirectory(), ksProtoc));
		auto const sTemporaryFile(Path::GetTempFileAbsoluteFilename());
		auto const scoped(MakeScopedAction([]() {}, [&]()
		{
			(void)FileManager::Get()->Delete(sTemporaryFile);
		}));

		Bool bReturn = RunCommandLineProcess(sProtoc, GetArguments(rContext, filePath, sTemporaryFile));
		if (bReturn)
		{
			bReturn = WriteOutput(rContext, sTemporaryFile, filePath);
		}
		else
		{
			SEOUL_LOG_COOKING("%s: failed running protoc command (%s)", filePath.CStr(), sProtoc.CStr());
		}

		return bReturn;
	}

	// TODO: Redundant with equivalent function in ScriptProtobufContentLoader.
	void Obfuscate(
		Byte* pData,
		UInt32 uDataSizeInBytes,
		FilePath filePath) const
	{
		// Get the base filename.
		String const s(Path::GetFileName(filePath.GetRelativeFilenameWithoutExtension().ToString()).ToLowerASCII());

		UInt32 uXorKey = 0xB29F8D49;
		for (UInt32 i = 0u; i < s.GetSize(); ++i)
		{
			uXorKey = uXorKey * 33 + tolower(s[i]);
		}

		for (UInt32 i = 0u; i < uDataSizeInBytes; ++i)
		{
			// Mix in the file offset
			pData[i] ^= (Byte)((uXorKey >> ((i % 4) << 3)) + (i / 4) * 101);
		}
	}

	Bool WriteOutput(ICookContext& rContext, const String& sInput, FilePath filePath) const
	{
		void* pC = nullptr;
		UInt32 uC = 0u;
		auto const deferredC(MakeDeferredAction([&]()
			{
				MemoryManager::Deallocate(pC);
				pC = nullptr;
				uC = 0u;
			}));

		{
			void* p = nullptr;
			UInt32 u = 0u;
			auto const deferredU(MakeDeferredAction([&]()
				{
					MemoryManager::Deallocate(p);
					p = nullptr;
					u = 0u;
				}));

			if (!FileManager::Get()->ReadAll(sInput, p, u, 0u, MemoryBudgets::Cooking))
			{
				SEOUL_LOG_COOKING("%s: failed reading final file for protobuf cook", filePath.CStr());
				return false;
			}

			if (!LZ4Compress(p, u, pC, uC, LZ4CompressionLevel::kBest, MemoryBudgets::Cooking))
			{
				SEOUL_LOG_COOKING("%s: failed compressing final file for protobuf cook", filePath.CStr());
				return false;
			}
		}

		Obfuscate((Byte*)pC, uC, filePath);
		return AtomicWriteFinalOutput(rContext, pC, uC, filePath);
	}
}; // class ProtobufCookTask

} // namespace Cooking

SEOUL_BEGIN_TYPE(Cooking::ProtobufCookTask, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Cooking::BaseCookTask)
SEOUL_END_TYPE()

} // namespace Seoul
