/**
 * \file Animation3DCache.h
 * \brief A cache is used to accumulate animation data for a frame,
 * which is then applied to compute the new skeleton pose at
 * the end of animation updating.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION3D_CACHE_H
#define ANIMATION3D_CACHE_H

#include "HashSet.h"
#include "HashTable.h"
#include "Quaternion.h"
#include "Vector.h"
#include "Vector3D.h"
#include "Vector4D.h"

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul::Animation3D
{

struct Cache SEOUL_SEALED
{
	typedef HashTable<Int16, Vector3D, MemoryBudgets::Animation3D> Cache3D;
	typedef HashTable<Int16, Vector4D, MemoryBudgets::Animation3D> Cache4D;
	typedef HashTable<Int16, Quaternion, MemoryBudgets::Animation3D> CacheQ;

	void AccumPosition(Int16 i, const Vector3D& v)
	{
		auto p = m_tPosition.Find(i);
		if (nullptr == p)
		{
			SEOUL_VERIFY(m_tPosition.Insert(i, v).Second);
		}
		else
		{
			*p += v;
		}
	}
	void AccumRotation(Int16 i, const Quaternion& q)
	{
		auto p = m_tRotation.Find(i);
		if (nullptr == p)
		{
			SEOUL_VERIFY(m_tRotation.Insert(i, q).Second);
		}
		else
		{
			*p += q;
		}
	}

	void AccumScale(Int16 i, const Vector3D& v, Float fAlpha)
	{
		auto p = m_tScale.Find(i);
		if (nullptr == p)
		{
			SEOUL_VERIFY(m_tScale.Insert(i, Vector4D(v, fAlpha)).Second);
		}
		else
		{
			*p += Vector4D(v, fAlpha);
		}
	}

	void Clear()
	{
		m_tPosition.Clear();
		m_tRotation.Clear();
		m_tScale.Clear();
	}

	Bool IsDirty() const
	{
		return
			!m_tPosition.IsEmpty() ||
			!m_tRotation.IsEmpty() ||
			!m_tScale.IsEmpty();
	}

	Cache3D m_tPosition;
	CacheQ m_tRotation;
	Cache4D m_tScale;
}; // struct Cache

} // namespace Seoul::Animation3D

#endif // /#if SEOUL_WITH_ANIMATION_3D

#endif // include guard
