/**
 * \file ScriptUIFxInstance.h
 * \brief Script binding around UIFxInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_UI_FX_INSTANCE_H
#define SCRIPT_UI_FX_INSTANCE_H

#include "Fx.h"
#include "ScriptUIInstance.h"
namespace Seoul { namespace UI { class FxInstance; } }

namespace Seoul
{

class ScriptUIFxInstance SEOUL_SEALED : public ScriptUIInstance
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ScriptUIFxInstance);

	ScriptUIFxInstance();
	~ScriptUIFxInstance();

	virtual HString GetClassName() const SEOUL_OVERRIDE;

	Float GetDepth3D() const;

	SharedPtr<UI::FxInstance> GetInstance() const;
	FxProperties GetProperties() const;

	void SetDepth3D(Float f);
	void SetDepth3DBias(Float f);
	void SetDepth3DNativeSource(Script::FunctionInterface* pInterface);
	Bool SetRallyPoint(Float fX, Float fY);
	void SetTreatAsLooping(Bool b);
	void Stop(Bool bStopImmediately);

private:
	SEOUL_DISABLE_COPY(ScriptUIFxInstance);
}; // class ScriptUIFxInstance

} // namespace Seoul

#endif // include guard
