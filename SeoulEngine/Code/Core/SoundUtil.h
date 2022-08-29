/**
 * \file SoundUtil.h
 * \brief Shared utility for loading some SeoulEngine format
 * audio system data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SOUND_UTIL_H
#define SOUND_UTIL_H

#include "FilePath.h"
#include "HashSet.h"
#include "HashTable.h"
#include "Prereqs.h"
#include "SeoulHString.h"
#include "SeoulString.h"
#include "Vector.h"
namespace Seoul { class StreamBuffer; }

namespace Seoul::SoundUtil
{

typedef Vector<FilePath, MemoryBudgets::Audio> BankFiles;
typedef HashSet<FilePath, MemoryBudgets::Audio> BankSet;
typedef HashTable<HString, BankSet, MemoryBudgets::Audio> EventDependencies;
Bool ReadBanksAndEvents(
	const String& sProjectFileDirectory,
	StreamBuffer& r,
	BankFiles& rvBankFiles,
	EventDependencies& rtEvents);

static const HString kStrings(".strings");

inline static Bool IsStringsBank(FilePath filePath)
{
	if (filePath.GetType() != FileType::kSoundBank) { return false; }

	auto const relative(filePath.GetRelativeFilenameWithoutExtension());
	return 0 == strcmp(relative.CStr() + relative.GetSizeInBytes() - kStrings.GetSizeInBytes(), kStrings.CStr());
}

Bool ReadAllAndObfuscate(FilePath filePath, void*& rp, UInt32& ru);

} // namespace Seoul::SoundUtil

#endif // include guard
