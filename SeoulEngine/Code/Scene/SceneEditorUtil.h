/**
 * \file SceneEditorUtil.h
 * \brief Used for rotation handling in the editor only.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_EDITOR_UTIL_H
#define SCENE_EDITOR_UTIL_H

#include "Prereqs.h"
#include "Quaternion.h"
#include "Vector3D.h"

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

#if SEOUL_EDITOR_AND_TOOLS

// TODO: Move this into Quaternion:: and add unit tests.
static inline Quaternion ToQuaternion(const Vector3D& vEulerRadians)
{
	auto const f0 = Cos(vEulerRadians.X * 0.5f);
	auto const f1 = Sin(vEulerRadians.X * 0.5f);
	auto const f2 = Cos(vEulerRadians.Y * 0.5f);
	auto const f3 = Sin(vEulerRadians.Y * 0.5f);
	auto const f4 = Cos(vEulerRadians.Z * 0.5f);
	auto const f5 = Sin(vEulerRadians.Z * 0.5f);

	return Quaternion::Normalize(Quaternion(
		(f4 * f1 * f2) - (f5 * f0 * f3),
		(f4 * f0 * f3) + (f5 * f1 * f2),
		(f5 * f0 * f2) - (f4 * f1 * f3),
		(f4 * f0 * f2) + (f5 * f1 * f3)));
}

// TODO: Move this into Quaternion:: and add unit tests.
static inline Vector3D ToEuler(const Quaternion& q)
{
	auto const fYsqr = (q.Y * q.Y);

	// roll (x-axis rotation)
	auto const f0 = 2.0f * (q.W * q.X + q.Y * q.Z);
	auto const f1 = 1.0f - (2.0f * ((q.X * q.X) + fYsqr));
	auto const fX = Atan2(f0, f1);

	// pitch (y-axis rotation)
	auto f2 = 2.0f * ((q.Y * q.W) - (q.X * q.Z));
	f2 = ((f2 > 1.0f) ? 1.0f : f2);
	f2 = ((f2 < -1.0f) ? -1.0f : f2);
	auto const fY = Asin(f2);

	// yaw (z-axis rotation)
	auto const f3 = 2.0f * ((q.X * q.Y) + (q.Z * q.W));
	auto const f4 = 1.0f - (2.0f * ((q.Z * q.Z) + fYsqr));
	auto const fZ = Atan2(f3, f4);

	return Vector3D(fX, fY, fZ);
}

static inline Vector3D ToDegrees(const Vector3D& v)
{
	return Vector3D(RadiansToDegrees(v.X), RadiansToDegrees(v.Y), RadiansToDegrees(v.Z));
}

static inline Vector3D ToRadians(const Vector3D& v)
{
	return Vector3D(DegreesToRadians(v.X), DegreesToRadians(v.Y), DegreesToRadians(v.Z));
}

static inline Vector3D GetEulerDegrees(const Vector3D& vEulerRadians, const Quaternion& qActual)
{
	// Check if quaternion has changed and is out of sync with
	// euler angles. When this happens, update the euler angles.
	auto const qCheck(ToQuaternion(vEulerRadians));

	Vector3D vReturn;
	if (!qCheck.Equals(qActual, 1e-4f))
	{
		vReturn = ToEuler(qActual);
	}
	else
	{
		vReturn = vEulerRadians;
	}

	return ToDegrees(vReturn);
}

static inline void SetEulerDegrees(
	Vector3D vInDegrees,
	Vector3D& rvOutEulerRadians,
	Quaternion& rqRotation)
{
	// Normalize.
	vInDegrees.X = ClampDegrees(vInDegrees.X);
	vInDegrees.Y = ClampDegrees(vInDegrees.Y);
	vInDegrees.Z = ClampDegrees(vInDegrees.Z);

	// Commit.
	rvOutEulerRadians = ToRadians(vInDegrees);
	rqRotation = ToQuaternion(rvOutEulerRadians);
}

#endif // /#if SEOUL_EDITOR_AND_TOOLS

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
