/**
 * \file ScriptUIMotionCompletion.h
 * \brief Subclass of UI::MotionCompletionInterface, invokes a script callback.
 *
 * ScriptUIMotionCompletion binds a Script::VmObject as a UI::Motion completion callback.
 * Completion of the UI::Motion invokes the script function.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_UI_MOTION_COMPLETION_H
#define SCRIPT_UI_MOTION_COMPLETION_H

#include "UIMotion.h"
namespace Seoul { namespace Script { class VmObject; } }

namespace Seoul
{

class ScriptUIMotionCompletion SEOUL_SEALED : public UI::MotionCompletionInterface
{
public:
	ScriptUIMotionCompletion(const SharedPtr<Script::VmObject>& pObject);
	~ScriptUIMotionCompletion();

	virtual void OnComplete() SEOUL_OVERRIDE;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(ScriptUIMotionCompletion);

	SharedPtr<Script::VmObject> m_pObject;

	SEOUL_DISABLE_COPY(ScriptUIMotionCompletion);
}; // class ScriptUIMotionCompletion

} // namespace Seoul

#endif // include guard
