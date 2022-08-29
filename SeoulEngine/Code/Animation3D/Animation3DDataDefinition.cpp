/**
 * \file Animation3DDataDefinition.cpp
 * \brief Serializable animation data. Includes skeleton and bones,
 * and animation clip data. This is read-only data at runtime.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Animation3DClipDefinition.h"
#include "Animation3DContentLoader.h"
#include "Animation3DDataDefinition.h"
#include "ContentLoadManager.h"
#include "ReflectionDefine.h"
#include "SeoulFile.h"
#include "SeoulFileReaders.h"

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul
{

namespace Animation3D
{

DataDefinition::DataDefinition()
	: m_vBones()
	, m_tBones()
	, m_tClips()
{
}

DataDefinition::~DataDefinition()
{
}

UInt32 DataDefinition::GetMemoryUsageInBytes() const
{
	UInt32 uReturn = 0u;
	{
		auto const iBegin = m_tClips.Begin();
		auto const iEnd = m_tClips.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			uReturn += i->Second->GetMemoryUsageInBytes();
		}
	}
	uReturn += m_vBones.GetCapacityInBytes();
	return uReturn;
}

Bool DataDefinition::Load(FilePath filePath, void const* pData, UInt32 uDataSizeInBytes)
{
	// Wrap the data into a sync file for reading.
	FullyBufferedSyncFile file(
		(void*)pData,
		uDataSizeInBytes,
		false);

	auto const uFileSize = file.GetSize();
	while (true)
	{
		// Get current position, terminate with failure
		// on failure to acquire.
		Int64 iGlobalOffset = 0;
		if (!file.GetCurrentPositionIndicator(iGlobalOffset) ||
			iGlobalOffset < 0 ||
			(UInt64)iGlobalOffset > uFileSize)
		{
			return false;
		}

		// If offset is at the end, break out of the loop
		if ((UInt64)iGlobalOffset == uFileSize)
		{
			break;
		}
	
		// Get the tag chunk.
		Int32 iTag = 0;
		if (!ReadInt32(file, iTag)) { return false; }
		UInt32 uSizeInBytes = 0u;
		if (!ReadUInt32(file, uSizeInBytes)) { return false; }

		// Wrap the chunk in an inner file buffer.
		Int64 iOffset = 0;
		SEOUL_VERIFY(file.GetCurrentPositionIndicator(iOffset));
		FullyBufferedSyncFile innerFile(
			(void*)(((Byte const*)pData) + (size_t)iOffset),
			uSizeInBytes,
			false);

		// Advance the outer file passed the tag chunk.
		file.Seek((Int64)uSizeInBytes, File::kSeekFromCurrent);

		switch (iTag)
		{
		case DataTypeAnimationClip: // fall-through
			{
				if (!VerifyDelimiter(DataTypeAnimationClip, innerFile)) { return false; }

				HString id;
				if (!ReadHString(innerFile, id)) { return false; }

				SharedPtr<ClipDefinition> pClip(SEOUL_NEW(MemoryBudgets::Rendering) ClipDefinition);
				if (!pClip->Load(innerFile))
				{
					return false;
				}

				m_tClips.Insert(id, pClip);
			}
			break;
		case DataTypeAnimationSkeleton:
			{
				if (!VerifyDelimiter(DataTypeAnimationSkeleton, innerFile)) { return false; }

				UInt32 uBones = 0u;
				if (!ReadUInt32(innerFile, uBones)) { return false; }

				Bones vBones;
				vBones.Resize(uBones);
				for (UInt32 i = 0u; i < uBones; ++i)
				{
					auto& r = vBones[i];

					if (!ReadHString(innerFile, r.m_Id)) { return false; }
					if (!ReadHString(innerFile, r.m_ParentId)) { return false; }
					if (!ReadQuaternion(innerFile, r.m_qRotation)) { return false; }
					if (!ReadVector3D(innerFile, r.m_vPosition)) { return false; }
					if (!ReadVector3D(innerFile, r.m_vScale)) { return false; }
				}

				m_vBones.Swap(vBones);
			}
			break;

			// Intentionally skipped, handled in Asset
			// in the Engine project.
		case DataTypeMaterialLibrary: // fall-through
		case DataTypeMesh:
			break;

		default:
			return false;
		};
	}

	return FinalizeBones();
}

Bool DataDefinition::FinalizeBones()
{
	m_tBones.Clear();

	UInt32 const u = m_vBones.GetSize();
	for (UInt32 i = 0u; i < u; ++i)
	{
		auto const& bone = m_vBones[i];
		SEOUL_VERIFY(m_tBones.Insert(bone.m_Id, (Int16)i).Second);
	}

	for (UInt32 i = 0u; i < u; ++i)
	{
		auto& bone = m_vBones[i];
		if (!bone.m_ParentId.IsEmpty())
		{
			SEOUL_VERIFY(m_tBones.GetValue(bone.m_ParentId, bone.m_iParent));

			// Sanity check.
			if (bone.m_iParent >= (Int32)i)
			{
				return false;
			}
		}
	}

	return true;
}

} // namespace Animation3D

SEOUL_TYPE(Animation3DDataContentHandle)

SharedPtr<Animation3D::DataDefinition> Content::Traits<Animation3D::DataDefinition>::GetPlaceholder(FilePath filePath)
{
	return SharedPtr<Animation3D::DataDefinition>();
}

Bool Content::Traits<Animation3D::DataDefinition>::FileChange(FilePath filePath, const Animation3DDataContentHandle& hEntry)
{
	if (FileType::kSceneAsset == filePath.GetType())
	{
		Load(filePath, hEntry);
		return true;
	}

	return false;
}

void Content::Traits<Animation3D::DataDefinition>::Load(FilePath filePath, const Animation3DDataContentHandle& hEntry)
{
	Content::LoadManager::Get()->Queue(
		SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) Animation3D::ContentLoader(filePath, hEntry)));
}

Bool Content::Traits<Animation3D::DataDefinition>::PrepareDelete(FilePath filePath, Content::Entry<Animation3D::DataDefinition, KeyType>& entry)
{
	return true;
}

} // namespace Seoul

#endif // /SEOUL_WITH_ANIMATION_3D
