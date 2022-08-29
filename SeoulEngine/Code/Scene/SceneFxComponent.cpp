/**
 * \file SceneFxComponent.cpp
 * \brief Binds a visual fx (typically particles, but can
 * be more than that) into a Scene object.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Fx.h"
#include "FxManager.h"
#include "ReflectionDefine.h"
#include "SceneFxComponent.h"
#include "SceneFxRenderer.h"
#include "SceneObject.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(Scene::FxComponent, TypeFlags::kDisableCopy)
	SEOUL_DEV_ONLY_ATTRIBUTE(Category, "Drawing")
	SEOUL_DEV_ONLY_ATTRIBUTE(DisplayName, "Fx")
	SEOUL_PARENT(Scene::Component)
	SEOUL_PROPERTY_N("StartOnLoad", m_bStartOnLoad)
		SEOUL_ATTRIBUTE(NotRequired)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "If false, Fx must be started manually by script.")
	SEOUL_PROPERTY_PAIR_N("FxFilePath", GetFxFilePath, SetFxFilePath)
		SEOUL_DEV_ONLY_ATTRIBUTE(EditorFileSpec, GameDirectory::kContent, FileType::kFxBank)
		SEOUL_DEV_ONLY_ATTRIBUTE(Description, "Fx (*.xfx) file that provides the FX data.")
SEOUL_END_TYPE()

namespace Scene
{

FxComponent::FxComponent()
	: m_pFx()
	, m_bStartOnLoad(true)
	, m_bStarted(false)
	, m_bNeedsRender(false)
{
}

FxComponent::~FxComponent()
{
}

SharedPtr<Component> FxComponent::Clone(const String& sQualifier) const
{
	SharedPtr<FxComponent> pReturn(SEOUL_NEW(MemoryBudgets::SceneComponent) FxComponent);
	if (m_pFx.IsValid())
	{
		pReturn->m_pFx.Reset(m_pFx->Clone());
	}
	pReturn->m_bStartOnLoad = m_bStartOnLoad;
	return pReturn;
}

Float FxComponent::GetFxDuration() const
{
	if (m_pFx.IsValid())
	{
		FxProperties props;
		if (m_pFx->GetProperties(props))
		{
			return props.m_fDuration;
		}
	}

	return 0.0f;
}

void FxComponent::Render(
	const Frustum& frustum,
	FxRenderer& rRenderer)
{
	// TODO: Decide how we want to compute and maintain Fx culling volumes,
	// and then implement culling here.

	if (m_pFx.IsValid())
	{
		rRenderer.DrawFx(*m_pFx);

		// TODO: DrawFx should return a flag when the Fx
		// no longer needs rendering, and we should refresh that here.
	}
}

void FxComponent::SetFxFilePath(FilePath filePath)
{
	m_pFx.Reset();

	Fx* pFx = nullptr;
	if (FxManager::Get()->GetFx(filePath, pFx))
	{
		m_pFx.Reset(pFx);
		m_bStarted = false;
	}
}

Bool FxComponent::StartFx()
{
	if (m_pFx.IsValid())
	{
		// Update m_bNeedsRender after successful start.
		if (m_pFx->Start(
			// TODO: Apply full transform?
			GetOwner().IsValid() ? Matrix4D::CreateTranslation(GetOwner()->GetPosition()) : Matrix4D::Identity(),
			0u))
		{
			m_bNeedsRender = m_pFx->NeedsRender();
			return true;
		}
	}

	return false;
}

void FxComponent::StopFx(Bool bStopImmediately /*= false*/)
{
	if (m_pFx.IsValid())
	{
		m_pFx->Stop(bStopImmediately);
	}
}

void FxComponent::Tick(Float fDeltaTimeInSeconds)
{
	if (m_pFx.IsValid())
	{
		// Handle start on load.
		if (m_bStartOnLoad && !m_bStarted)
		{
			m_bStarted = StartFx();
		}

		// TODO: Cache full world transform in FxComponent
		// after dirty.

		Matrix4D const mWorldTransform(
			GetOwner().IsValid() ? GetOwner()->ComputeNormalTransform() : Matrix4D::Identity());

		m_pFx->SetTransform(mWorldTransform);
		m_pFx->Tick(fDeltaTimeInSeconds);
	}
}

} // namespace Scene

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
