/**
 * \file FalconTextureCache.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_TEXTURE_CACHE_H
#define FALCON_TEXTURE_CACHE_H

#include "FalconBitmapDefinition.h"
#include "FalconTextureCacheSettings.h"
#include "FalconTexturePacker.h"
#include "FalconTextChunk.h"
#include "FalconTexture.h"
#include "HashTable.h"
#include "Mutex.h"
#include "SharedPtr.h"
namespace Seoul { namespace Falcon { class RendererInterface; } }
namespace Seoul { namespace Falcon { class TextureCacheListEntry; } }
namespace Seoul { namespace Falcon { class TextureCacheTextureEntry; } }

namespace Seoul::Falcon
{

class TextureCacheList SEOUL_SEALED
{
public:
	TextureCacheList()
		: m_pHeadGlobal(nullptr)
		, m_pTailGlobal(nullptr)
		, m_pHeadPacked(nullptr)
		, m_pTailPacked(nullptr)
	{
	}

	~TextureCacheList()
	{
		// Sanity checking - TextureCache should manage our list membership
		// prior to destruction, so that the list is already empty.
		SEOUL_ASSERT(nullptr == m_pHeadGlobal);
		SEOUL_ASSERT(nullptr == m_pTailGlobal);
		SEOUL_ASSERT(nullptr == m_pHeadPacked);
		SEOUL_ASSERT(nullptr == m_pTailPacked);
	}

	TextureCacheTextureEntry* GetHeadGlobal() const
	{
		return m_pHeadGlobal;
	}

	TextureCacheTextureEntry* GetTailGlobal() const
	{
		return m_pTailGlobal;
	}

	TextureCacheListEntry* GetHeadPacked() const
	{
		return m_pHeadPacked;
	}

	TextureCacheListEntry* GetTailPacked() const
	{
		return m_pTailPacked;
	}

	void RemoveAll();

private:
	friend class TextureCacheListEntry;
	friend class TextureCacheTextureEntry;
	TextureCacheTextureEntry* m_pHeadGlobal;
	TextureCacheTextureEntry* m_pTailGlobal;
	TextureCacheListEntry* m_pHeadPacked;
	TextureCacheListEntry* m_pTailPacked;

	SEOUL_DISABLE_COPY(TextureCacheList);
}; // class TextureCacheList

class TextureCacheListEntry SEOUL_ABSTRACT
{
public:
	TextureCacheListEntry()
		: m_pNextPacked(nullptr)
		, m_pPrevPacked(nullptr)
		, m_uLastDrawFrameCount(0)
		, m_PackedNodeID(0)
		, m_bPackReady(false)
	{
	}

	virtual ~TextureCacheListEntry()
	{
		// Sanity checking - TextureCache should manage our list membership
		// prior to destruction.
		SEOUL_ASSERT(nullptr == m_pNextPacked);
		SEOUL_ASSERT(nullptr == m_pPrevPacked);
	}

	UInt32 GetLastDrawFrameCount() const
	{
		return m_uLastDrawFrameCount;
	}

	TextureCacheListEntry* GetNextPacked() const
	{
		return m_pNextPacked;
	}

	PackerTree2D::NodeID GetPackedNodeID() const
	{
		return m_PackedNodeID;
	}

	TextureCacheListEntry* GetPrevPacked() const
	{
		return m_pPrevPacked;
	}

	Bool IsPackReady() const
	{
		return m_bPackReady;
	}

	void Pack(TextureCacheList& rList, PackerTree2D::NodeID nodeID)
	{
		InsertPacked(rList.m_pHeadPacked, rList.m_pTailPacked);
		m_PackedNodeID = nodeID;
		m_bPackReady = false;
	}

	virtual void Remove(TextureCacheList& rList)
	{
		UnPack(rList);
	}

	virtual void SetPackReady(Bool b)
	{
		m_bPackReady = b;
	}

	virtual void UnPack(TextureCacheList& rList)
	{
		m_PackedNodeID = 0;
		m_bPackReady = false;
		RemovePacked(rList.m_pHeadPacked, rList.m_pTailPacked);
	}

	virtual void Use(TextureCacheList& rList, UInt32 uCurrentDrawFrameCount)
	{
		if (0 != m_PackedNodeID)
		{
			InsertPacked(rList.m_pHeadPacked, rList.m_pTailPacked);
		}
		m_uLastDrawFrameCount = uCurrentDrawFrameCount;
	}

private:
	void InsertPacked(
		TextureCacheListEntry*& rpHead,
		TextureCacheListEntry*& rpTail)
	{
		RemovePacked(rpHead, rpTail);

		m_pNextPacked = rpHead;
		if (rpHead)
		{
			rpHead->m_pPrevPacked = this;
		}
		if (!rpTail)
		{
			rpTail = this;
		}
		rpHead = this;
	}

	void RemovePacked(
		TextureCacheListEntry*& rpHead,
		TextureCacheListEntry*& rpTail)
	{
		if (m_pNextPacked)
		{
			m_pNextPacked->m_pPrevPacked = m_pPrevPacked;
		}

		if (m_pPrevPacked)
		{
			m_pPrevPacked->m_pNextPacked = m_pNextPacked;
		}

		if (rpHead == this)
		{
			rpHead = m_pNextPacked;
		}

		if (rpTail == this)
		{
			rpTail = m_pPrevPacked;
		}

		m_pNextPacked = nullptr;
		m_pPrevPacked = nullptr;
	}

	TextureCacheListEntry* m_pNextPacked;
	TextureCacheListEntry* m_pPrevPacked;
	UInt32 m_uLastDrawFrameCount;
	PackerTree2D::NodeID m_PackedNodeID;
	Bool m_bPackReady;

	SEOUL_DISABLE_COPY(TextureCacheListEntry);
}; // class TextureCacheListEntry

class TextureCacheGlyphEntry SEOUL_SEALED : public TextureCacheListEntry
{
public:
	TextureCacheGlyphEntry()
		: TextureCacheListEntry()
		, m_Glyph()
		, m_pTexture()
	{
	}

	Glyph m_Glyph;
	SharedPtr<Texture> m_pTexture;

private:
	SEOUL_DISABLE_COPY(TextureCacheGlyphEntry);
}; // class TextureCacheGlyphEntry

class TextureCacheTextureEntry SEOUL_SEALED : public TextureCacheListEntry
{
public:
	TextureCacheTextureEntry(TextureCacheList& rList)
		: m_pNextGlobal(nullptr)
		, m_pPrevGlobal(nullptr)
		, m_pOriginalTexture()
		, m_Reference()
		, m_PackedReference()
		, m_ID()
		// Assume an entry supports packing until we discover otherwise.
		, m_bSupportsPacking(true)
	{
		InsertGlobal(rList);
	}

	~TextureCacheTextureEntry()
	{
		// Sanity checking - TextureCache should manage our list membership
		// prior to destruction.
		SEOUL_ASSERT(nullptr == m_pNextGlobal);
		SEOUL_ASSERT(nullptr == m_pPrevGlobal);
	}

	TextureCacheTextureEntry* GetNextGlobal() const
	{
		return m_pNextGlobal;
	}

	TextureCacheTextureEntry* GetPrevGlobal() const
	{
		return m_pPrevGlobal;
	}

	void InsertGlobal(TextureCacheList& rList)
	{
		InsertGlobal(rList.m_pHeadGlobal, rList.m_pTailGlobal);
	}

	virtual void Remove(TextureCacheList& rList) SEOUL_OVERRIDE
	{
		TextureCacheListEntry::Remove(rList);
		RemoveGlobal(rList.m_pHeadGlobal, rList.m_pTailGlobal);
	}

	virtual void SetPackReady(Bool b) SEOUL_OVERRIDE
	{
		TextureCacheListEntry::SetPackReady(b);
		if (b) { m_Reference = m_PackedReference; }
		else { m_Reference = m_UnpackedReference; }
	}

	virtual void UnPack(TextureCacheList& rList) SEOUL_OVERRIDE
	{
		TextureCacheListEntry::UnPack(rList);
		m_Reference = m_UnpackedReference;
	}

	virtual void Use(TextureCacheList& rList, UInt32 uCurrentDrawFrameCount) SEOUL_OVERRIDE
	{
		InsertGlobal(rList);
		TextureCacheListEntry::Use(rList, uCurrentDrawFrameCount);
	}

private:
	TextureCacheTextureEntry* m_pNextGlobal;
	TextureCacheTextureEntry* m_pPrevGlobal;

public:
	SharedPtr<Texture> m_pOriginalTexture;
	TextureReference m_Reference;
	TextureReference m_PackedReference;
	TextureReference m_UnpackedReference;
	FilePath m_ID;
	Bool m_bSupportsPacking;

private:
	void InsertGlobal(
		TextureCacheTextureEntry*& rpHead,
		TextureCacheTextureEntry*& rpTail)
	{
		RemoveGlobal(rpHead, rpTail);

		m_pNextGlobal = rpHead;
		if (rpHead)
		{
			rpHead->m_pPrevGlobal = this;
		}
		if (!rpTail)
		{
			rpTail = this;
		}
		rpHead = this;
	}

	void RemoveGlobal(
		TextureCacheTextureEntry*& rpHead,
		TextureCacheTextureEntry*& rpTail)
	{
		if (m_pNextGlobal)
		{
			m_pNextGlobal->m_pPrevGlobal = m_pPrevGlobal;
		}

		if (m_pPrevGlobal)
		{
			m_pPrevGlobal->m_pNextGlobal = m_pNextGlobal;
		}

		if (rpHead == this)
		{
			rpHead = m_pNextGlobal;
		}

		if (rpTail == this)
		{
			rpTail = m_pPrevGlobal;
		}

		m_pNextGlobal = nullptr;
		m_pPrevGlobal = nullptr;
	}

	SEOUL_DISABLE_COPY(TextureCacheTextureEntry);
}; // class TextureCacheTextureEntry

inline void TextureCacheList::RemoveAll()
{
	while (m_pHeadPacked)
	{
		m_pHeadPacked->Remove(*this);
	}

	while (m_pHeadGlobal)
	{
		m_pHeadGlobal->Remove(*this);
	}
}

class TextureCache SEOUL_SEALED
{
public:
	typedef HashTable<UniChar, TextureCacheGlyphEntry*, MemoryBudgets::Falcon> GlyphsTable;
	typedef HashTable<HString, GlyphsTable*, MemoryBudgets::Falcon> Fonts;

	TextureCache(
		RendererInterface* pRendererInterface,
		const TextureCacheSettings& settings);
	~TextureCache();

	const TextureCacheList& GetList() const
	{
		return m_List;
	}

	const SharedPtr<Texture>& GetPackerTexture() const
	{
		return m_pPackerTexture;
	}

	void Destroy();

	Bool Prefetch(
		Float fRenderThreshold,
		FilePath filePath);

	void Purge();

	Bool ResolveTextureReference(
		Float fRenderThreshold,
		const FilePath& filePath,
		TextureReference& rReference,
		Bool bUsePacked)
	{
		return DoResolveTextureReference(
			fRenderThreshold,
			filePath,
			nullptr,
			0u,
			0u,
			false,
			bUsePacked,
			rReference);
	}

	Bool ResolveBitmapFilePath(Float fRenderThreshold, FilePath& rFilePath);

	TextureCacheGlyphEntry const* ResolveGlyph(
		const TextChunk& textChunk,
		GlyphsTable& rtGlyphs,
		UniChar uCodePoint);

	GlyphsTable& ResolveGlyphTable(const TextChunk& textChunk)
	{
		HString const sFontName = textChunk.m_Format.m_Font.m_pData->GetUniqueIdentifier();
		GlyphsTable* ptGlyphs = nullptr;
		if (!m_tFonts.GetValue(sFontName, ptGlyphs))
		{
			ptGlyphs = SEOUL_NEW(MemoryBudgets::Falcon) GlyphsTable;
			SEOUL_VERIFY(m_tFonts.Insert(sFontName, ptGlyphs).Second);
		}

		return *ptGlyphs;
	}

	Bool ResolveTextureReference(
		Float fRenderThreshold,
		const SharedPtr<BitmapDefinition const>& p,
		TextureReference& rReference,
		Bool bUsePacked);

	// Configure an indirect texture - this establishes
	// a key-to-FilePath association that can be accessed
	// by more than one BitmapInstance.
	void UpdateIndirectTexture(FilePathRelativeFilename name, FilePath filePath);

private:
	RendererInterface* m_pRendererInterface;
	SharedPtr<BitmapDefinition> m_pSolidFillBitmap;
	TexturePacker m_Packer;
	TextureCacheSettings m_Settings;
	SharedPtr<Texture> m_pPackerTexture;

	typedef HashTable<FilePathRelativeFilename, FilePath, MemoryBudgets::Falcon> IndirectTextureLookup;
	IndirectTextureLookup m_tIndirectTextureLookup;
	Mutex m_IndirectTextureMutex;

	typedef HashTable< FilePath, TextureCacheTextureEntry*, MemoryBudgets::Falcon > Textures;
	Textures m_tTextures;
	typedef HashTable< FilePath, TextureLoadingData, MemoryBudgets::Falcon > TextureLoadingDataTable;
	TextureLoadingDataTable m_tTextureLoadingData;
	typedef Vector< SharedPtr<Texture>, MemoryBudgets::Falcon > LoadingTextures;
	LoadingTextures m_vLoadingTextures;
	Int32 m_iTotalTextureMemoryUsageInBytes;
	Fonts m_tFonts;
	TextureCacheList m_List;

	Bool DoResolveTextureReference(
		Float fRenderThreshold,
		FilePath filePath,
		UInt8 const* pData,
		UInt32 uDataWidth,
		UInt32 uDataHeight,
		Bool bIsFullOccluder,
		Bool bCanPack,
		TextureReference& rReference);

	Bool MakeRoomInPacker();

	Bool Pack(
		const Font& font,
		UniChar uCodePoint,
		TextureCacheGlyphEntry& rGlyphEntry);

	Bool Pack(
		const SharedPtr<Texture>& pSource,
		const Rectangle2DInt& source,
		PackerTree2D::NodeID& rNodeID,
		Int32& riX,
		Int32& riY);

	Bool Prepare(TextureCacheTextureEntry& rEntry);
	void ProcessLoading();
	void PurgeTextures(UInt32 uFramesThreshold);

	TextureCacheTextureEntry* Resolve(
		const FilePath& filePath,
		UInt8 const* pData = nullptr,
		UInt32 uDataWidth = 0u,
		UInt32 uDataHeight = 0u,
		Bool bIsFullOccluder = false,
		Bool bCanPack = true);

	SEOUL_DISABLE_COPY(TextureCache);
}; // class TextureCache

} // namespace Seoul::Falcon

#endif // include guard
