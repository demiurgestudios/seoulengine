/**
 * \file EditorUICommandTransformObjects.cpp
 * \brief DevUI::Command for wrapping a command that manipulates
 * the transform parts (scale, rotation, position) of a scene
 * object.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorUICommandTransformObjects.h"
#include "ReflectionDefine.h"
#include "SceneMeshDrawComponent.h"
#include "SceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(EditorUI::CommandTransformObjects, TypeFlags::kDisableNew)
	SEOUL_PARENT(DevUI::Command)
SEOUL_END_TYPE()

namespace EditorUI
{

CommandTransformObjects::CommandTransformObjects(
	const Entries& vEntries,
	const Transform& mReferenceTransform,
	const Transform& mTargetTransform)
	: DevUI::Command(false)
	, m_vEntries(vEntries)
	, m_vTransforms()
	, m_sDescription(String::Printf("Transform %u Objects", vEntries.GetSize()))
{
	ComputeEntries(vEntries, mReferenceTransform, mTargetTransform);
}

CommandTransformObjects::~CommandTransformObjects()
{
}

void CommandTransformObjects::Do()
{
	// Sanity check.
	SEOUL_ASSERT(m_vEntries.GetSize() == m_vTransforms.GetSize());

	Entries::ConstIterator iB = m_vEntries.Begin();
	for (Transforms::ConstIterator iA = m_vTransforms.Begin(); m_vTransforms.End() != iA; ++iA, ++iB)
	{
		auto pMesh(iB->m_pObject->GetComponent<Scene::MeshDrawComponent>());
		if (pMesh.IsValid()) { pMesh->SetScale(iA->m_vScale); }
		iB->m_pObject->SetRotation(iA->m_qRotation);
		iB->m_pObject->SetPosition(iA->m_vTranslation);
	}
}

const String& CommandTransformObjects::GetDescription() const
{
	return m_sDescription;
}

UInt32 CommandTransformObjects::GetSizeInBytes() const
{
	return (UInt32)(
		m_sDescription.GetCapacity() +
		m_vTransforms.GetSizeInBytes() +
		m_vEntries.GetSizeInBytes() +
		sizeof(*this));
}

void CommandTransformObjects::Undo()
{
	for (auto const& entry : m_vEntries)
	{
		auto pMesh(entry.m_pObject->GetComponent<Scene::MeshDrawComponent>());
		if (pMesh.IsValid()) { pMesh->SetScale(entry.m_mTransform.m_vScale); }
		entry.m_pObject->SetRotation(entry.m_mTransform.m_qRotation);
		entry.m_pObject->SetPosition(entry.m_mTransform.m_vTranslation);
	}
}

void CommandTransformObjects::ComputeEntries(
	const Entries& vEntries,
	const Transform& mReferenceTransform,
	const Transform& mTargetTransform)
{
	m_vTransforms.Clear();
	m_vTransforms.Reserve(vEntries.GetSize());

	// Special handling for single object,
	// when the reference transform is equal
	// to the starting transform (expected
	// to be the most common case).
	if (vEntries.GetSize() == 1u &&
		vEntries.Front().m_mTransform == mReferenceTransform)
	{
		m_vTransforms.PushBack(mTargetTransform);
	}
	// Complex case - need to remove
	// the reference transform from the existing,
	// then apply the target.
	else
	{
		auto const mDelta = (mTargetTransform.ToMatrix4D() * mReferenceTransform.ToMatrix4D().Inverse());
		for (auto const& entry : vEntries)
		{
			// Special case for the reference. We do this to
			// maintain stability in light of mirroring
			// (a negative scale is underconstrained - e.g. a
			// negative scale on X is equivalent to a negative
			// scale on a different axis with a modified rotation).
			//
			// If we decomposed the reference, we would get
			// "fluttering", as the scale/rotation could change
			// frame to frame.
			if (entry.m_mTransform == mReferenceTransform)
			{
				m_vTransforms.PushBack(mTargetTransform);
				continue;
			}

			// Compute the new transform for the entry.
			auto const mNewTransform = (mDelta * entry.m_mTransform.ToMatrix4D());

			// Now decompose into parts.
			Transform transform;

			Matrix3D mPreRotation;
			Matrix3D mRotation;
			Vector3D vPosition;

			// Degenerate case means one of the primary axes is 0,
			// so we set scale to 0 and carry through translation.
			if (!Matrix4D::Decompose(mNewTransform, mPreRotation, mRotation, vPosition))
			{
				transform.m_qRotation = Quaternion::Identity();
				transform.m_vScale = Vector3D::Zero();
				transform.m_vTranslation = mNewTransform.GetTranslation();
			}
			// Otherwise, set straight away.
			else
			{
				transform.m_vScale = mPreRotation.GetDiagonal();
				transform.m_qRotation = Quaternion::CreateFromRotationMatrix(mRotation);
				transform.m_vTranslation = vPosition;
			}

			m_vTransforms.PushBack(transform);
		}
	}
}

Bool CommandTransformObjects::DoMerge(DevUI::Command const* pCommand)
{
	CommandTransformObjects const* p = DynamicCast<CommandTransformObjects const*>(pCommand);
	if (nullptr == p)
	{
		return false;
	}

	if (m_vEntries.GetSize() != p->m_vEntries.GetSize())
	{
		return false;
	}

	{
		Entries::ConstIterator iB = p->m_vEntries.Begin();
		for (Entries::ConstIterator iA = m_vEntries.Begin(); m_vEntries.End() != iA; ++iA, ++iB)
		{
			if (iA->m_pObject != iB->m_pObject)
			{
				return false;
			}

			if (iA->m_mTransform != iB->m_mTransform)
			{
				return false;
			}
		}
	}

	{
		Transforms::ConstIterator iB = p->m_vTransforms.Begin();
		for (Transforms::Iterator iA = m_vTransforms.Begin(); m_vTransforms.End() != iA; ++iA, ++iB)
		{
			(*iA) = (*iB);
		}
	}

	return true;
}

} // namespace EditorUI

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
