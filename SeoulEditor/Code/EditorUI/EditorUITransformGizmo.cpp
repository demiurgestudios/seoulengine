/**
 * \file EditorUITransformGizmo.cpp
 * \brief Utility for drawing a 3D gizmo for
 * controlling translation, scale, and rotation.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "EditorUITransformGizmo.h"
#include "EditorUIUtil.h"
#include "Plane.h"
#include "Ray3D.h"
#include "ScenePrimitiveRenderer.h"
#include "SeoulHString.h"
#include "Viewport.h"

#if SEOUL_WITH_SCENE

namespace Seoul::EditorUI
{

static const HString kEffectTechniqueRenderGizmo("seoul_RenderGizmo");
static const HString kEffectTechniqueRenderGizmoNoLighting("seoul_RenderGizmoNoLighting");

// TODO: Configurable?
static const Float kfRotationMagnitudeFactor = 1.0f;
static const Float kfScalingMagnitudeFactor = 2.0f;
static const Float kfTranslationMagnitudeFactor = 1.0f;

static const Float kfDesiredGizmoSizeInPixels = 100.0f;

static const ColorARGBu8 kDisabledColor(ColorARGBu8::Create(127, 127, 127, 255));

static const ColorARGBu8 kHighlightColor(ColorARGBu8::Create(255, 255, 0, 255));

static const ColorARGBu8 kPickX(ColorARGBu8::Create(248, 255, 255, 255));
static const ColorARGBu8 kPickY(ColorARGBu8::Create(249, 255, 255, 255));
static const ColorARGBu8 kPickZ(ColorARGBu8::Create(250, 255, 255, 255));
static const ColorARGBu8 kPickXY(ColorARGBu8::Create(251, 255, 255, 255));
static const ColorARGBu8 kPickYZ(ColorARGBu8::Create(252, 255, 255, 255));
static const ColorARGBu8 kPickXZ(ColorARGBu8::Create(253, 255, 255, 255));
static const ColorARGBu8 kPickAll(ColorARGBu8::Create(254, 255, 255, 255));

/**
 * Visibility of a Gizmo axis is based on whether enough of that axis
 * can be seen in screen space. This is based on the length of
 * a translation gizmo axis in screen space.
 */
static inline FixedArray<Bool, 3> ComputeGizmoVisibility(
	const TransformGizmo::CameraState& cameraState,
	const Transform& transform,
	Bool bGlobalMode,
	Float fGizmoScale)
{
	// Compute our tolerance as 3% of the viewport width and height.
	Vector2D const vTolerance(
		(Float)cameraState.m_Viewport.m_iViewportWidth * 0.03f,
		(Float)cameraState.m_Viewport.m_iViewportHeight * 0.03f);

	// Compute the transform equivalent of the translation gizmo.
	auto const mTransform(Matrix4D::CreateRotationTranslation(
		(bGlobalMode ? Quaternion::Identity() : transform.m_qRotation),
		transform.m_vTranslation));

	// Center point is always the same.
	auto const v0(transform.m_vTranslation);

	// Project into screen space.
	auto const s0(cameraState.m_Camera.ConvertWorldToScreenSpace(cameraState.m_Viewport, v0).GetXY());

	// Now compute s1 for all three axes, and mark those for
	// which s0 and s1 project to the (approximately) same screen
	// space position as not visible.
	FixedArray<Bool, 3> abReturn;
	for (Int i = (Int)Axis::kX; i <= (Int)Axis::kZ; ++i)
	{
		Vector3D vAxis(Vector3D::Zero());
		vAxis[i] = fGizmoScale;
		auto const v1(Matrix4D::TransformPosition(mTransform, vAxis));
		auto const s1(cameraState.m_Camera.ConvertWorldToScreenSpace(cameraState.m_Viewport, v1).GetXY());

		auto const vD((s1 - s0).Abs());
		abReturn[i] = (vD.X >= vTolerance.X || vD.Y >= vTolerance.Y);
	}

	return abReturn;
}

TransformGizmo::DeltaState::DeltaState(
	const MouseState& mouseState)
	: m_MouseState(mouseState)
{
}

TransformGizmo::RenderState::RenderState(
	Bool bPicking,
	const CameraState& cameraState,
	Scene::PrimitiveRenderer& rRenderer,
	const Transform& transform,
	Bool bGlobalMode)
	: m_bPicking(bPicking)
	, m_CameraState(cameraState)
	, m_rRenderer(rRenderer)
	, m_fGizmoScale(ComputeGizmoScale(kfDesiredGizmoSizeInPixels, cameraState.m_Camera, cameraState.m_Viewport, transform.m_vTranslation))
	, m_abVisible(ComputeGizmoVisibility(cameraState, transform, bGlobalMode, m_fGizmoScale))
{
}

TransformGizmoHandle TransformGizmo::PickColorToHandle(ColorARGBu8 cColor)
{
	     if (kPickX == cColor) { return TransformGizmoHandle::kX; }
	else if (kPickY == cColor) { return TransformGizmoHandle::kY; }
	else if (kPickZ == cColor) { return TransformGizmoHandle::kZ; }
	else if (kPickXY == cColor) { return TransformGizmoHandle::kXY; }
	else if (kPickYZ == cColor) { return TransformGizmoHandle::kYZ; }
	else if (kPickXZ == cColor) { return TransformGizmoHandle::kXZ; }
	else if (kPickAll == cColor) { return TransformGizmoHandle::kAll; }
	else
	{
		return TransformGizmoHandle::kNone;
	}
}

TransformGizmo::TransformGizmo()
	: m_Transform(Vector3D::One(), Quaternion::Identity(), Vector3D::Zero())
	, m_CapturedTransform(m_Transform)
	, m_CapturedMousePosition(0, 0)
	, m_eMode(TransformGizmoMode::kTranslation)
	, m_eCapturedHandle(TransformGizmoHandle::kNone)
	, m_eHoveredHandle(TransformGizmoHandle::kNone)
	, m_fRotationSnapDegrees(10.0f)
	, m_fScaleSnapFactor(0.25f)
	, m_fTranslationSnapFactor(0.1f)
	, m_bRotationSnap(true)
	, m_bScaleSnap(true)
	, m_bTranslationSnap(true)
	, m_bGlobalMode(false)
	, m_bEnabled(true)
{
}

TransformGizmo::~TransformGizmo()
{
}

void TransformGizmo::OnMouseDelta(const MouseState& state)
{
	DeltaState const deltaState(state);
	if (m_eCapturedHandle != TransformGizmoHandle::kNone)
	{
		switch (m_eMode)
		{
		case TransformGizmoMode::kRotation: InternalDeltaRotation(deltaState); break;
		case TransformGizmoMode::kScale: InternalDeltaScale(deltaState); break;
		case TransformGizmoMode::kTranslation: InternalDeltaTranslation(deltaState); break;
		default:
			break;
		};
	}
}

void TransformGizmo::Pick(
	const CameraState& cameraState,
	Scene::PrimitiveRenderer& rRenderer)
{
	RenderState const state(true, cameraState, rRenderer, m_Transform, m_bGlobalMode);
	InternalDraw(state);
}

void TransformGizmo::Render(
	const CameraState& cameraState,
	Scene::PrimitiveRenderer& rRenderer)
{
	RenderState const state(false, cameraState, rRenderer, m_Transform, m_bGlobalMode);
	InternalDraw(state);
}

Matrix4D TransformGizmo::InternalComputeDrawTransformRotation() const
{
	return Matrix4D::CreateRotationTranslation(
		(m_bGlobalMode ? Quaternion::Identity() : m_Transform.m_qRotation),
		m_Transform.m_vTranslation);
}

Matrix4D TransformGizmo::InternalComputeDrawTransformScale() const
{
	// Scale is always local, ignores global mode.
	return Matrix4D::CreateRotationTranslation(
		m_Transform.m_qRotation,
		m_Transform.m_vTranslation);
}

Matrix4D TransformGizmo::InternalComputeDrawTransformTranslation() const
{
	return Matrix4D::CreateRotationTranslation(
		(m_bGlobalMode ? Quaternion::Identity() : m_Transform.m_qRotation),
		m_Transform.m_vTranslation);
}

void TransformGizmo::InternalDeltaRotation(const DeltaState& state)
{
	static Float const kfGizmoRadius = 1.0f; // TODO:
	static Float const kfRotationRescale = 100.0f; // TODO:

	// Cache members from input state.
	const Camera& camera = state.m_MouseState.m_CameraState.m_Camera;
	const Viewport& viewport = state.m_MouseState.m_CameraState.m_Viewport;

	// World and screen space center position of the gizmo.
	Vector3D const vW0(m_CapturedTransform.m_vTranslation);
	Vector2D const vS0(camera.ConvertWorldToScreenSpace(viewport, vW0).GetXY());

	// Compute the axis of rotation.
	Vector3D vWorldAxis(Vector3D::Zero());
	{
		switch (m_eCapturedHandle)
		{
			// Ring axis.
		case TransformGizmoHandle::kX: // fall-through
		case TransformGizmoHandle::kY: // fall-through
		case TransformGizmoHandle::kZ:
			vWorldAxis[((Int)m_eCapturedHandle - (Int)TransformGizmoHandle::kX)] = 1.0f;
			vWorldAxis = InternalApplyGlobalModeCapturedToAxis(vWorldAxis);
			break;

			// "All" axis is the camera view.
		case TransformGizmoHandle::kAll:
			vWorldAxis = -camera.GetViewAxis();
			break;

			// No other supported modes.
		default:
			break;
		};
	}

	// Convert the computed world axis into a screen axis.
	Vector2D const vScreenAxis = Vector2D::Normalize(
		camera.ConvertWorldToScreenSpace(viewport, vW0 + vWorldAxis).GetXY() - vS0);

	// Compute the total change as a 2D vector in screen space (mouse space).
	Vector2D const vScreenDelta(
		Vector2D((Float32)state.m_MouseState.m_Current.X, (Float32)state.m_MouseState.m_Current.Y) -
		Vector2D((Float32)m_CapturedMousePosition.X, (Float32)m_CapturedMousePosition.Y));

	// Two cases - if the rotation axis is very close to parallel with the camera axis,
	// we use the 2D cross between screen delta and normal (as formed by the original
	// captured mouse position and the screen space center of the gizmo).
	Float fRotationAngle;
	if (Equals(Abs(Vector3D::Dot(vWorldAxis, camera.GetViewAxis())), 1.0f, 1e-3f))
	{
		Vector2D const vCapturedMouse((Float)m_CapturedMousePosition.X, (Float)m_CapturedMousePosition.Y);
		Vector2D const vScreenNormal(Vector2D::Normalize(vCapturedMouse - vS0));
		fRotationAngle = (Vector2D::Cross(vScreenDelta, vScreenNormal) / kfRotationRescale) * kfRotationMagnitudeFactor;
	}
	// Otherwise, compute the rotation amount as the 2D negative cross between screen delta and screen
	// space axis of rotation.
	else
	{
		fRotationAngle = (-Vector2D::Cross(vScreenDelta, vScreenAxis) / kfRotationRescale) * kfRotationMagnitudeFactor;
	}

	// Apply snapping, if enabled.
	if (m_bRotationSnap)
	{
		Float const fRotationSnapRadians = DegreesToRadians(m_fRotationSnapDegrees);
		fRotationAngle = Round(fRotationAngle / fRotationSnapRadians) * fRotationSnapRadians;
	}

	// Apply the rotation - we're computing delta angle,
	// so apply as an concatenation to the current rotation.
	m_Transform.m_qRotation = Quaternion::Normalize(
		Quaternion::CreateFromAxisAngle(vWorldAxis, fRotationAngle) *
		m_CapturedTransform.m_qRotation);
}

void TransformGizmo::InternalDeltaScale(const DeltaState& state)
{
	static Float const kfScaleRescale = 100.0f; // TODO:

	// Cache camera and viewport for further calculations.
	const Camera& camera = state.m_MouseState.m_CameraState.m_Camera;
	const Viewport& viewport = state.m_MouseState.m_CameraState.m_Viewport;

	// Compute the total change as a 2D vector in screen space (mouse space).
	Vector2D const vScreenDelta(
		Vector2D((Float32)state.m_MouseState.m_Current.X, (Float32)state.m_MouseState.m_Current.Y) -
		Vector2D((Float32)m_CapturedMousePosition.X, (Float32)m_CapturedMousePosition.Y));

	// Compute scaling axis in world and screen, and components in
	// local (components are the terms to actually apply scaling to).
	// Compute a single scaling vector, in screen and world space.
	Vector2D vScreenAxis(Vector2D::Zero());
	Vector3D vComponents(Vector3D::Zero());

	// Compute axes and components.
	{
		// Cache captured gizmo center in world and screen space.
		Vector3D const vW0(m_CapturedTransform.m_vTranslation);
		Vector2D const vS0(camera.ConvertWorldToScreenSpace(viewport, vW0).GetXY());

		// Assign appropriate axes.
		switch (m_eCapturedHandle)
		{
			// Single axis scaling.
		case TransformGizmoHandle::kX: // fall-through
		case TransformGizmoHandle::kY: // fall-through
		case TransformGizmoHandle::kZ:
			vComponents[(Int)m_eCapturedHandle - (Int)TransformGizmoHandle::kX] = 1.0f;
			break;

			// Gizmo axes planar scaling.
		case TransformGizmoHandle::kXY:
			vComponents.X = 1.0f;
			vComponents.Y = 1.0f;
			break;
		case TransformGizmoHandle::kXZ:
			vComponents.X = 1.0f;
			vComponents.Z = 1.0f;
			break;
		case TransformGizmoHandle::kYZ:
			vComponents.Y = 1.0f;
			vComponents.Z = 1.0f;
			break;

			// "All" scaling.
		case TransformGizmoHandle::kAll:
			vComponents = Vector3D::One();
			break;
		default:
			break;
		};

		if (TransformGizmoHandle::kAll != m_eCapturedHandle)
		{
			// Apply the global mode, then normalize the components
			// to determine the axis.
			auto vWorldAxis = InternalApplyGlobalModeCapturedToAxis(Vector3D::Normalize(vComponents));

			// Finally, convert the computed world axis into a screen axis.
			vScreenAxis = Vector2D::Normalize(
				camera.ConvertWorldToScreenSpace(viewport, vW0 + vWorldAxis).GetXY() - vS0);
		}
		else
		{
			// For all scaling, just use the up screen axis.
			vScreenAxis = -Vector2D::UnitY();
		}
	}

	// Scaling magnitude along the axis.
	Float fScaling = (Vector2D::Dot(vScreenAxis, vScreenDelta) / kfScaleRescale) * kfScalingMagnitudeFactor;

	// Apply snapping to scaling.
	if (m_bScaleSnap)
	{
		fScaling = Round(fScaling / m_fScaleSnapFactor) * m_fScaleSnapFactor;
	}

	// Compute and apply final delta.
	Vector3D const vDelta = (vComponents * fScaling);
	m_Transform.m_vScale = vDelta + m_CapturedTransform.m_vScale;
}

void TransformGizmo::InternalDeltaTranslation(const DeltaState& state)
{
	// Cache camera and viewport for further calculations.
	const Camera& camera = state.m_MouseState.m_CameraState.m_Camera;
	const Viewport& viewport = state.m_MouseState.m_CameraState.m_Viewport;

	// Compute the total change as a 2D vector in screen space (mouse space).
	Vector2D const vScreenDelta(
		Vector2D((Float32)state.m_MouseState.m_Current.X, (Float32)state.m_MouseState.m_Current.Y) -
		Vector2D((Float32)m_CapturedMousePosition.X, (Float32)m_CapturedMousePosition.Y));

	// We will compute two motion vectors, in screen and world space.
	Vector2D vScreenAxis0(Vector2D::Zero());
	Vector2D vScreenAxis1(Vector2D::Zero());
	Vector3D vWorldAxis0(Vector3D::Zero());
	Vector3D vWorldAxis1(Vector3D::Zero());

	// Rescale factor for world motion.
	Float fMotionRescale = 1.0f;
	{
		// Axis 0 and 1 dependent on captured handle mode.
		switch (m_eCapturedHandle)
		{
			// Single axis motion.
		case TransformGizmoHandle::kX: vWorldAxis0 = Vector3D::UnitX(); break;
		case TransformGizmoHandle::kY: vWorldAxis0 = Vector3D::UnitY(); break;
		case TransformGizmoHandle::kZ: vWorldAxis0 = Vector3D::UnitZ(); break;

			// Gizmo axes planar motion.
		case TransformGizmoHandle::kXY:
			vWorldAxis0 = Vector3D::UnitX();
			vWorldAxis1 = Vector3D::UnitY();
			break;
		case TransformGizmoHandle::kXZ:
			vWorldAxis0 = Vector3D::UnitX();
			vWorldAxis1 = Vector3D::UnitZ();
			break;
		case TransformGizmoHandle::kYZ:
			vWorldAxis0 = Vector3D::UnitY();
			vWorldAxis1 = Vector3D::UnitZ();
			break;

			// "All" motion for translation is in the camera plane.
		case TransformGizmoHandle::kAll:
			vWorldAxis0 = camera.GetRightAxis();
			vWorldAxis1 = camera.GetUpAxis();
			break;
		default:
			break;
		};

		// For all motion types other than all, we need to
		// apply the global vs. local mode setting. "All"
		// motion is always in the camera plane.
		if (TransformGizmoHandle::kAll != m_eCapturedHandle)
		{
			vWorldAxis0 = InternalApplyGlobalModeCapturedToAxis(vWorldAxis0);
			vWorldAxis1 = InternalApplyGlobalModeCapturedToAxis(vWorldAxis1);
		}

		// Cache captured gizmo center in world and screen space.
		Vector3D const vW0(m_CapturedTransform.m_vTranslation);
		Vector2D const vS0(camera.ConvertWorldToScreenSpace(viewport, vW0).GetXY());

		// TODO: Not correct. This factor should actually be an integral of
		// a functino which varies as we translate the gizmo. The scaling factor
		// which each delta change is dependent on the current position of the gizmo,
		// not the capture position.

		// Motion rescale is the maximum delta magnitude in the camera plane's 2 axes (up and right)
		// at the cached transform position.
		fMotionRescale = Max(
			(camera.ConvertWorldToScreenSpace(viewport, vW0 + camera.GetRightAxis()).GetXY() - vS0).Length(),
			(camera.ConvertWorldToScreenSpace(viewport, vW0 + camera.GetUpAxis()).GetXY() - vS0).Length());

		// Finally, convert the computed world axes into screen axes.
		vScreenAxis0 = Vector2D::Normalize(
			camera.ConvertWorldToScreenSpace(viewport, vW0 + vWorldAxis0).GetXY() - vS0);
		vScreenAxis1 = Vector2D::Normalize(
			camera.ConvertWorldToScreenSpace(viewport, vW0 + vWorldAxis1).GetXY() - vS0);
	}

	// Motion along the 2 axes, rescaled by the motion rescale,
	// which keeps motion relative to distance.
	Float f0 = (Vector2D::Dot(vScreenAxis0, vScreenDelta) / fMotionRescale) * kfTranslationMagnitudeFactor;
	Float f1 = (Vector2D::Dot(vScreenAxis1, vScreenDelta) / fMotionRescale) * kfTranslationMagnitudeFactor;

	// Apply snapping to the two axes of motion if enabled.
	if (m_bTranslationSnap)
	{
		f0 = Round(f0 / m_fTranslationSnapFactor) * m_fTranslationSnapFactor;
		f1 = Round(f1 / m_fTranslationSnapFactor) * m_fTranslationSnapFactor;
	}

	// Compute and apply final delta.
	Vector3D const vDelta = (vWorldAxis0 * f0 + vWorldAxis1 * f1);
	m_Transform.m_vTranslation = m_CapturedTransform.m_vTranslation + vDelta;
}

void TransformGizmo::InternalDraw(const RenderState& state)
{
	// Pick and select the render technique.
	HString const technique(InternalGetEffectTechnique(state.m_bPicking));
	state.m_rRenderer.UseEffectTechnique(technique);

	// TODO: 3.0f * kfInfiniteProjectionEpsilon
	// is a bit of a "magic number" that is setup to be closer
	// than the sky and "infinite" projection mesh draw modes.
	// Need to assemble these into a single constants header file.

	// Switch to an infinite projection - prevent
	// clipping against the far or near planes.
	state.m_rRenderer.UseInfiniteProjection(3.0f * kfInfiniteProjectionEpsilon);

	// Enable normal generation when using the lighting technique.
	state.m_rRenderer.SetGenerateNormals(kEffectTechniqueRenderGizmo == technique);

	switch (m_eMode)
	{
	case TransformGizmoMode::kTranslation: InternalDrawTranslation(state); break;
	case TransformGizmoMode::kRotation: InternalDrawRotation(state); break;
	case TransformGizmoMode::kScale: InternalDrawScale(state); break;
	default:
		break;
	};

	// Disable normal generation.
	state.m_rRenderer.SetGenerateNormals(false);

	// Disable infinite projection.
	state.m_rRenderer.UseInfiniteProjection(false);

	// Switch the technique back to the default.
	state.m_rRenderer.UseEffectTechnique();

	// Always reset clipping.
	state.m_rRenderer.ResetClipValue();
}

void TransformGizmo::InternalDrawRotation(const RenderState& state)
{
	auto const& abVisible = state.m_abVisible;

	// The outline ring, when drawn, is not clipped.
	InternalDrawRotationRing(Axis::kW, state);

	// For rotation, we only want to render the front half of the
	// gizmo. So, compute a clip value based on the view space z
	// of the gizmo center.
	Float32 const fClipValue = -Matrix4D::TransformPosition(state.m_CameraState.m_Camera.GetViewMatrix(), m_Transform.m_vTranslation).Z;
	state.m_rRenderer.SetClipValue(fClipValue);

	// Rotation tori
	if (abVisible[(Int)Axis::kX]) InternalDrawRotationRing(Axis::kX, state);
	if (abVisible[(Int)Axis::kY]) InternalDrawRotationRing(Axis::kY, state);
	if (abVisible[(Int)Axis::kZ]) InternalDrawRotationRing(Axis::kZ, state);
}

void TransformGizmo::InternalDrawRotationRing(
	Axis eAxis,
	const RenderState& state)
{
	static const Float32 kfRadius = 0.08f;
	static const Int32 kiPickSegmentsPerRing = 16;
	static const Int32 kiRenderSegmentsPerRing = 32;
	static const Int32 kiTotalRings = 32;

	Vector3D vAxis(Vector3D::Zero());
	ColorARGBu8 cColor;

	Matrix4D const mTransform(InternalComputeDrawTransformRotation());

	// kW is used as a special value for the outline ring.
	if (Axis::kW == eAxis)
	{
		vAxis = -state.m_CameraState.m_Camera.GetViewAxis();
		cColor = InternalGetAllColor(state.m_bPicking);
	}
	else
	{
		// Normal axes are normal.
		vAxis[(Int)eAxis] = 1.0f;
		vAxis = Matrix4D::TransformDirection(mTransform, vAxis);
		cColor = InternalGetAxisColor(state.m_bPicking, eAxis);
	}

	if (state.m_bPicking)
	{
		// Render rotation tori
		state.m_rRenderer.TriangleTorus(
			mTransform.GetTranslation(),
			vAxis,
			(1.0f - 2.0f * kfRadius) * state.m_fGizmoScale,
			state.m_fGizmoScale,
			kiPickSegmentsPerRing,
			kiTotalRings,
			false,
			cColor);
	}
	else
	{
		// Render rotation circle.
		state.m_rRenderer.LineCircle(
			mTransform.GetTranslation(),
			vAxis,
			state.m_fGizmoScale,
			kiRenderSegmentsPerRing,
			true,
			cColor);
	}
}

void TransformGizmo::InternalDrawScale(const RenderState& state)
{
	auto const& abVisible = state.m_abVisible;

	// Beam, axes, handles of scaling. Order here is important.
	if (abVisible[(Int)Axis::kY] && abVisible[(Int)Axis::kZ]) InternalDrawScalePanel(Axis::kX, state);
	if (abVisible[(Int)Axis::kX] && abVisible[(Int)Axis::kZ]) InternalDrawScalePanel(Axis::kY, state);
	if (abVisible[(Int)Axis::kX] && abVisible[(Int)Axis::kY]) InternalDrawScalePanel(Axis::kZ, state);

	if (abVisible[(Int)Axis::kX]) InternalDrawScaleAxis(Axis::kX, state);
	if (abVisible[(Int)Axis::kY]) InternalDrawScaleAxis(Axis::kY, state);
	if (abVisible[(Int)Axis::kZ]) InternalDrawScaleAxis(Axis::kZ, state);

	if (abVisible[(Int)Axis::kX]) InternalDrawScaleHandle(Axis::kX, state);
	if (abVisible[(Int)Axis::kY]) InternalDrawScaleHandle(Axis::kY, state);
	if (abVisible[(Int)Axis::kZ]) InternalDrawScaleHandle(Axis::kZ, state);

	// Central box, "all" scaling.
	InternalDrawScaleBox(state);
}

void TransformGizmo::InternalDrawScaleAxis(
	Axis eAxis,
	const RenderState& state)
{
	static const Float32 kfHandleLength = 0.2f; // TODO:
	static const Float32 kfRadius = 0.03f; // TODO:
	static const Int32 kiSegmentsPerRing = 16; // TODO:

	Vector3D vAxis(Vector3D::Zero());
	vAxis[(Int)eAxis] = (1.0f - kfHandleLength) * state.m_fGizmoScale;
	Matrix4D const mTransform(InternalComputeDrawTransformScale());
	Vector3D const v0(m_Transform.m_vTranslation);
	Vector3D const v1(Matrix4D::TransformPosition(mTransform, vAxis));

	// Axes - cylinders.
	state.m_rRenderer.TriangleCylinder(
		v0,
		v1,
		kfRadius * state.m_fGizmoScale,
		kiSegmentsPerRing,
		true,
		InternalGetAxisColor(state.m_bPicking, eAxis));
}

void TransformGizmo::InternalDrawScaleBox(const RenderState& state)
{
	static const Float32 kfBoxExtents = 0.1f; // TODO: Break out.

	// All mode is just a center box.
	state.m_rRenderer.TriangleBox(
		InternalComputeDrawTransformScale(),
		Vector3D(kfBoxExtents * state.m_fGizmoScale),
		InternalGetAllColor(state.m_bPicking));
}

void TransformGizmo::InternalDrawScaleHandle(
	Axis eAxis,
	const RenderState& state)
{
	static const Float32 kfBoxExtents = 0.1f; // TODO: Break out.

	Vector3D vAxis(Vector3D::Zero());
	vAxis[(Int)eAxis] = (1.0f - kfBoxExtents) * state.m_fGizmoScale;
	Matrix4D const mTransform(
		InternalComputeDrawTransformScale() *
		Matrix4D::CreateTranslation(vAxis));

	// Handles - boxes.
	state.m_rRenderer.TriangleBox(
		mTransform,
		Vector3D(kfBoxExtents * state.m_fGizmoScale),
		InternalGetAxisColor(state.m_bPicking, eAxis));
}

void TransformGizmo::InternalDrawScalePanel(
	Axis eAxis,
	const RenderState& state)
{
	static const Float32 kfDimension = (Float32)(1.0 / 3.0); // TODO:

	Matrix4D const mTransform(InternalComputeDrawTransformScale());
	Vector3D const v0(m_Transform.m_vTranslation);

	ColorARGBu8 cColor(InternalGetPlaneColor(state.m_bPicking, eAxis));

	Int32 const iAxis0 = ((Int32)eAxis + 1) % 3;
	Int32 const iAxis1 = ((Int32)eAxis + 2) % 3;
	Vector3D vAxis0(Vector3D::Zero());
	vAxis0[(Int)iAxis0] = (1.0f) * state.m_fGizmoScale;
	vAxis0 = InternalApplyGlobalModeCurrentToAxis(vAxis0);
	Vector3D vAxis1(Vector3D::Zero());
	vAxis1[(Int)iAxis1] = (1.0f) * state.m_fGizmoScale;
	vAxis1 = InternalApplyGlobalModeCurrentToAxis(vAxis1);

	Vector3D const v1(v0 + vAxis0 * kfDimension);
	Vector3D const v2(v0 + vAxis1 * kfDimension);
	Vector3D const v3(v0 + vAxis0 * kfDimension + vAxis1 * kfDimension);

	// Plane's facing both directions - slightly modified
	// cColor when not picking for the second quad so its
	// vertices are considered unique and generate unique normals.
	state.m_rRenderer.TriangleQuad(v0, v1, v2, v3, cColor);
	if (!state.m_bPicking)
	{
		if (cColor.m_R < 255) { cColor.m_R += 1; }
		else { cColor.m_R -= 1; }
	}
	state.m_rRenderer.TriangleQuad(v0, v2, v1, v3, cColor);
}

void TransformGizmo::InternalDrawTranslation(const RenderState& state)
{
	auto const& abVisible = state.m_abVisible;

	// Planes, axes, handles of motion. Order here is important.
	if (abVisible[(Int)Axis::kY] && abVisible[(Int)Axis::kZ]) InternalDrawTranslationPanel(Axis::kX, state);
	if (abVisible[(Int)Axis::kX] && abVisible[(Int)Axis::kZ]) InternalDrawTranslationPanel(Axis::kY, state);
	if (abVisible[(Int)Axis::kX] && abVisible[(Int)Axis::kY]) InternalDrawTranslationPanel(Axis::kZ, state);

	if (abVisible[(Int)Axis::kX]) InternalDrawTranslationAxis(Axis::kX, state);
	if (abVisible[(Int)Axis::kY]) InternalDrawTranslationAxis(Axis::kY, state);
	if (abVisible[(Int)Axis::kZ]) InternalDrawTranslationAxis(Axis::kZ, state);

	if (abVisible[(Int)Axis::kX]) InternalDrawTranslationHandle(Axis::kX, state);
	if (abVisible[(Int)Axis::kY]) InternalDrawTranslationHandle(Axis::kY, state);
	if (abVisible[(Int)Axis::kZ]) InternalDrawTranslationHandle(Axis::kZ, state);

	// Central box, "all" motion (dependent on the mode, but usually motion in the camera plane).
	InternalDrawTranslationBox(state);
}

void TransformGizmo::InternalDrawTranslationAxis(
	Axis eAxis,
	const RenderState& state)
{
	static const Float32 kfHandleLength = 0.23f; // TODO:
	static const Float32 kfRadius = 0.03f; // TODO:
	static const Int32 kiSegmentsPerRing = 16; // TODO:

	Vector3D vAxis(Vector3D::Zero());
	vAxis[(Int)eAxis] = (1.0f - kfHandleLength) * state.m_fGizmoScale;
	Matrix4D const mTransform(InternalComputeDrawTransformTranslation());
	Vector3D const v0(m_Transform.m_vTranslation);
	Vector3D const v1(Matrix4D::TransformPosition(mTransform, vAxis));

	// Axes - cylinders.
	state.m_rRenderer.TriangleCylinder(
		v0,
		v1,
		kfRadius * state.m_fGizmoScale,
		kiSegmentsPerRing,
		true,
		InternalGetAxisColor(state.m_bPicking, eAxis));
}

void TransformGizmo::InternalDrawTranslationBox(const RenderState& state)
{
	static const Float32 kfBoxExtents = 0.05f; // TODO: Break out.

	// All mode is just a center box.
	state.m_rRenderer.TriangleBox(
		InternalComputeDrawTransformTranslation(),
		Vector3D(kfBoxExtents * state.m_fGizmoScale),
		InternalGetAllColor(state.m_bPicking));
}

void TransformGizmo::InternalDrawTranslationHandle(
	Axis eAxis,
	const RenderState& state)
{
	static const Float32 kfHandleLength = 0.25f; // TODO:
	static const Float32 kfRadius = 0.1f; // TODO:
	static const Int32 kiSegmentsPerRing = 16; // TODO:

	Vector3D vAxis(Vector3D::Zero());
	Matrix4D const mTransform(InternalComputeDrawTransformTranslation());
	vAxis[(Int)eAxis] = (1.0f - kfHandleLength) * state.m_fGizmoScale;
	Vector3D const v0(Matrix4D::TransformPosition(mTransform, vAxis));
	vAxis[(Int)eAxis] = (1.0f) * state.m_fGizmoScale;
	Vector3D const v1(Matrix4D::TransformPosition(mTransform, vAxis));

	// Handles - cones.
	state.m_rRenderer.TriangleCone(
		v0,
		v1,
		kfRadius * state.m_fGizmoScale,
		kiSegmentsPerRing,
		true,
		InternalGetAxisColor(state.m_bPicking, eAxis));
}

void TransformGizmo::InternalDrawTranslationPanel(
	Axis eAxis,
	const RenderState& state)
{
	static const Float32 kfDimension = (Float32)(1.0 / 3.0); // TODO:

	Matrix4D const mTransform(InternalComputeDrawTransformTranslation());
	Vector3D const v0(m_Transform.m_vTranslation);

	ColorARGBu8 cColor(InternalGetPlaneColor(state.m_bPicking, eAxis));

	Int32 const iAxis0 = ((Int32)eAxis + 1) % 3;
	Int32 const iAxis1 = ((Int32)eAxis + 2) % 3;
	Vector3D vAxis0(Vector3D::Zero());
	vAxis0[iAxis0] = (1.0f) * state.m_fGizmoScale;
	vAxis0 = InternalApplyGlobalModeCurrentToAxis(vAxis0);
	Vector3D vAxis1(Vector3D::Zero());
	vAxis1[iAxis1] = (1.0f) * state.m_fGizmoScale;
	vAxis1 = InternalApplyGlobalModeCurrentToAxis(vAxis1);

	Vector3D const v1(v0 + vAxis0 * kfDimension);
	Vector3D const v2(v0 + vAxis1 * kfDimension);
	Vector3D const v3(v0 + vAxis0 * kfDimension + vAxis1 * kfDimension);

	// Plane's facing both directions - slightly modified
	// cColor when not picking for the second quad so its
	// vertices are considered unique and generate unique normals.
	state.m_rRenderer.TriangleQuad(v0, v1, v2, v3, cColor);
	if (!state.m_bPicking)
	{
		if (cColor.m_R < 255) { cColor.m_R += 1; }
		else { cColor.m_R -= 1; }
	}
	state.m_rRenderer.TriangleQuad(v0, v2, v1, v3, cColor);
}

ColorARGBu8 TransformGizmo::InternalGetAllColor(Bool bPicking) const
{
	if (bPicking)
	{
		return kPickAll;
	}
	else if (!m_bEnabled)
	{
		return kDisabledColor;
	}
	else if (TransformGizmoHandle::kAll == GetCurrentHandleOfInterest())
	{
		return kHighlightColor;
	}
	else
	{
		return ColorARGBu8::White();
	}
}

ColorARGBu8 TransformGizmo::InternalGetAxisColor(Bool bPicking, Axis eAxis) const
{
	// TODO:
	static const ColorARGBu8 kPickColors[] =
	{
		kPickX, // X axis
		kPickY, // Y axis
		kPickZ, // Z axis
	};
	static const ColorARGBu8 kRenderColors[] =
	{
		ColorARGBu8::Red(), // X axis
		ColorARGBu8::Green(), // Y axis
		ColorARGBu8::Blue(), // Z axis
	};
	// /TODO:

	auto const eHandle =
		(TransformGizmoHandle)((Int)TransformGizmoHandle::kX + ((Int)eAxis - (Int)Axis::kX));

	if (bPicking)
	{
		return kPickColors[(Int)eAxis];
	}
	else if (!m_bEnabled)
	{
		return kDisabledColor;
	}
	else if (eHandle == GetCurrentHandleOfInterest())
	{
		return kHighlightColor;
	}
	else
	{
		return kRenderColors[(Int)eAxis];
	}
}

HString TransformGizmo::InternalGetEffectTechnique(Bool bPicking) const
{
	// When picking, or for rotation, don't use lighting.
	if (bPicking || m_eMode == TransformGizmoMode::kRotation)
	{
		return kEffectTechniqueRenderGizmoNoLighting;
	}
	// Otherwise, render with lighting.
	else
	{
		return kEffectTechniqueRenderGizmo;
	}
}

ColorARGBu8 TransformGizmo::InternalGetPlaneColor(Bool bPicking, Axis eAxis) const
{
	// TODO:
	static const UInt8 kAlpha = 127;
	static const ColorARGBu8 kPickColors[] =
	{
		kPickYZ, // X axis
		kPickXZ, // Y axis
		kPickXY, // Z axis
	};
	static const ColorARGBu8 kRenderColors[] =
	{
		ColorARGBu8::Create(255, 0, 0, kAlpha), // X axis
		ColorARGBu8::Create(0, 255, 0, kAlpha), // Y axis
		ColorARGBu8::Create(0, 0, 255, kAlpha), // Z axis
	};
	// /TODO:

	auto const eHandle =
		(TransformGizmoHandle)((Int)TransformGizmoHandle::kYZ + ((Int)eAxis - (Int)Axis::kX));

	if (bPicking)
	{
		return kPickColors[(Int)eAxis];
	}
	else if (!m_bEnabled)
	{
		return kDisabledColor;
	}
	else if (eHandle == GetCurrentHandleOfInterest())
	{
		return kHighlightColor;
	}
	else
	{
		return kRenderColors[(Int)eAxis];
	}
}

Vector3D TransformGizmo::InternalApplyGlobalModeCapturedToAxis(const Vector3D& vAxis) const
{
	// Scaling is always local.
	return (m_bGlobalMode && (m_eMode != TransformGizmoMode::kScale)
		? vAxis
		: Quaternion::Transform(m_CapturedTransform.m_qRotation, vAxis));
}

Vector3D TransformGizmo::InternalApplyGlobalModeCurrentToAxis(const Vector3D& vAxis) const
{
	// Scaling is always local.
	return (m_bGlobalMode && (m_eMode != TransformGizmoMode::kScale)
		? vAxis
		: Quaternion::Transform(m_Transform.m_qRotation, vAxis));
}

} // namespace Seoul::EditorUI

#endif // /#if SEOUL_WITH_SCENE
