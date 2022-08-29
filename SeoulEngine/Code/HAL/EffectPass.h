/**
 * \file EffectPass.h
 * \brief One pass in a multi-pass shader Effect.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EFFECT_PASS_H
#define EFFECT_PASS_H

#include "Prereqs.h"
namespace Seoul { class RenderCommandStreamBuilder; }

namespace Seoul
{

/**
 * Effects can have multiple passes. EffectPass represents one pass
 * in a multi-pass Effect.
 */
class EffectPass SEOUL_SEALED
{
public:
	EffectPass(UInt16 uPassIndex = 0u, UInt16 uPassCount = 0u)
		: m_uPassIndex(uPassIndex)
		, m_uPassCount(uPassCount)
	{
	}

	Bool IsValid() const
	{
		return (m_uPassIndex < m_uPassCount);
	}

	void Reset()
	{
		m_uPassIndex = 0;
		m_uPassCount = 0;
	}

	/**
	 * Gets the next pass in a multi-pass chain.
	 *
	 * If this is the last pass in the chain, the EffectPass object
	 * returned by this function will not be valid.
	 */
	EffectPass Next() const
	{
		UInt16 const uNext = (m_uPassIndex + 1u);
		if (uNext < m_uPassCount)
		{
			EffectPass ret(uNext, m_uPassCount);
			return ret;
		}
		else
		{
			return EffectPass();
		}
	}

private:
	friend class RenderCommandStreamBuilder;
	UInt16 m_uPassIndex;
	UInt16 m_uPassCount;
}; // class EffectPass

} // namespace Seoul

#endif // include guard
