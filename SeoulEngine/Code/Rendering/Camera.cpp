/**
 * \file Camera.cpp
 * \brief Camera represents a 3D POV. Mostly used for rendering,
 * can also be used to drive 3D spatially positioned audio.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "Viewport.h"

namespace Seoul
{

SEOUL_TYPE(Camera);

/**
 * Camera constructor
 *
 * Constructs a camera in a default position and rotation with perspective
 * matrix with default parameters.
 */
Camera::Camera()
	: m_vPosition(Vector3D::Zero())
	, m_qRotation(Quaternion::Identity())
	, m_mViewMatrix(Matrix4D::Identity())
	, m_mProjectionMatrix(Matrix4D::Identity())
	, m_Frustum(Frustum::CreateFromViewProjection(Matrix4D::Identity(), Matrix4D::Identity()))
	, m_RelativeViewport(0.0f, 0.0f, 1.0f, 1.0f)
	, m_bEnabled(true)
{
	// Default parameters
	SetPerspective((Float)PiOverTwo, 1.0f, 1.0f, 1000.0f);
}

Camera::~Camera()
{
}

/**
 * Sets the camera's position in world space
 *
 * @param[in] vPosition New camera position in world space
 */
void Camera::SetPosition(Vector3D vPosition)
{
	m_vPosition = vPosition;
	RecomputeViewMatrix();
}

/**
 * Sets the camera's rotation with respect to world space
 *
 * @param[in] qRotation New camera rotation with respect to world space
 */
void Camera::SetRotation(const Quaternion& qRotation)
{
	m_qRotation = qRotation;
	SEOUL_VERIFY(m_qRotation.Normalize());

	RecomputeViewMatrix();
}

/**
 * Apply this Camera's relative viewport rectangle to a parent
 *
 * Precondition: expects parentViewport to be reasonable:
 * - x >= 0
 * - y >= 0
 * - width >= 1
 * - height >= 1
 * - rectangle formed by viewport contained within target width and height.
 */
Viewport Camera::ApplyRelativeViewport(const Viewport& parentViewport) const
{
	// Compute initial rectangle.
	Int32 const iX0 = (Int32)Round(m_RelativeViewport.m_fLeft * (Float32)parentViewport.m_iViewportWidth) + parentViewport.m_iViewportX;
	Int32 const iY0 = (Int32)Round(m_RelativeViewport.m_fTop * (Float32)parentViewport.m_iViewportHeight) + parentViewport.m_iViewportY;
	Int32 const iX1 = (Int32)Round(m_RelativeViewport.m_fRight * (Float32)parentViewport.m_iViewportWidth) + parentViewport.m_iViewportX;
	Int32 const iY1 = (Int32)Round(m_RelativeViewport.m_fBottom * (Float32)parentViewport.m_iViewportHeight) + parentViewport.m_iViewportY;

	// Clamp it to reasonable values and return.
	return Viewport::Create(
		parentViewport.m_iTargetWidth,
		parentViewport.m_iTargetHeight,
		Clamp(iX0, 0, parentViewport.m_iViewportX + parentViewport.m_iViewportWidth - 1),
		Clamp(iY0, 0, parentViewport.m_iViewportY + parentViewport.m_iViewportHeight - 1),
		Max(iX1 - iX0, 1),
		Max(iY1 - iY0, 1));
}

/**
 * Recomputes the view matrix (private use only)
 *
 * Recomputes the view matrix.  This should be called whenever the position
 * or rotation changes.
 */
void Camera::RecomputeViewMatrix()
{
	Matrix4D const mWorldMatrix(Matrix4D::CreateRotationTranslation(
		m_qRotation,
		m_vPosition));

	m_mViewMatrix = mWorldMatrix.OrthonormalInverse();
	m_Frustum.Set(m_mProjectionMatrix, m_mViewMatrix);
}

/**
 * Sets the camera to use a perspective projection
 *
 * Sets the camera to use a perspective projection with the given parameters.
 *
 * @param[in] fFieldOfView Vertical field of view, in radians; must be between 0
 *                         and pi
 * @param[in] fAspectRatio Horizontal-to-vertical field of view aspect ratio
 * @param[in] fNearPlane   Near plane Z-coordinate; must be positive
 * @param[in] fFarPlane    Far plane Z-coordinate; must be larger than
 *                         fNearPlane
 */
void Camera::SetPerspective(
	Float fFieldOfViewInRadians,
	Float fAspectRatio,
	Float fNearPlane,
	Float fFarPlane)
{
	// Max far to near ratio - make sure we have good resolution
	// in the Z-buffer. 7 significant digits in 32-bit floating.
	//
	// Since we've standardized SeoulEngine around 1 unit = 1 meter,
	// this gives us a distance of 2 kilometers (or a max world dimension
	// of 4 kilometers) while maintaining precision down to 0.002 (or
	// 2 millimeters).
	static const Float kFarToNearRatio = 2000.0f;

	// Validate parameters
	if (fFieldOfViewInRadians <= 0.0f ||
		fFieldOfViewInRadians >= fPi ||
		fAspectRatio <= fEpsilon ||
		fNearPlane <= fEpsilon ||
		fFarPlane <= fNearPlane)
	{
		SEOUL_WARN("Invalid camera parameters passed to SetPerspective (%f, %f, %f, %f).",
			fFieldOfViewInRadians,
			fAspectRatio,
			fNearPlane,
			fFarPlane);
		return;
	}

	if (fFarPlane / fNearPlane > kFarToNearRatio + fEpsilon)
	{
		SEOUL_WARN("Far plane to near plane ratio is (%f), which is greater than "
			" the maximum allowed ratio (%f), clamping the far plane.",
			(fFarPlane / fNearPlane),
			kFarToNearRatio);

		fFarPlane = kFarToNearRatio * fNearPlane;
	}

	// Update the projection transform.
	m_mProjectionMatrix = Matrix4D::CreatePerspectiveFromVerticalFieldOfView(
		fFieldOfViewInRadians,
		fAspectRatio,
		fNearPlane,
		fFarPlane);

	// Update the view frustum
	m_Frustum.Set(m_mProjectionMatrix, m_mViewMatrix);
}

/**
 * Sets the camera to use an orthographic projection
 *
 * Sets the camera to use an orthographic projection with the given parameters.
 *
 * @param[in] fLeftPlane   Left plane X-coordinate
 * @param[in] fRightPlane  Right plane X-coordinate
 * @param[in] fBottomPlane Bottom plane Y-coordinate
 * @param[in] fTopPlane    Top plane Y-coordinate
 * @param[in] fNearPlane   Near plane Z-coordinate
 * @param[in] fFarPlane    Far plane Z-coordinate
 */
void Camera::SetOrthographic(
	Float fLeftPlane,
	Float fRightPlane,
	Float fBottomPlane,
	Float fTopPlane,
	Float fNearPlane,
	Float fFarPlane)
{
	SEOUL_ASSERT(fLeftPlane < fRightPlane);
	SEOUL_ASSERT(fBottomPlane < fTopPlane);
	SEOUL_ASSERT(fNearPlane < fFarPlane);

	// Update the projection transform
	m_mProjectionMatrix = Matrix4D::CreateOrthographic(
		fLeftPlane,
		fRightPlane,
		fBottomPlane,
		fTopPlane,
		fNearPlane,
		fFarPlane);

	// Update the view frustum
	m_Frustum.Set(m_mProjectionMatrix, m_mViewMatrix);
}

Float Camera::GetAspectRatio() const
{
	return Matrix4D::ExtractAspectRatio(m_mProjectionMatrix);
}

void Camera::SetAspectRatio(Float fAspectRatio)
{
	m_mProjectionMatrix.UpdateAspectRatio(fAspectRatio);
}

/**
 *  Gets the camera's view (forward) axis.
 *
 *  @return The camera's view (forward) axis
 */
Vector3D Camera::GetViewAxis() const
{
	return -m_mViewMatrix.GetUnitAxis((Int)Axis::kZ);
}

/**
 *  Gets the camera's right axis.
 *
 *  @return The camera's right axis
 */
Vector3D Camera::GetRightAxis() const
{
	return m_mViewMatrix.GetUnitAxis((Int)Axis::kX);
}

/**
 *  Gets the camera's up axis.
 *
 *  @return The camera's up axis
 */
Vector3D Camera::GetUpAxis() const
{
	return m_mViewMatrix.GetUnitAxis((Int)Axis::kY);
}

/**
 *  Converts a world space point vWorldSpace into screen space.
 *
 *  The returned value will be the (x, y) coordinate of the point
 *  in pixels from the upper-left corner, and the z depth value of the point
 *  projected into homogenous clip space, where [0, 1] is between the
 *  near and far planes.
 *
 *  The returned screen space (x, y) value in pixels will be from the
 *  upper-left corner of the provided viewport.
 */
Vector3D Camera::ConvertWorldToScreenSpace(
	const Viewport& parentViewport,
	const Vector3D& vWorldSpace) const
{
	// Compute the final viewport.
	Viewport const viewport = ApplyRelativeViewport(parentViewport);

	// Cache values from the viewport we use to convert
	// the point.
	Float const fViewX = (Float)viewport.m_iViewportX;
	Float const fViewY = (Float)viewport.m_iViewportY;
	Float const fViewWidth = (Float)viewport.m_iViewportWidth;
	Float const fViewHeight = (Float)viewport.m_iViewportHeight;

	// Project the point into clip space.
	Matrix4D const mViewProjection(GetViewProjectionMatrix());
	Vector4D v(Matrix4D::Transform(mViewProjection, Vector4D(vWorldSpace, 1.0f)));

	// Sanity check - return a default if W is zero, which means we have an invalid
	// projection transform.
	if (!IsZero(v.W))
	{
		// Homogenize the point.
		v /= v.W;

		// Convert clip space X and Y into screen space. Z carries through.
		Float const fScreenX = ((1.0f + v.X) * 0.5f * fViewWidth) + fViewX;
		Float const fScreenY = ((1.0f - v.Y) * 0.5f * fViewHeight) + fViewY;
		Float const fScreenZ = v.Z;

		// Done, return the point.
		return Vector3D(fScreenX, fScreenY, fScreenZ);
	}
	else
	{
		return Vector3D(0, 0, -1);
	}
}

/**
 *  Converts a screen space point vScreenSpace into world space.
 *
 *  The Vector3D vScreenSpace is expected to be the same as is returned
 *  from a call to ConvertWorldSpaceToScreenSpace, meaning, (x, y) are the
 *  pixel coordinates of the point from the upper-left corner and z is
 *  the homogenous clip space z depth, where [0, 1] is a depth between the
 *  near and far planes.
 *
 *  The expected screen space (x, y) values in pixels in vScreenSpace
 *  are from the upper-left corner of the provided viewport.
 */
Vector3D Camera::ConvertScreenSpaceToWorldSpace(
	const Viewport& parentViewport,
	const Vector3D& vScreenSpace) const
{
	// Compute the final viewport.
	Viewport const viewport = ApplyRelativeViewport(parentViewport);

	// Cache values from the viewport we use to convert
	// the point.
	Float const fViewX = (Float)viewport.m_iViewportX;
	Float const fViewY = (Float)viewport.m_iViewportY;
	Float const fViewWidth = (Float)viewport.m_iViewportWidth;
	Float const fViewHeight = (Float)viewport.m_iViewportHeight;

	// Normalize X and Y onto [-1, 1], Z carry through.
	Float const fX =  (((vScreenSpace.X - fViewX) / fViewWidth) * 2.0f - 1.0f);
	Float const fY = -(((vScreenSpace.Y - fViewY) / fViewHeight) * 2.0f - 1.0f);
	Float const fZ = vScreenSpace.Z;

	// Build a 4D homogenous point and transform by the invers view-projection transform.
	Vector4D v(fX, fY, fZ, 1.0f);
	v = Matrix4D::Transform(GetInverseViewProjectionMatrix(), v);

	// Sanity check, W can only be zero if a transform is invalid.
	if (!IsZero(v.W))
	{
		// Dehomogenize the point.
		v /= v.W;

		// Done.
		return v.GetXYZ();
	}
	else
	{
		return Vector3D(0, 0, -1);
	}
}

/** @return A Ray3D in world space, at the given screen space position. */
Ray3D Camera::GetWorldRayFromScreenSpace(
	const Viewport& viewport,
	const Point2DInt& screenSpace) const
{
	Vector3D const v0 = ConvertScreenSpaceToWorldSpace(viewport, Vector3D((Float32)screenSpace.X, (Float32)screenSpace.Y, 0.0f));
	Vector3D const v1 = ConvertScreenSpaceToWorldSpace(viewport, Vector3D((Float32)screenSpace.X, (Float32)screenSpace.Y, 1.0f));
	return Ray3D(v0, Vector3D::Normalize(v1 - v0));
}

} // namespace Seoul
