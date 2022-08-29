/**
 * \file ScriptUIInstance.cpp
 * \brief Script binding around Falcon::Instance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconMovieClipInstance.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ReflectionRegistry.h"
#include "ScriptFunctionInterface.h"
#include "ScriptUIInstance.h"
#include "ScriptUIMovie.h"
#include "ScriptUIMotionCompletion.h"
#include "ScriptUITweenCompletion.h"
#include "ScriptVm.h"
#include "SeoulProfiler.h"
#include "UIManager.h"
#include "UIRenderer.h"
#include "VmStats.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptUIInstance, TypeFlags::kDisableCopy)
	SEOUL_METHOD(AddMotion)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "int", "string sType, SlimCS.Vfunc0 callback, params object[] aArgs")
	SEOUL_METHOD(CancelMotion)
	SEOUL_METHOD(AddTween)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "int", "TweenTarget eTarget, params object[] aArgs")
	SEOUL_METHOD(AddTweenCurve)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "int", "TweenType eType, TweenTarget eTarget, params object[] aArgs")
	SEOUL_METHOD(CancelTween)
	SEOUL_METHOD(Clone)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "DisplayObject")
	SEOUL_METHOD(GetAdditiveBlend)
	SEOUL_METHOD(GetAlpha)
	SEOUL_METHOD(GetColorTransform)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double, double, double, double, double)")
	SEOUL_METHOD(GetBounds)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double, double, double)")
	SEOUL_METHOD(GetBoundsIn)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double, double, double)", "ScriptUIInstance targetCoordinateSpace")
	SEOUL_METHOD(GetClipDepth)
	SEOUL_METHOD(GetDepthInParent)
	SEOUL_METHOD(GetIgnoreDepthProjection)
	SEOUL_METHOD(GetLocalBounds)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double, double, double)")
	SEOUL_METHOD(GetName)
	SEOUL_METHOD(GetFullName)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string")
	SEOUL_METHOD(GetParent)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "MovieClip")
	SEOUL_METHOD(GetPosition)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)")
	SEOUL_METHOD(GetPositionX)
	SEOUL_METHOD(GetPositionY)
	SEOUL_METHOD(GetRotation)
	SEOUL_METHOD(GetScale)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)")
	SEOUL_METHOD(GetScaleX)
	SEOUL_METHOD(GetScaleY)
	SEOUL_METHOD(GetScissorClip)
	SEOUL_METHOD(GetVisible)
	SEOUL_METHOD(GetVisibleToRoot)
	SEOUL_METHOD(GetWorldBounds)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double, double, double)")
	SEOUL_METHOD(GetWorldPosition)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)")
	SEOUL_METHOD(LocalToWorld)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)", "double fX, double fY")
	SEOUL_METHOD(HasParent)
	SEOUL_METHOD(RemoveFromParent)
	SEOUL_METHOD(SetAdditiveBlend)
	SEOUL_METHOD(SetAlpha)
	SEOUL_METHOD(SetClipDepth)
	SEOUL_METHOD(SetColorTransform)
	SEOUL_METHOD(SetIgnoreDepthProjection)
	SEOUL_METHOD(SetName)
	SEOUL_METHOD(SetPosition)
	SEOUL_METHOD(SetPositionX)
	SEOUL_METHOD(SetPositionY)
	SEOUL_METHOD(SetRotation)
	SEOUL_METHOD(SetScale)
	SEOUL_METHOD(SetScaleX)
	SEOUL_METHOD(SetScaleY)
	SEOUL_METHOD(SetScissorClip)
	SEOUL_METHOD(SetVisible)
	SEOUL_METHOD(SetWorldPosition)
	SEOUL_METHOD(GetWorldDepth3D)

	SEOUL_METHOD(WorldToLocal)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)", "double fX, double fY")

	SEOUL_METHOD(GetLocalMousePosition)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)")
	SEOUL_METHOD(Intersects)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "double fWorldX, double fWorldY, bool bExactHitTest = false")

#if !SEOUL_SHIP
	SEOUL_METHOD(GetDebugName)
	SEOUL_METHOD(SetDebugName)
#endif // /#if !SEOUL_SHIP

SEOUL_END_TYPE()

static const HString kDefaultInstanceClassName("DisplayObject");
static const HString kConstructMethodName("Construct");

#if !SEOUL_SHIP
static Atomic32 s_NodeCountPerMovie[HStringDataProperties<HStringData::InternalIndexType>::GlobalArraySize];
#endif // /#if !SEOUL_SHIP

/** Shared body of AddTween() and AddTweenCommon(). */
static void AddTweenCommon(
	CheckedPtr<ScriptUIMovie> pOwner,
	const SharedPtr<Falcon::Instance>& pInstance,
	UI::TweenType eType,
	Script::FunctionInterface* pInterface,
	Int iArgument)
{
	SEOUL_ASSERT(pInstance.IsValid());

	if (!pOwner)
	{
		pInterface->RaiseError(-1, "null ScriptUIMovie owner.");
		return;
	}

	// First is the tween target argument.
	UI::TweenTarget eTarget = UI::TweenTarget::kAlpha;
	if (!pInterface->GetEnum(iArgument, eTarget))
	{
		pInterface->RaiseError(iArgument, "expected tween target.");
		return;
	}
	iArgument++;

	Float fStartValue = 0.0f;
	Float fEndValue = 0.0f;

	// No start/end value for timer targets.
	if (UI::TweenTarget::kTimer != eTarget)
	{
		if (!pInterface->GetNumber(iArgument, fStartValue))
		{
			pInterface->RaiseError(iArgument, "expected start value.");
			return;
		}
		iArgument++;

		if (!pInterface->GetNumber(iArgument, fEndValue))
		{
			pInterface->RaiseError(iArgument, "expected end value.");
			return;
		}
		iArgument++;
	}

	// All cases have a duration value.
	Float fDurationInSeconds = 0.0f;
	if (!pInterface->GetNumber(iArgument, fDurationInSeconds))
	{
		pInterface->RaiseError(iArgument, "expected duration in seconds.");
		return;
	}
	iArgument++;

	// Completion interface is always optional
	SharedPtr<ScriptUITweenCompletion> pCompletionInterface;
	if (!pInterface->IsNilOrNone(iArgument))
	{
		// When specified, must be a script function.
		SharedPtr<Script::VmObject> pObject;
		if (!pInterface->GetFunction(iArgument, pObject))
		{
			pInterface->RaiseError(iArgument, "expected completion callback, function.");
			return;
		}

		pOwner->TrackBinding(pObject);
		pCompletionInterface.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) ScriptUITweenCompletion(pObject));
	}

	// Finally, add the tween, then return the identifier.
	Int32 const iId = pOwner->AddTween(
		pInstance,
		eTarget,
		eType,
		fStartValue,
		fEndValue,
		fDurationInSeconds,
		pCompletionInterface);

	pInterface->PushReturnInteger(iId);
	return;
}

#if !SEOUL_SHIP
namespace
{

struct CountEntry SEOUL_SEALED
{
	HString m_Name;
	Atomic32Type m_Count{};

	Bool operator<(const CountEntry& b) const
	{
		return (m_Count > b.m_Count);
	}
};

} // namespace anonymous

void ScriptUIInstance::DebugLogInstanceCountsPerMovie()
{
	Vector<CountEntry, MemoryBudgets::Developer> v;
	for (auto i = 0; i < SEOUL_ARRAY_COUNT(s_NodeCountPerMovie); ++i)
	{
		if (s_NodeCountPerMovie[i] == 0)
		{
			continue;
		}

		CountEntry entry;
		entry.m_Count = s_NodeCountPerMovie[i];
		entry.m_Name.SetHandleValue((UInt32)i);
		v.PushBack(entry);
	}

	QuickSort(v.Begin(), v.End());

	for (auto const& e : v)
	{
		SEOUL_WARN("%s: %u", e.m_Name.CStr(), (UInt32)e.m_Count);
	}
}
#endif

ScriptUIInstance::ScriptUIInstance()
	: m_pInstance(nullptr)
	, m_hOwner()
{
#if !SEOUL_SHIP
	++g_VmStats.m_UIBindingUserData;
#endif
}

void ScriptUIInstance::Construct(
	const SharedPtr<Falcon::Instance>& pInstance,
	const ScriptUIMovie& owner)
{
	pInstance->AddWatcher();
	m_pInstance = pInstance;
	m_hOwner = owner.GetHandle();
#if !SEOUL_SHIP
	m_DebugMovieTypeName = GetOwner()->GetMovieTypeName();
	++s_NodeCountPerMovie[m_DebugMovieTypeName.GetHandleValue()];
#endif // /#if !SEOUL_SHIP
}

// We release root instance nodes a few per frame to avoid large spikes
// due to large sub-trees destroyed in single shots.
typedef Vector<Falcon::Instance*, MemoryBudgets::Scripting> FreeRootsVector;
static FreeRootsVector s_vFreeRoots;

// TODO: Use this as a minimum and adjust max based on total
// size of s_vFreeRoots.
static const UInt32 kuReleaseFreePerFrame = 100u;

static void Free(SharedPtr<Falcon::Instance>& rp)
{
	// Sanity handling, not thread safe.
	SEOUL_ASSERT(IsMainThread());

	// Get low-level, avoid ref counting overhead.
	s_vFreeRoots.PushBack(nullptr);
	rp.Swap(*reinterpret_cast<SharedPtr<Falcon::Instance>*>(&s_vFreeRoots.Back()));

	// Sanity that we performed the swap correctly.
	SEOUL_ASSERT(!rp.IsValid());
}

void ScriptUIInstance::FreeRoots(Bool bFlushAll)
{
	SEOUL_PROF("Script.FreeRoots");

	// Sanity handling, not thread safe.
	SEOUL_ASSERT(IsMainThread());

	// Start with initial.
	UInt32 uCount = kuReleaseFreePerFrame;

	// Get total.
	auto const uSize = s_vFreeRoots.GetSize();

	// If flushing, use total as count.
	if (bFlushAll)
	{
		uCount = uSize;
	}

	// Clamp to actual count.
	uCount = Min(uCount, uSize);

	// Release from end forward.
	auto pStart = (s_vFreeRoots.End() - 1);
	for (UInt32 i = 0u; i < uCount; ++i)
	{
		// Manual decrement.
		SeoulGlobalDecrementReferenceCount(*(pStart - i));
	}

	// Truncate to released.
	s_vFreeRoots.Resize(uSize - uCount);

	// If a flush all, also zero out the memory.
	if (bFlushAll)
	{
		FreeRootsVector vEmpty;
		s_vFreeRoots.Swap(vEmpty);
	}
}

ScriptUIInstance::~ScriptUIInstance()
{
	m_pInstance->RemoveWatcher();
	Free(m_pInstance);

#if !SEOUL_SHIP
	--s_NodeCountPerMovie[m_DebugMovieTypeName.GetHandleValue()];
	--g_VmStats.m_UIBindingUserData;
#endif
}

void ScriptUIInstance::Clone(Script::FunctionInterface* pInterface) const
{
	auto pOwner(GetOwner());
	if (!pOwner)
	{
		pInterface->RaiseError(-1, "null ScriptUIMovie owner.");
		return;
	}

	SharedPtr<Falcon::Instance> pClone(m_pInstance->Clone(*pOwner));

	// Can longjmp, so must be last with no complex members on the stack
	// (except for pClone). pOwner, while technically complex, is not
	// treated as such since a CheckedPtr<> matches the semantics of
	// a pointer, except for additional checked operations.
	pOwner->TransferOwnershipToScript(pInterface, pClone);
}

void ScriptUIInstance::AddMotion(Script::FunctionInterface* pInterface)
{
	SEOUL_ASSERT(m_pInstance.IsValid());

	auto pOwner(GetOwner());
	if (!pOwner)
	{
		pInterface->RaiseError(-1, "null ScriptUIMovie owner.");
		return;
	}

	Int iArgument = 1;

	// First is the motion type argument.
	HString sMotionType;
	if (!pInterface->GetString(iArgument, sMotionType))
	{
		pInterface->RaiseError(iArgument, "expected motion type.");
		return;
	}
	iArgument++;

	SharedPtr<UI::Motion> pMotion;
	Reflection::Type const* pType = Reflection::Registry::GetRegistry().GetType(sMotionType);
	if (nullptr != pType)
	{
		pMotion.Reset(pType->New<UI::Motion>(MemoryBudgets::UIRuntime));
	}

	if (!pMotion.IsValid())
	{
		pInterface->RaiseError(iArgument, "invalid motion type %s", sMotionType.CStr());
		return;
	}

	// record the instance onto the UI::Motion
	pMotion->SetInstance(m_pInstance);

	// Next is the completion callback
	// if the second argument is a function (required, but can be nil)
	if (!pInterface->IsNil(iArgument))
	{
		SharedPtr<ScriptUIMotionCompletion> pCompletionInterface;

		// When specified, must be a script function.
		SharedPtr<Script::VmObject> pObject;
		if (!pInterface->GetFunction(iArgument, pObject))
		{
			pInterface->RaiseError(iArgument, "expected completion callback, function.");
			return;
		}

		pOwner->TrackBinding(pObject);
		pCompletionInterface.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) ScriptUIMotionCompletion(pObject));
		pMotion->SetCompletionInterface(pCompletionInterface);
	}
	iArgument++;

	// pass remaining arguments on to the Construct method of the UI::Motion
	auto pMethod = pType->GetMethod(kConstructMethodName);
	if (nullptr != pMethod)
	{
		Reflection::MethodArguments aArguments;
		auto const iArgumentCount = pInterface->GetArgumentCount();
		for (auto i = iArgument; i < iArgumentCount; ++i)
		{
			if (!pInterface->GetAny(i, pMethod->GetTypeInfo().GetArgumentTypeInfo(i - iArgument), aArguments[i - iArgument]))
			{
				pInterface->RaiseError(i, "Argument type mismatch, expected %s", pMethod->GetTypeInfo().GetArgumentTypeInfo(i - iArgument).GetType().GetName().CStr());
				return;
			}
		}

		if (!pMethod->TryInvoke(pMotion->GetReflectionThis(), aArguments))
		{
			pInterface->RaiseError("Construct invoke failed for %s", sMotionType.CStr());
			return;
		}
	}
	else
	{
		pInterface->RaiseError("No Construct method found for %s", sMotionType.CStr());
		return;
	}

	// Finally, add the tween, then return the identifier.
	Int32 const iId = pOwner->AddMotion(pMotion);

	pInterface->PushReturnInteger(iId);
	return;
}

void ScriptUIInstance::CancelMotion(Int iIdentifier)
{
	auto pOwner(GetOwner());
	if (pOwner)
	{
		pOwner->CancelMotion(iIdentifier);
	}
}

/**
 * Run a tween with default curve (linear). Arguments:
 *
 * 1. UI::TweenTarget eTarget
 * [2. Float fStartValue]
 * [3. Float fEndValue]
 * 4. Float fDurationInSeconds
 * 5. <script function> pCompletionInterface
 *
 * Arguments #2 and #3 are optional if eTarget == kTimer.
 * Argument #5 is always optional.
 * All other arguments are required. Arguments #2 and #3 are
 * required if eTarget != kTimer.
 */
void ScriptUIInstance::AddTween(Script::FunctionInterface* pInterface)
{
	AddTweenCommon(GetOwner(), m_pInstance, UI::TweenType::kLine, pInterface, 1);
}

/**
 * Run a tween with explicit curve shape. Arguments:
 *
 * 1. UI::TweenType eType
 * 2. UI::TweenTarget eTarget
 * [3. Float fStartValue]
 * [4. Float fEndValue]
 * 5. Float fDurationInSeconds
 * 6. <script function> pCompletionInterface
 *
 * Arguments #3 and #4 are optional if eTarget == kTimer.
 * Argument #6 is always optional.
 * All other arguments are required. Arguments #2 and #3 are
 * required if eTarget != kTimer.
 */
void ScriptUIInstance::AddTweenCurve(Script::FunctionInterface* pInterface)
{
	Int iArgument = 1;

	// With AddTweenCurve, first argument is always the shape of the curve.
	UI::TweenType eType = UI::TweenType::kLine;
	if (!pInterface->GetEnum(iArgument, eType))
	{
		pInterface->RaiseError(iArgument, "expected tween curve type.");
		return;
	}
	iArgument++;

	// Remaining arguments are the same as AddTween().
	AddTweenCommon(GetOwner(), m_pInstance, eType, pInterface, iArgument);
}

void ScriptUIInstance::CancelTween(Int iIdentifier)
{
	auto pOwner(GetOwner());
	if (pOwner)
	{
		pOwner->CancelTween(iIdentifier);
	}
}

Bool ScriptUIInstance::GetAdditiveBlend() const
{
	return m_pInstance->GetBlendingFactor() != 0.0f;
}

Float ScriptUIInstance::GetAlpha() const
{
	return m_pInstance->GetAlpha();
}

void ScriptUIInstance::GetColorTransform(Script::FunctionInterface* pInterface) const
{
	const Falcon::ColorTransform& color = m_pInstance->GetColorTransform();

	pInterface->PushReturnNumber(color.m_fMulR);
	pInterface->PushReturnNumber(color.m_fMulG);
	pInterface->PushReturnNumber(color.m_fMulB);
	pInterface->PushReturnNumber(color.m_AddR);
	pInterface->PushReturnNumber(color.m_AddG);
	pInterface->PushReturnNumber(color.m_AddB);
}

void ScriptUIInstance::GetBounds(Script::FunctionInterface* pInterface) const
{
	Falcon::Rectangle bounds = Falcon::Rectangle::Create(0, 0, 0, 0);
	(void)m_pInstance->ComputeBounds(bounds);

	pInterface->PushReturnNumber(bounds.m_fLeft);
	pInterface->PushReturnNumber(bounds.m_fTop);
	pInterface->PushReturnNumber(bounds.m_fRight);
	pInterface->PushReturnNumber(bounds.m_fBottom);
}

HString ScriptUIInstance::GetClassName() const
{
	return kDefaultInstanceClassName;
}

void ScriptUIInstance::GetBoundsIn(Script::FunctionInterface* pInterface) const
{
	if (pInterface->IsNil(1))
	{
		GetLocalBounds(pInterface);
		return;
	}

	if (ScriptUIInstance* pInstance = pInterface->GetUserData<ScriptUIInstance>(1))
	{
		// Same as nil case.
		if (this == pInstance)
		{
			GetLocalBounds(pInterface);
			return;
		}

		Falcon::Rectangle bounds = Falcon::Rectangle::Create(0, 0, 0, 0);
		if (m_pInstance->ComputeLocalBounds(bounds))
		{
			// Transform into target coordinate space - world * inverse world of target.
			auto const m(pInstance->GetInstance()->ComputeWorldTransform().Inverse() * m_pInstance->ComputeWorldTransform());
			bounds = Falcon::TransformRectangle(m, bounds);
		}

		pInterface->PushReturnNumber(bounds.m_fLeft);
		pInterface->PushReturnNumber(bounds.m_fTop);
		pInterface->PushReturnNumber(bounds.m_fRight);
		pInterface->PushReturnNumber(bounds.m_fBottom);
	}
	else
	{
		pInterface->RaiseError(1, "expected DisplayObject");
		return;
	}
}

UInt16 ScriptUIInstance::GetClipDepth() const
{
	return m_pInstance->GetClipDepth();
}

UInt16 ScriptUIInstance::GetDepthInParent() const
{
	return m_pInstance->GetDepthInParent();
}

String ScriptUIInstance::GetDebugName() const
{
#if !SEOUL_SHIP
	return m_pInstance->GetDebugName();
#else
	return String();
#endif // /#if !SEOUL_SHIP
}

const SharedPtr<Falcon::Instance>& ScriptUIInstance::GetInstance() const
{
	return m_pInstance;
}

Bool ScriptUIInstance::GetIgnoreDepthProjection() const
{
	return m_pInstance->GetIgnoreDepthProjection();
}

void ScriptUIInstance::GetLocalBounds(Script::FunctionInterface* pInterface) const
{
	Falcon::Rectangle bounds = Falcon::Rectangle::Create(0, 0, 0, 0);
	(void)m_pInstance->ComputeLocalBounds(bounds);

	pInterface->PushReturnNumber(bounds.m_fLeft);
	pInterface->PushReturnNumber(bounds.m_fTop);
	pInterface->PushReturnNumber(bounds.m_fRight);
	pInterface->PushReturnNumber(bounds.m_fBottom);
}

HString ScriptUIInstance::GetName() const
{
	return m_pInstance->GetName();
}

void ScriptUIInstance::GetFullName(Script::FunctionInterface* pInterface) const
{
	String sFullName;
	m_pInstance->GatherFullName(sFullName);
	pInterface->PushReturnString(sFullName);
}

CheckedPtr<ScriptUIMovie> ScriptUIInstance::GetOwner() const
{
	return GetPtr<ScriptUIMovie>(m_hOwner);
}

void ScriptUIInstance::GetParent(Script::FunctionInterface* pInterface) const
{
	auto pParent = m_pInstance->GetParent();
	if (nullptr != pParent)
	{
		// Resolve the owner - always return nil on failure.
		auto pOwner(GetOwner());
		if (!pOwner)
		{
			pInterface->PushReturnNil();
			return;
		}

		// Done.
		SharedPtr<Falcon::Instance> pRef(pParent);
		pOwner->TransferOwnershipToScript(pInterface, pRef);
	}
	else
	{
		pInterface->PushReturnNil();
	}
}

void ScriptUIInstance::GetPosition(Script::FunctionInterface* pInterface) const
{
	Vector2D const vPosition = m_pInstance->GetPosition();

	pInterface->PushReturnNumber(vPosition.X);
	pInterface->PushReturnNumber(vPosition.Y);
}

Float ScriptUIInstance::GetPositionX() const
{
	return m_pInstance->GetPositionX();
}

Float ScriptUIInstance::GetPositionY() const
{
	return m_pInstance->GetPositionY();
}

Float ScriptUIInstance::GetRotation() const
{
	return m_pInstance->GetRotationInDegrees();
}

void ScriptUIInstance::GetScale(Script::FunctionInterface* pInterface) const
{
	Vector2D const vScale = m_pInstance->GetScale();

	pInterface->PushReturnNumber(vScale.X);
	pInterface->PushReturnNumber(vScale.Y);
}

Float ScriptUIInstance::GetScaleX() const
{
	return m_pInstance->GetScaleX();
}

Float ScriptUIInstance::GetScaleY() const
{
	return m_pInstance->GetScaleY();
}

Bool ScriptUIInstance::GetScissorClip() const
{
	return m_pInstance->GetScissorClip();
}

Bool ScriptUIInstance::GetVisible() const
{
	return m_pInstance->GetVisible();
}

Bool ScriptUIInstance::GetVisibleToRoot() const
{
	Falcon::Instance* p = m_pInstance.GetPtr();
	while (p)
	{
		if (!p->GetVisible())
		{
			return false;
		}
		p = p->GetParent();
	}

	return true;
}

void ScriptUIInstance::GetWorldBounds(Script::FunctionInterface* pInterface) const
{
	Falcon::Rectangle bounds = Falcon::Rectangle::Create(0, 0, 0, 0);
	if (m_pInstance->ComputeBounds(bounds))
	{
		if (m_pInstance->GetParent())
		{
			bounds = Falcon::TransformRectangle(m_pInstance->GetParent()->ComputeWorldTransform(), bounds);
		}
	}

	pInterface->PushReturnNumber(bounds.m_fLeft);
	pInterface->PushReturnNumber(bounds.m_fTop);
	pInterface->PushReturnNumber(bounds.m_fRight);
	pInterface->PushReturnNumber(bounds.m_fBottom);
}

void ScriptUIInstance::GetWorldPosition(Script::FunctionInterface* pInterface) const
{
	Vector2D const vWorldPosition = m_pInstance->ComputeWorldPosition();

	pInterface->PushReturnNumber(vWorldPosition.X);
	pInterface->PushReturnNumber(vWorldPosition.Y);
}

Float ScriptUIInstance::GetWorldDepth3D() const
{
	return m_pInstance->GetWorldDepth3D();
}

void ScriptUIInstance::LocalToWorld(Script::FunctionInterface* pInterface) const
{
	Double fX = 0.0;
	(void)pInterface->GetNumber(1, fX);
	Double fY = 0.0;
	(void)pInterface->GetNumber(2, fY);

	auto const v = Matrix2x3::TransformPosition(m_pInstance->ComputeWorldTransform(), Vector2D((Float)fX, (Float)fY));

	pInterface->PushReturnNumber(v.X);
	pInterface->PushReturnNumber(v.Y);
}

Bool ScriptUIInstance::HasParent() const
{
	return (nullptr != m_pInstance->GetParent());
}

Bool ScriptUIInstance::RemoveFromParent()
{
	if (m_pInstance->GetParent())
	{
		m_pInstance->GetParent()->RemoveChildAtDepth(m_pInstance->GetDepthInParent());
		return true;
	}
	else
	{
		return false;
	}
}

void ScriptUIInstance::SetAdditiveBlend(Bool bAdditiveBlend)
{
	m_pInstance->SetBlendingFactor(bAdditiveBlend ? 1.0f : 0.0f);
}

void ScriptUIInstance::SetAlpha(Float f)
{
	m_pInstance->SetAlpha(f);
}

void ScriptUIInstance::SetClipDepth(UInt16 uDepth)
{
	m_pInstance->SetClipDepth(uDepth);
}

void ScriptUIInstance::SetScissorClip(Bool bEnable)
{
	m_pInstance->SetScissorClip(bEnable);
}

void ScriptUIInstance::SetColorTransform(Float fMulR, Float fMulG, Float fMulB, UInt8 uAddR, UInt8 uAddG, UInt8 uAddB)
{
	Falcon::ColorTransform color;
	color.m_fMulR = fMulR;
	color.m_fMulG = fMulG;
	color.m_fMulB = fMulB;
	color.m_AddR = uAddR;
	color.m_AddG = uAddG;
	color.m_AddB = uAddB;
	m_pInstance->SetColorTransform(color);
}

void ScriptUIInstance::SetDebugName(const String& sName)
{
#if !SEOUL_SHIP
	m_pInstance->SetDebugName(sName);
#endif // /#if !SEOUL_SHIP
}

void ScriptUIInstance::SetIgnoreDepthProjection(Bool b)
{
	m_pInstance->SetIgnoreDepthProjection(b);
}

void ScriptUIInstance::SetName(HString name)
{
	m_pInstance->SetName(name);
}

void ScriptUIInstance::SetPosition(Float fX, Float fY)
{
	m_pInstance->SetPosition(fX, fY);
}

void ScriptUIInstance::SetPositionX(Float f)
{
	m_pInstance->SetPositionX(f);
}

void ScriptUIInstance::SetPositionY(Float f)
{
	m_pInstance->SetPositionY(f);
}

void ScriptUIInstance::SetRotation(Float fAngleInDegrees)
{
	m_pInstance->SetRotationInDegrees(fAngleInDegrees);
}

void ScriptUIInstance::SetScale(Float fX, Float fY)
{
	m_pInstance->SetScale(fX, fY);
}

void ScriptUIInstance::SetScaleX(Float f)
{
	m_pInstance->SetScaleX(f);
}

void ScriptUIInstance::SetScaleY(Float f)
{
	m_pInstance->SetScaleY(f);
}

void ScriptUIInstance::SetVisible(Bool bVisible)
{
	m_pInstance->SetVisible(bVisible);
}

void ScriptUIInstance::SetWorldPosition(Float fX, Float fY)
{
	m_pInstance->SetWorldPosition(fX, fY);
}

void ScriptUIInstance::WorldToLocal(Script::FunctionInterface* pInterface) const
{
	Double fX = 0.0;
	(void)pInterface->GetNumber(1, fX);
	Double fY = 0.0;
	(void)pInterface->GetNumber(2, fY);

	auto const v = Matrix2x3::TransformPosition(m_pInstance->ComputeWorldTransform().Inverse(), Vector2D((Float)fX, (Float)fY));

	pInterface->PushReturnNumber(v.X);
	pInterface->PushReturnNumber(v.Y);
}

void ScriptUIInstance::GetLocalMousePosition(Script::FunctionInterface* pInterface) const
{
	auto pOwner(GetOwner());
	if (!pOwner)
	{
		pInterface->RaiseError(-1, "null ScriptUIMovie owner.");
		return;
	}

	Vector2D v = ((UI::Movie const*)pOwner)->GetMousePositionInWorld(UI::Manager::Get()->GetMousePosition());
	v = Matrix2x3::TransformPosition(m_pInstance->ComputeWorldTransform().Inverse(), v);

	pInterface->PushReturnNumber(v.X);
	pInterface->PushReturnNumber(v.Y);
}

static Bool ExactIntersectsUtil(
	const Falcon::Instance& instance,
	const Matrix2x3& mParent,
	Float fWorldX, 
	Float fWorldY)
{
	if (instance.GetType() != Falcon::InstanceType::kMovieClip)
	{
		return instance.ExactHitTest(mParent, fWorldX, fWorldY, true);
	}
	else
	{
		auto const& r = static_cast<Falcon::MovieClipInstance const&>(instance);
		auto const mWorld(mParent * instance.GetTransform());
		auto const uChildren(r.GetChildCount());
		for (UInt32 i = 0u; i < uChildren; ++i)
		{
			SharedPtr<Falcon::Instance> pChild;
			SEOUL_VERIFY(r.GetChildAt(i, pChild));
			if (ExactIntersectsUtil(*pChild, mWorld, fWorldX, fWorldY))
			{
				return true;
			}
		}
	}

	return false;
}

static Bool IntersectsUtil(
	const Falcon::Instance& instance,
	const Matrix2x3& mParent,
	Float fWorldX,
	Float fWorldY)
{
	if (instance.GetType() != Falcon::InstanceType::kMovieClip)
	{
		return instance.HitTest(mParent, fWorldX, fWorldY, true);
	}
	else
	{
		auto const& r = static_cast<Falcon::MovieClipInstance const&>(instance);
		auto const mWorld(mParent * instance.GetTransform());
		auto const uChildren(r.GetChildCount());
		for (UInt32 i = 0u; i < uChildren; ++i)
		{
			SharedPtr<Falcon::Instance> pChild;
			SEOUL_VERIFY(r.GetChildAt(i, pChild));
			if (IntersectsUtil(*pChild, mWorld, fWorldX, fWorldY))
			{
				return true;
			}
		}
	}

	return false;
}

Bool ScriptUIInstance::Intersects(Float fWorldX, Float fWorldY, Bool bExactHitTest) const
{
	auto const mParent(m_pInstance->GetParent() ? m_pInstance->GetParent()->ComputeWorldTransform() : Matrix2x3::Identity());
	if (bExactHitTest)
	{
		return ExactIntersectsUtil(*m_pInstance, mParent, fWorldX, fWorldY);
	}
	else
	{
		return IntersectsUtil(*m_pInstance, mParent, fWorldX, fWorldY);
	}
}

} // namespace Seoul
