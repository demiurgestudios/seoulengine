/**
 * \file Animation3DClipDefinition.cpp
 * \brief Contains a set of timelines that can be applied to
 * an Animation3DInstance, to pose its skeleton into a current state.
 * This is read-only data. To apply a ClipDefinition at runtime, you must
 * instantiate an Animation3D::ClipInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Animation3DClipDefinition.h"
#include "SeoulFileReaders.h"

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul::Animation3D
{

ClipDefinition::ClipDefinition()
	: m_tBones()
{
}

ClipDefinition::~ClipDefinition()
{
}

UInt32 ClipDefinition::GetMemoryUsageInBytes() const
{
	return m_tBones.GetMemoryUsageInBytes();
}

Bool ClipDefinition::Load(SyncFile& rFile)
{
	UInt32 uEntries = 0u;
	if (!ReadUInt32(rFile, uEntries)) { return false; }

	Bones tBones;
	tBones.Reserve(uEntries);

	for (UInt32 i = 0u; i < uEntries; ++i)
	{
		Int16 iBone = -1;
		if (!ReadInt16(rFile, iBone)) { return false; }

		auto const e = tBones.Insert(iBone, BoneKeyFrames());
		if (!e.Second) { return false; }

		auto& frames = e.First->Second;
		if (!ReadBuffer(rFile, frames.m_vRotation)) { return false; }
		if (!ReadBuffer(rFile, frames.m_vScale)) { return false; }
		if (!ReadBuffer(rFile, frames.m_vTranslation)) { return false; }
	}

	m_tBones.Swap(tBones);
	return true;
}

} // namespace Seoul::Animation3D

#endif // /#if SEOUL_WITH_ANIMATION_3D
