/**
 * \file FxStudioFx.h
 * \brief Equivalent to the Sound::Event in the audio system, FxStudio::Fx is
 * a concrete sublcass of SeoulEngine Fx for the FxStudio integration.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_STUDIO_FX_H
#define FX_STUDIO_FX_H

#include "ContentHandle.h"
#include "FilePath.h"
#include "Fx.h"
#include "FxStudioBankFile.h"

#if SEOUL_WITH_FX_STUDIO
#include "FxStudioRT/FxInstance.h"

namespace Seoul::FxStudio
{

class Fx SEOUL_SEALED : public Seoul::Fx
{
public:
	Fx(const Content::Handle<BankFile>& hBankFile);
	~Fx();

	// Instantiate a copy of this Fx.
	virtual Fx* Clone() const SEOUL_OVERRIDE;

	/**
	 * @return True if the data referenced by this Fx is still being loaded,
	 * false otherwise.
	 */
	virtual Bool IsLoading() const SEOUL_OVERRIDE
	{
		return m_hBankFile.IsLoading();
	}

	// Return true if the Fx is currently playing, false otherwise.
	virtual Bool IsPlaying() const SEOUL_OVERRIDE;

	// Return the Assets that are used by this Fx.
	virtual Bool AppendAssets(FxAssetsVector& rvAssets) const SEOUL_OVERRIDE;

	// Timeline control support.
	virtual Bool Start(const Matrix4D& mTransform, UInt32 uFlags) SEOUL_OVERRIDE;
	virtual void Pause(Bool bPause = true) SEOUL_OVERRIDE;
	virtual void Stop(Bool bStopImmediately = false) SEOUL_OVERRIDE;

	// Raw update of flags.
	void SetFlags(UInt32 uFlags);

	// Update the world space position of this Fx.
	virtual Bool SetPosition(const Vector3D& vPosition) SEOUL_OVERRIDE;

	// Update the gravity of this Fx.
	virtual Bool SetGravity(Float fGravityAcceleration) SEOUL_OVERRIDE;

	// Update the full 3D transform of this Fx.
	virtual Bool SetTransform(const Matrix4D& mTransform) SEOUL_OVERRIDE;

	// Update the parent used if in worldspace
	virtual Bool SetParentIfWorldspace(const Matrix4D& mTransform) SEOUL_OVERRIDE;

	// Update the target rally point of any particle FX in this FX.
	virtual Bool SetRallyPoint(const Vector3D& vRallyPoint) SEOUL_OVERRIDE;

	// Get our content key.
	virtual FilePath GetFilePath() const SEOUL_OVERRIDE
	{
		return m_hBankFile.GetKey();
	}

	// Drawing support
	virtual void Draw(IFxRenderer& rRenderer) SEOUL_OVERRIDE;

	// Updating support.
	virtual void Tick(Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;

	// Return features of the overall Fx.
	virtual Bool GetProperties(FxProperties& rProperties) const SEOUL_OVERRIDE;

	// Return true if this Fx needs calls to Render or not.
	//
	// May be O(n), should be cached if evaluation time is important.
	virtual Bool NeedsRender() const SEOUL_OVERRIDE;

private:
	Content::Handle<BankFile> m_hBankFile;
	::FxStudio::FxInstance m_FxInstance;

	Bool ShouldPrewarm() const;

	SEOUL_DISABLE_COPY(Fx);
}; // class FxStudio::Fx

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO

#endif // include guard
