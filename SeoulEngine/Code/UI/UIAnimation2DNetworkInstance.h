/**
 * \file UIAnimation2DNetworkInstance.h
 * \brief SeoulEngine subclass/extension of Falcon::Instance for Animation2D playback.
 *
 * UI::Animation2DNetworkInstance binds the SeoulEngine Animation2D system into the Falcon scene graph.
 * Animation2Ds are rendered with the Falcon renderer and can be freely layered with
 * Falcon scene elements.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_ANIMATION2D_NETWORK_INSTANCE_H
#define UI_ANIMATION2D_NETWORK_INSTANCE_H

#include "FalconClipper.h"
#include "FalconInstance.h"
#include "FalconRenderFeature.h"
#include "FilePath.h"
#include "HashFunctions.h"
#include "HashTable.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
#include "SharedPtr.h"
#include "StandardVertex2D.h"
#include "UIMovieHandle.h"
#include "UnsafeBuffer.h"
namespace Seoul { namespace Animation { struct NetworkInstancePerformanceData; } }
namespace Seoul { namespace Animation2D { class Attachment; } }
namespace Seoul { namespace Animation2D { class MeshAttachment; } }
namespace Seoul { namespace Animation2D { class NetworkInstance; } }
namespace Seoul { namespace UI { class BoneAttachments; } }
namespace Seoul { namespace UI { class Movie; } }

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul::UI
{

/** Custom subclass of Falcon::Instance, implements binding of Animation2DInstances into the Falcon graph. */
class Animation2DNetworkInstance SEOUL_SEALED : public Falcon::Instance
{
public:
	typedef UnsafeBuffer<UInt16, MemoryBudgets::Falcon> Indices;
	typedef UnsafeBuffer<Falcon::Clipper::UtilityVertex, MemoryBudgets::Falcon> ClippedVertices;
	typedef Falcon::Clipper::MeshClipCacheT<Falcon::Clipper::UtilityVertex> MeshClipCache;
	typedef HashTable<FilePath, FilePath, MemoryBudgets::UIRuntime> Palette;
	typedef UnsafeBuffer<StandardVertex2D, MemoryBudgets::Falcon> Vertices;

	SEOUL_REFLECTION_POLYMORPHIC(Animation2DNetworkInstance);

	Animation2DNetworkInstance(const Movie& owner, const SharedPtr<Animation2D::NetworkInstance>& pNetworkInstance);
	~Animation2DNetworkInstance();

	virtual Instance* Clone(Falcon::AddInterface& rInterface) const SEOUL_OVERRIDE;

	// Falcon::Instance overrides.
	virtual bool ComputeLocalBounds(Falcon::Rectangle& rBounds) SEOUL_OVERRIDE;

	virtual void Pose(
		Falcon::Render::Poser& rPoser,
		const Matrix2x3& mParent,
		const Falcon::ColorTransformWithAlpha& cxParent) SEOUL_OVERRIDE;

#if SEOUL_ENABLE_CHEATS
	virtual void PoseInputVisualization(
		Falcon::Render::Poser& rPoser,
		const Matrix2x3& mParent,
		RGBA color) SEOUL_OVERRIDE;
#endif

	virtual void Draw(
		Falcon::Render::Drawer& rDrawer,
		const Falcon::Rectangle& worldBoundsPreClip,
		const Matrix2x3& mWorld,
		const Falcon::ColorTransformWithAlpha& cxWorld,
		const Falcon::TextureReference& textureReference,
		Int32 iSubInstanceId) SEOUL_OVERRIDE;

	virtual Bool HitTest(
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY,
		Bool bIgnoreVisibility = false) const SEOUL_OVERRIDE;

	virtual Falcon::InstanceType GetType() const SEOUL_OVERRIDE;

	virtual Bool CastShadow() const SEOUL_OVERRIDE
	{
		return m_bShadowCast;
	}

	virtual Vector2D GetShadowPlaneWorldPosition() const SEOUL_OVERRIDE
	{
		return ComputeWorldPosition() + m_vShadowOffset;
	}
	// /Falcon::Instance overrides.

	// Accessors related to updating shadow state.
	void SetCastShadow(Bool b) { m_bShadowCast = b; }
	void SetShadowOffset(const Vector2D& v) { m_vShadowOffset = v; }

	// Accessors to the underlying network.
	Float GetCurrentMaxTime() const;

	// returns true if the animation event was found after the current animation time, and sets the time to event
	// returns false if the animation event was not found
	Bool GetTimeToEvent(HString sEventName, Float& fTimeToEvent) const;

	void SetCondition(HString name, Bool bValue);
	void SetParameter(HString name, Float fValue);
	void TriggerTransition(HString name);
	void AddTimestepOffset(Float fTimestepOffset);
	// /

	// Custom tick functions, so Animation2D can run at 60 fps.
	void Tick(Float fDeltaTimeInSeconds);

	void AddBoneAttachment(Int16 iBoneIndex, SharedPtr<Falcon::Instance> pAttachment);

	String GetActiveStatePath(UInt32& ruId) const;
	Int16 GetBoneIndex(HString id) const;
	Vector2D GetBonePosition(Int16 boneIndex) const;
	Vector2D GetLocalBonePosition(Int16 boneIndex) const;
	Vector2D GetLocalBoneScale(Int16 boneIndex) const;
	HString GetActivePalette() const { return m_ActivePalette; }
	HString GetActiveSkin() const { return m_ActiveSkin; }
	Vector2D GetWorldSpaceBonePosition(Int16 boneIndex) const;
	Matrix2x3 GetWorldSpaceBoneTransform(Int16 boneIndex) const;

	void AllDonePlaying(Bool& rbDone, Bool& rbLooping) const;
	Bool IsInStateTransition() const;
	Bool IsReady() const;

	/**
	 * Update the active skin of the network.
	 *
	 * The "skin" is a Spine concept. Skins can use different skinned mesh
	 * attachments or even entirely different attachments on the same
	 * rigged skeleton with a shared animation set.
	 *
	 * Skins are a more flexible but also more brittle and higher maintenance
	 * overhead (for artists).
	 */
	void SetActiveSkin(HString skin)
	{
		m_ActiveSkin = skin;
	}

	/**
	 * Update the active palette of the network.
	 *
	 * The "palette" is a SeoulEngine concept. Palettes allow
	 * an exact set of images to be swapped in for the base set of
	 * an animation network.
	 *
	 * Swapping is done by path, using directories that are siblings
	 * to the base "images" directory of the network.
	 *
	 * The palette name should correspond exactly to the alternative
	 * subfolder you want to use for rendering.
	 */
	void SetActivePalette(HString palette)
	{
		if (palette != m_ActivePalette)
		{
			m_ActivePalette = palette;
			m_tPalette.Clear();
		}
	}

	/** @return True if variable time step, false otherwise. */
	Bool GetVariableTimeStep() const
	{
		return m_bVariableTimeStep;
	}

	/**
	 * Set whether this AnimationNetwork should be updated with
	 * a variable frame time. This is not used internally
	 * by the network - the ticking logic in UI::Movie
	 * is expected to respect this flag.
	 */
	void SetVariableTimeStep(Bool b)
	{
		m_bVariableTimeStep = b;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(Animation2DNetworkInstance);

	/**
	 * Structure used to cache clipped results. This includes both the stable
	 * utility clipped structure (that is used to remap skinned vertices into
	 * their clipped output without reclipping) was well as the storage buffers
	 * used for the fully clipped result.
	 */
	struct ClippedEntry SEOUL_SEALED
	{
		ClippedEntry();
		~ClippedEntry();

		Falcon::Rectangle m_ClipRectangle;
		Indices m_vIndices;
		ClippedVertices m_vRemap;
		Vertices m_vVertices;
		UInt32 m_uClippedCount;
	}; // struct ClippedEntry

	/**
	 * Structure used to cache posing results, as well as a list of fully clipped
	 * results. Used for meshes and linked meshes.
	 */
	struct PosedEntry SEOUL_SEALED
	{
		typedef Vector<ClippedEntry*, MemoryBudgets::UIRuntime> Clipped;

		PosedEntry();
		~PosedEntry();

		// TODO: We use a vector here instead of a table,
		// as the worst case size of this list is 5 (the number of mipmap
		// levels), so an O(n) lookup is typically faster than the hash
		// table lookup, and it simplifies the implementation.

		Clipped m_vClipped;
		Vertices m_vVertices;
		Falcon::Rectangle m_PositionBounds;
		Falcon::Rectangle m_TexCoordsBounds;
		UInt32 m_uPoseCount;
		Falcon::Render::Feature::Enum m_eRenderingFeature;
		Float m_fEffectiveWidth;
		Float m_fEffectiveHeight;
	}; // struct PosedEntry
	typedef Vector<Float, MemoryBudgets::Animation2D> DeformData;
	typedef HashTable<Animation2D::Attachment const*, PosedEntry*, MemoryBudgets::UIRuntime> Posed;
	typedef Vector<Matrix2x3, MemoryBudgets::Animation2D> SkinningPalette;

	const ClippedEntry& Clip(const Animation2D::MeshAttachment& mesh, const Falcon::TextureReference& textureReference, PosedEntry& r);
	void CloneTo(Falcon::AddInterface& rInterface, Animation2DNetworkInstance& rClone) const;
	ClippedEntry& ResolveClippedEntry(PosedEntry& r, const Falcon::Rectangle& clipRectangle);
	FilePath ResolveFilePath(FilePath filePath);
	PosedEntry& ResolvePosedEntry(const SharedPtr<Animation2D::Attachment>& pAttachment);
	void Skin(
		CheckedPtr<DeformData> pDeform,
		RGBA color,
		const Animation2D::MeshAttachment& mesh,
		const SkinningPalette& vPalette,
		PosedEntry& rPosedEntry) const;

	friend class Movie;
	MovieHandle m_hOwner;
	SharedPtr<Animation2D::NetworkInstance> m_pNetworkInstance;
	ScopedPtr<BoneAttachments> m_pAttachments;
	Palette m_tPalette;
	MeshClipCache* m_pMeshClipCache;
	Posed m_tPosed;
	Vector2D m_vShadowOffset;
	Falcon::Rectangle m_PositionBounds;
	UInt32 m_uTickCount;
	HString m_ActivePalette;
	HString m_ActiveSkin;
	Bool m_bShadowCast;
	Bool m_bVariableTimeStep;

#if SEOUL_HOT_LOADING
	Atomic32Type m_LoadDataCount;
	Atomic32Type m_LoadNetworkCount;
#endif // /#if SEOUL_HOT_LOADING

#if !SEOUL_SHIP
	Bool const m_bCheckSourceFilesForPalettes;
#endif // /#if !SEOUL_SHIP

	SEOUL_DISABLE_COPY(Animation2DNetworkInstance);
}; // class UI::Animation2DNetworkInstance

} // namespace Seoul::UI

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // include guard
