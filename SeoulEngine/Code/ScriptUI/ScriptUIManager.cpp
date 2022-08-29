/**
 * \file ScriptUIManager.cpp
 * \brief Binder instance for exposing the global UI::Manager singleton
 * into script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "FalconEditTextInstance.h"
#include "FalconMovieClipInstance.h"
#include "FalconRenderState.h"
#include "FileManager.h"
#include "HtmlReader.h"
#include "ReflectionDefine.h"
#include "ScriptFunctionInterface.h"
#include "ScriptUIManager.h"
#include "ScriptUIInstance.h"
#include "ScriptUIMovie.h"
#include "ScriptUIMovieClipInstance.h"
#include "SeoulWildcard.h"
#include "UIManager.h"
#include "UIRenderer.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptUIManager, TypeFlags::kDisableCopy)
	SEOUL_METHOD(BroadcastEvent)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "string sEvent, params object[] aArgs")
	SEOUL_METHOD(BroadcastEventTo)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "string sEvent, string sTarget, params object[] aArgs")
	SEOUL_METHOD(GetRootMovieClip)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "RootMovieClip", "string sStateMachine, string sTarget")
	SEOUL_METHOD(GetCondition)
	SEOUL_METHOD(GetPerspectiveFactorAdjustment)
	SEOUL_METHOD(SetCondition)
	SEOUL_METHOD(PersistentBroadcastEvent)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "string sEvent, params object[] aArgs")
	SEOUL_METHOD(PersistentBroadcastEventTo)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "string sEvent, string sTarget, params object[] aArgs")
	SEOUL_METHOD(SetPerspectiveFactorAdjustment)
	SEOUL_METHOD(SetStage3DSettings)
	SEOUL_METHOD(TriggerTransition)
	SEOUL_METHOD(GetViewportAspectRatio)
	SEOUL_METHOD(DebugLogEntireUIState)
	SEOUL_METHOD(ComputeWorldSpaceDepthProjection)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)", "double fX, double fY, double fDepth")
	SEOUL_METHOD(ComputeInverseWorldSpaceDepthProjection)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)", "double fX, double fY, double fDepth")
	SEOUL_METHOD(GetStateMachineCurrentStateId)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "string sStateName")
	SEOUL_METHOD(AddToInputWhitelist)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "Native.ScriptUIMovieClipInstance movieClip")
	SEOUL_METHOD(ClearInputWhitelist)
	SEOUL_METHOD(RemoveFromInputWhitelist)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "Native.ScriptUIMovieClipInstance movieClip")
	SEOUL_METHOD(SetInputActionsEnabled)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "bool bEnabled")
	SEOUL_METHOD(HasPendingTransitions)
#if SEOUL_ENABLE_CHEATS
	SEOUL_METHOD(GotoState)
#endif // /#if SEOUL_ENABLE_CHEATS
#if !SEOUL_SHIP
	SEOUL_METHOD(ValidateUiFiles)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "string sExcludeWildcard")
#endif // /#if !SEOUL_SHIP
	SEOUL_METHOD(TriggerRestart)
	SEOUL_METHOD(GetPlainTextString)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "string input")

#if SEOUL_HOT_LOADING
	SEOUL_METHOD(ShelveDataForHotLoad)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "string sId, object data")
	SEOUL_METHOD(UnshelveDataFromHotLoad)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "object", "string sId")
#endif
SEOUL_END_TYPE()

ScriptUIManager::ScriptUIManager()
{
}

ScriptUIManager::~ScriptUIManager()
{
}

/** Send a UI broadcast to all UI::Movie instances on the stack. */
void ScriptUIManager::BroadcastEvent(Script::FunctionInterface* pInterface)
{
	HString sEvent;
	if (!pInterface->GetString(1u, sEvent))
	{
		pInterface->RaiseError(1u, "invalid event name, must be convertible to string.");
		return;
	}

	Int const iArguments = pInterface->GetArgumentCount() - 2;
	Reflection::MethodArguments aArguments;
	if (iArguments < 0 ||
		(UInt32)iArguments > aArguments.GetSize())
	{
		pInterface->RaiseError(-1, "too many arguments to BroadcastEvent, got %d, max of %u",
			iArguments,
			aArguments.GetSize());
		return;
	}

	for (Int i = 0; i < iArguments; ++i)
	{
		if (!pInterface->GetAny((UInt32)(i + 2), TypeId<void>(), aArguments[i]))
		{
			pInterface->RaiseError(i + 2, "invalid argument, must be convertible to Seoul::Reflection::Any.");
			return;
		}
	}

	Bool const bReturn = UI::Manager::Get()->BroadcastEvent(sEvent, aArguments, iArguments);
	pInterface->PushReturnBoolean(bReturn);
}

/** Send a UI broadcast to a specific UI::Movie on the stack. */
void ScriptUIManager::BroadcastEventTo(Script::FunctionInterface* pInterface)
{
	HString sTargetType;
	if (!pInterface->GetString(1u, sTargetType))
	{
		pInterface->RaiseError(1u, "invalid target type, must be convertible to string.");
		return;
	}

	HString sEvent;
	if (!pInterface->GetString(2u, sEvent))
	{
		pInterface->RaiseError(2u, "invalid event name, must be convertible to string.");
		return;
	}

	Int const iArguments = pInterface->GetArgumentCount() - 3;
	Reflection::MethodArguments aArguments;
	if (iArguments < 0 ||
		(UInt32)iArguments > aArguments.GetSize())
	{
		pInterface->RaiseError(-1, "too many arguments to BroadcastEvent, got %d, max of %u",
			iArguments,
			aArguments.GetSize());
		return;
	}

	for (Int i = 0; i < iArguments; ++i)
	{
		if (!pInterface->GetAny((UInt32)(i + 3), TypeId<void>(), aArguments[i]))
		{
			pInterface->RaiseError(i + 3, "invalid argument, must be convertible to Seoul::Reflection::Any.");
			return;
		}
	}

	Bool const bReturn = UI::Manager::Get()->BroadcastEventTo(sTargetType, sEvent, aArguments, iArguments);
	pInterface->PushReturnBoolean(bReturn);
}

void ScriptUIManager::GetRootMovieClip(Script::FunctionInterface* pInterface) const
{
	CheckedPtr<ScriptUIMovie> pOwner;
	SharedPtr<Falcon::Instance> pChildInstance;
	{
		HString sStateName;
		if (!pInterface->GetString(1u, sStateName))
		{
			pInterface->RaiseError(1u, "invalid state machine name, must be convertible to string.");
			return;
		}

		HString sTargetMovie;
		if (!pInterface->GetString(2u, sTargetMovie))
		{
			pInterface->RaiseError(2u, "invalid target movie name, must be convertible to string.");
			return;
		}

		{
			CheckedPtr<UI::Movie> pMovie;
			pChildInstance = UI::Manager::Get()->GetRootMovieClip(sStateName, sTargetMovie, pMovie);
			pOwner = DynamicCast<ScriptUIMovie*>(pMovie.Get());
		}

		if (!pOwner.IsValid() || !pChildInstance.IsValid())
		{
			pInterface->PushReturnNil();
			return;
		}
	}

	// Can longjmp, so must be last with no complex members on the stack.
	pOwner->TransferOwnershipToScript(pInterface, pChildInstance);
}

/** @return The state of conditionName. */
Bool ScriptUIManager::GetCondition(HString conditionName) const
{
	return UI::Manager::Get()->GetCondition(conditionName);
}

Float ScriptUIManager::GetPerspectiveFactorAdjustment() const
{
	return UI::Manager::Get()->GetRenderer().GetPerspectiveFactorAdjustment();
}

/** Update the state of conditionName. */
void ScriptUIManager::SetCondition(HString conditionName, Bool bValue)
{
	UI::Manager::Get()->SetCondition(conditionName, bValue);
}

void ScriptUIManager::SetPerspectiveFactorAdjustment(Float f)
{
	UI::Manager::Get()->GetRenderer().SetPerspectiveFactorAdjustment(f);
}

/** Send a UI broadcast to all UI::Movie instances on the stack. */
void ScriptUIManager::PersistentBroadcastEvent(Script::FunctionInterface* pInterface)
{
	HString sEvent;
	if (!pInterface->GetString(1u, sEvent))
	{
		pInterface->RaiseError(1u, "invalid event name, must be convertible to string.");
		return;
	}

	Int const iArguments = pInterface->GetArgumentCount() - 2;
	Reflection::MethodArguments aArguments;
	if (iArguments < 0 ||
		(UInt32)iArguments > aArguments.GetSize())
	{
		pInterface->RaiseError(-1, "too many arguments to BroadcastEvent, got %d, max of %u",
			iArguments,
			aArguments.GetSize());
		return;
	}

	for (Int i = 0; i < iArguments; ++i)
	{
		if (!pInterface->GetAny((UInt32)(i + 2), TypeId<void>(), aArguments[i]))
		{
			pInterface->RaiseError(i + 2, "invalid argument, must be convertible to Seoul::Reflection::Any.");
			return;
		}
	}

	Bool const bReturn = UI::Manager::Get()->BroadcastEvent(sEvent, aArguments, iArguments, true);
	pInterface->PushReturnBoolean(bReturn);
}

/** Send a UI broadcast to a specific UI::Movie on the stack. */
void ScriptUIManager::PersistentBroadcastEventTo(Script::FunctionInterface* pInterface)
{
	HString sTargetType;
	if (!pInterface->GetString(1u, sTargetType))
	{
		pInterface->RaiseError(1u, "invalid target type, must be convertible to string.");
		return;
	}

	HString sEvent;
	if (!pInterface->GetString(2u, sEvent))
	{
		pInterface->RaiseError(2u, "invalid event name, must be convertible to string.");
		return;
	}

	Int const iArguments = pInterface->GetArgumentCount() - 3;
	Reflection::MethodArguments aArguments;
	if (iArguments < 0 ||
		(UInt32)iArguments > aArguments.GetSize())
	{
		pInterface->RaiseError(-1, "too many arguments to BroadcastEvent, got %d, max of %u",
			iArguments,
			aArguments.GetSize());
		return;
	}

	for (Int i = 0; i < iArguments; ++i)
	{
		if (!pInterface->GetAny((UInt32)(i + 3), TypeId<void>(), aArguments[i]))
		{
			pInterface->RaiseError(i + 3, "invalid argument, must be convertible to Seoul::Reflection::Any.");
			return;
		}
	}

	Bool const bReturn = UI::Manager::Get()->BroadcastEventTo(sTargetType, sEvent, aArguments, iArguments, true);
	pInterface->PushReturnBoolean(bReturn);
}

void ScriptUIManager::SetStage3DSettings(HString name)
{
	UI::Manager::Get()->GetRenderer().ConfigureStage3DSettings(name);
}

void ScriptUIManager::ComputeWorldSpaceDepthProjection(Script::FunctionInterface* pInterface) const
{
	float fPosX = 0;
	float fPosY = 0;
	float fDepth3d = 0;

	if (!pInterface->GetNumber(1, fPosX))
	{
		pInterface->RaiseError(1, "expected float value.");
		return;
	}

	if (!pInterface->GetNumber(2, fPosY))
	{
		pInterface->RaiseError(2, "expected float value.");
		return;
	}

	if (!pInterface->GetNumber(3, fDepth3d))
	{
		pInterface->RaiseError(3, "expected float value.");
		return;
	}

	Vector2D projectedPosition = UI::Manager::Get()->GetRenderer().GetRenderState().Project(Vector2D(fPosX, fPosY), fDepth3d);

	pInterface->PushReturnNumber(projectedPosition.X);
	pInterface->PushReturnNumber(projectedPosition.Y);
}

void ScriptUIManager::ComputeInverseWorldSpaceDepthProjection(Script::FunctionInterface* pInterface) const
{
	float fPosX = 0;
	float fPosY = 0;
	float fDepth3d = 0;

	if (!pInterface->GetNumber(1, fPosX))
	{
		pInterface->RaiseError(1, "expected float value.");
		return;
	}

	if (!pInterface->GetNumber(2, fPosY))
	{
		pInterface->RaiseError(2, "expected float value.");
		return;
	}

	if (!pInterface->GetNumber(3, fDepth3d))
	{
		pInterface->RaiseError(3, "expected float value.");
		return;
	}


	Vector2D projectedPosition = UI::Manager::Get()->GetRenderer().GetRenderState().InverseProject(Vector2D(fPosX, fPosY), fDepth3d);

	pInterface->PushReturnNumber(projectedPosition.X);
	pInterface->PushReturnNumber(projectedPosition.Y);
}

/** Fire a UI trigger with name triggerName. */
void ScriptUIManager::TriggerTransition(HString triggerName)
{
	UI::Manager::Get()->TriggerTransition(triggerName);
}

/** Returnes whether there's pending, unprocessed transitions. */
Bool ScriptUIManager::HasPendingTransitions() const
{
	return UI::Manager::Get()->HasPendingTransitions();
}

/** @return The back buffer viewport aspect ratio. */
Float ScriptUIManager::GetViewportAspectRatio() const
{
	return UI::Manager::Get()->ComputeViewport().GetViewportAspectRatio();
}

/** Debug print the entire UI state. */
void ScriptUIManager::DebugLogEntireUIState() const
{
	UI::Manager::Get()->DebugLogEntireUIState();
}

HString ScriptUIManager::GetStateMachineCurrentStateId(HString sStateMachineName) const
{
	return UI::Manager::Get()->GetStateMachineCurrentStateId(sStateMachineName);
}

void ScriptUIManager::AddToInputWhitelist(Script::FunctionInterface* pInterface)
{
	auto p = pInterface->GetUserData<ScriptUIMovieClipInstance>(1);
	if (nullptr == p)
	{
		pInterface->RaiseError(1, "expected MovieClip.");
		return;
	}

	UI::Manager::Get()->AddToInputWhitelist(p->GetInstance());
}

void ScriptUIManager::ClearInputWhitelist()
{
	UI::Manager::Get()->ClearInputWhitelist();
}

void ScriptUIManager::RemoveFromInputWhitelist(Script::FunctionInterface* pInterface)
{
	auto p = pInterface->GetUserData<ScriptUIMovieClipInstance>(1);
	if (nullptr == p)
	{
		pInterface->RaiseError(1, "expected MovieClip.");
		return;
	}

	UI::Manager::Get()->RemoveFromInputWhitelist(p->GetInstance());
}

void ScriptUIManager::SetInputActionsEnabled(bool bEnabled)
{
	UI::Manager::Get()->SetInputActionsEnabled(bEnabled);
}

#if SEOUL_ENABLE_CHEATS
void ScriptUIManager::GotoState(HString stateMachineName, HString stateName)
{
	UI::Manager::Get()->GotoState(stateMachineName, stateName);
}
#endif // /#if SEOUL_ENABLE_CHEATS

#if !SEOUL_SHIP
Bool ScriptUIManager::ValidateUiFiles(const String& sExcludeWildcard) const
{
	return UI::Manager::Get()->ValidateUiFiles(sExcludeWildcard, true);
}
#endif // /#if !SEOUL_SHIP

void ScriptUIManager::TriggerRestart(Bool bForceImmediate)
{
	UI::Manager::Get()->TriggerRestart(bForceImmediate);
}

String ScriptUIManager::GetPlainTextString(const String& input) const
{
	String output;
	HtmlReader rReader(input.Begin(), input.End(), output);

	HtmlTag eNextTag = HtmlTag::kUnknown;
	HtmlTagStyle eNextTagStyle = HtmlTagStyle::kNone;

	while (true)
	{
		rReader.ReadTag(eNextTag, eNextTagStyle);
		if (HtmlTag::kTextChunk == eNextTag)
		{
			String::Iterator unusedBegin;
			String::Iterator unusedEnd;
			// Termination is indicated by a failure to read a text chunk.
			if (!rReader.ReadTextChunk(unusedBegin, unusedEnd))
			{
				break;
			}
		}
	}

	return output;
}

#if SEOUL_HOT_LOADING
void ScriptUIManager::ShelveDataForHotLoad(Script::FunctionInterface* pInterface)
{
	String sId;
	if (!pInterface->GetString(1, sId))
	{
		pInterface->RaiseError(1, "expected string.");
		return;
	}

	DataStore ds;
	if (!pInterface->GetTable(2, ds))
	{
		pInterface->RaiseError(1, "expected table.");
		return;
	}

	UI::Manager::Get()->ShelveDataForHotLoad(sId, ds);
}

void ScriptUIManager::UnshelveDataFromHotLoad(Script::FunctionInterface* pInterface)
{
	String sId;
	if (!pInterface->GetString(1, sId))
	{
		pInterface->RaiseError(1, "expected string.");
		return;
	}

	auto pData(UI::Manager::Get()->UnshelveDataFromHotLoad(sId));
	if (pData.IsValid())
	{
		pInterface->PushReturnDataNode(*pData, pData->GetRootNode());
	}
	else
	{
		pInterface->PushReturnNil();
	}
}
#endif

} // namespace Seoul
