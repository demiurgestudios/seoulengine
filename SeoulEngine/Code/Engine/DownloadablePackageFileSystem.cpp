/**
 * \file DownloadablePackageFileSystem.cpp
 * \brief PackageFileSystem wrapper, supports on-the-fly piece-by-piece
 * downloading of its contents.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Directory.h"
#include "DiskFileSystem.h"
#include "DownloadablePackageFileSystem.h"
#include "FileManager.h"
#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "HTTPRequestList.h"
#include "HTTPResponse.h"
#include "JobsFunction.h"
#include "JobsManager.h"
#include "Logger.h"
#include "MemoryBarrier.h"
#include "Path.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#include "SeoulCrc32.h"
#include "SeoulTime.h"
#include "Thread.h"

namespace Seoul
{

/** HTTP result code of a successful download - code for "partial content". */
static const Int32 kDownloadSuccessStatusCode = 206;

/** Seconds to wait before retrying a failure when initializing the package. */
static const Double kfRetryTimeInSeconds = 3.0;

/** Utility that checks whether a file exists and if not, creates a 0-length file, if possible. */
static void InternalStaticCheckForAndCreateFile(const String& sAbsoluteFilename, UInt64 uSizeHintInBytes)
{
	// If there's no file, create an empty one.
	if (!DiskSyncFile::FileExists(sAbsoluteFilename))
	{
		// Create any dependent directory structure.
		(void)Directory::CreateDirPath(Path::GetDirectoryName(sAbsoluteFilename));

		// Create the file - note that size here is a recommendation/hint. On some platforms,
		// this may still produce a 0-byte file.
		(void)DiskSyncFile::CreateAllZeroSparseFile(sAbsoluteFilename, uSizeHintInBytes);

		// Set the "no backup flag", necessary on:
		// - iOS: disable cloud storage uploads of the file if the file is in the Documents/ area.
		(void)DiskSyncFile::SetDoNotBackupFlag(sAbsoluteFilename);
	}
}

/**
 * Utility, when a Fetch() must happen implicitly, select the best
 * initial priority for that fetch, based on FilePath.
 */
inline static NetworkFetchPriority GetBestImplicitPriority(FilePath filePath)
{
	// Special handling for texture types - if the texture that is 1 mip
	// away from the target is already available locally, use a priority
	// level of kMedium.
	if (IsTextureFileType(filePath.GetType()))
	{
		// Check mip levels 1 above and 1 below the target - if either
		// are not network serviced, use kMedium.
		FilePath alternateFilePath(filePath);

		// Mip above.
		alternateFilePath.SetType((FileType)Clamp((Int)filePath.GetType() - 1, (Int)FileType::FIRST_TEXTURE_TYPE, (Int)FileType::LAST_TEXTURE_TYPE));
		if (alternateFilePath != filePath)
		{
			if (!FileManager::Get()->IsServicedByNetwork(alternateFilePath))
			{
				return NetworkFetchPriority::kMedium;
			}
		}

		// Mip below.
		alternateFilePath.SetType((FileType)Clamp((Int)filePath.GetType() + 1, (Int)FileType::FIRST_TEXTURE_TYPE, (Int)FileType::LAST_TEXTURE_TYPE));
		if (alternateFilePath != filePath)
		{
			if (!FileManager::Get()->IsServicedByNetwork(alternateFilePath))
			{
				return NetworkFetchPriority::kMedium;
			}
		}
	}

	// Use a priority of kDefault for all other types and situations
	return NetworkFetchPriority::kDefault;
}

/**
 * @return true if package a can directly absorb data contained in package b,
 * false otherwise. Certain settings (e.g. obfuscation or compression) can
 * make the data on disk byte incompatible between two archives.
 */
static inline Bool AreCompatible(const PackageFileSystem& a, const PackageFileSystem& b)
{
	auto const& headerA = a.GetHeader();
	auto const& headerB = b.GetHeader();

	// Obfuscation setting must be the same between both.
	if (headerA.IsObfuscated() != headerB.IsObfuscated())
	{
		return false;
	}

	// Compression type must match (cannot go so far back that we switched between LZ4 and ZSTD between archives).
	if (headerA.IsOldLZ4Compression() != headerB.IsOldLZ4Compression())
	{
		return false;
	}

	// Here's the last and big one - both archive either must
	// not use a compression dict, or they both must use the
	// same compression dict.
	if (!a.GetCompressionDictFilePath().IsValid() &&
		!b.GetCompressionDictFilePath().IsValid())
	{
		// Neither uses a dict, we're good to go.
		return true;
	}

	// One uses but the other does not, fail.
	if (!(a.GetCompressionDictFilePath().IsValid() && b.GetCompressionDictFilePath().IsValid()))
	{
		return false;
	}

	// Check compatibility - this case is not actually expected
	// to happen in practice (since GetCompressionDictFilePath() will
	// be invalid if it has no entry) but we handle it here by
	// reporting "not compatible".
	PackageFileTableEntry dictA;
	PackageFileTableEntry dictB;
	if (!a.GetFileTable().GetValue(a.GetCompressionDictFilePath(), dictA) ||
		!b.GetFileTable().GetValue(b.GetCompressionDictFilePath(), dictB))
	{
		return false;
	}

	// Dict data must match - sizes and crc32.
	if (dictA.m_Entry.m_uCompressedFileSize != dictB.m_Entry.m_uCompressedFileSize)
	{
		return false;
	}
	if (dictA.m_Entry.m_uCrc32Pre != dictB.m_Entry.m_uCrc32Pre)
	{
		return false;
	}
	if (dictA.m_Entry.m_uUncompressedFileSize != dictB.m_Entry.m_uUncompressedFileSize)
	{
		return false;
	}

	return true;
}

/**
 * Utility used to handle initial setup of the package file for
 * a DownloadablePackageFileSystem. Used by the worker thread.
 */
class DownloadPackageInitializationUtility SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(DownloadPackageInitializationUtility);

	/** Various states of this utility. */
	enum State
	{
		/** Initial state, asked the server to send the header portion of the .sar. */
		kRequestHeader,

		/** Waiting for server to return the header. */
		kWaitingForHeader,

		/** Used for the HTTP callback to signal that the header has been retrieved. */
		kReceivedHeader,

		/** After kReceivedHeader, checks existing package and decides if it's ok and if further download actions are needed for initialization. */
		kCheckExistingPackage,

		/** Set an HTTP request for the file table. */
		kRequestFileTable,

		/** Waiting for server to return the file table. */
		kWaitingForFileTable,

		/** Used for the HTTP callback to signal that the file table has been retireved. */
		kReceivedFileTable,

		/** On the file IO thread, commit the header and file table, and reload the package. */
		kUpdateAndReloadPackage,

		/** At any stage, used for error handling. */
		kError,

		/** Task has completed successfully. */
		kComplete,
	};

	DownloadPackageInitializationUtility(DownloadablePackageFileSystem& rSystem)
		: m_iRetryStartTimeInTicks(-1)
		, m_rSystem(rSystem)
		, m_eState(kRequestHeader)
		, m_Header()
		, m_vFileTable()
	{
		ResetHeaderAndFileTable();
	}

	~DownloadPackageInitializationUtility()
	{
		// Make sure all our requests are complete.
		m_List.BlockingCancelAll();
	}

	/**
	 * When true, the init utility either detected no existing package
	 * on disk or needed to delete/move the package due to a version/signature
	 * change.
	 */
	Bool IsNewPackage() const { return m_bNewPackage; }

private:
	typedef Vector<Byte, MemoryBudgets::Io> Buffer;

	HTTP::RequestList m_List;
	Int64 m_iRetryStartTimeInTicks;
	DownloadablePackageFileSystem& m_rSystem;
	Atomic32Value<State> m_eState;
	PackageFileHeader m_Header;
	Buffer m_vFileTable;
	Atomic32Value<Bool> m_bNewPackage;

	/** Clear the header and file table to the default state. */
	void ResetHeaderAndFileTable()
	{
		memset(&m_Header, 0, sizeof(m_Header));
		{
			Buffer vEmptyBuffer;
			m_vFileTable.Swap(vEmptyBuffer);
		}
	}

	/**
	 * @return True if pResponse indicates an invalid package file header and
	 * requires a resend, false otherwise.
	 */
	Bool CheckAndGetHeader(HTTP::Response* pResponse, PackageFileHeader& rHeader) const
	{
		Bool const bReturn = (
			pResponse->GetStatus() == kDownloadSuccessStatusCode &&
			!pResponse->BodyDataWasTruncated() &&
			pResponse->GetBodySize() == (UInt32)sizeof(PackageFileHeader));

		// If the body looks ok, also load it to verify.
		if (bReturn)
		{
			// Read the header data - if it's invalid, fail the operation.
			if (!PackageFileSystem::ReadPackageHeader(pResponse->GetBody(), pResponse->GetBodySize(), rHeader))
			{
#if SEOUL_LOGGING_ENABLED
				{
					// Copy the body into a local header for debug reporting.
					PackageFileHeader header;
					memcpy(&header, pResponse->GetBody(), sizeof(header));

					String sHeader;
					(void)Reflection::SerializeToString(&header, sHeader, false, 0, true);
					SEOUL_LOG_ENGINE("DownloadPackageInitializationUtility::Poll(%s): Invalid header "
						"received in kReceivedHeader: %s",
						Path::GetFileNameWithoutExtension(m_rSystem.GetAbsolutePackageFilename()).CStr(),
						sHeader.CStr());
				}
#endif // /#if SEOUL_LOGGING_ENABLED

				return false;
			}
		}

		return bReturn;
	}

	/**
	 * @return True if pResponse indicates an invalid file table response and
	 * requires a resend, false otherwise.
	 */
	Bool CheckFileTable(HTTP::Response* pResponse) const
	{
		UInt32 const uFileTableSizeInBytes = m_Header.GetSizeOfFileTableInBytes();

		Bool const bReturn = (
			pResponse->GetStatus() == kDownloadSuccessStatusCode &&
			!pResponse->BodyDataWasTruncated() &&
			pResponse->GetBodySize() == uFileTableSizeInBytes);

		// TODO: Need a checksum for the file table.

		return bReturn;
	}

	/**
	 * Called by HTTP::Manager when the file table data request has completed.
	 */
	HTTP::CallbackResult OnFileTableReceived(HTTP::Result eResult, HTTP::Response* pResponse)
	{
		if (eResult != HTTP::Result::kSuccess || !CheckFileTable(pResponse))
		{
			// Clear the redirect URL on any error.
			m_rSystem.m_sURL = m_rSystem.m_Settings.m_sInitialURL;

			return HTTP::CallbackResult::kNeedsResend;
		}

		// Tell the worker the file table data has been received.
			// If a redirection occured, cache it so we don't perform a redirect with each request
			// to optimize interactions with the server.
		if (!pResponse->GetRedirectURL().IsEmpty())
		{
			m_rSystem.m_sURL = pResponse->GetRedirectURL();
		}

		m_eState = kReceivedFileTable;

		SeoulMemoryBarrier();

		// Wake things up.
		m_rSystem.m_Signal.Activate();

		return HTTP::CallbackResult::kSuccess;
	}

	/**
	 * Called by HTTP::Manager when the header data request has completed.
	 */
	HTTP::CallbackResult OnHeaderReceived(HTTP::Result eResult, HTTP::Response* pResponse)
	{
		PackageFileHeader header;
		if (eResult != HTTP::Result::kSuccess || !CheckAndGetHeader(pResponse, header))
		{
			// Clear the redirect URL on any error.
			m_rSystem.m_sURL = m_rSystem.m_Settings.m_sInitialURL;
			return HTTP::CallbackResult::kNeedsResend;
		}

		// Tell the worker the header data has been received.
		// If a redirection occured, cache it so we don't perform a redirect with each request
		// to optimize interactions with the server.
		if (!pResponse->GetRedirectURL().IsEmpty())
		{
			m_rSystem.m_sURL = pResponse->GetRedirectURL();
		}

		// Fill in the header.
		m_Header = header;

		m_eState = kReceivedHeader;

		SeoulMemoryBarrier();

		// Wake things up.
		m_rSystem.m_Signal.Activate();

		return HTTP::CallbackResult::kSuccess;
	}

	static void OnPrepForResend(void* p, HTTP::Response* pOriginalResponse, HTTP::Request* pOriginalRequest, HTTP::Request* pResendRequest)
	{
		DownloadPackageInitializationUtility* pUtility = (DownloadPackageInitializationUtility*)p;
		pResendRequest->SetURL(pUtility->m_rSystem.m_Settings.m_sInitialURL);
	}

public:
	State Poll()
	{
		// Simple state machine used to handle downloading the basic bits (header and file table)
		// of a .sar archive.
		switch (m_eState)
		{
		case kRequestHeader:
			{
				// Update the state before scheduling the operation.
				m_eState = kWaitingForHeader;

				// Schedule an HTTP request.
				auto& r = HTTP::Manager::Get()->CreateRequest(&m_List);
				r.AddRangeHeader(
					(UInt64)0u, // Start of the file
					(UInt64)(sizeof(m_Header) - 1)); // End of the header.
				r.SetDispatchCallbackOnMainThread(false);
				r.SetURL(m_rSystem.m_sURL);
				r.SetCallback(SEOUL_BIND_DELEGATE(&DownloadPackageInitializationUtility::OnHeaderReceived, this));
				r.SetPrepForResendCallback(SEOUL_BIND_DELEGATE(&DownloadPackageInitializationUtility::OnPrepForResend, this));
				r.Start();
			}

			// Done with kRequestHeader
			return m_eState;

		case kWaitingForHeader:
			// Wait on the signal.
			m_rSystem.m_Signal.Wait();

			// Done with kWaitingForHeader
			return m_eState;

		case kReceivedHeader:
			// Continue to the kCheckExistingPackage state.
			m_eState = kCheckExistingPackage;

			// Done with kReceivedHeader
			return m_eState;

		case kCheckExistingPackage:
			{
				// Cache the absolute package filename.
				String const sAbsolutePackageFilename(m_rSystem.m_Settings.m_sAbsolutePackageFilename);

				// Create the package initially - this may be reset if the basic info (header and file table) appear to be invalid.
				m_rSystem.m_pPackageFileSystem.Reset(SEOUL_NEW(MemoryBudgets::Io) PackageFileSystem(sAbsolutePackageFilename, false, true, true));
				Bool const bOk = m_rSystem.m_pPackageFileSystem->IsOk();

				// Check the package - if anything is wrong with the basic state, either delete it, or move
				// it into a temporary location as a local cache file.
				if (!bOk || m_rSystem.m_pPackageFileSystem->GetHeader() != m_Header)
				{
					// Package is now new.
					m_bNewPackage = true;

					m_rSystem.m_pPackageFileSystem.Reset();

					// If the existing package is ok, keep the file as a ".old" file to use
					// as a local cache.
					if (bOk)
					{
						String const sOldFilename(sAbsolutePackageFilename + ".old");
						(void)DiskSyncFile::DeleteFile(sOldFilename);
						(void)DiskSyncFile::RenameFile(sAbsolutePackageFilename, sOldFilename);

						// Set the "no backup flag", necessary on:
						// - iOS: disable cloud storage uploads of the file if the file is in the Documents/ area.
						(void)DiskSyncFile::SetDoNotBackupFlag(sOldFilename);
					}

					// Delete the package file.
					(void)DiskSyncFile::DeleteFile(sAbsolutePackageFilename);

					// If there's no file, create an empty one.
					InternalStaticCheckForAndCreateFile(sAbsolutePackageFilename, m_Header.GetTotalPackageFileSizeInBytes());

					// Initialize the package again.
					m_rSystem.m_pPackageFileSystem.Reset(SEOUL_NEW(MemoryBudgets::Io) PackageFileSystem(sAbsolutePackageFilename, false, true, true));

					// Continue to kRequestFileTable state.
					m_eState = kRequestFileTable;
				}
				// If everything is good to go, we're done with download actions for initialization.
				else
				{
					// Continue to the kComplete state.
					m_eState = kComplete;
				}
			}

			// Done with kCheckExistingPackage.
			return m_eState;

		case kRequestFileTable:
			{
				// Update the state before scheduling the operation.
				m_eState = kWaitingForFileTable;

				// Compute the file table size and allocate space for it.
				UInt32 const uFileTableSizeInBytes = m_Header.GetSizeOfFileTableInBytes();
				if (uFileTableSizeInBytes > 0)
				{
					UInt64 const uStartOffset = m_Header.GetOffsetToFileTableInBytes();
					UInt64 const uEndOffset = (UInt64)(uStartOffset + uFileTableSizeInBytes - 1u);
					m_vFileTable.Resize(uFileTableSizeInBytes);

					// Setup the HTTP request.
					auto& r = HTTP::Manager::Get()->CreateRequest(&m_List);
					r.AddRangeHeader(uStartOffset, uEndOffset);
					r.SetDispatchCallbackOnMainThread(false);
					r.SetURL(m_rSystem.m_sURL);
					r.SetCallback(SEOUL_BIND_DELEGATE(&DownloadPackageInitializationUtility::OnFileTableReceived, this));
					r.SetPrepForResendCallback(SEOUL_BIND_DELEGATE(&DownloadPackageInitializationUtility::OnPrepForResend, this));
					r.SetBodyOutputBuffer(m_vFileTable.Data(), m_vFileTable.GetSizeInBytes());
					r.Start();
				}
				// Special case handling for an empty file table.
				else
				{
					m_eState = kUpdateAndReloadPackage;
				}
			}
			// Done with kRequestFileTable
			return m_eState;

		case kWaitingForFileTable:
			// Wait on the signal.
			m_rSystem.m_Signal.Wait();

			// Done with kWaitingForFileTable
			return m_eState;

		case kReceivedFileTable:
			m_eState = kUpdateAndReloadPackage;

			// Done with kReceivedFileTable.
			return m_eState;

		case kUpdateAndReloadPackage:
			{
				// If we failed writing the header data, go to the error state.
				if (!m_rSystem.m_pPackageFileSystem->CommitChangeToSarFile(
					&m_Header, sizeof(m_Header), 0))
				{
					m_rSystem.m_bHasExperiencedWriteFailure = true;
					m_eState = kError;
					return m_eState;
				}

				// If we have file table data, write it.
				if (!m_vFileTable.IsEmpty())
				{
					// If we failed writing the file table data, go to the error state.
					if (!m_rSystem.m_pPackageFileSystem->CommitChangeToSarFile(
						m_vFileTable.Get(0u),
						m_vFileTable.GetSizeInBytes(),
						(Int64)m_Header.GetOffsetToFileTableInBytes()))
					{
						m_rSystem.m_bHasExperiencedWriteFailure = true;
						m_eState = kError;
						return m_eState;
					}
				}

				// If we reach here, writes to disk have succeeded, so clear the write failure bit.
				m_rSystem.m_bHasExperiencedWriteFailure = false;

				// Cache the package file name, then recreate the package to reinitialize it.
				String const sAbsolutePackageFilename(m_rSystem.m_Settings.m_sAbsolutePackageFilename);
				m_rSystem.m_pPackageFileSystem.Reset();
				m_rSystem.m_pPackageFileSystem.Reset(SEOUL_NEW(MemoryBudgets::Io) PackageFileSystem(sAbsolutePackageFilename, false, true, true));

				// Error handling, try again if we didn't end up with a valid package.
				if (!m_rSystem.m_pPackageFileSystem->IsOk())
				{
					m_eState = kError;
					return m_eState;
				}

				// Initialization has completed successfully.
				m_eState = kComplete;
			}

			// Done with kUpdateAndReloadPackage.
			return m_eState;

		case kError:
			{
				// Cache the absolute package filename.
				String const sAbsolutePackageFilename(m_rSystem.m_Settings.m_sAbsolutePackageFilename);

				// If we haven't entered the retry interval, do so now.
				if (m_iRetryStartTimeInTicks < 0)
				{
					m_iRetryStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
				}

				// If we're still in the retry interval, wait for it to complete.
				Int64 const iDeltaTimeInTicks = SeoulTime::GetGameTimeInTicks() - m_iRetryStartTimeInTicks;
				if (iDeltaTimeInTicks >= 0 &&
					SeoulTime::ConvertTicksToSeconds(iDeltaTimeInTicks) < kfRetryTimeInSeconds)
				{
					// Wait on the signal with a timeout.
					m_rSystem.m_Signal.Wait((UInt32)Ceil(SeoulTime::ConvertTicksToMilliseconds(iDeltaTimeInTicks)));

					return m_eState;
				}

				// If a write error occured, try to recreate the package file. This
				// allows the initial package open to be retried if it failed for some reason.
				if (m_rSystem.m_bHasExperiencedWriteFailure)
				{
					// On write failure, try deleting the old package, if it exists. This
					// prevents the old package from preventing the generation of the new package
					// on low disk space conditions.
					{
						String const sOldFilename(sAbsolutePackageFilename + ".old");
						(void)DiskSyncFile::DeleteFile(sOldFilename);
					}

					// Cache the absolute package filename.
					String const sAbsolutePackageFilename(m_rSystem.m_pPackageFileSystem->GetAbsolutePackageFilename());

					// Clear the handle to the file system.
					m_rSystem.m_pPackageFileSystem.Reset();

					// Delete the package on write failure.
					(void)DiskSyncFile::DeleteFile(sAbsolutePackageFilename);

					// If there's no file, create an empty one. Use a hint of 0 here,
					// we don't know if the header is valid or not.
					InternalStaticCheckForAndCreateFile(sAbsolutePackageFilename, 0);

					// Recreate the package.
					m_rSystem.m_pPackageFileSystem.Reset(SEOUL_NEW(MemoryBudgets::Io) PackageFileSystem(sAbsolutePackageFilename, false, true, true));
				}

				// Reset the retry start time.
				m_iRetryStartTimeInTicks = -1;

				// Clear header and file table data.
				ResetHeaderAndFileTable();

				// Error handling, return to the kRequestHeader state - try again.
				m_eState = kRequestHeader;
			}

			// Done with kError.
			return m_eState;

		case kComplete:
			// No action on kComplete.

			// Done with kComplete.
			return m_eState;

		default:
			SEOUL_FAIL("Out of sync enum.");
			return m_eState;
		};
	}

private:
	SEOUL_DISABLE_COPY(DownloadPackageInitializationUtility);
}; // class DownloadPackageInitializationUtility

SEOUL_BEGIN_ENUM(DownloadPackageInitializationUtility::State)
	SEOUL_ENUM_N("RequestHeader", DownloadPackageInitializationUtility::kRequestHeader)
	SEOUL_ENUM_N("WaitingForHeader", DownloadPackageInitializationUtility::kWaitingForHeader)
	SEOUL_ENUM_N("ReceivedHeader", DownloadPackageInitializationUtility::kReceivedHeader)
	SEOUL_ENUM_N("CheckExistingPackage", DownloadPackageInitializationUtility::kCheckExistingPackage)
	SEOUL_ENUM_N("RequestFileTable", DownloadPackageInitializationUtility::kRequestFileTable)
	SEOUL_ENUM_N("WaitingForFileTable", DownloadPackageInitializationUtility::kWaitingForFileTable)
	SEOUL_ENUM_N("ReceivedFileTable", DownloadPackageInitializationUtility::kReceivedFileTable)
	SEOUL_ENUM_N("UpdateAndReloadPackage", DownloadPackageInitializationUtility::kUpdateAndReloadPackage)
	SEOUL_ENUM_N("Error", DownloadPackageInitializationUtility::kError)
	SEOUL_ENUM_N("Complete", DownloadPackageInitializationUtility::kComplete)
SEOUL_END_ENUM()

/** Static class, collection of utilities used to download file data. */
class DownloadablePackageFileSystemHelpers
{
public:
	/**
	 * Utility used to track state while downloading.
	 */
	struct DownloadHelper
	{
		DownloadHelper(
			Atomic32Value<Bool>& rbWorkerThreadRunning,
			void* pBuffer,
			DownloadablePackageFileSystem& rFileSystem,
			UInt64 zStartingOffset,
			UInt32 zSizeInBytes)
			: m_rbWorkerThreadRunning(rbWorkerThreadRunning)
			, m_zStartingOffset(zStartingOffset)
			, m_rFileSystem(rFileSystem)
			, m_zSizeInBytes(zSizeInBytes)
			, m_zProgressInBytes(0u)
			, m_pBuffer(pBuffer)
			, m_bSuccess(false)
			, m_bDone(false)
		{
		}

		Atomic32Value<Bool>& m_rbWorkerThreadRunning;
		UInt64 m_zStartingOffset;
		DownloadablePackageFileSystem& m_rFileSystem;
		UInt32 m_zSizeInBytes;
		UInt32 m_zProgressInBytes;
		void* m_pBuffer;
		Atomic32Value<Bool> m_bSuccess;
		Atomic32Value<Bool> m_bDone;
	};

	/**
	 * @return True if pResponse contains invalid file data and requires
	 * a resend, false otherwise.
	 */
	static Bool DownloadNeedsResend(DownloadHelper const& rHelper, HTTP::Response* pResponse)
	{
		Bool const bReturn = (
			pResponse->GetStatus() != kDownloadSuccessStatusCode ||
			pResponse->BodyDataWasTruncated() ||
			pResponse->GetBodySize() != (rHelper.m_zSizeInBytes - rHelper.m_zProgressInBytes));

		return bReturn;
	}

	/** Call received by HTTP::Manager when a download completes, success or failure. */
	static HTTP::CallbackResult OnDownloadReceived(void* p, HTTP::Result eResult, HTTP::Response* pResponse)
	{
		// Get the helper data.
		DownloadHelper& rHelper = *((DownloadHelper*)p);

		// Successful if all successful.
		auto const bSuccess = (HTTP::Result::kSuccess == eResult && !DownloadNeedsResend(rHelper, pResponse));

		// Resend on failure unless the request was cancelled or if the worker thread is shutting down.
		auto const bResend = (!bSuccess && HTTP::Result::kCanceled != eResult && rHelper.m_rbWorkerThreadRunning);

		// Perform the resend now if determined.
		if (bResend)
		{
			// Clear the redirect URL on any error.
			rHelper.m_rFileSystem.m_sURL = rHelper.m_rFileSystem.m_Settings.m_sInitialURL;

			return HTTP::CallbackResult::kNeedsResend;
		}

		// If a redirection occured, cache it so we don't perform a redirect with each request
		// to optimize interactions with the server.
		if (bSuccess && !pResponse->GetRedirectURL().IsEmpty())
		{
			rHelper.m_rFileSystem.m_sURL = pResponse->GetRedirectURL();
		}

		// Success unless a resend should have occurred (we may have cancelled
		// under various HTTP conditions or if the downloader worker thread is
		// shutting down).
		rHelper.m_bSuccess = bSuccess;

		// Threading.
		SeoulMemoryBarrier();

		// Operation is complete.
		rHelper.m_bDone = true;

		// Threading.
		SeoulMemoryBarrier();

		// Fire the signal.
		rHelper.m_rFileSystem.m_Signal.Activate();

		return HTTP::CallbackResult::kSuccess;
	}

	/** Utility function, called when a request is interrupted and needs a resend. */
	static void OnDownloadPrepForResend(
		void* p,
		HTTP::Response* pOriginalResponse,
		HTTP::Request* pOriginalRequest,
		HTTP::Request* pResendRequest)
	{
		// Get the helper data.
		DownloadHelper& rHelper = *((DownloadHelper*)p);

		// Cache the total expected size.
		UInt32 const zTotalSizeInBytes = rHelper.m_zSizeInBytes;

		// Err on the side of caution - only attempt to "resume" the transfer if
		// the connection closed (no status code). Any other status code that requires
		// a resend, treat it as a server error and restart the entire transfer.
		if (pOriginalResponse->GetStatus() >= 0)
		{
			rHelper.m_zProgressInBytes = 0;

			// In this case, also reset the URL - if we're getting bad return codes,
			// it means something unexpected happened (server misconfiguration,
			// server crash), so we want to give the client a chance to
			// re-evaluate the (possible) redirect URL.
			rHelper.m_rFileSystem.m_sURL = rHelper.m_rFileSystem.m_Settings.m_sInitialURL;
			pResendRequest->SetURL(rHelper.m_rFileSystem.m_sURL);
		}
		// Otherwise, resume.
		else
		{
			rHelper.m_zProgressInBytes += pOriginalResponse->GetBodySize();
		}

		// Adjust the output buffer to account for the data already received.
		pResendRequest->SetBodyOutputBuffer(((Byte*)rHelper.m_pBuffer) + rHelper.m_zProgressInBytes, (zTotalSizeInBytes - rHelper.m_zProgressInBytes));

		// Adjust the range header.
		pResendRequest->AddRangeHeader(
			rHelper.m_zStartingOffset + rHelper.m_zProgressInBytes,
			rHelper.m_zStartingOffset + zTotalSizeInBytes - 1);
	}

	/**
	 * Download the entire specified range of the .sar file into pOut.
	 */
	static Bool Download(
		Atomic32Value<Bool>& rbWorkerThreadRunning,
		HTTP::RequestList* pList,
		void* pOut,
		DownloadablePackageFileSystem& rFileSystem,
		UInt64 zStartingOffset,
		UInt32 zSizeInBytes)
	{
		// Sanity handling, since this case will result in an invalid
		// range if we don't handle it here.
		if (0 == zSizeInBytes)
		{
			return true;
		}

		DownloadHelper helper(rbWorkerThreadRunning, pOut, rFileSystem, zStartingOffset, zSizeInBytes);

		{
			auto& r = HTTP::Manager::Get()->CreateRequest(pList);
			r.AddRangeHeader(
				helper.m_zStartingOffset,
				helper.m_zStartingOffset + helper.m_zSizeInBytes - 1);
			r.SetDispatchCallbackOnMainThread(false);
			// We rely on frequent, small requests. Request budgets interfere with that.
			r.SetIgnoreDomainRequestBudget(true);
			r.SetURL(helper.m_rFileSystem.m_sURL);
			r.SetCallback(SEOUL_BIND_DELEGATE(&OnDownloadReceived, (void*)&helper));
			r.SetBodyOutputBuffer(pOut, helper.m_zSizeInBytes);
			r.SetPrepForResendCallback(SEOUL_BIND_DELEGATE(&OnDownloadPrepForResend, (void*)&helper));
			r.Start();
		}

		// Wait until complete - the signal can prematurely fire, so
		// we need to check helper.m_bDone;
		SeoulMemoryBarrier();
		while (!helper.m_bDone)
		{
			rFileSystem.m_Signal.Wait();
			SeoulMemoryBarrier();
		}

		return helper.m_bSuccess;
	}

private:
	// Static class
	DownloadablePackageFileSystemHelpers() = delete;
	SEOUL_DISABLE_COPY(DownloadablePackageFileSystemHelpers);
}; // class DownloadablePackageFileSystemHelpers

DownloadablePackageFileSystem::DownloadablePackageFileSystem(const DownloadablePackageFileSystemSettings& settings)
	: m_Settings(settings)
	, m_sURL(settings.m_sInitialURL)
	, m_pRequestList(SEOUL_NEW(MemoryBudgets::Io) HTTP::RequestList)
	, m_pWorkerThread()
	, m_Signal()
	, m_zMaxSizePerDownloadInBytes(settings.m_uUpperBoundMaxSizePerDownloadInBytes)
	, m_pPackageFileSystem()
	, m_bDoneInitializing(false)
	, m_bInitializationStarted(false)
	, m_bInitializationComplete(false)
	, m_bWorkerThreadRunning(false)
	, m_bHasExperiencedWriteFailure(false)
	, m_bWorkerThreadWaiting(false)
	, m_tCrc32CheckTable()
	, m_TaskQueue()
{
	// Initialize immediately if networking is already initialized.
	if (FileManager::Get()->IsNetworkFileIOEnabled())
	{
		OnNetworkInitialize();
	}
}

DownloadablePackageFileSystem::~DownloadablePackageFileSystem()
{
	// Call OnNetworkShutdown() if necessary.
	if (m_bInitializationStarted)
	{
		OnNetworkShutdown();
	}

	// Sanity checks, we should have flushed any pending requests.
	SEOUL_ASSERT(m_pRequestList->IsEmpty());

	// Sanity checks - the environment should have called PreEngineShutdown() before
	// this point.
	SEOUL_ASSERT(m_bDoneInitializing);
	SEOUL_ASSERT(!m_bInitializationComplete);
	SEOUL_ASSERT(!m_bInitializationStarted);
	SEOUL_ASSERT(!m_bWorkerThreadRunning);
	SEOUL_ASSERT(!m_pWorkerThread.IsValid());
}

/**
 * Must be called after Engine::Initialize(), at the point that
 * you first wish DownloadablePackageFileSystem to start network operations.
 *
 * \pre Must be called from the main thread.
 */
void DownloadablePackageFileSystem::OnNetworkInitialize()
{
	SEOUL_ASSERT(!m_bInitializationStarted);

	m_bInitializationStarted = true;

	SeoulMemoryBarrier();

	m_TaskQueue.OnNetworkInitialize();

	SeoulMemoryBarrier();

	m_pWorkerThread.Reset(SEOUL_NEW(MemoryBudgets::Io) Thread(SEOUL_BIND_DELEGATE(&DownloadablePackageFileSystem::InternalWorkerThread, this), false));

	m_bWorkerThreadRunning = true;
	SeoulMemoryBarrier();

	m_pWorkerThread->Start(String::Printf("%s Thread", Path::GetFileNameWithoutExtension(m_Settings.m_sAbsolutePackageFilename).CStr()));
	if (!m_Settings.m_bNormalPriority)
	{
		m_pWorkerThread->SetPriority(Thread::kLow);
	}

	SeoulMemoryBarrier();
}

void DownloadablePackageFileSystem::OnNetworkShutdown()
{
	SEOUL_ASSERT(m_bInitializationStarted);

	// Terminating, indicate as such.
	m_bDoneInitializing = true;
	m_bInitializationComplete = false;
	m_bWorkerThreadRunning = false;
	SeoulMemoryBarrier();

	m_pRequestList->BlockingCancelAll();
	m_Signal.Activate();
	m_pWorkerThread.Reset();

	SeoulMemoryBarrier();
	m_bInitializationComplete = false; // Again, may have been set again by worker thread termination.
	m_bDoneInitializing = true; // Sanity.
	m_TaskQueue.OnNetworkShutdown();

	SeoulMemoryBarrier();

	// Unset m_bInitializationStarted last.
	SeoulMemoryBarrier();
	m_bInitializationStarted = false;
}

/** Convenience, wrap existence of Jobs::Manager. */
static inline void Yield()
{
	if (Jobs::Manager::Get())
	{
		Jobs::Manager::Get()->YieldThreadTime();
	}
	else
	{
		Thread::YieldToAnotherThread();
	}
}

Bool DownloadablePackageFileSystem::Fetch(FilePath filePath, NetworkFetchPriority ePriority /* = NetworkFetchPriority::kDefault*/)
{
	// If prefetch fails, fetch fails.
	if (!Prefetch(filePath, ePriority))
	{
		return false;
	}

	// Wait for the fetch to complete.
	while (IsInitialized())
	{
		if (m_tCrc32CheckTable.IsCrc32Ok(filePath))
		{
			break;
		}

		Yield();
	}

	return m_tCrc32CheckTable.IsCrc32Ok(filePath);
}

Bool DownloadablePackageFileSystem::Fetch(
	const Files& vInFilesToFetch,
	ProgressCallback progressCallback /*= ProgressCallback()*/,
	NetworkFetchPriority ePriority /* = NetworkFetchPriority::kDefault*/)
{
	// If prefetch fails, fetch fails.
	Files vFilesToFetch(vInFilesToFetch);
	if (!InternalPrefetch(vFilesToFetch, ePriority))
	{
		return false;
	}

	// Compute values for the progress callback.
	UInt32 const nFiles = vFilesToFetch.GetSize();
	UInt64 zDownloadSizeInBytes = 0;
	{
		// Cache the package file system table.
		const PackageFileSystem::FileTable& tFileTable = m_pPackageFileSystem->GetFileTable();

		// Enumerate the files.
		for (UInt32 i = 0u; i < nFiles; ++i)
		{
			PackageFileTableEntry entry;
			if (tFileTable.GetValue(vFilesToFetch[i], entry))
			{
				zDownloadSizeInBytes += entry.m_Entry.m_uCompressedFileSize;
			}
		}
	}

	// Wait for the fetch to complete.
	UInt64 zLastDownloadSoFarInBytes = 0;
	while (IsInitialized())
	{
		// Cache the package file system table.
		const PackageFileSystem::FileTable& tFileTable = m_pPackageFileSystem->GetFileTable();

		// Enumerate the files.
		Bool bDone = true;
		UInt64 zDownloadSoFarInBytes = 0;
		for (UInt32 i = 0u; i < nFiles; ++i)
		{
			FilePath const filePath = vFilesToFetch[i];
			if (m_tCrc32CheckTable.IsCrc32Ok(filePath))
			{
				PackageFileTableEntry entry;
				if (tFileTable.GetValue(filePath, entry))
				{
					zDownloadSoFarInBytes += entry.m_Entry.m_uCompressedFileSize;
				}
			}
			else
			{
				bDone = false;
			}
		}

		if (zDownloadSoFarInBytes != zLastDownloadSoFarInBytes)
		{
			if (progressCallback)
			{
				progressCallback(zDownloadSizeInBytes, zDownloadSoFarInBytes);
			}

			zLastDownloadSoFarInBytes = zDownloadSoFarInBytes;
		}

		if (bDone)
		{
			break;
		}

		Yield();
	}

	return InternalAreFilesFetched(vFilesToFetch);
}

Bool DownloadablePackageFileSystem::Prefetch(FilePath filePath, NetworkFetchPriority ePriority /* = NetworkFetchPriority::kDefault*/)
{
	// If initialization is not started and completed, can't open any files.
	if (!IsInitialized())
	{
		return false;
	}

	// Prefetching fails if the file doesn't exist in this archive.
	if (!m_pPackageFileSystem->Exists(filePath))
	{
		return false;
	}

	// Check if we need to fetch.
	if (m_tCrc32CheckTable.IsCrc32Ok(filePath))
	{
		return true;
	}

	// Fetch the file.
	m_TaskQueue.Fetch(filePath, ePriority);
	m_Signal.Activate();

	return true;
}

Bool DownloadablePackageFileSystem::Prefetch(const Files& vInFilesToPrefetch, NetworkFetchPriority ePriority /* = NetworkFetchPriority::kDefault*/)
{
	Files vFilesToPrefetch(vInFilesToPrefetch);
	return InternalPrefetch(vFilesToPrefetch, ePriority);
}

Bool DownloadablePackageFileSystem::InternalPrefetch(Files& rvFiles, NetworkFetchPriority ePriority)
{
	// If initialization is not started and completed, can't open any files.
	if (!IsInitialized())
	{
		return false;
	}

	// If rvFiles is empty, this is a special value which means "download all files".
	if (rvFiles.IsEmpty())
	{
		// Cache the package file system table.
		const PackageFileSystem::FileTable& tFileTable = m_pPackageFileSystem->GetFileTable();
		auto const iBegin = tFileTable.Begin();
		auto const iEnd = tFileTable.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			rvFiles.PushBack(i->First);
		}
	}
	else
	{
		// Prune the list to files that exist in this package, and immediately
		// return false if that list is empty (nothing to prefetch, prefetching fails).
		InternalPruneFilesThatDoNotExist(rvFiles);
		if (rvFiles.IsEmpty())
		{
			return false;
		}
	}

	// Check if we need to fetch.
	if (InternalAreFilesFetched(rvFiles))
	{
		return true;
	}

	// Fetch the files.
	m_TaskQueue.Fetch(rvFiles, ePriority);
	m_Signal.Activate();

	return true;
}

/**
 * Checks if all vFiles have been fetched (downloaded and have valid Crc32 codes).
 * This method assumes that Exists() returns true for all files in vFiles.
 */
Bool DownloadablePackageFileSystem::InternalAreFilesFetched(const Files& vFiles) const
{
	// Cache the number of files.
	UInt32 const nFiles = vFiles.GetSize();

	Bool bFetched = true;
	for (UInt32 i = 0; i < nFiles; ++i)
	{
		if (!m_tCrc32CheckTable.IsCrc32Ok(vFiles[i]))
		{
			bFetched = false;
			break;
		}
	}

	return bFetched;
}

void DownloadablePackageFileSystem::InternalPruneFilesThatDoNotExist(Files& rvFiles) const
{
	Int32 nFiles = (Int32)rvFiles.GetSize();
	for (Int32 i = 0; i < nFiles; ++i)
	{
		if (!m_pPackageFileSystem->Exists(rvFiles[i]))
		{
			Swap(rvFiles[i], rvFiles[nFiles-1]);
			--nFiles;
			--i;
		}
	}

	rvFiles.Resize((UInt32)nFiles);
}

void DownloadablePackageFileSystem::InternalUpdateMaxSizePerDownloadInBytes(
	Int64 iDownloadStartTimeInTicks,
	Int64 iDownloadEndTimeInTicks,
	UInt64 zDownloadSizeInBytes)
{
	// Compute download time.
	Double const fDownloadTimeInSeconds = SeoulTime::ConvertTicksToSeconds(iDownloadEndTimeInTicks - iDownloadStartTimeInTicks);

	// If the time was greater than our target, try to adjust the max download size down.
	if (fDownloadTimeInSeconds > m_Settings.m_fTargetPerDownloadTimeInSeconds)
	{
		// Clamp between upper and lower bounds so we end up with something reasonable.
		m_zMaxSizePerDownloadInBytes = Clamp(
			m_zMaxSizePerDownloadInBytes >> 1,
			m_Settings.m_uLowerBoundMaxSizePerDownloadInBytes,
			m_Settings.m_uUpperBoundMaxSizePerDownloadInBytes);
	}
	// If the size was greater than half our target and our download time was less than
	// half our target, try to adjust the max download size up.
	else if (zDownloadSizeInBytes >= (m_zMaxSizePerDownloadInBytes / 2) && fDownloadTimeInSeconds < (0.5 * m_Settings.m_fTargetPerDownloadTimeInSeconds))
	{
		// Clamp between upper and lower bounds so we end up with something reasonable.
		m_zMaxSizePerDownloadInBytes = Clamp(
			m_zMaxSizePerDownloadInBytes << 1,
			m_Settings.m_uLowerBoundMaxSizePerDownloadInBytes,
			m_Settings.m_uUpperBoundMaxSizePerDownloadInBytes);
	}
}

// Keys used for stat tracking.
static const HString kaInitStateNames[] =
{
	HString("init_request_header"), // kRequestHeader,
	HString("init_waiting_for_header"), // kWaitingForHeader,
	HString("init_received_header"), // kReceivedHeader,
	HString("init_check_existing_package"), // kCheckExistingPackage,
	HString("init_request_file_table"), // kRequestFileTable,
	HString("init_waiting_for_file_table"), // kWaitingForFileTable,
	HString("init_received_file_table"), // kReceivedFileTable,
	HString("init_update_and_reload_package"), // kUpdateAndReloadPackage,
	HString("init_error"), // kError,
	HString("init_complete"), // kComplete,
};
SEOUL_STATIC_ASSERT(SEOUL_ARRAY_COUNT(kaInitStateNames) - 1 == DownloadPackageInitializationUtility::kComplete);
static const HString kaInitErrorStateNames[] =
{
	HString("initerr_request_header"), // kRequestHeader,
	HString("initerr_waiting_for_header"), // kWaitingForHeader,
	HString("initerr_received_header"), // kReceivedHeader,
	HString("initerr_check_existing_package"), // kCheckExistingPackage,
	HString("initerr_request_file_table"), // kRequestFileTable,
	HString("initerr_waiting_for_file_table"), // kWaitingForFileTable,
	HString("initerr_received_file_table"), // kReceivedFileTable,
	HString("initerr_update_and_reload_package"), // kUpdateAndReloadPackage,
	HString("initerr_error"), // kError,
	HString("initerr_complete"), // kComplete,
};
SEOUL_STATIC_ASSERT(SEOUL_ARRAY_COUNT(kaInitErrorStateNames) - 1 == DownloadPackageInitializationUtility::kComplete);

// Utility that scopes a block and appends the time spent in that block
// to the downloader's time stats.
namespace
{

struct FindByFileOffset SEOUL_SEALED
{
	Bool operator()(const PackageCrc32Entry& a, Int64 iOffset) const
	{
		return ((Int64)a.m_Entry.m_uOffsetToFile < iOffset);
	}

	Bool operator()(Int64 iOffset, const PackageCrc32Entry& b) const
	{
		return (iOffset < (Int64)b.m_Entry.m_uOffsetToFile);
	}
};

class ScopedMeasure SEOUL_SEALED
{
public:
	ScopedMeasure(HString name, DownloadablePackageFileSystem::StatTracker& r)
		: m_iStart(SeoulTime::GetGameTimeInTicks())
		, m_r(r)
		, m_Name(name)
	{
	}

	~ScopedMeasure()
	{
		auto const iEnd = SeoulTime::GetGameTimeInTicks();
		m_r.OnDeltaTime(m_Name, (iEnd - m_iStart));
	}

private:
	SEOUL_DISABLE_COPY(ScopedMeasure);

	Int64 const m_iStart;
	DownloadablePackageFileSystem::StatTracker& m_r;
	HString const m_Name;
}; // class ScopedMeasure

} // namespace anonymous
#define SEOUL_EVENT(name, ...) \
	static const HString kEventName_##name(#name); \
	m_StatTracker.OnEvent(kEventName_##name, ##__VA_ARGS__)
#define SEOUL_SCOPED_MEASURE_IMPL(name, count) \
	static const HString kScopedName_##name(#name); \
	ScopedMeasure const scoped_##count(kScopedName_##name, m_StatTracker)
#define SEOUL_SCOPED_MEASURE(name) SEOUL_SCOPED_MEASURE_IMPL(name, __COUNT__)

Int DownloadablePackageFileSystem::InternalWorkerThread(const Thread&)
{
	// Initialize - output will be a full list of all files
	// in this archive, in file offset order.
	PackageCrc32Entries vEntriesByFileOrder;
	InternalWorkerThreadInitialize(vEntriesByFileOrder);

	// Data structures used to track entries to fetch and read.
	FetchEntries vFetchEntries;
	FetchTable tFetchTable;

	// Now loop until shutdown, performing prefetch tasks as requested.
	while (m_bWorkerThreadRunning)
	{
		// If we have no pending fetch or read entries, wait for more work.
		if (tFetchTable.IsEmpty() &&!m_TaskQueue.HasEntries())
		{
			// Wait to be activated.
			m_bWorkerThreadWaiting = true;
			m_Signal.Wait();
			m_bWorkerThreadWaiting = false;
		}

		// Get lists of work to do.
		{
			TaskQueue::FetchList vToFetch;
			m_TaskQueue.PopAll(vToFetch);

			// Accumulate files into our running fetch list.
			{
				SEOUL_SCOPED_MEASURE(loop_accum);

				// Cache the package file system table.
				const PackageFileSystem::FileTable& tFileTable = m_pPackageFileSystem->GetFileTable();

				UInt32 const nNewFetches = vToFetch.GetSize();
				for (UInt32 i = 0; i < nNewFetches; ++i)
				{
					// Cache the entry.
					TaskQueue::FetchEntry const inEntry = vToFetch[i];

					// If the entry already exists, just update the priority.
					{
						FetchEntry* pOutEntry = tFetchTable.Find(inEntry.m_FilePath);
						if (nullptr != pOutEntry)
						{
							pOutEntry->m_ePriority = (NetworkFetchPriority)Max((Int)inEntry.m_ePriority, (Int)pOutEntry->m_ePriority);
							continue;
						}
					}

					// Otherwise, add the entry if it exists (just for sanity, we shouldn't
					// get this far if inEntry.m_FilePath is not in our table) and is not
					// crc32 valid yet.
					PackageFileTableEntry packageFileTableEntry;
					if (tFileTable.GetValue(inEntry.m_FilePath, packageFileTableEntry) &&
						!m_tCrc32CheckTable.IsCrc32Ok(inEntry.m_FilePath))
					{
						FetchEntry entry;
						entry.m_zInProgressBytesCommitted = 0;
						entry.m_Entry = packageFileTableEntry;
						entry.m_FilePath = inEntry.m_FilePath;
						entry.m_ePriority = inEntry.m_ePriority;
						SEOUL_VERIFY(tFetchTable.Insert(entry.m_FilePath, entry).Second);
					}
				}
			}

			// Prepare the fetch entries list for processing.
			if (!tFetchTable.IsEmpty())
			{
				SEOUL_SCOPED_MEASURE(loop_fetch_sort);

				SEOUL_ASSERT(vFetchEntries.IsEmpty()); // Must be true by this point.
				vFetchEntries.Clear(); // Sanity.

				auto const iBegin = tFetchTable.Begin();
				auto const iEnd = tFetchTable.End();
				for (auto i = iBegin; iEnd != i; ++i)
				{
					vFetchEntries.PushBack(i->Second);
				}

				// Sort by priority.
				QuickSort(vFetchEntries.Begin(), vFetchEntries.End());
			}

			// Process the entries.
			if (!vFetchEntries.IsEmpty())
			{
				SEOUL_SCOPED_MEASURE(loop_process);
				SEOUL_EVENT(loop_process_count);

				InternalPerformFetch(
					vEntriesByFileOrder,
					vFetchEntries);

				// Update the Fetch table.
				UInt32 const nFetchEntries = vFetchEntries.GetSize();
				for (UInt32 i = 0; i < nFetchEntries; ++i)
				{
					// Cache the entry FilePath.
					FilePath const filePath = vFetchEntries[i].m_FilePath;

					// If the entry has a bytes committed value > 0, just merge that new value into the table.
					if (vFetchEntries[i].m_zInProgressBytesCommitted > 0)
					{
						FetchEntry* pEntry = tFetchTable.Find(filePath);
						SEOUL_ASSERT(nullptr != pEntry);

						pEntry->m_zInProgressBytesCommitted = Max(pEntry->m_zInProgressBytesCommitted, vFetchEntries[i].m_zInProgressBytesCommitted);
					}
					// Otherwise, if the Crc32 of the entry is now valid, remove it from the fetch table.
					else if (m_tCrc32CheckTable.IsCrc32Ok(filePath))
					{
						SEOUL_VERIFY(tFetchTable.Erase(filePath));
					}
				}
			}

			// Clear the entries list.
			vFetchEntries.Clear();

			// If the entries table is now empty, swap both the table and list
			// to release memory.
			if (tFetchTable.IsEmpty())
			{
				FetchTable emptyFetchTable;
				tFetchTable.Swap(emptyFetchTable);

				FetchEntries emptyFetchEntries;
				vFetchEntries.Swap(emptyFetchEntries);
			}
		}
	}

	return 0;
}

void DownloadablePackageFileSystem::InternalWorkerThreadInitialize(PackageCrc32Entries& rvAllEntries)
{
	// Local structure, don't set to out until we're about to
	// return successfully.
	PackageCrc32Entries vEntriesByFileOrder;

	{
		SEOUL_SCOPED_MEASURE(init);

		// Track whether the package is nwe this run or not.
		Bool bNewPackage = false;

		// Poll the initialization utility until it returns kComplete,
		// or until we're shutting down.
		{
			DownloadPackageInitializationUtility utility(*this);

			auto iLastStateChangeTime = SeoulTime::GetGameTimeInTicks();
			DownloadPackageInitializationUtility::State eState = utility.Poll();
			while (DownloadPackageInitializationUtility::kComplete != eState)
			{
				// If we're shutting down mid wait, return immediately.
				if (!m_bWorkerThreadRunning)
				{
					return;
				}

				auto const ePrevState = eState;
				eState = utility.Poll();
				if (ePrevState != eState)
				{
					auto const iCurrentTime = SeoulTime::GetGameTimeInTicks();
					auto const iDeltaTime = iCurrentTime - iLastStateChangeTime;

					// Track for all.
					{
						// We track two stats on state changes - accumulate time spent
						// in the state itself, and when transitioning to an error
						// state, we track the number of times the error state was
						// entered from the previous state.
						auto const stateName = kaInitStateNames[ePrevState];
						m_StatTracker.OnDeltaTime(stateName, iDeltaTime);

						// Transition to error.
						if (ePrevState != DownloadPackageInitializationUtility::kError &&
							eState == DownloadPackageInitializationUtility::kError)
						{
							auto const errorName = kaInitErrorStateNames[ePrevState];
							m_StatTracker.OnEvent(errorName);
						}
					}

					// Log for developers.
					SEOUL_LOG_ENGINE("DownloadablePacakgeFileSystem::Init(%s): %s -> %s (%.2f s)",
						Path::GetFileNameWithoutExtension(GetAbsolutePackageFilename()).CStr(),
						EnumToString<DownloadPackageInitializationUtility::State>(ePrevState),
						EnumToString<DownloadPackageInitializationUtility::State>(eState),
						SeoulTime::ConvertTicksToSeconds(iDeltaTime));

					iLastStateChangeTime = iCurrentTime;
				}
			}

			// Track whether we're starting with a new package or not.
			bNewPackage = utility.IsNewPackage();
		}

		// Now that basic init is done, proceed in one
		// of two approaches based on bNewPackage:
		// - if the package on disk is new, we want to perform
		//   populate steps *first* against an assumption of all
		//   data being invalid.
		// - otherwise, we assume that data on disk is mostly valid
		//   and perform populate steps last if there are still
		//   missing files.

		// Track whether we have an old archive to process during populate.
		String const sOldFilename(m_Settings.m_sAbsolutePackageFilename + ".old");

		Bool bHasOld = false;
		if (DiskSyncFile::FileExists(sOldFilename))
		{
			bHasOld = true;
		}

		// If bNewPackage is true, perform the populate step now.
		if (bNewPackage)
		{
			SEOUL_SCOPED_MEASURE(init_populate);

			// First build our entries list with all false values.
			m_pPackageFileSystem->GetFileTableAsEntries(vEntriesByFileOrder);

			// Now initialize the CRC32 table from the entries list - this
			// wil pre-populate it with all false values.
			m_tCrc32CheckTable.Initialize(vEntriesByFileOrder);

			// Buffer for processing.
			UInt32 uBuffer = 0u;
			void* pBuffer = nullptr;
			auto const scoped(MakeScopedAction(
				[]() {},
				[&]()
			{
				auto p = pBuffer;
				pBuffer = nullptr;
				MemoryManager::Deallocate(p);
			}));

			// Apply old if we have it.
			if (bHasOld)
			{
				InternalPerformPopulateFrom(pBuffer, uBuffer, sOldFilename, true);
			}

			// Any additional packages.
			for (auto const& s : m_Settings.m_vPopulatePackages)
			{
				InternalPerformPopulateFrom(pBuffer, uBuffer, s, false);
			}
		}

		// Before the final CRC32 check, make sure the compression dictionary is initialized,
		// if present.
		{
			SEOUL_SCOPED_MEASURE(init_cdict);

			// Cache to the package file's compression dictionary.
			auto const dictFilePath = m_pPackageFileSystem->GetCompressionDictFilePath();

			// If the fetch list is not empty, and the archive has a compression dictionary
			// that has not yet been populated, make sure we fetch it now. Other fetches
			// will fail to decompress if the dictionary is not ready and valid.
			PackageFileTableEntry dictEntry;
			if (dictFilePath.IsValid() &&
				!m_pPackageFileSystem->IsCompressionDictProcessed() &&
				m_pPackageFileSystem->GetFileTable().GetValue(dictFilePath, dictEntry) &&
				dictEntry.m_Entry.m_uCompressedFileSize > 0u)
			{
				// Allocate a buffer big enough for the data.
				auto const uSize = (UInt32)dictEntry.m_Entry.m_uCompressedFileSize;
				Vector<Byte, MemoryBudgets::Io> vDictData(uSize);

				// Loop until successful.
				while (!m_pPackageFileSystem->ProcessCompressionDict())
				{
					// Return immediately on shutdown.
					if (!m_bWorkerThreadRunning)
					{
						return;
					}

					SEOUL_SCOPED_MEASURE(init_cdict_download);
					SEOUL_EVENT(init_cdict_download_count);
					SEOUL_EVENT(init_cdict_download_bytes, uSize);

					// Download the data.
					if (DownloadablePackageFileSystemHelpers::Download(
						m_bWorkerThreadRunning,
						m_pRequestList.Get(),
						vDictData.Data(),
						*this,
						dictEntry.m_Entry.m_uOffsetToFile,
						uSize))
					{
						// Commit, don't bother checking this - ProcessCompressionDict() does that for us.
						(void)m_pPackageFileSystem->CommitChangeToSarFile(
							vDictData.Data(),
							uSize,
							(Int64)dictEntry.m_Entry.m_uOffsetToFile);
					}
				}
			}
		}

		// If we started fresh, CRC32 information has
		// been populated already from existing archives.
		//
		// Otherwise, initialize CRC32 from our archive on
		// disk and then perform the population step.
		if (!bNewPackage)
		{
			SEOUL_SCOPED_MEASURE(init_populate);

			// Initialize from a CRC32 check.
			{
				SEOUL_SCOPED_MEASURE(init_crc);
				(void)m_pPackageFileSystem->PerformCrc32Check(&vEntriesByFileOrder);
			}

			// Propagate to check table.
			m_tCrc32CheckTable.Initialize(vEntriesByFileOrder);

			// Buffer for processing.
			UInt32 uBuffer = 0u;
			void* pBuffer = nullptr;
			auto const scoped(MakeScopedAction(
				[]() {},
				[&]()
			{
				auto p = pBuffer;
				pBuffer = nullptr;
				MemoryManager::Deallocate(p);
			}));

			// Apply old if we have it.
			if (bHasOld)
			{
				InternalPerformPopulateFrom(pBuffer, uBuffer, sOldFilename, true);
			}

			// Any additional packages.
			for (auto const& s : m_Settings.m_vPopulatePackages)
			{
				InternalPerformPopulateFrom(pBuffer, uBuffer, s, false);
			}
		}
	}

	// Initialization is complete - order here is important. Completion
	// first, then signal to the outside word that we're done initializing.
	SeoulMemoryBarrier();
	m_bInitializationComplete = true;
	SeoulMemoryBarrier();
	m_bDoneInitializing = true;

	// Populate out.
	rvAllEntries.Swap(vEntriesByFileOrder);
}

/**
 * Construct a sorted list of fetch/download sets. Fetch entries are ordered by priority and
 * order in the .sar file, to facilitate contiguous downloads. Once those download sets are built, we
 * reorder the sets based on priority and a ratio, which is (# of files in set / size of download size).
 * A larger ratio implies more files in less size - in otherwords, we prioritize downloads which will
 * deliver more files, faster.
 */
void DownloadablePackageFileSystem::InternalBuildFetchSets(const FetchEntries& vFetchEntries, FetchSets& rvFetchSets)
{
	// Cache the number of entries.
	UInt32 const nEntries = vFetchEntries.GetSize();

	// iFirstDownload is the first index into the list of entries we'll download in
	// one transfer, iLastDownload is the last index into the list of entries.
	Int iFirstDownload = -1;
	Int iLastDownload = -1;

	// Walk the list of entries.
	for (UInt32 i = 0; i < nEntries; ++i)
	{
		// Cache the entry.
		FetchEntry const entry = vFetchEntries[i];

		// Update the iFirstDownload and iLastDownload indices.
		{
			// If we don't have a first index yet, set it, unless
			// the entry is a "big download".
			if (iFirstDownload < 0)
			{
				// If the fetch entry is already partially downloaded, or if
				// its total size is > m_zMaxSizePerDownloadInBytes, it
				// will be its own download entry (it is a "big download").
				if (entry.m_zInProgressBytesCommitted > 0 ||
					entry.m_Entry.m_Entry.m_uCompressedFileSize > m_zMaxSizePerDownloadInBytes)
				{
					FetchSet set;
					set.m_ePriority = entry.m_ePriority;
					set.m_iFirstDownload = (Int32)i;
					set.m_iLastDownload = (Int32)i;
					set.m_zSizeInBytes = entry.m_Entry.m_Entry.m_uCompressedFileSize;
					rvFetchSets.PushBack(set);

					continue;
				}

				// Set the start of the download set.
				iFirstDownload = (Int)i;
			}

			// Always set the last index.
			iLastDownload = (Int)i;
		}

		// Check if we should insert a set entry for the current group or not.
		if (iFirstDownload >= 0 && iLastDownload >= 0)
		{
			// Compute starting, ending offsets and then compute the total size.
			UInt64 const zStartingOffset = vFetchEntries[iFirstDownload].m_Entry.m_Entry.m_uOffsetToFile;
			UInt64 const zEndingOffset = vFetchEntries[iLastDownload].m_Entry.m_Entry.m_uOffsetToFile + vFetchEntries[iLastDownload].m_Entry.m_Entry.m_uCompressedFileSize;
			UInt32 const zSize = (UInt32)(zEndingOffset - zStartingOffset);

			// We treat the current set as a download if:
			// - we're at the end of the list.
			// - the current priority is not equal to the next priority.
			// - the next entry has a bytes committed value > 0.
			// - the next entry is too far from the last entry (based on m_Settings.m_uMaxRedownloadSizeThresholdInBytes).
			// - the total size of the download including the next entry is too big (based on m_zMaxSizePerDownloadInBytes).
			Bool bDownload = false;
			if (i + 1 == nEntries)
			{
				bDownload = true;
			}
			else if (vFetchEntries[i+1].m_ePriority != entry.m_ePriority)
			{
				bDownload = true;
			}
			else if (vFetchEntries[i+1].m_zInProgressBytesCommitted > 0)
			{
				bDownload = true;
			}
			else if ((vFetchEntries[i+1].m_Entry.m_Entry.m_uOffsetToFile - zEndingOffset) > m_Settings.m_uMaxRedownloadSizeThresholdInBytes)
			{
				bDownload = true;
			}
			else
			{
				// Size of the next block - this is the offset to the next block, minus the ending offset
				// of the current download set, plus the size of the next block.
				UInt32 const zNextSize = (UInt32)(
					(vFetchEntries[i+1].m_Entry.m_Entry.m_uOffsetToFile - zEndingOffset) +
					vFetchEntries[i+1].m_Entry.m_Entry.m_uCompressedFileSize);

				// Total size of the current download set with the next block.
				UInt32 const zSizeWithNextEntry = (zSize + zNextSize);

				// Download if the total of the next block and the current download set
				// would exceed the maximum desired download size per operation.
				bDownload = (zSizeWithNextEntry > m_zMaxSizePerDownloadInBytes);
			}

			// If bDownload is true, insert a set entry for the group and reset the iFirstDownload/iLastDownload variables.
			// Perform the download now if specified.
			if (bDownload)
			{
				FetchSet set;
				set.m_ePriority = vFetchEntries[iLastDownload].m_ePriority;
				set.m_iFirstDownload = iFirstDownload;
				set.m_iLastDownload = iLastDownload;
				set.m_zSizeInBytes = zSize;
				rvFetchSets.PushBack(set);

				// Clear the first/last markers.
				iFirstDownload = -1;
				iLastDownload = -1;
			}
		}
	}

	// To finalize, sort the fetch sets. If this is close
	// to a full archive fetch (within 90%), don't re-order
	// (just download in first-to-last order). Otherwise, re-order
	// to preference sets that will retrieve more individual files at a time.
	//
	// Higher priority sets will still take priority.
	if ((Int32)vFetchEntries.GetSize() >= (Int32)(0.9f * (Int32)m_pPackageFileSystem->GetHeader().GetTotalEntriesInFileTable()))
	{
		OrderByOffset offset;
		QuickSort(rvFetchSets.Begin(), rvFetchSets.End(), offset);
	}
	else
	{
		OrderByRatio ratio;
		QuickSort(rvFetchSets.Begin(), rvFetchSets.End(), ratio);
	}
}

void DownloadablePackageFileSystem::InternalPerformFetch(
	const PackageCrc32Entries& vEntriesByFileOrder,
	FetchEntries& rvFetchEntries)
{
	// Vector used to accumulate fetch sets.
	FetchSets vSets;

	{
		SEOUL_SCOPED_MEASURE(loop_build_fetch_sets);

		// Build the ordered list of fetch sets.
		InternalBuildFetchSets(rvFetchEntries, vSets);
	}

	// Cache the total number of sets.
	UInt32 const nSets = vSets.GetSize();

	// If nSets is 0, we're done.
	if (0 == nSets)
	{
		return;
	}

	// Perform fetch operations.
	{
		// Track the last max download size. When this changes, we return from this function to
		// restart the fetch operation. This is a little figgly - it depends on the knowledge
		// that the caller will keep calling InternalPerformFetch() with fetch entries that we
		// did not successfully download (based on whether m_tCrc32Table.IsCrc32Ok() returns
		// true or not for the file).
		UInt32 zLastMaxSizePerDownloadInBytes = m_zMaxSizePerDownloadInBytes;

		// Walk the list of sets.
		for (UInt32 i = 0; i < nSets; ++i)
		{
			SEOUL_EVENT(loop_fetch_set_count);

			// If the worker thread has shutdown, return immediately.
			if (!m_bWorkerThreadRunning)
			{
				return;
			}

			// Before looping, check if the max download size in bytes has changed. If so, return immediately.
			// We do this to restart the operation, so that the fetch sets will be recomputed based on the
			// new max download size.
			if (zLastMaxSizePerDownloadInBytes != m_zMaxSizePerDownloadInBytes)
			{
				return;
			}

			// Update the last max download size in bytes.
			zLastMaxSizePerDownloadInBytes = m_zMaxSizePerDownloadInBytes;

			// Before looping, check if the fetch queue has new entries. If so, return immediately.
			if (m_TaskQueue.HasEntries())
			{
				return;
			}

			// Cache the set entry.
			FetchSet const set = vSets[i];

			// Perform the set download.
			{
				// Cache the set download size and starting offset.
				UInt32 const zSize = (UInt32)set.m_zSizeInBytes;
				UInt64 const zStartingOffset = (UInt64)rvFetchEntries[set.m_iFirstDownload].m_Entry.m_Entry.m_uOffsetToFile;

				// Allocate a buffer for the data to be downloaded.
				void* pData = MemoryManager::Allocate(zSize, MemoryBudgets::Io);

				// Time the download operation.
				Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
				++m_NetworkFileRequestsIssued;

				// Download it - if successful, verify all the entries.
				Bool bDownloadSuccessful = false;
				{
					SEOUL_SCOPED_MEASURE(loop_download);
					SEOUL_EVENT(loop_download_count);
					SEOUL_EVENT(loop_download_bytes, zSize);
					bDownloadSuccessful = DownloadablePackageFileSystemHelpers::Download(
						m_bWorkerThreadRunning,
						m_pRequestList.Get(),
						pData,
						*this,
						zStartingOffset,
						zSize);
				}

				// Time the download operation.
				++m_NetworkFileRequestsCompleted;
				Int64 const iEndTimeInTicks = SeoulTime::GetGameTimeInTicks();
				m_NetworkTimeMilliseconds += (Atomic32Type)SeoulTime::ConvertTicksToMilliseconds(iEndTimeInTicks - iStartTimeInTicks);
				m_NetworkBytes += zSize;

				// Prior to additional process, gather the entire
				// list of files that were downloaded based on
				// the range - this can include files not explicitly
				// requested that were in-between the explicitly
				// downloaded files but were downloaded as part of
				// allowed overflow, to reduce the total number of download
				// operations.
				FindByFileOffset pred;
				auto const iFirst = LowerBound(
					vEntriesByFileOrder.Begin(),
					vEntriesByFileOrder.End(),
					zStartingOffset,
					pred);
				auto const iLast = UpperBound(
					vEntriesByFileOrder.Begin(),
					vEntriesByFileOrder.End(),
					rvFetchEntries[set.m_iLastDownload].m_Entry.m_Entry.m_uOffsetToFile,
					pred) - 1;

				// Sanity checks - other code must ensure that we
				// find both first and last and that
				// the range starts at first and ends with
				// the end of last.
				SEOUL_ASSERT(
					iFirst == vEntriesByFileOrder.Begin() ||
					(iFirst - 1)->m_Entry.m_uOffsetToFile < zStartingOffset);
				SEOUL_ASSERT(
					(iLast + 1) == vEntriesByFileOrder.End() ||
					(iLast + 1)->m_Entry.m_uOffsetToFile >= rvFetchEntries[set.m_iLastDownload].m_Entry.m_Entry.m_uOffsetToFile + rvFetchEntries[set.m_iLastDownload].m_Entry.m_Entry.m_uCompressedFileSize);
				SEOUL_ASSERT(vEntriesByFileOrder.End() != iFirst);
				SEOUL_ASSERT(vEntriesByFileOrder.End() != iLast);
				SEOUL_ASSERT(iFirst->m_Entry.m_uOffsetToFile == rvFetchEntries[set.m_iFirstDownload].m_Entry.m_Entry.m_uOffsetToFile);
				SEOUL_ASSERT(iLast->m_Entry.m_uOffsetToFile == rvFetchEntries[set.m_iLastDownload].m_Entry.m_Entry.m_uOffsetToFile);
				SEOUL_ASSERT(zStartingOffset == rvFetchEntries[set.m_iFirstDownload].m_Entry.m_Entry.m_uOffsetToFile);
				SEOUL_ASSERT(zStartingOffset + zSize == rvFetchEntries[set.m_iLastDownload].m_Entry.m_Entry.m_uOffsetToFile + rvFetchEntries[set.m_iLastDownload].m_Entry.m_Entry.m_uCompressedFileSize);

				// If we have post CRC32 values (CRC32 values taken
				// after any obfuscation or compression has been applied),
				// we can immediately validate all CRC32 values against
				// the data in memory prior to commit to disk.
				auto const bHasPostCrc32 = m_pPackageFileSystem->HasPostCrc32();
				if (bDownloadSuccessful && bHasPostCrc32)
				{
					// Walk the range and verify CRC32 of all.
					for (auto j = iFirst; j <= iLast; ++j)
					{
						// Cache entries for further processing.
						auto const& entry = *j;
						auto pEntryData = ((UInt8 const*)pData) + (size_t)(entry.m_Entry.m_uOffsetToFile - zStartingOffset);

						// Check CRC32.
						UInt32 const uCheckCrc32 = Seoul::GetCrc32(
							pEntryData,
							(size_t)entry.m_Entry.m_uCompressedFileSize);

						// On a check failure, abort the entire remainder of the operation.
						// Assume we got a bad buffer from the server.
						if (uCheckCrc32 != entry.m_Entry.m_uCrc32Post)
						{
							// Any failures, immediately set bDownloadSuccessful
							// to false and break out of this processing loop.
							bDownloadSuccessful = false;
							break;
						}
					}
				}

				// If the download was successful, commit the data.
				if (bDownloadSuccessful)
				{
					SEOUL_SCOPED_MEASURE(loop_commit);
					SEOUL_EVENT(loop_commit_count);
					if (!m_pPackageFileSystem->CommitChangeToSarFile(
						pData,
						zSize,
						zStartingOffset))
					{
						bDownloadSuccessful = false;
					}
				}

				// Last bit -if everything else was successful, we have
				// one final step here. If we had post CRC32 values,
				// we already know the CRC32 values of entries are ok,
				// so we can just commit them. Otherwise, we need to
				// check them via the inner package system.
				if (bDownloadSuccessful)
				{
					// Walk the range.
					for (auto j = iFirst; j <= iLast; ++j)
					{
						// Cache the file path.
						auto const& filePath = j->m_FilePath;

						// Quick case, can just set since we verified above.
						if (bHasPostCrc32)
						{
							m_tCrc32CheckTable.SetCrc32Ok(filePath);
						}
						// Otherwise, need to check. Slow, fortunately, only
						// expected to happen on old archives with compression
						// or obfuscation, which is not expected to be common.
						else if (m_pPackageFileSystem->PerformCrc32Check(filePath))
						{
							m_tCrc32CheckTable.SetCrc32Ok(filePath);
						}
						// On any failure, something unexpected happened - set
						// download to false so that we try again.
						else
						{
							bDownloadSuccessful = false;
						}
					}
				}

				// Release the download buffer.
				MemoryManager::Deallocate(pData);

				// If the download was successful, adjust the max download size for the next
				// download operation.
				if (bDownloadSuccessful)
				{
					InternalUpdateMaxSizePerDownloadInBytes(
						iStartTimeInTicks,
						iEndTimeInTicks,
						zSize);
				}

				// If the download failed, return immediately.
				if (!bDownloadSuccessful)
				{
					return;
				}
			}
		}
	}
}

/**
 * Called from worker thread only, performs operations
 * necessary to pull in files from a secondary archive
 * into the current archive.
 */
void DownloadablePackageFileSystem::InternalPerformPopulateFrom(
	void*& pBuffer,
	UInt32& uBuffer,
	const String& sAbsolutePathToPackageFile,
	Bool bDeleteAfterPopulate)
{
	// Always perform delete on exit if requested.
	auto const scoped(MakeScopedAction([](){},
	[&]()
	{
		// Delete the cache file if requested.
		if (bDeleteAfterPopulate)
		{
			(void)DiskSyncFile::DeleteFile(sAbsolutePathToPackageFile);
		}
	}));

	// Nothing to do if already CRC32 ok.
	if (m_tCrc32CheckTable.AllCrc32Ok())
	{
		return;
	}

	// Populate query set - this is the list of
	// remaining files we need to be crc32 ok before
	// the entire archive is valid.
	PackageCrc32Entries vCacheResults;
	m_tCrc32CheckTable.GetRemainingNotOk(vCacheResults);

	// Populate from cache.
	{
		ScopedPtr<PackageFileSystem> pCache;
		{
			SEOUL_SCOPED_MEASURE(init_populate_cache);
			pCache.Reset(SEOUL_NEW(MemoryBudgets::Io) PackageFileSystem(sAbsolutePathToPackageFile));
		}

		// Archive is invalid, skip it.
		if (!pCache->IsOk())
		{
			return;
		}

		// Incompatibility between source and target, skip it.
		if (!AreCompatible(*m_pPackageFileSystem, *pCache))
		{
			return;
		}

		// Perform a crc32 on the input archive to establish
		// what is useful from that archive. We don't care
		// about return value here, only individual values.
		{
			SEOUL_SCOPED_MEASURE(init_populate_crc);

			// Gather entries from the cache that we can use.
			(void)pCache->PerformCrc32Check(&vCacheResults);
		}

		// Cache our file table.
		const PackageFileSystem::FileTable& tCurrentFileTable = m_pPackageFileSystem->GetFileTable();

		// Enumerate list and check against our file table
		// to determine what is valueable to copy through.
		for (auto const& e : vCacheResults)
		{
			// Skip entries not valid in the cache.
			if (!e.m_bCrc32Ok)
			{
				continue;
			}

			// Cache some useful values.
			auto const& filePath = e.m_FilePath;
			auto const& cacheEntry = e.m_Entry;

			// Get the corresponding entry - this is not expected
			// to fail (since the entry would not have been reported
			// via GetQuery()) but we handle it here by skipping
			// the entry.
			PackageFileTableEntry currentEntry;
			if (!tCurrentFileTable.GetValue(filePath, currentEntry))
			{
				continue;
			}

			// If data is not a match, skip it.
			if (cacheEntry.m_uCompressedFileSize != currentEntry.m_Entry.m_uCompressedFileSize ||
				cacheEntry.m_uUncompressedFileSize != currentEntry.m_Entry.m_uUncompressedFileSize ||
				cacheEntry.m_uCrc32Pre != currentEntry.m_Entry.m_uCrc32Pre)
			{
				continue;
			}

			// Read the data - if the read succeeds and the data size
			// is as expected, commit the data to our file system.
			//
			// NOTE: ReadRaw reads the data directly from the PackageFileSystem, without deobfuscating
			// or decompressing the data (which is what we need in this case).
			auto const uDataSizeInBytes = (UInt32)cacheEntry.m_uCompressedFileSize;

			// Resize our intermediate buffer if needed before reading.
			if (uDataSizeInBytes > uBuffer)
			{
				pBuffer = MemoryManager::Reallocate(pBuffer, uDataSizeInBytes, MemoryBudgets::Io);
				uBuffer = uDataSizeInBytes;
			}

			// Perform the read.
			Bool bReadRaw = false;
			{
				SEOUL_SCOPED_MEASURE(init_populate_readraw);
				bReadRaw = pCache->ReadRaw(cacheEntry.m_uOffsetToFile, pBuffer, uDataSizeInBytes);
			}

			// On success, commit the data.
			if (bReadRaw)
			{
				SEOUL_SCOPED_MEASURE(init_populate_commit);

				// Commit the data.
				if (m_pPackageFileSystem->CommitChangeToSarFile(
					pBuffer,
					uDataSizeInBytes,
					(Int64)currentEntry.m_Entry.m_uOffsetToFile))
				{
					// On a successful commit, mark the entry
					// as valid.
					m_tCrc32CheckTable.SetCrc32Ok(filePath);
				}
			}
		}
	}
}

/**
 * The only function of DownloadablePackageFileSystem that differs notably from
 * PackageFileSystem. The behavior of this method is as follows:
 * - initialization must complete
 * - filePath is opened via an underlying PackageFileSystem
 * - if a Crc32 check has been run on filePath already, behavior is identical to PackageFileSystem
 * - if a Crc32 check has not been run on filePath already, a special SyncFile will be returned
 *   which handles checking and (if necessary) downloading the file data when SyncFile::ReadRawData() is called.
 */
Bool DownloadablePackageFileSystem::Open(
	FilePath filePath,
	File::Mode eMode,
	ScopedPtr<SyncFile>& rpFile)
{
	// If the fetch operation fails, the entire operation fails.
	if (!Fetch(filePath, GetBestImplicitPriority(filePath)))
	{
		return false;
	}

	// Open the file with the internal PackageFileSystem.
	return m_pPackageFileSystem->Open(filePath, eMode, rpFile);
}

/**
 * Specialization for DownloadablePackageFileSystem, avoids the overhead
 * of caching an entire file in memory in cases where the entire file
 * will be read into memory anyway.
 */
Bool DownloadablePackageFileSystem::ReadAll(
	FilePath filePath,
	void*& rpOutputBuffer,
	UInt32& rzOutputSizeInBytes,
	UInt32 zAlignmentOfOutputBuffer,
	MemoryBudgets::Type eOutputBufferMemoryType,
	UInt32 zMaxReadSize /*= kDefaultMaxReadSize*/)
{
	// If the fetch operation fails, the entire operation fails.
	if (!Fetch(filePath, GetBestImplicitPriority(filePath)))
	{
		return false;
	}

	// Handle the operation with the internal PackageFileSystem.
	return m_pPackageFileSystem->ReadAll(
		filePath,
		rpOutputBuffer,
		rzOutputSizeInBytes,
		zAlignmentOfOutputBuffer,
		eOutputBufferMemoryType,
		zMaxReadSize);
}

/**
 * Spin wait in a Job-aware manner for initialization to complete,
 * or until uTimeoutInMs elapses.
 *
 * @param[in] uTimeoutInMs The time in milliseconds to wait. Waits
 *            indefinitely if this value is 0.
 *
 * @return true if initialization was completed successfully,
 * false otherwise.
 */
Bool DownloadablePackageFileSystem::WaitForInit(UInt32 uTimeoutInMs)
{
	if (0 != uTimeoutInMs)
	{
		auto const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
		while (
			m_bInitializationStarted &&
			!m_bInitializationComplete &&
			(UInt32)((Int32)Floor(SeoulTime::ConvertTicksToMilliseconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks))) < uTimeoutInMs)
		{
			Yield();
		}
	}
	else
	{
		while (m_bInitializationStarted && !m_bInitializationComplete)
		{
			Yield();
		}
	}

	return m_bInitializationComplete;
}

/**
 * Utility - (optionally) populates a query list for input with all files
 * that are still not CRC32 ok. Updates the internal AllOk flag based
 * on the results.
 */
void DownloadablePackageFileSystem::Crc32CheckTable::GetRemainingNotOk(PackageCrc32Entries& rv) const
{
	PackageCrc32Entry entry;
	rv.Clear();

	// Early out, easy case - if all ok, nothing to
	// query for.
	if (AllCrc32Ok()) { return; }

	{
		// Lock fo the body of this operation.
		Lock lock(m_Mutex);
		for (auto const& e : m_tCrc32CheckTable)
		{
			// False value indicates not valid
			// so output it to rv.
			if (!e.Second)
			{
				entry.m_FilePath = e.First;
				rv.PushBack(entry);
			}
		}
	}
}

/** Bulk initial population used during initialization. */
void DownloadablePackageFileSystem::Crc32CheckTable::Initialize(const PackageCrc32Entries& v)
{
	Lock lock(m_Mutex);

	// Reset not ok count.
	m_NotOkCount.Reset();

	for (auto const& e : v)
	{
		// Must succeed for all input entries.
		SEOUL_VERIFY(m_tCrc32CheckTable.Overwrite(e.m_FilePath, e.m_bCrc32Ok).Second);

		// Track.
		if (!e.m_bCrc32Ok)
		{
			++m_NotOkCount;
		}
	}
}

Bool DownloadablePackageFileSystem::Crc32CheckTable::IsCrc32Ok(FilePath filePath) const
{
	// Quick check without lock - if m_bAllOk has been set
	// true, it will remain so.
	if (AllCrc32Ok()) { return true; }

	Bool bValue = false;
	{
		Lock lock(m_Mutex);
		(void)m_tCrc32CheckTable.GetValue(filePath, bValue);
	}
	return bValue;
}

void DownloadablePackageFileSystem::Crc32CheckTable::SetCrc32Ok(FilePath filePath)
{
	Lock lock(m_Mutex);

	// Get the entry.
	auto pEntry = m_tCrc32CheckTable.Find(filePath);

	// Check - we should have pre-populated
	// the table with all possible entries, so
	// that we're just changing a false to a true,
	// never inserting a new value.
	SEOUL_ASSERT(nullptr != pEntry);

	// Sanity - expected to never be null but handle the
	// case where it is.
	if (nullptr != pEntry)
	{
		// Only update if false.
		if (false == *pEntry)
		{
			*pEntry = true;

			// Check - must always be true.
			SEOUL_ASSERT(m_NotOkCount > 0);

			// We're inside a mutex, so just check for sanity sake.
			if (m_NotOkCount > 0) { --m_NotOkCount; }
		}
	}
	// Otherwise, insert the entry.
	else
	{
		SEOUL_VERIFY(m_tCrc32CheckTable.Insert(filePath, true).Second);
	}
}

DownloadablePackageFileSystem::TaskQueue::TaskQueue()
	: m_tFetchTable()
	, m_Mutex()
{
}

DownloadablePackageFileSystem::TaskQueue::~TaskQueue()
{
}

void DownloadablePackageFileSystem::TaskQueue::OnNetworkInitialize()
{
	// Nop
}

void DownloadablePackageFileSystem::TaskQueue::OnNetworkShutdown()
{
	// Flush any remaining operations.
	{
		Lock lock(m_Mutex);
		m_tFetchTable.Clear();
	}
}

void DownloadablePackageFileSystem::TaskQueue::Fetch(FilePath filePath, NetworkFetchPriority ePriority)
{
	Lock lock(m_Mutex);

	// Check and use the higher priority.
	{
		NetworkFetchPriority eExistingPriority;
		if (m_tFetchTable.GetValue(filePath, eExistingPriority))
		{
			ePriority = Max(ePriority, eExistingPriority);
		}
	}

	// Overwrite the entry.
	SEOUL_VERIFY(m_tFetchTable.Overwrite(filePath, ePriority).Second);
}

void DownloadablePackageFileSystem::TaskQueue::Fetch(const Files& vFiles, NetworkFetchPriority ePriority)
{
	UInt32 const nFiles = vFiles.GetSize();

	Lock lock(m_Mutex);
	for (UInt32 i = 0; i < nFiles; ++i)
	{
		FilePath const filePath = vFiles[i];

		// Check and use the higher priority.
		{
			NetworkFetchPriority eExistingPriority;
			if (m_tFetchTable.GetValue(filePath, eExistingPriority))
			{
				ePriority = Max(ePriority, eExistingPriority);
			}
		}

		// Overwrite the entry.
		SEOUL_VERIFY(m_tFetchTable.Overwrite(filePath, ePriority).Second);
	}
}

Bool DownloadablePackageFileSystem::TaskQueue::HasEntries() const
{
	Lock lock(m_Mutex);
	return !m_tFetchTable.IsEmpty();
}

void DownloadablePackageFileSystem::TaskQueue::PopAll(FetchList& rvFetchList)
{
	Lock lock(m_Mutex);

	// Files to fetch.
	rvFetchList.Clear();
	{
		auto const iBegin = m_tFetchTable.Begin();
		auto const iEnd = m_tFetchTable.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			FetchEntry entry;
			entry.m_FilePath = i->First;
			entry.m_ePriority = i->Second;
			rvFetchList.PushBack(entry);
		}
	}
	m_tFetchTable.Clear();
}

} // namespace Seoul
