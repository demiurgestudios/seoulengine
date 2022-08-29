/**
 * \file SceneRenderer.h
 * \brief Root utility that handles scene rendering. Contains
 * both an FxRenderer and a MeshRenderer.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_RENDERER_H
#define SCENE_RENDERER_H

#include "Color.h"
#include "Effect.h"
#include "HashSet.h"
#include "ScopedPtr.h"
#include "SharedPtr.h"
namespace Seoul { class Camera; }
namespace Seoul { struct Frustum; }
namespace Seoul { class RenderCommandStreamBuilder; }
namespace Seoul { class RenderPass; }
namespace Seoul { namespace Scene { class FxRenderer; } }
namespace Seoul { namespace Scene { class MeshRenderer; } }
namespace Seoul { namespace Scene { class Object; } }

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

struct RendererConfig SEOUL_SEALED
{
	RendererConfig()
		: m_FxEffectFilePath()
		, m_MeshEffectFilePath()
	{
	}

	/** FilePath of the Microsoft FX to use for particle FX rendering. */
	FilePath m_FxEffectFilePath;

	/** FilePath of the Microsoft FX to use for mesh rendering. */
	FilePath m_MeshEffectFilePath;
}; // struct RendererSettings

class Renderer SEOUL_SEALED
{
public:
	typedef Vector<SharedPtr<Camera>, MemoryBudgets::Rendering> Cameras;
	typedef Vector<SharedPtr<Object>, MemoryBudgets::SceneObject> Objects;
	typedef HashTable<ColorARGBu8, SharedPtr<Object>, MemoryBudgets::Scene> PickTable;

	Renderer(const RendererConfig& config);
	~Renderer();

	void Configure(const RendererConfig& config);

	const Content::Handle<Effect>& GetFxEffect() const { return m_hFxEffect; }
	const Content::Handle<Effect>& GetMeshEffect() const { return m_hMeshEffect; }

	void Pick(
		const SharedPtr<Camera>& pCamera,
		const Objects& vObjects,
		RenderPass& rPass,
		RenderCommandStreamBuilder& rBuilder,
		PickTable& rtPickTable);

	// HighlightedObjects is an editor only hook for drawing selection highlights.
	typedef HashSet<SharedPtr<Object>, MemoryBudgets::Editor> HighlightedObjects;
	void Render(
		const Cameras& vCameras,
		const Objects& vObjects,
		RenderPass& rPass,
		RenderCommandStreamBuilder& rBuilder,
		HighlightedObjects const* pHighlighted = nullptr);

private:
	Bool InternalPickMeshes(
		const SharedPtr<Camera>& pCamera,
		const Objects& vObjects,
		RenderPass& rPass,
		RenderCommandStreamBuilder& rBuilder,
		UInt32& ruPickValue,
		PickTable& rtPickTable);
	void InternalRenderFx(
		const SharedPtr<Camera>& pCamera,
		const Objects& vObjects,
		RenderCommandStreamBuilder& rBuilder);
	void InternalInnerRenderFx(
		const SharedPtr<Camera>& pCamera,
		const Frustum& frustum,
		const Objects& vObjects,
		RenderCommandStreamBuilder& rBuilder);
	void InternalRenderMeshes(
		const SharedPtr<Camera>& pCamera,
		const Objects& vObjects,
		RenderCommandStreamBuilder& rBuilder,
		HighlightedObjects const* pHighlighted = nullptr);
	Bool InternalInnerRenderMeshes(
		const SharedPtr<Camera>& pCamera,
		const Frustum& frustum,
		const Objects& vObjects,
		RenderCommandStreamBuilder& rBuilder,
		HString techniqueOverride,
		HString skinnedTechniqueOverride,
		HighlightedObjects const* pHighlighted = nullptr);

	RendererConfig m_Config;
	Content::Handle<Effect> m_hFxEffect;
	Content::Handle<Effect> m_hMeshEffect;
	ScopedPtr<FxRenderer> m_pFxRenderer;
	ScopedPtr<MeshRenderer> m_pMeshRenderer;

	SEOUL_DISABLE_COPY(Renderer);
}; // class Renderer

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
