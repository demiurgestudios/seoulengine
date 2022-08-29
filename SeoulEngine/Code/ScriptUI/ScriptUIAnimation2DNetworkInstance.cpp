/**
 * \file ScriptUIAnimation2DNetworkInstance.cpp
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
#include "ScriptUIAnimation2DNetworkInstance.h"
#include "ScriptUIMovie.h"
#include "UIAnimation2DNetworkInstance.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptUIAnimation2DNetworkInstance, TypeFlags::kDisableCopy)
	SEOUL_PARENT(ScriptUIInstance)
	SEOUL_METHOD(AddBoneAttachment)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "double iIndex, ScriptUIInstance oInstance")
	SEOUL_METHOD(GetActiveStatePath)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(string, double)")
	SEOUL_METHOD(GetBoneIndex)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "double", "string sName")
	SEOUL_METHOD(GetBonePositionByIndex)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)", "double iIndex")
	SEOUL_METHOD(GetBonePositionByName)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)", "string sName")
	SEOUL_METHOD(GetActivePalette)
	SEOUL_METHOD(GetActiveSkin)
	SEOUL_METHOD(GetLocalBonePositionByIndex)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)", "double iIndex")
	SEOUL_METHOD(GetLocalBonePositionByName)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)", "string sName")
	SEOUL_METHOD(GetLocalBoneScaleByIndex)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)", "double iIndex")
	SEOUL_METHOD(GetLocalBoneScaleByName)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)", "string sName")
	SEOUL_METHOD(GetCurrentMaxTime)
	SEOUL_METHOD(GetTimeToEvent)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "double?", "string sEventName")
	SEOUL_METHOD(GetWorldSpaceBonePositionByIndex)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)", "double iIndex")
	SEOUL_METHOD(GetWorldSpaceBonePositionByName)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)", "string sName")
	SEOUL_METHOD(AllDonePlaying)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(bool, bool)")
	SEOUL_METHOD(IsInStateTransition)
	SEOUL_METHOD(IsReady)
	SEOUL_METHOD(SetCondition)
	SEOUL_METHOD(SetParameter)
	SEOUL_METHOD(SetActivePalette)
	SEOUL_METHOD(SetActiveSkin)
	SEOUL_METHOD(SetVariableTimeStep)
	SEOUL_METHOD(TriggerTransition)
	SEOUL_METHOD(AddTimestepOffset)
	SEOUL_METHOD(SetCastShadow)
	SEOUL_METHOD(SetShadowOffset)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "double? fX = null, double? fY = null")
SEOUL_END_TYPE()

static const HString kDefaultAnimation2DNetworkClassName("Animation2DNetwork");

ScriptUIAnimation2DNetworkInstance::ScriptUIAnimation2DNetworkInstance()
{
}

ScriptUIAnimation2DNetworkInstance::~ScriptUIAnimation2DNetworkInstance()
{
}

HString ScriptUIAnimation2DNetworkInstance::GetClassName() const
{
	return kDefaultAnimation2DNetworkClassName;
}

SharedPtr<UI::Animation2DNetworkInstance> ScriptUIAnimation2DNetworkInstance::GetInstance() const
{
	SEOUL_ASSERT(!m_pInstance.IsValid() || Falcon::InstanceType::kAnimation2D == m_pInstance->GetType());
	return SharedPtr<UI::Animation2DNetworkInstance>((UI::Animation2DNetworkInstance*)m_pInstance.GetPtr());
}

Float ScriptUIAnimation2DNetworkInstance::GetCurrentMaxTime() const
{
	return GetInstance()->GetCurrentMaxTime();
}

void ScriptUIAnimation2DNetworkInstance::GetTimeToEvent(Script::FunctionInterface* pInterface) const
{
	HString sEventName;
	if (!pInterface->GetString(1, sEventName))
	{
		pInterface->RaiseError(1, "Incorrect argument, expected string.");
		return;
	}

	Float fTimeToEvent;
	if (GetInstance()->GetTimeToEvent(sEventName, fTimeToEvent))
	{
		pInterface->PushReturnNumber(fTimeToEvent);
	}
	else
	{
		pInterface->PushReturnNil();
	}
}

void ScriptUIAnimation2DNetworkInstance::SetCondition(HString name, Bool bValue)
{
	GetInstance()->SetCondition(name, bValue);
}

void ScriptUIAnimation2DNetworkInstance::SetParameter(HString name, Float fValue)
{
	GetInstance()->SetParameter(name, fValue);
}

void ScriptUIAnimation2DNetworkInstance::TriggerTransition(HString name)
{
	GetInstance()->TriggerTransition(name);
}

void ScriptUIAnimation2DNetworkInstance::AddTimestepOffset(Float fTimestepOffset)
{
	GetInstance()->AddTimestepOffset(fTimestepOffset);
}

void ScriptUIAnimation2DNetworkInstance::SetCastShadow(Bool b)
{
	GetInstance()->SetCastShadow(b);
}

void ScriptUIAnimation2DNetworkInstance::SetShadowOffset(Script::FunctionInterface* pInterface)
{
	Float fX = 0.0f;
	Float fY = 0.0f;
	(void)pInterface->GetNumber(1, fX);
	(void)pInterface->GetNumber(2, fY);

	GetInstance()->SetShadowOffset(Vector2D(fX, fY));
}

void ScriptUIAnimation2DNetworkInstance::AddBoneAttachment(Script::FunctionInterface* pInterface)
{
	Int index = -1;
	if(!pInterface->GetInteger(1, index))
	{
		pInterface->RaiseError(1, "Incorrect argument, expected integer.");
		return;
	}
	if(index < 0)
	{
		pInterface->RaiseError(1, "Invalid bone index %d", index);
		return;
	}

	if (ScriptUIInstance* pInstance = pInterface->GetUserData<ScriptUIInstance>(2))
	{
		GetInstance()->AddBoneAttachment(index, pInstance->GetInstance());
	}
	else
	{
		pInterface->RaiseError(2, "invalid child, must be a native Falcon::Instance, Falcon::EditTextInstance, or Falcon::MovieClipInstance.");
		return;
	}
}

void ScriptUIAnimation2DNetworkInstance::GetActiveStatePath(Script::FunctionInterface* pInterface) const
{
	auto pInstance(GetInstance());

	UInt32 uId = 0u;
	pInterface->PushReturnString(pInstance->GetActiveStatePath(uId));
	pInterface->PushReturnUInt32(uId);
}

void ScriptUIAnimation2DNetworkInstance::GetBoneIndex(Script::FunctionInterface* pInterface)
{
	HString name;
	if(!pInterface->GetString(1, name))
	{
		pInterface->RaiseError(1, "Incorrect argument, expected string.");
		return;
	}

	auto instance = GetInstance();

	Int16 index = instance->GetBoneIndex(name);
	pInterface->PushReturnInteger(index);
}

void ScriptUIAnimation2DNetworkInstance::GetBonePositionByName(Script::FunctionInterface* pInterface)
{
	HString name;
	if(!pInterface->GetString(1, name))
	{
		pInterface->RaiseError(1, "Incorrect argument, expected string.");
		return;
	}

	auto instance = GetInstance();

	Int16 index = instance->GetBoneIndex(name);

	if(index >= 0)
	{
		Vector2D vPos = instance->GetBonePosition(index);
		pInterface->PushReturnNumber(vPos.X);
		pInterface->PushReturnNumber(vPos.Y);
	}
	else
	{
		pInterface->RaiseError(1, "Invalid bone name %s", name.CStr());
		return;
	}
}

void ScriptUIAnimation2DNetworkInstance::GetBonePositionByIndex(Script::FunctionInterface* pInterface)
{
	Int index = -1;
	if(!pInterface->GetInteger(1, index))
	{
		pInterface->RaiseError(1, "Incorrect argument, expected integer.");
		return;
	}

	if(index >= 0)
	{
		Vector2D vPos = GetInstance()->GetBonePosition((Int16)index);
		pInterface->PushReturnNumber(vPos.X);
		pInterface->PushReturnNumber(vPos.Y);
	}
	else
	{
		pInterface->RaiseError(1, "Invalid bone index %d", index);
		return;
	}
}

void ScriptUIAnimation2DNetworkInstance::GetLocalBonePositionByName(Script::FunctionInterface* pInterface)
{
	HString name;
	if(!pInterface->GetString(1, name))
	{
		pInterface->RaiseError(1, "Incorrect argument, expected string.");
		return;
	}

	auto instance = GetInstance();

	Int16 index = instance->GetBoneIndex(name);

	if(index >= 0)
	{
		Vector2D vPos = instance->GetLocalBonePosition(index);
		pInterface->PushReturnNumber(vPos.X);
		pInterface->PushReturnNumber(vPos.Y);
	}
	else
	{
		pInterface->RaiseError(1, "Invalid bone name %s", name.CStr());
		return;
	}
}

HString ScriptUIAnimation2DNetworkInstance::GetActivePalette() const
{
	return GetInstance()->GetActivePalette();
}

HString ScriptUIAnimation2DNetworkInstance::GetActiveSkin() const
{
	return GetInstance()->GetActiveSkin();
}

void ScriptUIAnimation2DNetworkInstance::GetLocalBonePositionByIndex(Script::FunctionInterface* pInterface)
{
	Int index = -1;
	if(!pInterface->GetInteger(1, index))
	{
		pInterface->RaiseError(1, "Incorrect argument, expected integer.");
		return;
	}

	if(index >= 0)
	{
		Vector2D vPos = GetInstance()->GetLocalBonePosition((Int16)index);
		pInterface->PushReturnNumber(vPos.X);
		pInterface->PushReturnNumber(vPos.Y);
	}
	else
	{
		pInterface->RaiseError(1, "Invalid bone index %d", index);
		return;
	}
}

void ScriptUIAnimation2DNetworkInstance::GetLocalBoneScaleByName(Script::FunctionInterface* pInterface)
{
	HString name;
	if(!pInterface->GetString(1, name))
	{
		pInterface->RaiseError(1, "Incorrect argument, expected string.");
		return;
	}

	auto instance = GetInstance();

	Int16 index = instance->GetBoneIndex(name);

	if(index >= 0)
	{
		Vector2D vScale = instance->GetLocalBoneScale(index);
		pInterface->PushReturnNumber(vScale.X);
		pInterface->PushReturnNumber(vScale.Y);
	}
	else
	{
		pInterface->RaiseError(1, "Invalid bone name %s", name.CStr());
		return;
	}
}

void ScriptUIAnimation2DNetworkInstance::GetLocalBoneScaleByIndex(Script::FunctionInterface* pInterface)
{
	Int index = -1;
	if(!pInterface->GetInteger(1, index))
	{
		pInterface->RaiseError(1, "Incorrect argument, expected integer.");
		return;
	}

	if(index >= 0)
	{
		Vector2D vScale = GetInstance()->GetLocalBoneScale((Int16)index);
		pInterface->PushReturnNumber(vScale.X);
		pInterface->PushReturnNumber(vScale.Y);
	}
	else
	{
		pInterface->RaiseError(1, "Invalid bone index %d", index);
		return;
	}
}

void ScriptUIAnimation2DNetworkInstance::GetWorldSpaceBonePositionByName(Script::FunctionInterface* pInterface)
{
	HString name;
	if(!pInterface->GetString(1, name))
	{
		pInterface->RaiseError(1, "Incorrect argument, expected string.");
		return;
	}

	auto instance = GetInstance();

	Int16 index = instance->GetBoneIndex(name);

	if (index >= 0)
	{
		Vector2D vPos = instance->GetWorldSpaceBonePosition(index);
		pInterface->PushReturnNumber(vPos.X);
		pInterface->PushReturnNumber(vPos.Y);
	}
	else
	{
		pInterface->RaiseError(1, "%s: invalid bone name %s",
			instance->GetName().CStr(),
			name.CStr());
		return;
	}
}

void ScriptUIAnimation2DNetworkInstance::GetWorldSpaceBonePositionByIndex(Script::FunctionInterface* pInterface)
{
	Int index = -1;
	if(!pInterface->GetInteger(1, index))
	{
		pInterface->RaiseError(1, "Incorrect argument, expected integer.");
		return;
	}

	if(index >= 0)
	{
		Vector2D vPos = GetInstance()->GetWorldSpaceBonePosition((Int16)index);
		pInterface->PushReturnNumber(vPos.X);
		pInterface->PushReturnNumber(vPos.Y);
	}
	else
	{
		pInterface->RaiseError(1, "Invalid bone index %d", index);
		return;
	}
}

void ScriptUIAnimation2DNetworkInstance::AllDonePlaying(Script::FunctionInterface* pInterface) const
{
	Bool bDone = false;
	Bool bLooping = false;
	GetInstance()->AllDonePlaying(bDone, bLooping);
	pInterface->PushReturnBoolean(bDone);
	pInterface->PushReturnBoolean(bLooping);
}

Bool ScriptUIAnimation2DNetworkInstance::IsInStateTransition() const
{
	return GetInstance()->IsInStateTransition();
}

Bool ScriptUIAnimation2DNetworkInstance::IsReady() const
{
	return GetInstance()->IsReady();
}

void ScriptUIAnimation2DNetworkInstance::SetActivePalette(HString palette)
{
	GetInstance()->SetActivePalette(palette);
}

void ScriptUIAnimation2DNetworkInstance::SetActiveSkin(HString skin)
{
	GetInstance()->SetActiveSkin(skin);
}

void ScriptUIAnimation2DNetworkInstance::SetVariableTimeStep(Bool b)
{
	GetInstance()->SetVariableTimeStep(b);
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_2D

