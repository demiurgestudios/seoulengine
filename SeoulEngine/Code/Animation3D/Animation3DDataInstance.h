/**
 * \file Animation3DDataInstance.h
 * \brief Mutable container of per-frame animation data state. Used to
 * capture an instance pose for query and rendering.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION3D_DATA_INSTANCE_H
#define ANIMATION3D_DATA_INSTANCE_H

#include "Matrix3x4.h"
#include "Matrix4D.h"
#include "Quaternion.h"
#include "ScopedPtr.h"
#include "SharedPtr.h"
#include "Vector.h"
#include "Vector3D.h"
namespace Seoul { class Effect; }
namespace Seoul { class RenderCommandStreamBuilder; }
namespace Seoul { namespace Animation { class EventInterface; } }
namespace Seoul { namespace Animation3D { struct BoneDefinition; } }
namespace Seoul { namespace Animation3D { struct Cache; } }
namespace Seoul { namespace Animation3D { class DataDefinition; } }

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul
{

namespace Animation3D
{

struct BoneInstance SEOUL_SEALED
{
	BoneInstance()
		: m_vPosition(Vector3D::Zero())
		, m_qRotation(Quaternion::Identity())
		, m_vScale(Vector3D::One())
	{
	}

	Vector3D m_vPosition;
	Quaternion m_qRotation;
	Vector3D m_vScale;

	BoneInstance& Assign(const BoneDefinition& data);

	static void ComputeWorldTransform(
		const Vector3D& vPosition,
		const Quaternion& qRotation,
		const Vector3D& vScale,
		Matrix3x4& r)
	{
		// TODO: Optimize.
		r = Matrix3x4(
			Matrix4D::CreateRotationTranslation(qRotation, vPosition) *
			Matrix4D::CreateScale(vScale));
	}

	void ComputeWorldTransform(Matrix3x4& r) const
	{
		ComputeWorldTransform(
			m_vPosition,
			m_qRotation,
			m_vScale,
			r);
	}
}; // struct BoneInstance

} // namespace Animation3D
template <> struct CanMemCpy<Animation3D::BoneInstance> { static const Bool Value = true; };

namespace Animation3D
{

class DataInstance SEOUL_SEALED
{
public:
	typedef Vector<BoneInstance, MemoryBudgets::Animation3D> BoneInstances;
	typedef Vector<Matrix3x4, MemoryBudgets::Rendering> InverseBindPoses;
	typedef Vector<Matrix3x4, MemoryBudgets::Animation3D> SkinningPalette;

	DataInstance(
		const SharedPtr<DataDefinition const>& pData,
		const SharedPtr<Animation::EventInterface>& pEventInterface,
		const InverseBindPoses& vInverseBindPoses);
	~DataInstance();

	DataInstance* Clone() const;

	void CommitSkinningPalette(
		RenderCommandStreamBuilder& rBuilder,
		const SharedPtr<Effect>& pEffect,
		HString parameterSemantic) const;

	const BoneInstances& GetBones() const { return m_vBones; }
	BoneInstances& GetBones() { return m_vBones; }

	// Return the animation accumulator cache owned by this NetworkInstance.
	const Cache& GetCache() const { return *m_pCache; }
	Cache& GetCache() { return *m_pCache; }

	const SharedPtr<Animation::EventInterface>& GetEventInterface() const { return m_pEventInterface; }
	const SharedPtr<DataDefinition const>& GetData() const { return m_pData; }

	const SkinningPalette& GetSkinningPalette() const { return m_vSkinningPalette; }

	// Apply the current state of the animation cache to the instance state. This also resets the cache.
	void ApplyCache();

	// Prepare the skinning palette state of this instance for query and render.
	// Applies any animation changes made until now to the active skinning
	// palette.
	void PoseSkinningPalette();

private:
	ScopedPtr<Cache> const m_pCache;
	SharedPtr<DataDefinition const> const m_pData;
	SharedPtr<Animation::EventInterface> const m_pEventInterface;
	InverseBindPoses const m_vInverseBindPoses;
	BoneInstances m_vBones;
	SkinningPalette m_vSkinningPalette;

	void InternalConstruct();
	void InternalPoseBone(Int16 iBone);
	void InternalPoseBone(
		Int16 iBone,
		const Vector3D& vPosition,
		const Quaternion& qRotation,
		const Vector3D& vScale);

	SEOUL_DISABLE_COPY(DataInstance);
}; // class DataInstance

} // namespace Animation3D

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_3D

#endif // include guard
