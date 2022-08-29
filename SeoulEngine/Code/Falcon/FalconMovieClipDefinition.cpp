/**
 * \file FalconMovieClipDefinition.cpp
 * \brief The shared, immutable data of a Falcon::MovieClipInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconFCNFile.h"
#include "FalconMovieClipDefinition.h"
#include "FalconMovieClipInstance.h"
#include "FalconSwfReader.h"
#include "Logger.h"

namespace Seoul::Falcon
{

void AddObject::Apply(AddInterface& rInterface, MovieClipInstance* pOwner, DisplayList& r) const
{
	SharedPtr<Instance> pInstance;

	// Even though AddObject is only generated when an Add is necessary,
	// checking for duplication here is necessary when the last frame wraps
	// around to frame 0.
	if (!r.GetAtDepth(m_Data.m_uDepth, pInstance) ||
		pInstance->GetDefinitionID() != m_pDefinition->GetDefinitionID())
	{
		m_pDefinition->CreateInstance(pInstance);

		r.SetAtDepth(rInterface, pOwner, m_Data.m_uDepth, pInstance);
	}
	// If an add has become an update apply (due to the wrap around
	// to frame 0), we need to reapply default state for any updated
	// state not explicitly applied.
	else
	{
		UpdateObject::DoApplyWithDefaults(pInstance, rInterface, pOwner, r);
		return;
	}

	// Perform any updates.
	if (0 != m_Data.m_uFlags)
	{
		UpdateObject::DoApply(pInstance, rInterface, pOwner, r);
	}
}

void RemoveObject::Apply(AddInterface& rInterface, MovieClipInstance* pOwner, DisplayList& r) const
{
	(void)r.RemoveAtDepth(m_uDepth);
}

void UpdateObject::Apply(AddInterface& rInterface, MovieClipInstance* pOwner, DisplayList& r) const
{
	SharedPtr<Instance> pInstance;
	if (!r.GetAtDepth(m_Data.m_uDepth, pInstance))
	{
		SEOUL_WARN("'%s': GetAtDepth() failed, either a child was removed by code from "
			"a timeline that still expects it to exist, or this MovieClip contains invalid tags, "
			"which likely indicates a Falcon bug.",
			GetPath(pOwner).CStr());
		return;
	}

	DoApply(pInstance, rInterface, pOwner, r);
}

void UpdateObject::DoApply(const SharedPtr<Instance>& pInstance, AddInterface& rInterface, MovieClipInstance* pOwner, DisplayList& r) const
{
	if (HasClipDepth())
	{
		pInstance->SetClipDepth(m_Data.m_uClipDepth);
	}

	if (HasColorTransform())
	{
		pInstance->SetColorTransform(m_Data.m_cxTransform);
		pInstance->SetAlpha(m_Data.m_fMulA);
	}

	if (HasTransform())
	{
		pInstance->SetTransform(m_Data.m_mTransform);
	}

	if (HasName())
	{
		pInstance->SetName(m_Data.m_sName);
	}

	if (HasBlendMode())
	{
		// TODO: Entry point when/if we want to support other blend modes
		// in Falcon.
		pInstance->SetBlendingFactor(BlendMode::kAdd == m_Data.m_eBlendMode ? 1.0f : 0.0f);
	}
}

void UpdateObject::DoApplyWithDefaults(const SharedPtr<Instance>& pInstance, AddInterface& rInterface, MovieClipInstance* pOwner, DisplayList& r) const
{
	pInstance->SetClipDepth(HasClipDepth() ? m_Data.m_uClipDepth : 0u);

	if (HasColorTransform())
	{
		pInstance->SetColorTransform(m_Data.m_cxTransform);
		pInstance->SetAlpha(m_Data.m_fMulA);
	}
	else
	{
		pInstance->SetColorTransform(ColorTransform::Identity());
		pInstance->SetAlpha(1.0f);
	}

	pInstance->SetTransform(HasTransform() ? m_Data.m_mTransform : Matrix2x3::Identity());
	pInstance->SetName(HasName() ? m_Data.m_sName : HString());
	pInstance->SetBlendingFactor(HasBlendMode() && BlendMode::kAdd == m_Data.m_eBlendMode ? 1.0f : 0.0f);
}

void MovieClipDefinition::DoCreateInstance(SharedPtr<Instance>& rp) const
{
	rp.Reset(SEOUL_NEW(MemoryBudgets::Falcon) MovieClipInstance(SharedPtr<MovieClipDefinition const>(this)));
}

Bool MovieClipDefinition::AddDisplayListTag(
	TagId eTagId,
	FCNFile& rFile,
	SwfReader& rBuffer)
{
	DisplayListTag* pTag = nullptr;
	switch (eTagId)
	{
	case TagId::kPlaceObject:
		SEOUL_WARN("'%s' contains unsupported PlaceObject tag, verify that "
			"publish settings are set to Flash Player 9 or higher.",
			rFile.GetURL().CStr());
		return false;

	case TagId::kPlaceObject2:
		{
			UpdateObjectData data;

			Bool const bHasClipActions = rBuffer.ReadBit();
			Bool const bHasClipDepth = rBuffer.ReadBit();
			Bool const bHasName = rBuffer.ReadBit();
			Bool const bHasRatio = rBuffer.ReadBit();
			Bool const bHasColorTransform = rBuffer.ReadBit();
			Bool const bHasMatrix = rBuffer.ReadBit();
			Bool const bHasDefinitionID = rBuffer.ReadBit();
			Bool const bHasMove = rBuffer.ReadBit();

			data.m_uDepth = rBuffer.ReadUInt16();
			UInt16 uDefinitionID = 0;

			// This actually places a new object.
			if (bHasDefinitionID)
			{
				uDefinitionID = rBuffer.ReadUInt16();
			}
			// Otherwise, this modifies an object at the existing depth.

			if (bHasMatrix)
			{
				// Shape transforms, we need to undo twips translation,
				// but scaling is correct.
				data.m_mTransform = rBuffer.ReadMatrix();
				data.m_mTransform.TX = TwipsToPixels(data.m_mTransform.TX);
				data.m_mTransform.TY = TwipsToPixels(data.m_mTransform.TY);
				data.m_uFlags |= (UInt16)UpdateObjectDataFlag::kHasTransform;
			}

			if (bHasColorTransform)
			{
				ColorTransformWithAlpha const cxTransform = rBuffer.ReadColorTransformWithAlpha();

				// Need to ignore the blend factor, since that is manipulated by a different
				// input (blend effect).
				data.m_cxTransform = cxTransform.GetTransform();
				data.m_fMulA = cxTransform.m_fMulA;
				data.m_uFlags |= (UInt16)UpdateObjectDataFlag::kHasColorTransform;
			}

			if (bHasRatio)
			{
				(void)rBuffer.ReadUInt16(); // m_uRatio
			}

			if (bHasName)
			{
				data.m_sName = rBuffer.ReadHString();
				data.m_uFlags |= (UInt16)UpdateObjectDataFlag::kHasName;
			}

			if (bHasClipDepth)
			{
				data.m_uClipDepth = rBuffer.ReadUInt16();
				data.m_uFlags |= (UInt16)UpdateObjectDataFlag::kHasClipDepth;
			}

			if (bHasClipActions)
			{
				SEOUL_WARN("'%s' contains unsupported clip actions, verify that "
					"publish settings are set to Flash Player 9 or higher.",
					rFile.GetURL().CStr());
				return false;
			}

			rBuffer.Align();

			// We need to determine if this is an add or an update - if bHasDefinitionID
			// is true, and there is a previous operation at our depth that is either a
			// remove, or an Add with a different ID, this is an add. Otherwise, it is
			// an Update.
			Bool bAdd = false;
			SharedPtr<Definition> pDefinition;
			if (bHasDefinitionID)
			{
				rFile.GetDefinition(uDefinitionID, pDefinition);
				bAdd = true;

				// If bMove is also true, always create a new instance.
				if (!bHasMove)
				{
					// Otherwise, check existing entries, see if the operation
					// keeps the same shape or not.
					Bool bFoundShowFrame = false;
					DisplayListTag** ppRemove = nullptr;
					Int32 const nCount = (Int32)m_vDisplayListTags.GetSize();
					for (Int32 i = nCount - 1; i >= 0; --i)
					{
						DisplayListTag* pOtherTag = m_vDisplayListTags[i];
						UInt16 uOtherTagDepth = 0;
						if (!pOtherTag->GetDepth(uOtherTagDepth) ||
							uOtherTagDepth != data.m_uDepth)
						{
							continue;
						}

						// Track show frame and continue.
						if (pOtherTag->GetType() == DisplayListTagType::kShowFrame)
						{
							bFoundShowFrame = true;
							continue;
						}

						if (pOtherTag->GetType() == DisplayListTagType::kAddObject)
						{
							AddObject* p = (AddObject*)pOtherTag;
							if (p->GetDefinition() == pDefinition)
							{
								// Nop the remove.
								if (nullptr != ppRemove)
								{
									SafeDelete(*ppRemove);
									*ppRemove = SEOUL_NEW(MemoryBudgets::Falcon) NoopDisplayListTag;
								}

								// Treat the add as an update, since the last
								// add in the same slot used the same definition.
								bAdd = false;
							}

							// Either way, we're done searching.
							break;
						}
						else if (pOtherTag->GetType() == DisplayListTagType::kRemoveObject)
						{
							// Flash can generate a spurious Remove/Add pair if an object
							// is a child of a mask. This causes problems if the object
							// has a name and will be used by code. We filter the Remove/Add
							// pair by tracking the remove, continuing the search, and then
							// use the bAdd = false case above to prune the remove.
							//
							// This is only safe/ok if the remove and add are in the same frame,
							// so we disallow this filtering as soon as a ShowFrame has been
							// encountered.
							if (!bFoundShowFrame &&
								nullptr == ppRemove &&
								bHasName)
							{
								// Track the remove for noop-ing and continue.
								ppRemove = m_vDisplayListTags.Get(i);
								continue;
							}

							// We're done - a remove at the same depth means that
							// this Add should be treated as an add.
							break;
						}
					}
				}
			}

			// Add operation
			if (bAdd)
			{
				// Definition must be non-null.
				if (!pDefinition.IsValid())
				{
					SEOUL_WARN("'%s' contains invalid definition ID: '%d'",
						rFile.GetURL().CStr(),
						(int)uDefinitionID);
					return false;
				}

				pTag = SEOUL_NEW(MemoryBudgets::Falcon) AddObject(data, pDefinition);
			}
			// Update operation
			else
			{
				pTag = SEOUL_NEW(MemoryBudgets::Falcon) UpdateObject(data);
			}
		}
		break;

	case TagId::kPlaceObject3:
		{
			UpdateObjectData data;

			Bool const bHasClipActions = rBuffer.ReadBit();
			Bool const bHasClipDepth = rBuffer.ReadBit();
			Bool const bHasName = rBuffer.ReadBit();
			Bool const bHasRatio = rBuffer.ReadBit();
			Bool const bHasColorTransform = rBuffer.ReadBit();
			Bool const bHasMatrix = rBuffer.ReadBit();
			Bool const bHasDefinitionID = rBuffer.ReadBit();
			Bool const bHasMove = rBuffer.ReadBit();
			if (false != rBuffer.ReadBit()) // Reserved entry that must always be 0.
			{
				SEOUL_WARN("'%s' is invalid or corrupt, expected "
					"bit value 0, got bit value 1.",
					rFile.GetURL().CStr());
				return false;
			}
			Bool const bOpaqueBackground = rBuffer.ReadBit();
			Bool const bHasVisible = rBuffer.ReadBit();
			(void)rBuffer.ReadBit(); // Bool const bHasImage =
			Bool const bHasDefinitionClassName = rBuffer.ReadBit();
			Bool const bHasCacheAsBitmap = rBuffer.ReadBit();
			Bool const bHasBlendMode = rBuffer.ReadBit();
			Bool const bHasFilterList = rBuffer.ReadBit();

			data.m_uDepth = rBuffer.ReadUInt16();

			// NOTE: Page 38, "PlaceObject3" section says to read this field "If PlaceFlagHasClassName or (PlaceFlagHasImage and PlaceFlagHasCharacter), HString",
			// however, we seem to get garbage if the second condition is true.
			HString sDefinitionClassName;
			SharedPtr<Definition> pDefinition;
			if (bHasDefinitionClassName /*|| (m_bHasImage && m_bHasDefinitionID)*/)
			{
				sDefinitionClassName = rBuffer.ReadHString();
				rFile.GetImportedDefinition(sDefinitionClassName, pDefinition);
			}

			// This actually places a new object.
			UInt16 uDefinitionID = 0;
			if (bHasDefinitionID)
			{
				uDefinitionID = rBuffer.ReadUInt16();
				rFile.GetDefinition(uDefinitionID, pDefinition);
			}
			// Otherwise, this modifies an object at the existing depth.

			if (bHasMatrix)
			{
				// Shape transforms, we need to undo twips translation,
				// but scaling is correct.
				data.m_mTransform = rBuffer.ReadMatrix();
				data.m_mTransform.TX = TwipsToPixels(data.m_mTransform.TX);
				data.m_mTransform.TY = TwipsToPixels(data.m_mTransform.TY);
				data.m_uFlags |= (UInt16)UpdateObjectDataFlag::kHasTransform;
			}

			if (bHasColorTransform)
			{
				ColorTransformWithAlpha const cxTransform = rBuffer.ReadColorTransformWithAlpha();
				data.m_cxTransform = cxTransform.GetTransform();
				data.m_fMulA = cxTransform.m_fMulA;
				data.m_uFlags |= (UInt16)UpdateObjectDataFlag::kHasColorTransform;
			}

			if (bHasRatio)
			{
				(void)rBuffer.ReadUInt16(); // m_uRatio
			}

			if (bHasName)
			{
				data.m_sName = rBuffer.ReadHString();
				data.m_uFlags |= (UInt16)UpdateObjectDataFlag::kHasName;
			}

			if (bHasClipDepth)
			{
				data.m_uClipDepth = rBuffer.ReadUInt16();
				data.m_uFlags |= (UInt16)UpdateObjectDataFlag::kHasClipDepth;
			}

			if (bHasFilterList)
			{
				SEOUL_WARN("'%s' contains unsupported filter effects, disable "
					"all filter effects before publishing.",
					rFile.GetURL().CStr());
				return false;
			}

			if (bHasBlendMode)
			{
				data.m_eBlendMode = (BlendMode)rBuffer.ReadUInt8();
				data.m_uFlags |= (UInt16)UpdateObjectDataFlag::kHasBlendMode;
			}

			if (bHasCacheAsBitmap)
			{
				rBuffer.ReadUInt8(); // m_uBitmapCache
			}

			if (bHasVisible)
			{
				(void)(rBuffer.ReadUInt8() == 0 ? false : true); // m_bVisible =
			}

			if (bOpaqueBackground)
			{
				(void)rBuffer.ReadRGBA(); // m_BackgroundColor =
			}

			if (bHasClipActions)
			{
				SEOUL_WARN("'%s' contains unsupported clip actions, verify that "
					"publish settings are set to Flash Player 9 or higher.",
					rFile.GetURL().CStr());
				return false;
			}

			rBuffer.Align();

			// We need to determine if this is an add or an update - if bHasDefinitionID
			// is true, and there is a previous operation at our depth that is either a
			// remove, or an Add with a different ID, this is an add. Otherwise, it is
			// an Update.
			Bool bAdd = false;
			if (bHasDefinitionID || bHasDefinitionClassName)
			{
				bAdd = true;

				// If bMove is also true, always create a new instance.
				if (!bHasMove)
				{
					// Otherwise, check existing entries, see if the operation
					// keeps the same shape or not.
					Int32 const nCount = (Int32)m_vDisplayListTags.GetSize();
					for (Int32 i = nCount - 1; i >= 0; --i)
					{
						DisplayListTag* pOtherTag = m_vDisplayListTags[i];
						UInt16 uOtherTagDepth = 0;
						if (!pOtherTag->GetDepth(uOtherTagDepth) ||
							uOtherTagDepth != data.m_uDepth)
						{
							continue;
						}

						if (pOtherTag->GetType() == DisplayListTagType::kAddObject)
						{
							AddObject* p = (AddObject*)pOtherTag;
							if (p->GetDefinition() == pDefinition)
							{
								// Treat the add as an update, since the last
								// add in the same slot used the same definition.
								bAdd = false;
							}

							// Either way, we're done searching.
							break;
						}
						else if (pOtherTag->GetType() == DisplayListTagType::kRemoveObject)
						{
							// We're done - a remove at the same depth means that
							// this Add should be treated as an add.
							break;
						}
					}
				}
			}

			// Add operation
			if (bAdd)
			{
				// Definition must be non-null.
				if (!pDefinition.IsValid())
				{
					if (bHasDefinitionClassName)
					{
						SEOUL_WARN("'%s' contains invalid definition name: '%s', check for missing imports.",
							rFile.GetURL().CStr(),
							sDefinitionClassName.CStr());
					}
					else
					{
						SEOUL_WARN("'%s' contains invalid definition ID: '%d'",
							rFile.GetURL().CStr(),
							(int)uDefinitionID);
					}

					return false;
				}

				pTag = SEOUL_NEW(MemoryBudgets::Falcon) AddObject(data, pDefinition);
			}
			// Update operation
			else
			{
				pTag = SEOUL_NEW(MemoryBudgets::Falcon) UpdateObject(data);
			}
		}
		break;

	case TagId::kRemoveObject:
		SEOUL_WARN("'%s' contains unsupported RemoveObject tag, verify that "
			"publish settings are set to Flash Player 9 or higher.",
			rFile.GetURL().CStr());
		return false;

	case TagId::kRemoveObject2:
		{
			UInt16 const uDepth = rBuffer.ReadUInt16();
			pTag = SEOUL_NEW(MemoryBudgets::Falcon) RemoveObject(uDepth);
		}
		break;

	case TagId::kShowFrame:
		{
			pTag = SEOUL_NEW(MemoryBudgets::Falcon) ShowFrame();
			m_vFrameOffsets.PushBack((UInt32)(m_vDisplayListTags.GetSize()));
		}
		break;

		// Other tags unsupported.
	default:
		SEOUL_WARN("'%s' contains unsupported or invalid tag: '%d'",
			rFile.GetURL().CStr(),
			(int)eTagId);
		return false;
	};

	if (!pTag)
	{
		SEOUL_WARN("'%s' contains unsupported or invalid display list.",
			rFile.GetURL().CStr());
		return false;
	}

	m_vDisplayListTags.PushBack(pTag);

	// Now generate a reverse tag:
	// - Add becomes a Remove if there was no previous Add, otherwise
	//   it becomes an Add with the settings of the previous Add.
	// - Remove becomes an Add - the Add is the last Add operation
	//   plus any updates that occur to the Remove's Depth.
	// - Update becomes an Update - the Update is the accumulation of
	//   any updates since the last Add.
	// - ShowFrame is just a ShowFrame
	{
		DisplayListTag* pReverseTag = nullptr;
		switch (pTag->GetType())
		{
		case DisplayListTagType::kAddObject:
			{
				UInt16 const uDepth = ((AddObject*)pTag)->GetData().m_uDepth;
				UpdateObjectData data;
				Int32 const iAddIndex = ReverseAccumulateUpdatesToAdd(uDepth, (Int32)m_vDisplayListTags.GetSize() - 2, data);

				// This is a remove.
				if (iAddIndex < 0)
				{
					pReverseTag = SEOUL_NEW(MemoryBudgets::Falcon) RemoveObject(uDepth);
				}
				// This is a replace, an Add of the previous object.
				else
				{
					pReverseTag = SEOUL_NEW(MemoryBudgets::Falcon) AddObject(data, ((AddObject*)m_vDisplayListTags[iAddIndex])->GetDefinition());
				}
			}
			break;
		case DisplayListTagType::kRemoveObject:
			{
				UInt16 const uDepth = ((RemoveObject*)pTag)->GetDepth();
				UpdateObjectData data;
				Int32 const iAddIndex = ReverseAccumulateUpdatesToAdd(uDepth, (Int32)m_vDisplayListTags.GetSize() - 2, data);
				if (iAddIndex < 0)
				{
					SEOUL_WARN("'%s' contains unsupported or invalid display list, "
						"failed creating reverse tag for remove operation.",
						rFile.GetURL().CStr());
					return false;
				}

				pReverseTag = SEOUL_NEW(MemoryBudgets::Falcon) AddObject(data, ((AddObject*)m_vDisplayListTags[iAddIndex])->GetDefinition());
			}
			break;
		case DisplayListTagType::kNoop:
			pReverseTag = SEOUL_NEW(MemoryBudgets::Falcon) NoopDisplayListTag;
			break;
		case DisplayListTagType::kShowFrame:
			pReverseTag = SEOUL_NEW(MemoryBudgets::Falcon) ShowFrame();
			break;
		case DisplayListTagType::kUpdateObject:
			{
				const UpdateObjectData& forwardData = ((UpdateObject*)pTag)->GetData();
				UInt16 const uDepth = forwardData.m_uDepth;
				UpdateObjectData data;
				Int32 const iAddIndex = ReverseAccumulateUpdatesToAdd(uDepth, (Int32)m_vDisplayListTags.GetSize() - 2, data);
				if (iAddIndex < 0)
				{
					SEOUL_WARN("'%s' contains unsupported or invalid display list, "
						"failed creating reverse tag for update operation.",
						rFile.GetURL().CStr());
					return false;
				}

				// For reverse update actions, we need to make sure that unset members are set to apply the defaults.
				if (forwardData.HasClipDepth() && !data.HasClipDepth())
				{
					data.SetClipDepth(0);
				}

				if (forwardData.HasColorTransform() && !data.HasColorTransform())
				{
					data.SetColorTransform(ColorTransform::Identity(), 1.0f);
				}

				if (forwardData.HasName() && !data.HasName())
				{
					data.SetName(HString());
				}

				if (forwardData.HasTransform() && !data.HasTransform())
				{
					data.SetTransform(Matrix2x3::Identity());
				}

				if (forwardData.HasBlendMode() && !data.HasBlendMode())
				{
					data.SetBlendMode(BlendMode::kNormal0);
				}

				pReverseTag = SEOUL_NEW(MemoryBudgets::Falcon) UpdateObject(data);
			}
			break;
		default:
			break;
		};

		if (nullptr == pReverseTag)
		{
			SEOUL_WARN("'%s' contains unsupported or invalid display list, "
				"failed creating reverse display list.",
				rFile.GetURL().CStr());
			return false;
		}

		m_vReverseDisplayListTags.PushBack(pReverseTag);
	}

	UInt16 uDepth = 0;
	if (pTag->GetDepth(uDepth))
	{
		m_uMaxDepth = Seoul::Max(uDepth, m_uMaxDepth);
	}

	return true;
}

void MovieClipDefinition::AccumulateUpdates(
	UInt16 uDepth,
	Int32 iStartIndex,
	Int32 iEndIndex,
	UpdateObjectData& rData) const
{
	rData = UpdateObjectData();

	for (Int32 i = iStartIndex; i <= iEndIndex; ++i)
	{
		DisplayListTag* pTag = m_vDisplayListTags[i];
		switch (pTag->GetType())
		{
		case DisplayListTagType::kAddObject:
			{
				AddObject* pAddTag = (AddObject*)pTag;
				if (pAddTag->GetData().m_uDepth == uDepth)
				{
					rData.AccumulateWith(pAddTag->GetData());
				}
			}
			break;

		case DisplayListTagType::kUpdateObject:
			{
				UpdateObject* pUpdateTag = (UpdateObject*)pTag;
				if (pUpdateTag->GetData().m_uDepth == uDepth)
				{
					rData.AccumulateWith(pUpdateTag->GetData());
				}
			}
			break;

		default:
			break;
		};
	}

	rData.m_uDepth = uDepth;
}

Int32 MovieClipDefinition::ReverseAccumulateUpdatesToAdd(
	UInt16 uDepth,
	Int32 iStartIndex,
	UpdateObjectData& rData) const
{
	// Find the add operation.
	for (Int32 i = iStartIndex; i >= 0; --i)
	{
		DisplayListTag* pTag = m_vDisplayListTags[i];
		if (pTag->GetType() == DisplayListTagType::kAddObject)
		{
			AddObject* pAddTag = ((AddObject*)pTag);
			if (pAddTag->GetData().m_uDepth == uDepth)
			{
				// Now accumulate the updates and return the index.
				AccumulateUpdates(uDepth, i, iStartIndex, rData);
				return i;
			}
		}
		// Hit a remove at the specified depth before hitting the add,
		// we're done.
		else if (pTag->GetType() == DisplayListTagType::kRemoveObject)
		{
			RemoveObject* pRemoveTag = ((RemoveObject*)pTag);
			if (pRemoveTag->GetDepth() == uDepth)
			{
				return -1;
			}
		}
	}

	return -1;
}

} // namespace Seoul::Falcon
