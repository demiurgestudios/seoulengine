/**
 * \file SaveLoadManager.cpp
 * \brief Global singleton that handles encrypted storage
 * of data blobs to disk and (optionally) remote cloud storage.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "CrashManager.h"
#include "DataStoreParser.h"
#include "EncryptAES.h"
#include "Engine.h"
#include "FileManager.h"
#include "JobsFunction.h"
#include "Logger.h"
#include "HashSet.h"
#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "HTTPResponse.h"
#include "MemoryBarrier.h"
#include "ReflectionDefine.h"
#include "ReflectionDeserialize.h"
#include "ReflectionSerialize.h"
#include "SaveApi.h"
#include "SaveLoadManager.h"
#include "SaveLoadUtil.h"
#include "ScopedAction.h"
#include "SeoulFile.h"
#include "SeoulSignal.h"
#include "SeoulUUID.h"
#include "Thread.h"

namespace Seoul
{

static const UInt32 kuSaveContainerSignature = 0x27eadb42;
static const Int32 kiMaxSaveContainerVersion = 3;
static const Int32 kiMinSaveContainerVersion = 3;

static const UInt8 kauKey[32] =
{
#if SEOUL_PLATFORM_WINDOWS
	0xd5, 0xd9, 0x74, 0xf6, 0xd0, 0xde, 0xbb, 0x13,
	0xe1, 0xa3, 0x1b, 0x7d, 0xbd, 0x24, 0xa8, 0x12,
	0x2d, 0x48, 0x01, 0x70, 0x01, 0xf1, 0x59, 0x35,
	0xca, 0xeb, 0xaf, 0x24, 0x22, 0x55, 0x83, 0x25,
#elif SEOUL_PLATFORM_ANDROID
	0x2f, 0x38, 0x5f, 0x28, 0x91, 0x0e, 0x5a, 0x55,
	0xbd, 0x51, 0xaa, 0x8e, 0xa6, 0x4c, 0xb7, 0x51,
	0xed, 0xae, 0xc6, 0xe6, 0x04, 0xb0, 0xe9, 0x03,
	0x3d, 0x9f, 0xd6, 0xd7, 0x57, 0xdd, 0xee, 0x8b,
#elif SEOUL_PLATFORM_IOS
	0xdf, 0x3b, 0xf1, 0xdd, 0xc3, 0x78, 0x3c, 0xe0,
	0x41, 0x33, 0x69, 0x28, 0x0b, 0x55, 0x2c, 0x54,
	0x64, 0xcd, 0x01, 0x07, 0xcf, 0x53, 0xcb, 0x97,
	0xf2, 0xb8, 0x62, 0x63, 0xaa, 0x7c, 0x48, 0xf4,
#elif SEOUL_PLATFORM_LINUX
	0x56, 0x70, 0x9c, 0x35, 0xb5, 0x39, 0xba, 0x23,
	0xaf, 0x06, 0x91, 0x2a, 0x43, 0xf5, 0x73, 0xdc,
	0x50, 0x9d, 0x96, 0x79, 0x1f, 0xfe, 0x0f, 0x9a,
	0x25, 0x20, 0x60, 0x37, 0x2c, 0x28, 0xd1, 0x5c,
#else
#error "Define for this platform"
#endif
};

namespace Reflection
{

/**
 * Context for saving and loading.
 *
 * Completely ignores some warnings/errors, and suppresses all other
 * errors in ship builds, issuing a warning dialogue in non-ship builds.
 */
class SaveLoadContext SEOUL_SEALED : public DefaultSerializeContext
{
public:
	typedef Vector<String, MemoryBudgets::Game> SuppressedErrors;

	SaveLoadContext(
		const ContentKey& contentKey,
		const DataStore& dataStore,
		const DataNode& table,
		const TypeInfo& typeInfo)
		: DefaultSerializeContext(contentKey, dataStore, table, typeInfo)
		, m_vsSuppressedErrors()
	{
	}

	virtual Bool HandleError(SerializeError eError, HString additionalData = HString()) SEOUL_OVERRIDE
	{
		using namespace Reflection;

		// Required and similar errors are always (silently) ignored, no properties
		// in PlayerData are considered required.
		if (SerializeError::kRequiredPropertyHasNoCorrespondingValue != eError &&
			SerializeError::kDataStoreContainsUndefinedProperty != eError)
		{
			// Use the default handling to issue a warning, record the warning
			// if it has occurred, but always return true. If we need to fallback
			// on a partially loaded save, we want to load as much of it as
			// possible.
			Bool const bSuccess = DefaultSerializeContext::HandleError(eError, additionalData);
			if (!bSuccess)
			{
				// Get the error message.
				String sMessage;
				DefaultSerializeErrorMessaging(
					*this,
					eError,
					additionalData,
					sMessage);
				sMessage = String::Printf("%s\n%s",
					ScopeToString().CStr(),
					sMessage.CStr());

				// Add it to our running set.
				m_vsSuppressedErrors.PushBack(sMessage);
			}
		}

		return true;
	}

	SuppressedErrors m_vsSuppressedErrors;
}; // class SaveLoadContext

} // namespace Reflection

/**
 * Local utility used in a few loading paths. Reads and decompresses
 * a DataStore.
 *
 * @param[in] rBuffer Data blob to read the compressed DataStore from.
 * @param[out] rDataStore Output DataStore. Left unmodified on failure.
 *
 * @return kSuccess on success, or various error codes on failure.
 */
static SaveLoadResult ReadDataStore(StreamBuffer& rBuffer, DataStore& rDataStore)
{
	// Read header data.
	UInt32 uUncompressedDataSizeInBytes = 0u;
	UInt32 uCompressedDataSizeInBytes = 0u;
	if (!rBuffer.Read(uUncompressedDataSizeInBytes) ||
		uUncompressedDataSizeInBytes > SaveLoadUtil::kuMaxDataSizeInBytes ||
		!rBuffer.Read(uCompressedDataSizeInBytes) ||
		uCompressedDataSizeInBytes > SaveLoadUtil::kuMaxDataSizeInBytes)
	{
		return SaveLoadResult::kErrorTooBig;
	}

	// Decompress the data.
	Vector<Byte, MemoryBudgets::Saving> vUncompressedData(uUncompressedDataSizeInBytes);
	if (!vUncompressedData.IsEmpty())
	{
		if (!ZlibDecompress(
			rBuffer.GetBuffer() + rBuffer.GetOffset(),
			uCompressedDataSizeInBytes,
			vUncompressedData.Data(),
			uUncompressedDataSizeInBytes))
		{
			return SaveLoadResult::kErrorCompression;
		}
		else
		{
			// Advance passed the data we just consumed.
			rBuffer.SeekToOffset(rBuffer.GetOffset() + uCompressedDataSizeInBytes);
		}
	}

	// If we get here successfully, data is now an array of the uncompressed
	// DataStore serialized data, so we need to deserialize it into a
	// DataStore object.
	DataStore dataStore;
	if (!vUncompressedData.IsEmpty())
	{
		FullyBufferedSyncFile file(vUncompressedData.Data(), vUncompressedData.GetSizeInBytes(), false);
		if (!dataStore.Load(file))
		{
			return SaveLoadResult::kErrorSaveData;
		}

		if (!dataStore.VerifyIntegrity())
		{
			return SaveLoadResult::kErrorSaveCheck;
		}
	}

	// Done success, swap in the output DataStore.
	rDataStore.Swap(dataStore);
	return SaveLoadResult::kSuccess;
}

/**
 * Local utility, reads a complex data structure from a byte stream,
 * expected to have been serialized as a DataStore.
 *
 * @param[in] rBuffer The data stream to read from.
 * @param[out] r The complex object instance to populate.
 * @param[in] eDeserializationFailedError Custom error value to return
 *                                        if deserialization into r
 *                                        fails.
 *
 * @return kSuccess or a failure code on error. r is left unmodified
 *         on failure.
 */
template <typename T>
static SaveLoadResult ReadComplex(StreamBuffer& rBuffer, T& r, SaveLoadResult eDeserializationFailedError)
{
	// Read a DataStore from the byte stream. Return immediately
	// on error.
	DataStore dataStore;
	auto eResult = ReadDataStore(rBuffer, dataStore);
	if (SaveLoadResult::kSuccess != eResult)
	{
		return eResult;
	}

	// Deserialize into a local instance of T.
	T inst;
	Reflection::SaveLoadContext context(ContentKey(), dataStore, dataStore.GetRootNode(), TypeId<T>());
	if (!Reflection::DeserializeObject(context, dataStore, dataStore.GetRootNode(), &inst))
	{
		return eDeserializationFailedError;
	}

	// Done, success. Assign through the value and return.
	r = inst;
	return SaveLoadResult::kSuccess;
}

/**
 * Local utility used in a few saving paths. Serializes,
 * compresses, and commits a DataStore to an output stream.
 *
 * @param[in] rBuffer Data blob to write the compressed DataStore to.
 * @param[out] dataStore The DataStore to serialize.
 *
 * @return kSuccess on success, or various error codes on failure.
 */
static SaveLoadResult WriteDataStore(StreamBuffer& rBuffer, const DataStore& dataStore)
{
	// Verify DataStore integrity prior to save.
	if (!dataStore.VerifyIntegrity())
	{
		return SaveLoadResult::kErrorSaveCheck;
	}

	// Write the DataStore.
	MemorySyncFile file;
	if (!dataStore.Save(file, keCurrentPlatform))
	{
		return SaveLoadResult::kErrorSaveData;
	}

	// Compress the DataStore.
	void* pCompressedData = nullptr;
	UInt32 uCompressedDataSizeInBytes = 0u;
	if (!ZlibCompress(
		file.GetBuffer().GetBuffer(),
		file.GetBuffer().GetTotalDataSizeInBytes(),
		pCompressedData,
		uCompressedDataSizeInBytes,
		ZlibCompressionLevel::kDefault,
		MemoryBudgets::Saving))
	{
		return SaveLoadResult::kErrorCompression;
	}

	// Cache the uncompressed size to write as header data.
	UInt32 const uUncompressedDataSizeInBytes = file.GetBuffer().GetTotalDataSizeInBytes();

	// Write the uncompressed size, then compressed size, and then
	// finally the data itself.
	rBuffer.Write(uUncompressedDataSizeInBytes);
	rBuffer.Write(uCompressedDataSizeInBytes);
	rBuffer.Write(pCompressedData, uCompressedDataSizeInBytes);

	// Release the compressed data.
	MemoryManager::Deallocate(pCompressedData);

	// Done, success.
	return SaveLoadResult::kSuccess;
}

/**
 * Local utility, writes a complex data structure ti a byte stream,
 * serialized as a DataStore.
 *
 * @param[in] rBuffer The data stream to write to.
 * @param[out] v The complex object instance to serialize.
 * @param[in] eSerializationFailedError Custom error value to return
 *                                      if serialization of v fails.
 *
 * @return kSuccess or a failure code on error.
 */
template <typename T>
static SaveLoadResult WriteComplex(StreamBuffer& rBuffer, const T& v, SaveLoadResult eSerializationFailedError)
{
	DataStore dataStore;
	if (!Reflection::SerializeToDataStore(&v, dataStore))
	{
		return eSerializationFailedError;
	}

	return WriteDataStore(rBuffer, dataStore);
}

/** Utility, gets or creates a SaveFileState entry as needed into rtState. */
static inline SaveFileState& ResolveState(
	SaveLoadManager::StateTable& rtState,
	FilePath filePath)
{
	// Find an existing state instance.
	auto p = rtState.Find(filePath);

	// Not defined, create one.
	if (nullptr == p)
	{
		SaveFileState state;
		auto const e = rtState.Insert(filePath, state);
		SEOUL_ASSERT(e.Second);
		p = &e.First->Second;
	}

	// Must never be nullptr by this point.
	SEOUL_ASSERT(nullptr != p);
	return *p;
}

// Anonymous namespace of internal utilities used for cloud loading and saving.
namespace
{

/**
 * Manages callback of a cloud load and save request. Blocks
 * on calls to WaitForCompletion().
 */
class CloudRequestMonitor SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(CloudRequestMonitor);

	/**
	 * These are the various response codes we can get on a load or save request,
	 * and their various meanings.
	 */
	enum
	{
		/**
		 * For loads, this means the server accepted the client's data and everything is ok.
		 * For saves, this means the server accepted the client's data and everything is ok.
		 */
		kSuccess = 200,

		/**
		 * Only valid for loads - indicates that the client sent no data to the server, expecting
		 * a load from the server, but the server had no data to send. Equivalent to a local
		 * "file not found".
		 */
		kServerHasNoData = 250,

		/**
		 * Only valid for loads, or saves when the testing flag was specified - indicates that
		 * the server has sent data that it expects the client to use in place of any local data.
		 * The server body may be empty, which indicates a full save game reset.
		 */
		kServerHasSentData = 251,

		/**
		 * Valid for saves and loads - this indicates that the server needs a full local save checkpoint.
		 * The client is expected to send another save or load event with a save data delta, with min
		 * transaction id set to 0, and max transaction id set to an appropriate value (typically,
		 * whatever the last max transaction id was + 1).
		 */
		kServerNeedsFullCheckpoint = 252,

		/**
		 * Valid for saves only - identical to HttpStatusSuccess, except that metadata is included.
		 * The included metadata is expected to match the metadata sent
		 * by the client with the save request.
		 */
		kSuccessWithMetadata = 253,
	};

	/**
	 * @return Utility to check return codes. These are codes that
	 * we expect the server to return. Anything else is considered
	 * unusual and, specifically for saving, is used to identify
	 * server desync conditions.
	 */
	static inline Bool IsExpectedStatus(Int32 iStatus)
	{
		return (kSuccess == iStatus || (iStatus >= kServerHasNoData && iStatus <= kSuccessWithMetadata));
	}

	CloudRequestMonitor()
		: m_Signal()
		, m_Data()
		, m_Metadata()
		, m_eResult(HTTP::Result::kFailure)
		, m_iStatus(-1)
		, m_bDone(false)
	{
	}

	/** @return The returned data. Only valid when IsDone() and HasData() are true. */
	DataStore& GetData() { return m_Data; }

	/** @return The returned metadata. Only valid when IsDone() and HasData() are true. */
	const SaveLoadUtil::SaveFileMetadata& GetMetadata() const { return m_Metadata; }

	/** @return The result of the network request. */
	HTTP::Result GetResult() const { return m_eResult; }

	/** @return The result status code of the request. Only valid when IsDone() and WasSuccessful() are true. */
	Int32 GetStatus() const { return m_iStatus; }

	/** @return True when the operation has completed. */
	Bool IsDone() const { return m_bDone; }

	/**
	 * Bind this method into an HTTP request to handle cloud load
	 * requests.
	 */
	HTTP::CallbackResult Callback(HTTP::Result eResult, HTTP::Response *pResponse)
	{
		// Sanity checking, the HTTP system should never do this.
		SEOUL_ASSERT(!m_bDone);

		// Track the result.
		m_eResult = eResult;

		// Track the status.
		m_iStatus = pResponse->GetStatus();

		// Successful request and code 200 means we potentially have
		// data and a successful result.
		if (HTTP::Result::kSuccess == m_eResult && IsExpectedStatus(m_iStatus))
		{
			// Different processing dependent on code.
			switch (m_iStatus)
			{
			case kSuccess: // This case means "everything's ok, nothing more to do."
			case kServerHasNoData: // Roughly equivalent, except it means that the client expected data, and the server has none.
			case kServerNeedsFullCheckpoint: // Server is explicitly requesting a resend from the client, with a full checkpoint.
			default:
				break;

				// Need to deserialize the metadata in the body.
			case kSuccessWithMetadata:
				// TODO: Eliminate Seoul::String construction here.
				if (!Reflection::DeserializeFromString(
					String((Byte const*)pResponse->GetBody(), pResponse->GetBodySize()),
					&m_Metadata))
				{
					m_Metadata = SaveLoadUtil::SaveFileMetadata();
					DataStore emptyDataStore;
					m_Data.Swap(emptyDataStore);

					// If this case happens, we treat it as a network failure.
					m_eResult = HTTP::Result::kFailure;
				}
				break;

				// Same as success, except there is a potential data body to parse. May be empty, intentionally, to indicate
				// a reset.
			case kServerHasSentData:
				// Done immediately if no data.
				if (0u != pResponse->GetBodySize())
				{
					// TODO: Eliminate Seoul::String construction here.
					auto const e = SaveLoadUtil::FromBase64(
						String((Byte const*)pResponse->GetBody(), pResponse->GetBodySize()),
						m_Metadata,
						m_Data);
					if (SaveLoadResult::kSuccess != e)
					{
						m_Metadata = SaveLoadUtil::SaveFileMetadata();
						DataStore emptyDataStore;
						m_Data.Swap(emptyDataStore);

						// If this case happens, we treat it as a network failure.
						m_eResult = HTTP::Result::kFailure;
					}
				}
				break;
			}
		}

		// Memory barrier, so that m_bDone is done after all other
		// variable config above, and prior to signaling.
		SeoulMemoryBarrier();

		// Done with the operation.
		m_bDone = true;

		// Memory barrier, as this set must occur prior to
		// signaling.
		SeoulMemoryBarrier();

		m_Signal.Activate();

		return HTTP::CallbackResult::kSuccess;
	}

	/**
	 * Block the caller until the operation completes. This is an absolute
	 * blocking wait, so only call from non-Jobs::Manager worker threads
	 * (e.g. the SaveLoadManager worker thread).
	 */
	void WaitForCompletion()
	{
		while (!m_bDone)
		{
			m_Signal.Wait();
		}
	}

private:
	SEOUL_DISABLE_COPY(CloudRequestMonitor);

	Signal m_Signal;
	DataStore m_Data;
	SaveLoadUtil::SaveFileMetadata m_Metadata;
	Atomic32Value<HTTP::Result> m_eResult;
	Atomic32Value<Int32> m_iStatus;
	Atomic32Value<Bool> m_bDone;
}; // struct CloudRequestMonitor

} // namespace anonymous

// Save/load low-level.
SaveLoadResult SaveLoadManager::LoadLocalData(
	StreamBuffer& data,
	DataStore& rSaveData,
	DataStore& rPendingDelta,
	SaveLoadUtil::SaveFileMetadata& rMetadata)
{
	// Signature check.
	UInt32 uSignature = 0u;
	if (!data.Read(uSignature)) { return SaveLoadResult::kErrorSignatureData; }
	if (uSignature != kuSaveContainerSignature) { return SaveLoadResult::kErrorSignatureCheck; }

	// If signature check was succesful, version check.
	Int32 iVersion = -1;
	if (!data.Read(iVersion)) { return SaveLoadResult::kErrorVersionData; }
	if (!(iVersion >= kiMinSaveContainerVersion && iVersion <= kiMaxSaveContainerVersion)) { return SaveLoadResult::kErrorVersionCheck; }

	// If version check was successful, decrypt the data.
	UInt32 uChecksumOffset = 0u;
	UInt8 auNonce[EncryptAES::kEncryptionNonceLength];
	if (!data.Read(auNonce, sizeof(auNonce)))
	{
		return SaveLoadResult::kErrorEncryption;
	}
	else
	{
		uChecksumOffset = data.GetOffset();
		EncryptAES::DecryptInPlace(
			data.GetBuffer() + uChecksumOffset,
			data.GetTotalDataSizeInBytes() - uChecksumOffset,
			kauKey,
			sizeof(kauKey),
			auNonce);
	}

	// Read and verify the checksum.
	UInt8 auChecksum[EncryptAES::kSHA512DigestLength];
	if (!data.Read(auChecksum, sizeof(auChecksum)))
	{
		return SaveLoadResult::kErrorChecksumData;
	}
	else
	{
		// Verify the checksum -- since the checksum was originally computed
		// with the checksum bytes set to 0, we need to set them back to 0 to
		// verify.
		memset(data.GetBuffer() + uChecksumOffset, 0, sizeof(auChecksum));
		UInt8 auComputedChecksum[EncryptAES::kSHA512DigestLength];
		EncryptAES::SHA512Digest(
			data.GetBuffer(),
			data.GetTotalDataSizeInBytes(),
			auComputedChecksum);

		if (0 != memcmp(auChecksum, auComputedChecksum, sizeof(auChecksum)))
		{
			return SaveLoadResult::kErrorChecksumCheck;
		}
	}

	// Deserialize metadata.
	SaveLoadUtil::SaveFileMetadata metadata;
	{
		auto const eResult = ReadComplex(data, metadata, SaveLoadResult::kErrorSerialization);
		if (SaveLoadResult::kSuccess != eResult)
		{
			return eResult;
		}
	}

	// Read the checkpoint DataStore.
	DataStore saveData;
	{
		auto const eResult = ReadDataStore(data, saveData);
		if (SaveLoadResult::kSuccess != eResult)
		{
			return eResult;
		}
	}

	// Read the pending delta.
	DataStore pendingDelta;
	{
		auto const eResult = ReadDataStore(data, pendingDelta);
		if (SaveLoadResult::kSuccess != eResult)
		{
			return eResult;
		}
	}

	// Verify that we completely consumed the input data.
	if (data.GetOffset() != data.GetTotalDataSizeInBytes())
	{
		return SaveLoadResult::kErrorExtraData;
	}

	// Populate and return success.
	rSaveData.Swap(saveData);
	rPendingDelta.Swap(pendingDelta);
	rMetadata = metadata;
	return SaveLoadResult::kSuccess;
}

SaveLoadResult SaveLoadManager::SaveLocalData(
	StreamBuffer& data,
	const DataStore& saveData,
	const DataStore& pendingDelta,
	const SaveLoadUtil::SaveFileMetadata& metadata)
{
	// Write the signature.
	data.Write((UInt32)kuSaveContainerSignature);

	// Write the version.
	data.Write((Int32)kiMaxSaveContainerVersion);

	// Generate the nonce for encryption.
	UInt8 auNonce[EncryptAES::kEncryptionNonceLength];
	EncryptAES::InitializeNonceForEncrypt(auNonce);

	// Prepare the checksum structure.
	UInt8 auChecksum[EncryptAES::kSHA512DigestLength];
	memset(auChecksum, 0, sizeof(auChecksum));

	// Write the nonce, then record the checksum spot prior
	// to writing the (0 byte) checksum.
	data.Write(auNonce, sizeof(auNonce));
	UInt32 const uChecksumOffset = data.GetOffset();
	data.Write(auChecksum, sizeof(auChecksum));

	// Write metadata.
	{
		auto const eResult = WriteComplex(data, metadata, SaveLoadResult::kErrorSerialization);
		if (SaveLoadResult::kSuccess != eResult)
		{
			return eResult;
		}
	}

	// Write save data checkpoint.
	{
		auto const eResult = WriteDataStore(data, saveData);
		if (SaveLoadResult::kSuccess != eResult)
		{
			return eResult;
		}
	}

	// Write pending delta - note that this is the next delta that
	// we need to apply to the server's checkpoint to bring it in sync
	// with our local checkpoint. This is *not* a delta to apply to the
	// local checkpoint saved above.
	{
		auto const eResult = WriteDataStore(data, pendingDelta);
		if (SaveLoadResult::kSuccess != eResult)
		{
			return eResult;
		}
	}

	// Compute the checksum.
	EncryptAES::SHA512Digest(data.GetBuffer(), data.GetTotalDataSizeInBytes(), auChecksum);

	// Write the checksum back in over the placeholder
	data.SeekToOffset(uChecksumOffset);
	data.Write(auChecksum, sizeof(auChecksum));
	data.SeekToOffset(data.GetTotalDataSizeInBytes());

	// Encrypt the data in-place, starting from the checksum
	EncryptAES::EncryptInPlace(
		data.GetBuffer() + uChecksumOffset,
		data.GetTotalDataSizeInBytes() - uChecksumOffset,
		kauKey,
		sizeof(kauKey),
		auNonce);

	return SaveLoadResult::kSuccess;
}

SaveLoadManager::SaveLoadManager(const SaveLoadManagerSettings& settings)
	: m_Settings(settings)
	, m_sSessionGuid(UUID::GenerateV4().ToString())
	, m_pSaveApi(Engine::Get()->CreateSaveApi())
	, m_WorkQueue()
	, m_pWorkerThread()
	, m_pSignal(SEOUL_NEW(MemoryBudgets::Saving) Signal)
	, m_bRunning(true)
{
	// Startup and kick off the worker thread.
	m_pWorkerThread.Reset(SEOUL_NEW(MemoryBudgets::Saving) Thread(
		SEOUL_BIND_DELEGATE(&SaveLoadManager::WorkerThreadMain, this),
		false));
	m_pWorkerThread->Start("SaveLoadManagerWorkerThread");
	m_pWorkerThread->SetPriority(Thread::kLow);
}

SaveLoadManager::~SaveLoadManager()
{
	if (m_bRunning)
	{
		// Mark that we're done running.
		m_bRunning = false;

		// Wake up the thread.
		m_pSignal->Activate();

		// Wait for the thread to shutdown.
		(void)m_pWorkerThread->WaitUntilThreadIsNotRunning();

		// Now cleanup any unfinished entries.
		while (Entry* pEntry = m_WorkQueue.Pop())
		{
			SafeDelete(pEntry);
		}
	}
}

/**
 * Enqueue a load operation.
 *
 * @param[in] objectType Type to instantiate after successful load.
 * @param[in] savePath Path to file on disk to load from.
 * @param[in] sCloudURL When non-empty, URL endpoint to hit for loading the save from remote cloud storage.
 * @param[in] iExpectedVersion Data version, migrations will be applied to old data.
 * @param[in] pCallback Delivery of result.
 * @param[in] tMigrations Table of migrations that can be applied on version mismatch. Version to migration function.
 * @param[in] bResetSession Prior to the load operation, generate a new session guid.
 */
void SaveLoadManager::QueueLoad(
	const Reflection::Type& objectType,
	FilePath savePath,
	const String& sCloudURL,
	Int32 iExpectedVersion,
	const SharedPtr<ISaveLoadOnComplete>& pCallback,
	const Migrations& tMigrations,
	Bool bResetSession)
{
	// Setup the load entry.
	Entry* pEntry = SEOUL_NEW(MemoryBudgets::Saving) Entry;
	pEntry->m_pCallback = pCallback;
	pEntry->m_eEntryType = (bResetSession ? kLoadSessionChange : kLoadNoSessionChange);
	pEntry->m_pLoadDataType = &objectType;
	pEntry->m_tMigrations = tMigrations;
	pEntry->m_Path = savePath;
	pEntry->m_iVersion = iExpectedVersion;
	pEntry->m_sCloudURL = sCloudURL;

	// Enqueue.
	m_WorkQueue.Push(pEntry);
	m_pSignal->Activate();
}

/**
 * Enqueue a save operation.
 *
 * The data object is immediately serialized and is effectively "cloned" before
 * this function returns.
 *
 * @param[in] savePath Path to attempt to save to.
 * @param[in] sCloudURL When non-empty, URL endpoint to hit for saving data to remote cloud storage.
 * @param[in] pObject The object to save.
 * @param[in] iDataVersion Version tag to associate with the save.
 * @param[in] pCallback Delivery of save results.
 * @param[in] bForceImmediateCloudSave When true, this save operation will be a cloud save,
 *            independent of time spent in between other save operations.
 */
void SaveLoadManager::QueueSave(
	FilePath savePath,
	const String& sCloudURL,
	const Reflection::WeakAny& pObject,
	Int32 iDataVersion,
	const SharedPtr<ISaveLoadOnComplete>& pCallback,
	Bool bForceImmediateCloudSave)
{
	// Setup most of the save entry.
	Entry* pEntry = SEOUL_NEW(MemoryBudgets::Saving) Entry;
	pEntry->m_pCallback = pCallback;
	pEntry->m_eEntryType = kSave;
	pEntry->m_pLoadDataType = nullptr;
	pEntry->m_tMigrations.Clear();
	pEntry->m_Path = savePath;
	pEntry->m_iVersion = iDataVersion;
	pEntry->m_sCloudURL = sCloudURL;
	pEntry->m_bForceImmediateCloudSave = bForceImmediateCloudSave;

	// Immediately serialize the data - we need
	// to clone the data so the object does not
	// change out from under us prior to save.
	pEntry->m_pSaveData.Reset(SEOUL_NEW(MemoryBudgets::Saving) DataStore);
	if (!Reflection::SerializeToDataStore(pObject, *pEntry->m_pSaveData))
	{
		SafeDelete(pEntry);
		DispatchSaveCallback(
			pCallback,
			SaveLoadResult::kErrorSerialization,
			SaveLoadResult::kErrorSerialization,
			SaveLoadResult::kErrorSerialization);
		return;
	}

	// Enqueue.
	m_WorkQueue.Push(pEntry);
	m_pSignal->Activate();
}

#if SEOUL_ENABLE_CHEATS
/**
 * Developer only functionality. Reset a save to its default state.
 */
void SaveLoadManager::QueueSaveReset(FilePath savePath, const String& sCloudURL, Bool bResetSession)
{
	// Setup the entry.
	Entry* pEntry = SEOUL_NEW(MemoryBudgets::Saving) Entry;
	pEntry->m_eEntryType = (bResetSession ? kSaveResetSessionChange : kSaveResetNoSessionChange);
	pEntry->m_Path = savePath;
	pEntry->m_sCloudURL = sCloudURL;

	// Enqueue.
	m_WorkQueue.Push(pEntry);
	m_pSignal->Activate();
}
#endif // /#if SEOUL_ENABLE_CHEATS

#if SEOUL_UNIT_TESTS
Bool SaveLoadManager::IsFirstTimeTestingComplete() const
{
	if (!m_Settings.m_bEnableFirstTimeTests)
	{
		return true;
	}

	return m_bFirstTimeLoadTestsComplete && m_bFirstTimeSaveTestsComplete;
}
#endif // /#if SEOUL_UNIT_TESTS

/**
 * Utility for the worker thread, runs a migration
 * sequence on a JObject hierarchy, as needed.
 *
 * @param[in] tMigrations Available migration functions.
 * @param[in] iTargetVersion Version we're trying to migrate to.
 * @param[in] rDataStore The data blob to migrate.
 * @param[in] rootDataNode The root of the user's data blob. Apply migrations starting at this node.
 * @param[in] iCurrentVersion The current tagged version of the input data.
 *
 * @return The result of the migration operation, success, or various forms of failure.
 */
SaveLoadResult SaveLoadManager::WorkerThreadApplyMigrations(
	const Migrations& tMigrations,
	Int32 iTargetVersion,
	DataStore& rDataStore,
	const DataNode& rootDataNode,
	Int32 iCurrentVersion)
{
	// Early out, no migrations.
	if (tMigrations.IsEmpty())
	{
		return (iTargetVersion == iCurrentVersion)
			? SaveLoadResult::kSuccess
			: SaveLoadResult::kErrorNoMigrations;
	}

	if (iTargetVersion < iCurrentVersion)
	{
		return SaveLoadResult::kErrorFutureMigrationVersion;
	}

	// Track migrations we've applied so we don't end up
	// in a cyclical migration loop.
	HashSet<Int32, MemoryBudgets::Saving> appliedMigrations;
	while (iTargetVersion != iCurrentVersion)
	{
		// Get the migration for the current version - if this fails, the migration
		// operation fails.
		MigrationCallback callback;
		if (!tMigrations.GetValue(iCurrentVersion, callback))
		{
#if SEOUL_LOGGING_ENABLED
			auto const iBegin = tMigrations.Begin();
			auto const iEnd = tMigrations.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				SEOUL_LOG("WorkerThreadApplyMigrations is looking for current version '%d', has '%d'\n", iCurrentVersion, i->First);
			}
#endif // /#if SEOUL_LOGGING_ENABLED

			return SaveLoadResult::kErrorNoMigrations;
		}

		// Applying migration for the current version - if
		// this insert fails, it means we're reapplying an already
		// applied migration and the migration chain has a cyclica reference.
		if (!appliedMigrations.Insert(iCurrentVersion).Second)
		{
			return SaveLoadResult::kErrorCyclicalMigrations;
		}

		// Run the migration - if this fails, migration immediately fails.
		Int32 iNewVersion = 0;
		if (!callback(rDataStore, rootDataNode, iNewVersion))
		{
			return SaveLoadResult::kErrorMigrationCallback;
		}

		// current version is now new version.
		iCurrentVersion = iNewVersion;
	}

	return SaveLoadResult::kSuccess;
}

/**
* Invokes a load callback with checking and safety.
*
* @param[in] pCallback Callback to invoke, can be invalid.
* @param[in] eLocalResult Result of local save/load to pass to the callback.
* @param[in] eCloudReuslt Result of remote (cloud) save/load to pass to the callback.
* @param[in] eFinalResult Result of the overall save/load operation. Can
*                         be a failure, even if both eLocalResult and
*                         eCloudResult are success.
* @param[in] pObject Data to send with the callback.
*/
void SaveLoadManager::DispatchLoadCallback(
	const SharedPtr<ISaveLoadOnComplete>& pCallback,
	SaveLoadResult eLocalResult,
	SaveLoadResult eCloudResult,
	SaveLoadResult eFinalResult,
	const Reflection::WeakAny& pObject)
{
	if (pCallback.IsValid())
	{
		if (pCallback->DispatchOnMainThread() && !IsMainThread())
		{
			Jobs::AsyncFunction(
				GetMainThreadId(),
				&DispatchLoadCallback,
				pCallback,
				eLocalResult,
				eCloudResult,
				eFinalResult,
				pObject);
		}
		else
		{
			pCallback->OnLoadComplete(eLocalResult, eCloudResult, eFinalResult, pObject);
		}
	}
}

/**
 * Invokes a save callback with checking and safety.
 *
 * @param[in] pCallback Callback to invoke, can be invalid.
 * @param[in] eLocalResult Result of local save/load to pass to the callback.
 * @param[in] eCloudReuslt Result of remote (cloud) save/load to pass to the callback.
 * @param[in] eFinalResult Result of the overall save/load operation. Can
 *                         be a failure, even if both eLocalResult and
 *                         eCloudResult are success.
 */
void SaveLoadManager::DispatchSaveCallback(
	const SharedPtr<ISaveLoadOnComplete>& pCallback,
	SaveLoadResult eLocalResult,
	SaveLoadResult eCloudResult,
	SaveLoadResult eFinalResult)
{
	if (pCallback.IsValid())
	{
		if (pCallback->DispatchOnMainThread() && !IsMainThread())
		{
			Jobs::AsyncFunction(
				GetMainThreadId(),
				&DispatchSaveCallback,
				pCallback,
				eLocalResult,
				eCloudResult,
				eFinalResult);
		}
		else
		{
			pCallback->OnSaveComplete(eLocalResult, eCloudResult, eFinalResult);
		}
	}
}

/**
 * Worker-thread only entry point, performs the full load operation, including
 * deserialization and invocation of callback, and managed load from cloud
 * storage, if enabled for this save object.
 *
 * @param[inout] rState SaveLoadManager tracked state for this save object.
 * @param[in] entry Entry describing the load operation to perform.
 */
void SaveLoadManager::WorkerThreadLoad(SaveFileState& rState, const Entry& entry) const
{
	// From the local save, we load the current save checkpoint, as
	// well as a (possibly empty) pending delta. The delta will be
	// sent to the server as part of a cloud load request, to bring
	// the server up to date with our local checkpoint. This is our
	// last opportunity to do so (locally, we no longer have the
	// server's version of the checkpoint, so we can generate no
	// more valid delta's to send to the server until it is brought
	// in sync with our local checkpoint by applying this last delta).
	DataStore saveData;
	DataStore deltaData;
	SaveLoadUtil::SaveFileMetadata metadata;

	// Parse the data out into metadata and save game object
	auto eLocalResult = WorkerThreadLoadLocalData(entry.m_Path, saveData, deltaData, metadata);

	// If local loading has failed, clear all structures, and rely on the cloud load.
	if (SaveLoadResult::kSuccess != eLocalResult)
	{
		// Clear data and delta.
		{
			DataStore emptyDataStore;
			saveData.Swap(emptyDataStore);
		}
		{
			DataStore emptyDataStore;
			deltaData.Swap(emptyDataStore);
		}

		// Blank out the metadata.
		metadata = SaveLoadUtil::SaveFileMetadata();
	}

	// Track cloud load result, default is "not enabled".
	SaveLoadResult eCloudResult = SaveLoadResult::kCloudDisabled;
	if (!entry.m_sCloudURL.IsEmpty())
	{
		HTTP::ResendTimer timer = HTTP::Manager::Get()->CopyHttpResendTimer();
		timer.ResetResendSeconds();

		// There are only a limited set of codes that we allow from cloud loading, everything
		// else is a retry.
		while (
			SaveLoadResult::kSuccess != eCloudResult &&
			SaveLoadResult::kCloudNoUpdate != eCloudResult &&
			SaveLoadResult::kCloudCancelled != eCloudResult &&
			SaveLoadResult::kErrorFileNotFound != eCloudResult)
		{
			// Sleep as needed on retries.
			Double fResendIntervalInSeconds = timer.NextResendSeconds();
			if (fResendIntervalInSeconds > 0.0)
			{
				Thread::Sleep((UInt32)Round(fResendIntervalInSeconds * 1000.0));
			}

			// Prior to each cloud load submission, we increment the max
			// transaction id.
			metadata.m_iTransactionIdMax++;
			eCloudResult = WorkerThreadLoadCloudData(
				entry.m_sCloudURL,
				metadata,
				saveData,
				metadata,
				deltaData,
				metadata,
				saveData);

			// After the first attempt, if requested, submit a full save checkpoint.
			if (SaveLoadResult::kCloudNeedsFullCheckpoint == eCloudResult)
			{
				// If we're in testing mode, this case should never occur.
#if SEOUL_UNIT_TESTS
				if (m_Settings.m_bEnableValidation)
				{
					SEOUL_WARN("Unexpected kCloudLoadWantsFullSaveCheckpoint from WorkerThreadLoadCloudData.");
				}
#endif // /#if SEOUL_UNIT_TESTS

				// For the resubmit, we use the metadata of the local save,
				// with the min transaction ID set to 0, to indicate
				// a reset.
				metadata.m_iTransactionIdMax++;
				auto updatedMetadata = metadata;
				updatedMetadata.m_iTransactionIdMin = 0;
				eCloudResult = WorkerThreadLoadCloudData(
					entry.m_sCloudURL,
					metadata,
					saveData,
					updatedMetadata,
					saveData,
					metadata,
					saveData);
			}
		}
	}

	// If we're in testing mode, then eCloudResult must be either
	// kErrorFileNotFound (the first time), or kCloud
#if SEOUL_UNIT_TESTS
	if (m_Settings.m_bEnableValidation || m_Settings.m_bEnableFirstTimeTests)
	{
		if (m_Settings.m_bEnableFirstTimeTests && 0 == rState.m_iUnitTestLoadCount)
		{
			if (SaveLoadResult::kErrorFileNotFound != eLocalResult)
			{
				SEOUL_WARN("Unexpected '%s' from WorkerThreadLoadLocalData, expected kErrorFileNotFound.", EnumToString<SaveLoadResult>(eLocalResult));
			}

			if ((entry.m_sCloudURL.IsEmpty() && SaveLoadResult::kCloudDisabled != eCloudResult) ||
				(!entry.m_sCloudURL.IsEmpty() && SaveLoadResult::kErrorFileNotFound != eCloudResult))
			{
				SEOUL_WARN("Unexpected '%s' from WorkerThreadLoadCloudData, expected kErrorFileNotFound.", EnumToString<SaveLoadResult>(eCloudResult));
			}
		}
		else if (m_Settings.m_bEnableValidation && rState.m_iUnitTestLoadCount > 0)
		{
			if (SaveLoadResult::kSuccess != eLocalResult)
			{
				SEOUL_WARN("Unexpected '%s' from WorkerThreadLoadLocalData, expected kSuccess.", EnumToString<SaveLoadResult>(eLocalResult));
			}

			if (SaveLoadResult::kCloudNoUpdate != eCloudResult)
			{
				SEOUL_WARN("Unexpected '%s' from WorkerThreadLoadCloudData, expected kCloudLoadNoUpdate.", EnumToString<SaveLoadResult>(eCloudResult));
			}
		}

		++rState.m_iUnitTestLoadCount;
	}
#endif // /#if SEOUL_UNIT_TESTS

	// If either operation was success, check and if necessary, apply migrations
	// to the data.
	SaveLoadResult eCommonResult = Min(eLocalResult, eCloudResult);

	// Special case handling, if kSuccess == eCommonResult but saveData is
	// empty, this means the server is triggering a reset (either explicitly
	// on QA, or in live when the client's data is invalid and the server
	// has no data). We switch to th kErrorFileNotFound code in this case.
	if (SaveLoadResult::kSuccess == eCommonResult && saveData.GetRootNode().IsNull())
	{
		eCommonResult = SaveLoadResult::kErrorFileNotFound;
	}

	Bool bWasMigrated = false;

	if (SaveLoadResult::kSuccess == eCommonResult)
	{
		// Version mismatch, attempt to migrate the data.
		if (metadata.m_iVersion != entry.m_iVersion)
		{
			// Get the root data object.
			DataNode const dataRootNode = saveData.GetRootNode();

			// Run the migrations. If this succeeds,
			// update the version in the metadata. Otherwise,
			// return failure.
			eCommonResult = WorkerThreadApplyMigrations(
				entry.m_tMigrations,
				entry.m_iVersion,
				saveData,
				dataRootNode,
				metadata.m_iVersion);
			if (SaveLoadResult::kSuccess == eCommonResult)
			{
				metadata.m_iVersion = entry.m_iVersion;
				bWasMigrated = true;
			}
		}
	}

	// Try to generate the loaded object on success.
	Reflection::WeakAny pLoadedObject;
	if (SaveLoadResult::kSuccess == eCommonResult)
	{
		eCommonResult = WorkerThreadCreateObject(entry, saveData, pLoadedObject);
	}

	// Default behavior is for the checkpoint to be reset.
	rState.m_pCheckpoint.Reset();

	// Track the server checkpoint on and overall success operation,
	// where the cloud result is not "file not found". In this latter
	// case, the server is explicitly resetting our checkpointing.
	if (SaveLoadResult::kSuccess == eCommonResult && SaveLoadResult::kErrorFileNotFound != eCloudResult)
	{
		rState.m_pCheckpoint.Reset(SEOUL_NEW(MemoryBudgets::Saving) DataStore);
		rState.m_pCheckpoint->CopyFrom(saveData);
	}

	// Synchronize minimum transaction id when we've successfully sent an update
	// and the server did not need to modify it further.
	if (SaveLoadResult::kCloudNoUpdate == eCloudResult)
	{
		metadata.m_iTransactionIdMin = metadata.m_iTransactionIdMax;
	}

	// If there was a migration, we have to send the whole save and not just a delta
	if (bWasMigrated)
	{
		metadata.m_iTransactionIdMin = 0;
		rState.m_pCheckpoint.Reset();
	}

	// Merge in metadata, then update the session guid.
	if (SaveLoadResult::kCloudCancelled != eCommonResult)
	{
		rState.m_Metadata = metadata;
		rState.m_Metadata.m_sSessionGuid = m_sSessionGuid;
	}

	// Last state - perform a local save, if cloud loading was enabled. We do this to
	// capture a blanked out pending data with an updated session guid.
	if (!entry.m_sCloudURL.IsEmpty() && SaveLoadResult::kCloudCancelled != eCloudResult)
	{
		DataStore emptyDataStore;
		auto const eResult = WorkerThreadSaveLocalData(
			entry.m_Path,
			saveData,
			emptyDataStore,
			rState.m_Metadata);
		(void)eResult;

#if SEOUL_UNIT_TESTS
		if (m_Settings.m_bEnableValidation && SaveLoadResult::kSuccess != eResult)
		{
			SEOUL_WARN("Unexpected '%s' from post load WorkerThreadSaveLocalData, expected kSuccess.", EnumToString<SaveLoadResult>(eResult));
		}
#endif // /#if SEOUL_UNIT_TESTS
	}

	// Dispatch callback, if we have one.
	DispatchLoadCallback(entry.m_pCallback, eLocalResult, eCloudResult, eCommonResult, pLoadedObject);
}

/**
 * Worker-thread only entry point, performs a load operation from cloud storage,
 * if enabled.
 *
 * @param[in] sURL The remote URL endpoint to request data from.
 * @param[in] targetMetadata The target metadata state of server's checkpoint after applying pendingDelta.
 * @param[in] targetSaveData The target save data state of the server's checkpoint after applying pendingDelta.
 * @param[in] pendingDeltaMetadata Metadata to be included with the pending delta when sent to the server.
 * @param[in] pendingDelta A pending delta transaction to submit
 *                         to the server, loaded from the local save.
 *                         Cloud loads require step to be brought fully
 *                         up-to-date with local, so we submit any
 *                         remaining transactions with load requests.
 * @param[out] rOutMetadata The read cloud metadata on success.
 * @param[out] rOutSaveData The read cloud data on success.
 * @param[in] bTestOnlyNoEmail In testing builds only, when true, tells the server not to send out
 *                             error emails on error conditions, used when a test is intentionally
 *                             triggering an error.
 * @param[in] bTestOnlyNoVerify In testing builds only, when true, uses the non-verifying version
 *                              of ToBase64 to intentionally send corrupted data to the server.
 *
 * @return Success or failure code for the load operation. Failure
 *         results leave rData unmodified.
 */
SaveLoadResult SaveLoadManager::WorkerThreadLoadCloudData(
	const String& sURL,
	const SaveLoadUtil::SaveFileMetadata& targetMetadata,
	const DataStore& targetSaveData,
	const SaveLoadUtil::SaveFileMetadata& pendingDeltaMetadata,
	const DataStore& pendingDelta,
	SaveLoadUtil::SaveFileMetadata& rOutMetadata,
	DataStore& rOutSaveData,
	Bool bTestOnlyNoEmail /* = false */,
	Bool bTestOnlyNoVerify /*= false*/) const
{
	// Prepare delta DataStore for cloud submission.
	String sCompressedData;
	SaveLoadResult eResult = SaveLoadResult::kErrorUnknown;

	// TODO: Temporary log to try to track down a periodic cloud save problem.
	SEOUL_LOG("Cloud load: %s (%d, %d, %s, %s)\n", sURL.CStr(), (Int32)pendingDeltaMetadata.m_iTransactionIdMin, (Int32)pendingDeltaMetadata.m_iTransactionIdMax, (bTestOnlyNoEmail ? "true" : "false"), (bTestOnlyNoVerify ? "true" : "false"));

#if SEOUL_UNIT_TESTS
	if (bTestOnlyNoVerify)
	{
		eResult = SaveLoadUtil::UnitTestHook_ToBase64NoVerify(pendingDeltaMetadata, pendingDelta, sCompressedData);
	}
	else
#endif // /#if SEOUL_UNIT_TESTS
	{
		eResult = SaveLoadUtil::ToBase64(pendingDeltaMetadata, pendingDelta, sCompressedData);
	}

	if (SaveLoadResult::kSuccess != eResult)
	{
		return eResult;
	}

	// Compute the MD5 of the local data. This will be used by the server
	// to verify application of the delta.
	String const sTargetMD5(targetSaveData.ComputeMD5());

	// Monitor for the HTTP request.
	CloudRequestMonitor monitor;

	// Create and issue the request. Load requests must succeed, but we
	// handle this in our own throttled loop outside of the HTTP system,
	// so do not resend on failure.
	auto pRequest = m_Settings.m_CreateRequest(
		sURL,
		SEOUL_BIND_DELEGATE(&CloudRequestMonitor::Callback, &monitor),
		HTTP::Method::ksPost,
		false,
		bTestOnlyNoEmail);

#if SEOUL_UNIT_TESTS
	if (bTestOnlyNoEmail)
	{
		pRequest->SetIgnoreDomainRequestBudget(true);
	}
#endif // /SEOUL_UNIT_TESTS

	// The callback will return null if it can no longer fulfill
	// request creation attempts. this is equivalent to a cancellation
	// event.
	if (nullptr == pRequest)
	{
		return SaveLoadResult::kCloudCancelled;
	}

	pRequest->SetDispatchCallbackOnMainThread(false);

	// Exclude post arguments if we have no save data (rOutSaveData is empty). This
	// tells the server that we have no data and need a load operation in full.
	if (!rOutSaveData.GetRootNode().IsNull())
	{
		pRequest->AddPostData("data", sCompressedData);
		pRequest->AddPostData("target_md5", sTargetMD5);
	}

	pRequest->Start();

	// Now blocking wait on monitor completion.
	monitor.WaitForCompletion();

	// Success means we may or may not have data.
	if (HTTP::Result::kSuccess == monitor.GetResult())
	{
		// Different behavior based on status code.
		switch (monitor.GetStatus())
		{
			// We wanted data, but server had none, this is equivalent to a "file not found".
		case CloudRequestMonitor::kServerHasNoData:
			return SaveLoadResult::kErrorFileNotFound;

			// We wanted data, and the server sent it. Data may be empty at this point, this
			// forces a reset.
		case CloudRequestMonitor::kServerHasSentData:
			rOutMetadata = monitor.GetMetadata();
			rOutSaveData.Swap(monitor.GetData());
			return SaveLoadResult::kSuccess;

			// Server wants a resend with all data.
		case CloudRequestMonitor::kServerNeedsFullCheckpoint:
			return SaveLoadResult::kCloudNeedsFullCheckpoint;

			// Success from the server means no update from the server.
		case CloudRequestMonitor::kSuccess:
			return SaveLoadResult::kCloudNoUpdate;

			// Any other result here is treated as a permanent server
			// rejection or a temporary server error, depending on
			// the specific status code.
			//
			// Success with metadata should never be sent with
			// a load request, so we also classify that as
			// a permanent error case.
		case CloudRequestMonitor::kSuccessWithMetadata:
		case (Int32)HTTP::Status::kInternalServerError:
		default:
			// Temporary server errors return an appropriate code -
			// indicates failure but also indicates that the client
			// should ignore the error/treat it as temporary.
			if (monitor.GetStatus() >= (Int32)HTTP::Status::kInternalServerError)
			{
				return SaveLoadResult::kErrorServerInternalFailure;
			}
			else
			{
				return SaveLoadResult::kErrorServerRejection;
			}
		};
	}
	else if (HTTP::Result::kCanceled == monitor.GetResult())
	{
		// Special case for handling.
		return SaveLoadResult::kCloudCancelled;
	}
	else
	{
		// Any fall-through to here means a temporary network error.
		return SaveLoadResult::kErrorNetworkFailure;
	}
}

/**
 * Worker thread function for turning a byte array into
 * a fully parsed object with associated metadata.
 *
 * @param[in] filePath Local path to load the data from.
 * @param[out] rSaveData DataStore of the local save data checkpoint.
 * @param[out] rPendingDelta The last accumulated delta. Must
 *             be applied to the remote cloud checkpoint to bring it
 *             in-sync with the local checkpoint.
 * @param[out] rMetadata The local metadata state of the save container.
 *
 * @return kSuccess or an error code on failure. On failure,
 *         all output parameters are left unmodified.
 */
SaveLoadResult SaveLoadManager::WorkerThreadLoadLocalData(
	FilePath filePath,
	DataStore& rSaveData,
	DataStore& rPendingDelta,
	SaveLoadUtil::SaveFileMetadata& rMetadata) const
{
	// Load the data via the SaveApi system.
	StreamBuffer data;
	{
		auto const eResult = m_pSaveApi->Load(filePath, data);
		if (SaveLoadResult::kSuccess != eResult)
		{
			return eResult;
		}
	}

	// Finish the load.
	return LoadLocalData(data, rSaveData, rPendingDelta, rMetadata);
}

/**
 * Shared utility used for several local load paths. Instantiates
 * a concrete, type object from (migrated) save data.
 *
 * @param[in] entry A save entry object describing the load operation.
 * @param[in] dataStore The input data to instantiate the object from.
 * @param[out] rpObject An any wrapper around a pointer to the heap-allocated instance.
 */
SaveLoadResult SaveLoadManager::WorkerThreadCreateObject(
	const Entry& entry,
	const DataStore& dataStore,
	Reflection::WeakAny& rpObject)
{
	// Get the root data object.
	DataNode const dataRootNode = dataStore.GetRootNode();

	// Create an object of the desired type.
	auto pLoadedObject = entry.m_pLoadDataType->New(MemoryBudgets::Saving);
	if (!pLoadedObject.IsValid())
	{
		return SaveLoadResult::kErrorSerialization;
	}
	else
	{
		Reflection::SaveLoadContext context(entry.m_Path, dataStore, dataRootNode, pLoadedObject.GetTypeInfo());
		if (!Reflection::DeserializeObject(context, dataStore, dataRootNode, pLoadedObject))
		{
			entry.m_pLoadDataType->Delete(pLoadedObject);
			return SaveLoadResult::kErrorSerialization;
		}
		else
		{
			rpObject = pLoadedObject;
			return SaveLoadResult::kSuccess;
		}
	}
}

/**
 * Worker-thead only body, handles the full saving process,
 * including bundling of body data and encryption.
 *
 * Unlike loading, serialization of an object to a DataStore
 * occured in QueueSave(), since the saving body is threaded and
 * we need to "clone" the data prior to returning from QueueSave().
 *
 * @param[inout] rState The SaveLoadManager's captured state of save
 *                      for the corresponding entry.
 * @param[inout] entry The save operation to process.
 */
void SaveLoadManager::WorkerThreadSave(SaveFileState& rState, const Entry& entry) const
{
	// TODO: Break this out into config.
	static const Int64 kiCloudSaveLimitInMilliseconds = 30 * 1000;

	// Session guid is always updated on save attempt.
	rState.m_Metadata.m_sSessionGuid = m_sSessionGuid;

	// Version tracking is always updated on save attempt.
	rState.m_Metadata.m_iVersion = entry.m_iVersion;

	// Pending diff is either generated against the checkpoint, or is the full
	// save.
	DataStore pendingDiff;
	if (rState.m_pCheckpoint.IsValid())
	{
		// TODO: This failure case is a big problem, see if we can remove it.
		if (!ComputeDiff(*rState.m_pCheckpoint, *entry.m_pSaveData, pendingDiff))
		{
			DataStore emptyDataStore;
			pendingDiff.Swap(emptyDataStore);

			// If we're in testing mode, this is always a warning.
#if SEOUL_UNIT_TESTS
			if (m_Settings.m_bEnableValidation)
			{
				SEOUL_WARN("Failed generating diff during cloud save.");
			}
#endif // /#if SEOUL_UNIT_TESTS

			// Dispatch callback, if we have one.
			DispatchSaveCallback(entry.m_pCallback, SaveLoadResult::kErrorDiffGenerate, SaveLoadResult::kErrorDiffGenerate, SaveLoadResult::kErrorDiffGenerate);
			return;
		}
	}
	else
	{
		// Use the entire save snapshot.
		pendingDiff.CopyFrom(*entry.m_pSaveData);
	}

	SaveLoadResult eCloudResult = SaveLoadResult::kErrorUnknown;

	// Cloud saving.
	{
		// Apply rate limiting to the cloud saving.
		Int64 const iUptimeInMilliseconds = Engine::Get()->GetUptimeInMilliseconds();
		if (entry.m_bForceImmediateCloudSave ||
			rState.m_iLastSaveUptimeInMilliseconds <= 0 ||
			(iUptimeInMilliseconds - rState.m_iLastSaveUptimeInMilliseconds) >= kiCloudSaveLimitInMilliseconds)
		{
			// Just before we cloud save, we always attempt a local save
			// The motivation for this is IAP handling.  We count on the local save
			// finishing right away in this case, so that when the purchase resumes
			// the pending IAP is ready.
			// We don't check the success/failure of the pre-save, because it will always
			// be followed by another local save attempt after the cloud save
			(void)WorkerThreadSaveLocalData(entry.m_Path, *entry.m_pSaveData, pendingDiff, rState.m_Metadata);

			// Time for a new save, commit the uptime.
			rState.m_iLastSaveUptimeInMilliseconds = iUptimeInMilliseconds;

			// Cloud saving is disabled if no URL.
			if (entry.m_sCloudURL.IsEmpty())
			{
				eCloudResult = SaveLoadResult::kCloudDisabled;
			}
			else
			{
				// Compute the target MD5 - this is used by the server to
				// verify application of the delta against its current checkpoint.
				String const sTargetMD5(entry.m_pSaveData->ComputeMD5());

				// Prior to each cloud save submission, we increment the max
				// transaction id.
				rState.m_Metadata.m_iTransactionIdMax++;
				eCloudResult = WorkerThreadSaveCloudData(
					rState,
					entry,
					rState.m_Metadata,
					pendingDiff,
					sTargetMD5,
#if SEOUL_UNIT_TESTS
					m_Settings.m_bEnableValidation);
#else // !#if SEOUL_UNIT_TESTS
					false);
#endif // /!#if SEOUL_UNIT_TESTS

				// Issue a retry now.
				if (SaveLoadResult::kCloudNeedsFullCheckpoint == eCloudResult)
				{
					// If we're in testing mode, this is always a warning.
#if SEOUL_UNIT_TESTS
					if (m_Settings.m_bEnableValidation)
					{
						SEOUL_WARN("Unexpected kCloudSaveWantsFullSaveCheckpoint from WorkerThreadSaveCloudData.");
					}
#endif // /#if SEOUL_UNIT_TESTS

					// For a resend, we increment the max again, but set the
					// min to 0.
					rState.m_Metadata.m_iTransactionIdMax++;
					auto pendingDiffMetadata = rState.m_Metadata;
					pendingDiffMetadata.m_iTransactionIdMin = 0;
					eCloudResult = WorkerThreadSaveCloudData(
						rState,
						entry,
						pendingDiffMetadata,
						*entry.m_pSaveData,
						sTargetMD5,
#if SEOUL_UNIT_TESTS
						m_Settings.m_bEnableValidation);
#else // !#if SEOUL_UNIT_TESTS
						false);
#endif // /!#if SEOUL_UNIT_TESTS
				}

				// On success, update delta and state.
				if (SaveLoadResult::kSuccess == eCloudResult)
				{
					// Update the checkpoint data.
					rState.m_pCheckpoint.Reset(SEOUL_NEW(MemoryBudgets::Saving) DataStore);
					rState.m_pCheckpoint->CopyFrom(*entry.m_pSaveData);

					// Synchronize minimum transaction id.
					rState.m_Metadata.m_iTransactionIdMin = rState.m_Metadata.m_iTransactionIdMax;

					// Now that the checkpoint is up to date, clear the pending diff.
					DataStore emptyDataStore;
					pendingDiff.Swap(emptyDataStore);
				}
			}
		}
		else
		{
			eCloudResult = SaveLoadResult::kCloudRateLimiting;
		}
	}

	auto const eLocalResult = WorkerThreadSaveLocalData(entry.m_Path, *entry.m_pSaveData, pendingDiff, rState.m_Metadata);
	auto const eFinalResult = Min(eLocalResult, eCloudResult);

	// If we're in testing mode, check values.
#if SEOUL_UNIT_TESTS
	if (m_Settings.m_bEnableValidation)
	{
		if (SaveLoadResult::kSuccess != eLocalResult)
		{
			SEOUL_WARN("Unexpected %s from WorkerThreadSaveLocalData: %s", EnumToString<SaveLoadResult>(eLocalResult), entry.m_Path.GetAbsoluteFilenameInSource().CStr());
		}
		// Ignore kErrorServerInternalFailure: it's often just S3, and should be retryable
		if (SaveLoadResult::kSuccess != eCloudResult && SaveLoadResult::kCloudRateLimiting != eCloudResult && SaveLoadResult::kErrorServerInternalFailure != eCloudResult)
		{
			// During shutdown, cancelled is allowed as well.
			if (!(SaveLoadResult::kCloudCancelled == eCloudResult && CrashManager::Get()->GetCrashContext() == CrashContext::kShutdown))
			{
				SEOUL_WARN("Unexpected %s from WorkerThreadSaveCloudData: %s", EnumToString<SaveLoadResult>(eCloudResult), entry.m_Path.GetAbsoluteFilenameInSource().CStr());
			}
		}
	}
#endif // /#if SEOUL_UNIT_TESTS

	// In ship only, send local save failures as custom crashes.
	//
	// This error doesn't apply in success or file-not-found cases, which are typical.
	//
	// We also filter a few additional errors that we can't fix (out of space on disk):.
#if SEOUL_SHIP
	if (SaveLoadResult::kSuccess != eLocalResult &&
		SaveLoadResult::kErrorFileNotFound != eLocalResult &&
		SaveLoadResult::kErrorRenameNoSpace != eLocalResult &&
		SaveLoadResult::kErrorFileWriteNoSpace != eLocalResult)
	{
		if (CrashManager::Get())
		{
			CustomCrashErrorState state;
			CustomCrashErrorState::Frame frame;
			frame.m_sFilename = __FILE__;
			frame.m_iLine = __LINE__;
			frame.m_sFunction = __FUNCTION__;
			state.m_vStack.PushBack(frame);
			state.m_sReason = String::Printf(
				"WorkerThreadSaveLocalData(%s): %s",
				EnumToString<SaveLoadResult>(eLocalResult),
				entry.m_Path.GetAbsoluteFilenameInSource().CStr());
			CrashManager::Get()->SendCustomCrash(state);
		}
	}
#endif // /#if SEOUL_SHIP

	// Dispatch callback, if we have one.
	DispatchSaveCallback(entry.m_pCallback, eLocalResult, eCloudResult, eFinalResult);
}

/** Debug only utility, verifies that the server is properly in-sync with our local data. */
#if SEOUL_UNIT_TESTS
static void VerifyCloudSaveAgainstLocal(
	const SaveLoadUtil::SaveFileMetadata& cloudMetadata,
	const DataStore& cloud,
	const SaveLoadUtil::SaveFileMetadata& localMetadata,
	const DataStore& local)
{
	// Min and max transaction ids of the cloudMetadata must
	// be equal to the max transaction id of the localMetadata,
	// and versions must be equal.
	if (cloudMetadata.m_iVersion != localMetadata.m_iVersion ||
		cloudMetadata.m_iTransactionIdMax != localMetadata.m_iTransactionIdMax ||
		cloudMetadata.m_iTransactionIdMin != localMetadata.m_iTransactionIdMax ||
		cloudMetadata.m_sSessionGuid != localMetadata.m_sSessionGuid)
	{
		SEOUL_WARN("Cloud and local save metadata are out of sync, see log for details.");

		String s;
		(void)Reflection::SerializeToString(&cloudMetadata, s, true, 0, true);
		SEOUL_LOG("Cloud metadata: %s\n", s.CStr());

		(void)Reflection::SerializeToString(&localMetadata, s, true, 0, true);
		SEOUL_LOG("Local metadata: %s\n", s.CStr());
	}

	if (!DataStore::Equals(
		cloud, cloud.GetRootNode(),
		local, local.GetRootNode(), true))
	{
		SEOUL_WARN("Cloud and local data are out of sync, see log for details.");

		DataStore diff;
		(void)ComputeDiff(cloud, local, diff);

		String s;
		diff.ToString(diff.GetRootNode(), s, true, 0, true);
		SEOUL_LOG("Local to Cloud Diff: %s", s.CStr());
	}
}
#endif // /#if SEOUL_UNIT_TESTS

/**
 * When specified, generates a delta and commits the given entry to remote
 * cloud storage.
 *
 * @param[inout] state The SaveLoadManager's captured state of save
 *                     for the corresponding entry.
 * @param[in] entry The save operation to process.
 * @param[in] pendingDeltaMetadata Metadata to submit to the server.
 * @param[in] pendingDelta Data to submit to the server.
 * @param[in] sTargetMD5 The expected MD5 of the output data after processing on the server.
 * @param[in] bEnableTests Development builds only, enables special code paths for testing cloud save functionality.
 * @param[in] bTestOnlyNoEmail In testing builds only, when true, tells the server not to send out
 *                             error emails on error conditions, used when a test is intentionally
 *                             triggering an error.
 * @param[in] bTestOnlyNoVerify In testing builds only, when true, uses the non-verifying version
 *                              of ToBase64 to intentionally send corrupted data to the server.
 */
SaveLoadResult SaveLoadManager::WorkerThreadSaveCloudData(
	const SaveFileState& state,
	const Entry& entry,
	const SaveLoadUtil::SaveFileMetadata& pendingDeltaMetadata,
	const DataStore& pendingDelta,
	const String& sTargetMD5,
	Bool bEnableTests,
	Bool bTestOnlyNoEmail /* = false */,
	Bool bTestOnlyNoVerify /* = false */) const
{
	// Prepare data for cloud submission.
	String sCompressedData;
	SaveLoadResult eResult = SaveLoadResult::kErrorUnknown;

	// TODO: Temporary log to try to track down a periodic cloud save problem.
	SEOUL_LOG("Cloud save: %s (%d, %d, %s, %s, %s)\n", entry.m_sCloudURL.CStr(), (Int32)pendingDeltaMetadata.m_iTransactionIdMin, (Int32)pendingDeltaMetadata.m_iTransactionIdMax, (bEnableTests ? "true" : "false"), (bTestOnlyNoEmail ? "true" : "false"), (bTestOnlyNoVerify ? "true" : "false"));

#if SEOUL_UNIT_TESTS
	if (bTestOnlyNoVerify)
	{
		eResult = SaveLoadUtil::UnitTestHook_ToBase64NoVerify(pendingDeltaMetadata, pendingDelta, sCompressedData);
	}
	else
#endif // /#if SEOUL_UNIT_TESTS
	{
		eResult = SaveLoadUtil::ToBase64(pendingDeltaMetadata, pendingDelta, sCompressedData);
	}

	if (SaveLoadResult::kSuccess != eResult)
	{
		return eResult;
	}

	CloudRequestMonitor monitor;

	// Create and issue the request.
	auto pRequest = m_Settings.m_CreateRequest(
		entry.m_sCloudURL,
		SEOUL_BIND_DELEGATE(&CloudRequestMonitor::Callback, &monitor),
		HTTP::Method::ksPost,
		false, // Do not resend on failure
		bTestOnlyNoEmail);

	// The callback will return null if it can no longer fulfill
	// request creation attempts. this is equivalent to a cancellation
	// event.
	if (nullptr == pRequest)
	{
		return SaveLoadResult::kCloudCancelled;
	}

	pRequest->SetDispatchCallbackOnMainThread(false);

	pRequest->AddPostData("data", sCompressedData);
	pRequest->AddPostData("target_md5", sTargetMD5);
#if SEOUL_UNIT_TESTS
	if (bEnableTests)
	{
		pRequest->AddPostData("testing", "true");
		pRequest->SetIgnoreDomainRequestBudget(true);
	}
#endif // /#if SEOUL_UNIT_TESTS

	pRequest->Start();

	// Blocking wait on operation completion.
	monitor.WaitForCompletion();

	// Update tracking for diff purposes.
	if (HTTP::Result::kSuccess == monitor.GetResult())
	{
		if (CloudRequestMonitor::kServerNeedsFullCheckpoint == monitor.GetStatus())
		{
			// Need to issue a new request with all data.
			return SaveLoadResult::kCloudNeedsFullCheckpoint;
		}
		else
		{
			// Verify that the server's state is in sync with the client.
#if SEOUL_UNIT_TESTS
			if (bEnableTests && CloudRequestMonitor::kServerHasSentData == monitor.GetStatus())
			{
				VerifyCloudSaveAgainstLocal(
					monitor.GetMetadata(),
					monitor.GetData(),
					state.m_Metadata,
					*entry.m_pSaveData);

				return SaveLoadResult::kSuccess;
			}
#endif // /#if SEOUL_UNIT_TESTS

			// Done, success if kSuccess and the metadata matches our
			// expectation, otherwise an appropriate error code.
			if (CloudRequestMonitor::kSuccessWithMetadata == monitor.GetStatus())
			{
				auto const& returnedMetadata = monitor.GetMetadata();
				if (returnedMetadata.m_sSessionGuid != pendingDeltaMetadata.m_sSessionGuid)
				{
					return SaveLoadResult::kErrorSessionGuid;
				}
				if (returnedMetadata.m_iTransactionIdMax != pendingDeltaMetadata.m_iTransactionIdMax)
				{
					return SaveLoadResult::kErrorTransactionIdMax;
				}

				return SaveLoadResult::kSuccess;
			}
			else
			{
				// Temporary server error, report as such.
				if (monitor.GetStatus() >= (Int32)HTTP::Status::kInternalServerError)
				{
					return SaveLoadResult::kErrorServerInternalFailure;
				}
				// Otherwise, permanent server rejection. Means we're sending
				// bad or out-of-date data.
				else
				{
					return SaveLoadResult::kErrorServerRejection;
				}
			}
		}
	}
	else if (HTTP::Result::kCanceled == monitor.GetResult())
	{
		return SaveLoadResult::kCloudCancelled;
	}
	// All others are temporary network errors.
	else
	{
		return SaveLoadResult::kErrorNetworkFailure;
	}
}

/**
 * Worker-thread only body, handles the body of the saving
 * process.
 *
 * @param[in] filePath The local path to save the data to.
 * @param[in] saveData The current local checkpoint. pendingDelta, if not empty,
 *                     needs to be applied to the server checkpoint (at transaction
 *                     m_iTransactionIdMin) to bring it up-to-date with the local
 *                     checkpoint.
 * @param[in] pendingDelta The delta of entry against the current save checkpoint,
 *                         used for cloud saving operations, computed by
 *                         WorkerThreadCloudSave().
 * @param[in] metadata The metadata the describes the pendingDelta + saveData pair.
 */
SaveLoadResult SaveLoadManager::WorkerThreadSaveLocalData(
	FilePath filePath,
	const DataStore& saveData,
	const DataStore& pendingDelta,
	const SaveLoadUtil::SaveFileMetadata& metadata) const
{
	StreamBuffer data;
	auto const eResult = SaveLocalData(data, saveData, pendingDelta, metadata);
	if (SaveLoadResult::kSuccess != eResult)
	{
		return eResult;
	}

	// Final step, actually write the container data to platform storage.
	return m_pSaveApi->Save(filePath, data);
}

/**
 * Threaded worker body for saving and loading operations.
 *
 * Processes a queue of load/save operations. Redundancy
 * elimination is performed here (the goal is to avoid, mainly,
 * saving an unnecessary number of times).
 */
Int SaveLoadManager::WorkerThreadMain(const Thread&)
{
#if SEOUL_ENABLE_CHEATS
	// Pending hashset of save to reset, developer only function.
	HashTable<FilePath, Pair<String, Bool>, MemoryBudgets::Saving> tResetSaves;
#endif // /#if SEOUL_ENABLE_CHEATS

	// Table of save state stored during a single game session.
	StateTable tState;

	while (m_bRunning)
	{
		// Resume immediately if there are entries in the queue.
		if (m_WorkQueue.IsEmpty())
		{
			// Otherwise, wait for work.
			m_pSignal->Wait();
		}

		// Build a list of tasks to do - we do this
		// so we can filter redundant saves to the same
		// path, only executing the last queued save operation.
		Vector<Entry*> vEntries;

		// Get all entries until we consume all current
		// or until no longer running.
		{
			while (m_bRunning)
			{
				// Get the next entry - break if none.
				Entry* pEntry = m_WorkQueue.Pop();
				if (nullptr == pEntry)
				{
					break;
				}

				// Buffer up this entry.
				vEntries.PushBack(pEntry);
			}
		}

		// While we still have entries to process and
		// while still running, run them.
		for (UInt32 uEntry = 0u; m_bRunning && uEntry < vEntries.GetSize(); ++uEntry)
		{
			auto pEntry = vEntries[uEntry];

			// Test only hook - expose the path currently being handled.
#if SEOUL_UNIT_TESTS
			auto const scoped(MakeScopedAction(
			[&]()
			{
				Lock lock(m_UnitTestActiveFilePathMutex);
				m_UnitTestActiveFilePath = pEntry->m_Path;
			},
			[&]()
			{
				Lock lock(m_UnitTestActiveFilePathMutex);
				m_UnitTestActiveFilePath = FilePath();
			}));
#endif // /#if SEOUL_UNIT_TESTS

			switch (pEntry->m_eEntryType)
			{
				case kLoadNoSessionChange:
				case kLoadSessionChange:
				{
					Bool bGeneratedGuid = false;

#if SEOUL_ENABLE_CHEATS
					Bool bDone = false;

					// In non-ship builds, if pEntry's file path is in the reset
					// save hash set, don't actually load. Just return an empty
					// entry.
					Pair<String, Bool> resetData;
					if (tResetSaves.GetValue(pEntry->m_Path, resetData))
					{
						// On reset, delete the local file if it exists.
						if (FileManager::Get()->Exists(pEntry->m_Path))
						{
							FileManager::Get()->Delete(pEntry->m_Path);
						}

						(void)tResetSaves.Erase(pEntry->m_Path);
						(void)tState.Erase(pEntry->m_Path);

						// If requested, reset the session GUID.
						if (resetData.Second)
						{
							m_sSessionGuid = UUID::GenerateV4().ToString();
							bGeneratedGuid = true;
						}

						// If sCloudURL is empty, just dispatch the callback
						// immediately with a "file not found" result. Skip
						// loading.
						if (resetData.First.IsEmpty() || pEntry->m_sCloudURL.IsEmpty())
						{
							bDone = true;
							DispatchLoadCallback(
								pEntry->m_pCallback,
								SaveLoadResult::kErrorFileNotFound,
								SaveLoadResult::kErrorFileNotFound,
								SaveLoadResult::kErrorFileNotFound,
								Reflection::WeakAny());
						}
						// If there is a cloud URL (this is a reset URL),
						// use this in place of the load URL and continue
						// on with the load request.
						else
						{
							pEntry->m_sCloudURL = resetData.First;
						}
					}

					if (!bDone)
#endif // /#if SEOUL_ENABLE_CHEATS
					{
						auto& rState = ResolveState(tState, pEntry->m_Path);

#if SEOUL_UNIT_TESTS
						// Prior to load, if we haven't run tests on this entry yet, do so now.
						RunFirstTimeLoadTests(rState, *pEntry);
#endif // /#if SEOUL_UNIT_TESTS

						// Just before the load, if requested and still needed,
						// generate a new session guid.
						if (kLoadSessionChange == pEntry->m_eEntryType && !bGeneratedGuid)
						{
							m_sSessionGuid = UUID::GenerateV4().ToString();
							bGeneratedGuid = true;
						}

						WorkerThreadLoad(rState, *pEntry);
					}
					break;
				}
				case kSave:
				{
					// Skip save operations that are redundant.
					if (uEntry + 1u < vEntries.GetSize())
					{
						auto pNextEntry = vEntries[uEntry + 1u];
						if (pEntry->m_pCallback == pNextEntry->m_pCallback &&
							pEntry->m_eEntryType == pNextEntry->m_eEntryType &&
							pEntry->m_Path == pNextEntry->m_Path &&
							pEntry->m_iVersion == pNextEntry->m_iVersion &&
							pEntry->m_bForceImmediateCloudSave == pNextEntry->m_bForceImmediateCloudSave)
						{
							// Skip this save operation, run the next one instead.
							continue;
						}
					}

					auto& rState = ResolveState(tState, pEntry->m_Path);

#if SEOUL_UNIT_TESTS
					// Prior to save, if we haven't run tests on this entry yet, do so now.
					RunFirstTimeSaveTests(rState, *pEntry);
#endif // /#if SEOUL_UNIT_TESTS

					WorkerThreadSave(rState, *pEntry);
					break;
				}
#if SEOUL_ENABLE_CHEATS
				case kSaveResetNoSessionChange:
				case kSaveResetSessionChange:
				{
					tResetSaves.Insert(pEntry->m_Path, MakePair(pEntry->m_sCloudURL, (kSaveResetSessionChange == pEntry->m_eEntryType)));
					break;
				}
#endif // /#if SEOUL_ENABLE_CHEATS
				default:
					// nop
					break;
			};
		}

		// Cleanup all entries we just processed.
		SafeDeleteVector(vEntries);
	}

	return 0;
}

#if SEOUL_UNIT_TESTS
#define SEOUL_TEST_TRUE(expr, sMessage, ...) (void)( (!!(expr)) || (SEOUL_WARN("[SaveLoadManagerTest(%d)]: %s", __LINE__, Seoul::String::Printf((sMessage), ##__VA_ARGS__).CStr()), 0))
#define SEOUL_TEST_EXPECT(expect, actual) SEOUL_TEST_TRUE(expect == actual, "Expected '%s', got '%s'", EnumToString<SaveLoadResult>(expect), EnumToString<SaveLoadResult>((actual)))

void SaveLoadManager::RunFirstTimeLoadTests(SaveFileState& rState, const Entry& entry)
{
	// Prior to load, if we haven't run tests on this entry yet, do so now.
	if (!m_Settings.m_bEnableFirstTimeTests || rState.m_bRanFirstTimeLoadTests)
	{
		return;
	}

	// Now run.
	rState.m_bRanFirstTimeLoadTests = true;

	// Only cloud tests currently, we handle local testing with unit tests.
	if (entry.m_sCloudURL.IsEmpty())
	{
		return;
	}

	// For testing purposes, we assume we're running on a fresh account, so the cloud should have no data initially.
	DataStore const zeroDataStore;
	{
		SaveLoadUtil::SaveFileMetadata emptyMetadata;
		DataStore emptyDataStore;
		auto const eResult = WorkerThreadLoadCloudData(
			entry.m_sCloudURL,
			emptyMetadata,
			emptyDataStore,
			emptyMetadata,
			emptyDataStore,
			emptyMetadata,
			emptyDataStore,
			true);

		SEOUL_TEST_EXPECT(SaveLoadResult::kErrorFileNotFound, eResult);

		// Test that no mutations occured.
		SEOUL_TEST_TRUE(SaveLoadUtil::SaveFileMetadata() == emptyMetadata, "unexpected mutation of metadata.");
		SEOUL_TEST_TRUE(DataStore::UnitTestHook_ByteForByteEqual(zeroDataStore, emptyDataStore), "unexpected mutation of DataStore.");
	}

	// Now we're going to send an "update", but at a transaction id the server doesn't know about. The server
	// should respond with a "needs full data".
	{
		SaveLoadUtil::SaveFileMetadata metadata;
		metadata.m_iTransactionIdMin = 1;
		metadata.m_iTransactionIdMax = 2;
		metadata.m_sSessionGuid = m_sSessionGuid;
		auto metadataCopy = metadata;
		DataStore dataStore;
		dataStore.MakeTable();
		DataStore dataStoreCopy;
		dataStoreCopy.CopyFrom(dataStore);
		auto const eResult = WorkerThreadLoadCloudData(
			entry.m_sCloudURL,
			metadata,
			dataStore,
			metadata,
			dataStore,
			metadata,
			dataStore,
			true);

		SEOUL_TEST_EXPECT(SaveLoadResult::kCloudNeedsFullCheckpoint, eResult);

		// Test that no mutations occured.
		SEOUL_TEST_TRUE(metadataCopy == metadata, "unexpected mutation of metadata.");
		SEOUL_TEST_TRUE(DataStore::UnitTestHook_ByteForByteEqual(dataStoreCopy, dataStore), "unexpected mutation of DataStore.");
	}

	// Now we're actually going to send a full update, but that update is going to contain garbage data.
	// The client should reject it prior to send (a verify integrity check should occur in the save blob).
	for (Int32 i = 0; i < DataStore::CORRUPTION_TYPES; ++i)
	{
		SaveLoadUtil::SaveFileMetadata metadata;
		metadata.m_iTransactionIdMin = 0;
		metadata.m_iTransactionIdMax = 2;
		metadata.m_sSessionGuid = m_sSessionGuid;
		auto metadataCopy = metadata;
		DataStore dataStore;
		dataStore.UnitTestHook_FillWithCorruptedData((DataStore::CorruptedDataType)i);
		DataStore dataStoreCopy;
		dataStoreCopy.CopyFrom(dataStore);
		auto const eResult = WorkerThreadLoadCloudData(
			entry.m_sCloudURL,
			metadata,
			dataStore,
			metadata,
			dataStore,
			metadata,
			dataStore,
			true);

		SEOUL_TEST_EXPECT(SaveLoadResult::kErrorSaveCheck, eResult);

		// Test that no mutations occured.
		SEOUL_TEST_TRUE(metadataCopy == metadata, "unexpected mutation of metadata.");
		SEOUL_TEST_TRUE(DataStore::UnitTestHook_ByteForByteEqual(dataStoreCopy, dataStore), "unexpected mutation of DataStore.");
	}

	// Same as the previous set of tests, but this time we'll use the "no verify" version of ToBase64,
	// to intentionally send bad data to the server.
	for (Int32 i = 0; i < DataStore::CORRUPTION_TYPES; ++i)
	{
		SaveLoadUtil::SaveFileMetadata metadata;
		metadata.m_iTransactionIdMin = 0;
		metadata.m_iTransactionIdMax = 2;
		auto metadataCopy = metadata;
		DataStore dataStore;
		dataStore.UnitTestHook_FillWithCorruptedData((DataStore::CorruptedDataType)i);
		DataStore dataStoreCopy;
		dataStoreCopy.CopyFrom(dataStore);

		auto const eResult = WorkerThreadLoadCloudData(
			entry.m_sCloudURL,
			metadata,
			dataStore,
			metadata,
			dataStore,
			metadata,
			dataStore,
			true,
			true);

		// In this situation, we expect a kSuccess result (the server is overwriting our local data)
		// but it shouldn't have any data to send, so dataStore and metadata should now be equal
		// to the default state.

		SEOUL_TEST_EXPECT(SaveLoadResult::kSuccess, eResult);

		// Test that no mutations occured.
		SEOUL_TEST_TRUE(SaveLoadUtil::SaveFileMetadata() == metadata, "metadata is not the default.");
		SEOUL_TEST_TRUE(DataStore::UnitTestHook_ByteForByteEqual(zeroDataStore, dataStore), "DataStore is not the default.");
	}

	// Once again, the server should have no data, since it didn't accept the previous submission.
	{
		SaveLoadUtil::SaveFileMetadata emptyMetadata;
		DataStore emptyDataStore;
		auto const eResult = WorkerThreadLoadCloudData(
			entry.m_sCloudURL,
			emptyMetadata,
			emptyDataStore,
			emptyMetadata,
			emptyDataStore,
			emptyMetadata,
			emptyDataStore,
			true);

		SEOUL_TEST_EXPECT(SaveLoadResult::kErrorFileNotFound, eResult);

		// Test that no mutations occured.
		SEOUL_TEST_TRUE(SaveLoadUtil::SaveFileMetadata() == emptyMetadata, "unexpected mutation of metadata.");
		SEOUL_TEST_TRUE(DataStore::UnitTestHook_ByteForByteEqual(zeroDataStore, emptyDataStore), "unexpected mutation of DataStore.");
	}

	// Global tracking - must be lack to mark completion.
	m_bFirstTimeLoadTestsComplete = true;
}

void SaveLoadManager::RunFirstTimeSaveTests(SaveFileState& rState, const Entry& entry)
{
	// Prior to save, if we haven't run tests on this entry yet, do so now.
	if (!m_Settings.m_bEnableFirstTimeTests || rState.m_bRanFirstTimeSaveTests)
	{
		return;
	}

	// Now run.
	rState.m_bRanFirstTimeSaveTests = true;

	// Only cloud tests currently, we handle local testing with unit tests.
	if (entry.m_sCloudURL.IsEmpty())
	{
		return;
	}

	// Send an update, but at a transaction id the server doesn't know about. The server
	// should respond with a "needs full data".
	{
		SaveLoadUtil::SaveFileMetadata metadata;
		metadata.m_iTransactionIdMin = 1;
		metadata.m_iTransactionIdMax = 2;
		metadata.m_sSessionGuid = m_sSessionGuid;
		auto metadataCopy = metadata;
		DataStore dataStore;
		dataStore.MakeTable();
		DataStore dataStoreCopy;
		dataStoreCopy.CopyFrom(dataStore);
		auto const eResult = WorkerThreadSaveCloudData(
			rState,
			entry,
			metadata,
			dataStore,
			dataStore.ComputeMD5(),
			true,
			true);

		SEOUL_TEST_EXPECT(SaveLoadResult::kCloudNeedsFullCheckpoint, eResult);

		// Test that no mutations occured.
		SEOUL_TEST_TRUE(metadataCopy == metadata, "unexpected mutation of metadata.");
		SEOUL_TEST_TRUE(DataStore::UnitTestHook_ByteForByteEqual(dataStoreCopy, dataStore), "unexpected mutation of DataStore.");
	}

	// Send an update with an invalid transaction id min/max (max <= min)
	{
		SaveLoadUtil::SaveFileMetadata metadata;
		metadata.m_iTransactionIdMin = 1;
		metadata.m_iTransactionIdMax = 0;
		metadata.m_sSessionGuid = m_sSessionGuid;
		auto metadataCopy = metadata;
		DataStore dataStore;
		dataStore.MakeTable();
		DataStore dataStoreCopy;
		dataStoreCopy.CopyFrom(dataStore);
		auto const eResult = WorkerThreadSaveCloudData(
			rState,
			entry,
			metadata,
			dataStore,
			dataStore.ComputeMD5(),
			true,
			true);

		SEOUL_TEST_EXPECT(SaveLoadResult::kErrorServerRejection, eResult);

		// Test that no mutations occured.
		SEOUL_TEST_TRUE(metadataCopy == metadata, "unexpected mutation of metadata.");
		SEOUL_TEST_TRUE(DataStore::UnitTestHook_ByteForByteEqual(dataStoreCopy, dataStore), "unexpected mutation of DataStore.");
	}

	// Now we're actually going to send a full update, but that update is going to contain garbage data.
	// The client should reject it prior to send (a verify integrity check should occur in the save blob).
	for (Int32 i = 0; i < DataStore::CORRUPTION_TYPES; ++i)
	{
		SaveLoadUtil::SaveFileMetadata metadata;
		metadata.m_iTransactionIdMin = 0;
		metadata.m_iTransactionIdMax = 2;
		metadata.m_sSessionGuid = m_sSessionGuid;
		auto metadataCopy = metadata;
		DataStore dataStore;
		dataStore.UnitTestHook_FillWithCorruptedData((DataStore::CorruptedDataType)i);
		DataStore dataStoreCopy;
		dataStoreCopy.CopyFrom(dataStore);
		auto const eResult = WorkerThreadSaveCloudData(
			rState,
			entry,
			metadata,
			dataStore,
			dataStore.ComputeMD5(),
			true,
			true);

		SEOUL_TEST_EXPECT(SaveLoadResult::kErrorSaveCheck, eResult);

		// Test that no mutations occured.
		SEOUL_TEST_TRUE(metadataCopy == metadata, "unexpected mutation of metadata.");
		SEOUL_TEST_TRUE(DataStore::UnitTestHook_ByteForByteEqual(dataStoreCopy, dataStore), "unexpected mutation of DataStore.");
	}

	// Same as the previous set of tests, except this time we don't verify on the client. This tests
	// the server's ability to handle garbage data.
	for (Int32 i = 0; i < DataStore::CORRUPTION_TYPES; ++i)
	{
		SaveLoadUtil::SaveFileMetadata metadata;
		metadata.m_iTransactionIdMin = 0;
		metadata.m_iTransactionIdMax = 2;
		metadata.m_sSessionGuid = m_sSessionGuid;
		auto metadataCopy = metadata;
		DataStore dataStore;
		dataStore.UnitTestHook_FillWithCorruptedData((DataStore::CorruptedDataType)i);
		DataStore dataStoreCopy;
		dataStoreCopy.CopyFrom(dataStore);
		auto const eResult = WorkerThreadSaveCloudData(
			rState,
			entry,
			metadata,
			dataStore,
			dataStore.ComputeMD5(),
			true,
			true,
			true);

		SEOUL_TEST_EXPECT(SaveLoadResult::kErrorServerRejection, eResult);

		// Test that no mutations occured.
		SEOUL_TEST_TRUE(metadataCopy == metadata, "unexpected mutation of metadata.");
		SEOUL_TEST_TRUE(DataStore::UnitTestHook_ByteForByteEqual(dataStoreCopy, dataStore), "unexpected mutation of DataStore.");
	}

	// Global tracking - must be lack to mark completion.
	m_bFirstTimeSaveTestsComplete = true;
}

#undef SEOUL_TEST_TRUE
#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul
