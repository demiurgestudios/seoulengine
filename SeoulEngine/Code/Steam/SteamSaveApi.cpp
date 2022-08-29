/**
 * \file SteamSaveApi.cpp
 * \brief Interface for using Steam cloud saving.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "SeoulFile.h"
#include "SteamPrivateApi.h"
#include "SteamSaveApi.h"
#include "StreamBuffer.h"

#if SEOUL_WITH_STEAM

namespace Seoul
{

SteamSaveApi::SteamSaveApi()
{
}

SteamSaveApi::~SteamSaveApi()
{
}

static Bool SteamLoad(const String& sRelativeFilename, StreamBuffer& rData)
{
	auto pSteamRemoteStorage = SteamRemoteStorage();
	if (nullptr != pSteamRemoteStorage)
	{
		if (pSteamRemoteStorage->FileExists(sRelativeFilename.CStr()))
		{
			auto const iFileSize = pSteamRemoteStorage->GetFileSize(sRelativeFilename.CStr());

			// Sanity check the size returned from Steam.
			if (iFileSize > 0 && (UInt32)iFileSize <= kDefaultMaxReadSize)
			{
				// Pad out the buffer to the desired size.
				StreamBuffer buffer;
				buffer.PadTo((UInt32)iFileSize);

				// Read in the data.
				if (pSteamRemoteStorage->FileRead(
					sRelativeFilename.CStr(),
					buffer.GetBuffer(),
					iFileSize))
				{
					// If successful, restore the head pointer to 0 to
					// match the behavior of StreamBuffer::Load() and
					// report success.
					buffer.SeekToOffset(0u);
					rData.Swap(buffer);

					return true;
				}
			}
		}
	}

	// If we get here for any reason, failure.
	return false;
}

SaveLoadResult::Enum SteamSaveApi::Load(FilePath filePath, StreamBuffer& rData) const
{
	// First, try to load from the Steam cloud. If this fails, try to load from
	// the standard local path.
	if (SteamLoad(filePath.GetRelativeFilename(), rData))
	{
		return SaveLoadResult::kSuccess;
	}

	// If we get here, we can't load using steam cloud, so use the base class
	// implementation.
	return GenericSaveApi::Load(filePath, rData);
}

static Bool SteamSave(const String& sRelativeFilename, const StreamBuffer& data)
{
	auto pSteamRemoteStorage = SteamRemoteStorage();
	if (nullptr != pSteamRemoteStorage)
	{
		// In ship builds, this is not expected to fail - in either case,
		// the only response we have is to save locally.
		if (pSteamRemoteStorage->FileWrite(
			sRelativeFilename.CStr(),
			data.GetBuffer(),
			data.GetTotalDataSizeInBytes()))
		{
			return true;
		}
	}

	return false;
}

SaveLoadResult::Enum SteamSaveApi::Save(FilePath filePath, const StreamBuffer& data) const
{
	// First, try to save to the Steam cloud. If this fails, save to the
	// standard local path.
	if (SteamSave(filePath.GetRelativeFilename(), data))
	{
		return SaveLoadResult::kSuccess;
	}

	// If we get here, we can't save using steam cloud, so use the base class
	// implementation.
	return GenericSaveApi::Save(filePath, data);
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_STEAM
