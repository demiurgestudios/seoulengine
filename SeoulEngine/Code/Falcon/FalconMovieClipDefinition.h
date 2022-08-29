/**
 * \file FalconMovieClipDefinition.h
 * \brief The shared, immutable data of a Falcon::MovieClipInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_MOVIE_CLIP_DEFINITION_H
#define FALCON_MOVIE_CLIP_DEFINITION_H

#include "FalconDefinition.h"
#include "FalconLabelName.h"
#include "FalconTypes.h"
#include "HashTable.h"
#include "SeoulHString.h"

namespace Seoul::Falcon
{

// forward declarations
class DisplayList;
class MovieClipInstance;
class SwfReader;

enum class DisplayListTagType
{
	kUnknown,
	kAddObject,
	kRemoveObject,
	kShowFrame,
	kUpdateObject,

	// Used to ellide tags.
	kNoop,
};

class DisplayListTag SEOUL_ABSTRACT
{
public:
	virtual ~DisplayListTag()
	{
	}

	virtual void Apply(AddInterface& rInterface, MovieClipInstance* pOwner, DisplayList& r) const = 0;

	virtual Bool GetDepth(UInt16& ruDepth) const = 0;

	DisplayListTagType GetType() const
	{
		return m_eType;
	}

protected:
	DisplayListTag(DisplayListTagType eType)
		: m_eType(eType)
	{
	}

private:
	DisplayListTagType const m_eType;

	SEOUL_DISABLE_COPY(DisplayListTag);
}; // class DisplayListTag

class NoopDisplayListTag SEOUL_SEALED : public DisplayListTag
{
public:
	NoopDisplayListTag()
		: DisplayListTag(DisplayListTagType::kNoop)
	{
	}

	virtual void Apply(AddInterface& rInterface, MovieClipInstance* pOwner, DisplayList& r) const SEOUL_OVERRIDE
	{
	}

	virtual Bool GetDepth(UInt16& ruDepth) const SEOUL_OVERRIDE
	{
		return false;
	}

private:
	SEOUL_DISABLE_COPY(NoopDisplayListTag);
}; // class NoopDisplayListTag

class RemoveObject SEOUL_SEALED : public DisplayListTag
{
public:
	RemoveObject(UInt16 uDepth)
		: DisplayListTag(DisplayListTagType::kRemoveObject)
		, m_uDepth(uDepth)
	{
	}

	virtual void Apply(AddInterface& rInterface, MovieClipInstance* pOwner, DisplayList& r) const SEOUL_OVERRIDE;
	UInt16 GetDepth() const
	{
		return m_uDepth;
	}

	virtual Bool GetDepth(UInt16& ruDepth) const SEOUL_OVERRIDE
	{
		ruDepth = m_uDepth;
		return true;
	}

private:
	UInt16 m_uDepth;

	SEOUL_DISABLE_COPY(RemoveObject);
}; // class RemoveObject

enum class UpdateObjectDataFlag : UInt16
{
	kNone = 0,
	kHasColorTransform = (1 << 0),
	kHasTransform = (1 << 1),
	kHasClipDepth = (1 << 2),
	kHasName = (1 << 3),
	kHasBlendMode = (1 << 4),
};

struct UpdateObjectData SEOUL_SEALED
{
	UpdateObjectData()
		: m_cxTransform(ColorTransform::Identity())
		, m_fMulA(1.0f)
		, m_eBlendMode(BlendMode::kNormal0)
		, m_mTransform(Matrix2x3::Identity())
		, m_uDepth(0)
		, m_uClipDepth(0)
		, m_sName()
		, m_uFlags(0)
	{
	}

	void AccumulateWith(const UpdateObjectData& data)
	{
		m_uFlags |= data.m_uFlags;

		if (data.HasColorTransform())
		{
			m_cxTransform = data.m_cxTransform;
			m_fMulA = data.m_fMulA;
		}

		if (data.HasTransform())
		{
			m_mTransform = data.m_mTransform;
		}

		if (data.HasClipDepth())
		{
			m_uClipDepth = data.m_uClipDepth;
		}

		if (data.HasName())
		{
			m_sName = data.m_sName;
		}

		if (data.HasBlendMode())
		{
			m_eBlendMode = data.m_eBlendMode;
		}
	}

	Bool HasBlendMode() const { return (0 != ((UInt16)UpdateObjectDataFlag::kHasBlendMode & m_uFlags)); }
	Bool HasClipDepth() const { return (0 != ((UInt16)UpdateObjectDataFlag::kHasClipDepth & m_uFlags)); }
	Bool HasColorTransform() const { return (0 != ((UInt16)UpdateObjectDataFlag::kHasColorTransform & m_uFlags)); }
	Bool HasTransform() const { return (0 != ((UInt16)UpdateObjectDataFlag::kHasTransform & m_uFlags)); }
	Bool HasName() const { return (0 != ((UInt16)UpdateObjectDataFlag::kHasName & m_uFlags)); }

	void SetBlendMode(BlendMode eBlendMode)
	{
		m_eBlendMode = eBlendMode;
	}

	void SetClipDepth(UInt16 uClipDepth)
	{
		m_uClipDepth = uClipDepth;
		m_uFlags |= (UInt16)UpdateObjectDataFlag::kHasClipDepth;
	}

	void SetColorTransform(const ColorTransform& cxTransform, Float fMulA)
	{
		m_cxTransform = cxTransform;
		m_fMulA = fMulA;
		m_uFlags |= (UInt16)UpdateObjectDataFlag::kHasColorTransform;
	}

	void SetName(const HString& sName)
	{
		m_sName = sName;
		m_uFlags |= (UInt16)UpdateObjectDataFlag::kHasName;
	}

	void SetTransform(const Matrix2x3& mTransform)
	{
		m_mTransform = mTransform;
		m_uFlags |= (UInt16)UpdateObjectDataFlag::kHasTransform;
	}

	ColorTransform m_cxTransform;
	Float m_fMulA;
	BlendMode m_eBlendMode;
	Matrix2x3 m_mTransform;
	UInt16 m_uDepth;
	UInt16 m_uClipDepth;
	HString m_sName;
	UInt16 m_uFlags;
}; // struct UpdateObjectData

class UpdateObject : public DisplayListTag
{
public:
	UpdateObject(const UpdateObjectData& data)
		: DisplayListTag(DisplayListTagType::kUpdateObject)
		, m_Data(data)
	{
	}

	virtual void Apply(AddInterface& rInterface, MovieClipInstance* pOwner, DisplayList& r) const SEOUL_OVERRIDE;

	const UpdateObjectData& GetData() const
	{
		return m_Data;
	}

	virtual Bool GetDepth(UInt16& ruDepth) const SEOUL_OVERRIDE
	{
		ruDepth = m_Data.m_uDepth;
		return true;
	}

	Bool HasBlendMode() const { return m_Data.HasBlendMode(); }
	Bool HasColorTransform() const { return m_Data.HasColorTransform(); }
	Bool HasTransform() const { return m_Data.HasTransform(); }
	Bool HasClipDepth() const { return m_Data.HasClipDepth(); }
	Bool HasName() const { return m_Data.HasName(); }

protected:
	UpdateObject(DisplayListTagType eType, const UpdateObjectData& data)
		: DisplayListTag(eType)
		, m_Data(data)
	{
	}

	UpdateObjectData m_Data;

	void DoApply(const SharedPtr<Instance>& pInstance, AddInterface& rInterface, MovieClipInstance* pOwner, DisplayList& r) const;
	void DoApplyWithDefaults(const SharedPtr<Instance>& pInstance, AddInterface& rInterface, MovieClipInstance* pOwner, DisplayList& r) const;

private:
	SEOUL_DISABLE_COPY(UpdateObject);
}; // class UpdateObject

class AddObject SEOUL_SEALED : public UpdateObject
{
public:
	AddObject(const UpdateObjectData& data, const SharedPtr<Definition>& pDefinition)
		: UpdateObject(DisplayListTagType::kAddObject, data)
		, m_pDefinition(pDefinition)
	{
	}

	virtual void Apply(AddInterface& rInterface, MovieClipInstance* pOwner, DisplayList& r) const SEOUL_OVERRIDE;

	const SharedPtr<Definition>& GetDefinition() const
	{
		return m_pDefinition;
	}

private:
	SharedPtr<Definition> m_pDefinition;

	SEOUL_DISABLE_COPY(AddObject);
}; // class AddObject

class ShowFrame SEOUL_SEALED : public DisplayListTag
{
public:
	ShowFrame()
		: DisplayListTag(DisplayListTagType::kShowFrame)
	{
	}

	virtual void Apply(AddInterface& rInterface, MovieClipInstance* pOwner, DisplayList& r) const SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual Bool GetDepth(UInt16& ruDepth) const SEOUL_OVERRIDE
	{
		return false;
	}

private:
	SEOUL_DISABLE_COPY(ShowFrame);
}; // class ShowFrame

class MovieClipDefinition SEOUL_SEALED : public Definition
{
public:
	MovieClipDefinition(const HString& sClassName)
		: Definition(DefinitionType::kMovieClip, 0)
		, m_ScalingGrid()
		, m_vFrameOffsets()
		, m_tFrameLabels()
		, m_vDisplayListTags()
		, m_vReverseDisplayListTags()
		, m_uFrameCount(1)
		, m_SimpleActions()
		, m_uMaxDepth(0)
		, m_bHasScalingGrid(false)
	{
		SetClassName(sClassName);
	}

	MovieClipDefinition(UInt32 uFrameCount, UInt16 uDefinitionID)
		: Definition(DefinitionType::kMovieClip, uDefinitionID)
		, m_ScalingGrid()
		, m_vFrameOffsets()
		, m_tFrameLabels()
		, m_vDisplayListTags()
		, m_vReverseDisplayListTags()
		, m_uFrameCount(uFrameCount)
		, m_SimpleActions()
		, m_sClassName()
		, m_uMaxDepth(0)
		, m_bHasScalingGrid(false)
	{
	}

	~MovieClipDefinition()
	{
		SafeDeleteVector(m_vDisplayListTags);
		SafeDeleteVector(m_vReverseDisplayListTags);
	}

	typedef Vector<UInt32, MemoryBudgets::Falcon> FrameOffsets;
	typedef HashTable<LabelName, UInt16, MemoryBudgets::Falcon> FrameLabels;
	typedef Vector<DisplayListTag*, MemoryBudgets::Falcon> DisplayListTags;

	HString GetClassName() const
	{
		return m_sClassName;
	}

	const DisplayListTags& GetDisplayListTags() const
	{
		return m_vDisplayListTags;
	}

	const DisplayListTags& GetReverseDisplayListTags() const
	{
		return m_vReverseDisplayListTags;
	}

	UInt32 GetFrameCount() const
	{
		return m_uFrameCount;
	}

	const FrameOffsets& GetFrameOffsets() const
	{
		return m_vFrameOffsets;
	}

	const FrameLabels& GetFrameLabels() const
	{
		return m_tFrameLabels;
	}

	UInt16 GetMaxDepth() const
	{
		return m_uMaxDepth;
	}

	const Rectangle& GetScalingGrid() const
	{
		return m_ScalingGrid;
	}

	const SimpleActions& GetSimpleActions() const
	{
		return m_SimpleActions;
	}

	Bool HasScalingGrid() const
	{
		return m_bHasScalingGrid;
	}

	void SetClassName(const HString& className)
	{
		// If the class name has an '$' in it then this is a placeholder class name in order to not have conflicts in Flash.
		// The actual class name in this case is what follows after the '$'.
		const Byte* s = strchr(className.CStr(), '$');
		if (s != nullptr)
		{
			UInt32 offset = (UInt32)(s - className.CStr());
			UInt32 newSize = className.GetSizeInBytes() - offset - 1;
			m_sClassName = HString(s + 1, newSize);
		}
		else
		{
			m_sClassName = HString(className);
		}
	}

	void SetScalingGrid(const Rectangle& r)
	{
		m_ScalingGrid = r;
		m_bHasScalingGrid = true;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(MovieClipDefinition);

	virtual void DoCreateInstance(SharedPtr<Instance>& rp) const SEOUL_OVERRIDE;

	friend class FCNFile;
	Bool AddDisplayListTag(
		TagId eTagId,
		FCNFile& rFile,
		SwfReader& rBuffer);
	void AccumulateUpdates(UInt16 uDepth, Int32 iStartIndex, Int32 iEndIndex, UpdateObjectData& rData) const;
	Int32 ReverseAccumulateUpdatesToAdd(UInt16 uDepth, Int32 iStartIndex, UpdateObjectData& rData) const;

	Bool AddFrameLabel(UInt16 uFrame, const LabelName& sLabel)
	{
		return m_tFrameLabels.Insert(sLabel, uFrame).Second;
	}

	Rectangle m_ScalingGrid;
	FrameOffsets m_vFrameOffsets;
	FrameLabels m_tFrameLabels;
	DisplayListTags m_vDisplayListTags;
	DisplayListTags m_vReverseDisplayListTags;
	UInt32 m_uFrameCount;
	SimpleActions m_SimpleActions;
	HString m_sClassName;
	UInt16 m_uMaxDepth;
	Bool m_bHasScalingGrid;

	SEOUL_DISABLE_COPY(MovieClipDefinition);
}; // class MovieClipDefinition

template <> struct DefinitionTypeOf<MovieClipDefinition> { static const DefinitionType Value = DefinitionType::kMovieClip; };

} // namespace Seoul::Falcon

#endif // include guard
