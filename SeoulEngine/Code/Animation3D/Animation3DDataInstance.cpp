/**
 * \file Animation3DDataInstance.cpp
 * \brief Mutable container of per-frame instance state. Used to
 * capture an instance pose for query and rendering.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnimationEventInterface.h"
#include "Animation3DCache.h"
#include "Animation3DClipDefinition.h"
#include "Animation3DDataDefinition.h"
#include "Animation3DDataInstance.h"
#include "Effect.h"
#include "RenderCommandStreamBuilder.h"

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul::Animation3D
{

// TODO: Make this easier to keep in-sync - must be
// kept in-sync with max constant in the shader.
static const UInt32 kuMaxSkinningPaletteSize = 68u;

BoneInstance& BoneInstance::Assign(const BoneDefinition& data)
{
	m_vPosition = data.m_vPosition;
	m_qRotation = data.m_qRotation;
	m_vScale = data.m_vScale;

	return *this;
}

DataInstance::DataInstance(
	const SharedPtr<DataDefinition const>& pData,
	const SharedPtr<Animation::EventInterface>& pEventInterface,
	const InverseBindPoses& vInverseBindPoses)
	: m_pCache(SEOUL_NEW(MemoryBudgets::Animation3D) Cache)
	, m_pData(pData)
	, m_pEventInterface(pEventInterface)
	, m_vInverseBindPoses(vInverseBindPoses)
	, m_vBones()
	, m_vSkinningPalette()
{
	InternalConstruct();
}

DataInstance::~DataInstance()
{
}

DataInstance* DataInstance::Clone() const
{
	auto p = SEOUL_NEW(MemoryBudgets::Animation3D) DataInstance(m_pData, m_pEventInterface, m_vInverseBindPoses);
	p->m_vBones = m_vBones;
	p->m_vSkinningPalette = m_vSkinningPalette;
	return p;
}

void DataInstance::CommitSkinningPalette(
	RenderCommandStreamBuilder& rBuilder,
	const SharedPtr<Effect>& pEffect,
	HString parameterSemantic) const
{
	if (m_vSkinningPalette.IsEmpty())
	{
		return;
	}

	rBuilder.SetMatrix3x4ArrayParameter(
		pEffect,
		parameterSemantic,
		m_vSkinningPalette.Data(),
		Min(m_vSkinningPalette.GetSize(), kuMaxSkinningPaletteSize));
}

/** Apply the current state of the animation cache to the instance state. This also resets the cache. */
void DataInstance::ApplyCache()
{
	auto const& data = *m_pData;
	auto const& vBones = data.GetBones();

	// Bone transformation.
	{
		Int32 const iBones = (Int32)m_vBones.GetSize();
		for (Int32 iBone = 0; iBone < iBones; ++iBone)
		{
			auto const& base = vBones[iBone];
			auto& r = m_vBones[iBone];

			// Position
			{
				auto p = m_pCache->m_tPosition.Find(iBone);
				if (nullptr == p)
				{
					r.m_vPosition = base.m_vPosition;
				}
				else
				{
					auto const& v = *p;
					r.m_vPosition = base.m_vPosition + v;
				}
			}

			// Rotation.
			{
				auto p = m_pCache->m_tRotation.Find(iBone);
				if (nullptr == p)
				{
					r.m_qRotation = base.m_qRotation;
				}
				else
				{
					r.m_qRotation = Quaternion::Normalize(*p * base.m_qRotation);
				}
			}

			// Scale.
			{
				auto p = m_pCache->m_tScale.Find(iBone);
				if (nullptr == p)
				{
					r.m_vScale = base.m_vScale;
				}
				else
				{
					auto const& v = *p;
					Float const fBaseAlpha = (1.0f - Clamp(v.Z, 0.0f, 1.0f));
					r.m_vScale = Vector3D::ComponentwiseMultiply(base.m_vScale, v.GetXYZ()) + (base.m_vScale * fBaseAlpha);
				}
			}
		}
	}

	m_pCache->Clear();
}

/**
 * Prepare the skinning palette state of this instance for query and render.
 * Applies any animation changes made until now to the active skinning
 * palette.
 */
void DataInstance::PoseSkinningPalette()
{
	// Nothing to do if no bones.
	UInt32 const u = m_vSkinningPalette.GetSize();
	if (0u == u)
	{
		return;
	}

	// Pose all bones from beginning to end.
	UInt32 const uBones = m_vBones.GetSize();
	for (UInt32 i = 0u; i < uBones; ++i)
	{
		InternalPoseBone((Int16)i);
	}

	// Now apply the inverse bind poses.
	if (m_vInverseBindPoses.GetSize() == m_vBones.GetSize())
	{
		for (UInt32 i = 0u; i < uBones; ++i)
		{
			auto& r = m_vSkinningPalette[i];
			r = r * m_vInverseBindPoses[i];
		}
	}
}

void DataInstance::InternalConstruct()
{
	auto const& vBones = m_pData->GetBones();

	auto const uBones = vBones.GetSize();
	m_vBones.Resize(uBones);
	m_vSkinningPalette.Resize(uBones, Matrix3x4::Identity());

	for (auto i = 0u; i < uBones; ++i) { m_vBones[i].Assign(vBones[i]); }

	PoseSkinningPalette();
}

void DataInstance::InternalPoseBone(Int16 iBone)
{
	auto const& state = m_vBones[iBone];
	InternalPoseBone(
		iBone,
		state.m_vPosition,
		state.m_qRotation,
		state.m_vScale);
}

void DataInstance::InternalPoseBone(
	Int16 iBone,
	const Vector3D& vPosition,
	const Quaternion& qRotation,
	const Vector3D& vScale)
{
	auto& r = m_vSkinningPalette[iBone];
	m_vBones[iBone].ComputeWorldTransform(vPosition, qRotation, vScale, r);

	auto const& data = m_pData->GetBones()[iBone];

	if (data.m_iParent >= 0)
	{
		auto const& mParent = m_vSkinningPalette[data.m_iParent];
		r = mParent * r;
	}
}

} // namespace Seoul::Animation3D

#endif // /#if SEOUL_WITH_ANIMATION_3D
