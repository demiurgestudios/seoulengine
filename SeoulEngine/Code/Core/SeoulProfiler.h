/**
 * \file SeoulProfiler.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_PROFILER_H
#define SEOUL_PROFILER_H

#include "Prereqs.h"
#include "SeoulHString.h"


namespace Seoul::Profiler
{

#ifdef SEOUL_PROFILING_BUILD

#define SEOUL_PROF_ENABLED 1

void BeginSample(HString id);
void EndSample();
Int64 GetTicks(HString id);

struct ProfilerScopeEnd { ~ProfilerScopeEnd() { EndSample(); } };

void LogCurrent(HString rootId = HString(), Double fMinMs = 0.05);

#define SEOUL_PROF_BEGIN_(name, counter) { \
	static const HString seoulSampleId_##counter(name); \
	Seoul::Profiler::BeginSample(seoulSampleId_##counter); }

#define SEOUL_PROF_(name, counter) \
	SEOUL_PROF_BEGIN_(name, counter) \
	Seoul::Profiler::ProfilerScopeEnd SEOUL_CONCAT(seoulSampleScopeEnd_, counter)

#define SEOUL_PROF_VAR_(var, counter) \
	Seoul::Profiler::BeginSample(var); \
	Seoul::Profiler::ProfilerScopeEnd seoulSampleScopedEnd_##counter

// Explicit profile start - allocates a static variable for tracking.
#define SEOUL_PROF_BEGIN(name) SEOUL_PROF_BEGIN_(name, __COUNTER__)

// Explicit profile end.
#define SEOUL_PROF_END() Seoul::Profiler::EndSample()

// Scoped profiling, name only. Allocates a static variable for tracking.
#define SEOUL_PROF(name) SEOUL_PROF_(name, __COUNTER__)

// Define a member variable to be used for dynamically
#define SEOUL_PROF_DEF_VAR(name) HString name;

// Separate macro to set the name of a defined profiling variable.
#define SEOUL_PROF_INIT_VAR(name, label) name = HString(label)

// Scoped profiling with explicit counter variable. Must be a UInt32.
#define SEOUL_PROF_VAR(var) SEOUL_PROF_VAR_(var, __COUNTER__)

// Emit all current profiling data (for the current thread) to stdout.
#define SEOUL_PROF_LOG_CURRENT(...) Seoul::Profiler::LogCurrent(__VA_ARGS__)

// Find the first sample (on the current thread) with the given name
// and return its current ticks value.
#define SEOUL_PROF_TICKS(name) Seoul::Profiler::GetTicks(HString(name))

#else

#define SEOUL_PROF_ENABLED 0

#define SEOUL_PROF_BEGIN(name) ((void)0)
#define SEOUL_PROF_END(name) ((void)0)
#define SEOUL_PROF(name) ((void)0)
#define SEOUL_PROF_DEF_VAR(name)
#define SEOUL_PROF_INIT_VAR(name, label) ((void)0)
#define SEOUL_PROF_VAR(name) ((void)0)
#define SEOUL_PROF_LOG_CURRENT(...) ((void)0)
#define SEOUL_PROF_TICKS(name) Seoul::Int64(0)

#endif

} // namespace Seoul::Profiler

#endif  // Include guard
