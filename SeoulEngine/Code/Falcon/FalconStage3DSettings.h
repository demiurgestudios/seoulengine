/**
 * \file FalconStage3DSettings.h
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

#pragma once
#ifndef FALCON_STAGE_3D_SETTINGS_H
#define FALCON_STAGE_3D_SETTINGS_H

#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "Vector3D.h"
#include "Vector4D.h"
namespace Seoul { namespace Reflection { struct SerializeContext; } }

namespace Seoul::Falcon
{

class Stage3DPropLightingSettings SEOUL_SEALED
{
public:
	Stage3DPropLightingSettings();

	Vector3D m_vColor;
}; // class Stage3DPropLightingSettings

class Stage3DLightingSettings SEOUL_SEALED
{
public:
	Stage3DLightingSettings();

	Stage3DPropLightingSettings m_Props;
}; // class Stage3DLightingSettings

class Stage3DPerspectiveSettings SEOUL_SEALED
{
public:
	Stage3DPerspectiveSettings();

	Float m_fFactor;
	Float m_fHorizon;
	Bool m_bDebugShowGridTexture;
}; // class Stage3DPerspectiveSettings

class Stage3DShadowSettings SEOUL_SEALED
{
public:
	Stage3DShadowSettings();

	Vector4D ComputeShadowPlane(const Vector2D& vVanishingPoint) const;
	Vector4D ShadowProject(
		const Vector4D& vPlane,
		const Vector3D& vP) const;

	const Vector3D& GetPlaneNormal() const { return m_vPlaneNormal; }
	const Vector3D& GetProjectionDirection() const { return m_vProjectionDirection; }

	Float GetAlpha() const { return m_fAlpha; }
	void SetAlpha(Float f) { m_fAlpha = f; }

	Bool GetDebugForceOnePassRendering() const { return m_bDebugForceOnePass; }
	void SetDebugForceOnePassRendering(Bool b) { m_bDebugForceOnePass = b; }

	Bool GetEnabled() const { return m_bEnabled; }
	void SetEnabled(Bool bEnabled) { m_bEnabled = bEnabled; }

	Float GetLightPitchInDegrees() const { return m_fLightPitchInDegrees; }
	void SetLightPitchInDegrees(Float f) { m_fLightPitchInDegrees = f; Recompute(); }

	Float GetLightYawInDegrees() const { return m_fLightYawInDegrees; }
	void SetLightYawInDegrees(Float f) { m_fLightYawInDegrees = f; Recompute(); }

	Float GetPlanePitchInDegrees() const { return m_fPlanePitchInDegrees; }
	void SetPlanePitchInDegrees(Float f) { m_fPlanePitchInDegrees = f; Recompute(); }

	Float GetResolutionScale() const { return m_fResolutionScale; }
	void SetResolutionScale(Float f) { m_fResolutionScale = f; }

private:
	SEOUL_REFLECTION_FRIENDSHIP(Stage3DShadowSettings);

	Float m_fAlpha;
	Float m_fPlanePitchInDegrees;
	Float m_fLightPitchInDegrees;
	Float m_fLightYawInDegrees;
	Float m_fResolutionScale;
	Vector3D m_vPlaneNormal;
	Vector3D m_vProjectionDirection;
	Bool m_bDebugForceOnePass;
	Bool m_bEnabled;

	Bool PostDeserialize(Reflection::SerializeContext* pContext);
	void Recompute();
}; // class Stage3DShadowSettings

class Stage3DSettings SEOUL_SEALED
{
public:
	Stage3DSettings();

	Stage3DLightingSettings m_Lighting;
	Stage3DPerspectiveSettings m_Perspective;
	Stage3DShadowSettings m_Shadow;
}; // class Stage3DSettings

} // namespace Seoul::Falcon

#endif // include guard
