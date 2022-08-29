/**
 * \file ScriptUIMovieClipInstance.cpp
 * \brief Script binding around Falcon::MovieClipInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FxManager.h"
#include "Logger.h"
#include "FalconBitmapDefinition.h"
#include "FalconBitmapInstance.h"
#include "FalconEditTextInstance.h"
#include "FalconMovieClipInstance.h"
#include "ReflectionDefine.h"
#include "ScriptFunctionInterface.h"
#include "ScriptUIAnimation2DEvent.h"
#include "ScriptUIEditTextInstance.h"
#include "ScriptUIMovie.h"
#include "ScriptUIMovieClipInstance.h"
#include "ScriptVm.h"
#include "SeoulMath.h"
#include "UIAnimation2DNetworkInstance.h"
#include "UIFacebookTextureInstance.h"
#include "UIFxInstance.h"
#include "UIHitShapeInstance.h"
#include "UIManager.h"
#include "UIRenderer.h"
#include "UIStage3D.h"

#if SEOUL_WITH_ANIMATION_2D
#include "Animation2DManager.h"
#include "Animation2DNetworkInstance.h"
#endif

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptUIMovieClipInstance, TypeFlags::kDisableCopy)
	SEOUL_PARENT(ScriptUIInstance)
	SEOUL_METHOD(AddChild)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "ScriptUIInstance oInstance, string sName = null, double iDepth = -1")
#if SEOUL_WITH_ANIMATION_2D
	SEOUL_METHOD(AddChildAnimationNetwork)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "Animation2DNetwork", "FilePath networkFilePath, FilePath dataFilePath, object callback = null")
#endif // /#if SEOUL_WITH_ANIMATION_2D
	SEOUL_METHOD(AddChildBitmap)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "DisplayObject", "FilePath filePath, double iWidth, double iHeight, string sName = null, bool? bCenter = null, double iDepth = -1, bool bPrefetch = false")
	SEOUL_METHOD(AddChildFacebookBitmap)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "DisplayObject", "string sFacebookUserGuid, double iWidth, double iHeight, FilePath defaultImageFilePath")
	SEOUL_METHOD(AddChildFx)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "FxDisplayObject", "object fxNameOrFilePath, double iWidth, double iHeight, FxFlags? iFxFlags = null, Native.ScriptUIInstance udChildNativeInstance = null, string sVariationName = null")
	SEOUL_METHOD(AddChildHitShape)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "DisplayObject", "double fLeft, double fTop, double fRight, double fBottom, string sName = null, double iDepth = -1")
	SEOUL_METHOD(AddChildHitShapeFullScreen)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "DisplayObject", "string sName = null")
	SEOUL_METHOD(AddChildHitShapeWithMyBounds)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "DisplayObject", "string sName = null")
	SEOUL_METHOD(AddChildStage3D)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "DisplayObject", "FilePath filePath, double iWidth, double iHeight, string sName = null, bool? bCenter = null")
	SEOUL_METHOD(AddFullScreenClipper)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void")
	SEOUL_METHOD(GetAbsorbOtherInput)
	SEOUL_METHOD(GetChildCount)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "int")
	SEOUL_METHOD(GetCurrentFrame)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "int")
	SEOUL_METHOD(GetCurrentLabel)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string")
	SEOUL_METHOD(GetDepth3D)
	SEOUL_METHOD(GetExactHitTest)
	SEOUL_METHOD(GetHitTestChildren)
	SEOUL_METHOD(GetHitTestChildrenMode)
	SEOUL_METHOD(GetHitTestSelf)
	SEOUL_METHOD(GetHitTestSelfMode)
	SEOUL_METHOD(GetChildAt)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "DisplayObject", "double iIndex")
	SEOUL_METHOD(GetChildByNameFromSubTree)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "DisplayObject", "string sName")
	SEOUL_METHOD(GetChildByPath)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "DisplayObject", "params string[] asParts")
	SEOUL_METHOD(GetTotalFrames)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "int")
	SEOUL_METHOD(GetHitTestableBounds)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double, double, double)", "double iMask")
	SEOUL_METHOD(GetHitTestableLocalBounds)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double, double, double)", "double iMask")
	SEOUL_METHOD(GetHitTestableWorldBounds)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double, double, double)", "double iMask")
	SEOUL_METHOD(GotoAndPlay)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "double iFrame")
	SEOUL_METHOD(GotoAndPlayByLabel)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "string sLabel")
	SEOUL_METHOD(GotoAndStop)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "double iFrame")
	SEOUL_METHOD(GotoAndStopByLabel)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "string sLabel")
	SEOUL_METHOD(IsPlaying)
	SEOUL_METHOD(HitTest)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "DisplayObject", "double iMask, double? fX = null, double? fY = null")
	SEOUL_METHOD(IncreaseAllChildDepthByOne)
	SEOUL_METHOD(Play)
	SEOUL_METHOD(RemoveAllChildren)
	SEOUL_METHOD(RemoveChildAt)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "double iIndex")
	SEOUL_METHOD(RemoveChildByName)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "string sName")
	SEOUL_METHOD(RestoreTypicalDefault)
	SEOUL_METHOD(SetAbsorbOtherInput)
	SEOUL_METHOD(SetAutoCulling)
	SEOUL_METHOD(SetAutoDepth3D)
	SEOUL_METHOD(SetCastPlanarShadows)
	SEOUL_METHOD(SetDeferDrawing)
	SEOUL_METHOD(SetDepth3D)
	SEOUL_METHOD(SetDepth3DFromYLoc)
	SEOUL_METHOD(SetEnterFrame)
	SEOUL_METHOD(SetInputActionDisabled)
	SEOUL_METHOD(SetTickEvent)
	SEOUL_METHOD(SetTickScaledEvent)
	SEOUL_METHOD(SetExactHitTest)
	SEOUL_METHOD(SetHitTestChildren)
	SEOUL_METHOD(SetHitTestChildrenMode)
	SEOUL_METHOD(SetHitTestSelf)
	SEOUL_METHOD(SetHitTestSelfMode)
	SEOUL_METHOD(SetReorderChildrenFromDepth3D)
	SEOUL_METHOD(Stop)
	SEOUL_METHOD(SetChildrenVisible)
SEOUL_END_TYPE()

static const HString kFullScreenClipper("FullScreenClipper");

/**
 * We use this to cache the one-based index of the ScriptUIMovieClipInstance
 * type in the Reflection registry. Makes PreCollectionHook as fast
 * as possible.
 *
 * This is set and updated by ResolveLuaJITPreCollectionHook(). The index
 * never changes once static initialization of the application has completed.
 */
static UInt32 s_uOneBasedScriptUIMovieClipInstanceIndex = 0u;

/** Low-level and ugly - built for speed, not safety or elegance. */
namespace
{

Int PreCollection(void* p, UInt32 uUserData)
{
	// Only a ScriptUIMovieClipInstance if uUserData == s_uOneBasedScriptUIMovieClipInstanceIndex
	if (s_uOneBasedScriptUIMovieClipInstanceIndex != uUserData) { return 1; }

	// Garbage collect only if the instance is the unique owner
	// of its Falcon::Instance pointer.
	auto pInstance = (ScriptUIMovieClipInstance*)p;
	return (pInstance->IsUniqueOwner() ? 1 : 0);
}

} // namespace anonymous

ScriptUIMovieClipInstance::PreCollectionHook ScriptUIMovieClipInstance::ResolveLuaJITPreCollectionHook()
{
	// Cache the index if not set already.
	if (0u == s_uOneBasedScriptUIMovieClipInstanceIndex)
	{
		s_uOneBasedScriptUIMovieClipInstanceIndex = TypeOf<ScriptUIMovieClipInstance>().GetRegistryIndex() + 1u;
	}

	// Return the global handler.
	return PreCollection;
}

ScriptUIMovieClipInstance::ScriptUIMovieClipInstance()
	: m_uDynamicDepth(0)
	, m_bEnableTickEvents(false)
	, m_bEnableTickScaledEvents(false)
{
}

void ScriptUIMovieClipInstance::Construct(
	const SharedPtr<Falcon::Instance>& pInstance,
	const ScriptUIMovie& owner)
{
	// Instance must have no existing watchers.
	SEOUL_ASSERT(0 == pInstance->GetWatcherCount());

	SEOUL_ASSERT(!pInstance.IsValid() || Falcon::InstanceType::kMovieClip == pInstance->GetType());
	ScriptUIInstance::Construct(pInstance, owner);

	SharedPtr<Falcon::MovieClipInstance> pt((Falcon::MovieClipInstance*)pInstance.GetPtr());
	m_uDynamicDepth = (pt->GetMovieClipDefinition()->GetMaxDepth() + 1);
}

ScriptUIMovieClipInstance::~ScriptUIMovieClipInstance()
{
	// Instance must have exactly 1 watcher.
	SEOUL_ASSERT(1 == m_pInstance->GetWatcherCount());

	// Memory management around ScriptUI* instances (which are binding objects
	// that connect script to native) requires that the objects are the sole owners
	// of their native instance (m_pInstance) at the time of their destruction. If
	// this is violated, we can have subtle bugs in script, where object state is
	// reset at unexpected times.
#if !SEOUL_ASSERTIONS_DISABLED
	// Don't apply this assert when in the destructor of the VM, we expect
	// to be released "prematurely" in that case.
	if (!Script::Vm::DebugIsInVmDestroy())
	{
		// Small number of possibilities:
		// - unique.
		// - 2 references if either ticking.
		// - 3 references if both ticking.
		auto const uExpectedRefCount = 1u +
			(m_bEnableTickEvents ? 1u : 0u) +
			(m_bEnableTickScaledEvents ? 1u : 0u);
		auto const bUnique = IsUniqueOwner() ||
			(uExpectedRefCount == m_pInstance.GetReferenceCount());

		SEOUL_ASSERT(bUnique);
	}
#endif // /#if !SEOUL_ASSERTIONS_DISABLED

	if (m_bEnableTickEvents)
	{
		CheckedPtr<ScriptUIMovie> pOwner(GetPtr<ScriptUIMovie>(m_hOwner));
		if (pOwner.IsValid())
		{
			pOwner->DisableTickEvents(m_pInstance.GetPtr());
		}

		m_bEnableTickEvents = false;
	}

	if (m_bEnableTickScaledEvents)
	{
		CheckedPtr<ScriptUIMovie> pOwner(GetPtr<ScriptUIMovie>(m_hOwner));
		if (pOwner.IsValid())
		{
			pOwner->DisableTickScaledEvents(m_pInstance.GetPtr());
		}

		m_bEnableTickScaledEvents = false;
	}
}

Bool ScriptUIMovieClipInstance::IsUniqueOwner() const
{
	// This is a quick sanity case - if the owner has been
	// destroyed, all script nodes should always be destroyed,
	// so we always consider ourselves the unique owner.
	if (!GetOwner())
	{
		return true;
	}

	// This is not exhaustive (there are cases where a unique
	// owner is effectively true if the parent hierarchy is
	// only owned by script nodes) but it is sufficient.
	return m_pInstance.IsUnique();
}

void ScriptUIMovieClipInstance::AddChild(Script::FunctionInterface* pInterface)
{
	if (ScriptUIInstance* pInstance = pInterface->GetUserData<ScriptUIInstance>(1))
	{
		auto pOwner(GetOwner());
		if (!pOwner)
		{
			pInterface->RaiseError(-1, "owner native UIMovie has already been destroyed.");
			return;
		}

		Int32 iInsertionDepth = 0;
		if (!pInterface->GetInteger(3, iInsertionDepth))
		{
			pInterface->RaiseError(3, "Child Insertion Index required. Set this to -1 if you want children added dynamically (to end of UI list).");
			return;
		}

		if (iInsertionDepth <= 0)
		{
			iInsertionDepth = m_uDynamicDepth;
		}
		m_uDynamicDepth = Max(m_uDynamicDepth, (UInt16) iInsertionDepth);

		auto pNativeInstance = pInstance->GetInstance();
		GetInstance()->SetChildAtDepth(
			*pOwner,
			iInsertionDepth,
			pNativeInstance);

		if (!pInterface->IsNilOrNone(2))
		{
			Byte const* sName = nullptr;
			UInt32 zNameLengthInBytes = 0;
			if (!pInterface->GetString(2, sName, zNameLengthInBytes))
			{
				pInterface->RaiseError(2, "name argument must be string.");
				return;
			}

			pNativeInstance->SetName(HString(sName, zNameLengthInBytes));
		}
	}
	else
	{
		pInterface->RaiseError(1, "invalid child, must be a native Falcon::Instance, Falcon::EditTextInstance, or Falcon::MovieClipInstance.");
		return;
	}

	++m_uDynamicDepth;
}

#if SEOUL_WITH_ANIMATION_2D
void ScriptUIMovieClipInstance::AddChildAnimationNetwork(Script::FunctionInterface* pInterface)
{
	auto pOwner(DynamicCast<ScriptUIMovie*>(GetOwner()));
	if (nullptr == pOwner)
	{
		pInterface->RaiseError(-1, "null ScriptUIMovie owner.");
		return;
	}

	SharedPtr<Falcon::Instance> pChildInstance;
	{
		FilePath networkFilePath;
		if (!pInterface->GetFilePath(1, networkFilePath))
		{
			pInterface->RaiseError(1, "expected FilePath to animation network.");
			return;
		}

		FilePath dataFilePath;
		if (!pInterface->GetFilePath(2, dataFilePath))
		{
			pInterface->RaiseError(2, "expected FilePath to animation data.");
			return;
		}

		SharedPtr<ScriptUIAnimation2DEvent> pEvent;
		{
			SharedPtr<Script::VmObject> pCallback;
			if (pInterface->IsFunction(3))
			{
				if (!pInterface->GetFunction(3, pCallback))
				{
					pInterface->RaiseError(3, "expected callback function.");
					return;
				}
			}

			if (pCallback.IsValid())
			{
				pOwner->TrackBinding(pCallback);
				pEvent.Reset(SEOUL_NEW(MemoryBudgets::Scripting) ScriptUIAnimation2DEvent(pCallback));
			}
		}

		SharedPtr<Animation2D::NetworkInstance> pNetworkInstance(Animation2D::Manager::Get()->CreateInstance(networkFilePath, dataFilePath, pEvent));

		pChildInstance.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) UI::Animation2DNetworkInstance(
			*pOwner,
			pNetworkInstance));

		GetInstance()->SetChildAtDepth(
			*pOwner,
			m_uDynamicDepth,
			pChildInstance);

		++m_uDynamicDepth;
	}

	// Can longjmp, so must be last with no complex members on the stack.
	pOwner->TransferOwnershipToScript(pInterface, pChildInstance);
}
#endif // /#if SEOUL_WITH_ANIMATION_2D

void ScriptUIMovieClipInstance::AddChildBitmap(Script::FunctionInterface* pInterface)
{
	auto pOwner(GetOwner());
	if (!pOwner)
	{
		pInterface->RaiseError(-1, "owner native UIMovie has already been destroyed.");
		return;
	}

	SharedPtr<Falcon::Instance> pChildInstance;
	{
		FilePath filePath;
		if (!pInterface->GetFilePath(1, filePath))
		{
			pInterface->RaiseError(1, "expected FilePath to texture substitution bitmap.");
			return;
		}

		Int32 iWidth = 0;
		if (!pInterface->GetInteger(2, iWidth))
		{
			pInterface->RaiseError(2, "width of the bitmap in pixels is required.");
			return;
		}
		if (iWidth < 0)
		{
			pInterface->RaiseError(2, "width of the bitmap cannot be negative.");
			return;
		}

		Int32 iHeight = 0;
		if (!pInterface->GetInteger(3, iHeight))
		{
			pInterface->RaiseError(3, "height of the bitmap in pixels is required.");
			return;
		}
		if (iHeight < 0)
		{
			pInterface->RaiseError(3, "height of the bitmap cannot be negative.");
			return;
		}

		Int32 iInsertionDepth = 0;
		if (!pInterface->GetInteger(6, iInsertionDepth))
		{
			pInterface->RaiseError(6, "Child Insertion Index required. Set this to -1 if you want children added dynamically (to end of UI list).");
			return;
		}

		Bool bPrefetch = false;
		(void)pInterface->GetBoolean(7, bPrefetch);

		if (iInsertionDepth <= 0)
		{
			iInsertionDepth = m_uDynamicDepth;
		}
		m_uDynamicDepth = Max(m_uDynamicDepth, (UInt16)iInsertionDepth);

		pChildInstance.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) Falcon::BitmapInstance(SharedPtr<Falcon::BitmapDefinition>(
			SEOUL_NEW(MemoryBudgets::UIRuntime) Falcon::BitmapDefinition(
				filePath,
				(UInt32)iWidth,
				(UInt32)iHeight,
				0u,
				bPrefetch))));

		GetInstance()->SetChildAtDepth(
			*pOwner,
			iInsertionDepth,
			pChildInstance);

		if (!pInterface->IsNilOrNone(4))
		{
			Byte const* sName = nullptr;
			UInt32 zNameLengthInBytes = 0;
			if (!pInterface->GetString(4, sName, zNameLengthInBytes))
			{
				pInterface->RaiseError(4, "optional name argument must be a string.");
				return;
			}
			pChildInstance->SetName(HString(sName, zNameLengthInBytes));
		}

		if (!pInterface->IsNilOrNone(5))
		{
			Bool bCenter = false;
			if (!pInterface->GetBoolean(5, bCenter))
			{
				pInterface->RaiseError(5, "optional bCenterToParent argument must be a boolean.");
				return;
			}

			if (bCenter)
			{
				Falcon::Rectangle bounds;
				if (pChildInstance->ComputeBounds(bounds))
				{
					Float32 const fWidth = bounds.GetWidth();
					Float32 const fHeight = bounds.GetHeight();

					Vector2D vPosition = pChildInstance->GetPosition();
					vPosition.X -= (fWidth / 2.0f);
					vPosition.Y -= (fHeight / 2.0f);
					pChildInstance->SetPosition(vPosition.X, vPosition.Y);
				}
			}
		}

		++m_uDynamicDepth;
	}

	// Can longjmp, so must be last with no complex members on the stack.
	pOwner->TransferOwnershipToScript(pInterface, pChildInstance);
}

void ScriptUIMovieClipInstance::AddChildFacebookBitmap(Script::FunctionInterface* pInterface)
{
	auto pOwner(GetOwner());
	if (!pOwner)
	{
		pInterface->RaiseError(-1, "owner native UIMovie has already been destroyed.");
		return;
	}

	SharedPtr<Falcon::Instance> pChildInstance;
	{
		FilePath* pDefaultImageFilePath = nullptr;
		String facebookUserGuid;

		if (!pInterface->GetString(1, facebookUserGuid))
		{
			pInterface->RaiseError(1, "must be a string identifier for facebook guid");
			return;
		}

		Int32 iWidth = 0;
		if (!pInterface->GetInteger(2, iWidth))
		{
			pInterface->RaiseError(2, "width of the bitmap in pixels is required.");
			return;
		}

		Int32 iHeight = 0;
		if (!pInterface->GetInteger(3, iHeight))
		{
			pInterface->RaiseError(3, "height of the bitmap in pixels is required.");
			return;
		}

		pDefaultImageFilePath = pInterface->GetUserData<FilePath>(4);

		if (!pDefaultImageFilePath)
		{
			pInterface->RaiseError(4, "must be a string identifier for the bitmap symbol.");
			return;
		}

		pChildInstance.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) UI::FacebookTextureInstance(
			facebookUserGuid,
			*pDefaultImageFilePath,
			iWidth,
			iHeight));

		GetInstance()->SetChildAtDepth(
			*pOwner,
			m_uDynamicDepth,
			pChildInstance);

		++m_uDynamicDepth;
	}

	// Can longjmp, so must be last with no complex members on the stack.
	pOwner->TransferOwnershipToScript(pInterface, pChildInstance);
}

void ScriptUIMovieClipInstance::AddChildFx(Script::FunctionInterface* pInterface)
{
	auto pOwner(GetOwner());
	if (!pOwner.IsValid())
	{
		pInterface->RaiseError(-1, "dangling owner reference, cannot instantiate effects.");
		return;
	}

	SharedPtr<Falcon::Instance> pChildInstance;
	{
		Int iArgumentIndex = 1;

		Byte const* sFxId = nullptr;
		UInt32 zFxIdSizeInBytes = 0;
		FilePath fxFilePath;
		if (!pInterface->GetString(iArgumentIndex, sFxId, zFxIdSizeInBytes))
		{
			if (!pInterface->GetFilePath(iArgumentIndex, fxFilePath))
			{
				pInterface->RaiseError(iArgumentIndex, "fx identifier is required.");
				return;
			}
		}
		++iArgumentIndex;

		Float fX = 0;
		if (!pInterface->GetNumber(iArgumentIndex, fX))
		{
			pInterface->RaiseError(iArgumentIndex, "x position of fx is required.");
			return;
		}
		++iArgumentIndex;

		Float fY = 0;
		if (!pInterface->GetNumber(iArgumentIndex, fY))
		{
			pInterface->RaiseError(iArgumentIndex, "y position of fx is required.");
			return;
		}
		++iArgumentIndex;

		Int32 iFlags = 0;
		if (pInterface->IsNumberExact(iArgumentIndex))
		{
			if (!pInterface->GetInteger(iArgumentIndex, iFlags))
			{
				pInterface->RaiseError(iArgumentIndex, "flags argument must be convertible to an integer.");
				return;
			}

			++iArgumentIndex;
		}

		SharedPtr<Falcon::Instance> pParentInstanceIfWorldspace;
		if (ScriptUIInstance* pParentIfWorldspace = pInterface->GetUserData<ScriptUIInstance>(iArgumentIndex))
		{
			pParentInstanceIfWorldspace = pParentIfWorldspace->GetInstance();
		}
		++iArgumentIndex;

		Byte const* sVariationId = nullptr;
		UInt32 zVariationIdSizeInBytes = 0;
		if (pInterface->IsStringExact(iArgumentIndex))
		{
			SEOUL_VERIFY(pInterface->GetString(iArgumentIndex, sVariationId, zVariationIdSizeInBytes));
			++iArgumentIndex;
		}

		Fx* pFx = nullptr;
		if (fxFilePath.IsValid())
		{
			(void)FxManager::Get()->GetFx(fxFilePath, pFx);
		}
		else
		{
			pFx = pOwner->GetContent().GetFx().CreateFx(
				FxKey(HString(sFxId, zFxIdSizeInBytes), HString(sVariationId, zVariationIdSizeInBytes)));
		}

		if (nullptr == pFx)
		{
			pInterface->PushReturnNil();
			return;
		}

		// Creating the instance tries to play the FX
		SharedPtr<UI::FxInstance> pFalconFxInstance(
			SEOUL_NEW(MemoryBudgets::UIRuntime) UI::FxInstance(
				*pOwner,
				pFx,
				(UInt32)iFlags,
				pParentInstanceIfWorldspace));

		GetInstance()->SetChildAtDepth(
			*pOwner,
			m_uDynamicDepth,
			pFalconFxInstance);

		pFalconFxInstance->Init(Vector2D(fX, fY));

		if (pInterface->IsStringCoercible(iArgumentIndex))
		{
			Byte const* sFxName = nullptr;
			UInt32 zFxNameSizeInBytes = 0;
			SEOUL_VERIFY(pInterface->GetString(iArgumentIndex, sFxName, zFxNameSizeInBytes));

			++iArgumentIndex;
			pFalconFxInstance->SetName(HString(sFxName, zFxNameSizeInBytes));
		}

		++m_uDynamicDepth;

		pChildInstance = pFalconFxInstance;
	}

	// Can longjmp, so must be last with no complex members on the stack.
	pOwner->TransferOwnershipToScript(pInterface, pChildInstance);
}

void ScriptUIMovieClipInstance::AddChildHitShape(Script::FunctionInterface* pInterface)
{
	auto pOwner(GetOwner());
	if (!pOwner)
	{
		pInterface->RaiseError(-1, "owner native UIMovie has already been destroyed.");
		return;
	}

	SharedPtr<Falcon::Instance> pChildInstance;
	{
		Falcon::Rectangle bounds = Falcon::Rectangle::Create(0, 0, 0, 0);
		if (!pInterface->GetNumber(1, bounds.m_fLeft))
		{
			pInterface->RaiseError(1, "left bounds as a number is required.");
			return;
		}
		if (!pInterface->GetNumber(2, bounds.m_fTop))
		{
			pInterface->RaiseError(2, "top bounds as a number is required.");
			return;
		}
		if (!pInterface->GetNumber(3, bounds.m_fRight))
		{
			pInterface->RaiseError(3, "right bounds as a number is required.");
			return;
		}
		if (!pInterface->GetNumber(4, bounds.m_fBottom))
		{
			pInterface->RaiseError(4, "bottom bounds as a number is required.");
			return;
		}

		Int32 iInsertionDepth = 0;
		if (!pInterface->GetInteger(6, iInsertionDepth))
		{
			pInterface->RaiseError(6, "insertion depth number expected.");
			return;
		}

		if (iInsertionDepth <= 0)
		{
			iInsertionDepth = m_uDynamicDepth;
		}
		m_uDynamicDepth = Max(m_uDynamicDepth, (UInt16)iInsertionDepth);

		pChildInstance.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) UI::HitShapeInstance(
			bounds));

		GetInstance()->SetChildAtDepth(
			*pOwner,
			(UInt16)iInsertionDepth,
			pChildInstance);

		Byte const* sName = nullptr;
		UInt32 zNameLengthInBytes = 0;
		if (pInterface->GetString(5, sName, zNameLengthInBytes))
		{
			pChildInstance->SetName(HString(sName, zNameLengthInBytes));
		}

		++m_uDynamicDepth;
	}

	// Can longjmp, so must be last with no complex members on the stack.
	pOwner->TransferOwnershipToScript(pInterface, pChildInstance);
}

/**
 * Utility - shared by AddFullScreenClipper and AddNativeChildHitShapeFullScreen,
 * returns a full screen sized bounds centered at the origin.
 */
static inline Falcon::Rectangle GetCenteredFullScreenBounds(const UI::Movie& movie)
{
	// Generate the bounds from the viewport.
	auto const bounds = movie.ViewportToWorldBounds();

	// Recenter - we always want the bounds centered around (0, 0, 0)
	// so that is scrolled properly.
	auto const fWidth = bounds.GetWidth();
	auto const fHeight = bounds.GetHeight();
	auto const vCenter = bounds.GetCenter();
	return Falcon::Rectangle::Create(
		vCenter.X - fWidth * 0.5f,
		vCenter.X + fWidth * 0.5f,
		vCenter.Y - fHeight * 0.5f,
		vCenter.Y + fHeight * 0.5f);
}

/**
 * A full-screen clipper is a MovieClip that will be inserted at the
 * lowest depth (depth 1) and will have bounds equal to the size
 * of the current rendering viewport.
 */
void ScriptUIMovieClipInstance::AddFullScreenClipper(Script::FunctionInterface* pInterface)
{
	// Signed 16-bit max value.
	static const UInt16 kuMaxClipDepth = 32767;

	auto pInstance = GetInstance();

	// Retrieve the owner movie.
	auto pOwner(GetOwner());
	if (!pOwner)
	{
		pInterface->RaiseError(-1, "owner native UIMovie has already been destroyed.");
		return;
	}

	// Drop any existing clipper, if it exists.
	pInstance->RemoveChildByName(kFullScreenClipper);

	// Clipper must go first, so check for an existing element. If already
	// a clipper, nothing to do. If not a clipper, check its depth - if
	// a depth of 0, we need to push back all existing children to make
	// room for the clipper.
	//
	// Depth of 0 is special - Flash timelines always place children
	// at a depth of at least 1, but Falcon code is fine with usage of
	// 0 depth. As such, we use this "reserved" depth to place the clipper
	// in front of all other children under normal usage circumstances.
	// This avoids the need to push back elements (and also of movie clip
	// timelines in the root fighting with this runtime change).
	SharedPtr<Falcon::Instance> pChild;
	if (pInstance->GetChildAt(0, pChild))
	{
		// Check if already a clipper.
		if (pChild->GetType() == Falcon::InstanceType::kMovieClip)
		{
			auto pMovieClipChild = (Falcon::MovieClipInstance*)pChild.GetPtr();
			if (pMovieClipChild->GetScissorClip() &&
				pMovieClipChild->GetClipDepth() == kuMaxClipDepth)
			{
				// This is already a clipper, we're done.
				goto done;
			}
		}

		// One way or another, we need to insert a clipper, so
		// check depth - if 0, we need to push back all existing elements.
		if (pChild->GetDepthInParent() <= 0)
		{
			// Push back all children by 1 depth value so the clipper is first.
			m_uDynamicDepth = pInstance->IncreaseAllChildDepthByOne();
		}
	}

	// If we get here, generate a clipper MovieClip.
	{
		SharedPtr<Falcon::MovieClipInstance> pClipper(SEOUL_NEW(MemoryBudgets::Falcon) Falcon::MovieClipInstance(SharedPtr<Falcon::MovieClipDefinition>(
			SEOUL_NEW(MemoryBudgets::Falcon) Falcon::MovieClipDefinition(kDefaultMovieClipClassName))));

		// Clipper shape is a hit shape with viewport bounds.
		// Generate the bounds from the viewport.
		auto const bounds = GetCenteredFullScreenBounds(*pOwner);

		// Generate the hit shape that will size the clipper.
		SharedPtr<UI::HitShapeInstance> pChildInstance(SEOUL_NEW(MemoryBudgets::UIRuntime) UI::HitShapeInstance(
			bounds));

		// Set the clipper's hit shape.
		pClipper->SetChildAtDepth(*pOwner, 1u, pChildInstance);

		// The clipper has a max clip depth and is a scissor clip for perf.
		pClipper->SetClipDepth(kuMaxClipDepth);
		pClipper->SetScissorClip(true);
		pClipper->SetName(kFullScreenClipper);

		// Now insert the clipper itself - place at depth 0
		// to give it special placement in front of everything else.
		pInstance->SetChildAtDepth(*pOwner, 0u, pClipper);
	}

done:
	return;
}

/**
 * Adds a child hit shape for hit testing that is sized
 * to the current rendering viewport.
 */
void ScriptUIMovieClipInstance::AddChildHitShapeFullScreen(Script::FunctionInterface* pInterface)
{
	auto pOwner(GetOwner());
	if (!pOwner)
	{
		pInterface->RaiseError(-1, "owner native UIMovie has already been destroyed.");
		return;
	}

	SharedPtr<Falcon::Instance> pChildInstance;
	{
		// Generate the bounds from the viewport.
		auto const bounds = GetCenteredFullScreenBounds(*pOwner);

		// Generate the hit shape instance itself for testing.
		pChildInstance.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) UI::HitShapeInstance(
			bounds));

		// Insert the hit tester.
		GetInstance()->SetChildAtDepth(
			*pOwner,
			m_uDynamicDepth,
			pChildInstance);

		// Give it a name if one was defined.
		Byte const* sName = nullptr;
		UInt32 zNameLengthInBytes = 0;
		if (pInterface->GetString(1, sName, zNameLengthInBytes))
		{
			pChildInstance->SetName(HString(sName, zNameLengthInBytes));
		}

		++m_uDynamicDepth;
	}

	// Can longjmp, so must be last with no complex members on the stack.
	pOwner->TransferOwnershipToScript(pInterface, pChildInstance);
}

void ScriptUIMovieClipInstance::AddChildHitShapeWithMyBounds(Script::FunctionInterface* pInterface)
{
	auto pOwner(GetOwner());
	if (!pOwner)
	{
		pInterface->RaiseError(-1, "owner native UIMovie has already been destroyed.");
		return;
	}

	SharedPtr<Falcon::Instance> pChildInstance;
	{
		Falcon::Rectangle bounds;
		if (!m_pInstance->ComputeBounds(bounds))
		{
			pInterface->RaiseError(-1, "failed computing bounds of parent, likely parent is a Falcon::MovieClip with no children (it has no bounds).");
			return;
		}

		bounds = Falcon::TransformRectangle(m_pInstance->GetTransform().Inverse(), bounds);

		pChildInstance.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) UI::HitShapeInstance(
			bounds));

		GetInstance()->SetChildAtDepth(
			*pOwner,
			m_uDynamicDepth,
			pChildInstance);

		Byte const* sName = nullptr;
		UInt32 zNameLengthInBytes = 0;
		if (pInterface->GetString(1, sName, zNameLengthInBytes))
		{
			pChildInstance->SetName(HString(sName, zNameLengthInBytes));
		}

		++m_uDynamicDepth;
	}

	// Can longjmp, so must be last with no complex members on the stack.
	pOwner->TransferOwnershipToScript(pInterface, pChildInstance);
}

void ScriptUIMovieClipInstance::AddChildStage3D(Script::FunctionInterface* pInterface)
{
	auto pOwner(GetOwner());
	if (!pOwner)
	{
		pInterface->RaiseError(-1, "owner native UIMovie has already been destroyed.");
		return;
	}

	SharedPtr<Falcon::Instance> pChildInstance;
	{
		FilePath filePath;
		if (!pInterface->GetFilePath(1, filePath))
		{
			pInterface->RaiseError(1, "expected FilePath to texture substitution bitmap.");
			return;
		}

		Int32 iWidth = 0;
		if (!pInterface->GetInteger(2, iWidth))
		{
			pInterface->RaiseError(2, "width of the bitmap in pixels is required.");
			return;
		}

		Int32 iHeight = 0;
		if (!pInterface->GetInteger(3, iHeight))
		{
			pInterface->RaiseError(3, "height of the bitmap in pixels is required.");
			return;
		}

		pChildInstance.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) UI::Stage3D(
			filePath,
			iWidth,
			iHeight));

		GetInstance()->SetChildAtDepth(
			*pOwner,
			m_uDynamicDepth,
			pChildInstance);

		if (!pInterface->IsNilOrNone(4))
		{
			Byte const* sName = nullptr;
			UInt32 zNameLengthInBytes = 0;
			if (!pInterface->GetString(4, sName, zNameLengthInBytes))
			{
				pInterface->RaiseError(4, "optional name argument must be a string.");
				return;
			}
			pChildInstance->SetName(HString(sName, zNameLengthInBytes));
		}

		if (!pInterface->IsNilOrNone(5))
		{
			Bool bCenter = false;
			if (!pInterface->GetBoolean(5, bCenter))
			{
				pInterface->RaiseError(5, "optional bCenterToParent argument must be a boolean.");
				return;
			}

			if (bCenter)
			{
				Falcon::Rectangle bounds;
				if (pChildInstance->ComputeBounds(bounds))
				{
					Float32 const fWidth = bounds.GetWidth();
					Float32 const fHeight = bounds.GetHeight();

					Vector2D vPosition = pChildInstance->GetPosition();
					vPosition.X -= (fWidth / 2.0f);
					vPosition.Y -= (fHeight / 2.0f);
					pChildInstance->SetPosition(vPosition.X, vPosition.Y);
				}
			}
		}

		++m_uDynamicDepth;
	}

	// Can longjmp, so must be last with no complex members on the stack.
	pOwner->TransferOwnershipToScript(pInterface, pChildInstance);
}

Float ScriptUIMovieClipInstance::GetDepth3D() const
{
	return GetInstance()->GetDepth3D();
}

Bool ScriptUIMovieClipInstance::GetAbsorbOtherInput() const
{
	return GetInstance()->GetAbsorbOtherInput();
}

UInt32 ScriptUIMovieClipInstance::GetChildCount() const
{
	return GetInstance()->GetChildCount();
}

HString ScriptUIMovieClipInstance::GetClassName() const
{
	// Determine the class name.
	HString const sClassName(GetInstance()->GetMovieClipDefinition()->GetClassName());
	HString const className(sClassName.IsEmpty() ? kDefaultMovieClipClassName : sClassName);

	return className;
}

Int32 ScriptUIMovieClipInstance::GetCurrentFrame() const
{
	return GetInstance()->GetCurrentFrame() + 1;
}

void ScriptUIMovieClipInstance::GetCurrentLabel(Script::FunctionInterface* pInterface) const
{
	auto const label = GetInstance()->GetCurrentLabel();
	if (!label.IsEmpty())
	{
		pInterface->PushReturnString(label.CStr(), label.GetSizeInBytes());
	}
	else
	{
		pInterface->PushReturnNil();
	}
}

Bool ScriptUIMovieClipInstance::GetExactHitTest() const
{
	return GetInstance()->GetExactHitTest();
}

SharedPtr<Falcon::MovieClipInstance> ScriptUIMovieClipInstance::GetInstance() const
{
	SEOUL_ASSERT(!m_pInstance.IsValid() || Falcon::InstanceType::kMovieClip == m_pInstance->GetType());
	return SharedPtr<Falcon::MovieClipInstance>((Falcon::MovieClipInstance*)m_pInstance.GetPtr());
}

void ScriptUIMovieClipInstance::GetChildAt(Script::FunctionInterface* pInterface) const
{
	SharedPtr<Falcon::Instance> pChildInstance;
	{
		Int32 iIndex = 0;
		if (!pInterface->GetInteger(1, iIndex) || iIndex < 0)
		{
			pInterface->RaiseError(1, "1-based index of child to get is required.");
			return;
		}

		if (!GetInstance()->GetChildAt(iIndex, pChildInstance))
		{
			pInterface->PushReturnNil();
			return;
		}
	}

	auto pOwner(GetOwner());
	if (!pOwner)
	{
		pInterface->RaiseError(-1, "owner native UIMovie has already been destroyed.");
		return;
	}

	// Can longjmp, so must be last with no complex members on the stack.
	pOwner->TransferOwnershipToScript(pInterface, pChildInstance);
}

void ScriptUIMovieClipInstance::GetChildByNameFromSubTree(Script::FunctionInterface* pInterface) const
{
	SharedPtr<Falcon::Instance> pChildInstance;
	{
		UInt32 zSizeInBytes = 0;
		const char* sLuaName = "";
		if (!pInterface->GetString(1, sLuaName, zSizeInBytes))
		{
			pInterface->RaiseError(1, "string name of child to get is required.");
			return;
		}

		HString const sName(sLuaName, zSizeInBytes);

		if (!GetInstance()->GetChildByNameFromSubTree(sName, pChildInstance))
		{
			pInterface->PushReturnNil();
			return;
		}
	}

	auto pOwner(GetOwner());
	if (!pOwner)
	{
		pInterface->RaiseError(-1, "owner native UIMovie has already been destroyed.");
		return;
	}

	// Can longjmp, so must be last with no complex members on the stack.
	pOwner->TransferOwnershipToScript(pInterface, pChildInstance);
}

void ScriptUIMovieClipInstance::GetChildByPath(Script::FunctionInterface* pInterface) const
{
	SharedPtr<Falcon::Instance> pChildInstance;
	{
		auto const iArgs = pInterface->GetArgumentCount();

		if (iArgs < 2)
		{
			pInterface->RaiseError("at least one string child name argument is required.");
			return;
		}

		UInt32 u = 0;
		Byte const* s = nullptr;
		pChildInstance = GetInstance();
		for (Int i = 1; i < iArgs; ++i)
		{
			if (!pInterface->GetString(i, s, u))
			{
				pInterface->RaiseError(i, "string name of child to get is required.");
				return;
			}

			HString childName;
			if (!HString::Get(childName, s, u))
			{
				pInterface->PushReturnNil();
				return;
			}

			if (pChildInstance->GetType() != Falcon::InstanceType::kMovieClip)
			{
				pInterface->RaiseError(i, "attempt to get child of non-MovieClip instance.");
				return;
			}

			if (!((Falcon::MovieClipInstance const*)pChildInstance.GetPtr())->GetChildByName(childName, pChildInstance))
			{
				pInterface->PushReturnNil();
				return;
			}
		}
	}

	auto pOwner(GetOwner());
	if (!pOwner)
	{
		pInterface->RaiseError(-1, "owner native UIMovie has already been destroyed.");
		return;
	}

	// Can longjmp, so must be last with no complex members on the stack.
	pOwner->TransferOwnershipToScript(pInterface, pChildInstance);
}

Bool ScriptUIMovieClipInstance::GetHitTestChildren() const
{
	return GetInstance()->GetHitTestChildrenMask() != 0;
}

UInt8 ScriptUIMovieClipInstance::GetHitTestChildrenMode() const
{
	return GetInstance()->GetHitTestChildrenMask();
}

Bool ScriptUIMovieClipInstance::GetHitTestSelf() const
{
	return GetInstance()->GetHitTestSelfMask() != 0;
}

UInt8 ScriptUIMovieClipInstance::GetHitTestSelfMode() const
{
	return GetInstance()->GetHitTestSelfMask();
}

UInt32 ScriptUIMovieClipInstance::GetTotalFrames() const
{
	return GetInstance()->GetTotalFrames();
}

void ScriptUIMovieClipInstance::GetHitTestableBounds(Script::FunctionInterface* pInterface) const
{
	UInt32 uHitTestMask = 0u;
	if (!pInterface->GetUInt32(1, uHitTestMask))
	{
		pInterface->RaiseError(1, "hit testing mask expected.");
		return;
	}

	Falcon::Rectangle bounds = Falcon::Rectangle::Create(0, 0, 0, 0);
	GetInstance()->ComputeHitTestableBounds(bounds, (UInt8)uHitTestMask);

	pInterface->PushReturnNumber(bounds.m_fLeft);
	pInterface->PushReturnNumber(bounds.m_fTop);
	pInterface->PushReturnNumber(bounds.m_fRight);
	pInterface->PushReturnNumber(bounds.m_fBottom);
}

void ScriptUIMovieClipInstance::GetHitTestableLocalBounds(Script::FunctionInterface* pInterface) const
{
	UInt32 uHitTestMask = 0u;
	if (!pInterface->GetUInt32(1, uHitTestMask))
	{
		pInterface->RaiseError(1, "hit testing mask expected.");
		return;
	}

	Falcon::Rectangle bounds = Falcon::Rectangle::Create(0, 0, 0, 0);
	GetInstance()->ComputeHitTestableLocalBounds(bounds, (UInt8)uHitTestMask);

	pInterface->PushReturnNumber(bounds.m_fLeft);
	pInterface->PushReturnNumber(bounds.m_fTop);
	pInterface->PushReturnNumber(bounds.m_fRight);
	pInterface->PushReturnNumber(bounds.m_fBottom);
}

void ScriptUIMovieClipInstance::GetHitTestableWorldBounds(Script::FunctionInterface* pInterface) const
{
	UInt32 uHitTestMask = 0u;
	if (!pInterface->GetUInt32(1, uHitTestMask))
	{
		pInterface->RaiseError(1, "hit testing mask expected.");
		return;
	}

	Falcon::Rectangle bounds = Falcon::Rectangle::Create(0, 0, 0, 0);
	GetInstance()->ComputeHitTestableWorldBounds(bounds, (UInt8)uHitTestMask);

	pInterface->PushReturnNumber(bounds.m_fLeft);
	pInterface->PushReturnNumber(bounds.m_fTop);
	pInterface->PushReturnNumber(bounds.m_fRight);
	pInterface->PushReturnNumber(bounds.m_fBottom);
}

Bool ScriptUIMovieClipInstance::GotoAndPlay(UInt32 uFrame)
{
	auto pOwner(GetOwner());
	if (!pOwner)
	{
		return false;
	}

	// GotoAndStop and GotoAndPlay are 1-based to match ActionScript 3 semantics
	// (frame 1 is considered the first frame, not 0), but we also mimic the apparent
	// behavior around 0, in which case the value is effectively treated as if it were
	// 1.
	uFrame = (0 == uFrame ? uFrame : (uFrame - 1));

	return GetInstance()->GotoAndPlay(
		*pOwner,
		(UInt16)uFrame);
}

void ScriptUIMovieClipInstance::GotoAndPlayByLabel(Script::FunctionInterface* pInterface)
{
	Falcon::LabelName label;
	{
		Byte const* s = nullptr;
		UInt32 u = 0u;
		if (!pInterface->GetString(1, s, u))
		{
			pInterface->RaiseError(1, "expected string.");
			return;
		}

		label = Falcon::LabelName(s, u);
	}

	pInterface->PushReturnBoolean(DoGotoAndPlayByLabel(label));
}

Bool ScriptUIMovieClipInstance::DoGotoAndPlayByLabel(const Falcon::LabelName& label)
{
	auto pOwner(GetOwner());
	if (!pOwner)
	{
		return false;
	}

	Bool bResult = GetInstance()->GotoAndPlayByLabel(
		*pOwner,
		label);

	if (!bResult)
	{
		SEOUL_LOG_FAILEDGOTOLABEL("GotoAndPlayByLabel did not find label: %s for instance named %s", label.CStr(), GetName().CStr());
	}
	return bResult;
}

Bool ScriptUIMovieClipInstance::GotoAndStop(UInt32 uFrame)
{
	auto pOwner(GetOwner());
	if (!pOwner)
	{
		return false;
	}

	// GotoAndStop and GotoAndPlay are 1-based to match ActionScript 3 semantics
	// (frame 1 is considered the first frame, not 0), but we also mimic the apparent
	// behavior around 0, in which case the value is effectively treated as if it were
	// 1.
	uFrame = (0 == uFrame ? uFrame : (uFrame - 1));

	return GetInstance()->GotoAndStop(
		*pOwner,
		(UInt16)uFrame);
}

void ScriptUIMovieClipInstance::GotoAndStopByLabel(Script::FunctionInterface* pInterface)
{
	Falcon::LabelName label;
	{
		Byte const* s = nullptr;
		UInt32 u = 0u;
		if (!pInterface->GetString(1, s, u))
		{
			pInterface->RaiseError(1, "expected string.");
			return;
		}

		label = Falcon::LabelName(s, u);
	}

	pInterface->PushReturnBoolean(DoGotoAndStopByLabel(label));
}

Bool ScriptUIMovieClipInstance::DoGotoAndStopByLabel(const Falcon::LabelName& label)
{
	auto pOwner(GetOwner());
	if (!pOwner)
	{
		return false;
	}

	Bool bResult = GetInstance()->GotoAndStopByLabel(
		*pOwner,
		label);
	(void)bResult;

#if SEOUL_LOGGING_ENABLED
	if (!bResult)
	{
		SEOUL_LOG_FAILEDGOTOLABEL("GotoAndStopByLabel did not find label: %s for instance named %s", label.CStr(), GetName().CStr());
	}
#endif // /#if SEOUL_LOGGING_ENABLED

	return bResult;
}

Bool ScriptUIMovieClipInstance::IsPlaying() const
{
	return GetInstance()->IsPlaying();
}

void ScriptUIMovieClipInstance::SetAutoCulling(Bool bEnableAutoCulling)
{
	GetInstance()->SetAutoCulling(bEnableAutoCulling);
}

void ScriptUIMovieClipInstance::SetCastPlanarShadows(Bool bCastPlanarShadows)
{
	GetInstance()->SetCastPlanarShadows(bCastPlanarShadows);
}

void ScriptUIMovieClipInstance::SetDeferDrawing(Bool bDeferDrawing)
{
	GetInstance()->SetDeferDrawing(bDeferDrawing);
}

void ScriptUIMovieClipInstance::HitTest(Script::FunctionInterface* pInterface) const
{
	auto pOwner(GetOwner());
	if (!pOwner)
	{
		pInterface->RaiseError(-1, "owner native UIMovie has already been destroyed.");
		return;
	}

	SharedPtr<Falcon::Instance> pChildInstance;
	{
		// Hit test position are optional. If either is not specified,
		// the current mouse position is used instead.
		auto const vMousePosition(pOwner->GetMousePositionInWorld(UI::Manager::Get()->GetMousePosition()));

		Int32 iMask = 0;
		Float fX = 0.0f;
		Float fY = 0.0f;
		if (!pInterface->GetInteger(1, iMask))
		{
			pInterface->RaiseError(1, "Missing required argument mask bits for hit test.");
			return;
		}
		if (!pInterface->GetNumber(2, fX))
		{
			fX = vMousePosition.X;
		}
		if (!pInterface->GetNumber(3, fY))
		{
			fY = vMousePosition.Y;
		}

		auto tester(pOwner->GetHitTester());

		// Add any additional depth.
		{
			auto pParent = GetInstance()->GetParent();
			while (nullptr != pParent)
			{
				tester.PushDepth3D(pParent->GetDepth3D(), pParent->GetIgnoreDepthProjection());
				pParent = pParent->GetParent();
			}
		}

		SharedPtr<Falcon::MovieClipInstance> pInstance;
		SharedPtr<Falcon::Instance> pLeafInstance;
		Falcon::HitTestResult const eResult = GetInstance()->HitTest(
			tester,
			(UInt8)iMask,
			fX,
			fY,
			pInstance,
			pLeafInstance);

		if (Falcon::HitTestResult::kHit != eResult)
		{
			pInterface->PushReturnNil();
			return;
		}

		// Done.
		pChildInstance = pInstance;
	}

	// Can longjmp, so must be last with no complex members on the stack.
	pOwner->TransferOwnershipToScript(pInterface, pChildInstance);
}

void ScriptUIMovieClipInstance::IncreaseAllChildDepthByOne()
{
	GetInstance()->IncreaseAllChildDepthByOne();
}

void ScriptUIMovieClipInstance::Play()
{
	GetInstance()->Play();
}

void ScriptUIMovieClipInstance::SetAutoDepth3D(Bool bEnableAutoDepth3D)
{
	GetInstance()->SetAutoDepth3D(bEnableAutoDepth3D);
}

void ScriptUIMovieClipInstance::RemoveAllChildren()
{
	GetInstance()->RemoveAllChildren();
}

void ScriptUIMovieClipInstance::RemoveChildAt(Script::FunctionInterface* pInterface)
{
	UInt32 uIndex = 0u;
	if (!pInterface->GetUInt32(1, uIndex))
	{
		pInterface->RaiseError(1, "expected child index.");
		return;
	}

	if (GetInstance()->RemoveChildAt(uIndex))
	{
		RefreshDynamicDepth();
		pInterface->PushReturnBoolean(true);
	}
	else
	{
		pInterface->PushReturnBoolean(false);
	}
}

void ScriptUIMovieClipInstance::RemoveChildByName(Script::FunctionInterface* pInterface)
{
	HString name;
	if (!pInterface->GetString(1, name))
	{
		pInterface->RaiseError(1, "expected string child name.");
		return;
	}

	if (GetInstance()->RemoveChildByName(name))
	{
		RefreshDynamicDepth();
		pInterface->PushReturnBoolean(true);
	}
	else
	{
		pInterface->PushReturnBoolean(false);
	}
}

/**
 * Convenience utility for nodes that are pooled. Performs the following
 * operations:
 * - SetAlpha(1)
 * - GotoAndStop(1)
 * - SetPosition(0, 0)
 * - SetRotation(0)
 * - SetVisible(true)
 *
  * Also, kills any tweens that may still be running on this instance.
  */
void ScriptUIMovieClipInstance::RestoreTypicalDefault()
{
	auto p(GetInstance());

	// Remove any dynamically spawned children.
	auto const uMinDynamicDepth = (p->GetMovieClipDefinition()->GetMaxDepth() + 1);
	while (m_uDynamicDepth > uMinDynamicDepth)
	{
		p->RemoveChildAtDepth(m_uDynamicDepth);
		--m_uDynamicDepth;
	}

	auto pOwner(GetOwner());
	if (pOwner)
	{
		p->GotoAndStop(*pOwner, 0u);
		pOwner->CancelAllTweens(p);
		pOwner->CancelAllMotions(p);
	}

	p->SetAlpha(1);
	p->SetPosition(0, 0);
	p->SetRotationInRadians(0);
	p->SetVisible(true);
}

void ScriptUIMovieClipInstance::SetAbsorbOtherInput(Bool bAbsorbOtherInput)
{
	GetInstance()->SetAbsorbOtherInput(bAbsorbOtherInput);
}

void ScriptUIMovieClipInstance::SetDepth3D(Float f)
{
	GetInstance()->SetDepth3D(f);
}

void ScriptUIMovieClipInstance::SetDepth3DFromYLoc(Float fYLoc)
{
	Float fDepth = UI::Manager::Get()->GetRenderer().ComputeDepth3D(fYLoc);
	GetInstance()->SetDepth3D(fDepth);
}

void ScriptUIMovieClipInstance::SetEnterFrame(Bool bEnableEnterFrame)
{
	GetInstance()->SetEnableEnterFrame(bEnableEnterFrame);
}

void ScriptUIMovieClipInstance::SetInputActionDisabled(Bool bInputActionDisabled)
{
	GetInstance()->SetInputActionDisabled(bInputActionDisabled);
}

void ScriptUIMovieClipInstance::SetTickEvent(Bool bEnableTickEvent)
{
	if (m_bEnableTickEvents != bEnableTickEvent)
	{
		m_bEnableTickEvents = bEnableTickEvent;
		auto pOwner(GetOwner());
		if (pOwner.IsValid())
		{
			if (m_bEnableTickEvents)
			{
				pOwner->EnableTickEvents(m_pInstance.GetPtr());
			}
			else
			{
				pOwner->DisableTickEvents(m_pInstance.GetPtr());
			}
		}
	}
}

void ScriptUIMovieClipInstance::SetTickScaledEvent(Bool bEnableTickEvent)
{
	if (m_bEnableTickScaledEvents != bEnableTickEvent)
	{
		m_bEnableTickScaledEvents = bEnableTickEvent;
		auto pOwner(GetOwner());
		if (pOwner.IsValid())
		{
			if (m_bEnableTickScaledEvents)
			{
				pOwner->EnableTickScaledEvents(m_pInstance.GetPtr());
			}
			else
			{
				pOwner->DisableTickScaledEvents(m_pInstance.GetPtr());
			}
		}
	}
}

void ScriptUIMovieClipInstance::SetExactHitTest(Bool bExactHitTest)
{
	GetInstance()->SetExactHitTest(bExactHitTest);
}

void ScriptUIMovieClipInstance::SetHitTestChildren(Bool bHitTestChildren)
{
	GetInstance()->SetHitTestChildrenMask(bHitTestChildren ? 0xFF : 0);
}

void ScriptUIMovieClipInstance::SetHitTestChildrenMode(Int32 iMask)
{
	GetInstance()->SetHitTestChildrenMask((UInt8)iMask);
}

void ScriptUIMovieClipInstance::SetHitTestSelf(Bool bHitTestSelf)
{
	GetInstance()->SetHitTestSelfMask(bHitTestSelf ? 0xFF : 0);
}

void ScriptUIMovieClipInstance::SetHitTestSelfMode(Int32 iMask)
{
	GetInstance()->SetHitTestSelfMask((UInt8)iMask);
}

void ScriptUIMovieClipInstance::SetReorderChildrenFromDepth3D(Bool b)
{
	GetInstance()->SetReorderChildrenFromDepth3D(b);
}

void ScriptUIMovieClipInstance::Stop()
{
	GetInstance()->Stop();
}

void ScriptUIMovieClipInstance::RefreshDynamicDepth()
{
	UInt16 const uMaxStaticDepth = GetInstance()->GetMovieClipDefinition()->GetMaxDepth();
	while (m_uDynamicDepth > uMaxStaticDepth + 1)
	{
		if (GetInstance()->HasChildAtDepth(m_uDynamicDepth))
		{
			return;
		}

		--m_uDynamicDepth;
	}
}

void ScriptUIMovieClipInstance::SetChildrenVisible(Bool bVisible)
{
	SharedPtr<Falcon::Instance> pChild;
	auto pParent(GetInstance());
	UInt32 const uCount = pParent->GetChildCount();
	for (UInt32 i = 0u; i < uCount; ++i)
	{
		SEOUL_VERIFY(pParent->GetChildAt(i, pChild));
		pChild->SetVisible(bVisible);
	}
}

} // namespace Seoul
