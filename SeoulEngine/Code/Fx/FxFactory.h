/**
 * \file FxFactory.h
 * \brief Utility class which handles loading and playback of fx
 * by HString identifier. It is not necessary to use this class to use Fx,
 * it is provided as a convenience when you want more flexibility regarding
 * Fx lifespan.
 *
 * "Named" fx are fx for which the handle persists,
 * and the particular instance of the fx can be manipulated after the fx
 * has started. You want to use a named fx for looping fx,
 * fx that you want to stop at a specific time, or fx for which you want to update
 * the position.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_FACTORY_H
#define FX_FACTORY_H

#include "HashTable.h"
#include "ContentKey.h"
#include "DataStore.h"
#include "FixedArray.h"
#include "Fx.h"
#include "SeoulHString.h"
#include "Vector3D.h"

namespace Seoul
{

// Forward declarations
class Fx;

/**
 * Structure to identify an Fx by primary id and sub id. The sub id is optional
 * and can be empty.
 */
struct FxKey SEOUL_SEALED
{
	FxKey()
		: m_FxId()
		, m_FxSubId()
	{
	}

	FxKey(HString fxId, HString fxSubId = HString())
		: m_FxId(fxId)
		, m_FxSubId(fxSubId)
	{
	}

	/**
	 * @return The combined hash code of this FxKey's id and sub id.
	 */
	UInt32 GetHash() const
	{
		UInt32 uReturn = 0u;
		IncrementalHash(uReturn, m_FxId.GetHash());
		IncrementalHash(uReturn, m_FxSubId.GetHash());
		return uReturn;
	}

	/**
	 * @return True if this key is exactly equal to key.
	 */
	Bool operator==(const FxKey& key) const
	{
		return (
			m_FxId == key.m_FxId &&
			m_FxSubId == key.m_FxSubId);
	}

	Bool operator!=(const FxKey& key) const
	{
		return !(*this == key);
	}

	HString m_FxId;
	HString m_FxSubId;
}; // struct FxKey

inline UInt32 GetHash(const FxKey& key)
{
	return key.GetHash();
}

template <>
struct DefaultHashTableKeyTraits<FxKey>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static FxKey GetNullKey()
	{
		return FxKey();
	}

	static const Bool kCheckHashBeforeEquals = false;
};

class FxFactory SEOUL_SEALED
{
public:
	/**
	 * If these bits of the uFlags passed on a start Fx call are non-zero, then the Fx is
	 * considered mirrored.
	 */
	static const UInt32 kMirrorBits = (1 << 0) | (1 << 1) | (1 << 2);

	FxFactory();
	~FxFactory();

	// Setup this FxFactory with settings in a DataStore table,
	// contain in dataStore. WARNING: Calling this method will immediately
	// stop and release any Fx already contained in this
	// FxFactory.
	Bool Configure(
		const DataStore& dataStore,
		const DataNode& tableNode,
		Bool bAppend = false,
		FilePath configFilePath = FilePath()
	);

	// Convenience function - query the duration of a factoried FX based on its template
	// id.
	Bool GetFxDuration(HString id, Float& rfDuration);

	// Append any assets that the Fx defined by fxId (and its variations) are dependent on.
	Bool AppendAssets(HString fxId, FxAssetsVector& rvAssets) const;

	// Utility used to add additional FX to the factory beyond
	// the initial configuration.
	void AppendFx(HString fxId, FilePath filePath);

	// Kick off a one-off fx - must be a non-looping, finite fx.
	Fx* CreateFx(const FxKey& fxKey);

	// Return true if the fx in this table are still in the process
	// of being loaded.
	Bool IsLoading() const;

	// Start async loading of Fx we expect to use in the short-term
	// (if not already loading).
	void Prefetch(const FxKey& fxKey);

	// Configure whether all FX are preloaded/prefetched at
	// the time of configure.
	void SetPreloadAllFx(Bool bPreload)
	{
		m_bPreloadAll = bPreload;
	}

	// Swap the contents of this factory with another.
	void Swap(FxFactory& rFactory);

private:
	typedef HashTable<FxKey, FilePath, MemoryBudgets::Fx> FxTable;
	FxTable m_tFxLookup;
	typedef HashTable<FxKey, Fx*, MemoryBudgets::Fx> TemplateTable;
	TemplateTable m_tTemplates;
	Bool m_bPreloadAll = true;

	Fx* GetOrCreateTemplate(const FxKey& fxKey);

	SEOUL_DISABLE_COPY(FxFactory);
}; // class FxFactory

} // namespace Seoul

#endif // include guard
