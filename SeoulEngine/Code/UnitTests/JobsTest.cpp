/**
 * \file JobsTest.cpp
 * \brief Unit tests for the Jobs library.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Atomic32.h"
#include "JobsFunction.h"
#include "JobsManager.h"
#include "JobsTest.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"
#include "UnitTestsEngineHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(JobsTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestJobContinuity)
	SEOUL_METHOD(TestJobYieldOnMainRegression)
	SEOUL_METHOD(TestMakeJobFunction)
	SEOUL_METHOD(TestCallJobFunction)
	SEOUL_METHOD(TestAwaitJobFunction)
SEOUL_END_TYPE()

struct FileThreadJob SEOUL_SEALED : public Jobs::Job
{
	struct DependentJob : public Jobs::Job
	{
		DependentJob(FileThreadJob& rOwner)
			: m_rOwner(rOwner)
		{
		}

		virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE
		{
			m_rOwner.m_bDependentDone = true;
			reNextState = Jobs::State::kComplete;
		}

	private:
		FileThreadJob& m_rOwner;

		~DependentJob()
		{
		}

		SEOUL_REFERENCE_COUNTED_SUBCLASS(DependentJob);
	};

	FileThreadJob()
		: Job(GetFileIOThreadId())
		, m_pDependentJob(nullptr)
		, m_iStage(0)
		, m_bDependentDone(false)
	{
		m_pDependentJob.Reset(SEOUL_NEW(MemoryBudgets::TBD) DependentJob(*this));
	}

	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE
	{
		SEOUL_UNITTESTING_ASSERT(IsFileIOThread());
		SEOUL_UNITTESTING_ASSERT(!IsMainThread());

		Int const iStage = m_iStage;
		++m_iStage;

		switch (iStage)
		{
		case 0:
			SEOUL_UNITTESTING_ASSERT(m_pDependentJob->GetJobState() == Jobs::State::kNotStarted);
			m_pDependentJob->StartJob();
			break;
		default:
			SEOUL_UNITTESTING_ASSERT(m_pDependentJob->WasJobStarted());
			if (!m_pDependentJob->IsJobRunning())
			{
				SEOUL_UNITTESTING_ASSERT(m_pDependentJob->GetJobState() == Jobs::State::kComplete);
				SEOUL_UNITTESTING_ASSERT(m_bDependentDone);
				reNextState = Jobs::State::kComplete;
			}
			break;
		};
	}

	SharedPtr<DependentJob> m_pDependentJob;
	Int m_iStage;
	Bool m_bDependentDone;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(FileThreadJob);

	~FileThreadJob()
	{}
};

struct MainThreadJob SEOUL_SEALED : public Jobs::Job
{
	MainThreadJob()
		: Job(GetMainThreadId())
	{
	}

	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE
	{
		SEOUL_UNITTESTING_ASSERT(IsMainThread());
		SEOUL_UNITTESTING_ASSERT(!IsFileIOThread());

		reNextState = Jobs::State::kComplete;
	}

private:
	~MainThreadJob()
	{}

	SEOUL_REFERENCE_COUNTED_SUBCLASS(MainThreadJob);
};

struct WorkerThreadJob SEOUL_SEALED : public Jobs::Job
{
	WorkerThreadJob()
		: Job(ThreadId())
		, m_iStage(0)
	{
	}

	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE
	{
		Int iStage = m_iStage;
		m_iStage++;

		switch (iStage)
		{
		case 0:
			SEOUL_UNITTESTING_ASSERT(!IsMainThread());
			SEOUL_UNITTESTING_ASSERT(!IsFileIOThread());
			reNextState = Jobs::State::kScheduledForOrRunning;
			rNextThreadId = GetMainThreadId();
			return;
		case 1:
			SEOUL_UNITTESTING_ASSERT(IsMainThread());
			SEOUL_UNITTESTING_ASSERT(!IsFileIOThread());
			reNextState = Jobs::State::kScheduledForOrRunning;
			rNextThreadId = GetFileIOThreadId();
			return;
		case 2:
			SEOUL_UNITTESTING_ASSERT(!IsMainThread());
			SEOUL_UNITTESTING_ASSERT(IsFileIOThread());
			reNextState = Jobs::State::kScheduledForOrRunning;
			rNextThreadId = ThreadId();
			return;
		case 3:
			reNextState = Jobs::State::kComplete;
			return;
		};

		reNextState = Jobs::State::kError;
	}

	Int m_iStage;

private:
	~WorkerThreadJob()
	{}

	SEOUL_REFERENCE_COUNTED_SUBCLASS(WorkerThreadJob);
};

void JobsTest::TestBasic()
{
	static const UInt32 kTestJobs = 1024u;

	// The false argument tells Jobs::Manager not to wait for an external render
	// thread on platforms that use one (iOS), since the unit test app
	// never spawns a separate thread for the "main" thread.
	ScopedPtr<Jobs::Manager> pJobManager(SEOUL_NEW(MemoryBudgets::TBD) Jobs::Manager);

	Vector< SharedPtr<Jobs::Job> > vJobs;
	for (UInt32 i = 0u; i < kTestJobs; ++i)
	{
		vJobs.PushBack(SharedPtr<Jobs::Job>(SEOUL_NEW(MemoryBudgets::TBD) FileThreadJob));
		pJobManager->Schedule(*vJobs.Back());
		vJobs.PushBack(SharedPtr<Jobs::Job>(SEOUL_NEW(MemoryBudgets::TBD) MainThreadJob));
		vJobs.Back()->StartJob();
		vJobs.PushBack(SharedPtr<Jobs::Job>(SEOUL_NEW(MemoryBudgets::TBD) WorkerThreadJob));
		pJobManager->Schedule(*vJobs.Back());
	}

	UInt32 const uCount = vJobs.GetSize();
	for (UInt32 i = 0u; i < uCount; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(Jobs::State::kNotStarted, vJobs[i]->GetJobState());
	}

	pJobManager.Reset();

	for (UInt32 i = 0u; i < uCount; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kComplete, vJobs[i]->GetJobState());
	}
	vJobs.Clear();

	SetFileIOThreadId(Thread::GetThisThreadId());
}

struct ContinuityTestJob SEOUL_SEALED : public Jobs::Job
{
	ContinuityTestJob()
	{
	}

	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE
	{
		static const UInt32 kuYields = 512u;

		SEOUL_UNITTESTING_ASSERT(!IsMainThread());
		SEOUL_UNITTESTING_ASSERT(!IsFileIOThread());

		SEOUL_UNITTESTING_ASSERT(!m_LastThreadId.IsValid());
		m_LastThreadId = Thread::GetThisThreadId();

		for (UInt32 i = 0u; i < kuYields; ++i)
		{
			Jobs::Manager::Get()->YieldThreadTime();
			SEOUL_UNITTESTING_ASSERT_EQUAL(m_LastThreadId, Thread::GetThisThreadId());
		}

		reNextState = Jobs::State::kComplete;
	}

private:
	~ContinuityTestJob()
	{}

	ThreadId m_LastThreadId;

	SEOUL_REFERENCE_COUNTED_SUBCLASS(MainThreadJob);
};

void JobsTest::TestJobContinuity()
{
	// On some platforms (iOS, due to auto-release pools), it is
	// required that once a job runs on a thread,
	// it continues to run on that thread when context-swapped from a Yield().
	// Meaning, the only way a Job can switch thread contexts is if it
	// explicitly requests it by returning from its run job method. This
	// test checks for this behavior.

	static const UInt32 kTestJobs = 128;

	// The false argument tells Jobs::Manager not to wait for an external render
	// thread on platforms that use one (iOS), since the unit test app
	// never spawns a separate thread for the "main" thread.
	ScopedPtr<Jobs::Manager> pJobManager(SEOUL_NEW(MemoryBudgets::TBD) Jobs::Manager);

	Vector< SharedPtr<Jobs::Job> > vJobs;
	for (UInt32 i = 0u; i < kTestJobs; ++i)
	{
		vJobs.PushBack(SharedPtr<Jobs::Job>(SEOUL_NEW(MemoryBudgets::TBD) ContinuityTestJob));
		pJobManager->Schedule(*vJobs.Back());
	}

	UInt32 const uCount = vJobs.GetSize();
	for (UInt32 i = 0u; i < uCount; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(Jobs::State::kNotStarted, vJobs[i]->GetJobState());
	}
	for (UInt32 i = 0u; i < uCount; ++i)
	{
		vJobs[i]->WaitUntilJobIsNotRunning();
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kComplete, vJobs[i]->GetJobState());
	}

	pJobManager.Reset();

	for (UInt32 i = 0u; i < uCount; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kComplete, vJobs[i]->GetJobState());
	}
	vJobs.Clear();

	SetFileIOThreadId(Thread::GetThisThreadId());
}

namespace
{

struct Util
{
	SEOUL_DELEGATE_TARGET(Util);

	Atomic32 m_iCounter;
	void JobBody()
	{
		for (Int i = 0; i < 1000; ++i)
		{
			++m_iCounter;
			Jobs::Manager::Get()->YieldThreadTime();
		}
	}
};

} // namespace anonymous

// Regression for an oversight that prevented
// proper yield back to the main thread if a Job,
// instead of returning, called Jobs::Manager::Get()->Yield()
// while it was waiting for work to complete. This
// could deadlock or hitch the main thread.
void JobsTest::TestJobYieldOnMainRegression()
{
	{
		Jobs::Manager jobManager;

		Util util;
		SharedPtr<Jobs::Job> p(Jobs::MakeFunction(
			GetMainThreadId(),
			SEOUL_BIND_DELEGATE(&Util::JobBody, &util)));
		p->StartJob();

		// Increment with each successful run.
		while (p->IsJobRunning())
		{
			if (Jobs::Manager::Get()->YieldThreadTime())
			{
				++util.m_iCounter;
			}
		}

		// Due to how job scheduling works, we will always
		// yield at least 2000 times (if all working as expected)
		// but may yield more times - some yields may not actually
		// execute the job.
		SEOUL_UNITTESTING_ASSERT_EQUAL(2001, util.m_iCounter);
	}

	// Restore file IO thread association.
	SetFileIOThreadId(Thread::GetThisThreadId());
}

void JobsTest::TestMakeJobFunction()
{
	Jobs::Manager jobManager;

	// Jobs::MakeFunction(ThreadId threadId, FUNC&& func)
	{
		auto pJob(Jobs::MakeFunction(GetMainThreadId(), []() {}));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT(pJob.IsUnique());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kNotStarted, pJob->GetJobState());
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(!pJob->IsJobRunning());
		SEOUL_UNITTESTING_ASSERT(!pJob->WasJobStarted());
	}

	// Jobs::MakeFunction(FUNC&& func)
	{
		auto pJob(Jobs::MakeFunction([]() {}));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT(pJob.IsUnique());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kNotStarted, pJob->GetJobState());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(!pJob->IsJobRunning());
		SEOUL_UNITTESTING_ASSERT(!pJob->WasJobStarted());
	}

	// Jobs::MakeFunction(ThreadId threadId, FUNC&& func, ARGS... args)
	{
		auto pJob(Jobs::MakeFunction(GetMainThreadId(), [](Int a) {}, 1));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT(pJob.IsUnique());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kNotStarted, pJob->GetJobState());
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(!pJob->IsJobRunning());
		SEOUL_UNITTESTING_ASSERT(!pJob->WasJobStarted());
	}

	// Jobs::MakeFunction(ThreadId threadId, FUNC&& func, ARGS... args)
	{
		auto pJob(Jobs::MakeFunction(GetMainThreadId(), [](Int a, Byte const*) {}, 1, "Hello World"));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT(pJob.IsUnique());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kNotStarted, pJob->GetJobState());
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(!pJob->IsJobRunning());
		SEOUL_UNITTESTING_ASSERT(!pJob->WasJobStarted());
	}

	// Jobs::MakeFunction(ThreadId threadId, FUNC&& func, ARGS... args)
	{
		auto pJob(Jobs::MakeFunction(GetMainThreadId(), [](Int a, Byte const*, const String&) {}, 1, "Hello World", "Goodbye World"));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT(pJob.IsUnique());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kNotStarted, pJob->GetJobState());
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(!pJob->IsJobRunning());
		SEOUL_UNITTESTING_ASSERT(!pJob->WasJobStarted());
	}

	// Jobs::MakeFunction(FUNC&& func, ARGS... args)
	{
		auto pJob(Jobs::MakeFunction([](Int a) {}, 1));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT(pJob.IsUnique());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kNotStarted, pJob->GetJobState());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(!pJob->IsJobRunning());
		SEOUL_UNITTESTING_ASSERT(!pJob->WasJobStarted());
	}

	// Jobs::MakeFunction(FUNC&& func, ARGS... args)
	{
		auto pJob(Jobs::MakeFunction([](Int a, Byte const*) {}, 1, "Hello World"));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT(pJob.IsUnique());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kNotStarted, pJob->GetJobState());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(!pJob->IsJobRunning());
		SEOUL_UNITTESTING_ASSERT(!pJob->WasJobStarted());
	}

	// Jobs::MakeFunction(FUNC&& func, ARGS... args)
	{
		auto pJob(Jobs::MakeFunction([](Int a, Byte const*, const String&) {}, 1, "Hello World", "Goodbye World"));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT(pJob.IsUnique());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kNotStarted, pJob->GetJobState());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(!pJob->IsJobRunning());
		SEOUL_UNITTESTING_ASSERT(!pJob->WasJobStarted());
	}
}

void JobsTest::TestCallJobFunction()
{
	Jobs::Manager jobManager;

	// Jobs::AsyncFunction(ThreadId threadId, FUNC&& func)
	{
		Atomic32Value<Bool> bRun;
		auto pJob(Jobs::AsyncFunction(GetMainThreadId(), [&bRun]() { bRun = true; }));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(pJob->WasJobStarted());

		pJob->WaitUntilJobIsNotRunning();
		SEOUL_UNITTESTING_ASSERT(bRun);
	}

	// Jobs::AsyncFunction(FUNC&& func)
	{
		Atomic32Value<Bool> bRun;
		auto pJob(Jobs::AsyncFunction([&bRun]() { bRun = true; }));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(pJob->WasJobStarted());

		pJob->WaitUntilJobIsNotRunning();
		SEOUL_UNITTESTING_ASSERT(bRun);
	}

	// Jobs::AsyncFunction(ThreadId threadId, FUNC&& func, ARGS... args)
	{
		Atomic32Value<Int32> iRun;
		auto pJob(Jobs::AsyncFunction(GetMainThreadId(), [](Atomic32Value<Int32>* piValue) { (*piValue) = 7; }, &iRun));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(pJob->WasJobStarted());

		pJob->WaitUntilJobIsNotRunning();
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, (Int32)iRun);
	}

	// Jobs::AsyncFunction(ThreadId threadId, FUNC&& func, ARGS... args)
	{
		Atomic32Value<Int32> iRun;
		auto pJob(Jobs::AsyncFunction(GetMainThreadId(), [](Atomic32Value<Int32>* piValue, Int32 iValue) { (*piValue) = iValue; }, &iRun, 7));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(pJob->WasJobStarted());

		pJob->WaitUntilJobIsNotRunning();
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, (Int32)iRun);
	}

	// Jobs::AsyncFunction(FUNC&& func, ARGS... args)
	{
		Atomic32Value<Int32> iRun;
		auto pJob(Jobs::AsyncFunction([](Atomic32Value<Int32>* piValue) { (*piValue) = 7; }, &iRun));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(pJob->WasJobStarted());

		pJob->WaitUntilJobIsNotRunning();
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, (Int32)iRun);
	}

	// Jobs::AsyncFunction(FUNC&& func, ARGS... args)
	{
		Atomic32Value<Int32> iRun;
		auto pJob(Jobs::AsyncFunction([](Atomic32Value<Int32>* piValue, Int32 iValue) { (*piValue) = iValue; }, &iRun, 7));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(pJob->WasJobStarted());

		pJob->WaitUntilJobIsNotRunning();
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, (Int32)iRun);
	}
}

void JobsTest::TestAwaitJobFunction()
{
	Jobs::Manager jobManager;

	// Jobs::AwaitFunction(ThreadId threadId, FUNC&& func)
	{
		Atomic32Value<Bool> bRun;
		auto pJob(Jobs::AwaitFunction(GetMainThreadId(), [&bRun]() { bRun = true; }));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kComplete, pJob->GetJobState());
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(!pJob->IsJobRunning());
		SEOUL_UNITTESTING_ASSERT(pJob->WasJobStarted());
		SEOUL_UNITTESTING_ASSERT(bRun);
	}

	// Jobs::AwaitFunction(FUNC&& func)
	{
		Atomic32Value<Bool> bRun;
		auto pJob(Jobs::AwaitFunction([&bRun]() { bRun = true; }));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kComplete, pJob->GetJobState());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(!pJob->IsJobRunning());
		SEOUL_UNITTESTING_ASSERT(pJob->WasJobStarted());
		SEOUL_UNITTESTING_ASSERT(bRun);
	}

	// Jobs::AwaitFunction(ThreadId threadId, FUNC&& func, ARGS... args)
	{
		Atomic32Value<Int32> iRun;
		auto pJob(Jobs::AwaitFunction(GetMainThreadId(), [](Atomic32Value<Int32>* piValue) { (*piValue) = 7; }, &iRun));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kComplete, pJob->GetJobState());
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(!pJob->IsJobRunning());
		SEOUL_UNITTESTING_ASSERT(pJob->WasJobStarted());
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, (Int32)iRun);
	}

	// Jobs::AwaitFunction(ThreadId threadId, FUNC&& func, ARGS... args)
	{
		Atomic32Value<Int32> iRun;
		auto pJob(Jobs::AwaitFunction(GetMainThreadId(), [](Atomic32Value<Int32>* piValue, Int32 iValue) { (*piValue) = iValue; }, &iRun, 7));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kComplete, pJob->GetJobState());
		SEOUL_UNITTESTING_ASSERT_EQUAL(GetMainThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(!pJob->IsJobRunning());
		SEOUL_UNITTESTING_ASSERT(pJob->WasJobStarted());
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, (Int32)iRun);
	}

	// Jobs::AwaitFunction(FUNC&& func, ARGS... args)
	{
		Atomic32Value<Int32> iRun;
		auto pJob(Jobs::AwaitFunction([](Atomic32Value<Int32>* piValue) { (*piValue) = 7; }, &iRun));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kComplete, pJob->GetJobState());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(!pJob->IsJobRunning());
		SEOUL_UNITTESTING_ASSERT(pJob->WasJobStarted());
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, (Int32)iRun);
	}

	// Jobs::AwaitFunction(FUNC&& func, ARGS... args)
	{
		Atomic32Value<Int32> iRun;
		auto pJob(Jobs::AwaitFunction([](Atomic32Value<Int32>* piValue, Int32 iValue) { (*piValue) = iValue; }, &iRun, 7));
		SEOUL_UNITTESTING_ASSERT(pJob.IsValid());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::Quantum::kDefault, pJob->GetJobQuantum());
		SEOUL_UNITTESTING_ASSERT_EQUAL(Jobs::State::kComplete, pJob->GetJobState());
		SEOUL_UNITTESTING_ASSERT_EQUAL(ThreadId(), pJob->GetThreadId());
		SEOUL_UNITTESTING_ASSERT(!pJob->IsJobRunning());
		SEOUL_UNITTESTING_ASSERT(pJob->WasJobStarted());
		SEOUL_UNITTESTING_ASSERT_EQUAL(7, (Int32)iRun);
	}
}

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul
