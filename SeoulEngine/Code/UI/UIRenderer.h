/**
 * \file UIRenderer.h
 * \brief Specialization of Falcon::RendererInterface
 * and combination of the various bits
 * necessary to render Falcon graph data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include "Delegate.h"
#include "FalconRenderMode.h"
#include "FalconRendererInterface.h"
#include "Texture.h"
#include "UIDrawerSettings.h"
#include "Viewport.h"
namespace Seoul { class Camera; }
namespace Seoul { namespace Content { struct ChangeEvent; } }
namespace Seoul { class RenderCommandStreamBuilder; }
namespace Seoul { class RenderPass; }
namespace Seoul { class RenderSurface2D; }
namespace Seoul { namespace UI { class Drawer; } }
namespace Seoul { namespace UI { class DrawerState; } }
namespace Seoul { namespace UI { class FxRenderer; } }
namespace Seoul { namespace UI { class Movie; } }
namespace Seoul { namespace Falcon { class HitTester; } }
namespace Seoul { namespace Falcon { namespace Render { class BatchOptimizer; } } }
namespace Seoul { namespace Falcon { namespace Render { class OcclusionOptimizer; } } }
namespace Seoul { namespace Falcon { namespace Render { class State; } } }

namespace Seoul::UI
{

/** Psuedo world distance used by the FX camera. */
static const Float kfUIRendererFxCameraWorldDistance = 100.0f;

/**
 * Rendering backend for SeoulEngine's integration of the Falcon project into the UI project.
 */
class Renderer SEOUL_SEALED : public Falcon::RendererInterface
{
public:
	SEOUL_DELEGATE_TARGET(Renderer);

	typedef Vector<Viewport, MemoryBudgets::UIRendering> ViewportStack;

	Renderer(const DrawerSettings& settings = DrawerSettings());
	~Renderer();

	void BeginFrame(const Viewport& initialViewport);
	void BeginMovie(
		Movie* pMovie,
		const Falcon::Rectangle& stageBounds);
	void BeginMovieFxOnly(Movie* pMovie, FxRenderer& rRenderer);
	void EndMovie(Bool bFlushDeferred = false);
	void EndMovieFxOnly(FxRenderer& rRenderer);
	void EndFrame(
		RenderCommandStreamBuilder& rBuilder,
		RenderPass* pPass);

	Falcon::HitTester GetHitTester(
		const Movie& movie,
		const Falcon::Rectangle& stageBounds,
		const Viewport& activeViewport) const;

	void PoseRoot(const SharedPtr<Falcon::MovieClipInstance>& pRoot);
	void PoseCustomDraw(const Delegate<void(RenderPass&, RenderCommandStreamBuilder&)>& callback);

#if SEOUL_ENABLE_CHEATS
	typedef HashSet< SharedPtr<Falcon::MovieClipInstance>, MemoryBudgets::UIData > InputWhitelist;
	void PoseInputVisualization(
		const InputWhitelist& inputWhitelist,
		UInt8 uInputMask,
		const SharedPtr<Falcon::MovieClipInstance>& pRoot);
	void PoseInputVisualizationWithRestrictionRectangle(
		const Rectangle2D& rect,
		const InputWhitelist& inputWhitelist,
		UInt8 uInputMask,
		const SharedPtr<Falcon::MovieClipInstance>& pRoot);
#endif // /#if SEOUL_ENABLE_CHEATS

	Float ComputeDepth3D(Float fY) const;

	/**
	 * Custom Camera that maps a fixed psuedo 3D world space for rendering Fx
	 * as part of the UI system. Used to map 3D world space Fx into UI space.
	 */
	Camera& GetCamera() const
	{
		return *m_pCamera;
	}

	/** @return The SharedPtr<> access around Camera, for sharing. */
	const SharedPtr<Camera>& GetCameraPtr() const
	{
		return m_pCamera;
	}

	/** @return The current renderer's Fx camera offset. */
	const Vector3D& GetFxCameraOffset() const
	{
		return m_vFxCameraOffset;
	}

	/** @return The current Fx camera zoom factor, as an inverse. */
	Float GetFxCameraInverseZoom() const
	{
		return m_fFxCameraInverseZoom;
	}

	Float GetPerspectiveFactor() const;
	Float GetPerspectiveFactorAdjustment() const;

	const Vector4D& GetViewProjectionTransform() const;

	/** Update the current renderer's Fx camera offset. */
	void SetFxCameraOffset(const Vector3D& vOffset)
	{
		m_vFxCameraOffset = vOffset;
	}

	/**
	 * Zoom the Fx camera in and out. Used for editor modes and special
	 * cases, not a general feature.
	 */
	void SetFxCameraInverseZoom(Float fInverseZoom)
	{
		m_fFxCameraInverseZoom = fInverseZoom;
	}

	void SetPerspectiveFactorAdjustment(Float f);
	void SetStage3DProjectionBounds(Float fTopY, Float fBottomY);

	// Viewport override functionality
	Viewport GetActiveViewport() const
	{
		return (m_vViewportStack.IsEmpty()
			? Viewport()
			: m_vViewportStack.Back());
	}

	void PopViewport();
	void PushViewport(const Viewport& viewport);
	// /Viewport override functionality

	/** @return True if a requested texture cache purge is still pending, false otherwise. */
	Bool IsTexturePurgePending() const
	{
		return (PurgeState::kInactive != m_ePendingPurge);
	}

	/** Request a purge of the texture cache. Typically used around hot loading and similar operations. */
	void PurgeTextureCache()
	{
		m_ePendingPurge = PurgeState::kPurgeTextures;
	}

	void SetMovieDependentState(Movie* pMovie, Viewport activeViewport, const Falcon::Rectangle& stageBounds);

	void UpdateTextureReplacement(FilePathRelativeFilename name, FilePath filePath);

#if SEOUL_ENABLE_CHEATS
	// Developer functionality, used for rendering input visualizatino hit rectangles.
	void BeginInputVisualizationMode();
	void EndInputVisualizationMode();

	Falcon::Render::Mode::Enum GetRenderMode() const;
	void SetRenderMode(Falcon::Render::Mode::Enum eMode);

	Bool GetDebugEnableBatchOptimizer() const { return m_bDebugEnableBatchOptimizer; }
	void SetDebugEnableBatchOptimizer(Bool bEnable) { m_bDebugEnableBatchOptimizer = bEnable; }

	Bool GetDebugEnableOcclusionOptimizer() const { return m_bDebugEnableOcclusionOptimizer; }
	void SetDebugEnableOcclusionOptimizer(Bool bEnable) { m_bDebugEnableOcclusionOptimizer = bEnable; }

	Bool GetDebugEnableOverfillOptimizer() const;
	void SetDebugEnableOverfillOptimizer(Bool bEnable);
#endif // /#if SEOUL_ENABLE_CHEATS

	void ConfigureStage3DSettings(HString name)
	{
		m_Stage3DSettings = name;
	}

	Bool ResolveTextureReference(
		Float fRenderThreshold,
		const FilePath& filePath,
		Falcon::TextureReference& rTextureReference);

	/**
	 * Begin a scope where the occlusion optimizer is temporarily suppressed.
	 *
	 * Occlusion optimization is only a win if UI is stacked such that
	 * UI above hides UI below. In situations where the client knows
	 * this will not be the case, the occlusion optimizer can be temporarily
	 * suppressed to avoid the CPU overhead of the optimizer itself.
	 */
	void BeginOcclusionOptimizerSuppress() { ++m_SuppressOcclusionOptimizer; }
	void EndOcclusionOptimizerSuppress() { --m_SuppressOcclusionOptimizer; }

	const Falcon::Render::State& GetRenderState() const
	{
		return *m_pState;
	}

private:
	void ApplyFxOnlyCamera();
	void ApplyStandardCamera();
	void ApplyCameraCommon(Float fZoom, const Vector3D& vOffset);

	// Falcon::RendererInterface overrides.
	virtual void ClearPack() SEOUL_OVERRIDE;
	virtual void Pack(
		Falcon::PackerTree2D::NodeID nodeID,
		const SharedPtr<Falcon::Texture>& pSource,
		const Rectangle2DInt& source,
		const Point2DInt& destination) SEOUL_OVERRIDE;

	virtual UInt32 GetRenderFrameCount() const SEOUL_OVERRIDE
	{
		return m_uRenderFrameCount;
	}
	virtual void ResolvePackerTexture(
		Falcon::TexturePacker& rPacker,
		SharedPtr<Falcon::Texture>& rp) SEOUL_OVERRIDE;
	virtual void ResolveTexture(
		FilePath filePath,
		SharedPtr<Falcon::Texture>& rp) SEOUL_OVERRIDE;
	virtual void ResolveTexture(
		UInt8 const* pData,
		UInt32 uDataWidth,
		UInt32 uDataHeight,
		UInt32 uStride,
		Bool bIsFullOccluder,
		SharedPtr<Falcon::Texture>& rp) SEOUL_OVERRIDE;
	virtual void UnPack(Falcon::PackerTree2D::NodeID nodeID) SEOUL_OVERRIDE;
	// /Falcon::RendererInterface overrides.

	void InternalCommitActiveViewport();
	void InternalHandlePendingPurge();

#if SEOUL_HOT_LOADING
	void OnFileChange(Content::ChangeEvent* pFileChangeEvent);
	void OnFileLoadComplete(FilePath filePath);
	typedef HashTable<FilePath, Bool, MemoryBudgets::UIRendering> HotLoadingTable;
	HotLoadingTable m_tHotLoading;
#endif // /#if SEOUL_HOT_LOADING

	DrawerSettings const m_Settings;
	ScopedPtr<Falcon::Render::Poser> m_pPoser;
	ScopedPtr<Falcon::Render::BatchOptimizer> m_pBatchOptimizer;
	ScopedPtr<Falcon::Render::OcclusionOptimizer> m_pOcclusionOptimizer;
	ScopedPtr<Drawer> m_pDrawer;
	ScopedPtr<Falcon::Render::State> m_pState;
	ScopedPtr<DrawerState> m_pDrawerState;
	SharedPtr<Camera> m_pCamera;
	Movie* m_pActiveMovie;
	ViewportStack m_vViewportStack;
	UInt32 m_uRenderFrameCount;
	HString m_Stage3DSettings;

	struct MovieState SEOUL_SEALED
	{
		MovieState()
			: m_WorldCullRectangle(Falcon::Rectangle::Max())
			, m_fWorldHeightToScreenHeight(1.0f)
			, m_fWorldWidthToScreenWidth(1.0f)
			, m_vViewProjectionTransform(Vector4D::Zero())
			, m_bHasState(false)
		{
		}

		Falcon::Rectangle m_WorldCullRectangle;
		Float m_fWorldHeightToScreenHeight;
		Float m_fWorldWidthToScreenWidth;
		Vector4D m_vViewProjectionTransform;
		Bool m_bHasState;
	}; // struct MovieState
	MovieState m_MovieState;
	Vector3D m_vFxCameraOffset;
	Float m_fFxCameraInverseZoom;
	Atomic32 m_SuppressOcclusionOptimizer;

	typedef Vector<TextureContentHandle, MemoryBudgets::UIRendering> TextureScratch;
	TextureScratch m_vTextureScratch;
	enum class PurgeState
	{
		kInactive,
		kWaitForReacquire,
		kPurgeTextures,
	};
	Atomic32Value<PurgeState> m_ePendingPurge;

#if SEOUL_ENABLE_CHEATS
	Bool m_bDebugEnableBatchOptimizer;
	Bool m_bDebugEnableOcclusionOptimizer;
#endif // /#if SEOUL_ENABLE_CHEATS

	SEOUL_DISABLE_COPY(Renderer);
}; // class UI::Renderer

} // namespace Seoul::UI

#endif // include guard
