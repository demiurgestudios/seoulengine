/**
 * \file EditorSceneEditState.h
 * \brief Scene data that is only used by the editor (e.g. editor camera settings).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_SCENE_EDIT_STATE
#define EDITOR_SCENE_EDIT_STATE

#include "EditorSceneCameraMode.h"
#include "Prereqs.h"
#include "Vector3D.h"

#if SEOUL_WITH_SCENE

namespace Seoul::EditorScene
{

/** Simple easing utility used by camera modes. */
template <typename T>
struct CameraEase SEOUL_SEALED
{
	T m_0{};
	T m_1{};
	Float m_f = 1.0f;

	/**
	* Apply delta f (on [0, 1]) to the current ease
	* progress and recompute the progress into rv.
	*
	* @return true if easing was required, false otherwise.
	*/
	void Advance(Float f, T& rv)
	{
		// Intentionally, don't modify the output if already
		// at the target.
		if (m_f >= 1.0f)
		{
			return;
		}

		// Update alpha and then update output.
		m_f = Clamp(m_f + f, 0.0f, 1.0f);
		rv = Lerp(m_0, m_1, m_f);
	}
};

struct CameraModeState SEOUL_SEALED
{
	CameraEase<Vector3D> m_EasePosition;
	CameraEase<Float> m_EaseZoom;
	Float32 m_fPitchInRadians{};
	Float32 m_fYawInRadians{};
	Float32 m_fUnitsPerSecond{};
	Vector3D m_vPosition{};
	Float m_fZoom{};

	/** Advance all ease values of this mode. */
	void EaseAdvance(Float f)
	{
		m_EasePosition.Advance(f, m_vPosition);
		m_EaseZoom.Advance(f, m_fZoom);
	}
	void SetEasePosition(const Vector3D& v);
	void SetEaseZoom(Float f);
}; // struct CameraModeState

struct CameraState SEOUL_SEALED
{
	CameraState();

	/** Mode of the camera - essentially, perspective and variations of orthographic. */
	CameraMode GetMode() const { return m_eMode; }
	void SetMode(CameraMode eMode) { m_eMode = eMode; }

	/**
	 * Movement rate of the camera.
	 *
	 * Controls fly camera speed. e.g. a value of 1
	 * would cause the Camera to fly at a rate of
	 * 1 unit per second (in default scale, 1 meter per second).
	 */
	Float32 GetUnitsPerSecond() const { return m_aStates[(UInt32)m_eMode].m_fUnitsPerSecond; }
	void SetUnitsPerSecond(Float32 fUnitsPerSecond) { m_aStates[(UInt32)m_eMode].m_fUnitsPerSecond = fUnitsPerSecond; }

	CameraMode m_eMode{};
	FixedArray<CameraModeState, (UInt32)CameraMode::COUNT> m_aStates;
}; // struct CameraState

struct EditState SEOUL_SEALED
{
	CameraState m_CameraState;
}; // EditState

} // namespace Seoul::EditorScene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
