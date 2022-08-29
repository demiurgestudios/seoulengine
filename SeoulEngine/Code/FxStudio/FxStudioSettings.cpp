/**
 * \file FxStudioSettings.cpp
 * \brief Specialization of FxStudio::Component that stores global settings
 * for the effect.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FxManager.h"
#include "FxStudioSettings.h"

#if SEOUL_WITH_FX_STUDIO

namespace Seoul::FxStudio
{

Settings::Settings(
	Int iComponentIndex,
	const InternalDataType& internalData,
	FilePath filePath)
	: ComponentBase(internalData)
	, m_bPreWarm("Pre-Warm", this)
{
}

Settings::~Settings()
{
}

HString Settings::GetComponentTypeName() const
{
	return HString(Settings::StaticTypeName());
}

Bool Settings::ShouldPrewarm() const
{
	return m_bPreWarm;
}

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO
