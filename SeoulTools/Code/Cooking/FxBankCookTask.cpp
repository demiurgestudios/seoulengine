/**
 * \file FxBankCookTask.cpp
 * \brief Implement cooking of .xfx files into SeoulEngine .fxb files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BaseCookTask.h"
#include "ICookContext.h"
#include "Compress.h"
#include "CookPriority.h"
#include "Logger.h"
#include "Prereqs.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#include "StreamBuffer.h"

namespace Seoul
{

namespace Cooking
{

namespace FxBankCookDetail { struct ComponentDefinition; }


Bool FxBankCook(const FxBankCookDetail::ComponentDefinition& componentDefinition, Platform ePlatform, FilePath filePath, StreamBuffer& r);
CheckedPtr<FxBankCookDetail::ComponentDefinition const> FxBankLoadComponentDefinition(Platform ePlatform);
void FxBankDestroyComponentDefinition(CheckedPtr<FxBankCookDetail::ComponentDefinition const>& rp);

class FxBankCookTask SEOUL_SEALED : public BaseCookTask
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(FxBankCookTask);

	Mutex m_Mutex;
	Atomic32Value<Bool> m_bHasComponentDefinition;
	CheckedPtr<FxBankCookDetail::ComponentDefinition const> m_pComponentDefinition;

	FxBankCookTask()
	{
	}

	~FxBankCookTask()
	{
		Lock lock(m_Mutex);
		FxBankDestroyComponentDefinition(m_pComponentDefinition);
	}

	virtual Bool CanCook(FilePath filePath) const SEOUL_OVERRIDE
	{
		return (filePath.GetType() == FileType::kFxBank);
	}

	virtual Bool CookAllOutOfDateContent(ICookContext& rContext) SEOUL_OVERRIDE
	{
		// Do this first before issuing (parallel) individual cooks.
		if (!InternalResolveComponentDefinition(rContext))
		{
			return false;
		}

		ContentFiles v;
		if (!DefaultOutOfDateCook(rContext, FileType::kFxBank, v, true))
		{
			return false;
		}

		return true;
	}

	virtual Int32 GetPriority() const SEOUL_OVERRIDE
	{
		return CookPriority::kFxBank;
	}

private:
	SEOUL_DISABLE_COPY(FxBankCookTask);

	virtual Bool InternalCook(ICookContext& rContext, FilePath filePath) SEOUL_OVERRIDE
	{
		// Acquire ComponentDefinition.
		auto const pComponentDefinition(InternalResolveComponentDefinition(rContext));
		if (!pComponentDefinition)
		{
			return false;
		}

		StreamBuffer buffer;
		if (!FxBankCook(*pComponentDefinition, rContext.GetPlatform(), filePath, buffer))
		{
			return false;
		}

		return WriteOutput(rContext, buffer, filePath);
	}

	CheckedPtr<FxBankCookDetail::ComponentDefinition const> InternalResolveComponentDefinition(ICookContext& rContext)
	{
		// Check first and return immediately.
		if (m_bHasComponentDefinition)
		{
			return m_pComponentDefinition;
		}

		// Exclusive for parallel case (not strictly necessary due
		// to upfront resolve in CookAllOutOfDateContent())
		// but proofing behavior for possible future changes).
		Lock lock(m_Mutex);
		// Check again inside lock.
		if (m_bHasComponentDefinition)
		{
			return m_pComponentDefinition;
		}

		// Failure reported from within FxBankLoadComponentDefinition - nullptr returned on failure.
		m_pComponentDefinition = FxBankLoadComponentDefinition(rContext.GetPlatform());
		// Must be last.
		m_bHasComponentDefinition = true;
		// Done.
		return m_pComponentDefinition;
	}

	Bool WriteOutput(ICookContext& rContext, StreamBuffer& r, FilePath filePath) const
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
			StreamBuffer tmp;
			tmp.Swap(r);

			if (!ZSTDCompress(tmp.GetBuffer(), tmp.GetTotalDataSizeInBytes(), pC, uC, ZSTDCompressionLevel::kBest, MemoryBudgets::Cooking))
			{
				SEOUL_LOG_COOKING("%s: failed compressing fx data.", filePath.CStr());
				return false;
			}
		}

		return AtomicWriteFinalOutput(rContext, pC, uC, filePath);
	}
}; // class FxBankCookTask

} // namespace Cooking

SEOUL_BEGIN_TYPE(Cooking::FxBankCookTask, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Cooking::BaseCookTask)
SEOUL_END_TYPE()

} // namespace Seoul
