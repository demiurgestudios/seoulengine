/**
 * \file StandardVertex2D.h
 * \brief Core definition of a set of POD structures for 2D rendering.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef STANDARD_VERTEX_2D_H
#define STANDARD_VERTEX_2D_H

#include "Color.h"
#include "Prereqs.h"
#include "SeoulMath.h"
#include "Vector2D.h"

namespace Seoul
{

struct StandardVertex2D SEOUL_SEALED
{
	static StandardVertex2D Create(
		Float fX,
		Float fY,
		RGBA colorMultiply = RGBA::White(),
		RGBA colorAdd = RGBA::TransparentBlack(),
		Float fTX = 0.0f,
		Float fTY = 0.0f)
	{
		StandardVertex2D ret;
		ret.m_vP.X = fX;
		ret.m_vP.Y = fY;
		ret.m_ColorMultiply = colorMultiply;
		ret.m_ColorAdd = ColorAdd::Create(colorAdd);
		ret.m_vT.X = fTX;
		ret.m_vT.Y = fTY;
		ret.m_vT.Z = 0.0f;
		ret.m_vT.W = 0.0f;
		return ret;
	}

	static StandardVertex2D Create(
		Float fX,
		Float fY,
		RGBA colorMultiply,
		ColorAdd colorAdd,
		Float fTX,
		Float fTY)
	{
		StandardVertex2D ret;
		ret.m_vP.X = fX;
		ret.m_vP.Y = fY;
		ret.m_ColorMultiply = colorMultiply;
		ret.m_ColorAdd = colorAdd;
		ret.m_vT.X = fTX;
		ret.m_vT.Y = fTY;
		ret.m_vT.Z = 0.0f;
		ret.m_vT.W = 0.0f;
		return ret;
	}

	static StandardVertex2D Create(
		const Vector2D& v,
		RGBA colorMultiply = RGBA::White(),
		RGBA colorAdd = RGBA::TransparentBlack(),
		Float fTX = 0.0f,
		Float fTY = 0.0f)
	{
		StandardVertex2D ret;
		ret.m_vP.X = v.X;
		ret.m_vP.Y = v.Y;
		ret.m_ColorMultiply = colorMultiply;
		ret.m_ColorAdd = ColorAdd::Create(colorAdd);
		ret.m_vT.X = fTX;
		ret.m_vT.Y = fTY;
		ret.m_vT.Z = 0.0f;
		ret.m_vT.W = 0.0f;
		return ret;
	}

	StandardVertex2D()
		: m_vP()
		, m_ColorMultiply()
		, m_ColorAdd()
		, m_vT()
	{
	}

	Bool operator==(const StandardVertex2D& b) const
	{
		return (
			m_vP == b.m_vP &&
			m_ColorMultiply == b.m_ColorMultiply &&
			m_ColorAdd == b.m_ColorAdd &&
			m_vT == b.m_vT);
	}

	Bool operator!=(const StandardVertex2D& b) const
	{
		return !(*this == b);
	}

	Bool Equals(
		const StandardVertex2D& b,
		Float fTolerance = fEpsilon) const
	{
		return
			m_vP.Equals(b.m_vP, fTolerance) &&
			m_ColorMultiply == b.m_ColorMultiply &&
			m_ColorAdd == b.m_ColorAdd &&
			m_vT.Equals(b.m_vT, fTolerance);
	}

	Vector2D m_vP;
	RGBA m_ColorMultiply;
	ColorAdd m_ColorAdd;
	// TODO: Reduce to Vector2D (again). Vector4D is used
	// for multi-texture cases, which are the exception rather than
	// the rule (detail/face texture on text).
	Vector4D m_vT;
}; // struct StandardVertex2D
SEOUL_STATIC_ASSERT(sizeof(StandardVertex2D) == 32);
template <> struct CanMemCpy<StandardVertex2D> { static const Bool Value = true; };
template <> struct CanZeroInit<StandardVertex2D> { static const Bool Value = true; };

static inline Bool Equals(const StandardVertex2D& a, const StandardVertex2D& b, Float fTolerance = fEpsilon)
{
	return a.Equals(b, fTolerance);
}

} // namespace Seoul

#endif // include guard
