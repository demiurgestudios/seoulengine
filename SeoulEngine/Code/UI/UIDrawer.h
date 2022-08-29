/**
 * \file UIDrawer.h
 * \brief Component of UI::Renderer, handles direct interactions with
 * the render backend.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_DRAWER_H
#define UI_DRAWER_H

#include "Delegate.h"
#include "Effect.h"
#include "FalconPackerTree2D.h"
#include "FalconRenderCommand.h"
#include "FalconTextureCacheSettings.h"
#include "FilePath.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
#include "SharedPtr.h"
#include "UIDrawerSettings.h"
#include "Vector.h"
#include "Viewport.h"
namespace Seoul { class IndexBuffer; }
namespace Seoul { class VertexBuffer; }
namespace Seoul { class VertexFormat; }
namespace Seoul { class RenderCommandStreamBuilder; }
namespace Seoul { class RenderPass; }
namespace Seoul { namespace Falcon { class Texture; } }
namespace Seoul { namespace Falcon { class TextureCache; } }
namespace Seoul { namespace Falcon { namespace Render { class Drawer; } } }
namespace Seoul { namespace Falcon { namespace Render { class Features; } } }
namespace Seoul { struct StandardVertex2D; }

namespace Seoul::UI
{

/**
 * Similar to Falcon::Render::State(), with
 * additional data for UIDrawer.
 */
class DrawerState SEOUL_SEALED
{
public:
	DrawerState(Falcon::Render::State& rState);
	~DrawerState();

	Falcon::Render::State& m_rState;
	Vector<Delegate<void(RenderPass&, RenderCommandStreamBuilder&)>, MemoryBudgets::Falcon> m_vCustomDraws;
	Vector<Vector4D, MemoryBudgets::Falcon> m_vVector4Ds;
	Vector<Viewport, MemoryBudgets::Falcon> m_vViewports;

	// Reset the buffered portions of state to its default.
	void Reset();
};

/**
 * Drawer backend, component of UI::Renderer, directly interfaces with the render backend.
 */
class Drawer SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(Drawer);

	Drawer(const DrawerSettings& settings = DrawerSettings());
	~Drawer();

	// Entry points for Falcon's drawer.
	void DrawTriangleListRI(
		const SharedPtr<Falcon::Texture>& pColorTex,
		const SharedPtr<Falcon::Texture>& pDetailTex,
		UInt16 const* pIndices,
		UInt32 uIndexCount,
		Float32 const* pDepths3D,
		StandardVertex2D const* pVertices,
		UInt32 uVertexCount,
		const Falcon::Render::Features& features);
	// /Entry points for Falcon's drawer.

	void ClearPack()
	{
		m_vPackOps.Clear();
		m_tPackNodes.Clear();
	}

	void Pack(
		Falcon::PackerTree2D::NodeID nodeID,
		const SharedPtr<Falcon::Texture>& pSource,
		const Rectangle2DInt& source,
		const Point2DInt& destination)
	{
		PackOp op;
		op.m_NodeID = nodeID;
		op.m_Destination = destination;
		op.m_pSource = pSource;
		op.m_SourceRectangle = source;
		m_vPackOps.PushBack(op);
	}

	void ProcessDraw(
		DrawerState& rState,
		CheckedPtr<RenderCommandStreamBuilder> pBuilder,
		CheckedPtr<RenderPass> pPass);

	void UnPack(Falcon::PackerTree2D::NodeID nodeID);

#if SEOUL_ENABLE_CHEATS
	Falcon::Render::Mode::Enum GetRenderMode() const { return m_eRendererMode; }
	void SetRenderMode(Falcon::Render::Mode::Enum eMode) { m_eRendererMode = eMode; }

	Bool GetDebugEnableOverfillOptimizer() const;
	void SetDebugEnableOverfillOptimizer(Bool bEnable);
#endif // /#if SEOUL_ENABLE_CHEATS

	class EffectUtil SEOUL_SEALED
	{
	public:
		EffectUtil(const EffectContentHandle& h);
		~EffectUtil();

		Bool Acquire(RenderCommandStreamBuilder& r);
		Bool FeatureLocked() const { return m_bFeatureLocked; }
		HString GetActiveTechnique() const { return m_ActiveTechnique; }
		void Release();
		void SetActiveTechnique(Bool bFeatureLocked = false, HString name = HString());
		void SetColorTexture(const TextureContentHandle& hTexture);
		void SetDetailTexture(const TextureContentHandle& hTexture);
		void SetVector4DParameter(HString name, const Vector4D& v, Bool bCommit = false);

		// Developer debugging only.
#if !SEOUL_SHIP
		const SharedPtr<Effect>& GetAcquired() const { return m_pAcquired; }
#endif

	private:
		EffectContentHandle m_hEffect;
		SharedPtr<Effect> m_pAcquired;
		HString m_ActiveTechnique;
		EffectPass m_Pass;
		CheckedPtr< RenderCommandStreamBuilder> m_p;
		Bool m_bHasColorTexture{};
		Bool m_bHasDetailTexture{};
		Bool m_bFeatureLocked{};

		SEOUL_DISABLE_COPY(EffectUtil);
	}; // class EffectUtil

	// Array of techniques for each extended blend mode type.
	typedef FixedArray<HString, Falcon::Render::Feature::EXTENDED_COUNT> ExtendedBlendModeTechniques;

private:
	ExtendedBlendModeTechniques const m_aExtendedBlendModeTechniques;
	HString GetExtendedBlendModeTechniqueName(const Falcon::Render::Features& features) const;

	void InternalBeginCustomDraw();
	void InternalEndCustomDraw();

#if SEOUL_ENABLE_CHEATS
	void InternalBeginInputVisualizationMode();
	void InternalEndInputVisualizationMode();
#endif // /#if SEOUL_ENABLE_CHEATS

	void InternalBeginPlanarShadows();
	void InternalCommitActiveViewport(const Viewport& activeViewport);
	void InternalCommitViewProjection(const Vector4D& vViewProjection);
	void InternalCheckPackNodes();
	void InternalEndPlanarShadows();
	void InternalPerformDraw();
	void InternalProcessPackOps();
	void InternalSetupStateTechnique(
		const Falcon::Render::Features& features);
	void InternalSetupEffectTechnique(
		const Falcon::Render::Features& features,
		const SharedPtr<Falcon::Texture>& pColorTex,
		Bool b3D);
	void InternalSetupVertexFormat(Bool b3D);
	Viewport InternalWorldToScissorViewport(const Falcon::Rectangle& world) const;

	DrawerSettings const m_Settings;
	ScopedPtr<Falcon::Render::Drawer> m_pDrawer;
	EffectUtil m_StateEffect;
	EffectUtil m_RenderEffect;
	EffectUtil m_PackEffect;
	CheckedPtr<EffectUtil> m_pDrawEffect;

	SharedPtr<IndexBuffer> m_pIndices;
	SharedPtr<VertexBuffer> m_pVertices;
	SharedPtr<VertexBuffer> m_pDepths3D;
	SharedPtr<VertexFormat> m_pVertexFormat2D;
	SharedPtr<VertexFormat> m_pVertexFormat3D;
	SharedPtr<VertexFormat> m_pActiveVertexFormat;

	Bool m_bTwoPassShadows;
	Bool m_bInTwoPassShadowRender;

	struct PackOp SEOUL_SEALED
	{
		PackOp()
			: m_Destination()
			, m_pSource()
			, m_SourceRectangle()
			, m_NodeID(0)
		{
		}

		Point2DInt m_Destination;
		SharedPtr<Falcon::Texture> m_pSource;
		Rectangle2DInt m_SourceRectangle;
		Falcon::PackerTree2D::NodeID m_NodeID;
	}; // struct PackOp
	typedef Vector<PackOp, MemoryBudgets::Falcon> PackOps;
	PackOps m_vPackOps;
	typedef Vector<SharedPtr<BaseTexture>, MemoryBudgets::Falcon> PackAcquired;
	PackAcquired m_vPackAcquired;
	typedef HashTable<Falcon::PackerTree2D::NodeID, PackOp, MemoryBudgets::Falcon> PackNodes;
	PackNodes m_tPackNodes;
	Atomic32Type m_LastPackTargetReset;

	CheckedPtr<DrawerState> m_pState;
	CheckedPtr<RenderCommandStreamBuilder> m_pBuilder;
	CheckedPtr<RenderPass> m_pPass;
	Falcon::TextureReference m_SolidFill;

#if SEOUL_ENABLE_CHEATS
	Falcon::Render::Mode::Enum m_eRendererMode;
#endif // /#if SEOUL_ENABLE_CHEATS

#if !SEOUL_SHIP
	void ValidateEffects();
	Bool m_bValidated = false;
#endif

	SEOUL_DISABLE_COPY(Drawer);
}; // class UI::Drawer

} // namespace Seoul::UI

#endif // include guard
