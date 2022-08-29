/**
 * \file ScriptBenchmarks.cpp
 * \brief Declaration of script benchmarking tests.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FileManager.h"
#include "GamePaths.h"
#include "ReflectionAttributes.h"
#include "ReflectionDefine.h"
#include "ScriptBenchmarks.h"
#include "ScriptFunctionInvoker.h"
#include "ScriptManager.h"
#include "ScriptVm.h"
#include "UnitTesting.h"
#include "UnitTestsEngineHelper.h"

namespace Seoul
{

#if SEOUL_BENCHMARK_TESTS

#define SEOUL_BMETHOD(name) \
	SEOUL_METHOD(name)

SEOUL_BEGIN_TYPE(ScriptBenchmarks, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(BenchmarkTest)
	SEOUL_BMETHOD(AddNV)
	SEOUL_BMETHOD(AddVN)
	SEOUL_BMETHOD(AddVV)
	SEOUL_BMETHOD(DivNV)
	SEOUL_BMETHOD(DivVN)
	SEOUL_BMETHOD(DivVV)
	SEOUL_BMETHOD(ModNV)
	SEOUL_BMETHOD(ModVN)
	SEOUL_BMETHOD(ModVV)
	SEOUL_BMETHOD(MulNV)
	SEOUL_BMETHOD(MulVN)
	SEOUL_BMETHOD(MulVV)
	SEOUL_BMETHOD(SubNV)
	SEOUL_BMETHOD(SubVN)
	SEOUL_BMETHOD(SubVV)
	SEOUL_BMETHOD(I32AddNV)
	SEOUL_BMETHOD(I32AddVN)
	SEOUL_BMETHOD(I32AddVV)
	SEOUL_BMETHOD(I32DivNV)
	SEOUL_BMETHOD(I32DivVN)
	SEOUL_BMETHOD(I32DivVV)
	SEOUL_BMETHOD(I32ModNV)
	SEOUL_BMETHOD(I32ModVN)
	SEOUL_BMETHOD(I32ModVV)
	SEOUL_BMETHOD(I32MulNV)
	SEOUL_BMETHOD(I32MulVN)
	SEOUL_BMETHOD(I32MulVV)
	SEOUL_BMETHOD(I32SubNV)
	SEOUL_BMETHOD(I32SubVN)
	SEOUL_BMETHOD(I32SubVV)
	SEOUL_BMETHOD(I32Truncate)
	
	// No expected time.
	SEOUL_METHOD(FibI)
	SEOUL_METHOD(FibR)
	SEOUL_METHOD(Primes)

	SEOUL_METHOD(NativeAdd2N)
	SEOUL_METHOD(NativeAdd3N)
	SEOUL_METHOD(NativeAdd4N)
	SEOUL_METHOD(NativeAdd5N)

	SEOUL_METHOD(FFT)
	SEOUL_METHOD(SOR)
	SEOUL_METHOD(MC)
	SEOUL_METHOD(SPARSE)
	SEOUL_METHOD(LU)

	SEOUL_METHOD(BinaryTrees)
	SEOUL_METHOD(Fasta)
	SEOUL_METHOD(Nbody)
SEOUL_END_TYPE()

struct ScriptBenchmarkNativeTester
{
	Double Add2N(Double a, Double b)
	{
		return a + b;
	}

	Double Add3N(Double a, Double b, Double c)
	{
		return a + b + c;
	}

	Double Add4N(Double a, Double b, Double c, Double d)
	{
		return a + b + c + d;
	}

	Double Add5N(Double a, Double b, Double c, Double d, Double e)
	{
		return a + b + c + d + e;
	}
};
SEOUL_BEGIN_TYPE(ScriptBenchmarkNativeTester)
	SEOUL_METHOD(Add2N)
	SEOUL_METHOD(Add3N)
	SEOUL_METHOD(Add4N)
	SEOUL_METHOD(Add5N)
SEOUL_END_TYPE()

void ScriptBenchmarkTestLog(Byte const* s)
{
	SEOUL_LOG("%s", s);
}

void ScriptBenchmarkTestError(const CustomCrashErrorState& state)
{
	SEOUL_LOG("%s", state.m_sReason.CStr());
}

ScriptBenchmarks::ScriptBenchmarks()
	: m_pHelper(SEOUL_NEW(MemoryBudgets::Developer) UnitTestsEngineHelper)
	, m_pScriptManager(SEOUL_NEW(MemoryBudgets::Scripting) Script::Manager)
{
	Script::VmSettings settings;
	settings.m_StandardOutput = SEOUL_BIND_DELEGATE(ScriptBenchmarkTestLog);
	settings.m_ErrorHandler = SEOUL_BIND_DELEGATE(ScriptBenchmarkTestError);
	settings.m_vBasePaths.PushBack(Path::Combine(
		GamePaths::Get()->GetSourceDir(),
		"Authored/Scripts/"));
	auto const sBase(settings.m_vBasePaths.Back());

	m_pVm.Reset(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(settings));

	static Byte const* s_kaFiles[] =
	{
		"BinaryTrees.lua",
		"ComplexMath.lua",
		"Fasta.lua",
		"NativeInvoke.lua",
		"Nbody.lua",
		"SciMark.lua",
		"SimpleMath.lua",
	};

	for (auto const& s : s_kaFiles)
	{
		SEOUL_UNITTESTING_ASSERT(m_pVm->RunScript(
			Path::Combine("DevOnly/Benchmarks", s)));
	}
}

ScriptBenchmarks::~ScriptBenchmarks()
{
	m_pVm.Reset();
	m_pScriptManager.Reset();
	m_pHelper.Reset();
}

#define SEOUL_BENCH(name) \
	static const HString k##name(#name); \
	void ScriptBenchmarks::name(Int64 iIterations) { Benchmark(iIterations, k##name); }

SEOUL_BENCH(AddNV)
SEOUL_BENCH(AddVN)
SEOUL_BENCH(AddVV)
SEOUL_BENCH(DivNV)
SEOUL_BENCH(DivVN)
SEOUL_BENCH(DivVV)
SEOUL_BENCH(ModNV)
SEOUL_BENCH(ModVN)
SEOUL_BENCH(ModVV)
SEOUL_BENCH(MulNV)
SEOUL_BENCH(MulVN)
SEOUL_BENCH(MulVV)
SEOUL_BENCH(SubNV)
SEOUL_BENCH(SubVN)
SEOUL_BENCH(SubVV)

SEOUL_BENCH(I32AddNV)
SEOUL_BENCH(I32AddVN)
SEOUL_BENCH(I32AddVV)
SEOUL_BENCH(I32DivNV)
SEOUL_BENCH(I32DivVN)
SEOUL_BENCH(I32DivVV)
SEOUL_BENCH(I32ModNV)
SEOUL_BENCH(I32ModVN)
SEOUL_BENCH(I32ModVV)
SEOUL_BENCH(I32MulNV)
SEOUL_BENCH(I32MulVN)
SEOUL_BENCH(I32MulVV)
SEOUL_BENCH(I32SubNV)
SEOUL_BENCH(I32SubVN)
SEOUL_BENCH(I32SubVV)
SEOUL_BENCH(I32Truncate)

SEOUL_BENCH(FibI)
SEOUL_BENCH(FibR)
SEOUL_BENCH(Primes)

SEOUL_BENCH(NativeAdd2N)
SEOUL_BENCH(NativeAdd3N)
SEOUL_BENCH(NativeAdd4N)
SEOUL_BENCH(NativeAdd5N)

SEOUL_BENCH(FFT)
SEOUL_BENCH(SOR)
SEOUL_BENCH(MC)
SEOUL_BENCH(SPARSE)
SEOUL_BENCH(LU)

SEOUL_BENCH(BinaryTrees)
SEOUL_BENCH(Fasta)
SEOUL_BENCH(Nbody)

#undef SEOUL_BENCH

void ScriptBenchmarks::Benchmark(Int64 iIterations, HString name)
{
	Script::FunctionInvoker invoker(*m_pVm, name);
	invoker.PushNumber((Double)iIterations);
	SEOUL_UNITTESTING_ASSERT(invoker.TryInvoke());
}

#endif // /SEOUL_BENCHMARK_TESTS

} // namespace Seoul
