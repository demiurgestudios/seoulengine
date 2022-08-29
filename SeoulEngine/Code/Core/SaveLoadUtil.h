/**
 * \file SaveLoadUtil.h
 * \brief Shared utility used for saving, converts a DataStore
 * to/from base64 encoded data with a checksum MD5.
 *
 * Implemented in Core, so it can be easily shared
 * client and server.
 *
 * This is our "wire" and cloud format. Local saves use a different
 * container format, defined in SaveLoadManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SAVE_LOAD_UTIL_H
#define SAVE_LOAD_UTIL_H

#include "SaveLoadResult.h"
namespace Seoul { class DataStore; }

namespace Seoul::SaveLoadUtil
{

/** Max size of a save data blob. */
static const UInt32 kuMaxDataSizeInBytes = (1 << 30u);

/**
 * Metadata about the save.
 *
 * Stored in local and wire formats, tracks data version
 * (not container version, the version of the data schema,
 * used to apply migrations in SaveLoadManager), as well
 * as a transaction ID range to identify the from and to
 * IDs of a delta save.
 */
struct SaveFileMetadata SEOUL_SEALED
{
	SaveFileMetadata()
		: m_sSessionGuid()
		, m_iTransactionIdMin(0)
		, m_iTransactionIdMax(0)
		, m_iVersion(0)

	{
	}

	Bool operator==(const SaveFileMetadata& b) const
	{
		return
			(m_sSessionGuid == b.m_sSessionGuid) &&
			(m_iTransactionIdMin == b.m_iTransactionIdMin) &&
			(m_iTransactionIdMax == b.m_iTransactionIdMax) &&
			(m_iVersion == b.m_iVersion);
	}

	// Return a CRC32 code for the members of this SaveFileMetadata.
	UInt32 ComputeCRC32() const;

	/**
	 * The session guid during which this save data was generated. Used
	 * by the server to disambiguate save data from multiple clients
	 * attached to the same user identifier.
	 */
	String m_sSessionGuid;

	/**
	 * This is the starting point of a delta save. It is the transaction
	 * id of the checkpoint against which a delta save must be applied
	 * to generate the final save checkpoint (identified by
	 * m_uTransactionIdMax).
	 */
	Int64 m_iTransactionIdMin;

	/**
	 * The target transaction ID. After a delta save is applied to
	 * the checkpoint (identified by m_uTransactionIdMin), the output
	 * checkpoint save is identified by m_uTransactionIdMax.
	 */
	Int64 m_iTransactionIdMax;

	/**
	 * Data/migration version of the save data.
	 */
	Int32 m_iVersion;
}; // struct SaveFileMetadata

// Converts a serialized binary blob into save data.
SaveLoadResult FromBlob(void const* pIn, UInt32 uInputSizeInBytes, SaveFileMetadata& rMetadata, DataStore& rSaveData);

// Converts base64 encoded data into save data.
SaveLoadResult FromBase64(const String& sData, SaveFileMetadata& rMetadata, DataStore& rSaveData);

// Converts save data into a serialized binary blob (with an embedded MD5 checksum).
SaveLoadResult ToBlob(const SaveFileMetadata& metadata, const DataStore& saveData, void*& rp, UInt32& ru);

// Converts save data into base64 encoded save data (with an embedded MD5 checksum).
SaveLoadResult ToBase64(const SaveFileMetadata& metadata, const DataStore& saveData, String& rsData);

#if SEOUL_UNIT_TESTS
// Unit testing only variation of ToBase64, does not verify the integrity of the DataStore prior to save.
SaveLoadResult UnitTestHook_ToBase64NoVerify(const SaveFileMetadata& metadata, const DataStore& saveData, String& rsData);
#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul::SaveLoadUtil

#endif // include guard
