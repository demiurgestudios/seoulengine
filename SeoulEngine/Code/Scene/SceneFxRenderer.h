/**
 * \file SceneFxRenderer.h
 * \brief Utility that handles rendering of particles defined
 * in scene Fx.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_FX_RENDERER_H
#define SCENE_FX_RENDERER_H

#include "CheckedPtr.h"
#include "EffectPass.h"
#include "Fx.h"
#include "SharedPtr.h"
#include "Prereqs.h"
#include "Texture.h"
namespace Seoul { class Camera; }
namespace Seoul { class Effect; }
namespace Seoul { class IndexBuffer; }
namespace Seoul { class Material; }
namespace Seoul { struct Matrix4D; }
namespace Seoul { class RenderCommandStreamBuilder; }
namespace Seoul { class VertexBuffer; }
namespace Seoul { class VertexFormat; }

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

class FxRenderer SEOUL_SEALED : public IFxRenderer
{
public:
	typedef Vector<Matrix4D, MemoryBudgets::Scene> WorldStack;

	FxRenderer();
	~FxRenderer();

	void BeginFrame(
		const SharedPtr<Camera>& pCamera,
		RenderCommandStreamBuilder& rBuilder);
	void UseEffect(const SharedPtr<Effect>& pEffect);
	void DrawFx(Fx& fx);
	void EndFrame();
	void PopWorldMatrix();
	const Matrix4D& PushWorldMatrix(const Matrix4D& m);

	virtual Camera& GetCamera() const SEOUL_OVERRIDE;

	virtual Buffer& LockFxBuffer() SEOUL_OVERRIDE;
	virtual void UnlockFxBuffer(
		UInt32 uParticles,
		FilePath textureFilePath,
		FxRendererMode eMode,
		Bool bNeedsScreenAlign) SEOUL_OVERRIDE;

private:
	SharedPtr<Camera> m_pCamera;
	CheckedPtr<RenderCommandStreamBuilder> m_pBuilder;
	Buffer m_vFxBuffer;
	WorldStack m_vWorldStack;
	SharedPtr<Effect> m_pActiveEffect;
	HString m_ActiveEffectTechnique;
	EffectPass m_ActiveEffectPass;
	TextureContentHandle m_hActiveTexture;
	SharedPtr<IndexBuffer> m_pIndexBuffer;
	SharedPtr<VertexBuffer> m_pVertexBuffer;
	SharedPtr<VertexFormat> m_pVertexFormat;

	// TODO: These caches are showing up in a few 3D managers,
	// need to unify and apply cache management.
	typedef HashTable<FilePath, TextureContentHandle, MemoryBudgets::Fx> Cache;
	Cache m_tCache;

	Bool InternalUseEffectTechnique(HString techniqueName);
	void Resolve(FilePath filePath, TextureContentHandle& rh);

	SEOUL_DISABLE_COPY(FxRenderer);
}; // class FxRenderer

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
