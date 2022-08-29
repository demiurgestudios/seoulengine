/**
 * \file FalconFCNFile.cpp
 * \brief An FCN file exactly corresponds to a Flash SWF file,
 * with some Falcon specific extensions.
 *
 * An FCN is a cooked SWF. More or less, the structure of the FCN
 * is identical to the SWF, except the data is never GZIP compressed,
 * but rather uses ZSTD compression on the entire file.
 *
 * Further, embedded image tags never exist in an FCN file. They
 * are always replaced with an FCN extension, the "external image
 * reference".
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconBitmapDefinition.h"
#include "FalconDefinition.h"
#include "FalconEditTextDefinition.h"
#include "FalconFCNFile.h"
#include "FalconFont.h"
#include "FalconGlobalConfig.h"
#include "FalconMovieClipDefinition.h"
#include "FalconShapeDefinition.h"
#include "FalconSwfReader.h"
#include "Logger.h"
#include "Path.h"
#include "ReflectionUtil.h"

namespace Seoul::Falcon
{

// TODO: Unify with loading path.
Bool FCNFile::GetFCNFileDependencies(
	FilePath filePath,
	void const* pData,
	UInt32 uSizeInBytes,
	FCNDependencies& rv)
{
	auto const sBase(Path::GetDirectoryName(filePath.GetAbsoluteFilename()));
	SwfReader buffer((UInt8 const*)pData, uSizeInBytes);

	{
		UInt32 const uActualVersion = buffer.ReadUInt32();
		if (kFCNVersion != uActualVersion)
		{
			SEOUL_WARN("'%s' is unsupported or corrupted, expected FCN version "
				"'%u', got version '%u'",
				filePath.CStr(),
				kFCNVersion,
				uActualVersion);
			return false;
		}
	}

	{
		UInt32 const uActualSignature = buffer.ReadUInt32();
		if (kFCNSignature != uActualSignature)
		{
			SEOUL_WARN("'%s' is unsupported or corrupted, expected FCN signature "
				"'%u', got signature '%u'",
				filePath.CStr(),
				kFCNSignature,
				uActualSignature);
			return false;
		}
	}

	// Skip.
	buffer.ReadRectangle(); // Bounds rectangle.
	buffer.ReadFixed88(); // Frames per second.
	buffer.ReadUInt16(); // Root MovieClip count.

	// Population.
	FCNDependencies v;

	TagId eTagId = TagId::kEnd;
	do
	{
		typedef SwfReader::SizeType SizeType;

		UInt16 const uTagData = buffer.ReadUInt16();
		eTagId = (TagId)(((UInt32)uTagData) >> 6);
		SizeType zTagLengthInBytes = (SizeType)(((UInt32)uTagData) & 0x3F);

		// If the size is 0x3F, then there's an additional 32-bit
		// entry describing a "long" tag.
		if (zTagLengthInBytes == 0x3F)
		{
			zTagLengthInBytes = (SizeType)buffer.ReadUInt32();
		}

		// Expected offset after processing the tag.
		SizeType const zNewOffsetInBytes = (buffer.GetOffsetInBytes() + zTagLengthInBytes);

		switch (eTagId)
		{
		case TagId::kDefineExternalBitmap:
			{
				buffer.ReadUInt16(); // uDefinitionID
				auto const sFilename = buffer.ReadSizedString(); // Filename.

				// Apply.
				v.PushBack(FilePath::CreateContentFilePath(sFilename));
			}
			break;
		case TagId::kImportAssets: // fall-through
		case TagId::kImportAssets2:
			{
				auto const sFilename = buffer.ReadString();

				// Apply.
				v.PushBack(FilePath::CreateContentFilePath(Path::Combine(sBase, sFilename)));
			}
			break;
		default:
			break;
		};

		// Advance.
		buffer.SetOffsetInBytes(zNewOffsetInBytes);
	} while (TagId::kEnd != eTagId);

	// Done.
	rv.Swap(v);
	return true;
}

FCNFile::FCNFile(
	const HString& sURL,
	UInt8 const* pData,
	UInt32 uSizeInBytes)
	: m_pRootMovieClip()
	, m_tDictionary()
	, m_tExportedSymbols()
	, m_tImports()
	, m_tMainTimelineFrameLabels()
	, m_tMainTimelineSceneLabels()
	, m_LibraryReferenceCount(0)
	, m_Bounds(Rectangle::Create(0, 0, 0, 0))
	, m_BackgroundColor(RGBA::Black())
	, m_fFramesPerSeconds(0.0f)
	, m_sURL(sURL)
	, m_bOk(false)
{
	SwfReader buffer(pData, uSizeInBytes);
	m_bOk = Read(buffer);
}

FCNFile::~FCNFile()
{
	SafeDeleteTable(m_tImportSources);
}

Bool FCNFile::GetImportedDefinition(const HString& sName, SharedPtr<Definition>& rp, Bool bCheckNested) const
{
	ImportEntry entry;
	if (m_tImports.GetValue(sName, entry))
	{
		FCNLibraryAnchor* pImportFCNFile;
		if (!m_tImportSources.GetValue(entry.m_sURL, pImportFCNFile))
		{
			return false;
		}

		return pImportFCNFile->GetPtr()->GetDefinition(entry.m_uDefinitionID, rp);
	}
	else
	{
		auto const iBegin = m_tImportSources.Begin();
		auto const iEnd = m_tImportSources.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			if (i->Second->GetPtr()->GetExportedDefinition(sName, rp))
			{
				return true;
			}
		}

		// Check for definitions in imported fcn files
		if (bCheckNested)
		{
			for (auto i = iBegin; iEnd != i; ++i)
			{
				if (i->Second->GetPtr()->GetImportedDefinition(sName, rp))
				{
					return true;
				}
			}
		}

		return false;
	}
}

#if SEOUL_LOGGING_ENABLED
static String GetFriendlyName(const FCNFile& file)
{
	return Path::GetFileNameWithoutExtension(String(file.GetURL())) + ".swf";
}
#endif // /SEOUL_LOGGING_ENABLED

Bool FCNFile::Read(SwfReader& rBuffer)
{
	{
		UInt32 const uActualVersion = rBuffer.ReadUInt32();
		if (kFCNVersion != uActualVersion)
		{
			SEOUL_WARN("'%s' is unsupported or corrupted, expected FCN version "
				"'%u', got version '%u'",
				GetFriendlyName(*this).CStr(),
				kFCNVersion,
				uActualVersion);
			return false;
		}
	}

	{
		UInt32 const uActualSignature = rBuffer.ReadUInt32();
		if (kFCNSignature != uActualSignature)
		{
			SEOUL_WARN("'%s' is unsupported or corrupted, expected FCN signature "
				"'%u', got signature '%u'",
				GetFriendlyName(*this).CStr(),
				kFCNSignature,
				uActualSignature);
			return false;
		}
	}

	// Read the SWF rectangle, rate, and root sprite frame count.
	m_Bounds = rBuffer.ReadRectangle();
	m_fFramesPerSeconds = rBuffer.ReadFixed88().GetFloatValue();
	UInt16 const uRootMovieClipFrameCount = rBuffer.ReadUInt16();

	m_pRootMovieClip.Reset(SEOUL_NEW(MemoryBudgets::Falcon) MovieClipDefinition(uRootMovieClipFrameCount, 0));
	if (!ReadTags(rBuffer, m_pRootMovieClip))
	{
		return false;
	}

	// Update symbol names.
	{
		auto const iBegin = m_tExportedSymbols.Begin();
		auto const iEnd = m_tExportedSymbols.End();

		// TODO: In the cooker, automatically strip class names that are auto-generated
		// by Flash professional for timeline actions (these names will contain a '.'),
		// so we don't bother pretending that a particular definition may have
		// runtime behavior.

		SharedPtr<MovieClipDefinition> pDefinition;
		for (auto i = iBegin; iEnd != i; ++i)
		{
			if (GetDefinition(i->Second, pDefinition))
			{
				// TODO: Verify that class names which are valid can never contain a '.'
				if (0 == strchr(i->First.CStr(), '.'))
				{
					pDefinition->SetClassName(i->First);
				}
				else
				{
					pDefinition->SetClassName(HString());
				}
			}
		}
	}

	// Update with frame stops.
	{
		auto const iBegin = m_tAllSimpleActions.Begin();
		auto const iEnd = m_tAllSimpleActions.End();

		for (auto i = iBegin; iEnd != i; ++i)
		{
			UInt16 uDefinitionID = 0;

			HString const key = i->First;
			if (!m_tExportedSymbols.GetValue(key, uDefinitionID))
			{
				SEOUL_WARN("'%s' is unsupported or corrupted, simple actions "
					"exist for key '%s' but no definition exists for that key.",
					GetFriendlyName(*this).CStr(),
					key.CStr());
				return false;
			}

			SharedPtr<MovieClipDefinition> pMovieClip;
			if (!GetDefinition(uDefinitionID, pMovieClip))
			{
				SEOUL_WARN("'%s' is unsupported or corrupted, simple actions "
					"exist for key '%s' but definition ID '%d' did not resolve "
					"to a valid MovieClipDefinition.",
					GetFriendlyName(*this).CStr(),
					key.CStr(),
					(int)uDefinitionID);
				return false;
			}

			pMovieClip->m_SimpleActions = i->Second;
		}
	}

	return true;
}

Bool FCNFile::ReadTags(SwfReader& rBuffer, const SharedPtr<MovieClipDefinition>& pMovieClip)
{
	UInt32 uCurrentFrame = 0;

	TagId eTagId = TagId::kEnd;
	do
	{
		if (!ReadTag(rBuffer, pMovieClip, uCurrentFrame, eTagId))
		{
			return false;
		}
	} while (TagId::kEnd != eTagId);

	return true;
}

Bool FCNFile::ReadTag(SwfReader& rBuffer, const SharedPtr<MovieClipDefinition>& pMovieClip, UInt32& ruCurrentFrame, TagId& reTagId)
{
	typedef SwfReader::SizeType SizeType;

	UInt16 const uTagData = rBuffer.ReadUInt16();
	TagId const eTagId = (TagId)(((UInt32)uTagData) >> 6);
	SizeType zTagLengthInBytes = (SizeType)(((UInt32)uTagData) & 0x3F);

	// If the size is 0x3F, then there's an additional 32-bit
	// entry describing a "long" tag.
	if (zTagLengthInBytes == 0x3F)
	{
		zTagLengthInBytes = (SizeType)rBuffer.ReadUInt32();
	}

	// Expected offset after processing the tag.
	SizeType const zNewOffsetInBytes = (rBuffer.GetOffsetInBytes() + zTagLengthInBytes);

	switch (eTagId)
	{
	case TagId::kDefineEditText:
		{
			UInt16 const uDefinitionID = rBuffer.ReadUInt16();

			SharedPtr<EditTextDefinition> pEditTextDefinition(SEOUL_NEW(MemoryBudgets::Falcon) EditTextDefinition(uDefinitionID));
			if (!pEditTextDefinition->Read(*this, rBuffer))
			{
				return false;
			}

			if (!m_tDictionary.Insert(uDefinitionID, pEditTextDefinition).Second)
			{
				SEOUL_WARN("'%s' is unsupported or corrupted, multiple definitions "
					"with the same ID exist (EditTextDefinition ID: '%d').",
					GetFriendlyName(*this).CStr(),
					(int)uDefinitionID);
				return false;
			}
		}
		break;
	case TagId::kDefineExternalBitmap:
		{
			UInt16 const uDefinitionID = rBuffer.ReadUInt16();
			String const sFilename = rBuffer.ReadSizedString();
			UInt32 const uWidth = rBuffer.ReadUInt32();
			UInt32 const uHeight = rBuffer.ReadUInt32();
			Rectangle visibleRectangle;
			visibleRectangle.m_fLeft = (Float32)rBuffer.ReadInt32();
			visibleRectangle.m_fRight = (Float32)rBuffer.ReadInt32();
			visibleRectangle.m_fTop = (Float32)rBuffer.ReadInt32();
			visibleRectangle.m_fBottom = (Float32)rBuffer.ReadInt32();

			SharedPtr<BitmapDefinition> pBitmap(SEOUL_NEW(MemoryBudgets::Falcon) BitmapDefinition(sFilename, uWidth, uHeight, visibleRectangle, uDefinitionID));
			if (!m_tDictionary.Insert(uDefinitionID, pBitmap).Second)
			{
				SEOUL_WARN("'%s' is unsupported or corrupted, multiple definitions "
					"with the same ID exist (BitmapDefinition ID: '%d').",
					GetFriendlyName(*this).CStr(),
					(int)uDefinitionID);
				return false;
			}
		}
		break;
	case TagId::kDefineFontTrueType:
		{
			UInt16 const uDefinitionID = rBuffer.ReadUInt16();
			HString const sFontName = rBuffer.ReadSizedHString();
			Bool const bBold = rBuffer.ReadBit();
			Bool const bItalic = rBuffer.ReadBit();
			rBuffer.Align();

			Font font;
			if (g_Config.m_pGetFont(sFontName, bBold, bItalic, font))
			{
				SharedPtr<FontDefinition> pFontDefinition(SEOUL_NEW(MemoryBudgets::Falcon) FontDefinition(font, uDefinitionID));
				if (!m_tDictionary.Insert(uDefinitionID, pFontDefinition).Second)
				{
					SEOUL_WARN("'%s' is unsupported or corrupted, multiple definitions "
						"with the same ID exist (FontDefinition ID: '%d').",
						GetFriendlyName(*this).CStr(),
						(int)uDefinitionID);
					return false;
				}
			}
		}
		break;
	case TagId::kDefineScalingGrid:
		{
			UInt16 const uDefinitionID = rBuffer.ReadUInt16();
			Rectangle const rectangle = rBuffer.ReadRectangle();
			rBuffer.Align(); // align is necessary so we've consumed the entire tag.

			SharedPtr<Definition> p;
			if (!m_tDictionary.GetValue(uDefinitionID, p))
			{
				SEOUL_WARN("'%s' contains a 9-slice scaling grid that references an unknown sprite '%d'.",
					GetFriendlyName(*this).CStr(),
					(Int)uDefinitionID);
				return false;
			}

			if (DefinitionType::kMovieClip != p->GetType())
			{
				SEOUL_WARN("%s' contains a 9-slice scaling grid that targets an instance of type '%s', only "
					"MovieClip targets are supported.",
					GetFriendlyName(*this).CStr(),
					EnumToString<DefinitionType>(p->GetType()));
				return false;
			}

			SharedPtr<MovieClipDefinition> pMovieClip((MovieClipDefinition*)p.GetPtr());
			pMovieClip->SetScalingGrid(rectangle);
		}
		break;
	case TagId::kDefineSimpleActions:
		{
			UInt16 const uActions = rBuffer.ReadUInt16();
			for (UInt16 u = 0; u < uActions; ++u)
			{
				SimpleActions actions;

				HString sAction = rBuffer.ReadSizedHString();
				UInt16 const uFrameStops = rBuffer.ReadUInt16();
				for (UInt16 uFrameStop = 0; uFrameStop < uFrameStops; ++uFrameStop)
				{
					UInt16 const uFrame = rBuffer.ReadUInt16();
					SimpleActions::FrameActions* pFrameActions = actions.m_tFrameActions.Find(uFrame);
					if (nullptr == pFrameActions)
					{
						SEOUL_VERIFY(actions.m_tFrameActions.Insert(uFrame, SimpleActions::FrameActions()).Second);
						pFrameActions = actions.m_tFrameActions.Find(uFrame);
					}

					pFrameActions->m_bStop = true;
				}

				UInt16 const uEventSets = rBuffer.ReadUInt16();
				for (UInt16 uEventSet = 0; uEventSet < uEventSets; ++uEventSet)
				{
					UInt16 const uFrame = rBuffer.ReadUInt16();
					SimpleActions::FrameActions* pFrameActions = actions.m_tFrameActions.Find(uFrame);
					if (nullptr == pFrameActions)
					{
						SEOUL_VERIFY(actions.m_tFrameActions.Insert(uFrame, SimpleActions::FrameActions()).Second);
						pFrameActions = actions.m_tFrameActions.Find(uFrame);
					}

					UInt16 const uEvents = rBuffer.ReadUInt16();
					pFrameActions->m_vEvents.Resize(uEvents);
					for (UInt16 uEvent = 0; uEvent < uEvents; ++uEvent)
					{
						pFrameActions->m_vEvents[uEvent].First = rBuffer.ReadSizedHString();
					}
					for (UInt16 uEvent = 0; uEvent < uEvents; ++uEvent)
					{
						pFrameActions->m_vEvents[uEvent].Second = (SimpleActions::EventType)rBuffer.ReadUInt8();
					}
				}

				UInt16 const uVisibleChanges = rBuffer.ReadUInt16();
				for (UInt16 uVisibleChange = 0; uVisibleChange < uVisibleChanges; ++uVisibleChange)
				{
					UInt16 const uFrame = rBuffer.ReadUInt16();
					Bool const bVisible = (rBuffer.ReadUInt8() != 0);

					SimpleActions::FrameActions* pFrameActions = actions.m_tFrameActions.Find(uFrame);
					if (nullptr == pFrameActions)
					{
						SEOUL_VERIFY(actions.m_tFrameActions.Insert(uFrame, SimpleActions::FrameActions()).Second);
						pFrameActions = actions.m_tFrameActions.Find(uFrame);
					}

					pFrameActions->m_eVisibleChange = (bVisible ? SimpleActions::kSetVisibleTrue : SimpleActions::kSetVisibleFalse);
				}

				UInt16 const uPerFrameProperties = rBuffer.ReadUInt16();
				for (UInt16 uPerFrameProperty = 0; uPerFrameProperty < uPerFrameProperties; ++uPerFrameProperty)
				{
					UInt16 const uFrame = rBuffer.ReadUInt16();
					SimpleActions::FrameActions* pFrameActions = actions.m_tFrameActions.Find(uFrame);
					if (nullptr == pFrameActions)
					{
						SEOUL_VERIFY(actions.m_tFrameActions.Insert(uFrame, SimpleActions::FrameActions()).Second);
						pFrameActions = actions.m_tFrameActions.Find(uFrame);
					}

					UInt16 const uChildren = rBuffer.ReadUInt16();
					for (UInt16 uChild = 0; uChild < uChildren; ++uChild)
					{
						HString const sChildName = rBuffer.ReadSizedHString();
						SimpleActions::Properties* pProperties = &pFrameActions->m_tPerChildProperties.Insert(sChildName, SimpleActions::Properties()).First->Second;
						SEOUL_ASSERT(nullptr != pProperties);
						UInt16 const uProperties = rBuffer.ReadUInt16();

						for (UInt16 uProperty = 0; uProperty < uProperties; ++uProperty)
						{
							HString const sPropertyName = rBuffer.ReadSizedHString();
							SimpleActionValue value;
							value.m_eType = (SimpleActionValueType)rBuffer.ReadUInt8();
							switch (value.m_eType)
							{
							case SimpleActionValueType::kNumber:
								value.m_fValue = rBuffer.ReadFloat64();
								break;
							case SimpleActionValueType::kString:
								{
									HString const symbol = rBuffer.ReadSizedHString();
									value.m_hValue = symbol.GetHandleValue();
								}
								break;
							default:
								break;
							};

							SEOUL_VERIFY(pProperties->Insert(sPropertyName, value).Second);
						}
					}
				}

				if (!m_tAllSimpleActions.Insert(sAction, actions).Second)
				{
					SEOUL_WARN("'%s' is unsupported or corrupted, multiple simple actions "
						"with the same key exist (Key: '%s').",
						GetFriendlyName(*this).CStr(),
						sAction.CStr());
					return false;
				}
			}
		}
		break;
	case TagId::kDefineSceneAndFrameLabelData:
		{
			UInt32 const nSceneCount = rBuffer.ReadEncodedUInt32();
			m_tMainTimelineSceneLabels.Reserve(nSceneCount);
			for (UInt32 i = 0; i < nSceneCount; ++i)
			{
				UInt32 const uFrame = rBuffer.ReadEncodedUInt32();
				HString const sLabel = rBuffer.ReadHString();

				if (!m_tMainTimelineSceneLabels.Insert(
					uFrame,
					sLabel).Second)
				{
					SEOUL_WARN("'%s' is unsupported or corrupted, invalid "
						"main timeline scene label '%s'(%u).",
						GetFriendlyName(*this).CStr(),
						sLabel.CStr(),
						uFrame);
					return false;
				}
			}

			UInt32 const nFrameLabelCount = rBuffer.ReadEncodedUInt32();
			m_tMainTimelineFrameLabels.Reserve(nFrameLabelCount);
			for (UInt32 i = 0; i < nFrameLabelCount; ++i)
			{
				UInt32 const uFrame = rBuffer.ReadEncodedUInt32();
				HString const sLabel = rBuffer.ReadHString();

				if (!m_tMainTimelineFrameLabels.Insert(
					uFrame,
					sLabel).Second)
				{
					SEOUL_WARN("'%s' is unsupported or corrupted, invalid "
						"main timeline frame label '%s'(%u).",
						GetFriendlyName(*this).CStr(),
						sLabel.CStr(),
						uFrame);
					return false;
				}
			}
		}
		break;
	case TagId::kDefineShape:
		{
			UInt16 const uDefinitionID = rBuffer.ReadUInt16();
			Rectangle const bounds = rBuffer.ReadRectangle();

			SharedPtr<ShapeDefinition> pShape(SEOUL_NEW(MemoryBudgets::Falcon) ShapeDefinition(bounds, uDefinitionID));
			if (!pShape->Read(*this, rBuffer, 1))
			{
				return false;
			}

			if (!m_tDictionary.Insert(uDefinitionID, pShape).Second)
			{
				SEOUL_WARN("'%s' is unsupported or corrupted, multiple definitions "
					"with the same ID exist (ShapeDefinition ID: '%d').",
					GetFriendlyName(*this).CStr(),
					(int)uDefinitionID);
				return false;
			}
		}
		break;
	case TagId::kDefineShape2:
		{
			UInt16 const uDefinitionID = rBuffer.ReadUInt16();
			Rectangle const bounds = rBuffer.ReadRectangle();

			SharedPtr<ShapeDefinition> pShape(SEOUL_NEW(MemoryBudgets::Falcon) ShapeDefinition(bounds, uDefinitionID));
			if (!pShape->Read(*this, rBuffer, 2))
			{
				return false;
			}

			if (!m_tDictionary.Insert(uDefinitionID, pShape).Second)
			{
				SEOUL_WARN("'%s' is unsupported or corrupted, multiple definitions "
					"with the same ID exist (ShapeDefinition2 ID: '%d').",
					GetFriendlyName(*this).CStr(),
					(int)uDefinitionID);
				return false;
			}
		}
		break;
	case TagId::kDefineShape3:
		{
			UInt16 const uDefinitionID = rBuffer.ReadUInt16();
			Rectangle const bounds = rBuffer.ReadRectangle();

			SharedPtr<ShapeDefinition> pShape(SEOUL_NEW(MemoryBudgets::Falcon) ShapeDefinition(bounds, uDefinitionID));
			if (!pShape->Read(*this, rBuffer, 3))
			{
				return false;
			}

			if (!m_tDictionary.Insert(uDefinitionID, pShape).Second)
			{
				SEOUL_WARN("'%s' is unsupported or corrupted, multiple definitions "
					"with the same ID exist (ShapeDefinition3 ID: '%d').",
					GetFriendlyName(*this).CStr(),
					(int)uDefinitionID);
				return false;
			}
		}
		break;
	case TagId::kDefineShape4:
		{
			UInt16 const uDefinitionID = rBuffer.ReadUInt16();
			Rectangle const bounds = rBuffer.ReadRectangle();

			// TODO: Prune these ignored values out in the cooker.
			(void)rBuffer.ReadRectangle(); // edgeBounds

			rBuffer.Align();

			{
				UInt32 const uReservedBits = rBuffer.ReadUBits(5);
				if (0 != uReservedBits) // Reserved 5 bits, always 0
				{
					SEOUL_WARN("'%s' is unsupported or corrupted, invalid DefineShape4 "
						"reserved bits expected to be 0 have value '%u'.",
						GetFriendlyName(*this).CStr(),
						uReservedBits);
					return false;
				}
			}

			(void)rBuffer.ReadBit(); // bUsesFillWindingRule
			(void)rBuffer.ReadBit(); // bUsesNonScalingStrokes
			(void)rBuffer.ReadBit(); // bUsesScalingStrokes

			SharedPtr<ShapeDefinition> pShape(SEOUL_NEW(MemoryBudgets::Falcon) ShapeDefinition(bounds, uDefinitionID));
			if (!pShape->Read(*this, rBuffer, 4))
			{
				return false;
			}

			if (!m_tDictionary.Insert(uDefinitionID, pShape).Second)
			{
				SEOUL_WARN("'%s' is unsupported or corrupted, multiple definitions "
					"with the same ID exist (ShapeDefinition4 ID: '%d').",
					GetFriendlyName(*this).CStr(),
					(int)uDefinitionID);
				return false;
			}
		}
		break;
	case TagId::kDefineSprite:
		{
			UInt16 uDefinitionID = rBuffer.ReadUInt16();
			UInt16 nFrameCount = rBuffer.ReadUInt16();

			SharedPtr<MovieClipDefinition> pChild(SEOUL_NEW(MemoryBudgets::Falcon) MovieClipDefinition(nFrameCount, uDefinitionID));

			if (!m_tDictionary.Insert(uDefinitionID, pChild).Second)
			{
				SEOUL_WARN("'%s' is unsupported or corrupted, multiple definitions "
					"with the same ID exist (MovieClipDefinition ID: '%d').",
					GetFriendlyName(*this).CStr(),
					(int)uDefinitionID);
				return false;
			}

			if (!ReadTags(rBuffer, pChild))
			{
				return false;
			}
		}
		break;
	case TagId::kEnd:
		// No data, indicates end of sprite.
		break;
	case TagId::kFrameLabel:
		{
			{
				auto const sFrameLabel = rBuffer.ReadFrameLabel();
				if (!pMovieClip->AddFrameLabel((UInt16)ruCurrentFrame, sFrameLabel))
				{
					SEOUL_WARN("'%s' is unsupported or corrupted, multiple frame labels "
						"with the same frame exist '%s'(%d).",
						GetFriendlyName(*this).CStr(),
						sFrameLabel.CStr(),
						(int)ruCurrentFrame);
					return false;
				}
			}
		}
		break;
	case TagId::kImportAssets2:
		{
			HString sURL = rBuffer.ReadHString();
			{
				UInt8 const uReservedBit = rBuffer.ReadUInt8();
				if (1 != uReservedBit) // Reserved byte that must be 1
				{
					SEOUL_WARN("'%s' is unsupported or corrupted, reserved bit "
						"with expected value 1 has value '%d'.",
						GetFriendlyName(*this).CStr(),
						(int)uReservedBit);
					return false;
				}
			}

			{
				UInt8 const uReservedBit = rBuffer.ReadUInt8();
				if (0 != uReservedBit) // Reserved byte that must be 0
				{
					SEOUL_WARN("'%s' is unsupported or corrupted, reserved bit "
						"with expected value 0 has value '%d'.",
						GetFriendlyName(*this).CStr(),
						(int)uReservedBit);
					return false;
				}
			}

			// Cache the import source.
			if (!sURL.IsEmpty())
			{
				if (!m_tImportSources.HasValue(sURL))
				{
					FCNLibraryAnchor* pFCNAnchor = nullptr;
					if (!g_Config.m_pGetFCNFile(m_sURL, sURL, pFCNAnchor) ||
						nullptr == pFCNAnchor ||
						!pFCNAnchor->GetPtr()->IsOk())
					{
						SafeDelete(pFCNAnchor);

						SEOUL_WARN("'%s' has import dependency '%s' but that "
							"dependency could not be resolved, check for missing "
							"or invalid file.",
							GetFriendlyName(*this).CStr(),
							sURL.CStr());

						return false;
					}

					SEOUL_VERIFY(m_tImportSources.Insert(sURL, pFCNAnchor).Second);
				}
			}

			Int32 const nCount = (Int32)rBuffer.ReadUInt16();
			for (Int32 i = 0; i < nCount; ++i)
			{
				UInt16 const uImportedDefinitionID = rBuffer.ReadUInt16();
				HString const sImportedDefinitionName = rBuffer.ReadHString();

				ImportEntry entry;
				entry.m_sURL = sURL;
				entry.m_uDefinitionID = uImportedDefinitionID;

				// duplicate entry across files.
				if (!m_tImports.Insert(sImportedDefinitionName, entry).Second)
				{
					SEOUL_WARN("'%s' is unsupported or invalid, contains "
						"duplicate import dependency '%s'(%d).",
						GetFriendlyName(*this).CStr(),
						sURL.CStr(),
						(int)uImportedDefinitionID);

					return false;
				}
			}
		}
		break;
	case TagId::kPlaceObject2:
		if (!pMovieClip->AddDisplayListTag(TagId::kPlaceObject2, *this, rBuffer))
		{
			return false;
		}
		break;
	case TagId::kPlaceObject3:
		if (!pMovieClip->AddDisplayListTag(TagId::kPlaceObject3, *this, rBuffer))
		{
			return false;
		}
		break;
	case TagId::kRemoveObject2:
		if (!pMovieClip->AddDisplayListTag(TagId::kRemoveObject2, *this, rBuffer))
		{
			return false;
		}
		break;
	case TagId::kSetBackgroundColor:
		{
			m_BackgroundColor = rBuffer.ReadRGB();
		}
		break;
	case TagId::kShowFrame:
		{
			if (!pMovieClip->AddDisplayListTag(TagId::kShowFrame, *this, rBuffer))
			{
				return false;
			}

			++ruCurrentFrame;
		}
		break;
	case TagId::kSymbolClass:
		{
			UInt32 const nHStrings = (UInt32)rBuffer.ReadUInt16();
			for (UInt32 i = 0; i < nHStrings; ++i)
			{
				UInt16 const uDefinitionID = rBuffer.ReadUInt16();
				HString const sExportName = rBuffer.ReadHString();
				if (!m_tExportedSymbols.Insert(sExportName, uDefinitionID).Second)
				{
					SEOUL_WARN("'%s' is unsupported or invalid, contains "
						"duplicate export definition '%s'(%d).",
						GetFriendlyName(*this).CStr(),
						sExportName.CStr(),
						(int)uDefinitionID);
					return false;
				}
			}
		}
		break;
	default:
		SEOUL_WARN("'%s' is unsupported or invalid, contains "
			"unsupported SWF tag '%d'.",
			GetFriendlyName(*this).CStr(),
			(int)eTagId);
		return false;
	};

	// Cache the offset before processing the tag.
	if (rBuffer.GetOffsetInBytes() != zNewOffsetInBytes)
	{
		SEOUL_WARN("'%s' is unsupported or invalid, contains "
			"tag that was not fully processed (expected offset '%u', "
			"actual offset '%u').",
			GetFriendlyName(*this).CStr(),
			(UInt32)zNewOffsetInBytes,
			(UInt32)rBuffer.GetOffsetInBytes());
		return false;
	}

	reTagId = eTagId;
	return true;
}

Bool FCNFile::Validate() const
{
	Bool bReturn = true;

	// Symbol validation.
	{
		ValidateSymbolsSet visited;
		ValidateSymbolsTable t;
		visited.Insert(GetURL());
		bReturn = ValidateUniqueSymbols(GetURL(), visited, t) && bReturn;
	}

	// Timeline validation.
	bReturn = ValidateTimelines() && bReturn;

	return bReturn;
}

namespace
{

struct CheckEntry
{
	HString m_Name;
	DefinitionType m_eDefinitionType{};
	UInt16 m_uDefinitionID{};
};

}

/**
 * Valid, but causes headaches, so we disallow it.
 * Check that if a symbol is created with a name,
 * that it keeps that name throughout the timeline.
 */
Bool FCNFile::ValidateTimelines() const
{
	// Tracking table.
	HashTable<UInt64, CheckEntry, MemoryBudgets::Falcon> tTracking;

	// Check all definitions.
	for (auto const& pair : m_tDictionary)
	{
		// Only MovieClips have timelines.
		if (pair.Second->GetType() != DefinitionType::kMovieClip) { continue; }

		SharedPtr<MovieClipDefinition> pMovieClip((MovieClipDefinition*)pair.Second.GetPtr());

		// For error reporting.
		HString movieClipName = pMovieClip->GetClassName();

		// Walk tags, check - looking for collisions at the same depth
		// of clips with a name.
		UInt32 uFrame = 1u;
		tTracking.Clear();
		auto const& tags = pMovieClip->GetDisplayListTags();
		for (auto const& pTag : tags)
		{
			switch (pTag->GetType())
			{
			case DisplayListTagType::kAddObject:
			{
				auto const pAdd = static_cast<AddObject const*>(pTag);

				// Check for collision - add creates a new object unless
				// the existing object has the same definition id.
				auto pEntry = tTracking.Find(pAdd->GetData().m_uDepth);
				if (nullptr != pEntry &&
					pEntry->m_uDefinitionID != pAdd->GetDefinition()->GetDefinitionID() &&
					!pEntry->m_Name.IsEmpty())
				{
					// TODO: Consider types other than MovieClip?
					if (DefinitionType::kMovieClip == pEntry->m_eDefinitionType ||
						DefinitionType::kMovieClip == pAdd->GetDefinition()->GetType())
					{
						SEOUL_WARN("'%s:%s' changes named child '%s' to a new library symbol id "
							"%hu on frame %u",
							GetFriendlyName(*this).CStr(),
							movieClipName.CStr(),
							pEntry->m_Name.CStr(),
							pAdd->GetDefinition()->GetDefinitionID(),
							uFrame);
						return false;
					}
				}

				// Otherwise, add or update the entry.
				if (nullptr == pEntry)
				{
					pEntry = &tTracking.Overwrite(pAdd->GetData().m_uDepth, CheckEntry()).First->Second;

					if (pAdd->HasName())
					{
						pEntry->m_Name = pAdd->GetData().m_sName;
					}
					pEntry->m_eDefinitionType = pAdd->GetDefinition()->GetType();
					pEntry->m_uDefinitionID = pAdd->GetDefinition()->GetDefinitionID();
				}
			} break;
			case DisplayListTagType::kRemoveObject:
			{
				auto const pRemove = static_cast<RemoveObject const*>(pTag);

				// Always invalid to remove an object if it has a name.
				auto pEntry = tTracking.Find(pRemove->GetDepth());
				if (nullptr != pEntry && !pEntry->m_Name.IsEmpty())
				{
					// TODO: Consider types other than MovieClip?
					if (DefinitionType::kMovieClip == pEntry->m_eDefinitionType)
					{
						SEOUL_WARN("'%s:%s' removes named child '%s' on frame %u. "
							"This can happen unexpectedly if the child is masked and the mask "
							"is keyed.",
							GetFriendlyName(*this).CStr(),
							movieClipName.CStr(),
							pEntry->m_Name.CStr(),
							uFrame);
						return false;
					}
				}

				// Remove.
				tTracking.Erase(pRemove->GetDepth());
			} break;
			case DisplayListTagType::kShowFrame:
			{
				++uFrame;
			} break;
			case DisplayListTagType::kUpdateObject:
			{
				auto const pUpdate = static_cast<AddObject const*>(pTag);

				// Set/update name.
				if (pUpdate->HasName())
				{
					auto pEntry = tTracking.Find(pUpdate->GetData().m_uDepth);

					// Must exist or we have an error.
					if (nullptr == pEntry)
					{
						SEOUL_WARN("%s:%s has UpdateObject tag which references depth %hu, "
							"but there is no child at that depth.",
							GetFriendlyName(*this).CStr(),
							movieClipName.CStr(),
							pUpdate->GetData().m_uDepth);
						return false;
					}

					pEntry->m_Name = pUpdate->GetData().m_sName;
				}
			} break;
			default:
				break;
			};
		}
	}

	return true;
}

/**
 * Although technically valid, it leads
 * to hard to identify problems if more than
 * one resolution is available for
 * a particular import. So we validate
 * that all imports can only resolve to
 * a single result in all our library and
 * internal dependencies.
 */
Bool FCNFile::ValidateUniqueSymbols(
	HString rootUrl,
	ValidateSymbolsSet& visited,
	ValidateSymbolsTable& t) const
{
	// Tracking.
	Bool bReturn = true;

	// Validate self exports.
	auto const self = GetURL();
	for (auto const& e : m_tExportedSymbols)
	{
		// Handle some special cases that we filter and ignore.
		{
			SharedPtr<Definition> pDef;
			if (GetDefinition(e.Second, pDef))
			{
				// Filter targets of type font definition,
				// since there is no ambiguity with this type.
				if (pDef->GetType() == DefinitionType::kFont)
				{
					continue;
				}
				// Also filter references to the root of a Flash
				// file, since these are always exported but can
				// never conflict.
				else if (pDef->GetType() == DefinitionType::kMovieClip && 0u == e.Second)
				{
					continue;
				}
			}
		}

		auto const i = t.Insert(e.First, self);
		if (!i.Second && self != i.First->Second)
		{
			auto const sRoot(Path::GetFileNameWithoutExtension(String(rootUrl)) + ".swf");
			auto sA(Path::GetFileNameWithoutExtension(String(self)) + ".swf");
			auto sB(Path::GetFileNameWithoutExtension(String(i.First->Second)) + ".swf");

			// If one of the two URLs is equal to the root URL, use
			// a different message.
			if (rootUrl == self || rootUrl == i.First->Second)
			{
				if (rootUrl == i.First->Second)
				{
					Swap(sA, sB);
				}

				SEOUL_WARN("%s: validation failure, symbol '%s' has been copied "
					"from '%s' into '%s' and left as 'export for runtime sharing', "
					"it should instead be 'import for runtime sharing' in '%s'. Try "
					"running the 'Fix Sharing' command in Adobe Animate to fix this.",
					sRoot.CStr(),
					e.First.CStr(),
					sB.CStr(),
					sA.CStr(),
					sA.CStr());
			}
			else
			{
				SEOUL_WARN("%s: validation failure, symbol '%s' is exported from both "
					"'%s' and '%s'. This requires a manual fix - one file set to "
					"export and the other file set to import.",
					sRoot.CStr(),
					e.First.CStr(),
					sA.CStr(),
					sB.CStr());
			}
			bReturn = false;
		}
	}

	// Now process imports as well, including deep nesting.
	for (auto const& e : m_tImportSources)
	{
		auto const pImport(e.Second->GetPtr());

		// Don't visit a dependency that we've hit before,
		// which can happen (e.g.) if A -> C and A -> B -> C.
		if (visited.Insert(pImport->GetURL()).Second)
		{
			bReturn = pImport->ValidateUniqueSymbols(rootUrl, visited, t) && bReturn;
		}
	}

	return bReturn;
}

} // namespace Seoul::Falcon
