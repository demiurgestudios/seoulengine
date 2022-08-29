/**
 * \file ScriptUIMovie.cpp
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

#include "Engine.h"
#include "FalconBitmapInstance.h"
#include "FalconEditTextInstance.h"
#include "FalconMovieClipInstance.h"
#include "FileManager.h"
#include "LocManager.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "RenderDevice.h"
#include "Renderer.h"
#include "ScriptFunctionInterface.h"
#include "ScriptFunctionInvoker.h"
#include "ScriptUIAnimation2DNetworkInstance.h"
#include "ScriptUIBitmapInstance.h"
#include "ScriptUIEditTextInstance.h"
#include "ScriptUIFxInstance.h"
#include "ScriptUIMovie.h"
#include "ScriptUIMovieClipInstance.h"
#include "ScriptVm.h"
#include "UIFxInstance.h"
#include "UIManager.h"
#include "UIMovieInternal.h"
#include "UIUtil.h"

namespace Seoul
{

static const HString kBindAndConstructRoot("BindAndConstructRoot");
static const HString kDestructor("Destructor");
static const HString kDispatchEvent("DispatchEvent");
static const HString kLinkClicked("linkClicked");
static const HString kOnAddToParent("OnAddToParent");
static const HString kOnEnterState("OnEnterState");
static const HString kOnExitState("OnExitState");
static const HString kOnInputEvent("OnInputEvent");
static const HString kOnButtonDownEvent("OnButtonDownEvent");
static const HString kOnButtonReleasedEvent("OnButtonReleasedEvent");
static const HString kOnLoad("OnLoad");
static const HString kMouseDown("mouseDown");
static const HString kMouseMove("mouseMove");
static const HString kMouseOut("mouseOut");
static const HString kMouseOver("mouseOver");
static const HString kMouseUp("mouseUp");
static const HString kMouseWheel("mouseWheel");
static const HString kMovieMouseDown("movieMouseDown");
static const HString kMovieMouseUp("movieMouseUp");
static const HString kStartEditing("startEditing");
static const HString kStopEditing("stopEditing");
static const HString kApplyEditing("applyEditing");
static const HString kAllowClickPassthroughToProceed("allowclickpassthrough");
static const HString kYieldToTasks("YieldToTasks");
static const HString kTickIgnoringPause("TickIgnoringPause");
static const HString kOnResumeMovie("OnResumeMovie");
static const HString kOnSuspendMovie("OnSuspendMovie");

#if SEOUL_HOT_LOADING
static const HString kHotLoadBegin("OnHotLoadBegin");
static const HString kHotLoadEnd("OnHotLoadEnd");
#endif

SEOUL_BEGIN_TYPE(ScriptUIMovie, TypeFlags::kDisableNew)
	SEOUL_PARENT(UI::Movie)
	SEOUL_METHOD(AppendFx)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "params object[] asArgs")
	SEOUL_METHOD(AppendSoundEvents)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "params string[] asArgs")
	SEOUL_METHOD(BindAndConstructRoot)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "string sClassName")
	SEOUL_METHOD(GetMovieTypeName)
	SEOUL_METHOD(GetStateConfigValue)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "object", "string sKey")
	SEOUL_METHOD(GetSiblingRootMovieClip)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "RootMovieClip", "string sSiblingName")
	SEOUL_METHOD(GetWorldCullBounds)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double, double, double)")
	SEOUL_METHOD(NewBitmap)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "Bitmap")
	SEOUL_METHOD(NewMovieClip)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "MovieClip", "string sClassName, params object[] varargs")
	SEOUL_METHOD(GetMousePositionFromWorld)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)", "double fX, double fY")
	SEOUL_METHOD(OnAddToParent)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "object ludInstance")
	SEOUL_METHOD(ReturnMousePositionInWorld)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)")
	SEOUL_METHOD(GetRootMovieClip)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "RootMovieClip")
	SEOUL_METHOD(GetLastViewportWidth)
	SEOUL_METHOD(GetLastViewportHeight)
SEOUL_END_TYPE()

ScriptUIMovie::ScriptUIMovie(const SharedPtr<Script::Vm>& pVm, HString sTypeName /* = HString()*/)
	: m_pExternalInterfaceBinding()
	, m_pRootMovieClipBinding()
	, m_pVm(pVm)
	, m_pCapturedMovieClipInstance()
	, m_vDispatchGarbage()
	, m_sTypeName(sTypeName)
	, m_bAdvancedOnce(false)
	, m_bCanSuspend(false)
	, m_bConstructedScript(false)
{
	// Sanity check.
	SEOUL_ASSERT(m_pVm.IsValid());
}

ScriptUIMovie::~ScriptUIMovie()
{
	SEOUL_ASSERT(!m_bConstructedScript);

	// Final cleanup.
	SafeDeleteVector(m_vDispatchGarbage);

	m_pRootMovieClipBinding.Reset();
	m_pExternalInterfaceBinding.Reset();
	m_pVm.Reset();
}

/**
 * Add (potentially multiple) new fx definitions
 * to the movie's FxFactory.
 */
void ScriptUIMovie::AppendFx(Script::FunctionInterface* pInterface)
{
	// 0 is always self.
	auto& fx = m_Content.GetFx();
	auto const iArgs = pInterface->GetArgumentCount();
	for (Int32 i = 1; i < iArgs; )
	{
		HString key;
		if (!pInterface->GetString(i++, key))
		{
			pInterface->RaiseError(i - 1, "expected fx id");
			return;
		}

		FilePath filePath;
		if (!pInterface->GetFilePath(i++, filePath))
		{
			pInterface->RaiseError(i - 1, "expected fx FilePath");
			return;
		}

		fx.AppendFx(key, filePath);
	}
}

/**
 * Add (potentially multiple) new sound event definitions
 * to the movie's SoundEventFactory.
 */
void ScriptUIMovie::AppendSoundEvents(Script::FunctionInterface* pInterface)
{
	// 0 is always self.
	auto& sound = m_Content.GetSoundEvents();
	auto const iArgs = pInterface->GetArgumentCount();
	for (Int32 i = 1; i < iArgs; )
	{
		HString key;
		if (!pInterface->GetString(i++, key))
		{
			pInterface->RaiseError(i - 1, "expected sound event key");
			return;
		}

		Byte const* sEventId = nullptr;
		UInt32 uEventId = 0u;
		if (!pInterface->GetString(i++, sEventId, uEventId))
		{
			pInterface->RaiseError(i - 1, "expected sound event id");
			return;
		}

		FilePath filePath;
		if (pInterface->IsUserData(i) && pInterface->GetFilePath(i, filePath))
		{
			++i;
		}

		sound.AppendSoundEvent(
			key,
			ContentKey(filePath, HString(sEventId, uEventId, true)));
	}
}

SharedPtr<Falcon::MovieClipInstance> ScriptUIMovie::CreateMovieClip(HString typeName)
{
	SharedPtr<Falcon::FCNFile> pFile(m_pInternal->GetFCNFile());
	if (!pFile.IsValid())
	{
		return SharedPtr<Falcon::MovieClipInstance>();
	}

	SharedPtr<Falcon::Definition> pDefinition;

	// Special handling, the base MovieClip class returns an empty SpriteDefinition.
	if (kDefaultMovieClipClassName == typeName)
	{
		pDefinition.Reset(SEOUL_NEW(MemoryBudgets::Falcon) Falcon::MovieClipDefinition(1, 0));
	}
	else
	{
		if (!pFile->GetExportedDefinition(typeName, pDefinition))
		{
			(void)pFile->GetImportedDefinition(typeName, pDefinition, true);
		}
	}

	if (!pDefinition.IsValid())
	{
		return SharedPtr<Falcon::MovieClipInstance>();
	}

	SharedPtr<Falcon::MovieClipInstance> pInstance;
	pDefinition->CreateInstance(pInstance);

	return pInstance;
}

void ScriptUIMovie::GetWorldCullBounds(Script::FunctionInterface* pInterface) const
{
	SharedPtr<Falcon::FCNFile> pFile(m_pInternal->GetFCNFile());
	if (!pFile.IsValid())
	{
		return;
	}

	Falcon::Rectangle stageBounds = pFile->GetBounds();
	Viewport activeViewport = GetViewport();
	Falcon::Rectangle worldCullRectangle = Falcon::Rectangle::Create(0, 0, 0, 0);

	// Cache the stage dimensions.
	Float32 const fStageHeight = (Float32)stageBounds.GetHeight();
	Float32 const fStageWidth = (Float32)stageBounds.GetWidth();

	// Cache top and bottom.
	auto const vStageCoords = ComputeStageTopBottom(activeViewport, fStageHeight);
	Float32 const fStageTopRenderCoord = vStageCoords.X;
	Float32 const fStageBottomRenderCoord = vStageCoords.Y;

	// Compute the factor.
	Float32 const fVisibleHeight = (fStageBottomRenderCoord - fStageTopRenderCoord);
	Float32 const fVisibleWidth = (fVisibleHeight *
		activeViewport.GetViewportAspectRatio());

	worldCullRectangle.m_fLeft = (fStageWidth - fVisibleWidth) / 2.0f;
	worldCullRectangle.m_fTop = fStageTopRenderCoord;
	worldCullRectangle.m_fBottom = fStageBottomRenderCoord;
	worldCullRectangle.m_fRight = (fStageWidth - worldCullRectangle.m_fLeft);

	pInterface->PushReturnNumber(worldCullRectangle.m_fLeft);
	pInterface->PushReturnNumber(worldCullRectangle.m_fTop);
	pInterface->PushReturnNumber(worldCullRectangle.m_fRight);
	pInterface->PushReturnNumber(worldCullRectangle.m_fBottom);
}

/**
 * Given a native instance, pushes the script binding of the
 * instance. If previously created, retrieves it from
 * the global weak lookup table. Can also push nil if
 * the instance is about to be garbage collected.
 */
void ScriptUIMovie::TransferOwnershipToScript(
	Script::FunctionInterface* pInterface,
	SharedPtr<Falcon::Instance>& rpInstance)
{
	// Easy case, early out - just push nil.
	if (!rpInstance.IsValid())
	{
		pInterface->PushReturnNil();
		return;
	}

	// Lookup in the script system's weak registry.
	if (pInterface->PushReturnBinderFromWeakRegistry(rpInstance.GetPtr()))
	{
		// Verify - rpInstance must have a reference count > 1u.
		SEOUL_ASSERT(rpInstance.GetReferenceCount() > 1u);

		// Release stack reference.
		rpInstance.Reset();

		// Done, resolve binder is now on the stack.
		return;
	}

	// Potentially generate a new instance - PushNewScriptBinderInstance
	// may push nil if pInstance is in the process of being garbage collected.
	auto pBinder = PushNewScriptBinderInstance(pInterface, rpInstance.GetPtr());

	// Done if binder is null - nil is already on the stack.
	if (nullptr == pBinder)
	{
		// Release the reference.
		rpInstance.Reset();
		return;
	}

	// Release the stack reference before the last step,
	// since it may longjmp. rpInstance must have a reference
	// count > 1u.
	SEOUL_ASSERT(rpInstance.GetReferenceCount() > 1u);
	rpInstance.Reset();

	// Push new instance - this will "replace"
	// the native instance already on the stack,
	// so we don't need to manipulate pInterface
	// any further.
	ReplaceScriptBinderOnStackWithScriptTable(
		nullptr,
		pInterface->GetLowLevelVm(),
		pBinder->GetClassName(),
		pBinder,
		false);
}

void ScriptUIMovie::TrackBinding(const SharedPtr<Script::VmObject>& p)
{
	(void)m_Bindings.Insert(p);
}

void ScriptUIMovie::OnConstructMovie(HString movieTypeName)
{
	SEOUL_ASSERT(!m_bConstructedScript);

	UI::Movie::OnConstructMovie(movieTypeName);

	// Cache the VM and bind the native (External) interface.
	SEOUL_VERIFY(m_pVm->BindWeakInstance(GetReflectionThis(), m_pExternalInterfaceBinding));

	// Construct the Lua object that is the script binding around the
	// root MovieClip. Also sets m_bConstructedScript to true.
	ResolveRootMovieClipBinding();

	// Give subclasses a chance to perform setup write after the root has been
	// constructed.
	OnPostResolveRootMovieClipBinding();

	// Now that we have a root (possibly), check if we're suspendable.
	m_bCanSuspend = false;
	if (m_pRootMovieClipBinding.IsValid())
	{
		Script::FunctionInvoker suspend(m_pRootMovieClipBinding, kOnSuspendMovie);
		Script::FunctionInvoker resume(m_pRootMovieClipBinding, kOnResumeMovie);
		if (suspend.IsValid() || resume.IsValid())
		{
			// If both are not defined, flag this as an error.
			if (!suspend.IsValid())
			{
				SEOUL_WARN("%s: movie defines OnResumeMovie but not OnSuspendMovie, both "
					"or neither must be defined. Treating as not resumable.", GetMovieTypeName().CStr());
			}
			else if (!resume.IsValid())
			{
				SEOUL_WARN("%s: movie defines OnSuspendMovie but not OnResumeMovie, both "
					"or neither must be defined. Treating as not resumable.", GetMovieTypeName().CStr());
			}
			else
			{
				m_bCanSuspend = true;
			}
		}
	}
}

void ScriptUIMovie::OnDestroyMovie()
{
	SEOUL_ASSERT(m_bConstructedScript);

	// Give the script a chance to do cleanup.
	{
		Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDestructor);
		if (invoker.IsValid())
		{
			(void)invoker.TryInvoke();
		}
	}

	// Cleanup bindings.
	{
		auto const iBegin = m_Bindings.Begin();
		auto const iEnd = m_Bindings.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			(*i)->ReleaseRef();
		}
	}
	m_Bindings.Clear();

	m_bConstructedScript = false;

	// Release our resources.
	m_pRootMovieClipBinding.Reset();
	if (m_pExternalInterfaceBinding.IsValid())
	{
		m_pExternalInterfaceBinding->SetWeakBindingToNil();
	}
	m_pExternalInterfaceBinding.Reset();
	m_pVm.Reset();

	// Release any movie in the template cache.
	SafeDeleteTable(m_tMovieClipTemplateCache);

	UI::Movie::OnDestroyMovie();
}

/**
 * Utility, given the movie type name, returns the root movie clip
 * of that movie. Must be in the same state machine of the
 * current movie (has to be a sibling).
 */
void ScriptUIMovie::GetSiblingRootMovieClip(Script::FunctionInterface* pInterface)
{
	SharedPtr<Falcon::Instance> pReturn;
	{
		HString key;
		if (!pInterface->GetString(1, key))
		{
			pInterface->RaiseError(1, "expected string key for configuration lookup.");
			return;
		}

		SharedPtr<Falcon::MovieClipInstance> pInstance;

		// Search right.
		auto bDone = false;
		auto pMovie = GetNextMovie();
		while (nullptr != pMovie)
		{
			if (pMovie->GetMovieTypeName() == key)
			{
				pMovie->GetRootMovieClip(pInstance);
				bDone = true;
				break;
			}

			pMovie = pMovie->GetNextMovie();
		}

		// Search left.
		if (!bDone)
		{
			pMovie = GetPrevMovie();
			while (nullptr != pMovie)
			{
				if (pMovie->GetMovieTypeName() == key)
				{
					pMovie->GetRootMovieClip(pInstance);
					bDone = true;
					break;
				}

				pMovie = pMovie->GetPrevMovie();
			}
		}

		// Found an instance, done.
		pReturn = pInstance;
	}

	// Push return.
	TransferOwnershipToScript(pInterface, pReturn);
}

void ScriptUIMovie::GetStateConfigValue(Script::FunctionInterface* pInterface)
{
	HString key;
	if (!pInterface->GetString(1, key))
	{
		pInterface->RaiseError(1, "expected string key for configuration lookup.");
		return;
	}

	auto pOwner = GetOwner();
	if (!pOwner)
	{
		pInterface->PushReturnNil();
		return;
	}

	DataStore const* pDataStore = nullptr;
	DataNode node;
	if (!pOwner->GetConfiguration(pDataStore, node))
	{
		pInterface->PushReturnNil();
		return;
	}
	if (!pDataStore->GetValueFromTable(node, key, node))
	{
		pInterface->PushReturnNil();
		return;
	}

	if (!pInterface->PushReturnDataNode(*pDataStore, node))
	{
		pInterface->PushReturnNil();
		return;
	}
}

void ScriptUIMovie::NewBitmap(Script::FunctionInterface* pInterface)
{
	SharedPtr<Falcon::Instance> pChildInstance(
		SEOUL_NEW(MemoryBudgets::UIRuntime) Falcon::BitmapInstance());

	// Can longjmp, so must be last with no complex members on the stack
	// except for the instance being returned.
	TransferOwnershipToScript(pInterface, pChildInstance);
}

void ScriptUIMovie::NewMovieClip(Script::FunctionInterface* pInterface)
{
	// Binder will be on the lua stack and is the only bit that can be
	// safely on the stack outside the next scope, since
	// we will call a raw lua function at the very end.
	ScriptUIInstance* pBinder = nullptr;
	UI::AdvanceInterfaceDeferredDispatch* pDeferredDispatch = nullptr;
	{
		SharedPtr<Falcon::FCNFile> pFile(m_pInternal->GetFCNFile());
		if (!pFile.IsValid())
		{
			pInterface->RaiseError(-1, "failed instantiating MovieClip, no FCN file.");
			return;
		}

		Byte const* sMovieClipName = nullptr;
		UInt32 zClassNameLengthInBytes = 0;
		if (!pInterface->GetString(1, sMovieClipName, zClassNameLengthInBytes))
		{
			pInterface->RaiseError(1, "string name of class to instantiate is required.");
			return;
		}

		HString const movieClipName(sMovieClipName, zClassNameLengthInBytes);

		// Try to retrieve an entry from the cache. Otherwise, generate one and
		// cache it.
		MovieClipTemplate* pTemplate = nullptr;
		if (!m_tMovieClipTemplateCache.GetValue(movieClipName, pTemplate))
		{
			SharedPtr<Falcon::Definition> pDefinition;

			// Special handling, the base MovieClip class returns an empty SpriteDefinition.
			if (kDefaultMovieClipClassName == movieClipName)
			{
				pDefinition.Reset(SEOUL_NEW(MemoryBudgets::Falcon) Falcon::MovieClipDefinition(1, 0));
			}
			else
			{
				if (!pFile->GetExportedDefinition(movieClipName, pDefinition))
				{
					if (!pFile->GetImportedDefinition(movieClipName, pDefinition, true))
					{
						// TODO: Verify - it appears that classes which are
						// used entirely internal in ActionScript
						// (never placed on the stage) do not get
						// exported and thus we need to fallback to an empty
						// MovieClip with the specified MovieClip name.
						pDefinition.Reset(SEOUL_NEW(MemoryBudgets::Falcon) Falcon::MovieClipDefinition(movieClipName));
					}
				}
			}

			if (!pDefinition.IsValid())
			{
				pInterface->RaiseError(-1, "failed instantiating MovieClip, check that, '%s', has a corresponding exported ActionScript name in Flash.", movieClipName.CStr());
				return;
			}

			SharedPtr<Falcon::MovieClipInstance> pInstance;
			pDefinition->CreateInstance(pInstance);

			if (!pInstance.IsValid())
			{
				pInterface->RaiseError(-1, "failed instantiating MovieClip, definition instancing error, this is unexpected.");
				return;
			}

			ScopedPtr<UI::AdvanceInterfaceDeferredDispatch> pDispatch(SEOUL_NEW(MemoryBudgets::UIRuntime) UI::AdvanceInterfaceDeferredDispatch);
			pInstance->Advance(*pDispatch);

			pTemplate = SEOUL_NEW(MemoryBudgets::Falcon) MovieClipTemplate;
			pTemplate->m_pTemplateAdvanceInterface.Swap(pDispatch);
			pTemplate->m_pTemplateRootInstance.Swap(pInstance);
			SEOUL_VERIFY(m_tMovieClipTemplateCache.Insert(movieClipName, pTemplate).Second);
		}

		// Two cases - if no events were enqueued when creating the template,
		// we can just clone the instance. Otherwise, we need to clone the template
		// interface first, then clone the instance against the cloned template
		// (so it fixes up instance references to point at the clones). Finally,
		// we dispatch those deferred events.
		SharedPtr<Falcon::Instance> pInstance;
		if (pTemplate->m_pTemplateAdvanceInterface->HasEventsToDispatch())
		{
			// Clone and track.
			pDeferredDispatch = pTemplate->m_pTemplateAdvanceInterface->Clone();
			m_vDispatchGarbage.PushBack(pDeferredDispatch);

			// Add watched and remove afterwards so we get cloned callbacks.
			pTemplate->m_pTemplateAdvanceInterface->MarkWatched();
			pInstance.Reset(pTemplate->m_pTemplateRootInstance->Clone(*pDeferredDispatch));
			pTemplate->m_pTemplateAdvanceInterface->MarkNotWatched();
		}
		// No events, can just clone away.
		else
		{
			pInstance.Reset(pTemplate->m_pTemplateRootInstance->Clone(*this));
		}

		// Populate the binder before leaving scope.
		pBinder = PushNewScriptBinderInstance(pInterface, pInstance.GetPtr());

		// If binder is null, immediately return, nothing more to do - instance
		// should be released (sanity that count is still 1).
		if (nullptr == pBinder)
		{
			SEOUL_ASSERT(pInstance.IsUnique());
			return;
		}
		// Otherwise, native binder is on the stack - verify that pInstance
		// has (at least) 2 references before leaving the scope.
		else
		{
			SEOUL_ASSERT(pInstance.GetReferenceCount() > 1u);
		}
	}

	// Compute arguments start and end for constructor.
	Int const iFirstArg = (pInterface->GetArgumentCount() > 2
		? 3
		: -1); // ReplaceScriptBinderOnStackWithScriptTable is 1-based
	Int const iLastArg = (pInterface->GetArgumentCount() > 2
		? (Int)pInterface->GetArgumentCount()
		: -1);

	// Now we can safely invoke the final (raw) lua function
	// to setup the movie clip's script table - note that
	// this effectively "swaps" the native instance on the stack
	// for a class table, so we don't need to manipulate pInterface
	// in any way. Verify a return value of 1 from PushScriptTableInstance().
	ReplaceScriptBinderOnStackWithScriptTable(
		pDeferredDispatch,
		pInterface->GetLowLevelVm(),
		pBinder->GetClassName(),
		pBinder,
		false,
		iFirstArg,
		iLastArg);
}

template <typename T>
inline static T* InternalStaticPushInstance(
	const ScriptUIMovie& owner,
	Script::FunctionInterface* pInterface,
	Falcon::Instance* pInstance)
{
	auto pReturn = pInterface->PushReturnUserData<T>();

	// Early out if no instance.
	if (nullptr == pReturn)
	{
		pInterface->PushReturnNil();
		return nullptr;
	}

	// Invoke the native constructor.
	pReturn->Construct(SharedPtr<Falcon::Instance>(pInstance), owner);
	return pReturn;
}

void ScriptUIMovie::BindAndConstructRoot(Script::FunctionInterface* pInterface)
{
	HString className;
	if (!pInterface->GetString(1, className))
	{
		pInterface->RaiseError(1, "expected class name");
	}

	auto pBinder = PushNewScriptBinderInstance(pInterface, m_pInternal->GetRoot().GetPtr());
	ReplaceScriptBinderOnStackWithScriptTable(
		&m_pInternal->GetDeferredDispatch(),
		pInterface->GetLowLevelVm(),
		className,
		pBinder,
		true);
}

ScriptUIInstance* ScriptUIMovie::PushNewScriptBinderInstance(
	Script::FunctionInterface* pInterface,
	Falcon::Instance* pInstance) const
{
	// First, verify pInstance - if it is null or if it
	// is about to be garbage collected, just push nil.
	if (nullptr == pInstance ||
		// About to be GCed if has a watcher and only one strong reference.
		(pInstance->GetWatcherCount() > 0u &&
			1 == SeoulGlobalGetReferenceCount(pInstance)))
	{
		pInterface->PushReturnNil();
		return nullptr;
	}

	// Otherwise, create a wrapper native instance and an interface
	// table for return.
	switch (pInstance->GetType())
	{
#if SEOUL_WITH_ANIMATION_2D
	case Falcon::InstanceType::kAnimation2D:
		return InternalStaticPushInstance<ScriptUIAnimation2DNetworkInstance>(
			*this,
			pInterface,
			pInstance);
#endif // /#if SEOUL_WITH_ANIMATION_2D

	case Falcon::InstanceType::kBitmap:
		return InternalStaticPushInstance<ScriptUIBitmapInstance>(
			*this,
			pInterface,
			pInstance);

	case Falcon::InstanceType::kEditText:
		return InternalStaticPushInstance<ScriptUIEditTextInstance>(
			*this,
			pInterface,
			pInstance);

	case Falcon::InstanceType::kFx:
		return InternalStaticPushInstance<ScriptUIFxInstance>(
			*this,
			pInterface,
			pInstance);

	case Falcon::InstanceType::kMovieClip:
		return InternalStaticPushInstance<ScriptUIMovieClipInstance>(
			*this,
			pInterface,
			pInstance);

	default:
		return InternalStaticPushInstance<ScriptUIInstance>(
			*this,
			pInterface,
			pInstance);
	};
}

void ScriptUIMovie::ReplaceScriptBinderOnStackWithScriptTable(
	UI::AdvanceInterfaceDeferredDispatch* pDeferredDispatch,
	lua_State* pVm,
	HString className,
	ScriptUIInstance* pBinder,
	Bool bRoot,
	Int iFirstArg /*= -1*/,
	Int iLastArg /*= -1*/)
{
	// Sanity check, binder must never be null into this function.
	SEOUL_ASSERT(nullptr != pBinder);

	SEOUL_SCRIPT_CHECK_VM_STACK(pVm);

	// Now fullly construct the native script class from the new native instance.
	// Native binder is expected to already be on the stack at -1.

	// Create a new table, then assign the appropriate instance
	// pointers to it.
	lua_createtable(pVm, 0, 0);

	// Copy for next operation.
	lua_pushvalue(pVm, -1);

	// Set the instance table as the environment table of the native user data instance
	SEOUL_VERIFY(1 == lua_setfenv(pVm, -3));

	// Set instance and movie pointers to the class table.
	lua_insert(pVm, -2); // Effectively swap top table and native instance - native instance will be popped from the stack with the next line.
	lua_setfield(pVm, -2, "m_udNativeInstance");
	m_pExternalInterfaceBinding->PushOntoVmStack(pVm);
	lua_setfield(pVm, -2, "m_udNativeInterface");

	// If this is the root construction, bind the native instance.
	if (bRoot)
	{
		lua_pushvalue(pVm, -1);
		Int const iObject = luaL_ref(pVm, LUA_REGISTRYINDEX);
		m_pRootMovieClipBinding.Reset(SEOUL_NEW(MemoryBudgets::Scripting) Script::VmObject(
			m_pVm->GetHandle(),
			iObject));
	}

	// Now setup the class table's metatable and invoke its constructor -
	// constructor invocation is the bit that is likely to raise a lua exception.
	lua_getfield(pVm, LUA_GLOBALSINDEX, className.CStr());

	// Sanity handling of undefined classes.
	if (lua_isnil(pVm, -1))
	{
		// This line will longjmp.
		luaL_error(pVm, "attempt to instantiate DisplayObject of type '%s' but class is undefined.", className.CStr());
		return;
	}

	// Set metatable.
	lua_pushvalue(pVm, -1); // Keep an "extra" copy of the class table on the stack, we'll need it shortly.
	lua_setmetatable(pVm, -3);

	// Prior to running the constructor, associated in the weak registry. This is
	// required, as in some cases a lookup may occur that references this node
	// from within the chain of executions that happen in the script constructor.
	{
		// Get the weak registry.
		lua_pushlightuserdata(pVm, Script::kpScriptWeakRegistryKey);
		lua_rawget(pVm, LUA_REGISTRYINDEX);
		// Push the pointer.
		lua_pushlightuserdata(pVm, pBinder->GetInstance().GetPtr());
		// Push the instance.
		lua_pushvalue(pVm, -4);
		// Commit.
		lua_settable(pVm, -3);
		// Remove the weak registry, leaving only the instance table.
		lua_pop(pVm, 1);
	}

	// Just before invoking the constructor, now
	// that the table is configured, dispatch
	// any deferred events.
	if (nullptr != pDeferredDispatch)
	{
		// Now dispatch the deferred events.
		pDeferredDispatch->SetInterface(this);
		(void)pDeferredDispatch->DispatchEvents();
		pDeferredDispatch->SetInterface(nullptr);
	}

	// Check for constructor.
	lua_getfield(pVm, -1, "Constructor");
	if (lua_isnil(pVm, -1))
	{
		lua_pop(pVm, 2); // class table and nil value.
	}
	// Otherwise, invoke constructor.
	else
	{
		// Remove the class table, don't need it anymore.
		lua_remove(pVm, -2);

		// Invoke constructor - instance, then any arguments.
		lua_pushvalue(pVm, -2);

		// Also any arguments to the constructor, if specified.
		Int iTotalArgs = 0;
		if (iFirstArg >= 1 && iLastArg >= iFirstArg)
		{
			iTotalArgs = (iLastArg - iFirstArg + 1);
			for (Int i = iFirstArg; i <= iLastArg; ++i)
			{
				lua_pushvalue(pVm, i);
			}
		}

		// Invoke constructor - instance argument plus any additional.
		lua_call(pVm, 1 + iTotalArgs, 0);
	}
}

void ScriptUIMovie::ResolveRootMovieClipBinding()
{
	m_pRootMovieClipBinding.Reset();

	// Early out if we have no tree defined to bind a script
	// hierarchy to.
	if (!m_pInternal.IsValid() || !m_pInternal->GetRoot().IsValid())
	{
		return;
	}

	// TODO: This is very close to the body of NewNativeMovieClip(),
	// with the biggest difference the lack of a call to Advance(). I'm still not
	// sure if that's the "right way" - that the root should *not* be advanced
	// prior to script hookup but children *must* be advanced.

	HString const sClassName(m_pInternal->GetRoot()->GetMovieClipDefinition()->GetClassName());
	// If there was a class name passed in from the JSON file, use that. If not see if there is a class
	// name from the ActionScript and use that, otherwise use the base movie clip.
	HString const className(m_sTypeName.IsEmpty() ? (sClassName.IsEmpty() ? kDefaultMovieClipClassName : sClassName) : m_sTypeName);

	m_bConstructedScript = true;

	// Invoke BindAndConstructRoot - this just calls
	// back into the native function defined in this file.
	//
	// We use a script invocation to properly handle script
	// runtime errors.
	{
		Script::FunctionInvoker invoker(m_pExternalInterfaceBinding, kBindAndConstructRoot);
		if (!invoker.IsValid())
		{
			SEOUL_WARN("%s: failed invoke gather '%s' of UIMovie '%s'.",
				__FUNCTION__,
				className.CStr(),
				GetMovieTypeName().CStr());
			m_pRootMovieClipBinding.Reset();
			return;
		}

		invoker.PushString(className);
		if (!invoker.TryInvoke())
		{
			SEOUL_WARN("%s: invocation failed while instantiating root MovieClip '%s' for UIMovie '%s'.",
				__FUNCTION__,
				className.CStr(),
				GetMovieTypeName().CStr());
			m_pRootMovieClipBinding.Reset();
			return;
		}
	}

	// Final check, otherwise - success.
	if (!m_pRootMovieClipBinding.IsValid())
	{
		SEOUL_WARN("%s: invocation succeeded but root movie clip is null for root MovieClip '%s' of UIMovie '%s'.",
			__FUNCTION__,
			className.CStr(),
			GetMovieTypeName().CStr());
		return;
	}
}

void ScriptUIMovie::GetMousePositionFromWorld(Script::FunctionInterface* pInterface)
{
	Float fX = 0.0f;
	Float fY = 0.0f;
	if (!pInterface->GetNumber(1, fX))
	{
		pInterface->RaiseError(1, "expected number.");
		return;
	}
	if (!pInterface->GetNumber(2, fY))
	{
		pInterface->RaiseError(2, "expected number.");
		return;
	}

	auto const v = UI::Movie::GetMousePositionFromWorld(Vector2D(fX, fY));
	pInterface->PushReturnNumber(v.X);
	pInterface->PushReturnNumber(v.Y);
}

void ScriptUIMovie::OnResumeMovie()
{
	// Sanity - we should never be resumed if we send we could not be suspend-resumed.
	SEOUL_ASSERT(m_bCanSuspend);

	UI::Movie::OnResumeMovie();

	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kOnResumeMovie);
	(void)invoker.TryInvoke();
}

void ScriptUIMovie::OnSuspendMovie()
{
	// Sanity - we should never be resumed if we send we could not be suspend-resumed.
	SEOUL_ASSERT(m_bCanSuspend);

	UI::Movie::OnSuspendMovie();

	// Invoke - assumed valid since we checked at startup.
	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kOnSuspendMovie);
	(void)invoker.TryInvoke();
}

void ScriptUIMovie::OnAdvanceWhenBlocked(Float fDeltaTimeInSeconds)
{
	UI::Movie::OnAdvanceWhenBlocked(fDeltaTimeInSeconds);

	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kTickIgnoringPause);
	if (invoker.IsValid())
	{
		invoker.PushNumber(fDeltaTimeInSeconds);
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::OnAdvance(Float fDeltaTimeInSeconds)
{
	UI::Movie::OnAdvance(fDeltaTimeInSeconds);

	// GC of dangling dispatches (occurs generally and
	// if an error occurred during dispatch).
	SafeDeleteVector(m_vDispatchGarbage);

	// Give the script code time to run a task, if defined.
	// We don't run tasks until we've advanced at least once,
	// to reduce the amount of work that is done in frame 0
	// of a movie construction.
	if (m_bAdvancedOnce)
	{
		Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kYieldToTasks);
		if (invoker.IsValid())
		{
			invoker.PushNumber(fDeltaTimeInSeconds);
			(void)invoker.TryInvoke();
		}
	}

	// Prune bindings.
	{
		// Enumerate and track any that we have the only reference to.
		{
			auto const iBegin = m_Bindings.Begin();
			auto const iEnd = m_Bindings.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				if (i->IsUnique())
				{
					m_vBindings.PushBack(*i);
				}
			}
		}

		// Erase the binding.
		for (auto i = m_vBindings.Begin(); m_vBindings.End() != i; ++i)
		{
			SEOUL_VERIFY(m_Bindings.Erase(*i));
		}

		// Clear all bindings we tracked for erase.
		m_vBindings.Clear();
	}

	// Now have advanced at least once.
	m_bAdvancedOnce = true;
}

void ScriptUIMovie::FalconOnAddToParent(
	Falcon::MovieClipInstance* pParent,
	Falcon::Instance* pInstance,
	const HString& sClassName)
{
	// We allow missing global types for OnAddToParent. Just filter
	// in these cases.
	if (!m_pVm->HasGlobal(sClassName)) { return; }

	Script::FunctionInvoker invoker(m_pExternalInterfaceBinding, kOnAddToParent);
	if (invoker.IsValid())
	{
		invoker.PushLightUserData(pInstance);
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::FalconDispatchEnterFrameEvent(
	Falcon::Instance* pInstance)
{
	FalconDispatchEvent(
		FalconConstants::kEnterFrame,
		Falcon::SimpleActions::EventType::kEventDispatch,
		pInstance);
}

void ScriptUIMovie::FalconDispatchEvent(
	const HString& sEventName,
	Falcon::SimpleActions::EventType eType,
	Falcon::Instance* pInInstance)
{
	if (FalconDispatchGotoEvent(pInInstance, sEventName))
	{
		return;
	}

#if SEOUL_LOGGING_ENABLED
	Bool bDispatched = true;
#endif

	auto const bBubble = (Falcon::SimpleActions::EventType::kEventDispatchBubble == eType);
	if (bBubble)
	{
		auto pInstance = pInInstance;
		while (nullptr != pInstance)
		{
			Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
			if (!invoker.IsValid())
			{
				break;
			}

			if (!invoker.PushBinderFromWeakRegistry(pInstance))
			{
				pInstance = pInstance->GetParent();
				continue;
			}

			invoker.PushString(sEventName);
			if (invoker.TryInvoke())
			{
				Bool bReturn = false;
				if (invoker.GetReturnCount() >= 2 &&
					invoker.GetBoolean(1, bReturn) &&
					bReturn)
				{
					break;
				}
			}
			else
			{
#if SEOUL_LOGGING_ENABLED
				bDispatched = false;
#endif
				break;
			}

			pInstance = pInstance->GetParent();
		}
	}
	else
	{
		Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
		if (invoker.IsValid())
		{
			if (invoker.PushBinderFromWeakRegistry(pInInstance))
			{
				invoker.PushString(sEventName);

#if SEOUL_LOGGING_ENABLED
				bDispatched = invoker.TryInvoke();
#else
				invoker.TryInvoke();
#endif
			}
		}
	}

#if SEOUL_LOGGING_ENABLED
	if (!bDispatched)
	{
		SEOUL_WARN("%s: attempt to dispatch event '%s' to path '%s', but dispatch failed.",
			GetMovieTypeName().CStr(),
			sEventName.CStr(),
			GetPath(pInInstance).CStr());
	}
#endif
}

UI::MovieHitTestResult ScriptUIMovie::OnSendInputEvent(UI::InputEvent eInputEvent)
{
	if (!AcceptingInput())
	{
		return UI::MovieHitTestResult::kNoHitStopTesting;
	}

	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kOnInputEvent);
	if (invoker.IsValid())
	{
		invoker.PushEnumAsNumber(eInputEvent);
		Bool bHandled = false;
		if (invoker.TryInvoke() &&
			invoker.GetBoolean(0, bHandled) &&
			bHandled)
		{
			return UI::MovieHitTestResult::kHit;
		}
	}

	return UI::Movie::OnSendInputEvent(eInputEvent);
}

UI::MovieHitTestResult ScriptUIMovie::OnSendButtonEvent(
	Seoul::InputButton eButtonID,
	Seoul::ButtonEventType eButtonEventType)
{
	if (!AcceptingInput())
	{
		return UI::MovieHitTestResult::kNoHitStopTesting;
	}

	if (eButtonEventType == ButtonEventType::ButtonPressed)
	{
		Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kOnButtonDownEvent);
		if (invoker.IsValid())
		{
			invoker.PushEnumAsNumber(eButtonID);
			Bool bHandled = false;
			if (invoker.TryInvoke() &&
				invoker.GetBoolean(0, bHandled) &&
				bHandled)
			{
				return UI::MovieHitTestResult::kHit;
			}
		}
	}

	if (eButtonEventType == ButtonEventType::ButtonReleased)
	{
		Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kOnButtonReleasedEvent);
		if (invoker.IsValid())
		{
			invoker.PushEnumAsNumber(eButtonID);
			Bool bHandled = false;
			if (invoker.TryInvoke() &&
				invoker.GetBoolean(0, bHandled) &&
				bHandled)
			{
				return UI::MovieHitTestResult::kHit;
			}
		}
	}

	return UI::Movie::OnSendButtonEvent(eButtonID, eButtonEventType);
}

void ScriptUIMovie::OnEnterState(CheckedPtr<UI::State const> pPreviousState, CheckedPtr<UI::State const> pNextState, Bool bWasInPreviousState)
{
	UI::Movie::OnEnterState(pPreviousState, pNextState, bWasInPreviousState);

	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kOnEnterState);
	if (invoker.IsValid())
	{
		invoker.PushString((pPreviousState.IsValid() ? pPreviousState->GetStateIdentifier() : HString()));
		invoker.PushString((pNextState.IsValid() ? pNextState->GetStateIdentifier() : HString()));
		invoker.PushBoolean(bWasInPreviousState);
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::OnExitState(CheckedPtr<UI::State const> pPreviousState, CheckedPtr<UI::State const> pNextState, Bool bIsInNextState)
{
	UI::Movie::OnExitState(pPreviousState, pNextState, bIsInNextState);

	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kOnExitState);
	if (invoker.IsValid())
	{
		invoker.PushString((pPreviousState.IsValid() ? pPreviousState->GetStateIdentifier() : HString()));
		invoker.PushString((pNextState.IsValid() ? pNextState->GetStateIdentifier() : HString()));
		invoker.PushBoolean(bIsInNextState);
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::OnLoad()
{
	UI::Movie::OnLoad();

	// Give the script a chance to do cleanup.
	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kOnLoad);
	if (invoker.IsValid())
	{
		(void)invoker.TryInvoke();
	}
}

Bool ScriptUIMovie::AllowClickPassthroughToProceed(
	const Point2DInt& mousePosition,
	const SharedPtr<Falcon::MovieClipInstance>& pInstance) const
{
	// Convert the mouse position (in screen pixels) into
	// stage pixels (screen pixels, if the Flash-to-screen ratio was 1:1, which
	// in most cases it is not).
	const Vector2D vMousePositionInWorld = GetMousePositionInWorld(mousePosition);

	Bool bReturn = true;

	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
	if (invoker.IsValid())
	{
		if (!invoker.PushBinderFromWeakRegistry(pInstance.GetPtr()))
		{
			return false;
		}

		invoker.PushString(kAllowClickPassthroughToProceed);
		invoker.PushNumber(vMousePositionInWorld.X);
		invoker.PushNumber(vMousePositionInWorld.Y);
		if (invoker.TryInvoke())
		{
			(void)invoker.GetBoolean(0, bReturn);
		}

	}

	return bReturn;
}

void ScriptUIMovie::OnGlobalMouseButtonPressed(
	const Point2DInt& mousePosition,
	const SharedPtr<Falcon::MovieClipInstance>& pInstance)
{
	UI::Movie::OnGlobalMouseButtonPressed(mousePosition, pInstance);

	// Convert the mouse position (in screen pixels) into
	// stage pixels (screen pixels, if the Flash-to-screen ratio was 1:1, which
	// in most cases it is not).
	auto const vMousePositionInWorld = GetMousePositionInWorld(mousePosition);

	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
	if (invoker.IsValid())
	{
		invoker.PushObject(m_pRootMovieClipBinding);
		invoker.PushString(kMovieMouseDown);
		if (!invoker.PushBinderFromWeakRegistry(pInstance.GetPtr()))
		{
			// TODO: Definitely not what's expected in this case, but
			// actually lazily created the script instance at this moment is tricky
			// because Lua can longjmp if something fails (need to make sure
			// no C++ complex objects are on the stack when calling the create instance
			// path).

			// Don't fail in this case, push nil instead, since the global case can
			// result in a hit on an instance that has otherwise never been accessed
			// in script.
			invoker.PushNil();
		}
		invoker.PushNumber(vMousePositionInWorld.X);
		invoker.PushNumber(vMousePositionInWorld.Y);
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::OnGlobalMouseButtonReleased(const Point2DInt& mousePosition)
{
	UI::Movie::OnGlobalMouseButtonReleased(mousePosition);

	// Convert the mouse position (in screen pixels) into
	// stage pixels (screen pixels, if the Flash-to-screen ratio was 1:1, which
	// in most cases it is not).
	auto const vMousePositionInWorld = GetMousePositionInWorld(mousePosition);

	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
	if (invoker.IsValid())
	{
		invoker.PushObject(m_pRootMovieClipBinding);
		invoker.PushString(kMovieMouseUp);
		invoker.PushNumber(vMousePositionInWorld.X);
		invoker.PushNumber(vMousePositionInWorld.Y);
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::OnMouseButtonPressed(
	const Point2DInt& mousePosition,
	const SharedPtr<Falcon::MovieClipInstance>& pInstance,
	Bool bInInstance)
{
	UI::Movie::OnMouseButtonPressed(mousePosition, pInstance, bInInstance);

	// Convert the mouse position (in screen pixels) into
	// stage pixels (screen pixels, if the Flash-to-screen ratio was 1:1, which
	// in most cases it is not).
	auto const vMousePositionInWorld = GetMousePositionInWorld(mousePosition);

	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
	if (invoker.IsValid())
	{
		if (!invoker.PushBinderFromWeakRegistry(pInstance.GetPtr()))
		{
			return;
		}

		invoker.PushString(kMouseDown);
		invoker.PushNumber(vMousePositionInWorld.X);
		invoker.PushNumber(vMousePositionInWorld.Y);
		invoker.PushBoolean(bInInstance);
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::OnMouseButtonReleased(
	const Point2DInt& mousePosition,
	const SharedPtr<Falcon::MovieClipInstance>& pInstance,
	Bool bInInstance,
	UInt8 uInputCaptureHitTestMask)
{
	UI::Movie::OnMouseButtonReleased(mousePosition, pInstance, bInInstance, uInputCaptureHitTestMask);

	// Convert the mouse position (in screen pixels) into
	// stage pixels (screen pixels, if the Flash-to-screen ratio was 1:1, which
	// in most cases it is not).
	auto const vMousePositionInWorld = GetMousePositionInWorld(mousePosition);

	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
	if (invoker.IsValid())
	{
		if (!invoker.PushBinderFromWeakRegistry(pInstance.GetPtr()))
		{
			return;
		}

		invoker.PushString(kMouseUp);
		invoker.PushNumber(vMousePositionInWorld.X);
		invoker.PushNumber(vMousePositionInWorld.Y);
		invoker.PushBoolean(bInInstance);
		invoker.PushInteger((Int32)uInputCaptureHitTestMask);
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::OnMouseMove(
	const Point2DInt& mousePosition,
	const SharedPtr<Falcon::MovieClipInstance>& pInstance,
	Bool bInInstance)
{
	UI::Movie::OnMouseMove(mousePosition, pInstance, bInInstance);

	// Convert the mouse position (in screen pixels) into
	// stage pixels (screen pixels, if the Flash-to-screen ratio was 1:1, which
	// in most cases it is not).
	auto const vMousePositionInWorld = GetMousePositionInWorld(mousePosition);

	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
	if (invoker.IsValid())
	{
		if (!invoker.PushBinderFromWeakRegistry(pInstance.GetPtr()))
		{
			return;
		}

		invoker.PushString(kMouseMove);
		invoker.PushNumber(vMousePositionInWorld.X);
		invoker.PushNumber(vMousePositionInWorld.Y);
		invoker.PushBoolean(bInInstance);
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::OnMouseWheel(
	const Point2DInt& mousePosition,
	const SharedPtr<Falcon::MovieClipInstance>& pInstance,
	Float fDelta)
{
	UI::Movie::OnMouseWheel(mousePosition, pInstance, fDelta);

	// Convert the mouse position (in screen pixels) into
	// stage pixels (screen pixels, if the Flash-to-screen ratio was 1:1, which
	// in most cases it is not).
	auto const vMousePositionInWorld = GetMousePositionInWorld(mousePosition);

	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
	if (invoker.IsValid())
	{
		if (!invoker.PushBinderFromWeakRegistry(pInstance.GetPtr()))
		{
			return;
		}

		invoker.PushString(kMouseWheel);
		invoker.PushNumber(vMousePositionInWorld.X);
		invoker.PushNumber(vMousePositionInWorld.Y);
		invoker.PushNumber(fDelta);
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::OnMouseOut(
	const Point2DInt& mousePosition,
	const SharedPtr<Falcon::MovieClipInstance>& pInstance)
{
	UI::Movie::OnMouseOut(mousePosition, pInstance);

	// Convert the mouse position (in screen pixels) into
	// stage pixels (screen pixels, if the Flash-to-screen ratio was 1:1, which
	// in most cases it is not).
	auto const vMousePositionInWorld = GetMousePositionInWorld(mousePosition);

	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
	if (invoker.IsValid())
	{
		if (!invoker.PushBinderFromWeakRegistry(pInstance.GetPtr()))
		{
			return;
		}

		invoker.PushString(kMouseOut);
		invoker.PushNumber(vMousePositionInWorld.X);
		invoker.PushNumber(vMousePositionInWorld.Y);
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::OnMouseOver(
	const Point2DInt& mousePosition,
	const SharedPtr<Falcon::MovieClipInstance>& pInstance)
{
	UI::Movie::OnMouseOver(mousePosition, pInstance);

	// Convert the mouse position (in screen pixels) into
	// stage pixels (screen pixels, if the Flash-to-screen ratio was 1:1, which
	// in most cases it is not).
	auto const vMousePositionInWorld = GetMousePositionInWorld(mousePosition);

	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
	if (invoker.IsValid())
	{
		if (!invoker.PushBinderFromWeakRegistry(pInstance.GetPtr()))
		{
			return;
		}

		invoker.PushString(kMouseOver);
		invoker.PushNumber(vMousePositionInWorld.X);
		invoker.PushNumber(vMousePositionInWorld.Y);
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::OnEditTextStartEditing(const SharedPtr<Falcon::MovieClipInstance>& pInstance)
{
	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
	if (invoker.IsValid())
	{
		if (!invoker.PushBinderFromWeakRegistry(pInstance.GetPtr()))
		{
			return;
		}

		invoker.PushString(kStartEditing);
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::OnEditTextStopEditing(const SharedPtr<Falcon::MovieClipInstance>& pInstance)
{
	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
	if (invoker.IsValid())
	{
		if (!invoker.PushBinderFromWeakRegistry(pInstance.GetPtr()))
		{
			return;
		}

		invoker.PushString(kStopEditing);
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::OnEditTextApply(const SharedPtr<Falcon::MovieClipInstance>& pInstance)
{
	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
	if (invoker.IsValid())
	{
		if (!invoker.PushBinderFromWeakRegistry(pInstance.GetPtr()))
		{
			return;
		}

		invoker.PushString(kApplyEditing);
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::OnAddToParent(Script::FunctionInterface* pInterface)
{
	void* pUserData = nullptr;
	if (!pInterface->GetLightUserData(1, pUserData))
	{
		pInterface->RaiseError(1, "expected light user data.");
		return;
	}

	SharedPtr<Falcon::Instance> p((Falcon::Instance*)pUserData);
	TransferOwnershipToScript(pInterface, p);
}

void ScriptUIMovie::ReturnMousePositionInWorld(Script::FunctionInterface* pInterface) const
{
	auto const vMousePosition = GetMousePositionInWorld(UI::Manager::Get()->GetMousePosition());

	pInterface->PushReturnNumber(vMousePosition.X);
	pInterface->PushReturnNumber(vMousePosition.Y);
}

void ScriptUIMovie::GetRootMovieClip(Script::FunctionInterface* pInterface) const
{
	if (!m_pRootMovieClipBinding.IsValid())
	{
		pInterface->PushReturnNil();
	}
	else
	{
		pInterface->PushReturnObject(m_pRootMovieClipBinding);
	}
}

Int32 ScriptUIMovie::GetLastViewportWidth() const
{
	return GetLastViewport().m_iViewportWidth;
}

Int32 ScriptUIMovie::GetLastViewportHeight() const
{
	return GetLastViewport().m_iViewportHeight;
}

void ScriptUIMovie::OnLinkClicked(
	const String& sLinkInfo,
	const String& sLinkType,
	const SharedPtr<Falcon::MovieClipInstance>& pInstance)
{
	UI::Movie::OnLinkClicked(sLinkInfo, sLinkType, pInstance);

	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
	if (invoker.IsValid())
	{
		if (!invoker.PushBinderFromWeakRegistry(pInstance.GetPtr()))
		{
			return;
		}

		invoker.PushString(kLinkClicked);
		invoker.PushString(sLinkInfo);
		invoker.PushString(sLinkType);
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::InvokePassthroughInputFunction()
{
	UI::Movie::InvokePassthroughInputFunction();

	if(!PassthroughInputFunction().IsEmpty())
	{
		Script::FunctionInvoker invoker(m_pRootMovieClipBinding, PassthroughInputFunction());
		if (invoker.IsValid())
		{
			(void)invoker.TryInvoke();
		}
	}
}

#if SEOUL_HOT_LOADING
void ScriptUIMovie::OnHotLoadBegin()
{
	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kHotLoadBegin);
	if (invoker.IsValid())
	{
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::OnHotLoadEnd()
{
	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kHotLoadEnd);
	if (invoker.IsValid())
	{
		(void)invoker.TryInvoke();
	}
}
#endif

void ScriptUIMovie::OnDispatchTickEvent(Falcon::Instance* pInstance) const
{
	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
	if (invoker.IsValid())
	{
		if (!invoker.PushBinderFromWeakRegistry(pInstance))
		{
			return;
		}

		invoker.PushString(FalconConstants::kTickEvent);
		invoker.PushNumber(Engine::Get()->GetSecondsInTick());
		(void)invoker.TryInvoke();
	}
}

void ScriptUIMovie::OnDispatchTickScaledEvent(Falcon::Instance* pInstance) const
{
	Script::FunctionInvoker invoker(m_pRootMovieClipBinding, kDispatchEvent);
	if (invoker.IsValid())
	{
		if (!invoker.PushBinderFromWeakRegistry(pInstance))
		{
			return;
		}

		invoker.PushString(FalconConstants::kTickScaledEvent);
		invoker.PushNumber(Engine::Get()->GetSecondsInTick() * Engine::Get()->GetSecondsInTickScale());
		(void)invoker.TryInvoke();
	}
}

Bool ScriptUIMovie::OnTryBroadcastEvent(
	HString eventName,
	const Reflection::MethodArguments& aMethodArguments,
	Int iArgumentCount)
{
	if (UI::Movie::OnTryBroadcastEvent(eventName, aMethodArguments, iArgumentCount))
	{
		return true;
	}
	else
	{
		Script::FunctionInvoker invoker(m_pRootMovieClipBinding, eventName);
		if (!invoker.IsValid())
		{
			return false;
		}

		for (Int i = 0; i < iArgumentCount; ++i)
		{
			invoker.PushAny(aMethodArguments[i]);
		}

		return invoker.TryInvoke();
	}

	return false;
}

Bool ScriptUIMovie::OnTryInvoke(
	HString methodName,
	const Reflection::MethodArguments& aMethodArguments,
	Int iArgumentCount,
	Bool bNativeCall)
{
	SEOUL_ASSERT_MESSAGE(bNativeCall || m_bConstructedScript, String::Printf("Trying to invoke the method %s before the corresponding movie, %s, has been bound to script.", methodName.CStr(), GetMovieTypeName().CStr()).CStr());

	if (bNativeCall)
	{
		Reflection::Method const* pMethod = GetReflectionThis().GetType().GetMethod(methodName);
		if (nullptr != pMethod)
		{
			return (Bool)pMethod->TryInvoke(GetReflectionThis(), aMethodArguments);
		}
	}
	else
	{
		Script::FunctionInvoker invoker(m_pRootMovieClipBinding, methodName);
		if (!invoker.IsValid())
		{
			SEOUL_WARN("Attempting to invoke script method %s::%s but method does not exist. If the method exists, check that it is not private.\n",
				GetMovieTypeName().CStr(),
				methodName.CStr());
			return false;
		}

		for (Int i = 0; i < iArgumentCount; ++i)
		{
			invoker.PushAny(aMethodArguments[i]);
		}

		return invoker.TryInvoke();
	}

	return false;
}

} // namespace Seoul
