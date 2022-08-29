/**
 * \file Animation3DClipDefinition.h
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

#pragma once
#ifndef ANIMATION3D_CLIP_DEFINITION_H
#define ANIMATION3D_CLIP_DEFINITION_H

#include "HashTable.h"
#include "Quaternion.h"
#include "Vector.h"
#include "Vector3D.h"

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul
{

namespace Animation3D
{

// Non-virtual by design (these are simple structs used
// in great quantities, and cache usage is a critical
// consideration). Don't use BaseKeyFrame directly, always
// use the subclasses (treat BaseKeyFrame as a mixin).
struct BaseKeyFrame
{
	BaseKeyFrame()
		: m_fTime(0.0f)
	{
	}

	Float32 m_fTime;
}; // struct BaseKeyFrame
SEOUL_STATIC_ASSERT(sizeof(BaseKeyFrame) == 4);

struct KeyFrame3D SEOUL_SEALED : public BaseKeyFrame
{
	KeyFrame3D()
		: m_v(Vector3D::Zero())
	{
	}

	Vector3D m_v;
}; // struct KeyFrame3D
SEOUL_STATIC_ASSERT(sizeof(KeyFrame3D) == 16);

struct KeyFrameRotation SEOUL_SEALED : public BaseKeyFrame
{
	KeyFrameRotation()
		: m_qRotation(Quaternion::Identity())
	{
	}

	Quaternion m_qRotation;
}; // struct KeyFrameRotation
SEOUL_STATIC_ASSERT(sizeof(KeyFrameRotation) == 20);

struct KeyFrameEvent SEOUL_SEALED
{
	KeyFrameEvent()
		: m_fTime(0.0f)
		, m_f(0.0f)
		, m_i(0)
		, m_s()
		, m_Id()
	{
	}

	Float32 m_fTime;
	Float32 m_f;
	Int32 m_i;
	String m_s;
	HString m_Id;
}; // struct KeyFrameEvent

} // namespace Animation3D

template <> struct CanMemCpy<Animation3D::BaseKeyFrame> { static const Bool Value = true; };
template <> struct CanMemCpy<Animation3D::KeyFrame3D> { static const Bool Value = true; };
template <> struct CanMemCpy<Animation3D::KeyFrameRotation> { static const Bool Value = true; };
template <> struct CanZeroInit<Animation3D::BaseKeyFrame> { static const Bool Value = true; };
template <> struct CanZeroInit<Animation3D::KeyFrame3D> { static const Bool Value = true; };
namespace Animation3D
{

typedef Vector<KeyFrame3D, MemoryBudgets::Animation3D> KeyFrames3D;
typedef Vector<KeyFrameEvent, MemoryBudgets::Animation3D> KeyFramesEvent;
typedef Vector<KeyFrameRotation, MemoryBudgets::Animation3D> KeyFramesRotation;

struct BoneKeyFrames SEOUL_SEALED
{
	BoneKeyFrames()
		: m_vRotation()
		, m_vScale()
		, m_vTranslation()
	{
	}

	KeyFramesRotation m_vRotation;
	KeyFrames3D m_vScale;
	KeyFrames3D m_vTranslation;
}; // struct BoneKeyFrames

class ClipDefinition SEOUL_SEALED
{
public:
	typedef HashTable<Int16, BoneKeyFrames, MemoryBudgets::Animation3D> Bones;

	ClipDefinition();
	~ClipDefinition();

	const Bones& GetBones() const { return m_tBones; }
	UInt32 GetMemoryUsageInBytes() const;
	Bool Load(SyncFile& rFile);

private:
	SEOUL_DISABLE_COPY(ClipDefinition);
	SEOUL_REFERENCE_COUNTED(ClipDefinition);

	Bones m_tBones;
}; // class ClipDefinition

} // namespace Animation3D

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_3D

#endif // include guard
