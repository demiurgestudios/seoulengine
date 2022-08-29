/**
 * \file TextureCookISPC.cpp
 * \brief Implementation of texture compression using Intel ISPC.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "JobsJob.h"
#include "JobsManager.h"
#include "TextureCookISPC.h"
#include "TextureCookISPCKernel_ispc.h"
#include "Vector.h"

extern "C"
{

/** ISPC generate task function pointer. */
typedef void (*ISPCTaskFunc)(
	void* pData,
	int iThreadIndex,
	int iThreadCount,
	int iTaskIndex,
	int iTaskCount,
	int iTaskIndex0,
	int iTaskIndex1,
	int iTaskIndex2,
	int iTaskCount0,
	int iTaskCount1,
	int iTaskCount2);

} // extern "C"

namespace Seoul::Cooking
{

namespace CompressorISPC
{

void CompressBlocksDXT1(const ispc::Image& image, UInt8* pOutput)
{
	ispc::DXT1_Compress((ispc::Image*)&image, pOutput);
}

void CompressBlocksDXT5(const ispc::Image& image, UInt8* pOutput)
{
	ispc::DXT5_Compress((ispc::Image*)&image, pOutput);
}

void CompressBlocksETC1(const ispc::Image& image, UInt8* pOutput, Int32 iQuality /*= ETC1Quality::kHighest*/)
{
	ispc::ETC1_Compress((ispc::Image*)&image, pOutput, iQuality);
}

} // namespace CompressorISPC

namespace
{

class Task SEOUL_SEALED : public Jobs::Job
{
public:
	Task(
		void* pFunc,
		void* pData,
		Int iThreadCount,
		Int iTaskIndex,
		Int iTaskCount,
		Int iX,
		Int iY,
		Int iZ,
		Int iCountX,
		Int iCountY,
		Int iCountZ)
		: m_pFunc((ISPCTaskFunc)pFunc)
		, m_pData(pData)
		, m_iThreadCount(iThreadCount)
		, m_iTaskIndex(iTaskIndex)
		, m_iTaskCount(iTaskCount)
		, m_iX(iX)
		, m_iY(iY)
		, m_iZ(iZ)
		, m_iCountX(iCountX)
		, m_iCountY(iCountY)
		, m_iCountZ(iCountZ)
	{
	}

	~Task()
	{
		WaitUntilJobIsNotRunning();
	}

private:
	SEOUL_DISABLE_COPY(Task);
	SEOUL_REFERENCE_COUNTED_SUBCLASS(Task);

	ISPCTaskFunc const m_pFunc;
	void* const m_pData;
	Int const m_iThreadCount;
	Int const m_iTaskIndex;
	Int const m_iTaskCount;
	Int const m_iX;
	Int const m_iY;
	Int const m_iZ;
	Int const m_iCountX;
	Int const m_iCountY;
	Int const m_iCountZ;

	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE
	{
		UInt32 uThreadIndex = 0u;
		SEOUL_VERIFY(Jobs::Manager::Get()->GetThreadIndex(uThreadIndex));
		SEOUL_ASSERT(uThreadIndex < (UInt32)m_iThreadCount);

		// Execute.
		m_pFunc(
			m_pData,
			(Int)uThreadIndex,
			m_iThreadCount,
			m_iTaskIndex,
			m_iTaskCount,
			m_iX,
			m_iY,
			m_iZ,
			m_iCountX,
			m_iCountY,
			m_iCountZ);

		reNextState = Jobs::State::kComplete;
	}
}; // class Task

class TaskGroup SEOUL_SEALED
{
public:
	TaskGroup()
		: m_uThreadCount(Jobs::Manager::Get()->GetThreadCount())
	{
	}

	~TaskGroup()
	{
		Sync();
	}

	void* AllocateAligned(Int64 iSize, Int32 iAlignment)
	{
		auto p = MemoryManager::AllocateAligned((size_t)iSize, MemoryBudgets::Cooking, (size_t)iAlignment);
		m_vBlocks.PushBack(p);
		return p;
	}

	void Launch(
		void* pFunc,
		void* pData,
		Int iCountX,
		Int iCountY,
		Int iCountZ)
	{
		auto const iTaskCount = (iCountX * iCountY * iCountZ);
		for (Int iZ = 0; iZ < iCountZ; ++iZ)
		{
			for (Int iY = 0; iY < iCountY; ++iY)
			{
				for (Int iX = 0; iX < iCountX; ++iX)
				{
					auto const iTaskIndex = (iX + iCountX * (iY + iCountY * iZ));
					SharedPtr<Task> pTask(SEOUL_NEW(MemoryBudgets::Cooking) Task(
						pFunc,
						pData,
						(Int)m_uThreadCount,
						iTaskIndex,
						iTaskCount,
						iX,
						iY,
						iZ,
						iCountX,
						iCountY,
						iCountZ));
					m_vTasks.PushBack(pTask);
				}
			}
		}

		// Kick all jobs.
		for (auto const& pTask : m_vTasks)
		{
			pTask->StartJob();
		}
	}

	void Sync()
	{
		for (auto const& pTask : m_vTasks)
		{
			pTask->WaitUntilJobIsNotRunning();
		}
		{
			Tasks vEmpty;
			m_vTasks.Swap(vEmpty);
		}

		for (auto const& p : m_vBlocks)
		{
			MemoryManager::Deallocate(p);
		}
		{
			Blocks vEmpty;
			m_vBlocks.Swap(vEmpty);
		}
	}

private:
	SEOUL_DISABLE_COPY(TaskGroup);

	UInt32 const m_uThreadCount;
	typedef Vector<void*, MemoryBudgets::Cooking> Blocks;
	Blocks m_vBlocks;
	typedef Vector<SharedPtr<Task>, MemoryBudgets::Cooking> Tasks;
	Tasks m_vTasks;
}; // class TaskGroup

} // namespace anonymous

static TaskGroup* Acquire(void** ppHandle)
{
	if (!(*ppHandle))
	{
		(*ppHandle) = SEOUL_NEW(MemoryBudgets::Cooking) TaskGroup;
	}

	return (TaskGroup*)(*ppHandle);
}

static void* ISPCAlloc(
	void** ppHandle,
	Int64 iSize,
	Int32 iAlignment)
{
	auto pTaskGroup = Acquire(ppHandle);
	return pTaskGroup->AllocateAligned(iSize, iAlignment);
}

static void ISPCLaunch(
	void** ppHandle,
	void* pFunc,
	void* pData,
	Int iCountX,
	Int iCountY,
	Int iCountZ)
{
	auto pTaskGroup = Acquire(ppHandle);
	pTaskGroup->Launch(pFunc, pData, iCountX, iCountY, iCountZ);
}

static void ISPCSync(void* pHandle)
{
	// Synchronize and deallocate.
	auto pTaskGroup = (TaskGroup*)pHandle;
	if (pTaskGroup)
	{
		pTaskGroup->Sync();
		SafeDelete(pTaskGroup);
	}
}

} // namespace Seoul::Cooking

// Integration with ISPC
extern "C"
{

void* ISPCAlloc(
	void** ppHandle,
	int64_t iSize,
	int32_t iAlignment)
{
	return Seoul::Cooking::ISPCAlloc(ppHandle, iSize, iAlignment);
}

void ISPCLaunch(
	void** ppHandle,
	void* pFunc,
	void* pData,
	int iCountX,
	int iCountY,
	int iCountZ)
{
	Seoul::Cooking::ISPCLaunch(ppHandle, pFunc, pData, iCountX, iCountY, iCountZ);
}

void ISPCSync(void* pHandle)
{
	Seoul::Cooking::ISPCSync(pHandle);
}

} // extern "C"
