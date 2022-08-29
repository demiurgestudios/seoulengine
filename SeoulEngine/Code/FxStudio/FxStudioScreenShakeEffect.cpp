/**
 * \file FxStudioScreenShakeEffect.cpp
 * \brief Specialization of FxStudio::Component that implements
 * an FxStudio component that will shake the screen.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FxManager.h"
#include "FxStudioScreenShakeEffect.h"

#if SEOUL_WITH_FX_STUDIO

namespace Seoul::FxStudio
{

ScreenShakeEffect::ScreenShakeEffect(
	Int iComponentIndex,
	const InternalDataType& internalData,
	FilePath filePath)
	: ComponentBase(internalData)
	, m_vCameraPos()
	, m_Motion("Motion", this)
{
}

ScreenShakeEffect::~ScreenShakeEffect()
{
	Deactivate();
}

void ScreenShakeEffect::Update(Float /*fDeltaTime*/)
{
	if (FxManager::Get())
	{
		FxManager::Get()->SetScreenShakeOffset(Vector2D(
			m_Motion.GetValue(0),
			m_Motion.GetValue(1)));
	}
}

void ScreenShakeEffect::Deactivate()
{
	if (FxManager::Get())
	{
		FxManager::Get()->SetScreenShakeOffset(Vector2D::Zero());
	}
}

HString ScreenShakeEffect::GetComponentTypeName() const
{
	return HString(ScreenShakeEffect::StaticTypeName());
}

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO
