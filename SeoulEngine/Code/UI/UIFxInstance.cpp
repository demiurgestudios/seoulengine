/**
 * \file UIFxInstance.cpp
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

#include "FalconClipper.h"
#include "FalconMovieClipInstance.h"
#include "FalconRenderDrawer.h"
#include "Fx.h"
#include "ParticleEmitterInstance.h"
#include "ReflectionDefine.h"
#include "Renderer.h"
#include "UIFxInstance.h"
#include "UIFxRenderer.h"
#include "UIManager.h"
#include "UIMovie.h"
#include "UIRenderer.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(UI::FxInstance, TypeFlags::kDisableNew)
	SEOUL_PARENT(Falcon::Instance)
	SEOUL_PROPERTY_PAIR_N("Depth3D", GetDepth3D, SetDepth3D)
SEOUL_END_TYPE()

namespace UI
{

FxInstance::FxInstance(
	const Movie& owner,
	Fx* pFx,
	UInt32 uFlags,
	const SharedPtr<Falcon::Instance>& pParentIfWorldspace)
	: Falcon::Instance(0)
	, m_pRenderer(SEOUL_NEW(MemoryBudgets::Fx) FxRenderer)
	, m_hOwner(owner.GetHandle())
	, m_pFx(pFx)
	// UI particles are always purely 2D or parallax 2D,
	// so we want all particles to be force snapped to the
	// emitter Z.
	, m_uFlags(uFlags | ParticleEmitterInstance::kForceSnapZ)
	, m_fDepth3D(0.0f)
	, m_fDepth3DBias(0.0f)
	, m_bWaitingToStartFX(false)
	, m_bTreatAsLooping(false)
	, m_pDepth3DSource()
	, m_pParentIfWorldspace(pParentIfWorldspace)
{
	// Let our owner know.
	CheckedPtr<Movie> pOwner(GetPtr<Movie>(m_hOwner));
	if (pOwner.IsValid())
	{
		pOwner->AddActiveFx(this);
	}
	// If owner add fails, release any dependencies, since we're
	// essentially a dead Fx instance anyway, and holding on to
	// the parent may leave a cycle.
	else
	{
		m_pDepth3DSource.Reset();
		m_pParentIfWorldspace.Reset();
	}
}

void FxInstance::Init(const Vector2D& vLocalOrWorldPosition)
{
	// Update the node position - either local or world
	// based on the local position flag.
	if (FxInstanceFlags::kInitPositionInWorldspace == (FxInstanceFlags::kInitPositionInWorldspace & m_uFlags))
	{
		SetWorldPosition(vLocalOrWorldPosition.X, vLocalOrWorldPosition.Y);
	}
	else
	{
		SetPosition(vLocalOrWorldPosition.X, vLocalOrWorldPosition.Y);
	}

	m_bWaitingToStartFX = false;
	if (!(m_pFx->Start(GetFxWorldTransform(*this), m_uFlags)) && m_pFx->IsLoading())
	{
		// Effect could not start because FX are still loading. Try in a later frame.
		m_bWaitingToStartFX = true;
	}
}

FxInstance::~FxInstance()
{
	// Let our owner know.
	CheckedPtr<Movie> pOwner(GetPtr<Movie>(m_hOwner));
	if (pOwner.IsValid())
	{
		pOwner->RemoveActiveFx(this);
	}
}

Bool FxInstance::ComputeLocalBounds(Falcon::Rectangle& rBounds)
{
	// TODO: Need to implement this for Fx integration.
	return false;
}

void FxInstance::Pose(
	Falcon::Render::Poser& rPoser,
	const Matrix2x3& mParent,
	const Falcon::ColorTransformWithAlpha& cxParent)
{
	if (!GetVisible())
	{
		return;
	}

	rPoser.PushDepth3D(0.0f, GetIgnoreDepthProjection()); // Only ignore, depth is handled by renderer.
	m_pRenderer->BeginPose(rPoser);
	m_pFx->Draw(*m_pRenderer);
	m_pRenderer->EndPose();
	rPoser.PopDepth3D(0.0f, GetIgnoreDepthProjection()); // Only ignore, depth is handled by renderer.
}

void FxInstance::Draw(
	Falcon::Render::Drawer& /*rDrawer*/,
	const Falcon::Rectangle& /*worldBoundsPreClip*/,
	const Matrix2x3& /*mWorld*/,
	const Falcon::ColorTransformWithAlpha& /*cxWorld*/,
	const Falcon::TextureReference& /*textureReference*/,
	Int32 /*iSubInstanceId*/)
{
	// Nop, handled by m_pRenderer.
}

Bool FxInstance::HitTest(
	const Matrix2x3& mParent,
	Float32 fWorldX,
	Float32 fWorldY,
	Bool bIgnoreVisibility /*= false*/) const
{
	// We never treat Fx as hit testable.
	return false;
}

Falcon::InstanceType FxInstance::GetType() const
{
	return Falcon::InstanceType::kFx;
}

Bool FxInstance::SetRallyPoint(const Vector3D& vRallyPoint)
{
	return m_pFx->SetRallyPoint(vRallyPoint);
}

void FxInstance::Stop(Bool bStopImmediately)
{
	return m_pFx->Stop(bStopImmediately);
}

FxProperties FxInstance::GetProperties() const
{
	FxProperties props;
	(void)m_pFx->GetProperties(props);
	return props;
}

void FxInstance::Tick(Float fDeltaTimeInSeconds)
{
	CheckedPtr<Movie> pOwner(GetPtr<Movie>(m_hOwner));

	if (m_bWaitingToStartFX)
	{
		// Try to start the effect, even if still loading
		if (!(m_pFx->Start(GetFxWorldTransform(*this), m_uFlags)))
		{
			// Failed to start. If we are done loading, don't try again. Give up.
			if (!(m_pFx->IsLoading()))
			{
				m_bWaitingToStartFX = false;
			}
			// else keep trying
		}
		else
		{
			if (m_pParentIfWorldspace.IsValid())
			{
				m_pFx->SetParentIfWorldspace(GetFxWorldTransform(*m_pParentIfWorldspace));
			}
			// Effect went off
			m_bWaitingToStartFX = false;
		}
	}
	else
	{
		// Effect has had a chance to play - check for stop
		if (!m_pFx->IsPlaying())
		{
			if (pOwner.IsValid())
			{
				pOwner->QueueFxToRemove(this);
			}
		}
		else
		{
			// Early out if not reachable/visible.
			CheckedPtr<Movie> pOwner(GetPtr<Movie>(m_hOwner));
			if (!pOwner.IsValid() || !pOwner->IsReachableAndVisible(this))
			{
				return;
			}

			m_pFx->SetTransform(GetFxWorldTransform(*this));

			if (m_pParentIfWorldspace.IsValid())
			{
				m_pFx->SetParentIfWorldspace(GetFxWorldTransform(*m_pParentIfWorldspace));
			}

			m_pFx->Tick(fDeltaTimeInSeconds);
		}
	}
}

FxInstance::FxInstance()
	: Falcon::Instance(0)
	, m_hOwner()
	, m_pFx(nullptr)
{
}

void FxInstance::CloneTo(Falcon::AddInterface& rInterface, FxInstance& rClone) const
{
	Falcon::Instance::CloneTo(rInterface, rClone);
	rClone.m_hOwner = m_hOwner;
	rClone.m_pFx.Reset(m_pFx.IsValid() ? m_pFx->Clone() : nullptr);
	rClone.m_uFlags = m_uFlags;
	rClone.m_fDepth3D = m_fDepth3D;
	rClone.m_fDepth3DBias = m_fDepth3DBias;
	rClone.m_bWaitingToStartFX = m_bWaitingToStartFX;
	rClone.m_pDepth3DSource = m_pDepth3DSource;
	rClone.m_pParentIfWorldspace = m_pParentIfWorldspace;
}

/**
 * @return The "psuedo" world position to use for the Fx position,
 * derived from settings and the current position of 
 * this FxIntance on the Falcon stage.
 */
Matrix4D FxInstance::GetFxWorldTransform(Falcon::Instance const& instance) const
{
	Vector2D vWorldPosition = instance.ComputeWorldPosition();
	Matrix4D mFxWorldTransform = Matrix4D::Identity();
	Matrix2x3 mWorldTransform = instance.ComputeWorldTransform();
	mFxWorldTransform.M00 = mWorldTransform.M00;
	mFxWorldTransform.M01 = mWorldTransform.M01;
	mFxWorldTransform.M10 = mWorldTransform.M10;
	mFxWorldTransform.M11 = mWorldTransform.M11;

	// TODO: This is a little weird - in short, the depth value
	// we feed into the particle system is always in world space. Either
	// this will be applied fresh (for local particles) or stored in the
	// particle (for world particles). This is only ok becuase we control
	// all conversions and we don't allow the "world" transform to
	// contain any depth change.
	auto const fDepth3D = instance.ComputeWorldDepth3D();

	CheckedPtr<Movie> pOwner(GetPtr<Movie>(m_hOwner));
	if (pOwner.IsValid())
	{
		auto vFxWorld = pOwner->ToFxWorldPosition(vWorldPosition.X, vWorldPosition.Y, fDepth3D);
		
		// Need to negate Y, since ToFxWorldPosition flips the position for us, but we're
		// about to flip it again below (we only want the rescaling of the position, not the
		// flipping).
		vFxWorld.Y = -vFxWorld.Y;
		mFxWorldTransform.SetTranslation(vFxWorld);
	}
	else
	{
		mFxWorldTransform.SetTranslation(Vector3D(vWorldPosition.X, vWorldPosition.Y, fDepth3D));
	}

	// FX system is +Y up, UI is +Y down.
	return Matrix4D::CreateScale(1.0f, -1.0f, 1.0f) * mFxWorldTransform * Matrix4D::CreateScale(1.0f, -1.0f, 1.0f);
}

} // namespace UI

} // namespace Seoul
