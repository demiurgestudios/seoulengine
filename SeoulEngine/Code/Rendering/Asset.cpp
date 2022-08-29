/**
 * \file Asset.cpp
 * \brief Base class of shared scene assets, exported from content
 * creation tools.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Asset.h"
#include "AssetContentLoader.h"
#include "ContentLoadManager.h"
#include "MaterialLibrary.h"
#include "Mesh.h"
#include "ReflectionDefine.h"
#include "SeoulFile.h"
#include "SeoulFileReaders.h"

namespace Seoul
{

Asset::Asset()
{
}

Asset::~Asset()
{
}

UInt32 Asset::GetMemoryUsageInBytes() const
{
	UInt32 uReturn = 0u;
	uReturn += (m_pMaterialLibrary.IsValid() ? m_pMaterialLibrary->GetMemoryUsageInBytes() : 0u);
	uReturn += (m_pMesh.IsValid() ? m_pMesh->GetMemoryUsageInBytes() : 0u);
	return uReturn;
}

Bool Asset::Load(FilePath filePath, void const* pData, UInt32 uDataSizeInBytes)
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

		// Advance the outer file passed the tag.
		file.Seek((Int64)uSizeInBytes, File::kSeekFromCurrent);

		switch (iTag)
		{
			// Intentionally skipped, handled in the Animation3D
			// project if enabled in the current build.
		case DataTypeAnimationClip: // fall-through
		case DataTypeAnimationSkeleton:
			break;
		case DataTypeMaterialLibrary:
			{
				SharedPtr<MaterialLibrary> pMaterialLibrary(SEOUL_NEW(MemoryBudgets::Rendering) MaterialLibrary);
				if (!pMaterialLibrary->Load(filePath, innerFile))
				{
					return false;
				}

				m_pMaterialLibrary = pMaterialLibrary;
			}
			break;
		case DataTypeMesh:
			{
				SharedPtr<Mesh> pMesh(SEOUL_NEW(MemoryBudgets::Rendering) Mesh);
				if (!pMesh->Load(filePath, innerFile))
				{
					return false;
				}

				// Cooker ensures material library will be first, so we can
				// just associate it here.
				pMesh->SetMaterialLibrary(m_pMaterialLibrary);
				m_pMesh = pMesh;
			}
			break;

		default:
			return false;
		};
	}

	return true;
}

SEOUL_TYPE(AssetContentHandle)

SharedPtr<Asset> Content::Traits<Asset>::GetPlaceholder(FilePath filePath)
{
	return SharedPtr<Asset>();
}

Bool Content::Traits<Asset>::FileChange(FilePath filePath, const AssetContentHandle& hEntry)
{
	if (FileType::kSceneAsset == filePath.GetType())
	{
		Load(filePath, hEntry);
		return true;
	}

	return false;
}

void Content::Traits<Asset>::Load(FilePath filePath, const AssetContentHandle& hEntry)
{
	Content::LoadManager::Get()->Queue(
		SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) AssetContentLoader(filePath, hEntry)));
}

Bool Content::Traits<Asset>::PrepareDelete(FilePath filePath, Content::Entry<Asset, KeyType>& entry)
{
	return true;
}

} // namespace Seoul
