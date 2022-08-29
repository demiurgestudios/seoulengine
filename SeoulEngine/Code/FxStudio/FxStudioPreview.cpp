/**
 * \file FxStudioPreview.cpp
 * \brief Specialization of FxStudio::Preview, integrates the FxStudio preview
 * system into SeoulEngine.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "FxStudioComponentBase.h"
#include "FxStudioPreview.h"
#include "RenderDevice.h"
#include "Renderer.h"

#if SEOUL_WITH_FX_STUDIO

namespace Seoul::FxStudio
{

Preview::Preview()
	: m_vSpawnPosition(Vector3D::Zero())
	, m_mSpawnTransform(Matrix4D::Identity())
	, m_uFlags(0u)
{
}

Preview::~Preview()
{
	// Tear down the base class.
	Terminate();
}

/** Per-frame render hook for Fx. */
void Preview::Render(void* pRenderData)
{
	::FxStudio::Preview::Render(pRenderData);
}

/**
 * Instantiate the effect for the preview system.
 */
::FxStudio::FxInstance Preview::SpawnFx(
	::FxStudio::Manager& rManager,
	UInt8 const* pByteStream,
	Byte const* sFxName)
{
	// If the preview fx currently exists, this is a request to restart it.
	SetPaused(GetPreviewFx().IsValid());

	// Use an invalid content key to mark the preview emitter data.
	FilePath filePath;
	::FxStudio::FxInstance fxInstance = rManager.CreateFx(
		pByteStream,
		sFxName,
		&filePath);

	// Commit the transform to an already playing effect.
	ApplyTransform(fxInstance);

	return fxInstance;
}

/**
 * Seek an FxInstance to the target time.
 *
 * This is identical to the base class implementation, but avoids
 * massive time steps by incrementing a max of 1/10 a second at a time.
 */
void Preview::SetFxTime(::FxStudio::FxInstance fxInstance, Float fTime)
{
	// Max delta time of 1/10 of a second.
	static const float kfMaxDeltaTimeInSeconds = 0.1f;

	if (fTime < fxInstance.GetTime())
	{
		fxInstance.Stop(true);
		fxInstance.Play();
	}

	Bool const bInitialPlayToEndValue = fxInstance.IsPlayingToEnd();
	fxInstance.SetPlayToEnd(true);

	Float fDeltaTime = (fTime - fxInstance.GetTime());

	while (fDeltaTime > kfMaxDeltaTimeInSeconds)
	{
		fxInstance.Update(kfMaxDeltaTimeInSeconds);
		fDeltaTime -= kfMaxDeltaTimeInSeconds;
	}

	if (fDeltaTime > 0.0f)
	{
		fxInstance.Update(fDeltaTime);
	}

	fxInstance.SetPlayToEnd(bInitialPlayToEndValue);
}

/**
 * Called per frame to update the preview FX state.
 */
void Preview::Update(Float fDeltaTimeInSeconds)
{
	::FxStudio::Preview::Update(fDeltaTimeInSeconds);
}

/**
 * Update the camera to be used for rendering preview FX.
 */
void Preview::SetPreviewFxCamera(const SharedPtr<Camera>& pCamera)
{
	m_pCamera = pCamera;
}

/**
 * Update the position used for the preview FX.
 */
void Preview::SetPreviewFxPosition(const Vector3D& vPosition)
{
	// Update the position.
	m_vSpawnPosition = vPosition;

	// Commit the transform to an already playing effect.
	ApplyTransform(GetPreviewFx());
}

// Parenting transform of the preview FX. In addition
// to the position.
void Preview::SetPreviewFxTransform(const Matrix4D& mTransform)
{
	// Update the transform.
	m_mSpawnTransform = mTransform;

	// Commit the transform to an already playing effect.
	ApplyTransform(GetPreviewFx());
}

/**
 * Update the flags used for the preview FX.
 */
void Preview::SetPreviewFxFlags(UInt32 uFlags)
{
	// Update the flags
	m_uFlags = uFlags;

	// Commit the flags to an already playing effect.
	::FxStudio::FxInstance fxInstance = GetPreviewFx();
	if (fxInstance.IsValid())
	{
		for (::FxStudio::ComponentData const* p = fxInstance.BeginComponents();
			fxInstance.EndComponents() != p;
			++p)
		{
			ComponentBase* pBase = static_cast<ComponentBase*>(p->m_pComponent);
			pBase->SetFlags(m_uFlags);
		}
	}
}

/** Compute transform based on transform + position and apply to running preview FX. */
void Preview::ApplyTransform(const ::FxStudio::FxInstance& fxInstance) const
{
	// Commit the transform to an already playing effect.
	if (fxInstance.IsValid())
	{
		// Compute.
		auto const m(Matrix4D::CreateTranslation(m_vSpawnPosition) * m_mSpawnTransform);

		for (::FxStudio::ComponentData const* p = fxInstance.BeginComponents();
			fxInstance.EndComponents() != p;
			++p)
		{
			ComponentBase* pBase = static_cast<ComponentBase*>(p->m_pComponent);
			pBase->SetTransform(m);
		}
	}
}

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO
