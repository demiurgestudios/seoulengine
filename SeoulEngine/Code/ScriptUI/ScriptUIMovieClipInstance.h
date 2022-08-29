/**
 * \file ScriptUIMovieClipInstance.h
 * \brief Script binding around Falcon::MovieClipInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_UI_MOVIE_CLIP_INSTANCE_H
#define SCRIPT_UI_MOVIE_CLIP_INSTANCE_H

#include "ScriptUIInstance.h"
namespace Seoul { namespace Falcon { class LabelName; } }
namespace Seoul { namespace Falcon { class MovieClipInstance; } }

namespace Seoul
{

class ScriptUIMovieClipInstance SEOUL_SEALED : public ScriptUIInstance
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ScriptUIMovieClipInstance);

	// Very low-level - part of lifespan management. We don't want Lua
	// to GC a ScriptUIMovieClipInstance user data until m_pInstance.IsUnique()
	// is true. In this way, we don't let any script-only data garbage collect
	// until we know that the underlying native UI node will also be released.
	//
	// This hook binds into the Lua VM directly and is called with
	// the user data block of memory and a user data, which is the 1-based
	// index into the Reflection::Registry() of Type data. We use
	// the latter to determine if we're being called for a ScriptUIMovieClipInstance
	// or not.
	//
	// Note that we only do this starting with ScriptUIMovieClipInstance for two reasons:
	// - it is sufficient, since leaf types are not expected/do not support script-only data
	// - it is faster, since a ScriptUIMovieClipInstance is sealed and will never have
	//   child classes, so we can quickly determine in the pre-collection hook if we're
	//   checking a movie clip instance or not.
	typedef Int (*PreCollectionHook)(void* p, UInt32 uData);
	static PreCollectionHook ResolveLuaJITPreCollectionHook();

	ScriptUIMovieClipInstance();
	virtual void Construct(
		const SharedPtr<Falcon::Instance>& pInstance,
		const ScriptUIMovie& owner) SEOUL_OVERRIDE;
	~ScriptUIMovieClipInstance();

	Bool IsUniqueOwner() const;

	void AddChild(Script::FunctionInterface* pInterface);
#if SEOUL_WITH_ANIMATION_2D
	void AddChildAnimationNetwork(Script::FunctionInterface* pInterface);
#endif // /#if SEOUL_WITH_ANIMATION_2D
	void AddChildBitmap(Script::FunctionInterface* pInterface);
	void AddChildFacebookBitmap(Script::FunctionInterface* pInterface);
	void AddChildFx(Script::FunctionInterface* pInterface);
	void AddChildHitShape(Script::FunctionInterface* pInterface);
	void AddChildHitShapeFullScreen(Script::FunctionInterface* pInterface);
	void AddChildHitShapeWithMyBounds(Script::FunctionInterface* pInterface);
	void AddChildStage3D(Script::FunctionInterface* pInterface);

	void AddFullScreenClipper(Script::FunctionInterface* pInterface);
	Bool GetAbsorbOtherInput() const;
	UInt32 GetChildCount() const;
	virtual HString GetClassName() const SEOUL_OVERRIDE;
	Int32 GetCurrentFrame() const;
	void GetCurrentLabel(Script::FunctionInterface* pInterface) const;
	Float GetDepth3D() const;
	Bool GetExactHitTest() const;
	SharedPtr<Falcon::MovieClipInstance> GetInstance() const;
	void GetChildAt(Script::FunctionInterface* pInterface) const;
	void GetChildByNameFromSubTree(Script::FunctionInterface* pInterface) const;
	void GetChildByPath(Script::FunctionInterface* pInterface) const;
	Bool GetHitTestChildren() const;
	UInt8 GetHitTestChildrenMode() const;
	Bool GetHitTestSelf() const;
	UInt8 GetHitTestSelfMode() const;
	UInt32 GetTotalFrames() const;

	void GetHitTestableBounds(Script::FunctionInterface* pInterface) const;
	void GetHitTestableLocalBounds(Script::FunctionInterface* pInterface) const;
	void GetHitTestableWorldBounds(Script::FunctionInterface* pInterface) const;

	Bool GotoAndPlay(UInt32 uFrame);
	void GotoAndPlayByLabel(Script::FunctionInterface* pInterface);
	Bool GotoAndStop(UInt32 uFrame);
	void GotoAndStopByLabel(Script::FunctionInterface* pInterface);

	Bool IsPlaying() const;
	void HitTest(Script::FunctionInterface* pInterface) const;
	void IncreaseAllChildDepthByOne();
	void Play();

	void RemoveAllChildren();
	void RemoveChildAt(Script::FunctionInterface* pInterface);
	void RemoveChildByName(Script::FunctionInterface* pInterface);

	void RestoreTypicalDefault();

	void SetAbsorbOtherInput(Bool bAbsorbOtherInput);
	void SetDepth3D(Float f);
	void SetDepth3DFromYLoc(Float fYLoc);
	void SetInputActionDisabled(Bool bInputActionDisabled);
	void SetEnterFrame(Bool bEnableEnterFrame);
	void SetAutoCulling(Bool bEnableAutoCulling);
	void SetAutoDepth3D(Bool bEnableAutoDepth3D);
	void SetCastPlanarShadows(Bool bCastPlanarShadows);
	void SetDeferDrawing(Bool bDeferDrawing);
	void SetTickEvent(Bool bEnableTickEvent);
	void SetTickScaledEvent(Bool bEnableTickEvent);
	void SetExactHitTest(Bool bExactHitTest);
	void SetHitTestChildren(Bool bHitTestChildren);
	void SetHitTestChildrenMode(Int32 iMask);
	void SetHitTestSelf(Bool bHitTestSelf);
	void SetHitTestSelfMode(Int32 iMask);
	void SetReorderChildrenFromDepth3D(Bool b);
	void Stop();

	void SetChildrenVisible(Bool bVisible);

private:
	Bool DoGotoAndPlayByLabel(const Falcon::LabelName& label);
	Bool DoGotoAndStopByLabel(const Falcon::LabelName& label);
	void RefreshDynamicDepth();

	friend class ScriptUIMovie;
	UInt16 m_uDynamicDepth;
	Bool m_bEnableTickEvents;
	Bool m_bEnableTickScaledEvents;

	SEOUL_DISABLE_COPY(ScriptUIMovieClipInstance);
}; // class ScriptUIMovieClipInstance

} // namespace Seoul

#endif // include guard
