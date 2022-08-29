/**
 * \file ScriptUIAnimation2DNetworkInstance.h
 * \brief Script binding around UIAnimation2DNetworkInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_UI_ANIMATION2D_NETWORK_INSTANCE_H
#define SCRIPT_UI_ANIMATION2D_NETWORK_INSTANCE_H

#include "ScriptUIInstance.h"
namespace Seoul { namespace Script { class FunctionInterface; } }
namespace Seoul { namespace UI { class Animation2DNetworkInstance; } }

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul
{

class ScriptUIAnimation2DNetworkInstance SEOUL_SEALED : public ScriptUIInstance
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ScriptUIAnimation2DNetworkInstance);

	ScriptUIAnimation2DNetworkInstance();
	~ScriptUIAnimation2DNetworkInstance();

	virtual HString GetClassName() const SEOUL_OVERRIDE;
	SharedPtr<UI::Animation2DNetworkInstance> GetInstance() const;

	Float GetCurrentMaxTime() const;
	void GetTimeToEvent(Script::FunctionInterface* pInterface) const;

	void SetCondition(HString name, Bool bValue);
	void SetParameter(HString name, Float fValue);
	void TriggerTransition(HString name);
	void AddTimestepOffset(Float fTimestepOffset);

	void SetCastShadow(Bool b);
	void SetShadowOffset(Script::FunctionInterface* pInterface);

	void AddBoneAttachment(Script::FunctionInterface* pInterface);
	void GetActiveStatePath(Script::FunctionInterface* pInterface) const;
	void GetBoneIndex(Script::FunctionInterface* pInterface);
	void GetBonePositionByName(Script::FunctionInterface* pInterface);
	void GetBonePositionByIndex(Script::FunctionInterface* pInterface);
	HString GetActivePalette() const;
	HString GetActiveSkin() const;
	void GetLocalBonePositionByName(Script::FunctionInterface* pInterface);
	void GetLocalBonePositionByIndex(Script::FunctionInterface* pInterface);
	void GetLocalBoneScaleByName(Script::FunctionInterface* pInterface);
	void GetLocalBoneScaleByIndex(Script::FunctionInterface* pInterface);
	void GetWorldSpaceBonePositionByName(Script::FunctionInterface* pInterface);
	void GetWorldSpaceBonePositionByIndex(Script::FunctionInterface* pInterface);
	void SetActivePalette(HString palette);
	void SetActiveSkin(HString skin);
	void SetVariableTimeStep(Bool b);

	void AllDonePlaying(Script::FunctionInterface* pInterface) const;
	Bool IsInStateTransition() const;
	Bool IsReady() const;

private:
	SEOUL_DISABLE_COPY(ScriptUIAnimation2DNetworkInstance);
}; // class ScriptUIAnimation2DNetworkInstance

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // include guard
