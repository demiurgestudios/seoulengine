/**
 * \file Animation3DDataDefinition.h
 * \brief Serializable animation data. Includes skeleton and bones,
 * and animation clip data. This is read-only data at runtime.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION3D_DATA_DEFINITION_H
#define ANIMATION3D_DATA_DEFINITION_H

#include "Animation3DClipDefinition.h"
#include "ContentHandle.h"
#include "HashTable.h"
#include "Quaternion.h"
#include "Vector.h"
#include "Vector3D.h"

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul
{

namespace Animation3D
{

struct BoneDefinition SEOUL_SEALED
{
	BoneDefinition()
		: m_Id()
		, m_ParentId()
		, m_vPosition(Vector3D::Zero())
		, m_qRotation(Quaternion::Identity())
		, m_vScale(Vector3D::One())
		, m_iParent(-1)
	{
	}


	HString m_Id;
	HString m_ParentId;
	Vector3D m_vPosition;
	Quaternion m_qRotation;
	Vector3D m_vScale;
	Int16 m_iParent;
}; // struct BoneDefinition

class DataDefinition SEOUL_SEALED
{
public:
	typedef Vector<BoneDefinition, MemoryBudgets::Animation3D> Bones;
	typedef HashTable<HString, SharedPtr<ClipDefinition>, MemoryBudgets::Animation3D> Clips;
	typedef HashTable<HString, Int16, MemoryBudgets::Animation3D> Lookup;

	DataDefinition();
	~DataDefinition();

	const Bones& GetBones() const { return m_vBones; }
	Int16 GetBoneIndex(HString id) const { Int16 i = -1; (void)m_tBones.GetValue(id, i); return i; }

	SharedPtr<ClipDefinition> GetClip(HString id) const { SharedPtr<ClipDefinition> p; (void)m_tClips.GetValue(id, p); return p; }

	UInt32 GetMemoryUsageInBytes() const;

	Bool Load(FilePath filePath, void const* pData, UInt32 uDataSizeInBytes);

private:
	SEOUL_REFERENCE_COUNTED(DataDefinition);

	Bool FinalizeBones();

	Bones m_vBones;
	Lookup m_tBones;
	Clips m_tClips;
	SEOUL_DISABLE_COPY(DataDefinition);
}; // class DataDefinition

} // namespace Animation3D

typedef Content::Handle<Animation3D::DataDefinition> Animation3DDataContentHandle;

namespace Content
{

/**
 * Specialization of Content::Traits<> for Animation3D::DataDefinition, allows Animation3D::DataDefinition to be managed
 * as loadable content in the content system.
 */
template <>
struct Traits<Animation3D::DataDefinition>
{
	typedef FilePath KeyType;
	static const Bool CanSyncLoad = false;

	static SharedPtr<Animation3D::DataDefinition> GetPlaceholder(FilePath filePath);
	static Bool FileChange(FilePath filePath, const Animation3DDataContentHandle& hEntry);
	static void Load(FilePath filePath, const Animation3DDataContentHandle& hEntry);
	static Bool PrepareDelete(FilePath filePath, Entry<Animation3D::DataDefinition, KeyType>& entry);
	static void SyncLoad(FilePath filePath, const Handle<Animation3D::DataDefinition>& hEntry) {}
	static UInt32 GetMemoryUsage(const SharedPtr<Animation3D::DataDefinition>& p) { return p->GetMemoryUsageInBytes(); }
}; // Content::Traits<Animation3D::DataDefinition>

} // namespace Content

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_3D

#endif // include guard
