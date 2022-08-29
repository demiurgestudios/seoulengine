/**
 * \file ScriptBenchmarks.h
 * \brief Declaration of script benchmarking tests.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_BENCHMARKS_H
#define SCRIPT_BENCHMARKS_H

#include "Prereqs.h"
namespace Seoul { class UnitTestsEngineHelper; }
namespace Seoul { namespace Script { class Manager; } }
namespace Seoul { namespace Script { class Vm; } }

namespace Seoul
{

#if SEOUL_BENCHMARK_TESTS

class ScriptBenchmarks SEOUL_SEALED
{
public:
	ScriptBenchmarks();
	~ScriptBenchmarks();

	void AddNV(Int64 iIterations);
	void AddVN(Int64 iIterations);
	void AddVV(Int64 iIterations);
	void DivNV(Int64 iIterations);
	void DivVN(Int64 iIterations);
	void DivVV(Int64 iIterations);
	void ModNV(Int64 iIterations);
	void ModVN(Int64 iIterations);
	void ModVV(Int64 iIterations);
	void MulNV(Int64 iIterations);
	void MulVN(Int64 iIterations);
	void MulVV(Int64 iIterations);
	void SubNV(Int64 iIterations);
	void SubVN(Int64 iIterations);
	void SubVV(Int64 iIterations);

	void I32AddNV(Int64 iIterations);
	void I32AddVN(Int64 iIterations);
	void I32AddVV(Int64 iIterations);
	void I32DivNV(Int64 iIterations);
	void I32DivVN(Int64 iIterations);
	void I32DivVV(Int64 iIterations);
	void I32ModNV(Int64 iIterations);
	void I32ModVN(Int64 iIterations);
	void I32ModVV(Int64 iIterations);
	void I32MulNV(Int64 iIterations);
	void I32MulVN(Int64 iIterations);
	void I32MulVV(Int64 iIterations);
	void I32SubNV(Int64 iIterations);
	void I32SubVN(Int64 iIterations);
	void I32SubVV(Int64 iIterations);
	void I32Truncate(Int64 iIterations);

	void FibI(Int64 iIterations);
	void FibR(Int64 iIterations);
	void NativeAdd2N(Int64 iIterations);
	void NativeAdd3N(Int64 iIterations);
	void NativeAdd4N(Int64 iIterations);
	void NativeAdd5N(Int64 iIterations);
	void Primes(Int64 iIterations);

	void FFT(Int64 iIterations);
	void SOR(Int64 iIterations);
	void MC(Int64 iIterations);
	void SPARSE(Int64 iIterations);
	void LU(Int64 iIterations);

	void BinaryTrees(Int64 iIterations);
	void Fasta(Int64 iIterations);
	void Nbody(Int64 iIterations);

private:
	SEOUL_DISABLE_COPY(ScriptBenchmarks);

	void Benchmark(Int64 iIterations, HString name);

	ScopedPtr<UnitTestsEngineHelper> m_pHelper;
	ScopedPtr<Script::Manager> m_pScriptManager;
	SharedPtr<Script::Vm> m_pVm;
}; // class ScriptBenchmarks

#endif // /SEOUL_BENCHMARK_TESTS

} // namespace Seoul

#endif // include guard
