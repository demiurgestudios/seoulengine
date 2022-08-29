/**
 * \file FxStudioPreview.h
 * \brief Specialization of FxStudio::Preview, integrates the FxStudio preview
 * system into SeoulEngine.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_STUDIO_PREVIEW_H
#define FX_STUDIO_PREVIEW_H

#include "Prereqs.h"

#if SEOUL_WITH_FX_STUDIO
#include "FxStudioRT.h"

namespace Seoul { class Camera; }

namespace Seoul::FxStudio
{

/**
 * Integrates the FxStudio preview system into SeoulEngine,
 * allows rapid iteration by updating preview fx
 * on the fly while modifying their values in the FxStudio editor.
 */
class Preview SEOUL_SEALED : public ::FxStudio::Preview
{
public:
	Preview();
	~Preview();

	// Retrieve the current spawn position.
	const Vector3D& GetSpawnPosition() const
	{
		return m_vSpawnPosition;
	}

	// Per-frame render function.
	virtual void Render(void* pRenderData) SEOUL_OVERRIDE;

	// Called by FxStudio when the preview system needs to
	// instantiate a new FxInstance object.
	virtual ::FxStudio::FxInstance SpawnFx(
		::FxStudio::Manager& rManager,
		UInt8 const* pByteStream,
		Byte const* sFxName) SEOUL_OVERRIDE;

	// Called by FxStudio when the preview system needs to
	// "seek" an effect to an arbitrary time.
	virtual void SetFxTime(::FxStudio::FxInstance fxInstance, Float fTime) SEOUL_OVERRIDE;

	// Per-frame poll function
	virtual void Update(Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;

	// Update the camera to be used for rendering preview FX.
	void SetPreviewFxCamera(const SharedPtr<Camera>& pCamera);

	// Update the spawn position of the preview FX.
	void SetPreviewFxPosition(const Vector3D& vPosition);

	// Parenting transform of the preview FX. In addition
	// to the position.
	void SetPreviewFxTransform(const Matrix4D& mTransform);

	// Update flags used during FX playback.
	void SetPreviewFxFlags(UInt32 uFlags);

private:
	SharedPtr<Camera> m_pCamera;
	Vector3D m_vSpawnPosition;
	Matrix4D m_mSpawnTransform;
	UInt32 m_uFlags;

	void ApplyTransform(const ::FxStudio::FxInstance& fxInstance) const;

	SEOUL_DISABLE_COPY(Preview);
}; // class FxStudio::Preview

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO

#endif // include guard
