/**
 * \file SceneFxComponent.h
 * \brief Binds a visual fx (typically particles, but can
 * be more than that) into a Scene object.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_FX_COMPONENT_H
#define SCENE_FX_COMPONENT_H

#include "Fx.h"
#include "Matrix4D.h"
#include "SceneComponent.h"
namespace Seoul { struct Frustum; }
namespace Seoul { namespace Scene { class FxRenderer; } }

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

class FxComponent SEOUL_SEALED : public Component
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(FxComponent);

	FxComponent();
	~FxComponent();

	virtual SharedPtr<Component> Clone(const String& sQualifier) const SEOUL_OVERRIDE;

	FilePath GetFxFilePath() const
	{
		return m_pFx.IsValid() ? m_pFx->GetFilePath() : FilePath();
	}

	Float GetFxDuration() const;

	/**
	 * @return True if this Fx contains renderable components, false otherwise.
	 *
	 * This value will change and is only accurate after a successful call
	 * to StartFx().
	 */
	Bool NeedsRender() const
	{
		return m_bNeedsRender;
	}

	void Render(
		const Frustum& frustum,
		FxRenderer& rRenderer);

	void SetFxFilePath(FilePath filePath);
	Bool StartFx();
	void StopFx(Bool bStopImmediately = false);
	void Tick(Float fDeltaTimeInSeconds);

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(FxComponent);

	SEOUL_REFLECTION_FRIENDSHIP(FxComponent);
	ScopedPtr<Fx> m_pFx;
	Bool m_bStartOnLoad;
	Bool m_bStarted;
	Bool m_bNeedsRender;

	SEOUL_DISABLE_COPY(FxComponent);
}; // class FxComponent

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
