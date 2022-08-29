/**
 * \file ScriptUIFxInstance.cpp
 * \brief Script binding around UIFxInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "ScriptFunctionInterface.h"
#include "ScriptUIFxInstance.h"
#include "ScriptUIMovie.h"
#include "UIFxInstance.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptUIFxInstance, TypeFlags::kDisableCopy)
	SEOUL_PARENT(ScriptUIInstance)
	SEOUL_METHOD(GetDepth3D)
	SEOUL_METHOD(GetProperties)
	SEOUL_METHOD(SetDepth3D)
	SEOUL_METHOD(SetDepth3DBias)
	SEOUL_METHOD(SetDepth3DNativeSource)
	SEOUL_METHOD(SetRallyPoint)
	SEOUL_METHOD(SetTreatAsLooping)
	SEOUL_METHOD(Stop)
SEOUL_END_TYPE()

static const HString kDefaultFxDisplayObjectClassName("FxDisplayObject");

ScriptUIFxInstance::ScriptUIFxInstance()
{
}

ScriptUIFxInstance::~ScriptUIFxInstance()
{
}

HString ScriptUIFxInstance::GetClassName() const
{
	return kDefaultFxDisplayObjectClassName;
}

Float ScriptUIFxInstance::GetDepth3D() const
{
	return GetInstance()->GetDepth3D();
}

SharedPtr<UI::FxInstance> ScriptUIFxInstance::GetInstance() const
{
	SEOUL_ASSERT(!m_pInstance.IsValid() || Falcon::InstanceType::kFx == m_pInstance->GetType());
	return SharedPtr<UI::FxInstance>((UI::FxInstance*)m_pInstance.GetPtr());
}

FxProperties ScriptUIFxInstance::GetProperties() const
{
	return GetInstance()->GetProperties();
}

void ScriptUIFxInstance::SetDepth3D(Float f)
{
	GetInstance()->SetDepth3D(f);
}

void ScriptUIFxInstance::SetDepth3DBias(Float f)
{
	GetInstance()->SetDepth3DBias(f);
}

void ScriptUIFxInstance::SetDepth3DNativeSource(Script::FunctionInterface* pInterface)
{
	if (ScriptUIInstance* pInstance = pInterface->GetUserData<ScriptUIInstance>(1))
	{
		GetInstance()->SetDepthSource(pInstance->GetInstance());
	}
	else
	{
		pInterface->RaiseError(1, "invalid argument, must be a native Falcon::Instance");
		return;
	}
}

Bool ScriptUIFxInstance::SetRallyPoint(Float fX, Float fY)
{
	auto pOwner(GetPtr(m_hOwner));
	if (pOwner)
	{
		return GetInstance()->SetRallyPoint(pOwner->ToFxWorldPosition(fX, fY));
	}

	return false;
}

void ScriptUIFxInstance::SetTreatAsLooping(Bool b)
{
	GetInstance()->SetTreatAsLooping(b);
}

void ScriptUIFxInstance::Stop(Bool bStopImmediately)
{
	GetInstance()->Stop(bStopImmediately);
}

} // namespace Seoul
