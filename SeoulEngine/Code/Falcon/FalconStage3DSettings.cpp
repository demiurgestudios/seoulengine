/**
 * \file FalconStage3DSettings.cpp
 * \brief A 3D stage is a limited set of perspective and 3D effects used to mix
 * 3D elements into 2D UI. Currently, these elements have a set of global
 * configurations in the Falcon UI system.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconStage3DSettings.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_SPEC_TEMPLATE_TYPE(CheckedPtr<Falcon::Stage3DSettings>)
SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, CheckedPtr<Falcon::Stage3DSettings>, 50, DefaultHashTableKeyTraits<HString>>)
SEOUL_BEGIN_TYPE(Falcon::Stage3DLightingSettings)
	SEOUL_PROPERTY_N("Props", m_Props)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Falcon::Stage3DPropLightingSettings)
	SEOUL_PROPERTY_N("Color", m_vColor)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Falcon::Stage3DPerspectiveSettings)
	SEOUL_PROPERTY_N("DebugShowGridTexture", m_bDebugShowGridTexture)
	SEOUL_PROPERTY_N("Factor", m_fFactor)
	SEOUL_PROPERTY_N("Horizon", m_fHorizon)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Falcon::Stage3DShadowSettings)
	SEOUL_ATTRIBUTE(PostSerializeType, "PostDeserialize")

	SEOUL_PROPERTY_N("Alpha", m_fAlpha)
	SEOUL_PROPERTY_N("DebugForceOnePass", m_bDebugForceOnePass)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("Enabled", m_bEnabled)
	SEOUL_PROPERTY_N("PlanePitch", m_fPlanePitchInDegrees)
	SEOUL_PROPERTY_N("LightPitch", m_fLightPitchInDegrees)
	SEOUL_PROPERTY_N("LightYaw", m_fLightYawInDegrees)
	SEOUL_PROPERTY_N("ResolutionScale", m_fResolutionScale)

	SEOUL_METHOD(PostDeserialize)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Falcon::Stage3DSettings)
	SEOUL_PROPERTY_N("Lighting", m_Lighting)
	SEOUL_PROPERTY_N("Perspective", m_Perspective)
	SEOUL_PROPERTY_N("Shadow", m_Shadow)
SEOUL_END_TYPE()

namespace Falcon
{

Stage3DLightingSettings::Stage3DLightingSettings()
	: m_Props()
{
}

Stage3DPropLightingSettings::Stage3DPropLightingSettings()
	: m_vColor(1.0f, 1.0f, 1.0f)
{
}

Stage3DPerspectiveSettings::Stage3DPerspectiveSettings()
	: m_fFactor(0.0f)
	, m_fHorizon(0.5f)
	, m_bDebugShowGridTexture(false)
{
}

Stage3DShadowSettings::Stage3DShadowSettings()
	: m_fAlpha(1.0f)
	, m_fPlanePitchInDegrees(0.0f)
	, m_fLightPitchInDegrees(0.0f)
	, m_fLightYawInDegrees(0.0f)
	, m_fResolutionScale(0.0f)
	, m_vPlaneNormal(Vector3D::Zero())
	, m_vProjectionDirection(Vector3D::Zero())
	, m_bDebugForceOnePass(false)
	, m_bEnabled(false)
{
	Recompute();
}

Bool Stage3DShadowSettings::PostDeserialize(Reflection::SerializeContext* pContext)
{
	Recompute();
	return true;
}

Vector4D Stage3DShadowSettings::ComputeShadowPlane(const Vector2D& vVanishingPoint) const
{
	Vector3D const vPlanePosition(vVanishingPoint, 0.0f);
	Vector4D const vPlane(
		GetPlaneNormal(),
		Vector3D::Dot(-GetPlaneNormal(), vPlanePosition));
	return vPlane;
}

Vector4D Stage3DShadowSettings::ShadowProject(
	const Vector4D& vPlane,
	const Vector3D& vP) const
{
	Float fDotNormal = Vector3D::Dot(vPlane.GetXYZ(), GetProjectionDirection());
	Float fDist = 0.0f;
	if (!IsZero(fDotNormal))
	{
		// We allow negative values to project "backward". This is sometimes
		// desirable (for example, if a shadow caster is a quad and the actual
		// renderable area is inset somewhat from the quad, some of the vertices
		// of that quad may sink through the shadow plane, which would result
		// in a negative projection).
		fDist = -Vector4D::Dot(vPlane, Vector4D(vP, 1.0f)) / fDotNormal;
	}
	return Vector4D(vP + GetProjectionDirection() * fDist, fDist);
}

void Stage3DShadowSettings::Recompute()
{
	m_vPlaneNormal = Matrix4D::TransformDirection(
		Matrix4D::CreateRotationX(DegreesToRadians(m_fPlanePitchInDegrees)),
		Vector3D::UnitZ());

	Vector3D const v = Matrix4D::TransformDirection(
		Matrix4D::CreateRotationX(DegreesToRadians(m_fPlanePitchInDegrees + m_fLightPitchInDegrees)),
		Vector3D::UnitZ());

	m_vProjectionDirection = Matrix4D::TransformDirection(
		Matrix4D::CreateRotationFromAxisAngle(m_vPlaneNormal, DegreesToRadians(m_fLightYawInDegrees)),
		v);
}

Stage3DSettings::Stage3DSettings()
	: m_Perspective()
	, m_Shadow()
{
}

} // namespace Falcon

} // namespace Seoul
