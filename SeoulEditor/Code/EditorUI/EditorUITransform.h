/**
 * \file EditorUITransform.h
 * \brief Decomposed Matrix4D used in various
 * capacities in the editor.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_TRANSFORM_H
#define EDITOR_UI_TRANSFORM_H

#include "Matrix4D.h"
#include "Quaternion.h"
#include "Vector3D.h"

namespace Seoul::EditorUI
{

struct Transform SEOUL_SEALED
{
	explicit Transform(
		const Vector3D& vScale = Vector3D::One(),
		const Quaternion& qRotation = Quaternion::Identity(),
		const Vector3D& vTranslation = Vector3D::Zero())
		: m_vScale(vScale)
		, m_qRotation(qRotation)
		, m_vTranslation(vTranslation)
	{
	}

	Bool Equals(const Transform& b, Float fTolerance = fEpsilon) const
	{
		return
			m_vScale.Equals(b.m_vScale, fTolerance) &&
			m_qRotation.Equals(b.m_qRotation, fTolerance) &&
			m_vTranslation.Equals(b.m_vTranslation, fTolerance);
	}

	Bool HasZeroScale() const
	{
		return
			Seoul::IsZero(m_vScale.X, 1e-4f) ||
			Seoul::IsZero(m_vScale.Y, 1e-4f) ||
			Seoul::IsZero(m_vScale.Z, 1e-4f);
	}

	Matrix4D ToMatrix4D() const
	{
		return Matrix4D::CreateRotationTranslation(m_qRotation, m_vTranslation) * Matrix4D::CreateScale(m_vScale);
	}

	Vector3D m_vScale;
	Quaternion m_qRotation;
	Vector3D m_vTranslation;

	Bool operator==(const Transform& b) const
	{
		return
			(m_vScale == b.m_vScale) &&
			(m_qRotation == b.m_qRotation) &&
			(m_vTranslation == b.m_vTranslation);
	}

	Bool operator!=(const Transform& b) const
	{
		return !(*this == b);
	}
}; // struct Transform

} // namespace Seoul::EditorUI

#endif // include guard
