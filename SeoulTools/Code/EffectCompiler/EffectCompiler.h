/**
 * \file EffectCompiler.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EFFECT_COMPILER_H
#define EFFECT_COMPILER_H

#include "FilePath.h"
#include "HashSet.h"
#include "HashTable.h"
#include "Prereqs.h"
#include "SeoulHString.h"
#include "SeoulString.h"

namespace Seoul
{

enum class EffectTarget
{
	D3D9,
	D3D11,
	GLSLES2,
};

typedef HashTable<HString, String, MemoryBudgets::Cooking> MacroTable;

Bool CompileEffectFile(
	EffectTarget eTarget,
	FilePath filePath,
	const MacroTable& tMacros,
	void*& rp,
	UInt32& ru);

typedef HashSet<FilePath, MemoryBudgets::Cooking> EffectFileDependencies;

Bool GetEffectFileDependencies(
	FilePath filePath,
	const MacroTable& tMacros,
	EffectFileDependencies& rSet);

} // namespace Seoul

#endif // include guard
