/**
 * \file Viewport.h
 * \brief Structure that defines the properties of the current rendering
 * viewport.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "Prereqs.h"

namespace Seoul
{

/**
 * Viewport defines the properties of a rendering viewport.
 */
struct Viewport SEOUL_SEALED
{
	static Viewport Create(
		Int32 iTargetWidth,
		Int32 iTargetHeight,
		Int32 iViewportX,
		Int32 iViewportY,
		Int32 iViewportWidth,
		Int32 iViewportHeight)
	{
		Viewport ret;
		ret.m_iTargetWidth = iTargetWidth;
		ret.m_iTargetHeight = iTargetHeight;
		ret.m_iViewportX = iViewportX;
		ret.m_iViewportY = iViewportY;
		ret.m_iViewportWidth = iViewportWidth;
		ret.m_iViewportHeight = iViewportHeight;
		return ret;
	}

	Viewport()
		: m_iTargetWidth(0)
		, m_iTargetHeight(0)
		, m_iViewportX(0)
		, m_iViewportY(0)
		, m_iViewportWidth(0)
		, m_iViewportHeight(0)
	{
	}

	Bool operator==(const Viewport& b) const
	{
		return (
			m_iTargetWidth == b.m_iTargetWidth &&
			m_iTargetHeight == b.m_iTargetHeight &&
			m_iViewportX == b.m_iViewportX &&
			m_iViewportY == b.m_iViewportY &&
			m_iViewportWidth == b.m_iViewportWidth &&
			m_iViewportHeight == b.m_iViewportHeight);
	}

	Bool operator!=(const Viewport& b) const
	{
		return !(*this == b);
	}

	Int32 m_iTargetWidth;
	Int32 m_iTargetHeight;
	Int32 m_iViewportX;
	Int32 m_iViewportY;
	Int32 m_iViewportWidth;
	Int32 m_iViewportHeight;

	/** Compute the center position of this viewport on X. */
	Float GetViewportCenterX() const { return (Float)m_iViewportX + (Float)m_iViewportWidth * 0.5f; }
	/** Compute the center position of this viewport on Y. */
	Float GetViewportCenterY() const { return (Float)m_iViewportY + (Float)m_iViewportHeight * 0.5f; }
	/** Compute the right edge of the viewport. */
	Int32 GetViewportRight() const { return (m_iViewportX + m_iViewportWidth); }
	/** Compute the bottom edge of the viewport. */
	Int32 GetViewportBottom() const { return (m_iViewportY + m_iViewportHeight); }

	/**
	 * Gets the aspect ratio of the full render target defined
	 * by this viewport structure.
	 */
	Float GetTargetAspectRatio() const
	{
		return (Float)m_iTargetWidth / (Float)m_iTargetHeight;
	}

	/**
	 * Gets the aspect ratio of the viewport that is defined
	 * by this viewport structure, which is <= the dimensions
	 * of the entire render target.
	 */
	Float GetViewportAspectRatio() const
	{
		return (Float)m_iViewportWidth / (Float)m_iViewportHeight;
	}

	/**
	 * Convenience, returns true if the viewport region intersects the given point.
	 */
	Bool Intersects(const Point2DInt& p) const
	{
		if (p.X < m_iViewportX) { return false; }
		if (p.Y < m_iViewportY) { return false; }
		if (p.X >= GetViewportRight()) { return false; }
		if (p.Y >= GetViewportBottom()) { return false; }

		return true;
	}

	/**
	 * Convenience, returns true if the viewport region intersects the given point.
	 */
	Bool Intersects(const Vector2D& v) const
	{
		if (v.X < (Float)m_iViewportX) { return false; }
		if (v.Y < (Float)m_iViewportY) { return false; }
		if (v.X >= (Float)GetViewportRight()) { return false; }
		if (v.Y >= (Float)GetViewportBottom()) { return false; }

		return true;
	}
}; // struct Viewport

/**
 * @return A viewport compatible with SetScissor() that has been oversized (safely)
 * to allow a 1-pixel clear border around a render viewport viewport.
 */
inline Viewport ToClearSafeScissor(const Viewport& viewport)
{
	Viewport ret(viewport);

	if (ret.m_iViewportX + ret.m_iViewportWidth + 1 <= ret.m_iTargetWidth)
	{
		ret.m_iViewportWidth++;
	}

	if (ret.m_iViewportY + ret.m_iViewportHeight + 1 <= ret.m_iTargetHeight)
	{
		ret.m_iViewportHeight++;
	}

	return ret;
}

} // namespace Seoul

#endif // include guard
