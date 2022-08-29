/**
 * \file EffectCookTask.cpp
 * \brief Cooking tasks for cooking Microsoft .fx shader files into runtime
 * SeoulEngine .fxc files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BaseCookTask.h"
#include "Compress.h"
#include "CookDatabase.h"
#include "CookPriority.h"
#include "EffectCompiler.h"
#include "Engine.h"
#include "FileManager.h"
#include "ICookContext.h"
#include "Logger.h"
#include "Platform.h"
#include "Prereqs.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#include "SeoulFileWriters.h"
#include "SeoulProcess.h"
#include "StackOrHeapArray.h"

namespace Seoul
{

namespace Cooking
{

namespace
{

struct DependenciesUtil SEOUL_SEALED
{
	SEOUL_DELEGATE_TARGET(DependenciesUtil);

	String m_sResults;

	void Func(Byte const* sIn)
	{
		m_sResults.Append(sIn);
	}
}; // struct DependenciesUtil

void LogError(Byte const* s)
{
	SEOUL_LOG_COOKING("%s", s);
}

struct Sorter
{
	Bool operator()(const CookSource& a, const CookSource& b)
	{
		return (STRCMP_CASE_INSENSITIVE(a.m_FilePath.CStr(), b.m_FilePath.CStr()) < 0);
	}
};

} // namespace anonymous

static const UInt32 kuPCEffectSignature = 0x4850A36F;
static const Int32 kiPCEffectVersion = 1;

static inline Bool PadToPosition(SyncFile& r, Int64 iNewPosition)
{
	Int64 iPosition = 0;
	if (!r.GetCurrentPositionIndicator(iPosition))
	{
		SEOUL_LOG_COOKING("%s: failed getting position indicator for alignment padding.", r.GetAbsoluteFilename().CStr());
		return false;
	}

	Int64 const iDiff = (iNewPosition - iPosition);

	StackOrHeapArray<Byte, 16, MemoryBudgets::Cooking> aPadding((UInt32)iDiff);
	aPadding.Fill((Byte)0);
	if (aPadding.GetSize() != r.WriteRawData(aPadding.Data(), aPadding.GetSizeInBytes()))
	{
		SEOUL_LOG_COOKING("%s: failed writing %u bytes for alignment padding.", r.GetAbsoluteFilename().CStr(), aPadding.GetSize());
		return false;
	}

	return true;
}

class EffectCookTask SEOUL_SEALED : public BaseCookTask
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(EffectCookTask);

	EffectCookTask()
	{
	}

	virtual Bool CanCook(FilePath filePath) const SEOUL_OVERRIDE
	{
		return (filePath.GetType() == FileType::kEffect);
	}

	virtual Bool CookAllOutOfDateContent(ICookContext& rContext) SEOUL_OVERRIDE
	{
		ContentFiles v;
		if (!DefaultOutOfDateCook(rContext, FileType::kEffect, v, true))
		{
			return false;
		}

		return true;
	}

	virtual Int32 GetPriority() const SEOUL_OVERRIDE
	{
		return CookPriority::kEffect;
	}

private:
	SEOUL_DISABLE_COPY(EffectCookTask);

	virtual Bool GetSources(ICookContext& rContext, FilePath filePath, Sources& rv) const SEOUL_OVERRIDE
	{
		return GetEffectDependencies(rContext, filePath, rv);
	}

	/** Join the D3D9 and D3D11 shaders into a combined container file. */
	Bool CombinePC(
		ICookContext& rContext,
		void const* pD3D9,
		UInt32 uD3D9,
		void const* pD3D11,
		UInt32 uD3D11,
		FilePath filePath) const
	{
		// Position of D3D11 data - header + alignment.
		Int64 iD3D11 = RoundUpToAlignment(24, 16);

		// Position of D3D9 data - header + D3D11 data, aligned.
		Int64 iD3D9 = RoundUpToAlignment(iD3D11 + uD3D11, 16);

		// Write data.
		MemorySyncFile file;
		if (!WriteUInt32(file, kuPCEffectSignature))
		{
			SEOUL_LOG_COOKING("%s: failed writing PC effect signature.", filePath.CStr());
			return false;
		}

		if (!WriteInt32(file, kiPCEffectVersion))
		{
			SEOUL_LOG_COOKING("%s: failed writing PC effect version.", filePath.CStr());
			return false;
		}

		// Position of D3D11 data.
		if (!WriteUInt32(file, (UInt32)iD3D11))
		{
			SEOUL_LOG_COOKING("%s: failed writing position of D3D11 chunk.", filePath.CStr());
			return false;
		}

		// Size of D3D11 data.
		if (!WriteUInt32(file, (UInt32)uD3D11))
		{
			SEOUL_LOG_COOKING("%s: failed writing size of D3D11 chunk.", filePath.CStr());
			return false;
		}

		// Position of D3D9 data.
		if (!WriteUInt32(file, (UInt32)iD3D9))
		{
			SEOUL_LOG_COOKING("%s: failed writing position of D3D9 chunk.", filePath.CStr());
			return false;
		}

		// Size of D3D9 data.
		if (!WriteUInt32(file, (UInt32)uD3D9))
		{
			SEOUL_LOG_COOKING("%s: failed writing size of D3D9 chunk.", filePath.CStr());
			return false;
		}

		// Now write out the data - D3D11 first, then D3D9.

		// D3D11.
		if (!PadToPosition(file, iD3D11))
		{
			SEOUL_LOG_COOKING("%s: failed padding for D3D11 chunk.", filePath.CStr());
			return false;
		}
		if (uD3D11 != file.WriteRawData(pD3D11, uD3D11))
		{
			SEOUL_LOG_COOKING("%s: failed writing D3D11 data.", filePath.CStr());
			return false;
		}

		// D3D9.
		if (!PadToPosition(file, iD3D9))
		{
			SEOUL_LOG_COOKING("%s: failed padding for D3D9 chunk.", filePath.CStr());
			return false;
		}
		if (uD3D9 != file.WriteRawData(pD3D9, uD3D9))
		{
			SEOUL_LOG_COOKING("%s: failed writing D3D9 data.", filePath.CStr());
			return false;
		}

		// Finalize the output.
		return WriteOutput(rContext, file.GetBuffer().GetBuffer(), file.GetBuffer().GetTotalDataSizeInBytes(), filePath);
	}

	/**
	* Cooking behavior for platforms other than PC.
	*/
	Bool CookNonPC(ICookContext& rContext, FilePath filePath) const
	{
		// Buffer.
		void* p = nullptr;
		UInt32 u = 0u;
		auto const deferred(MakeDeferredAction([&]() { MemoryManager::Deallocate(p); }));

		// Compile.
		Bool bSuccess = false;
		if (CompileEffectFile(
			EffectTarget::GLSLES2,
			filePath,
			ConstructStandardMacros(rContext.GetPlatform(), false),
			p,
			u))
		{
			bSuccess = WriteOutput(rContext, p, u, filePath);
		}

		return bSuccess;
	}

	/**
	 * Cooking behavior for the PC platform.
	 */
	Bool CookPC(ICookContext& rContext, FilePath filePath) const
	{
		// Buffers.
		void* pD3D9 = nullptr;
		UInt32 uD3D9 = 0u;
		auto const deferred(MakeDeferredAction([&]() { MemoryManager::Deallocate(pD3D9); }));
		void* pD3D11 = nullptr;
		UInt32 uD3D11 = 0u;
		auto const deferred2(MakeDeferredAction([&]() { MemoryManager::Deallocate(pD3D11); }));

		// Cook D3D9.
		if (!CompileEffectFile(
			EffectTarget::D3D9,
			filePath,
			ConstructStandardMacros(rContext.GetPlatform(), false),
			pD3D9,
			uD3D9))
		{
			return false;
		}

		// Cook D3D11.
		if (!CompileEffectFile(
			EffectTarget::D3D11,
			filePath,
			ConstructStandardMacros(rContext.GetPlatform(), true),
			pD3D11,
			uD3D11))
		{
			return false;
		}

		// Combine into output.
		return CombinePC(rContext, pD3D9, uD3D9, pD3D11, uD3D11, filePath);
	}

	/**
	 * @return Up-to-date effect dependencies for the effect file sAbsoluteEffectFileName.
	 *
	 * \remarks Includes the base file itself, as well as any includes.
	 */
	Bool GetEffectDependencies(ICookContext& rContext, FilePath filePath, Sources& rv) const
	{
		EffectFileDependencies set;
		if (!GetEffectFileDependencies(filePath, ConstructStandardMacros(rContext.GetPlatform(), false), set))
		{
			SEOUL_LOG_COOKING("%s: failed querying dependencies", filePath.CStr());
			return false;
		}
		// If kPC, need to also include D3D11, in case macros result in different includes.
		if (Platform::kPC == rContext.GetPlatform())
		{
			EffectFileDependencies tmp;
			if (!GetEffectFileDependencies(filePath, ConstructStandardMacros(rContext.GetPlatform(), true), tmp))
			{
				SEOUL_LOG_COOKING("%s: failed querying dependencies (D3D11)", filePath.CStr());
				return false;
			}

			// Merge.
			for (auto const& filePath : tmp) { set.Insert(filePath); }
		}

		// Convert.
		Sources vDepends;
		vDepends.Reserve(set.GetSize());
		for (auto const& filePath : set)
		{
			vDepends.PushBack(CookSource{ filePath, false });
		}

		// Sort.
		Sorter sorter;
		QuickSort(vDepends.Begin(), vDepends.End(), sorter);
		rv.Swap(vDepends);
		return true;
	}

	/**
	 * Based on the build config and platform, construct the standard
	 * set of compilation macros. These macros define the platform and
	 * build config within the shader source.
	 */
	MacroTable ConstructStandardMacros(Platform ePlatform, Bool bD3D11) const
	{
		MacroTable t;
		t.Insert(HString("SEOUL_PLATFORM_WINDOWS"), (Platform::kPC == ePlatform) ? "1" : "0");
		t.Insert(HString("SEOUL_PLATFORM_IOS"), (Platform::kIOS == ePlatform) ? "1" : "0");
		t.Insert(HString("SEOUL_PLATFORM_ANDROID"), (Platform::kAndroid == ePlatform) ? "1" : "0");
		t.Insert(HString("SEOUL_D3D9"), (!bD3D11 && Platform::kPC == ePlatform) ? "1" : "0");
		t.Insert(HString("SEOUL_D3D11"), (bD3D11 && Platform::kPC == ePlatform) ? "1" : "0");
		t.Insert(HString("SEOUL_OGLES2"), (Platform::kAndroid == ePlatform || Platform::kIOS == ePlatform) ? "1" : "0");
		return t;
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
			SEOUL_LOG_COOKING("%s: failed compression of effect data.", filePath.CStr());
			return false;
		}

		return AtomicWriteFinalOutput(rContext, pC, uC, filePath);
	}

	virtual Bool InternalCook(ICookContext& rContext, FilePath filePath) SEOUL_OVERRIDE
	{
		// Different processing between PC and other platforms.
		if (Platform::kPC == rContext.GetPlatform())
		{
			return CookPC(rContext, filePath);
		}
		else
		{
			return CookNonPC(rContext, filePath);
		}
	}
}; // class EffectCookTask

} // namespace Cooking

SEOUL_BEGIN_TYPE(Cooking::EffectCookTask, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Cooking::BaseCookTask)
SEOUL_END_TYPE()

} // namespace Seoul
