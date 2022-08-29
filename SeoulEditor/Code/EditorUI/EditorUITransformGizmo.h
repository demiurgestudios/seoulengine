/**
 * \file EditorUITransformGizmo.h
 * \brief Utility for drawing a 3D gizmo for
 * controlling translation, scale, and rotation.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EDITOR_UI_TRANSFORM_GIZMO_H
#define EDITOR_UI_TRANSFORM_GIZMO_H

#include "Color.h"
#include "EditorUITransform.h"
#include "EditorUITransformGizmoHandle.h"
#include "Matrix4D.h"
#include "Point2DInt.h"
#include "Prereqs.h"
#include "Ray3D.h"
namespace Seoul { class Camera; }
namespace Seoul { namespace Scene { class PrimitiveRenderer; } }
namespace Seoul { struct Viewport; }

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

enum class TransformGizmoMode
{
	kTranslation,
	kRotation,
	kScale,
	COUNT,
};

class TransformGizmo SEOUL_SEALED
{
public:
	static TransformGizmoHandle PickColorToHandle(ColorARGBu8 cColor);

	TransformGizmo();
	~TransformGizmo();

	struct CameraState SEOUL_SEALED
	{
		CameraState(const Camera& camera, const Viewport& viewport)
			: m_Camera(camera)
			, m_Viewport(viewport)
		{
		}

		const Camera& m_Camera;
		const Viewport& m_Viewport;
	};
	struct MouseState SEOUL_SEALED
	{
		MouseState(
			const Camera& camera,
			const Viewport& viewport,
			const Point2DInt& mouseCurrentPosition)
			: m_CameraState(camera, viewport)
			, m_Current(mouseCurrentPosition)
		{
		}

		CameraState m_CameraState;
		Point2DInt m_Current;
	};
	struct DeltaState SEOUL_SEALED
	{
		DeltaState(
			const MouseState& mouseState);

		const MouseState& m_MouseState;
	}; // struct DeltaState
	struct RenderState SEOUL_SEALED
	{
		RenderState(
			Bool bPicking,
			const CameraState& cameraState,
			Scene::PrimitiveRenderer& rRenderer,
			const Transform& transform,
			Bool bGlobalMode);

		Bool const m_bPicking;
		const CameraState& m_CameraState;
		Scene::PrimitiveRenderer& m_rRenderer;
		Float const m_fGizmoScale;
		FixedArray<Bool, 3> const m_abVisible;
	}; // struct RenderState

	void OnMouseDelta(const MouseState& state);
	void Pick(const CameraState& cameraState, Scene::PrimitiveRenderer& rRenderer);
	void Render(const CameraState& cameraState, Scene::PrimitiveRenderer& rRenderer);

	TransformGizmoHandle GetCapturedHandle() const
	{
		return m_eCapturedHandle;
	}

	const Transform& GetCapturedTransform() const
	{
		return m_CapturedTransform;
	}

	Bool GetEnabled() const
	{
		return m_bEnabled;
	}

	Bool GetGlobalMode() const
	{
		return m_bGlobalMode;
	}

	TransformGizmoMode GetMode() const
	{
		return m_eMode;
	}

	void SetEnabled(Bool bEnabled)
	{
		m_bEnabled = bEnabled;
	}

	void SetGlobalMode(Bool bEnabled)
	{
		m_bGlobalMode = bEnabled;
	}

	void SetMode(TransformGizmoMode eMode)
	{
		m_eMode = eMode;
	}

	void SetCapturedHandle(
		TransformGizmoHandle eHandle,
		const Point2DInt& mousePosition = Point2DInt(0, 0))
	{
		if (eHandle != m_eCapturedHandle)
		{
			m_eCapturedHandle = eHandle;
			m_CapturedTransform = m_Transform;
			m_CapturedMousePosition = mousePosition;
		}
	}

	void SetHoveredHandle(TransformGizmoHandle eHandle)
	{
		m_eHoveredHandle = eHandle;
	}

	const Transform& GetTransform() const
	{
		return m_Transform;
	}

	Bool GetRotationSnap() const
	{
		return m_bRotationSnap;
	}

	Float GetRotationSnapDegrees() const
	{
		return m_fRotationSnapDegrees;
	}

	Bool GetScaleSnap() const
	{
		return m_bScaleSnap;
	}

	Float GetScaleSnapFactor() const
	{
		return m_fScaleSnapFactor;
	}

	Bool GetTranslationSnap() const
	{
		return m_bTranslationSnap;
	}

	Float GetTranslationSnapFactor() const
	{
		return m_fTranslationSnapFactor;
	}

	void SetTransform(const Transform& transform)
	{
		m_Transform = transform;
	}

	void SetRotationSnap(Bool bEnable)
	{
		m_bRotationSnap = bEnable;
	}

	void SetRotationSnapDegrees(Float fDegrees)
	{
		m_fRotationSnapDegrees = fDegrees;
	}

	void SetScaleSnap(Bool bEnable)
	{
		m_bScaleSnap = bEnable;
	}

	void SetScaleSnapFactor(Float fFactor)
	{
		m_fScaleSnapFactor = fFactor;
	}

	void SetTranslationSnap(Bool bEnable)
	{
		m_bTranslationSnap = bEnable;
	}

	void SetTranslationSnapFactor(Float fFactor)
	{
		m_fTranslationSnapFactor = fFactor;
	}

private:
	Matrix4D InternalComputeDrawTransformRotation() const;
	Matrix4D InternalComputeDrawTransformScale() const;
	Matrix4D InternalComputeDrawTransformTranslation() const;

	void InternalDeltaRotation(const DeltaState& state);
	void InternalDeltaScale(const DeltaState& state);
	void InternalDeltaTranslation(const DeltaState& state);

	void InternalDraw(const RenderState& state);
	void InternalDrawRotation(const RenderState& state);
	void InternalDrawRotationRing(Axis eAxis, const RenderState& state);
	void InternalDrawScale(const RenderState& state);
	void InternalDrawScaleAxis(Axis eAxis, const RenderState& state);
	void InternalDrawScaleBox(const RenderState& state);
	void InternalDrawScaleHandle(Axis eAxis, const RenderState& state);
	void InternalDrawScalePanel(Axis eAxis, const RenderState& state);
	void InternalDrawTranslation(const RenderState& state);
	void InternalDrawTranslationAxis(Axis eAxis, const RenderState& state);
	void InternalDrawTranslationBox(const RenderState& state);
	void InternalDrawTranslationHandle(Axis eAxis, const RenderState& state);
	void InternalDrawTranslationPanel(Axis eAxis, const RenderState& state);

	TransformGizmoHandle GetCurrentHandleOfInterest() const
	{
		auto const eCurrentHandle = (TransformGizmoHandle::kNone != m_eCapturedHandle
			? m_eCapturedHandle
			: m_eHoveredHandle);
		return eCurrentHandle;
	}

	ColorARGBu8 InternalGetAllColor(Bool bPicking) const;
	ColorARGBu8 InternalGetAxisColor(Bool bPicking, Axis eAxis) const;
	HString InternalGetEffectTechnique(Bool bPicking) const;
	ColorARGBu8 InternalGetPlaneColor(Bool bPicking, Axis eAxis) const;

	Vector3D InternalApplyGlobalModeCapturedToAxis(const Vector3D& vAxis) const;
	Vector3D InternalApplyGlobalModeCurrentToAxis(const Vector3D& vAxis) const;

	Transform m_Transform;
	Transform m_CapturedTransform;
	Point2DInt m_CapturedMousePosition;
	TransformGizmoMode m_eMode;
	TransformGizmoHandle m_eCapturedHandle;
	TransformGizmoHandle m_eHoveredHandle;

	Float m_fRotationSnapDegrees;
	Float m_fScaleSnapFactor;
	Float m_fTranslationSnapFactor;
	Bool m_bRotationSnap;
	Bool m_bScaleSnap;
	Bool m_bTranslationSnap;
	Bool m_bGlobalMode;
	Bool m_bEnabled;

	SEOUL_DISABLE_COPY(TransformGizmo);
}; // class TransformGizmo

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
