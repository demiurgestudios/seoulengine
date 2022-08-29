/**
 * \file FalconBitmapDefinition.h
 * \brief Defines either on-disk or in-memory image
 * data to be cached and rendered by the Falcon
 * backend.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_BITMAP_DEFINITION_H
#define FALCON_BITMAP_DEFINITION_H

#include "FalconDefinition.h"
#include "FalconTypes.h"
#include "FilePath.h"
#include "ReflectionDeclare.h"
#include "SeoulHString.h"

namespace Seoul::Falcon
{

class BitmapDefinition SEOUL_SEALED : public Definition
{
public:
	BitmapDefinition(const FilePath& filePath, UInt32 uWidth, UInt32 uHeight, UInt16 uDefinitionID, Bool bPreload = false)
		: Definition(DefinitionType::kBitmap, uDefinitionID)
		, m_FilePath(filePath)
		, m_uHeight(uHeight)
		, m_uWidth(uWidth)
		, m_pData(nullptr)
		, m_VisibleRectangle(Rectangle::Create(0, (Float32)uWidth, 0, (Float32)uHeight))
		, m_bIsFullOccluder(false)
		, m_bCanPack(true)
		, m_bPreload(bPreload)
	{
	}

	BitmapDefinition(const String& sFilename, UInt32 uWidth, UInt32 uHeight, UInt16 uDefinitionID, Bool bPreload = false)
		: Definition(DefinitionType::kBitmap, uDefinitionID)
		, m_FilePath(FilePath::CreateContentFilePath(sFilename))
		, m_uHeight(uHeight)
		, m_uWidth(uWidth)
		, m_pData(nullptr)
		, m_VisibleRectangle(Rectangle::Create(0, (Float32)uWidth, 0, (Float32)uHeight))
		, m_bIsFullOccluder(false)
		, m_bCanPack(true)
		, m_bPreload(bPreload)
	{
	}

	BitmapDefinition(const FilePath& filePath, UInt32 uWidth, UInt32 uHeight, const Rectangle& visibleRectangle, UInt16 uDefinitionID, Bool bPreload = false)
		: Definition(DefinitionType::kBitmap, uDefinitionID)
		, m_FilePath(filePath)
		, m_uHeight(uHeight)
		, m_uWidth(uWidth)
		, m_pData(nullptr)
		, m_VisibleRectangle(visibleRectangle)
		, m_bIsFullOccluder(false)
		, m_bCanPack(true)
		, m_bPreload(bPreload)
	{
	}

	BitmapDefinition(const String& sFilename, UInt32 uWidth, UInt32 uHeight, const Rectangle& visibleRectangle,  UInt16 uDefinitionID, Bool bPreload = false)
		: Definition(DefinitionType::kBitmap, uDefinitionID)
		, m_FilePath(FilePath::CreateContentFilePath(sFilename))
		, m_uHeight(uHeight)
		, m_uWidth(uWidth)
		, m_pData(nullptr)
		, m_VisibleRectangle(visibleRectangle)
		, m_bIsFullOccluder(false)
		, m_bCanPack(true)
		, m_bPreload(bPreload)
	{
	}

	BitmapDefinition(
		FillStyleType eGradientType,
		const Gradient& gradient,
		Bool bCanPack);

	BitmapDefinition(UInt32 uWidth, UInt32 uHeight, UInt8* pData, Bool bIsFullOccluder);

	~BitmapDefinition()
	{
		if (nullptr != m_pData)
		{
			MemoryManager::Deallocate(m_pData);
			m_pData = nullptr;
		}
	}

	Bool CanPack() const
	{
		return m_bCanPack;
	}

	Bool IsVisible() const
	{
		return (
			m_VisibleRectangle.m_fRight > m_VisibleRectangle.m_fLeft &&
			m_VisibleRectangle.m_fBottom > m_VisibleRectangle.m_fTop);
	}

	Bool IsVisibleToEdges() const
	{
		return (
			0.0f == m_VisibleRectangle.m_fLeft &&
			(Float32)m_uWidth == m_VisibleRectangle.m_fRight &&
			0 == m_VisibleRectangle.m_fTop &&
			(Float32)m_uHeight == m_VisibleRectangle.m_fBottom);
	}

	UInt8 const* GetData() const
	{
		return m_pData;
	}

	const FilePath& GetFilePath() const
	{
		return m_FilePath;
	}

	UInt32 GetHeight() const
	{
		return m_uHeight;
	}

	Bool GetPreload() const
	{
		return m_bPreload;
	}

	const Rectangle& GetVisibleRectangle() const
	{
		return m_VisibleRectangle;
	}

	UInt32 GetWidth() const
	{
		return m_uWidth;
	}

	Bool HasData() const
	{
		return m_pData != nullptr;
	}

	/**
	 * @return true if the data in this BitmapDefinition
	 * is guaranteed to be fully opaque (all alpha values are
	 * 255 or high enough to occlude any other content below it).
	 *
	 * A false value only means that the data used to construct
	 * this BitmapDefinition is insufficient to determine whether
	 * or not this bitmap is a full occluder. It does *not* mean
	 * that the data is guaranteed to be a partial or no occluder.
	 *
	 * In other words, other data (e.g. the resolved texture associated
	 * with the FilePath of this BitmapData) may later resolve to
	 * a partial or full occluder.
	 */
	Bool IsFullOccluder() const
	{
		return m_bIsFullOccluder;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(BitmapDefinition);
	SEOUL_REFLECTION_FRIENDSHIP(BitmapDefinition);

	virtual void DoCreateInstance(SharedPtr<Instance>& rp) const SEOUL_OVERRIDE;

	FilePath m_FilePath;
	UInt32 m_uHeight;
	UInt32 m_uWidth;
	UInt8* m_pData;
	Rectangle m_VisibleRectangle;
	Bool m_bIsFullOccluder;
	Bool m_bCanPack;
	// TODO: Elevate to the instance instead?
	Bool m_bPreload;

	SEOUL_DISABLE_COPY(BitmapDefinition);
}; // class BitmapDefinition
template <> struct DefinitionTypeOf<BitmapDefinition> { static const DefinitionType Value = DefinitionType::kBitmap; };

} // namespace Seoul::Falcon

#endif // include guard
