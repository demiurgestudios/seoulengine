/**
 * \file UIDrawerSettings.h
 * \brief UI specific Drawer configuration.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_DRAWER_SETTINGS_H
#define UI_DRAWER_SETTINGS_H

#include "FilePath.h"
#include "Prereqs.h"
#include "FalconTextureCacheSettings.h"

namespace Seoul::UI
{

/** Full configuration of a UI::Renderer instance, including Effects and buffer sizes. */
struct DrawerSettings SEOUL_SEALED
{
	DrawerSettings()
		: m_uIndexBufferSizeInIndices(8192u)
		, m_uVertexBufferSizeInVertices(2048u)
		, m_StateEffectFilePath(FilePath::CreateContentFilePath("Authored/Effects/Falcon/FalconState.fx"))
		, m_EffectFilePath(FilePath::CreateContentFilePath("Authored/Effects/Falcon/Falcon.fx"))
		, m_PackEffectFilePath(FilePath::CreateContentFilePath("Authored/Effects/Falcon/Pack.fx"))
		, m_TextureCacheSettings()
	{
	}

	/**
	 * Fixed size of the single dynamic index buffer
	 * used for rendering, in indices. Larger values
	 * allow for large batches but also (potentially) risk
	 * hitting hardware limits or using too much GPU memory.
	 */
	UInt32 m_uIndexBufferSizeInIndices;

	/**
	 * Fixed size of the single dynamic vertex buffer
	 * used for rendering, in vertices. Larger values
	 * allow for large batches but also (potentially) risk
	 * hitting hardware limits or using too much GPU memory.
	 *
	 * Typically, this should be (#-of-indices / 6) * 4, since
	 * most rendering is done using quads.
	 */
	UInt32 m_uVertexBufferSizeInVertices;

	/** Graphics Effect that defines rendering state, set once per frame. */
	FilePath m_StateEffectFilePath;

	/** Graphics effect that is used for rendering, set once per batch. */
	FilePath m_EffectFilePath;

	/** Graphics effect that is used for packing textures into a larger texture atlas. */
	FilePath m_PackEffectFilePath;

	/** Control of Falcon's texture caching system, including the size of the texture atlas. */
	Falcon::TextureCacheSettings m_TextureCacheSettings;
}; // struct UI::DrawerSettings

} // namespace Seoul::UI

#endif // include guard
