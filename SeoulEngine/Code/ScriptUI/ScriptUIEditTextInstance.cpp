/**
 * \file ScriptUIEditTextInstance.cpp
 * \brief Script binding around Falcon::EditTextInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconEditTextInstance.h"
#include "FalconMovieClipInstance.h"
#include "LocManager.h"
#include "ReflectionDefine.h"
#include "ScriptFunctionInterface.h"
#include "ScriptUIEditTextInstance.h"
#include "ScriptUIMovie.h"
#include "ScriptUIMovieClipInstance.h"
#include "UIManager.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptUIEditTextInstance, TypeFlags::kDisableCopy)
	SEOUL_PARENT(ScriptUIInstance)
	SEOUL_METHOD(CommitFormatting)
	SEOUL_METHOD(GetAutoSizeBottom)
	SEOUL_METHOD(GetAutoSizeContents)
	SEOUL_METHOD(GetAutoSizeHorizontal)
	SEOUL_METHOD(GetCursorColor)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(int, int, int, int)")
	SEOUL_METHOD(GetHasTextEditFocus)
	SEOUL_METHOD(GetNumLines)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "int")
	SEOUL_METHOD(GetPlainText)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string")
	SEOUL_METHOD(GetText)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string")
	SEOUL_METHOD(GetXhtmlText)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string")
	SEOUL_METHOD(GetVerticalCenter)
	SEOUL_METHOD(GetVisibleCharacters)
	SEOUL_METHOD(GetXhtmlParsing)
	SEOUL_METHOD(SetAutoSizeBottom)
	SEOUL_METHOD(SetAutoSizeContents)
	SEOUL_METHOD(SetAutoSizeHorizontal)
	SEOUL_METHOD(SetCursorColor)
	SEOUL_METHOD(SetPlainText)
	SEOUL_METHOD(SetText)
	SEOUL_METHOD(SetXhtmlText)
	SEOUL_METHOD(SetTextToken)
	SEOUL_METHOD(SetVerticalCenter)
	SEOUL_METHOD(SetVisibleCharacters)
	SEOUL_METHOD(SetXhtmlParsing)
	SEOUL_METHOD(StartEditing)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "Native.ScriptUIMovieClipInstance eventReceiver, string sDescription, int iMaxCharacters, string sRestrict, bool bAllowNonLatinKeyboard")
	SEOUL_METHOD(StopEditing)
	SEOUL_METHOD(GetTextBounds)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double?, double?, double?, double?)")
	SEOUL_METHOD(GetLocalTextBounds)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double?, double?, double?, double?)")
	SEOUL_METHOD(GetWorldTextBounds)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double?, double?, double?, double?)")
SEOUL_END_TYPE()

static const HString kDefaultEditTextClassName("EditText");

ScriptUIEditTextInstance::ScriptUIEditTextInstance()
{
}

ScriptUIEditTextInstance::~ScriptUIEditTextInstance()
{
}

void ScriptUIEditTextInstance::CommitFormatting()
{
	GetInstance()->CommitFormatting();
}

Bool ScriptUIEditTextInstance::GetAutoSizeBottom() const
{
	return GetInstance()->GetAutoSizeBottom();
}

Bool ScriptUIEditTextInstance::GetAutoSizeContents() const
{
	return GetInstance()->GetAutoSizeContents();
}

Bool ScriptUIEditTextInstance::GetAutoSizeHorizontal() const
{
	return GetInstance()->GetAutoSizeHorizontal();
}

HString ScriptUIEditTextInstance::GetClassName() const
{
	return kDefaultEditTextClassName;
}

void ScriptUIEditTextInstance::GetCursorColor(Script::FunctionInterface* pInterface) const
{
	RGBA const cursorColor = GetInstance()->GetCursorColor();
	pInterface->PushReturnInteger((Int32)cursorColor.m_R);
	pInterface->PushReturnInteger((Int32)cursorColor.m_G);
	pInterface->PushReturnInteger((Int32)cursorColor.m_B);
	pInterface->PushReturnInteger((Int32)cursorColor.m_A);
}

void ScriptUIEditTextInstance::GetLocalTextBounds(Script::FunctionInterface* pInterface) const
{
	GetInstance()->CommitFormatting();

	Falcon::Rectangle bounds;
	if (GetInstance()->GetLocalTextBounds(bounds))
	{
		pInterface->PushReturnNumber(bounds.m_fLeft);
		pInterface->PushReturnNumber(bounds.m_fTop);
		pInterface->PushReturnNumber(bounds.m_fRight);
		pInterface->PushReturnNumber(bounds.m_fBottom);
	}
	else
	{
		pInterface->PushReturnNumber(0.0);
		pInterface->PushReturnNumber(0.0);
		pInterface->PushReturnNumber(0.0);
		pInterface->PushReturnNumber(0.0);
	}
}

void ScriptUIEditTextInstance::GetTextBounds(Script::FunctionInterface* pInterface) const
{
	GetInstance()->CommitFormatting();

	Falcon::Rectangle bounds;
	if (GetInstance()->GetTextBounds(bounds))
	{
		pInterface->PushReturnNumber(bounds.m_fLeft);
		pInterface->PushReturnNumber(bounds.m_fTop);
		pInterface->PushReturnNumber(bounds.m_fRight);
		pInterface->PushReturnNumber(bounds.m_fBottom);
	}
	else
	{
		pInterface->PushReturnNumber(0.0);
		pInterface->PushReturnNumber(0.0);
		pInterface->PushReturnNumber(0.0);
		pInterface->PushReturnNumber(0.0);
	}
}

void ScriptUIEditTextInstance::GetWorldTextBounds(Script::FunctionInterface* pInterface) const
{
	GetInstance()->CommitFormatting();

	Falcon::Rectangle bounds;
	if (GetInstance()->GetWorldTextBounds(bounds))
	{
		pInterface->PushReturnNumber(bounds.m_fLeft);
		pInterface->PushReturnNumber(bounds.m_fTop);
		pInterface->PushReturnNumber(bounds.m_fRight);
		pInterface->PushReturnNumber(bounds.m_fBottom);
	}
	else
	{
		pInterface->PushReturnNumber(0.0);
		pInterface->PushReturnNumber(0.0);
		pInterface->PushReturnNumber(0.0);
		pInterface->PushReturnNumber(0.0);
	}
}

bool ScriptUIEditTextInstance::GetHasTextEditFocus() const
{
	return GetInstance()->GetHasTextEditFocus();
}

SharedPtr<Falcon::EditTextInstance> ScriptUIEditTextInstance::GetInstance() const
{
	SEOUL_ASSERT(!m_pInstance.IsValid() || Falcon::InstanceType::kEditText == m_pInstance->GetType());
	return SharedPtr<Falcon::EditTextInstance>((Falcon::EditTextInstance*)m_pInstance.GetPtr());
}

Int32 ScriptUIEditTextInstance::GetNumLines() const
{
	return GetInstance()->GetNumLines();
}

void ScriptUIEditTextInstance::GetPlainText(Script::FunctionInterface* pInterface) const
{
	const String& sText = GetInstance()->GetPlainText();

	pInterface->PushReturnString(sText.CStr(), sText.GetSize());
}

void ScriptUIEditTextInstance::GetText(Script::FunctionInterface* pInterface) const
{
	const String& sText = GetInstance()->GetText();

	pInterface->PushReturnString(sText.CStr(), sText.GetSize());
}

Bool ScriptUIEditTextInstance::GetVerticalCenter() const
{
	return GetInstance()->GetVerticalCenter();
}

UInt32 ScriptUIEditTextInstance::GetVisibleCharacters() const
{
	return GetInstance()->GetVisibleCharacters();
}

Bool ScriptUIEditTextInstance::GetXhtmlParsing() const
{
	return GetInstance()->GetXhtmlParsing();
}

void ScriptUIEditTextInstance::GetXhtmlText(Script::FunctionInterface* pInterface) const
{
	const String& sText = GetInstance()->GetXhtmlText();

	pInterface->PushReturnString(sText.CStr(), sText.GetSize());
}

void ScriptUIEditTextInstance::SetAutoSizeBottom(Bool bAutoSizeBottom)
{
	GetInstance()->SetAutoSizeBottom(bAutoSizeBottom);
}

void ScriptUIEditTextInstance::SetAutoSizeContents(Bool bAutoSizeContents)
{
	GetInstance()->SetAutoSizeContents(bAutoSizeContents);
}

void ScriptUIEditTextInstance::SetAutoSizeHorizontal(Bool bAutoSizeHorizontal)
{
	GetInstance()->SetAutoSizeHorizontal(bAutoSizeHorizontal);
}

void ScriptUIEditTextInstance::SetCursorColor(UInt8 uR, UInt8 uG, UInt8 uB, UInt8 uA)
{
	RGBA const rgba = RGBA::Create(
		uR,
		uG,
		uB,
		uA);
	GetInstance()->SetCursorColor(rgba);
}

void ScriptUIEditTextInstance::SetPlainText(const String& sText)
{
	GetInstance()->SetPlainText(sText);
}

void ScriptUIEditTextInstance::SetText(const String& sText)
{
	GetInstance()->SetText(sText);
}

void ScriptUIEditTextInstance::SetTextToken(const String& sToken)
{
	String const s(LocManager::Get()->Localize(sToken));
	SetText(s);
}

void ScriptUIEditTextInstance::SetVerticalCenter(Bool bVerticalCenter)
{
	GetInstance()->SetVerticalCenter(bVerticalCenter);
}

void ScriptUIEditTextInstance::SetVisibleCharacters(UInt32 uVisibleCharacters)
{
	GetInstance()->SetVisibleCharacters(uVisibleCharacters);
}

void ScriptUIEditTextInstance::SetXhtmlParsing(Bool bXhtmlParsing)
{
	GetInstance()->SetXhtmlParsing(bXhtmlParsing);
}

void ScriptUIEditTextInstance::SetXhtmlText(const String& sText)
{
	GetInstance()->SetXhtmlText(sText);
}

void ScriptUIEditTextInstance::StartEditing(Script::FunctionInterface* pInterface)
{
	if (ScriptUIMovieClipInstance* pInstance = pInterface->GetUserData<ScriptUIMovieClipInstance>(1))
	{
		String sDescription;
		if (!pInterface->GetString(2, sDescription))
		{
			pInterface->RaiseError(2, "Incorrect argument, expected String sDescription.");
			return;
		}

		Int32 iMaxCharacters = -1;
		if (!pInterface->GetInteger(3, iMaxCharacters))
		{
			pInterface->RaiseError(3, "Incorrect argument, expected integer.");
		}

		String sRestrict;
		if (!pInterface->GetString(4, sRestrict))
		{
			pInterface->RaiseError(4, "Incorrect argument, expected String sRestrict.");
			return;
		}

		Bool bAllowNonLatinKeyboard = false;
		if (!pInterface->GetBoolean(5, bAllowNonLatinKeyboard))
		{
			pInterface->RaiseError(5, "Incorrect argument, expected Bool bAllowNonLatinKeyboard.");
			return;
		}

		SharedPtr<Falcon::MovieClipInstance> pEventReceiver = pInstance->GetInstance();

		CheckedPtr<UI::Movie> pOwner = GetPtr(m_hOwner);
		if (!pOwner.IsValid())
		{
			pInterface->RaiseError(-1, "owner is invalid, dangling movie.");
			return;
		}

		StringConstraints constraints;
		constraints.m_iMaxCharacters = iMaxCharacters;
		constraints.m_sRestrict = sRestrict;
		auto const bReturn = UI::Manager::Get()->StartTextEditing(
			pOwner,
			pEventReceiver,
			GetInstance().GetPtr(),
			sDescription,
			constraints,
			bAllowNonLatinKeyboard);

		pInterface->PushReturnBoolean(bReturn);
	}
	else
	{
		pInterface->RaiseError(1, "required first argument be owningmovie clip.");
		return;
	}
}

void ScriptUIEditTextInstance::StopEditing()
{
	UI::Manager::Get()->StopTextEditing();
}

} // namespace Seoul
