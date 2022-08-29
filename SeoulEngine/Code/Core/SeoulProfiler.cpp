/**
 * \file SeoulProfiler.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HeapAllocatedPerThreadStorage.h"
#include "ScopedAction.h"
#include "SeoulProfiler.h"
#include "SeoulTime.h"

#if SEOUL_PROF_ENABLED

namespace Seoul
{

namespace Profiler
{

namespace
{

struct Sample SEOUL_SEALED
{
	Int64 m_iStart{};
	Int64 m_iEnd{};
	HString m_Id{};
	UInt32 m_uCalls{};
	UInt16 m_uParent{};
	UInt16 m_uFirstChild{};
	UInt16 m_uLastChild{};
	UInt16 m_uNextSibling{};
};

} // namespace anonymous

} // namespace Profiler

template <> struct CanMemCpy<Profiler::Sample> { static const Bool Value = true; };
template <> struct CanZeroInit<Profiler::Sample> { static const Bool Value = true; };

namespace Profiler
{

namespace
{

struct PerThreadData SEOUL_SEALED
{
	explicit PerThreadData(Atomic32Type index)
		: m_ThreadIndex(index)
	{
	}
	~PerThreadData() = default;

	Atomic32Type const m_ThreadIndex;
	typedef Vector<Sample, MemoryBudgets::Profiler> Samples;
	Samples m_vSamples{};
	UInt16 m_uCurrent{};

	Sample const* Find(Sample const* p, HString id) const
	{
		if (p->m_Id == id) { return p; }

		for (auto pChild = GetSample(p->m_uFirstChild); nullptr != pChild; pChild = GetSample(pChild->m_uNextSibling))
		{
			auto pReturn = Find(pChild, id);
			if (nullptr != pReturn) { return pReturn; }
		}

		return nullptr;
	}

	Sample const* Find(HString id) const
	{
		auto p(FindCurrentTreeRoot());
		if (nullptr == p) { return nullptr; }

		return Find(p, id);
	}

	Sample const* FindCurrentTreeRoot() const
	{
		auto pRoot(GetSample(m_uCurrent));
		while (nullptr != pRoot)
		{
			auto pParent(GetSample(pRoot->m_uParent));
			if (nullptr == pParent)
			{
				break;
			}

			pRoot = pParent;
		}

		return pRoot;
	}

	Sample const* GetRoot(HString id = HString()) const
	{
		// First walk from current to the topmost root.
		auto pRoot(FindCurrentTreeRoot());

		// Now, if id is not empty, traverse for the first
		// hit against the given id.
		if (nullptr == pRoot || id.IsEmpty()) { return pRoot; }

		return Find(pRoot, id);
	}

	Sample const* GetSample(UInt16 u) const
	{
		return (0 == u ? nullptr : m_vSamples.Get(u - 1u));
	}

	Sample* GetSample(UInt16 u)
	{
		return (0 == u ? nullptr : m_vSamples.Get(u - 1u));
	}

	void Pop()
	{
		auto p(GetSample(m_uCurrent));
		if (nullptr == p) { return; }
		m_uCurrent = p->m_uParent;

		// Once we've gone back up the tree, we're starting a new frame.
		if (0u == m_uCurrent) { m_vSamples.Clear(); }
	}

	void Push(HString id)
	{
		// Absolute first, get the time.
		auto const iTicks = SeoulTime::GetGameTimeInTicks();

		// Populate first, we don't want to resize
		// this for the rest of the block since
		// we'll be acquiring and caching
		// pointers.
		//
		// Index is 1-based, so we can use 0u as a null equivalent.
		auto const uIndex = m_vSamples.GetSize() + 1u;
		m_vSamples.Resize(uIndex);

		// Get parent and potentially last sibling (prev).
		auto pParent(GetSample(m_uCurrent));
		auto pPrev(GetSample(nullptr == pParent ? 0u : pParent->m_uLastChild));

		// If our immediately sibling matches our id, aggregate. Pop the back
		// and just return pPrev.
		if (pPrev && pPrev->m_Id == id)
		{
			// Blank out end before returning.
			pPrev->m_iEnd = 0;

			// Increment call count.
			++pPrev->m_uCalls;

			// Update current.
			m_uCurrent = pParent->m_uLastChild;

			// Pop back.
			m_vSamples.PopBack();

			// Done.
			return;
		}

		// Setup a new sample.
		auto& sample = m_vSamples.Back();
		sample.m_uCalls = 1u;
		sample.m_Id = id;
		sample.m_uParent = m_uCurrent;
		sample.m_iStart = iTicks;

		// Hook into the tree.
		if (pParent)
		{
			if (0u == pParent->m_uFirstChild) { pParent->m_uFirstChild = uIndex; }
			if (pPrev) { pPrev->m_uNextSibling = uIndex; }
			pParent->m_uLastChild = uIndex;
		}

		m_uCurrent = uIndex;
	}
};

} // namespace anonymous

static HeapAllocatedPerThreadStorage<PerThreadData, 256u, MemoryBudgets::Profiler> s_PerThread;

void BeginSample(HString id)
{
	auto& t = s_PerThread.Get();
	t.Push(id);
}

Int64 GetTicks(HString id)
{
	auto const& t = s_PerThread.Get();
	auto p(t.Find(id));
	if (nullptr == p) { return 0; }

	return Max(p->m_iEnd - p->m_iStart, (Int64)0);
}

void EndSample()
{
	auto& t = s_PerThread.Get();
	{
		auto p(t.GetSample(t.m_uCurrent));
		p->m_iEnd = SeoulTime::GetGameTimeInTicks();
	}
	t.Pop();
}

static const String ksIndentation("  ");

static void LogSample(const PerThreadData& t, const Sample& sample, Int64 iMinTicks, const String& sIndentation)
{
	// Early out if less than 0.01 ms.
	auto const iDiff = (sample.m_iEnd - sample.m_iStart);
	if (iDiff < iMinTicks)
	{
		return;
	}

	auto const fMs = SeoulTime::ConvertTicksToMilliseconds(iDiff);

	PlatformPrint::PrintStringFormatted(
		PlatformPrint::Type::kInfo,
		"Performance: %s%s(%.2f ms, %u)\n",
		sIndentation.CStr(),
		sample.m_Id.CStr(),
		fMs,
		sample.m_uCalls);

	// Now long children with greater indentation.
	auto const sNested(sIndentation + ksIndentation);
	for (auto pChild = t.GetSample(sample.m_uFirstChild); nullptr != pChild; pChild = t.GetSample(pChild->m_uNextSibling))
	{
		LogSample(t, *pChild, iMinTicks, sNested);
	}
}

// Emit current thread data to stdout.
void LogCurrent(HString rootId /*= HString()*/, Double fMinMs /*= 0.05*/)
{
	auto const& t = s_PerThread.Get();

	auto const iMinTicks = SeoulTime::ConvertMillisecondsToTicks(fMinMs);

	// Log starting at the specified root.
	auto pRoot(t.GetRoot(rootId));

	// Early out if no root.
	if (nullptr == pRoot)
	{
		return;
	}

	// Enumerate root children.
	for (auto pChild = t.GetSample(pRoot->m_uFirstChild); nullptr != pChild; pChild = t.GetSample(pChild->m_uNextSibling))
	{
		LogSample(t, *pChild, iMinTicks, String());
	}
}

} // namespace Profiler

} // namespace Seoul

#endif
