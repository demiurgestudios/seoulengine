/**
 * \file SccIClient.cpp
 * \brief Abstract interface to a source control client provider.
 * Provides a generalized interface to various source control backends.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "SccIClient.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(Scc::FileTypeOptions::BaseFileType)
	SEOUL_ENUM_N("DefaultType", Scc::FileTypeOptions::kDefaultType)
	SEOUL_ENUM_N("Text", Scc::FileTypeOptions::kText)
	SEOUL_ENUM_N("Binary", Scc::FileTypeOptions::kBinary)
	SEOUL_ENUM_N("Symlink", Scc::FileTypeOptions::kSymlink)
	SEOUL_ENUM_N("Apple", Scc::FileTypeOptions::kApple)
	SEOUL_ENUM_N("Resource", Scc::FileTypeOptions::kResource)
	SEOUL_ENUM_N("Unicode", Scc::FileTypeOptions::kUnicode)
	SEOUL_ENUM_N("Utf16", Scc::FileTypeOptions::kUtf16)
SEOUL_END_ENUM()

SEOUL_BEGIN_ENUM(Scc::FileTypeOptions::FileTypeModifier)
	SEOUL_ENUM_N("None", Scc::FileTypeOptions::kNone)
	SEOUL_ENUM_N("AlwaysWriteable", Scc::FileTypeOptions::kAlwaysWriteable)
	SEOUL_ENUM_N("ExecuteBit", Scc::FileTypeOptions::kExecuteBit)
	SEOUL_ENUM_N("RcsKeywordExpansion", Scc::FileTypeOptions::kRcsKeywordExpansion)
	SEOUL_ENUM_N("ExclusiveOpen", Scc::FileTypeOptions::kExclusiveOpen)
	SEOUL_ENUM_N("StoreCompressedVersionOfEachRevision", Scc::FileTypeOptions::kStoreCompressedVersionOfEachRevision)
	SEOUL_ENUM_N("StoreDeltasInRcsFormat", Scc::FileTypeOptions::kStoreDeltasInRcsFormat)
	SEOUL_ENUM_N("StoreUncompressedVersionOfEachRevision", Scc::FileTypeOptions::kStoreUncompressedVersionOfEachRevision)
	SEOUL_ENUM_N("PreserveModificationTime", Scc::FileTypeOptions::kPreserveModificationTime)
	SEOUL_ENUM_N("ArchiveTriggerRequired", Scc::FileTypeOptions::kArchiveTriggerRequired)
SEOUL_END_ENUM()

SEOUL_BEGIN_ENUM(Scc::FileTypeOptions::NumberOfRevisions)
	SEOUL_ENUM_N("Unlimited", Scc::FileTypeOptions::kUnlimited)
	SEOUL_ENUM_N("1", Scc::FileTypeOptions::k1)
	SEOUL_ENUM_N("2", Scc::FileTypeOptions::k2)
	SEOUL_ENUM_N("3", Scc::FileTypeOptions::k3)
	SEOUL_ENUM_N("4", Scc::FileTypeOptions::k4)
	SEOUL_ENUM_N("5", Scc::FileTypeOptions::k5)
	SEOUL_ENUM_N("6", Scc::FileTypeOptions::k6)
	SEOUL_ENUM_N("7", Scc::FileTypeOptions::k7)
	SEOUL_ENUM_N("8", Scc::FileTypeOptions::k8)
	SEOUL_ENUM_N("9", Scc::FileTypeOptions::k9)
	SEOUL_ENUM_N("10", Scc::FileTypeOptions::k10)
	SEOUL_ENUM_N("16", Scc::FileTypeOptions::k16)
	SEOUL_ENUM_N("32", Scc::FileTypeOptions::k32)
	SEOUL_ENUM_N("64", Scc::FileTypeOptions::k64)
	SEOUL_ENUM_N("128", Scc::FileTypeOptions::k128)
	SEOUL_ENUM_N("256", Scc::FileTypeOptions::k256)
	SEOUL_ENUM_N("512", Scc::FileTypeOptions::k512)
SEOUL_END_ENUM()

} // namespace Seoul
