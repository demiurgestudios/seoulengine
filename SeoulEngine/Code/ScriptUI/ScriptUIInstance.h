/**
 * \file ScriptUIInstance.h
 * \brief Script binding around Falcon::Instance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_UI_INSTANCE_H
#define SCRIPT_UI_INSTANCE_H

#include "ReflectionDeclare.h"
#include "SeoulHString.h"
#include "UIMovieHandle.h"
namespace Seoul { namespace Falcon { class Instance; } }
namespace Seoul { namespace Script { class FunctionInterface; } }
namespace Seoul { class ScriptUIMovie; }

namespace Seoul
{

// Utility - must be cast consistently so the pointer value is guaranteed to
// always be the same.
static inline void* ToScriptNativeId(Falcon::Instance const* p)
{
	return (void*)p;
}
template <typename T>
static inline void* ToScriptNativeId(const SharedPtr<T>& p)
{
	return ToScriptNativeId(p.GetPtr());
}

class ScriptUIInstance
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(ScriptUIInstance);

#if !SEOUL_SHIP
	static void DebugLogInstanceCountsPerMovie();
#endif // /#if !SEOUL_SHIP

	// Part of gradual root movie release - should be called with (false)
	// once per frame, and then with (true) on shutdown, after all script
	// VMs have been destroyed.
	static void FreeRoots(Bool bFlushAll);

	ScriptUIInstance();
	virtual void Construct(
		const SharedPtr<Falcon::Instance>& pInstance,
		const ScriptUIMovie& owner);
	virtual ~ScriptUIInstance();

	void Clone(Script::FunctionInterface* pInterface) const;

	void AddMotion(Script::FunctionInterface* pInterface);
	void CancelMotion(Int iIdentifier);

	void AddTween(Script::FunctionInterface* pInterface);
	void AddTweenCurve(Script::FunctionInterface* pInterface);
	void CancelTween(Int iIdentifier);

	Bool GetAdditiveBlend() const;
	Float GetAlpha() const;
	void GetBoundsIn(Script::FunctionInterface* pInterface) const;
	void GetColorTransform(Script::FunctionInterface* pInterface) const;
	void GetBounds(Script::FunctionInterface* pInterface) const;
	virtual HString GetClassName() const;
	UInt16 GetClipDepth() const;
	UInt16 GetDepthInParent() const;
	const SharedPtr<Falcon::Instance>& GetInstance() const;
	Bool GetIgnoreDepthProjection() const;
	String GetDebugName() const;
	void GetLocalBounds(Script::FunctionInterface* pInterface) const;
	HString GetName() const;
	void GetFullName(Script::FunctionInterface* pInterface) const;
	CheckedPtr<ScriptUIMovie> GetOwner() const;
	void GetParent(Script::FunctionInterface* pInterface) const;
	void GetPosition(Script::FunctionInterface* pInterface) const;
	Float GetPositionX() const;
	Float GetPositionY() const;
	Float GetRotation() const;
	void GetScale(Script::FunctionInterface* pInterface) const;
	Float GetScaleX() const;
	Float GetScaleY() const;
	Bool GetScissorClip() const;
	Bool GetVisible() const;
	Bool GetVisibleToRoot() const;
	void GetWorldBounds(Script::FunctionInterface* pInterface) const;
	void GetWorldPosition(Script::FunctionInterface* pInterface) const;
	Float GetWorldDepth3D() const;

	void LocalToWorld(Script::FunctionInterface* pInterface) const;
	Bool HasParent() const;
	Bool RemoveFromParent();

	void SetAdditiveBlend(Bool bAdditiveBlend);
	void SetAlpha(Float f);
	void SetClipDepth(UInt16 uDepth);
	void SetColorTransform(Float fMulR, Float fMulG, Float fMulB, UInt8 uAddR, UInt8 uAddG, UInt8 uAddB);
	void SetDebugName(const String& sName);
	void SetIgnoreDepthProjection(Bool b);
	void SetName(HString name);
	void SetPosition(Float fX, Float fY);
	void SetPositionX(Float f);
	void SetPositionY(Float f);
	void SetRotation(Float fAngleInDegrees);
	void SetScale(Float fX, Float fY);
	void SetScaleX(Float fX);
	void SetScaleY(Float fY);
	void SetScissorClip(Bool bEnable);
	void SetVisible(Bool bVisible);
	void SetWorldPosition(Float fX, Float fY);

	void WorldToLocal(Script::FunctionInterface* pInterface) const;

	void GetLocalMousePosition(Script::FunctionInterface* pInterface) const;
	Bool Intersects(Float fWorldX, Float fWorldY, Bool bExactHitTest) const;
	
protected:
	SharedPtr<Falcon::Instance> m_pInstance;
	UI::MovieHandle m_hOwner;

private:
	SEOUL_DISABLE_COPY(ScriptUIInstance);

#if !SEOUL_SHIP
	HString m_DebugMovieTypeName;
#endif // /#if !SEOUL_SHIP
}; // class ScriptUIInstance

} // namespace Seoul

#endif // include guard
