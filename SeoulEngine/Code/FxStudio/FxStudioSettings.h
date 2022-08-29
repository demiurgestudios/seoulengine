/**
 * \file FxStudioSettings.h
 * \brief Specialization of ::FxStudio::Component that stores global settings
 * for the effect.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_STUDIO_SETTINGS_H
#define FX_STUDIO_SETTINGS_H

#include "FilePath.h"
#include "FxStudioComponentBase.h"
#include "Prereqs.h"

#if SEOUL_WITH_FX_STUDIO

namespace Seoul::FxStudio
{

class Settings SEOUL_SEALED : public ComponentBase
{
public:
	/**
	 * @return Fix class name used in the FxStudio ComponentDefinition file.
	 */
	static Byte const* StaticTypeName()
	{
		return "Settings";
	}

	typedef void (*AssetCallback)(void* pUserData, const char* sAssetID);
	static void GetAssets(::FxStudio::Component const* pComponent, AssetCallback assetCallback, void* pUserData)
	{
		// Nop;
	}

	Settings(Int iComponentIndex, const InternalDataType& internalData, FilePath filePath);
	virtual ~Settings();

	// FxBaseComponent overrides
	virtual HString GetComponentTypeName() const SEOUL_OVERRIDE;
	// /FxBaseComponent overrides

	bool ShouldPrewarm() const;

private:
	::FxStudio::BooleanProperty m_bPreWarm;

	SEOUL_DISABLE_COPY(Settings);
}; // class FxStudio::Settings

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO

#endif // include guard
