/**
 * \file Camera.h
 * \brief Camera represents a 3D POV. Mostly used for rendering,
 * can also be used to drive 3D spatially positioned audio.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include "Frustum.h"
#include "Geometry.h"
#include "Quaternion.h"
#include "Ray3D.h"
#include "Vector3D.h"
namespace Seoul { struct Viewport; }

namespace Seoul
{

/**
 * This class represents a single camera in the world.  A camera encapsulates
 * the transformation of world space coordinates into screen space coordinates,
 * via a view transformation and a projection transformation.
 *
 * The view transformation accounts for the camera's position and rotation
 * in world space, and it is an orthonormal transformation (that is, it consists
 * only of translations and rigid rotations, so it preserves geometry, i.e.
 * distances and angles).
 *
 * The projection transformation is an affine transformation which projects
 * model-view space into homogeneous 4D screen space.  The Camera class supports
 * two types of projections: perspective and orthographic.  A perspective
 * projection is defined by a field of view, an aspect ratio, a near plane, and
 * a far plane, which define the 6 planes of the viewing frustum.  An
 * orthographic projection is defined by a left, right, bottom, top, near, and
 * far planes.
 */
class Camera SEOUL_SEALED
{
public:
	Camera();
	~Camera();

	/**
	 * Gets the camera's current position in world space
	 *
	 * @return The camera's current position in world space
	 */
	const Vector3D& GetPosition() const
	{
		return m_vPosition;
	}

	// Sets the camera's position in world space
	void SetPosition(Vector3D vPosition);

	/**
	 * Gets the camera's current rotation with respect to world space
	 *
	 * @return The camera's current rotation with respect to world space
	 */
	const Quaternion& GetRotation() const
	{
		return m_qRotation;
	}

	// Sets the camera's rotation with respect to world space
	void SetRotation(const Quaternion& qRotation);

	/**
	 * Gets the camera's current projection matrix
	 *
	 * Gets the camera's current projection matrix.  The projection matrix expresses
	 * the transformation from camera space into homogeneous screen space.  It is
	 * either a perspective transform matrix or an orthographic transform matrix,
	 * depending on the current camera settings.
	 *
	 * @return The camera's current projection matrix.
	 */
	const Matrix4D& GetProjectionMatrix() const
	{
		return m_mProjectionMatrix;
	}

	/**
	 * Gets the camera's current view matrix
	 *
	 * Gets the camera's current view matrix.  The view matrix expresses the
	 * transformation from world space into camera space, taking into account the
	 * camera's current position and rotation.
	 *
	 * @return The camera's current view matrix.
	 */
	const Matrix4D& GetViewMatrix() const
	{
		return m_mViewMatrix;
	}

	/**
	 * @return The inverse of the camera's view-projection matrix.
	 */
	Matrix4D GetInverseViewProjectionMatrix() const
	{
		return GetViewProjectionMatrix().Inverse();
	}

	/**
	 * Gets the camera's current view x projection matrix
	 *
	 * Gets the camera's current view x projection matrix.  The
	 * view x projection matrix expresses the transformation from
	 * world space into camera projection space, taking into account the
	 * camera's current position and rotation and its camera
	 * frustum properties (FOV, aspect ratio, etc.).
	 *
	 * @return The camera's current view x projection matrix.
	 */
	Matrix4D GetViewProjectionMatrix() const
	{
		return (m_mProjectionMatrix * m_mViewMatrix);
	}

	void SetPerspective(
		Float fFieldOfViewInRadians,
		Float fAspectRatio,
		Float fNearPlane,
		Float fFarPlane);

	void SetOrthographic(
		Float fLeftPlane,
		Float fRightPlane,
		Float fBottomPlane,
		Float fTopPlane,
		Float fNearPlane,
		Float fFarPlane);

	Float GetAspectRatio() const;
	void SetAspectRatio(Float fAspectRatio);

	// Gets the camera's view (forward) axis.
	Vector3D GetViewAxis() const;
	// Gets the camera's right axis.
	Vector3D GetRightAxis() const;
	// Gets the camera's up axis.
	Vector3D GetUpAxis() const;

	/** The camera frustum, a 6-sided convex bounding volume
	*  defined by 6 planes.
	*/
	const Frustum& GetFrustum() const
	{
		return m_Frustum;
	}
	
	/** Return vWorldSpace as a screen space point, with Z on [0, 1]. */
	Vector3D ConvertWorldToScreenSpace(
		const Viewport& parentViewport,
		const Vector3D& vWorldSpace) const;

	/** Return vScreenSpcae as a world space point. Input Z should be on [0, 1]. */
	Vector3D ConvertScreenSpaceToWorldSpace(
		const Viewport& parentViewport,
		const Vector3D& vScreenSpace) const;

	/**
	 * @return true if this Camera is enabled, false otherwise.
	 *
	 * Enabled state does not affect internal Camera behavior. It can be used
	 * by client code to mark a Camera as "on" or "off.
	 */
	Bool GetEnabled() const
	{
		return m_bEnabled;
	}

	/** Update whether this Camera is enabled or disabled. */
	void SetEnabled(Bool bEnabled)
	{
		m_bEnabled = bEnabled;
	}

	// Apply this Camera's relative viewport rectangle to a parent Viewport.
	Viewport ApplyRelativeViewport(const Viewport& parentViewport) const;

	/** Get the current relative viewport - defaults to [0, 1], [0, 1]. */
	const Rectangle2D& GetRelativeViewport() const
	{
		return m_RelativeViewport;
	}

	/**
	 * Relative factors, defines the subregion to which this Camera
	 * renders. Used in ConvertWorldToScreenSpace() and
	 * ConvertScreenSpaceToWorldSpace(), applied to the passed in
	 * viewport to compute the final, total viewport.
	 */
	void SetRelativeViewport(const Rectangle2D& rect)
	{
		m_RelativeViewport = rect;
	}

	/** @return A Ray3D in world space, at the given screen space position. */
	Ray3D GetWorldRayFromScreenSpace(
		const Viewport& viewport,
		const Point2DInt& screenSpace) const;

private:
	SEOUL_REFERENCE_COUNTED(Camera);

	void RecomputeViewMatrix();

	/** Position in world space */
	Vector3D m_vPosition;

	/** Rotation in world space */
	Quaternion m_qRotation;

	/** View matrix, transforms world space into camera space */
	Matrix4D m_mViewMatrix;

	/** Projection matrix, transforms camera space into inhomogeneous screen space */
	Matrix4D m_mProjectionMatrix;

	/** View frustum in world space */
	Frustum m_Frustum;

	/** Viewport rectangle of this Camera, relative to a parent Viewport. */
	Rectangle2D m_RelativeViewport;

	/** Enable/disable tracking of a camera. Used outside the Camera class. */
	Bool m_bEnabled;
}; // class Camera

} // namespace Seoul

#endif // include guard

