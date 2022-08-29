/**
 * \file ScriptUITweenCompletion.h
 * \brief Subclass of UI::TweenCompletionInterface, invokes a script callback.
 *
 * ScriptUITweenCompletion binds a Script::VmObject as a tween completion callback.
 * Completion of the tween invokes the script function.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_UI_TWEEN_COMPLETION_H
#define SCRIPT_UI_TWEEN_COMPLETION_H

#include "UITween.h"
namespace Seoul { namespace Script { class VmObject; } }

namespace Seoul
{

class ScriptUITweenCompletion SEOUL_SEALED : public UI::TweenCompletionInterface
{
public:
	ScriptUITweenCompletion(const SharedPtr<Script::VmObject>& pObject);
	~ScriptUITweenCompletion();

	virtual void OnComplete() SEOUL_OVERRIDE;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(ScriptUITweenCompletion);

	SharedPtr<Script::VmObject> m_pObject;

	SEOUL_DISABLE_COPY(ScriptUITweenCompletion);
}; // class ScriptUITweenCompletion

} // namespace Seoul

#endif // include guard
