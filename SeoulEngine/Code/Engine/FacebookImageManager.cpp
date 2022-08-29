/**
 * \file FacebookImageManager.cpp
 * \brief Global singleton for requesting and caching
 * Facebook profile thumbnail images.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "DDS.h"
#include "FacebookImageManager.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "Image.h"
#include "JobsFunction.h"
#include "JobsJob.h"
#include "StreamBuffer.h"
#include "Texture.h"
#include "TextureFooter.h"

namespace Seoul
{

static const String s_kFacebookProfileImages("FacebookProfileImages");

typedef Pair<String, UInt64> FileNameAndTime;
struct FileTimeStampSorter SEOUL_SEALED
{
	Bool operator()(const FileNameAndTime& a, const FileNameAndTime& b) const
	{
		// sort by newest file first
		return (a.Second > b.Second);
	}
}; // struct FileTimeStampSorter

void RawFacebookImage::AllocateAndCopy(UInt8 const* pData, UInt32 uSize)
{
	// Set and copy the contents over
	m_uByteSize = uSize;
	m_pBuffer = (UInt8*)MemoryManager::AllocateAligned(
		uSize,
		MemoryBudgets::TBD,
		SEOUL_ALIGN_OF(UInt8),
		__FILE__,
		__LINE__);

	memcpy(m_pBuffer, pData, uSize);
}

void RawFacebookImage::SafeDelete()
{
	UInt8* pBuffer = m_pBuffer;
	m_pBuffer = nullptr;
	MemoryManager::Deallocate(pBuffer);

	m_uByteSize = 0u;
}

/**
 * Internal job used by FacebookImageManager that converts raw jpeg and into engine format at run Time
 * thread, onto the background thread.
 */
class FacebookImageConverterJob SEOUL_SEALED : public Jobs::Job
{
public:
	FacebookImageConverterJob(const String& sGuid, const RawFacebookImage& data)
		: m_RawImageBuffer(data)
		, m_pImageBuffer(nullptr)
		, m_pCompressedBuffer(nullptr)
		, m_uCompressedSize(0u)
		, m_sUserGuid(sGuid)
		, m_CachedFilePath()
		, m_bSuccess(true)
	{
	}

	~FacebookImageConverterJob()
	{
		WaitUntilJobIsNotRunning();
		CleanUp();
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(FacebookImageConverterJob);

	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE
	{
		Bool const bFileIOThread = IsFileIOThread();
		Bool const bMainThread = IsMainThread();

		// General purpose thread, do the initial convert.
		if (!bFileIOThread && !bMainThread)
		{
			// Convert the raw image
			m_bSuccess = ConvertRawImage();

			rNextThreadId = (m_bSuccess ? GetFileIOThreadId() : GetMainThreadId());
			return;
		}
		// Write the data to disk.
		else if (bFileIOThread)
		{
			// If the image was converted properly then we can create the file
			if (m_bSuccess)
			{
				m_bSuccess = CreateFile();
			}

			rNextThreadId = GetMainThreadId();
			return;
		}
		// Fixup entries referencing the image.
		else if (bMainThread)
		{
			if (FacebookImageManager::Get())
			{
				// Set to initialy to the default state
				CheckedPtr<FacebookImageManager> fbImageManger = FacebookImageManager::Get();
				FacebookImageInfo info;
				info.m_eState = FacebookImageState::kFailed;

				// If everything went right
				// then we can update the facebookImage info
				// by copy
				if (m_bSuccess)
				{
					// set the state of the image info with the new created file path
					info.m_eState = FacebookImageState::kSuccess;
					info.m_FilePath = m_CachedFilePath;
				}

				// Update the information in facebook image manager
				fbImageManger->UpdateImageInfoProcess(m_sUserGuid, info);
			}

			// Apply cleanup for any allocation that
			// we made when convering the raw profile image
			CleanUp();

			reNextState = Jobs::State::kComplete;
			return;
		}

		// We should never get here - this state will likely result in broken saving. The game
		// will stop saving.
		reNextState = Jobs::State::kError;
		SEOUL_FAIL("Unexpected thread execution of FacebookImageConverterJob.");
	}

	/**
	 * helper function that takes the raw facebook image and converts
	 * it to raw engine format  (look at Image.h)
	 */
	Bool ConvertRawImage()
	{
		// Convert our raw jpeq using our wrapper functionality for stb_image
		// for right we assume that all facebook images are opaque
		Int32 iW = 0;
		Int32 iH = 0;
		m_pImageBuffer = LoadImageFromMemory(
			m_RawImageBuffer.GetBuffer(),
			m_RawImageBuffer.GetSize(),
			&iW,
			&iH);

		// Do insanity check for right output from stb_image
		if (nullptr == m_pImageBuffer || iW <= 0 || iH <= 0)
		{
			return false;
		}

		// Go through and swap the R and A channel so we can be the format ARGB
		SEOUL_ASSERT((size_t)m_pImageBuffer % 4u == 0u);
		ColorSwapR8B8((UInt32*)m_pImageBuffer, (UInt32*)((UInt8*)m_pImageBuffer + ((UInt32)iW * (UInt32)iH * 4u)));

		// Take the image and package the image into DDS format so we can ship it
		DdsHeader header;
		memset(&header, 0, sizeof(header));
		header.m_MagicNumber = kDdsMagicValue;
		header.m_Size = sizeof(header) - sizeof(header.m_MagicNumber);
		header.m_HeaderFlags = kDdsHeaderFlagsTexture | kDdsHeaderFlagsLinearSize;
		header.m_Height = iH;
		header.m_Width  = iW;
		header.m_PitchOrLinearSize = (iW * 4);
		header.m_PixelFormat = kDdsPixelFormatA8R8G8B8;

		// Create the contents of the buffer
		// that combines the header of the DDS
		// a raw image data itself
		StreamBuffer buffer;
		buffer.Write(header);
		buffer.Write(m_pImageBuffer, iW * iH * 4);

		// fill out a texture footer for the end of the file
		// so the cooker knows how to handle the texture
		// and not delete it...
		TextureFooter const footer = TextureFooter::Default();
		buffer.Write(footer);

		// Insanity check if success.
		if (!ZSTDCompress(
			buffer.GetBuffer(),
			buffer.GetTotalDataSizeInBytes(),
			m_pCompressedBuffer,
			m_uCompressedSize))
		{
			return false;
		}

		return true;
	}

	Bool CreateFile()
	{
		// Create the file path name aka create the folder
		// and also create the full file path aka create the image file itself
		String const sFileNamePath = Path::Combine(GamePaths::Get()->GetSaveDir(), s_kFacebookProfileImages);
		String const sFileNameMip0 = Path::ReplaceExtension(Path::Combine(sFileNamePath, m_sUserGuid), ".sif0");

		for (Int32 i = 0; i < (Int32)FileType::LAST_TEXTURE_TYPE - (Int32)FileType::FIRST_TEXTURE_TYPE + 1; ++i)
		{
			String const sExt = String::Printf(".sif%d", i);
			String const sFileName = Path::ReplaceExtension(Path::Combine(sFileNamePath, m_sUserGuid), sExt);

			// If this fails we might do some logic
			// that puts the threads to sleep for a little
			// and then tries it again. not sure
			if (!FileManager::Get()->CreateDirPath(sFileNamePath))
			{
				return false;
			}

			ScopedPtr<SyncFile> pFile;

			// If we cant open the file then then we cant write to it so we failed
			if (!FileManager::Get()->OpenFile(sFileName, File::kWriteTruncate, pFile) ||
				!pFile->CanWrite())
			{
				return false;
			}

			UInt32 const uWriteSize = pFile->WriteRawData(m_pCompressedBuffer, m_uCompressedSize);

			// If we didnt the write the amount
			// that we specified that means something went wrong
			if (uWriteSize != m_uCompressedSize)
			{
				return false;
			}
		}


		// Check write size with compressed size
		m_CachedFilePath = FilePath::CreateSaveFilePath(sFileNameMip0);

		// Check if fail
		if (!m_CachedFilePath.IsValid())
		{
			return false;
		}

		return true;
	}

	void CleanUp()
	{
		m_bSuccess = true;
		m_CachedFilePath = FilePath();
		m_sUserGuid = String();
		m_uCompressedSize = 0u;

		if (m_pCompressedBuffer)
		{
			MemoryManager::Deallocate(m_pCompressedBuffer);
			m_pCompressedBuffer = nullptr;
		}

		if (m_pImageBuffer)
		{
			FreeImage(m_pImageBuffer);
			m_pImageBuffer = nullptr;
		}

		// Once we are done we can safely delete the
		// copy contents buffer that we created in the raw image (since we are the only one holding)
		// a reference to it.
		m_RawImageBuffer.SafeDelete();
	}

	// Raw facebook profile image buffer
	// format.
	RawFacebookImage m_RawImageBuffer;
	UInt8* m_pImageBuffer;
	void* m_pCompressedBuffer;
	UInt32 m_uCompressedSize;
	String m_sUserGuid;
	FilePath m_CachedFilePath;
	Bool m_bSuccess;

	SEOUL_DISABLE_COPY(FacebookImageConverterJob);
}; // class FacebookImageConverterJob

FacebookImageManager::FacebookImageManager()
	: m_FacebookImageConfig()
	, m_uNumberOfFiles(0u)
{
}

FacebookImageManager::FacebookImageManager(const FacebookImageManagerConfig& config)
	: m_FacebookImageConfig(config)
	, m_uNumberOfFiles(0u)
{
	LoadFacebookImages();
}

FacebookImageManager::~FacebookImageManager()
{
	if (m_pRemoveFilesJob.IsValid())
	{
		m_pRemoveFilesJob->WaitUntilJobIsNotRunning();
		m_pRemoveFilesJob.Reset();
	}
}

FilePath FacebookImageManager::RequestFacebookImageBitmap(const String& sFacebookUserGuid)
{
	FilePath filePath;

	// Early bail if id is empty
	if (sFacebookUserGuid.IsEmpty())
	{
		return filePath;
	}

	// Flag that will decide if we need to request a download form facebook
	Bool bDownloadRequest = false;

	{
		// Lock the table
		Lock lock(m_TableMutex);

		// Check if the user guid is already in the table
		FacebookImageInfo imageInfo;

		// If we wdont have the image info
		// inside the table that means its new and we
		// should request it for download
		if (!m_tFaceImageTable.GetValue(sFacebookUserGuid, imageInfo))
		{
			bDownloadRequest = true;

			// Set the image state and insert it into our FB image table
			FacebookImageInfo newImageInfo;
			newImageInfo.m_eState = FacebookImageState::kDownloading;
			SEOUL_VERIFY(m_tFaceImageTable.Insert(sFacebookUserGuid, newImageInfo).Second);
		}
		else
		{
			// Check the state of the image and handle occordingly
			switch (imageInfo.m_eState)
			{
			case FacebookImageState::kSuccess:
				filePath = imageInfo.m_FilePath;
				break;
			case FacebookImageState::kFailed:
				break;
			case FacebookImageState::kFailedRetry:
				// TODO: Try to make another facebook request?

				// This logic should only happen when certain failed HTTP request status code
				// come back. Here are the list of the HTTP status code http://httpstatus.es/
				// The ones that follow this case is some of the 4xx codes. Example. Server time out, conflics etc...
				break;
			default:
				break;
			}
		}
	}

	// Make the request to facebook to download
	// the new profile image
	if (bDownloadRequest)
	{
		DoFacebookImageRequest(sFacebookUserGuid);
	}

	return filePath;
}

void FacebookImageManager::DoFacebookImageRequest(const String& sFacebookUserGuid)
{
	// Set up the HTTP request to facebook
	auto& r = HTTP::Manager::Get()->CreateRequest();

	String* psFacebookUserGuid = SEOUL_NEW(MemoryBudgets::TBD) String(sFacebookUserGuid);

	r.SetMethod(HTTP::Method::ksGet);
	r.SetURL(String::Printf("%s/%s/picture?type=%s&redirect=true",
		m_FacebookImageConfig.m_sFacebookBaseUrl.CStr(),
		sFacebookUserGuid.CStr(),
		m_FacebookImageConfig.m_sImageDownloadFormat.CStr()));
	r.SetCallback(SEOUL_BIND_DELEGATE(&OnFacebookImageRequestCallback, psFacebookUserGuid));
	r.SetResendOnFailure(false);
	r.SetVerifyPeer(false);

	// Fire off the request
	r.Start();
}

HTTP::CallbackResult FacebookImageManager::OnFacebookImageRequestCallback(void* pData, HTTP::Result eResult, HTTP::Response* pResponse)
{
	SEOUL_ASSERT(IsMainThread());

	String const sData(*((String*)pData));
	SEOUL_DELETE (String*)pData;
	pData = nullptr;

	OnFacebookImageRequest(sData, eResult, pResponse);

	return HTTP::CallbackResult::kSuccess;
}

void FacebookImageManager::OnFacebookImageRequest(const String& sFacebookuserGuid, HTTP::Result eResult, HTTP::Response *pResponse)
{
	SEOUL_ASSERT(IsMainThread());

	Bool bError = false;

	// Get the body of the data which has the image data we want in Jpeg format
	void const* pImageBuffer = pResponse->GetBody();
	UInt32 const uByteSize = pResponse->GetBodySize();

	// If there was any problems then we just early out
	if (eResult != HTTP::Result::kSuccess)
	{
		bError = true;
	}

	// TODO: check the facebook docs for what status codes are success and failure.
	if (pResponse->GetStatus() < 200 || pResponse->GetStatus() >= 300)
	{
		bError = true;
	}

	if (nullptr == pImageBuffer || 0u == uByteSize)
	{
		bError = true;
	}

	if (FacebookImageManager::Get())
	{
		FacebookImageManager::Get()->ProcessFacebookImageRequest(sFacebookuserGuid, pImageBuffer, uByteSize, bError);
	}
}

void FacebookImageManager::ProcessFacebookImageRequest(
	const String& sFacebookUserGuid,
	void const* pImageBuffer,
	UInt32 uImageSize,
	Bool bErrorFlag)
{
	SEOUL_ASSERT(IsMainThread());

	{
		Lock lock(m_TableMutex);

		// Get the hash table from the singleton
		FacebookImageInfo* pImageInfo = m_tFaceImageTable.Find(sFacebookUserGuid);
		SEOUL_ASSERT(nullptr != pImageInfo);

		// Handling for Ship.
		if (nullptr == pImageInfo)
		{
			return;
		}

		// If there was an error with the request (change the state)
		if (bErrorFlag)
		{
			// Set the image info to a failed state
			pImageInfo->m_eState = FacebookImageState::kFailed;
			pImageInfo->m_FilePath = FilePath();

			return;
		}
	}

	// Copy the contents that we want inta a raw image buffer
	RawFacebookImage data;
	data.AllocateAndCopy((UInt8*)pImageBuffer, uImageSize);

	// Send it off for processing (converting from jpeg to seoul engine format)

	// Take the raw contents of the facebooks raw image
	// and create the job that takes that raw image data
	// which then converts it to engine format (always JPEG)
	SharedPtr<FacebookImageConverterJob> pFacebookImageConverterJob(SEOUL_NEW(MemoryBudgets::TBD) FacebookImageConverterJob(
		sFacebookUserGuid,
		data));

	// Start the save job.
	pFacebookImageConverterJob->StartJob();
}

void FacebookImageManager::UpdateImageInfoProcess(const String& sGuid, const FacebookImageInfo& info)
{
	Lock lock(m_TableMutex);

	Bool bAtMaxLimit = false;

	// Increment the number of active files in the cache
	if (m_uNumberOfFiles < m_FacebookImageConfig.m_uMaxNumberOfFiles)
	{
		++m_uNumberOfFiles;
	}
	else
	{
		bAtMaxLimit = true;
	}

	// Get the image info the the hash table
	SEOUL_VERIFY(m_tFaceImageTable.Overwrite(sGuid, info).Second);

	// If we go over our boundry size kick off a remove job that will
	// delete old files on the IO thread (only one run at a time)
	if (bAtMaxLimit)
	{
		if (!m_pRemoveFilesJob.IsValid() || !m_pRemoveFilesJob->IsJobRunning())
		{
			m_pRemoveFilesJob = Jobs::MakeFunction(GetFileIOThreadId(), SEOUL_BIND_DELEGATE(&FacebookImageManager::RemoveOldFiles, this));
			m_pRemoveFilesJob->StartJob();
		}
	}
}

void FacebookImageManager::LoadFacebookImages()
{
	// Output for all the files inside a directory
	Vector<String> vFileResults;

	// Create the absolute path for our facebook image folder
	String const sFileNamePath = Path::Combine(GamePaths::Get()->GetSaveDir(), s_kFacebookProfileImages);

	// Get all the files withnin this directory
	FileManager::Get()->GetDirectoryListing(sFileNamePath, vFileResults);

	// This file contains all the saved facebook profile image
	// that was saved last time the application was ranned.
	// We need to go through and load all the files up front when the application
	// starts up
	for (auto fileIter = vFileResults.Begin(); fileIter != vFileResults.End(); ++fileIter)
	{
		// Extract the extension
		String const sExt(Path::GetExtension(*fileIter));

		if (sExt == ".sif0")
		{
			// Create a file path with the name
			FilePath const path = FilePath::CreateSaveFilePath(*fileIter);

			// check if fail
			if (!path.IsValid())
			{
				continue;
			}

			//extract the facebook user guid
			String const sFacebookGuid(Path::GetFileNameWithoutExtension(*fileIter));
			FacebookImageInfo info(FacebookImageState::kSuccess, path);

			/*
			* insert the loaded profile image into
			* our hash map so we can use it to render
			* no need to lock here because its on startup and
			* there would be no jobs of conversion running while we are in this function.
			*/
			SEOUL_VERIFY(m_tFaceImageTable.Insert(sFacebookGuid, info).Second);

			//add to the number of facebook image files (does not include all the mip levels)
			m_uNumberOfFiles++;
		}
	}
}

void FacebookImageManager::RemoveOldFiles() const
{
	//output for all the files inside a directory
	Vector<String> vFileResults;
	Vector<FileNameAndTime> vFilePathAndTimeStamp;

	//create the absolute path for our facebook image folder
	String const sFileNamePath = Path::Combine(GamePaths::Get()->GetSaveDir(), s_kFacebookProfileImages);

	//get all the files withnin this directory
	(void)FileManager::Get()->GetDirectoryListing(sFileNamePath, vFileResults);

	//calc how many files we need to delete (excluding additional mips)
	Int32 const iMipCount = (Int32)FileType::LAST_TEXTURE_TYPE - (Int32)FileType::FIRST_TEXTURE_TYPE + 1;
	Int32 const iDeleteSize =  ((Int32)vFileResults.GetSize() / iMipCount) - (Int32)m_FacebookImageConfig.m_uMaxNumberOfFiles;

	//insanity check (early bail)
	if (iDeleteSize <= 0)
	{
		return;
	}

	// Go through all the files in the folder directory
	// and find the one with the oldest modified time (meaning oldest
	// in the folder)
	for (auto fileIter = vFileResults.Begin(); fileIter != vFileResults.End(); ++fileIter)
	{
		// Extract the extension
		String const sExt(Path::GetExtension(*fileIter));

		if (sExt == ".sif0")
		{
			// Get the modified time for this file so we can compare
			UInt64 const uModifiedTime(FileManager::Get()->GetModifiedTime(*fileIter));

			FileNameAndTime copy;
			copy.First = *fileIter;
			copy.Second = uModifiedTime;
			vFilePathAndTimeStamp.PushBack(copy);
		}
	}

	FileTimeStampSorter stampSorter;

	// Sort all the files with time stampss
	QuickSort(vFilePathAndTimeStamp.Begin(), vFilePathAndTimeStamp.End(), stampSorter);

	WorldTime const time = WorldTime::GetUTCTime();

	// Go through depending on delete size and delete the oldest file
	// if they pass a certain age limit
	for (Int32 i = 0; !vFilePathAndTimeStamp.IsEmpty() && i < iDeleteSize; ++i)
	{
		// Get the oldest one
		FileNameAndTime const fileNameAndTime = vFilePathAndTimeStamp.Back();

		// Calc the age of the file
		Int64 const iAge = (time.GetSeconds() - (Int64)fileNameAndTime.Second);

		// If the files is old enough
		// we can send it off to its death
		// marked for deletion
		if (iAge > m_FacebookImageConfig.m_iFileAgeLimitInSeconds)
		{
			// Go through and delete the files including all the generated mip levels
			for (Int32 i = 0; i < iMipCount; ++i)
			{
				String const sExt = String::Printf(".sif%d", i);
				String const sFileName = Path::ReplaceExtension(fileNameAndTime.First, sExt);

				// Delete the files
				(void)FileManager::Get()->Delete(sFileName);
			}

		}

		// Remove from list
		vFilePathAndTimeStamp.PopBack();
	}
}

} // namespace Seoul
