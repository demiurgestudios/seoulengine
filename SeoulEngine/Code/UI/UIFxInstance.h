/**
 * \file UIFxInstance.h
 * \brief SeoulEngine subclass/extension of Falcon::Instance for Fx playback.
 *
 * UI::FxInstance binds the SeoulEngine Fx system into the Falcon scene graph.
 * Fx are rendered with the Falcon renderer and can be freely layered with
 * Falcon scene elements.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_FX_INSTANCE_H
#define UI_FX_INSTANCE_H

#include "CheckedPtr.h"
#include "FalconInstance.h"
#include "Fx.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "ScopedPtr.h"
#include "UIMovieHandle.h"
namespace Seoul { class Fx; }
namespace Seoul { namespace UI { class Movie; } }
namespace Seoul { namespace UI { class FxRenderer; } }
namespace Seoul { struct Vector3D; }

namespace Seoul::UI
{

namespace FxInstanceFlags
{
	enum
	{
		// When set, initializes initial position in worldspace.
		kInitPositionInWorldspace = (1 << 30u),
		// When set, the animation system will update the FX position
		kFollowBone = (1 << 31u),
	};
} // namespace UI::FxInstanceFlags

/** Custom subclass of Falcon::Instance, implements binding of Seoul::Fx into the Falcon graph. */
class FxInstance SEOUL_SEALED : public Falcon::Instance
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(FxInstance);

	FxInstance(const Movie& owner, Fx* pFx, UInt32 uFlags, const SharedPtr<Falcon::Instance>& pParentIfWorldspace);
	void Init(const Vector2D& vLocalOrWorldPosition);
	~FxInstance();

	virtual Instance* Clone(Falcon::AddInterface& rInterface) const SEOUL_OVERRIDE
	{
		FxInstance* pReturn = SEOUL_NEW(MemoryBudgets::UIRuntime) FxInstance;
		CloneTo(rInterface, *pReturn);
		return pReturn;
	}

	// Falcon::Instance overrides.
	virtual bool ComputeLocalBounds(Falcon::Rectangle& rBounds) SEOUL_OVERRIDE;

	virtual void Pose(
		Falcon::Render::Poser& rPoser,
		const Matrix2x3& mParent,
		const Falcon::ColorTransformWithAlpha& cxParent) SEOUL_OVERRIDE;

	virtual void Draw(
		Falcon::Render::Drawer& rDrawer,
		const Falcon::Rectangle& worldBoundsPreClip,
		const Matrix2x3& mWorld,
		const Falcon::ColorTransformWithAlpha& cxWorld,
		const Falcon::TextureReference& textureReference,
		Int32 iSubInstanceId) SEOUL_OVERRIDE;

	virtual Bool HitTest(
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY,
		Bool bIgnoreVisibility = false) const SEOUL_OVERRIDE;

	virtual Falcon::InstanceType GetType() const SEOUL_OVERRIDE;
	// /Falcon::Instance overrides.

	Bool SetRallyPoint(const Vector3D& vRallyPoint);

	// Fx tagged as "treat as looping" do not actually loop, but
	// certain checks and systems assume they will be triggered
	// over and over again, repeatedly.
	Bool GetTreatAsLooping() const
	{
		return m_bTreatAsLooping;
	}
	void SetTreatAsLooping(Bool b)
	{
		m_bTreatAsLooping = b;
	}
	void Stop(Bool bStopImmediately);

	FxProperties GetProperties() const;

	// TODO: Likely, selectively implementing
	// Depth3D will result in unexpected behavior.
	//
	// We should just push the 3D depth down into Falcon::Instance.
	virtual Float GetDepth3D() const SEOUL_OVERRIDE
	{
		return m_fDepth3DBias + (m_pDepth3DSource.IsValid() ? m_pDepth3DSource->GetDepth3D() : m_fDepth3D);
	}

	const SharedPtr<Falcon::Instance>& GetParentIfWorldspace() const
	{
		return m_pParentIfWorldspace;
	}

	// TODO: Likely, selectively implementing
	// Depth3D will result in unexpected behavior.
	//
	// We should just push the 3D depth down into Falcon::Instance.
	virtual void SetDepth3D(Float f) SEOUL_OVERRIDE
	{
		m_fDepth3D = f;
	}

	/**
	 * Bias applied to the 3D depth - used to offset from source depth3D
	 * or base depth 3D.
	 */
	void SetDepth3DBias(Float f)
	{
		m_fDepth3DBias = f;
	}

	/**
	 * Similar to parent attachment, but explicit to providing a source
	 * of 3D depth. Useful for parent mixing.
	 */
	void SetDepthSource(const SharedPtr<Falcon::Instance>& p)
	{
		m_pDepth3DSource = p;
	}

	/**
	 * Update the parent used if a particle is set to world space (by default,
	 * world space particles have no further parent, but for special cases,
	 * a fallback reference parent can be used.
	 */
	void SetParentIfWorldspace(const SharedPtr<Falcon::Instance>& pParentIfWorldspace)
	{
		m_pParentIfWorldspace = pParentIfWorldspace;
	}

	// Custom tick functions, so Fx can run at 60 fps.
	void Tick(Float fDeltaTimeInSeconds);

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(FxInstance);

	FxInstance();

	void CloneTo(Falcon::AddInterface& rInterface, FxInstance& rClone) const;
	Matrix4D GetFxWorldTransform(Falcon::Instance const& instance) const;

	friend class Movie;
	ScopedPtr<FxRenderer> m_pRenderer;
	MovieHandle m_hOwner;
	ScopedPtr<Fx> m_pFx;
	UInt32 m_uFlags;
	Float m_fDepth3D;
	Float m_fDepth3DBias;
	Bool m_bWaitingToStartFX;
	Bool m_bTreatAsLooping;
	SharedPtr<Falcon::Instance> m_pDepth3DSource;
	SharedPtr<Falcon::Instance> m_pParentIfWorldspace;

	SEOUL_DISABLE_COPY(FxInstance);
}; // class UI::FxInstance

} // namespace Seoul::UI

#endif // include guard
