/**
 * \file Effect.h
 * \brief Represents a set of shaders, grouped into passes and techniques, as well
 * as render states that control how geometry is rendered on the GPU.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EFFECT_H
#define EFFECT_H

#include "BaseGraphicsObject.h"
#include "ContentHandle.h"
#include "ContentTraits.h"
#include "EffectParameterType.h"
#include "FilePath.h"
#include "HashTable.h"
#include "UnsafeHandle.h"

namespace Seoul
{

/**
 * An effect is a collection of render states and shader code.
 *
 * Both render states and shader code is optional in an Effect.
 * Effects can be used to purely control render state and contain no
 * shaders, or they can only contain shaders and not modify any render
 * state.
 */
class Effect SEOUL_ABSTRACT : public BaseGraphicsObject
{
public:
	// Total amount of graphics memory used by this Effect in bytes.
	UInt32 GetGraphicsMemoryUsageInBytes() const
	{
		return m_zFileSizeInBytes;
	}

	/**
	 * When called, sets all texture parameters of this effect to nullptr.
	 * This should be called before any textures are
	 * unloaded to prevent dangling references on some platforms.
	 */
	virtual void UnsetAllTextures() = 0;

	/**
	 * @return The FilePath of the Effect source file from which this Effect object
	 * was created.
	 */
	FilePath GetFilePath() const
	{
		return m_FilePath;
	}

	/** @return true if this Effect contains a parameter with semantic semantic, false otherwise. */
	Bool HasParameterWithSemantic(HString semantic) const
	{
		// No parameters until we're in a state other than kDestroyed.
		if (GetState() == kDestroyed)
		{
			return false;
		}

		return m_tParametersBySemantic.HasValue(semantic);
	}

	/** @return true if this Effect contains a technique with name name, false otherwise. */
	Bool HasTechniqueWithName(HString name) const
	{
		// No parameters until we're in a state other than kDestroyed.
		if (GetState() == kDestroyed)
		{
			return false;
		}

		return m_tTechniquesByName.HasValue(name);
	}

protected:
	Effect(
		FilePath filePath,
		void* pRawEffectFileData,
		UInt32 zFileSizeInBytes);
	virtual ~Effect();
	SEOUL_REFERENCE_COUNTED_SUBCLASS(Effect);

	virtual EffectParameterType InternalGetParameterType(UnsafeHandle hHandle) const = 0;

	struct ParameterEntry
	{
		UnsafeHandle m_hHandle;
		EffectParameterType m_eType;
	};

	typedef HashTable<HString, ParameterEntry, MemoryBudgets::Rendering> ParameterTable;
	ParameterTable m_tParametersBySemantic;

	struct TechniqueEntry
	{
		UnsafeHandle m_hHandle;
		UInt32 m_uPassCount;
	};

	typedef HashTable<HString, TechniqueEntry, MemoryBudgets::Rendering> TechniqueTable;
	TechniqueTable m_tTechniquesByName;

	void InternalFreeFileData();

	friend class RenderCommandStreamBuilder;
	UnsafeHandle m_hHandle;
	void* m_pRawEffectFileData;
	UInt32 m_zFileSizeInBytes;
	FilePath m_FilePath;
}; // class Effect

namespace Content
{

/**
 * Specialization of Content::Traits<> for Effect, allows Effect to be managed
 * as loadable content in the content system.
 */
template <>
struct Traits<Effect>
{
	typedef FilePath KeyType;
	static const Bool CanSyncLoad = false;

	static SharedPtr<Effect> GetPlaceholder(FilePath filePath);
	static Bool FileChange(FilePath filePath, const Handle<Effect>& pEntry);
	static void Load(FilePath filePath, const Handle<Effect>& pEntry);
	static Bool PrepareDelete(FilePath filePath, Entry<Effect, KeyType>& entry);
	static void SyncLoad(FilePath filePath, const Handle<Effect>& pEntry) {}
	static UInt32 GetMemoryUsage(const SharedPtr<Effect>& p) { return 0u; }
}; // Content::Traits<BaseTexture>

} // namespace Content
typedef Content::Handle<Effect> EffectContentHandle;

} // namespace Seoul

#endif // include guard
