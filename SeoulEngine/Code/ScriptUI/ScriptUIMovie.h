/**
 * \file ScriptUIMovie.h
 * \brief Derived class of UI::Movie that is scriptable. Interacts
 * with the script VM owned by ScriptUI in a consistent way
 * and allows for mirroring the Falcon scene graph into script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_UI_MOVIE_H
#define SCRIPT_UI_MOVIE_H

#include "HashSet.h"
#include "UIAdvanceInterfaceDeferredDispatch.h"
#include "UIMovie.h"
namespace Seoul { namespace Script { class FunctionInterface; } }
namespace Seoul { class ScriptUIInstance; }
namespace Seoul { namespace Script { class Vm; } }
namespace Seoul { namespace Script { class VmObject; } }
extern "C" { struct lua_State; }

namespace Seoul
{

/**
 * Specialization of UI::Movie, should be used
 * as an immediate or distant subclass of movies that want to
 * support Lua scripting.
 */
class ScriptUIMovie : public UI::Movie
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ScriptUIMovie);

	ScriptUIMovie(const SharedPtr<Script::Vm>& pVm, HString sTypeName = HString());
	virtual ~ScriptUIMovie();

	// Add (potentially multiple) new fx definitions
	// to the movie's FxFactory.
	void AppendFx(Script::FunctionInterface* pInterface);

	// Add (potentially multiple) new sound event definitions
	// to the movie's SoundEventFactory.
	void AppendSoundEvents(Script::FunctionInterface* pInterface);

	const SharedPtr<Script::VmObject>& GetRootMovieClipBinding() const
	{
		return m_pRootMovieClipBinding;
	}

	SharedPtr<Falcon::MovieClipInstance> CreateMovieClip(HString typeName);

	void GetWorldCullBounds(Script::FunctionInterface* pInterface) const;

	void TrackBinding(const SharedPtr<Script::VmObject>& p);

	void TransferOwnershipToScript(
		Script::FunctionInterface* pInterface,
		SharedPtr<Falcon::Instance>& rpInstance);

	virtual void OnEditTextStartEditing(const SharedPtr<Falcon::MovieClipInstance>& pInstance) SEOUL_OVERRIDE;
	virtual void OnEditTextStopEditing(const SharedPtr<Falcon::MovieClipInstance>& pInstance) SEOUL_OVERRIDE;
	virtual void OnEditTextApply(const SharedPtr<Falcon::MovieClipInstance>& pInstance) SEOUL_OVERRIDE;

protected:
	// These methods are sealed to discourage use higher up in
	// the stacks. They are limited to script construction,
	// frame 0 setup, and teardown respectively.
	//
	// OnPostResolveRootMovieClipBinding() is provided to allow "safe"
	// and more specific hooking for subclasses.
	virtual void OnConstructMovie(HString typeName) SEOUL_OVERRIDE SEOUL_SEALED;
	virtual void OnDestroyMovie() SEOUL_OVERRIDE SEOUL_SEALED;

	virtual void OnPostResolveRootMovieClipBinding() {}

	virtual Bool CanSuspendMovie() const SEOUL_OVERRIDE { return m_bCanSuspend; }
	virtual void OnResumeMovie() SEOUL_OVERRIDE;
	virtual void OnSuspendMovie() SEOUL_OVERRIDE;

	virtual void OnAdvance(Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;
	virtual void OnAdvanceWhenBlocked(Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;

	// Falcon::AddInterface overrides
	virtual void FalconOnAddToParent(
		Falcon::MovieClipInstance* pParent,
		Falcon::Instance* pInstance,
		const HString& sClassName) SEOUL_OVERRIDE;
	// /Falcon::AddInterface overrides

	virtual void FalconDispatchEnterFrameEvent(
		Falcon::Instance* pInstance) SEOUL_OVERRIDE;

	virtual void FalconDispatchEvent(
		const HString& sEventName,
		Falcon::SimpleActions::EventType eType,
		Falcon::Instance* pInstance) SEOUL_OVERRIDE;

	virtual UI::MovieHitTestResult OnSendInputEvent(UI::InputEvent eInputEvent) SEOUL_OVERRIDE;
	virtual UI::MovieHitTestResult OnSendButtonEvent(
		Seoul::InputButton eButtonID,
		Seoul::ButtonEventType eButtonEventType) SEOUL_OVERRIDE;

	virtual void OnEnterState(CheckedPtr<UI::State const> pPreviousState, CheckedPtr<UI::State const> pNextState, Bool bWasInPreviousState) SEOUL_OVERRIDE;
	virtual void OnExitState(CheckedPtr<UI::State const> pPreviousState, CheckedPtr<UI::State const> pNextState, Bool bIsInNextState) SEOUL_OVERRIDE;
	virtual void OnLoad() SEOUL_OVERRIDE;
	virtual Bool AllowClickPassthroughToProceed(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance) const SEOUL_OVERRIDE;
	virtual void OnGlobalMouseButtonPressed(const Point2DInt& mousePosition, const SharedPtr<Falcon::MovieClipInstance>& pInstance) SEOUL_OVERRIDE;
	virtual void OnGlobalMouseButtonReleased(const Point2DInt& mousePosition) SEOUL_OVERRIDE;
	virtual void OnMouseButtonPressed(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance,
		Bool bInInstance) SEOUL_OVERRIDE;
	virtual void OnMouseButtonReleased(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance,
		Bool bInInstance,
		UInt8 uInputCaptureHitTestMask) SEOUL_OVERRIDE;
	virtual void OnMouseMove(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance,
		Bool bInInstance) SEOUL_OVERRIDE;
	virtual void OnMouseWheel(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance,
		Float fDelta) SEOUL_OVERRIDE;
	virtual void OnMouseOut(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance) SEOUL_OVERRIDE;
	virtual void OnMouseOver(
		const Point2DInt& mousePosition,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance) SEOUL_OVERRIDE;
	virtual void OnLinkClicked(
		const String& sLinkInfo,
		const String& sLinkType,
		const SharedPtr<Falcon::MovieClipInstance>& pInstance) SEOUL_OVERRIDE;

	virtual void InvokePassthroughInputFunction() SEOUL_OVERRIDE;

#if SEOUL_HOT_LOADING
	virtual void OnHotLoadBegin() SEOUL_OVERRIDE;
	virtual void OnHotLoadEnd() SEOUL_OVERRIDE;
#endif

	virtual void OnDispatchTickEvent(Falcon::Instance* pInstance) const SEOUL_OVERRIDE;
	virtual void OnDispatchTickScaledEvent(Falcon::Instance* pInstance) const SEOUL_OVERRIDE;

	virtual Bool OnTryBroadcastEvent(
		HString eventName,
		const Reflection::MethodArguments& aMethodArguments,
		Int iArgumentCount) SEOUL_OVERRIDE;

	friend class ScriptUIMovieMutator;
	virtual Bool OnTryInvoke(
		HString methodName,
		const Reflection::MethodArguments& aMethodArguments,
		Int iArgumentCount,
		Bool bNativeCall);

private:
	SEOUL_DISABLE_COPY(ScriptUIMovie);
	SEOUL_REFLECTION_FRIENDSHIP(ScriptUIMovie);

	struct MovieClipTemplate SEOUL_SEALED
	{
		SharedPtr<Falcon::MovieClipInstance> m_pTemplateRootInstance;
		ScopedPtr<UI::AdvanceInterfaceDeferredDispatch> m_pTemplateAdvanceInterface;
	}; // struct NewMovieClipTemplate
	typedef HashTable<HString, MovieClipTemplate*, MemoryBudgets::Falcon> MovieClipTemplateCache;
	MovieClipTemplateCache m_tMovieClipTemplateCache;

	typedef HashSet< SharedPtr<Script::VmObject>, MemoryBudgets::Falcon > Bindings;
	Bindings m_Bindings;
	typedef Vector< SharedPtr<Script::VmObject>, MemoryBudgets::Falcon > BindingsVector;
	BindingsVector m_vBindings;

	SharedPtr<Script::VmObject> m_pExternalInterfaceBinding;
protected:
	SharedPtr<Script::VmObject> m_pRootMovieClipBinding;
private:
	SharedPtr<Script::Vm> m_pVm;
	SharedPtr<Falcon::MovieClipInstance> m_pCapturedMovieClipInstance;

	// Used to keep hard pointers to UI::AdvanceInterfaceDeferredDispatch, which
	// need to be carried into a context where script can longjmp (and therefore
	// these might otherwise leak).
	typedef Vector<UI::AdvanceInterfaceDeferredDispatch*, MemoryBudgets::Scripting> DispatchGarbage;
	DispatchGarbage m_vDispatchGarbage;

	HString m_sTypeName;
	Bool m_bAdvancedOnce;
	Bool m_bCanSuspend;

	void FlushDeferredDraw();

	ScriptUIInstance* PushNewScriptBinderInstance(
		Script::FunctionInterface* pInterface,
		Falcon::Instance* pInstance) const;
	void ReplaceScriptBinderOnStackWithScriptTable(
		UI::AdvanceInterfaceDeferredDispatch* pDeferredDispatch,
		lua_State* pVm,
		HString className,
		ScriptUIInstance* pBinder,
		Bool bRoot,
		Int iFirstArg = -1,
		Int iLastArg = -1);
	void ResolveRootMovieClipBinding();
	// /Internal methods.

	// Script bound methods.
	void BindAndConstructRoot(Script::FunctionInterface* pInterface);
	void GetSiblingRootMovieClip(Script::FunctionInterface* pInterface);
	void GetStateConfigValue(Script::FunctionInterface* pInterface);
	void NewBitmap(Script::FunctionInterface* pInterface);
	void NewMovieClip(Script::FunctionInterface* pInterface);
	void GetMousePositionFromWorld(Script::FunctionInterface* pInterface);
	void OnAddToParent(Script::FunctionInterface* pInterface);
	void ReturnMousePositionInWorld(Script::FunctionInterface* pInterface) const;
	void GetRootMovieClip(Script::FunctionInterface* pInterface) const;
	Int32 GetLastViewportWidth() const;
	Int32 GetLastViewportHeight() const;
	// /Script bound methods.

	Bool m_bConstructedScript;
}; // class ScriptUIMovie

/** Shared across several bits of ScriptUI* functionality. */
static const HString kDefaultMovieClipClassName("MovieClip");

} // namespace Seoul

#endif // include guard
