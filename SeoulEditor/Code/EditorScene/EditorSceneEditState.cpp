/**
* \file EditorSceneEditState.cpp
* \brief Scene data that is only used by the editor (e.g. editor camera settings).
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#include "EditorSceneEditState.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(EditorScene::CameraModeState)
	SEOUL_PROPERTY_N("PitchInRadians", m_fPitchInRadians)
	SEOUL_PROPERTY_N("YawInRadians", m_fYawInRadians)
	SEOUL_PROPERTY_N("UnitsPerSecond", m_fUnitsPerSecond)
	SEOUL_PROPERTY_N("Position", m_vPosition)
	SEOUL_PROPERTY_N("Zoom", m_fZoom)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(EditorScene::CameraState)
	SEOUL_PROPERTY_N("Mode", m_eMode)
	SEOUL_PROPERTY_N("States", m_aStates)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(EditorScene::EditState)
	SEOUL_PROPERTY_N("CameraState", m_CameraState)
SEOUL_END_TYPE()

namespace EditorScene
{

/**
 * Update the easing target position - also resets the progress back to 0.
 */
void CameraModeState::SetEasePosition(const Vector3D& v)
{
	m_EasePosition.m_f = 0.0f;
	m_EasePosition.m_0 = m_vPosition;
	m_EasePosition.m_1 = v;
}

/**
* Update the easing target zoom - also resets the progress back to 0.
*/
void CameraModeState::SetEaseZoom(Float f)
{
	m_EaseZoom.m_f = 0.0f;
	m_EaseZoom.m_0 = m_fZoom;
	m_EaseZoom.m_1 = f;
}

CameraState::CameraState()
{
	static const Float kfDefaultZoom = 50.0f; // TODO: Break out into a config.

	// TODO: Break these out into a config.
	// Setup some reasonable defaults.
	m_aStates[(UInt32)CameraMode::kPerspective].m_fPitchInRadians = -DegreesToRadians(35.0f);
	m_aStates[(UInt32)CameraMode::kPerspective].m_vPosition = Vector3D(0, 32, 30);

	// Orthographic just start with a default zoom.
	m_aStates[(UInt32)CameraMode::kTop].m_fZoom = kfDefaultZoom;
	m_aStates[(UInt32)CameraMode::kBottom].m_fZoom = kfDefaultZoom;
	m_aStates[(UInt32)CameraMode::kLeft].m_fZoom = kfDefaultZoom;
	m_aStates[(UInt32)CameraMode::kRight].m_fZoom = kfDefaultZoom;
	m_aStates[(UInt32)CameraMode::kFront].m_fZoom = kfDefaultZoom;
	m_aStates[(UInt32)CameraMode::kBack].m_fZoom = kfDefaultZoom;
}

} // namespace EditorScene

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
