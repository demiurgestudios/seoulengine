/**
 * \file EffectTechnique.h
 * \brief Represents a variation in an Effect file. For example, a technique for
 * drawing shadows vs. a technique for drawing lightable material properties.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EFFECT_TECHNIQUE_H
#define EFFECT_TECHNIQUE_H

#include "Prereqs.h"
#include "UnsafeHandle.h"

namespace Seoul
{

class Effect;
class EffectTechnique SEOUL_SEALED
{
public:
	Bool IsValid() const
	{
		return (m_hHandle.IsValid());
	}

	void Reset()
	{
		m_hHandle.Reset();
	}

	Bool operator==(const EffectTechnique& b) const
	{
		return (m_hHandle == b.m_hHandle);
	}

	Bool operator!=(const EffectTechnique& b) const
	{
		return (m_hHandle != b.m_hHandle);
	}

private:
	friend class Effect;
	UnsafeHandle m_hHandle;
};

} // namespace Seoul

#endif // include guard
