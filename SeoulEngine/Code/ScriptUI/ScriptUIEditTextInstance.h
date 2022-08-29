/**
 * \file ScriptUIEditTextInstance.h
 * \brief Script binding around Falcon::EditTextInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_UI_EDIT_TEXT_INSTANCE_H
#define SCRIPT_UI_EDIT_TEXT_INSTANCE_H

#include "ScriptUIInstance.h"
namespace Seoul { namespace Falcon { class EditTextInstance; } }

namespace Seoul
{

class ScriptUIEditTextInstance SEOUL_SEALED : public ScriptUIInstance
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ScriptUIEditTextInstance);

	ScriptUIEditTextInstance();
	~ScriptUIEditTextInstance();

	void CommitFormatting();

	Bool GetAutoSizeBottom() const;
	Bool GetAutoSizeContents() const;
	Bool GetAutoSizeHorizontal() const;
	virtual HString GetClassName() const SEOUL_OVERRIDE;
	void GetCursorColor(Script::FunctionInterface* pInterface) const;
	bool GetHasTextEditFocus() const;
	SharedPtr<Falcon::EditTextInstance> GetInstance() const;
	Int32 GetNumLines() const;
	void GetPlainText(Script::FunctionInterface* pInterface) const;
	void GetText(Script::FunctionInterface* pInterface) const;
	Bool GetVerticalCenter() const;
	UInt32 GetVisibleCharacters() const;
	Bool GetXhtmlParsing() const;
	void GetXhtmlText(Script::FunctionInterface* pInterface) const;

	void SetAutoSizeBottom(Bool bAutoSizeBottom);
	void SetAutoSizeContents(Bool bAutoSizeContents);
	void SetAutoSizeHorizontal(Bool bAutoSizeHorizontal);
	void SetCursorColor(UInt8 uR, UInt8 uG, UInt8 uB, UInt8 uA);
	void SetPlainText(const String& sText);
	void SetText(const String& sText);
	void SetTextToken(const String& sToken);
	void SetVerticalCenter(Bool bVerticalCenter);
	void SetVisibleCharacters(UInt32 uVisibleCharacters);
	void SetXhtmlParsing(Bool bXhtmlParsing);
	void SetXhtmlText(const String& sText);

	void GetTextBounds(Script::FunctionInterface* pInterface) const;
	void GetLocalTextBounds(Script::FunctionInterface* pInterface) const;
	void GetWorldTextBounds(Script::FunctionInterface* pInterface) const;

	void StartEditing(Script::FunctionInterface* pInterface);
	void StopEditing();

private:
	SEOUL_DISABLE_COPY(ScriptUIEditTextInstance);
}; // class ScriptUIEditTextInstance

} // namespace Seoul

#endif // include guard
