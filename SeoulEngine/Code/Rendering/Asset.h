/**
 * \file Asset.h
 * \brief Base class of shared scene assets, exported from content
 * creation tools.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ASSET_H
#define ASSET_H

#include "ContentHandle.h"
#include "FilePath.h"
#include "SharedPtr.h"
#include "Prereqs.h"
namespace Seoul { class MaterialLibrary; }
namespace Seoul { class Mesh; }
namespace Seoul { class SyncFile; }

namespace Seoul
{

class Asset SEOUL_SEALED
{
public:
	Asset();
	~Asset();

	const SharedPtr<MaterialLibrary>& GetMaterialLibrary() const
	{
		return m_pMaterialLibrary;
	}

	const SharedPtr<Mesh>& GetMesh() const
	{
		return m_pMesh;
	}

	UInt32 GetMemoryUsageInBytes() const;

	Bool Load(FilePath filePath, void const* pData, UInt32 uDataSizeInBytes);

private:
	SEOUL_REFERENCE_COUNTED(Asset);

	SharedPtr<MaterialLibrary> m_pMaterialLibrary;
	SharedPtr<Mesh> m_pMesh;

	SEOUL_DISABLE_COPY(Asset);
}; // class Asset
typedef Content::Handle<Asset> AssetContentHandle;

namespace Content
{

/**
 * Specialization of Content::Traits<> for Asset, allows Asset to be managed
 * as loadable content in the content system.
 */
template <>
struct Traits<Asset>
{
	typedef FilePath KeyType;
	static const Bool CanSyncLoad = false;

	static SharedPtr<Asset> GetPlaceholder(FilePath filePath);
	static Bool FileChange(FilePath filePath, const AssetContentHandle& hEntry);
	static void Load(FilePath filePath, const AssetContentHandle& hEntry);
	static Bool PrepareDelete(FilePath filePath, Entry<Asset, KeyType>& entry);
	static void SyncLoad(FilePath filePath, const AssetContentHandle& hEntry) {}
	static UInt32 GetMemoryUsage(const SharedPtr<Asset>& p) { return p->GetMemoryUsageInBytes(); }
}; // Content::Traits<Asset>

} // namespace Content

} // namespace Seoul

#endif // include guard
