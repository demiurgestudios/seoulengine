/**
 * \file FacebookImageManager.h
 * \brief Global singleton for requesting and caching
 * Facebook profile thumbnail images.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FACEBOOK_IMAGE_MANAGER_H
#define FACEBOOK_IMAGE_MANAGER_H

#include "Delegate.h"
#include "FilePath.h"
#include "HashTable.h"
#include "HTTPCommon.h"
#include "Mutex.h"
#include "Singleton.h"
namespace Seoul { class FacebookImageConverterJob; }
namespace Seoul { namespace Jobs { class Job; } }

namespace Seoul
{

/**
 * Image state of the requested
 * facebook profile bitmap. Upon request we set this
 * state to PENDING. Only once the resource is doing being processed
 * will the state be changed to success. Allowing the TextureInstance classs
 * being able to draw the desired facebook image
 */
enum class FacebookImageState
{
	kFailed = -1,
	kFailedRetry,
	kFailedNeverRetry,
	kDownloading,
	kSuccess,
	kPending
};

/**
 * Holds the current
 * state of the facebookimage
 * downloaded from facebook. Keeping
 * track of the  processing state.
 */
struct FacebookImageInfo SEOUL_SEALED
{
	FacebookImageInfo()
		: m_eState(FacebookImageState::kFailed)
		, m_FilePath()
	{
	}

	FacebookImageInfo(FacebookImageState eState, const FilePath& path)
		: m_eState(eState)
		, m_FilePath(path)
	{
	}

	FacebookImageState m_eState;
	FilePath m_FilePath;
}; // struct FacebookImageInfo

/**
 * Configuration for creatin the facebook ImageManager
 * that fills out the key values used in the manager
 * to manage all the images downloaded froom facebook
 */
struct FacebookImageManagerConfig SEOUL_SEALED
{
	FacebookImageManagerConfig()
		: m_iFileAgeLimitInSeconds(43200)
		, m_uMaxNumberOfFiles(200)
		, m_sFacebookBaseUrl("http://graph.facebook.com/")
		, m_sImageDownloadFormat("large")
	{
	}

	// bar for how old the file should be so we can delete (IN SECONDS) (default 43200 seconds = 1Day)
	Int64 m_iFileAgeLimitInSeconds;

	// Max number of allowed facebookImages files that be saved in the FacebookImage folder at a time
	UInt32 m_uMaxNumberOfFiles;

	// Base url for facebook image download request
	String m_sFacebookBaseUrl;

	// Format type for all images that we want to download from facebook (hardcoded for now)
	String m_sImageDownloadFormat;
}; // struct FacebookImageManagerConfig

/**
 * Once we receive a buffer image
 * from facebook we need to mem_copy
 * that data into RawFacebookImage so it
 * doesnt go away at the end of the scope.
 *
 * This is what is send to our Job
 * in a worker thread that handles convertion
 * of facebookImage into engine supported format
 */
class RawFacebookImage SEOUL_SEALED
{
public:
	RawFacebookImage()
		: m_pBuffer(nullptr)
		, m_uByteSize(0)
	{
	}

	RawFacebookImage(const RawFacebookImage& b)
		: m_pBuffer(b.m_pBuffer)
		, m_uByteSize(b.m_uByteSize)
	{
	}

	RawFacebookImage& operator=(const RawFacebookImage& b)
	{
		m_pBuffer = b.m_pBuffer;
		m_uByteSize = b.m_uByteSize;
		return *this;
	}

	~RawFacebookImage()
	{
	}

	// accessor functions for buffer contents
	UInt8* GetBuffer() const
	{
		return m_pBuffer;
	}

	UInt32 GetSize() const
	{
		return m_uByteSize;
	}

	// allocate/copy/delete helper functions for
	// manipulating the contents of the buffer
	void AllocateAndCopy(UInt8 const* pData, UInt32 uSize);
	void SafeDelete();

private:
	UInt8* m_pBuffer;
	UInt32 m_uByteSize;
}; // class RawFacebookImage

/**
 * This is a specialized facebook interface
 * that handles creating images related to user's facebook
 * profile.
 */
class FacebookImageManager SEOUL_SEALED : public Singleton<FacebookImageManager>
{
public:
	SEOUL_DELEGATE_TARGET(FacebookImageManager);

	FacebookImageManager();
	explicit FacebookImageManager(const FacebookImageManagerConfig& config);
	~FacebookImageManager();

	/**
	 * Every time a facebooktexture instance
	 * tries to render it will request a facebookImageInfo
	 * which holds the texture information for it to draw
	 *
	 * If the image is Failed/Pending we will return nullptr
	 * and let the facebooktexture instance decide what to render
	 * as a default
	 */
	FilePath RequestFacebookImageBitmap(const String& sFacebookUserGuid);

	/**
	 * Mutator function that allows anyone to set the current
	 * max number of files that are allowed to be saved
	 * to the local directory of facebook image manager
	 */
	void SetMaxNumberOfFiles(UInt8 uMaxNumberOfFiles)
	{
		m_FacebookImageConfig.m_uMaxNumberOfFiles = uMaxNumberOfFiles;
	}

	/**
	 * Mutator functions that allows to specify
	 * how old a file should be before we can delete it
	 * wihtout messing up the cooker. So if we create a file
	 * we dont go ahead and delete it a second later (we check
	 * for age boundry ex: 1Day)
	 */
	void SetFileAgeLimit(Int64 iAgeInSeconds)
	{
		m_FacebookImageConfig.m_iFileAgeLimitInSeconds = iAgeInSeconds;
	}

	/**
	 * Mutator functions that overites the current configuration
	 * for the facebook image manager config. Adjusting values
	 */
	void SetFacebookImageManagerConfig(const FacebookImageManagerConfig& config)
	{
		m_FacebookImageConfig = config;
	}

private:
	/**
	 * Fires off an http request to facebook
	 * asking to downloaded the users profile
	 * image
	 */
	void DoFacebookImageRequest(const String& sFacebookUserGuid);

	/**
	 * Receives a response from facebook
	 * with the image buffer in JPEG format.
	 *
	 * If all is good and no error occurred. We fire up the
	 * processing of the image
	 */
	static HTTP::CallbackResult OnFacebookImageRequestCallback(void* pData, HTTP::Result eResult, HTTP::Response* pResponse);

	static void OnFacebookImageRequest(const String& sFacebookuserGuid, HTTP::Result eResult, HTTP::Response* pResponse);

	void ProcessFacebookImageRequest(const String& sFacebookUserGuid, const void* pImageBuffer, UInt32 uImageSize, Bool bErrorFlag);

	/*
	 * This function allows the threads
	 * to update the process of a current image info
	 * by copy
	 */
	void UpdateImageInfoProcess(const String& sGuid, const FacebookImageInfo& info);

	/*
	 * functions looks through our file directory
	 * where save all facebook images and load
	 */
	void LoadFacebookImages();

	void RemoveOldFiles() const;

	/*
	 * Simple closed hash table. A facebook user guid is
	 * used for hashing and we get back a checked pointer to
	 * a facebook image info.
	 *
	 * Best Case O(1) look up
	 */
	typedef HashTable<String, FacebookImageInfo> FacebookImageTable;

	// Configuration for the facebook image manager
	FacebookImageManagerConfig m_FacebookImageConfig;

	// Max number of allowed facebookImages files that be saved in the FacebookImage folder at a time
	UInt32 m_uNumberOfFiles;

	// Best case O(1) lookup with the facebook user guid string as a hash
	FacebookImageTable m_tFaceImageTable;

	// Genereic job for removing old files
	SharedPtr<Jobs::Job> m_pRemoveFilesJob;

	// Mutex for our hash table of facebook images
	Mutex m_TableMutex;

	friend class FacebookImageConverterJob;

	SEOUL_DISABLE_COPY(FacebookImageManager);
}; // class FacebookImageManager

} // namespace Seoul

#endif  // Include guard
