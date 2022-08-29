/**
 * \file FxStudioScreenShakeEffect.h
 * \brief Specialization of ::FxStudio::Component that implements
 * an FxStudio component that will shake the screen.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_STUDIO_SCREEN_SHAKE_EFFECT_H
#define FX_STUDIO_SCREEN_SHAKE_EFFECT_H

#include "FilePath.h"
#include "FxStudioComponentBase.h"
#include "Prereqs.h"

#if SEOUL_WITH_FX_STUDIO

namespace Seoul::FxStudio
{

class ScreenShakeEffect SEOUL_SEALED : public ComponentBase
{
public:
	/**
	 * @return Fix class name used in the FxStudio ComponentDefinition file.
	 */
	static Byte const* StaticTypeName()
	{
		return "ScreenShake";
	}

	typedef void (*AssetCallback)(void* pUserData, const char* sAssetID);
	static void GetAssets(::FxStudio::Component const* pComponent, AssetCallback assetCallback, void* pUserData)
	{
		// Nop;
	}

	ScreenShakeEffect(Int iComponentIndex, const InternalDataType& internalData, FilePath filePath);
	virtual ~ScreenShakeEffect();

	// ::FxStudio::Component overrides
	virtual void Update( Float /*fDeltaTime*/ ) SEOUL_OVERRIDE;
	virtual void Deactivate() SEOUL_OVERRIDE;
	// /::FxStudio::Component overrides

	// FxBaseComponent overrides
	virtual HString GetComponentTypeName() const SEOUL_OVERRIDE;
	// /FxBaseComponent overrides

private:
	Vector3D m_vCameraPos; // preserve original camera position, to be restored post-shake
	::FxStudio::FloatKeyFrameProperty m_Motion;

	SEOUL_DISABLE_COPY(ScreenShakeEffect);
}; // class FxStudio::ScrenShakeEffect

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO

#endif // include guard
