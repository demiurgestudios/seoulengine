/**
 * \file EffectManager.h
 * \brief Singleton manager for loading graphics effects and
 * guaranteeing that each loaded Effect is unique.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EFFECT_MANAGER_H
#define EFFECT_MANAGER_H

#include "ContentStore.h"
#include "Effect.h"
#include "FilePath.h"

namespace Seoul
{

/**
 * Singleton manager for loading Effects and guaranteeing
 * that each loaded Effect is unique.
 */
class EffectManager SEOUL_SEALED : public Singleton<EffectManager>
{
public:
	SEOUL_DELEGATE_TARGET(EffectManager);

	EffectManager();
	~EffectManager();

	// Return the amount of graphics memory being used by active shader effects.
	UInt32 GetEffectGraphicsMemoryUsageInBytes() const;

	/**
	 * @return A persistent Content::Handle<> to the effect filePath.
	 */
	EffectContentHandle GetEffect(FilePath filePath)
	{
		return m_Content.GetContent(filePath);
	}

	// Unsets textures from all Effects - should be called at the end of a frame
	// to prevent dangling references before content unloads.
	void UnsetAllTextures();

private:
	Content::Store<Effect> m_Content;

	SEOUL_DISABLE_COPY(EffectManager);
}; // class EffectManager

} // namespace Seoul

#endif // include guard
