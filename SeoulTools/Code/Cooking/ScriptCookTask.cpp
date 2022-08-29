/**
 * \file ScriptCookTask.cpp
 * \brief Cooking tasks for cooking Lua .lua files into runtime
 * SeoulEngine .lbc files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BaseCookTask.h"
#include "Compress.h"
#include "CookPriority.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "ICookContext.h"
#include "Logger.h"
#include "Prereqs.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#include "SeoulFileWriters.h"
#include "StackOrHeapArray.h"

namespace Seoul
{

namespace Cooking
{

static inline Bool PadToPosition(SyncFile& r, Int64 iNewPosition)
{
	Int64 iPosition = 0;
	if (!r.GetCurrentPositionIndicator(iPosition))
	{
		SEOUL_LOG_COOKING("%s: failed getting position indicator for position padding.", r.GetAbsoluteFilename().CStr());
		return false;
	}

	Int64 const iDiff = (iNewPosition - iPosition);

	StackOrHeapArray<Byte, 16, MemoryBudgets::Cooking> aPadding((UInt32)iDiff);
	aPadding.Fill((Byte)0);
	if (aPadding.GetSize() != r.WriteRawData(aPadding.Data(), aPadding.GetSizeInBytes()))
	{
		SEOUL_LOG_COOKING("%s: failed writing %u bytes for position padding.", r.GetAbsoluteFilename().CStr(), aPadding.GetSize());
		return false;
	}

	return true;
}

class ScriptCookTask SEOUL_SEALED : public BaseCookTask
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ScriptCookTask);

	ScriptCookTask()
	{
	}

	virtual Bool CanCook(FilePath filePath) const SEOUL_OVERRIDE
	{
		return (filePath.GetType() == FileType::kScript);
	}

	virtual Bool CookAllOutOfDateContent(ICookContext& rContext) SEOUL_OVERRIDE
	{
		ContentFiles v;
		if (!DefaultOutOfDateCook(rContext, FileType::kScript, v, true))
		{
			return false;
		}

		return true;
	}

	virtual Int32 GetPriority() const SEOUL_OVERRIDE
	{
		return CookPriority::kScript;
	}

private:
	SEOUL_DISABLE_COPY(ScriptCookTask);

	const UInt32 kuUniversalScriptSignature = 0xA3C882F3;
	const int kiUniversalScriptVersion = 1;

	Bool CookToBytecodeCommon(
		ICookContext& rContext,
		FilePath filePath,
		Bool bGC64,
		String& rsTemporaryFileName) const
	{
		// Construct a full path to luajit.exe.
		auto const sFullPath(Path::Combine(
			rContext.GetToolsDirectory(),
			(bGC64 ? "LuaJITGC64\\luajit.exe" : "LuaJIT\\luajit.exe")));

		// Generate a temporary file.
		rsTemporaryFileName = Path::GetTempFileAbsoluteFilename();

		// Derive the root path - this is eithered Source/Authored/Scripts,
		// Source/Generated<Platform>/Scripts, or Source/Generated<Platform>/ScriptsDebug
		// depending on the file path.
		String const sRelative(filePath.GetRelativeFilenameWithoutExtension().ToString());
		UInt32 const uFirstSeparator = sRelative.Find(Path::kDirectorySeparatorChar);
		UInt32 const uSecondSeparator = (uFirstSeparator != String::NPos ? sRelative.Find(Path::kDirectorySeparatorChar, uFirstSeparator + 1u) : String::NPos);

		// Unexpected path.
		if (uSecondSeparator == String::NPos)
		{
			SEOUL_LOG_COOKING("%s: unexpected script path, must be in Authored/Scripts, Generated*/Scripts, or Generated*/ScriptsDebug", filePath.CStr());
			return false;
		}

		// Generate the absolute root path.
		auto const sRootPath(Path::Combine(GamePaths::Get()->GetSourceDir(), sRelative.Substring(0u, uSecondSeparator + 1u)));

		// Run the process.
		return RunCommandLineProcess(
			sRootPath,
			sFullPath,
			GetLuaCompilerArguments(rContext, sRootPath, filePath, rsTemporaryFileName));
	}

	Bool CookToUniversalBytecode(ICookContext& rContext, FilePath filePath) const
	{
		String sTemporaryFileNameStandard;
		String sTemporaryGC64;
		if (!CookToBytecodeCommon(rContext, filePath, false, sTemporaryFileNameStandard))
		{
			return false;
		}
		auto const scoped(MakeScopedAction([]() {}, [&]()
		{
			(void)FileManager::Get()->Delete(sTemporaryFileNameStandard);
		}));
		if (!CookToBytecodeCommon(rContext, filePath, true, sTemporaryGC64))
		{
			return false;
		}
		auto const scoped2(MakeScopedAction([]() {}, [&]()
		{
			(void)FileManager::Get()->Delete(sTemporaryGC64);
		}));

		return WriteOutput(
			rContext,
			sTemporaryFileNameStandard,
			sTemporaryGC64,
			filePath);
	}

	/** Generate Lua JIT bytecode, do not strip debug symbols, since we want them for crash reporting. */
	ProcessArguments GetLuaCompilerArguments(ICookContext& rContext, const String& sRootPath, FilePath filePath, const String& sOutputFile) const
	{
		auto const sRelativePath(filePath.GetAbsoluteFilenameInSource().Substring(sRootPath.GetSize()));

		ProcessArguments vs;
		vs.PushBack("-b");
		vs.PushBack("-g");
		vs.PushBack("-t");
		vs.PushBack("raw");
		vs.PushBack(sRelativePath);
		vs.PushBack(sOutputFile);
		return vs;
	}

	virtual Bool InternalCook(ICookContext& rContext, FilePath filePath) SEOUL_OVERRIDE
	{
		return CookToUniversalBytecode(rContext, filePath);
	}

	// TODO: Redundant with equivalent function in ScriptProtobufContentLoader.
	static void Obfuscate(
		Byte* pData,
		UInt32 uDataSizeInBytes,
		FilePath filePath)
	{
		// Get the base filename.
		String const s(Path::GetFileNameWithoutExtension(filePath.GetRelativeFilenameWithoutExtension().ToString()));

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
		void* p = nullptr;
		UInt32 u = 0u;
		if (!FileManager::Get()->ReadAll(sInput, p, u, 0u, MemoryBudgets::Cooking))
		{
			SEOUL_LOG_COOKING("%s: failed reading final file for script cook", filePath.CStr());
			return false;
		}

		Bool const bReturn = WriteOutput(rContext, p, u, filePath);

		MemoryManager::Deallocate(p);
		p = nullptr;
		u = 0u;

		return bReturn;
	}

	Bool WriteOutput(ICookContext& rContext, void const* p, UInt32 u, FilePath filePath) const
	{
		void* pC = nullptr;
		UInt32 uC = 0u;
		auto const deferredC(MakeDeferredAction([&]()
			{
				MemoryManager::Deallocate(pC);
				pC = nullptr;
				uC = 0u;
			}));

		if (!LZ4Compress(p, u, pC, uC, LZ4CompressionLevel::kBest, MemoryBudgets::Cooking))
		{
			SEOUL_LOG_COOKING("%s: failed compressing script data for script cook.", filePath.CStr());
			return false;
		}

		Obfuscate((Byte*)pC, uC, filePath);
		return AtomicWriteFinalOutput(rContext, pC, uC, filePath);
	}

	Bool WriteOutput(
		ICookContext& rContext,
		const String& sStandard,
		const String& sGC64,
		FilePath filePath) const
	{
		Vector<Byte> vStandard;
		Vector<Byte> vGC64;
		if (!FileManager::Get()->ReadAll(sStandard, vStandard))
		{
			SEOUL_LOG_COOKING("%s: failed reading standard script bytecode from \"%s\".", filePath.CStr(), sStandard.CStr());
			return false;
		}

		if (!FileManager::Get()->ReadAll(sGC64, vGC64))
		{
			SEOUL_LOG_COOKING("%s: failed reading GC64 script bytecode from \"%s\".", filePath.CStr(), sGC64.CStr());
			return false;
		}

		// Position of GC64 data - header + alignment.
		Int64 iGC64 = RoundUpToAlignment(24, 16);

		// Position of standard data - header + GC64 data, aligned.
		Int64 iStandard = RoundUpToAlignment(iGC64 + vGC64.GetSize(), 16);

		// Write data.
		MemorySyncFile file;
		if (!WriteUInt32(file, kuUniversalScriptSignature))
		{
			SEOUL_LOG_COOKING("%s: failed writing universal script signature.", filePath.CStr());
			return false;
		}
		if (!WriteUInt32(file, kiUniversalScriptVersion))
		{
			SEOUL_LOG_COOKING("%s: failed writing universal script version.", filePath.CStr());
			return false;
		}
		if (!WriteUInt32(file, (UInt32)iGC64))
		{
			SEOUL_LOG_COOKING("%s: failed writing universal script GC64 position.", filePath.CStr());
			return false;
		}
		if (!WriteUInt32(file, (UInt32)vGC64.GetSize()))
		{
			SEOUL_LOG_COOKING("%s: failed writing universal script GC64 size.", filePath.CStr());
			return false;
		}
		if (!WriteUInt32(file, (UInt32)iStandard))
		{
			SEOUL_LOG_COOKING("%s: failed writing universal script standard position.", filePath.CStr());
			return false;
		}
		if (!WriteUInt32(file, (UInt32)vStandard.GetSize()))
		{
			SEOUL_LOG_COOKING("%s: failed writing universal script standard size.", filePath.CStr());
			return false;
		}

		// Now write out the data - GC64 first, then standard.

		// GC64.
		if (!PadToPosition(file, iGC64))
		{
			SEOUL_LOG_COOKING("%s: failed writing universal script position padding for GC64.", filePath.CStr());
			return false;
		}
		if (vGC64.GetSize() != file.WriteRawData(vGC64.Data(), vGC64.GetSize()))
		{
			SEOUL_LOG_COOKING("%s: failed writing universal script GC64 data.", filePath.CStr());
			return false;
		}

		// Standard.
		if (!PadToPosition(file, iStandard))
		{
			SEOUL_LOG_COOKING("%s: failed writing universal script position padding for standard.", filePath.CStr());
			return false;
		}
		if (vStandard.GetSize() != file.WriteRawData(vStandard.Data(), vStandard.GetSize()))
		{
			SEOUL_LOG_COOKING("%s: failed writing universal script standard data.", filePath.CStr());
			return false;
		}

		// Finalize the output.
		return WriteOutput(rContext, file.GetBuffer().GetBuffer(), file.GetBuffer().GetTotalDataSizeInBytes(), filePath);
	}
}; // class ScriptCookTask

} // namespace Cooking

SEOUL_BEGIN_TYPE(Cooking::ScriptCookTask, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Cooking::BaseCookTask)
SEOUL_END_TYPE()

} // namespace Seoul
