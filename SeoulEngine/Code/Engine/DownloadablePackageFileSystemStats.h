/**
 * \file DownloadablePackageFileSystemStats.h
 * \brief Tracking structure used for recording times and
 * events of a downloader.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DOWNLOADABLE_PACKAGE_FILE_SYSTEM_STATS_H
#define DOWNLOADABLE_PACKAGE_FILE_SYSTEM_STATS_H

#include "HashTable.h"

namespace Seoul
{
struct DownloadablePackageFileSystemStats SEOUL_SEALED
{
	typedef HashTable<HString, UInt32, MemoryBudgets::Io> Counts;
	typedef HashTable<HString, Int64, MemoryBudgets::Io> TimesInTicks;

	Counts m_tEvents;
	TimesInTicks m_tTimes;
}; // struct DownloadablePackageFileSystemStats

} // namespace Seoul

#endif // include guard
