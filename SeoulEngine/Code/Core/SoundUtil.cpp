/**
 * \file SoundUtil.cpp
 * \brief Shared utility for loading some SeoulEngine format
 * audio system data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DiskFileSystem.h"
#include "FileManager.h"
#include "SoundUtil.h"
#include "StreamBuffer.h"

namespace Seoul::SoundUtil
{

Bool ReadBanksAndEvents(
	const String& sProjectFileDirectory,
	StreamBuffer& r, 
	BankFiles& rvBankFiles, 
	EventDependencies& rtEvents)
{
	// Local variables for reach - don't want to store partial results.
	BankFiles vBankFiles;
	EventDependencies tEvents;

	// Consume all data.
	UInt32 uBanks = 0u;
	if (!r.ReadLittleEndian32(uBanks)) { return false; }
	for (UInt32 i = 0u; i < uBanks; ++i)
	{
		// Banks are first.
		String sBankBaseFilename;
		if (!r.Read(sBankBaseFilename)) { return false; }

		// Add.
		vBankFiles.PushBack(FilePath::CreateContentFilePath(Path::Combine(sProjectFileDirectory, sBankBaseFilename)));
	}

	// Read events.
	UInt32 uEvents = 0u;
	if (!r.ReadLittleEndian32(uEvents)) { return false; }
	for (auto i = 0u; i < uEvents; ++i)
	{
		// Event name.
		String sName;
		if (!r.Read(sName)) { return false; }

		// Number of file id dependencies.
		UInt32 uDeps = 0u;
		if (!r.ReadLittleEndian32(uDeps)) { return false; }

		// Get the entry - use an insert, since we accumulate
		// all results (events can have dependencies split
		// across banks).
		auto const e = tEvents.Insert(HString(sName, true), BankSet());

		// Now resolve file dependencies.
		for (auto u = 0u; u < uDeps; ++u)
		{
			String sBankBaseFilename;
			if (!r.Read(sBankBaseFilename)) { return false; }

			// Add.
			e.First->Second.Insert(FilePath::CreateContentFilePath(Path::Combine(sProjectFileDirectory, sBankBaseFilename)));
		}
	}
	// Done.
	rvBankFiles.Swap(vBankFiles);
	rtEvents.Swap(tEvents);
	return true;
}

Bool ReadAllAndObfuscate(FilePath filePath, void*& rp, UInt32& ru)
{
	// Read the data.
	if (!FileManager::Get())
	{
		if (!DiskSyncFile::ReadAll(filePath, rp, ru, 0u, MemoryBudgets::Audio))
		{
			return false;
		}
	}
	else if (!FileManager::Get()->ReadAll(filePath, rp, ru, 0u, MemoryBudgets::Audio))
	{
		return false;
	}

	// Get the base filename.
	String const s(Path::GetFileNameWithoutExtension(filePath.GetRelativeFilenameWithoutExtension().ToString()));

	UInt32 uXorKey = 0xFF7C3080;
	for (UInt32 i = 0u; i < s.GetSize(); ++i)
	{
		uXorKey = uXorKey * 33 + tolower(s[i]);
	}

	for (UInt32 i = 0u; i < ru; ++i)
	{
		// Mix in the file offset
		((Byte*)rp)[i] ^= (Byte)((uXorKey >> ((i % 4) << 3)) + (i / 4) * 101);
	}

	return true;
}

} // namespace Seoul::SoundUtil
