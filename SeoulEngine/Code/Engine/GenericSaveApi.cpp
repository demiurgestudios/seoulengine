/**
 * \file GenericSaveApi.cpp
 * \brief GenericSaveApi is a concrete implementation of SaveApi
 * that writes files to disk using atomic semantics.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Directory.h"
#include "DiskFileSystem.h"
#include "GenericSaveApi.h"
#include "JobsManager.h"
#include "SeoulTime.h"

namespace Seoul
{

/**
 * Number of times that we attempt the deletion or rename of a backup file.
 * This can happen due to file locking on PC (anti-virus, for example),
 * and on mobile (due to unknown reasons, but we have seen what appears
 * to be temporary failure of these operations on iOS). As such, we want
 * to give ourselves a few changes for this to succeed instead of immediately
 * failing the entire operation.
 */
static const UInt32 kuMaxBackupFileDeleteAttempts = 5u;

/** Time to wait in between file delete retries. */
static const UInt32 kuRetryIntervalInSeconds = 1u;

/**
 * Sanity check - value of < 1u for kuMaxBackupFileDeleteAttempts
 * will break saving behavior.
 */
SEOUL_STATIC_ASSERT(kuMaxBackupFileDeleteAttempts > 0u);

/** Shared utility to wait for a retry in a Jobs::Manager friendly manner. */
static inline void WaitForRetryInterval()
{
	// Otherwise, wait for the timeout.
	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	Int64 const iTargetTimeInTicks = iStartTimeInTicks + SeoulTime::ConvertSecondsToTicks((Double)kuRetryIntervalInSeconds);
	Int64 iNow;
	while ((iNow = SeoulTime::GetGameTimeInTicks()) < iTargetTimeInTicks)
	{
		Jobs::Manager::Get()->YieldThreadTime();
	}
}

/** Convert of a rename failure into a save error code. */
static inline SaveLoadResult Convert(RenameResult eResult)
{
	switch (eResult)
	{
	case RenameResult::ErrorAccess: return SaveLoadResult::kErrorRenameAccess;
	case RenameResult::ErrorBusy: return SaveLoadResult::kErrorRenameBusy;
	case RenameResult::ErrorExist: return SaveLoadResult::kErrorRenameExist;
	case RenameResult::ErrorInvalid: return SaveLoadResult::kErrorRenameInvalid;
	case RenameResult::ErrorIo: return SaveLoadResult::kErrorRenameIo;
	case RenameResult::ErrorNameTooLong: return SaveLoadResult::kErrorRenameNameTooLong;
	case RenameResult::ErrorNoEntity: return SaveLoadResult::kErrorRenameNoEntity;
	case RenameResult::ErrorNoSpace: return SaveLoadResult::kErrorRenameNoSpace;
	case RenameResult::ErrorReadOnly: return SaveLoadResult::kErrorRenameReadOnly;

	case RenameResult::ErrorUnknown: // fall-through
	default:
		return SaveLoadResult::kErrorRenameUnknown;
	};
}

/** Convert of a write failure into a save error code. */
static inline SaveLoadResult Convert(WriteResult eResult)
{
	switch (eResult)
	{
	case WriteResult::ErrorAccess: return SaveLoadResult::kErrorFileWriteAccess;
	case WriteResult::ErrorBadFileDescriptor: return SaveLoadResult::kErrorFileWriteBadFileDescriptor;
	case WriteResult::ErrorBigFile: return SaveLoadResult::kErrorFileWriteBigFile;
	case WriteResult::ErrorEOF: return SaveLoadResult::kErrorFileWriteEOF;
	case WriteResult::ErrorExist: return SaveLoadResult::kErrorFileWriteExist;
	case WriteResult::ErrorInvalid: return SaveLoadResult::kErrorFileWriteInvalid;
	case WriteResult::ErrorIo: return SaveLoadResult::kErrorFileWriteIo;
	case WriteResult::ErrorIsDir: return SaveLoadResult::kErrorFileWriteIsDir;
	case WriteResult::ErrorNameTooLong: return SaveLoadResult::kErrorFileWriteNameTooLong;
	case WriteResult::ErrorNoBufferSpace: return SaveLoadResult::kErrorFileWriteNoBufferSpace;
	case WriteResult::ErrorNoEntity: return SaveLoadResult::kErrorFileWriteNoEntity;
	case WriteResult::ErrorNoSpace: return SaveLoadResult::kErrorFileWriteNoSpace;
	case WriteResult::ErrorReadOnly: return SaveLoadResult::kErrorFileWriteReadOnly;
	case WriteResult::ErrorTooManyProcess: return SaveLoadResult::kErrorFileWriteTooManyProcess;
	case WriteResult::ErrorTooManySystem: return SaveLoadResult::kErrorFileWriteTooManySystem;
	case WriteResult::ErrorWriteNotSupported: return SaveLoadResult::kErrorFileWriteNotSupported;

	case WriteResult::ErrorUnknown: // fall-through
	default:
		return SaveLoadResult::kErrorFileWriteUnknown;
	};
}

/**
 * Create the directory used for saving, if necessary.
 */
void GenericSaveApi::InternalCreateSaveDirectories(const String& sPath)
{
	String sDirectory(Path::GetDirectoryName(sPath));
	(void)Directory::CreateDirPath(sDirectory);
}

/**
 * Load data from disk, using atomic semantics.
 */
SaveLoadResult GenericSaveApi::Load(FilePath filePath, StreamBuffer& rData) const
{
	// Create the target filename and backup filename.
	String const sAbsoluteFilename(filePath.GetAbsoluteFilename());
	String const sBackupFilename(Path::ReplaceExtension(sAbsoluteFilename, ".bak"));

	// Buffer for loading - will swap on success.
	StreamBuffer buffer(MemoryBudgets::Saving);

	// If a file exists at the backup path, it indicates a previous save failed.
	// Delete the file at the load path and move the backup file.
	if (DiskSyncFile::FileExists(sBackupFilename))
	{
		if (DiskSyncFile::FileExists(sAbsoluteFilename))
		{
			if (!DiskSyncFile::DeleteFile(sAbsoluteFilename))
			{
				return SaveLoadResult::kErrorBackupRestore;
			}
		}

		if (!DiskSyncFile::RenameFile(sBackupFilename, sAbsoluteFilename))
		{
			return SaveLoadResult::kErrorBackupRestore;
		}
	}

	// Perform the actual load
	if (DiskSyncFile::FileExists(sAbsoluteFilename))
	{
		DiskSyncFile file(sAbsoluteFilename, File::kRead);
		Bool bReturn = buffer.Load(file);
		if (bReturn)
		{
			rData.Swap(buffer);
			return SaveLoadResult::kSuccess;
		}
		else
		{
			return SaveLoadResult::kErrorFileOp;
		}
	}
	else
	{
		return SaveLoadResult::kErrorFileNotFound;
	}
}

/**
 * Save data to disk, using atomic semantics.
 */
SaveLoadResult GenericSaveApi::Save(FilePath filePath, const StreamBuffer& data) const
{
	// Create the target filename and backup filename.
	String const sAbsoluteFilename(filePath.GetAbsoluteFilename());
	String const sBackupFilename(Path::ReplaceExtension(sAbsoluteFilename, ".bak"));

	// Create directories if necessary.
	InternalCreateSaveDirectories(sAbsoluteFilename);

	// Before writing the data, if a file exists at sAbsoluteFilename,
	// rename it to the backup filename.
	if (DiskSyncFile::FileExists(sAbsoluteFilename))
	{
		// Retry this operation a few times up to our max as needed.
		SaveLoadResult eBackupResult = SaveLoadResult::kSuccess;
		for (UInt32 i = 0u; i < kuMaxBackupFileDeleteAttempts; ++i)
		{
			// Wait for the retry interval after the first attempt.
			if (i > 0u)
			{
				WaitForRetryInterval();
			}

			// If there is already a backup file, assume it is valid, the file at sAbsoluteFilename is
			// invalid, so delete sAbsoluteFilename.
			if (DiskSyncFile::FileExists(sBackupFilename))
			{
				if (!DiskSyncFile::DeleteFile(sAbsoluteFilename))
				{
					// Fail for now, potentially retry.
					eBackupResult = SaveLoadResult::kErrorBackupCreateDeleteOld;
					continue;
				}
			}
			// Otherwise, make the current target file the backup file.
			else
			{
				auto const eResult = DiskSyncFile::RenameFileEx(sAbsoluteFilename, sBackupFilename);
				if (RenameResult::Success != eResult)
				{
					// Fail for now, potentially retry.
					eBackupResult = Convert(eResult);
					continue;
				}
			}

			// If we get here, succeed and break.
			eBackupResult = SaveLoadResult::kSuccess;
			break;
		}

		// Return immediately if not successful.
		if (SaveLoadResult::kSuccess != eBackupResult)
		{
			return eBackupResult;
		}
	}

	// Perform the actual save.
	SaveLoadResult eResult = SaveLoadResult::kErrorUnknown;
	{
		MemorySyncFile file(sAbsoluteFilename);
		Bool const bReturn = data.Save(file);
		if (bReturn)
		{
			// File buffer is empty indicates a write error,
			// since data store data must have a body.
			if (file.GetBuffer().IsEmpty())
			{
				eResult = SaveLoadResult::kErrorTooSmall;
			}
			else
			{
				// Write with a full flush to ensure commit.

				// Full write.
				DiskSyncFile diskFile(sAbsoluteFilename, File::kWriteTruncate);
				auto const& buffer = file.GetBuffer();
				auto const uToWrite = buffer.GetTotalDataSizeInBytes();
				auto const res = diskFile.WriteRawDataEx(buffer.GetBuffer(), uToWrite);

				// We only consider error cases if target size is not equal to result
				// size.
				if (uToWrite != res.First)
				{
					// Write failure - infer specific error based on amount of data written.
					eResult = Convert(res.Second);
				}
				// Now full flush - on success, we're done successfully.
				else if (file.Flush())
				{
					eResult = SaveLoadResult::kSuccess;
				}
				// Otherwise, a flush failure.
				else
				{
					eResult = SaveLoadResult::kErrorFileFlush;
				}
			}
		}
		else
		{
			eResult = SaveLoadResult::kErrorFileOp;
		}
	}

	// If the operation was successful, delete the old file at the backup path.
	if (SaveLoadResult::kSuccess == eResult)
	{
		// Retry this operation a few times up to our max as needed.
		Bool bSuccess = false;
		for (UInt32 i = 0u; i < kuMaxBackupFileDeleteAttempts; ++i)
		{
			// Wait for the retry interval after the first attempt.
			if (i > 0u)
			{
				WaitForRetryInterval();
			}

			// If no backup file, we're done.
			if (!DiskSyncFile::FileExists(sBackupFilename))
			{
				bSuccess = true;
				break;
			}

			// If we successfully delete the backup file, we're done.
			if (DiskSyncFile::DeleteFile(sBackupFilename))
			{
				bSuccess = true;
				break;
			}
		}

		// If we fail deleting the backup file here, this is equivalent to
		// a save failure, since the loading code will ignore whatever was
		// written to sAbsoluteFilename
		if (!bSuccess)
		{
			eResult = SaveLoadResult::kErrorBackupDelete;
		}
	}
	// Otherwise, delete any data that was written to the file path and
	// move the backup file back to the target file.
	else
	{
		// Ignore the results here, the loading code will give it another go
		// and deal with failures here gracefully.
		(void)DiskSyncFile::DeleteFile(sAbsoluteFilename);
		(void)DiskSyncFile::RenameFile(sBackupFilename, sAbsoluteFilename);
	}

	return eResult;
}

} // namespace Seoul
