/**
 * \file UIRenderer.cpp
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

#include "Camera.h"
#include "ClearFlags.h"
#include "EffectManager.h"
#include "Engine.h"
#include "FalconClipper.h"
#include "FalconGlobalConfig.h"
#include "FalconMovieClipInstance.h"
#include "FalconRenderCommand.h"
#include "FalconRenderDrawer.h"
#include "FalconRenderBatchOptimizer.h"
#include "FalconRenderOcclusionOptimizer.h"
#include "FalconRenderState.h"
#include "FalconRenderPoser.h"
#include "FalconTextureCache.h"
#include "UIData.h"
#include "UIManager.h"
#include "UIMovie.h"
#include "UIRenderer.h"
#include "FileManager.h"
#include "InputManager.h"
#include "LocManager.h"
#include "ReflectionDataStoreTableUtil.h"
#include "RenderCommandStreamBuilder.h"
#include "RenderDevice.h"
#include "RenderSurface.h"
#include "TextureManager.h"
#include "UIDrawer.h"
#include "UIFxRenderer.h"
#include "UITexture.h"

#if SEOUL_HOT_LOADING
#include "ContentLoadManager.h"
#include "EventsManager.h"
#endif

namespace Seoul::UI
{

using namespace Falcon;
using namespace Falcon::Render;

Renderer::Renderer(const DrawerSettings& settings /*= UI::DrawerSettings()*/)
	: m_Settings(settings)
	, m_pPoser(SEOUL_NEW(MemoryBudgets::Falcon) Falcon::Render::Poser)
	, m_pBatchOptimizer(SEOUL_NEW(MemoryBudgets::Falcon) Falcon::Render::BatchOptimizer)
	, m_pOcclusionOptimizer(SEOUL_NEW(MemoryBudgets::Falcon) Falcon::Render::OcclusionOptimizer)
	, m_pDrawer(SEOUL_NEW(MemoryBudgets::UIRendering) Drawer(settings))
	, m_pState()
	, m_pDrawerState()
	, m_pCamera(SEOUL_NEW(MemoryBudgets::UIRuntime) Camera)
	, m_pActiveMovie(nullptr)
	, m_vViewportStack()
	, m_uRenderFrameCount(0)
	, m_Stage3DSettings()
	, m_MovieState()
	, m_vFxCameraOffset(Vector3D::Zero())
	, m_fFxCameraInverseZoom(1.0f)
	, m_SuppressOcclusionOptimizer(0)
	, m_ePendingPurge(PurgeState::kInactive)
#if SEOUL_ENABLE_CHEATS
	, m_bDebugEnableBatchOptimizer(true)
	, m_bDebugEnableOcclusionOptimizer(true)
#endif // /#if SEOUL_ENABLE_CHEATS
{
	// Configure the common Falcon state that will be shared across
	// Poser, Drawer, and Optimizers.
	Falcon::Render::StateSettings stateSettings;
	stateSettings.m_CacheSettings = settings.m_TextureCacheSettings;
	stateSettings.m_DrawTriangleListRI = SEOUL_BIND_DELEGATE(&Drawer::DrawTriangleListRI, m_pDrawer.Get());
	stateSettings.m_pInterface = this;
	stateSettings.m_uMaxIndexCountBatch = settings.m_uIndexBufferSizeInIndices;
	stateSettings.m_uMaxVertexCountBatch = settings.m_uVertexBufferSizeInVertices;

	// Instantiate the state instance.
	m_pState.Reset(SEOUL_NEW(MemoryBudgets::Falcon) Falcon::Render::State(stateSettings));

	// Also instantatiate our higher DrawerState instance, which
	// contains some UI specific extension to the state
	// used by Falcon::Drawer.
	m_pDrawerState.Reset(SEOUL_NEW(MemoryBudgets::Falcon) DrawerState(*m_pState));

#if SEOUL_HOT_LOADING
	SEOUL_ASSERT(IsMainThread());

	// Register for appropriate callbacks with ContentLoadManager.
	Events::Manager::Get()->RegisterCallback(
		Content::FileChangeEventId,
		SEOUL_BIND_DELEGATE(&Renderer::OnFileChange, this));
	// Make sure we're first in line for the file change event,
	// so we come before the Content::Store that will actually handle the
	// change event.
	Events::Manager::Get()->MoveLastCallbackToFirst(Content::FileChangeEventId);

	Events::Manager::Get()->RegisterCallback(
		Content::FileLoadCompleteEventId,
		SEOUL_BIND_DELEGATE(&Renderer::OnFileLoadComplete, this));
#endif // /#if SEOUL_HOT_LOADING
}

Renderer::~Renderer()
{
#if SEOUL_HOT_LOADING
	SEOUL_ASSERT(IsMainThread());

	Events::Manager::Get()->UnregisterCallback(
		Content::FileLoadCompleteEventId,
		SEOUL_BIND_DELEGATE(&Renderer::OnFileLoadComplete, this));

	Events::Manager::Get()->UnregisterCallback(
		Content::FileChangeEventId,
		SEOUL_BIND_DELEGATE(&Renderer::OnFileChange, this));
#endif // /#if SEOUL_HOT_LOADING

	// Cleanup the camera.
	m_pCamera.Reset();
}

void Renderer::BeginFrame(const Viewport& initialViewport)
{
	{
		static const HString kDefault("Default");

		HString name = m_Stage3DSettings;
		if (name.IsEmpty())
		{
			name = kDefault;
		}

		auto pSettings = Falcon::g_Config.m_pGetStage3DSettings(name);
		if (nullptr != pSettings)
		{
			*m_pState->m_pStage3DSettings = *pSettings;
		}
		else
		{
			*m_pState->m_pStage3DSettings = Falcon::Stage3DSettings();
		}
	}

#if SEOUL_HOT_LOADING
	// Process the hot loading table - entries with true can be processed and removed.
	// To simplify things, we process all entries at once.
	if (!m_tHotLoading.IsEmpty())
	{
		Bool bProcessEntries = true;

		{
			auto const iBegin = m_tHotLoading.Begin();
			auto const iEnd = m_tHotLoading.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				if (!i->Second)
				{
					bProcessEntries = false;
					break;
				}
			}
		}

		if (bProcessEntries)
		{
			// Purge the texture cache and clear hot loads.
			PurgeTextureCache();
			m_tHotLoading.Clear();
		}
	}
#endif // /#if SEOUL_HOT_LOADING

	++m_uRenderFrameCount;

	// Cache the viewport
	PushViewport(initialViewport);

	// Update the Fx camera.
	ApplyStandardCamera();

	// Mark that we don't have a previous movie yet.
	m_MovieState = MovieState();

	// Process a pending texture purge now,
	// if requested.
	InternalHandlePendingPurge();
}

void Renderer::BeginMovie(
	Movie* pMovie,
	const Falcon::Rectangle& stageBounds)
{
	m_pActiveMovie = pMovie;

	// Cache the active viewport.
	Viewport const activeViewport = GetActiveViewport();
	pMovie->SetLastViewport(activeViewport);

	// Kick the poser.
	m_pPoser->Begin(*m_pState);

	SetMovieDependentState(pMovie, activeViewport, stageBounds);

	// Commit world cull command if necessary.
	if (!m_MovieState.m_bHasState ||
		m_MovieState.m_WorldCullRectangle != m_pState->m_WorldCullRectangle ||
		m_MovieState.m_fWorldHeightToScreenHeight != m_pState->m_fWorldHeightToScreenHeight ||
		m_MovieState.m_fWorldWidthToScreenWidth != m_pState->m_fWorldWidthToScreenWidth)
	{
		m_pState->m_Buffer.IssueWorldCullChange(
			m_pState->m_WorldCullRectangle,
			m_pState->m_fWorldWidthToScreenWidth,
			m_pState->m_fWorldHeightToScreenHeight);
	}

	// Commit view projection transform command if necessary.
	if (!m_MovieState.m_bHasState ||
		m_MovieState.m_vViewProjectionTransform != m_pState->m_vViewProjectionTransform)
	{
		auto const u = m_pDrawerState->m_vVector4Ds.GetSize();
		m_pDrawerState->m_vVector4Ds.PushBack(m_pState->m_vViewProjectionTransform);
		m_pState->m_Buffer.IssueGeneric(CommandType::kViewProjectionChange, u);
	}

	// Update movie state.
	m_MovieState.m_bHasState = true;
	m_MovieState.m_vViewProjectionTransform = m_pState->m_vViewProjectionTransform;
	m_MovieState.m_WorldCullRectangle = m_pState->m_WorldCullRectangle;
	m_MovieState.m_fWorldHeightToScreenHeight = m_pState->m_fWorldHeightToScreenHeight;
	m_MovieState.m_fWorldWidthToScreenWidth = m_pState->m_fWorldWidthToScreenWidth;
}

void Renderer::SetMovieDependentState(
	Movie* pMovie,
	Viewport activeViewport,
	const Falcon::Rectangle& stageBounds)
{
	// Cache the stage dimensions.
	Float32 const fStageWidth = (Float32)stageBounds.GetWidth();

	// Cache top and bottom.
	auto const vStageCoords = pMovie->ComputeStageTopBottom(activeViewport, stageBounds.GetHeight());
	Float32 const fStageTopRenderCoord = vStageCoords.X;
	Float32 const fStageBottomRenderCoord = vStageCoords.Y;

	// Compute the factor.
	Float32 const fVisibleHeight = (fStageBottomRenderCoord - fStageTopRenderCoord);
	Float32 const fVisibleWidth = (fVisibleHeight *
								   activeViewport.GetViewportAspectRatio());

	// Compute culling data.
	m_pState->m_WorldCullRectangle.m_fTop = fStageTopRenderCoord;
	m_pState->m_WorldCullRectangle.m_fBottom = fStageBottomRenderCoord;
	m_pState->m_WorldCullRectangle.m_fLeft = (fStageWidth - fVisibleWidth) / 2.0f;
	m_pState->m_WorldCullRectangle.m_fRight = (fStageWidth - m_pState->m_WorldCullRectangle.m_fLeft);
	m_pState->m_fWorldHeightToScreenHeight = ((Float)activeViewport.m_iViewportHeight) / fVisibleHeight;
	m_pState->m_fWorldWidthToScreenWidth = ((Float)activeViewport.m_iViewportWidth) / fVisibleWidth;
	m_pState->m_fWorldCullScreenArea = m_pState->m_WorldCullRectangle.GetWidth() * m_pState->m_WorldCullRectangle.GetHeight();
	m_pState->m_fMaxCostInBatchFromOverfill = ((Double)m_pState->m_fWorldCullScreenArea * kfMaxCostInBatchFromOverfillFactor);

	// Compute transform matrices.
	m_pState->m_vViewProjectionTransform.X = (2.0f / fVisibleWidth);
	m_pState->m_vViewProjectionTransform.Y = (-2.0f / fVisibleHeight);
	m_pState->m_vViewProjectionTransform.Z = -1.0f - 2.0f * (m_pState->m_WorldCullRectangle.m_fLeft / fVisibleWidth);
	m_pState->m_vViewProjectionTransform.W = 1.0f + 2.0f * (m_pState->m_WorldCullRectangle.m_fTop / fVisibleHeight);
}

void Renderer::UpdateTextureReplacement(FilePathRelativeFilename name, FilePath filePath)
{
	m_pState->m_pCache->UpdateIndirectTexture(name, filePath);
}

void Renderer::BeginMovieFxOnly(Movie* pMovie, FxRenderer& rRenderer)
{
	m_pActiveMovie = pMovie;

	ApplyFxOnlyCamera();

	// TODO: Need a reasonable way to specify the world cull and
	// view projection for an FxOnly movie. Right now, this is reusing whatever values were specified
	// by the last (standard) movie to render.

	// Commit view projection transform command if necessary.
	if (!m_MovieState.m_bHasState ||
		m_MovieState.m_vViewProjectionTransform != m_pState->m_vViewProjectionTransform)
	{
		auto const u = m_pDrawerState->m_vVector4Ds.GetSize();
		m_pDrawerState->m_vVector4Ds.PushBack(m_pState->m_vViewProjectionTransform);
		m_pState->m_Buffer.IssueGeneric(CommandType::kViewProjectionChange, u);
	}

	// Update the movie state.
	m_MovieState.m_bHasState = true;
	m_MovieState.m_vViewProjectionTransform = m_pState->m_vViewProjectionTransform;

	rRenderer.BeginPose(*m_pPoser);

	// TODO: Always desired/ok? Depth is not well established in
	// preview mode, which is the only use case of "fx only" movies.
	m_pPoser->PushDepth3D(0.0f, true);
}

void Renderer::EndMovie(Bool bFlushDeferred)
{
	if (bFlushDeferred)
	{
		m_pPoser->FlushDeferredDraw();
	}

	m_pActiveMovie = nullptr;
}

void Renderer::EndMovieFxOnly(FxRenderer& rRenderer)
{
	// TODO: Always desired/ok? Depth is not well established in
	// preview mode, which is the only use case of "fx only" movies.
	m_pPoser->PopDepth3D(0.0f, true);

	rRenderer.EndPose();
	EndMovie();

	ApplyStandardCamera();
}

void Renderer::EndFrame(
	RenderCommandStreamBuilder& rBuilder,
	RenderPass* pPass)
{
	// Pop the initial viewport.
	PopViewport();

#if SEOUL_ENABLE_CHEATS
	if (m_bDebugEnableOcclusionOptimizer)
#endif // /#if SEOUL_ENABLE_CHEATS
	{
		if (0 == m_SuppressOcclusionOptimizer)
		{
			// Optimize the built buffer.
			m_pOcclusionOptimizer->Optimize(m_pDrawerState->m_rState.m_Buffer);
		}
	}

#if SEOUL_ENABLE_CHEATS
	if (m_bDebugEnableBatchOptimizer)
#endif // /#if SEOUL_ENABLE_CHEATS
	{
		// Optimize the built buffer.
		m_pBatchOptimizer->Optimize(m_pDrawerState->m_rState.m_Buffer);
	}

	// Now perform draw off the posed command buffer.
	{
		m_pDrawer->ProcessDraw(
			*m_pDrawerState,
			&rBuilder,
			pPass);
	}
}

/**
 * Apply the current renderer configuration to compute
 * a 3D depth value based on a world Y position. This
 * assumes a correlation between the two (e.g. this
 * requires that a character is running along a ground
 * plane, not jumping).
 */
Float Renderer::ComputeDepth3D(Float fY) const
{
	return m_pState->ComputeDepth3D(fY);
}

Falcon::HitTester Renderer::GetHitTester(
	const Movie& movie,
	const Falcon::Rectangle& stageBounds,
	const Viewport& activeViewport) const
{
	// Cache the stage dimensions.
	Float32 const fStageWidth = (Float32)stageBounds.GetWidth();

	// Cache top and bottom.
	auto const vStageCoords = movie.ComputeStageTopBottom(activeViewport, stageBounds.GetHeight());
	Float32 const fStageTopRenderCoord = vStageCoords.X;
	Float32 const fStageBottomRenderCoord = vStageCoords.Y;

	// Compute the factor.
	Float32 const fVisibleHeight = (fStageBottomRenderCoord - fStageTopRenderCoord);
	Float32 const fVisibleWidth = (fVisibleHeight *
		activeViewport.GetViewportAspectRatio());

	// Compute cull rectangle.
	Rectangle worldCullRectangle;
	worldCullRectangle.m_fTop = fStageTopRenderCoord;
	worldCullRectangle.m_fBottom = fStageBottomRenderCoord;
	worldCullRectangle.m_fLeft = (fStageWidth - fVisibleWidth) / 2.0f;
	worldCullRectangle.m_fRight = (fStageWidth - worldCullRectangle.m_fLeft);

	// Compute transform matrices.
	Vector4D vViewProjectionTransform;
	vViewProjectionTransform.X = (2.0f / fVisibleWidth);
	vViewProjectionTransform.Y = (-2.0f / fVisibleHeight);
	vViewProjectionTransform.Z = -1.0f - 2.0f * (worldCullRectangle.m_fLeft / fVisibleWidth);
	vViewProjectionTransform.W = 1.0f + 2.0f * (worldCullRectangle.m_fTop / fVisibleHeight);

	return Falcon::HitTester(
		vViewProjectionTransform,
		worldCullRectangle,
		m_pState->GetPerspectiveFactor());
}

/**
 * Equivalent to FOV or aspect ratio in our UI's
 * very simplified 3D projection model.
 */
Float Renderer::GetPerspectiveFactor() const
{
	return m_pState->GetPerspectiveFactor();
}

/**
 * Applies a modification to the fixed base perspective
 * factor. Allows a base to be set at design time
 * and a modification to be applied for effects at
 * runtime.
 */
Float Renderer::GetPerspectiveFactorAdjustment() const
{
	return m_pState->m_fPerspectiveFactorAdjustment;
}

const Vector4D& Renderer::GetViewProjectionTransform() const
{
	return m_pState->m_vViewProjectionTransform;
}

/**
 * Entry point for posing the root node of a Falcon scene graph.
 */
void Renderer::PoseRoot(const SharedPtr<Falcon::MovieClipInstance>& pRoot)
{
	// Now pose the provided root node.
	pRoot->Pose(
		*m_pPoser,
		Matrix2x3::Identity(),
		Falcon::ColorTransformWithAlpha::Identity());
}

/**
 * This is an inner posing call - expected to be called from
 * within a Falcon::Instance::Pose() method to insert
 * a draw command for a custom (out-of-band) render operation
 * mixed into the Falcon command stream.
 */
void Renderer::PoseCustomDraw(const Delegate<void(RenderPass&, RenderCommandStreamBuilder&)>& callback)
{
	UInt32 const u = m_pDrawerState->m_vCustomDraws.GetSize();
	m_pDrawerState->m_vCustomDraws.PushBack(callback);
	m_pState->m_Buffer.IssueGeneric(CommandType::kCustomDraw, u);
}

#if SEOUL_ENABLE_CHEATS
Bool Renderer::GetDebugEnableOverfillOptimizer() const
{
	return m_pDrawer->GetDebugEnableOverfillOptimizer();
}

void Renderer::SetDebugEnableOverfillOptimizer(Bool bEnable)
{
	m_pDrawer->SetDebugEnableOverfillOptimizer(bEnable);
}

/**
 * Equivalent to PoseRoot(), but for the special
 * (developer only) posing pass that is used
 * to visualization input rectangles and shapes.
 */
void Renderer::PoseInputVisualization(
	const InputWhitelist& inputWhitelist,
	UInt8 uInputMask,
	const SharedPtr<Falcon::MovieClipInstance>& pRoot)
{
	pRoot->PoseInputVisualizationChildren(
		inputWhitelist,
		uInputMask,
		*m_pPoser,
		Matrix2x3::Identity(),
		Falcon::ColorTransformWithAlpha::Identity());
}

void Renderer::PoseInputVisualizationWithRestrictionRectangle(
	const Rectangle2D& rect,
	const InputWhitelist& inputWhitelist,
	UInt8 uInputMask,
	const SharedPtr<Falcon::MovieClipInstance>& pRoot)
{
	// Restriction rectangle prevents input capture within
	// the rectangle, so we add 4 clipping rectangles that
	// surround that rectangle.
	{
		auto const bounds = m_pPoser->GetState().m_WorldCullRectangle;
		auto const matrix = Matrix2x3::Identity();
		auto const left = Falcon::Rectangle::Create(bounds.m_fLeft, rect.m_fLeft, bounds.m_fTop, bounds.m_fBottom);
		m_pPoser->ClipStackAddRectangle(matrix, left);
		auto const right = Falcon::Rectangle::Create(rect.m_fRight, bounds.m_fRight, bounds.m_fTop, bounds.m_fBottom);
		m_pPoser->ClipStackAddRectangle(matrix, right);
		auto const top = Falcon::Rectangle::Create(rect.m_fLeft, rect.m_fRight, bounds.m_fTop, rect.m_fTop);
		m_pPoser->ClipStackAddRectangle(matrix, top);
		auto const bottom = Falcon::Rectangle::Create(rect.m_fLeft, rect.m_fRight, rect.m_fBottom, bounds.m_fBottom);
		m_pPoser->ClipStackAddRectangle(matrix, bottom);
		m_pPoser->ClipStackPush();
	}

	// Now pose the viz.
	pRoot->PoseInputVisualizationChildren(
		inputWhitelist,
		uInputMask,
		*m_pPoser,
		Matrix2x3::Identity(),
		Falcon::ColorTransformWithAlpha::Identity());

	// Cleanup the clip stack.
	{
		m_pPoser->ClipStackPop();
	}
}
#endif // /#if SEOUL_ENABLE_CHEATS

/**
 * Update the runtime mutation of the overall perspective factor.
 */
void Renderer::SetPerspectiveFactorAdjustment(Float f)
{
	m_pState->m_fPerspectiveFactorAdjustment = f;
}

/**
 * Updates the world top and bottom of the 3D stage. Used
 * as part of the overall 3D projection effect.
 */
void Renderer::SetStage3DProjectionBounds(Float fTopY, Float fBottomY)
{
	m_pState->m_fStage3DTopY = fTopY;
	m_pState->m_fStage3DBottomY = fBottomY;
}

/**
 * Call to unset/remove a viewport override.
 *
 * \pre A corresponding call to PushViewport().
 */
void Renderer::PopViewport()
{
	m_vViewportStack.PopBack();
	if (!m_vViewportStack.IsEmpty())
	{
		InternalCommitActiveViewport();
	}
}

/**
 * Call to set a viewport override. Must
 * later be followed with a call to PopViewport().
 */
void Renderer::PushViewport(const Viewport& viewport)
{
	m_vViewportStack.PushBack(viewport);
	InternalCommitActiveViewport();
}

#if SEOUL_ENABLE_CHEATS
/** Developer functionality, used for rendering input visualizatino hit rectangles. */
void Renderer::BeginInputVisualizationMode()
{
	m_pState->m_Buffer.IssueGeneric(CommandType::kBeginInputVisualization);
}

void Renderer::EndInputVisualizationMode()
{
	m_pState->m_Buffer.IssueGeneric(CommandType::kEndInputVisualization);
}

Falcon::Render::Mode::Enum Renderer::GetRenderMode() const
{
	return m_pDrawer->GetRenderMode();
}

void Renderer::SetRenderMode(Falcon::Render::Mode::Enum eMode)
{
	m_pDrawer->SetRenderMode(eMode);
}
#endif // /#if SEOUL_ENABLE_CHEATS

Bool Renderer::ResolveTextureReference(
	Float fRenderThreshold,
	const FilePath& filePath,
	Falcon::TextureReference& rTextureReference)
{
	return m_pState->m_pCache->ResolveTextureReference(
		fRenderThreshold,
		filePath,
		rTextureReference,
		true);
}

void Renderer::ApplyFxOnlyCamera()
{
	ApplyCameraCommon(m_fFxCameraInverseZoom, m_vFxCameraOffset);
}

void Renderer::ApplyStandardCamera()
{
	ApplyCameraCommon(1.0f, Vector3D::Zero());
}

void Renderer::ApplyCameraCommon(Float fZoom, const Vector3D& vOffset)
{
	// Cache half the world height.
	auto const initialViewport = GetActiveViewport();
	Float const fHalfHeight = 0.5f * fZoom * Manager::Get()->ComputeUIRendererFxCameraWorldHeight(initialViewport);
	Float const fHalfWidth = fHalfHeight * initialViewport.GetViewportAspectRatio();

	// Set camera position.
	m_pCamera->SetPosition(
		vOffset +
		Vector3D(0, 0, kfUIRendererFxCameraWorldDistance));

	// Setup the camera with specified values. For Game, this is an orthographic projection
	m_pCamera->SetOrthographic(
		-fHalfWidth,
		fHalfWidth,
		-fHalfHeight,
		fHalfHeight,
		1.0f,
		2.0f * kfUIRendererFxCameraWorldDistance + 1.0f);
}

void Renderer::ClearPack()
{
	m_pDrawer->ClearPack();
}

void Renderer::Pack(
	Falcon::PackerTree2D::NodeID nodeID,
	const SharedPtr<Falcon::Texture>& pSource,
	const Rectangle2DInt& source,
	const Point2DInt& destination)
{
	m_pDrawer->Pack(nodeID, pSource, source, destination);
}

void Renderer::ResolvePackerTexture(Falcon::TexturePacker& rPacker, SharedPtr<Falcon::Texture>& rp)
{
	rp.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) AtlasTexture(rPacker));
}

void Renderer::ResolveTexture(
	FilePath filePath,
	SharedPtr<Falcon::Texture>& rp)
{
	rp.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) Texture(filePath));
}

void Renderer::ResolveTexture(
	UInt8 const* pData,
	UInt32 uDataWidth,
	UInt32 uDataHeight,
	UInt32 uStride,
	Bool bIsFullOccluder,
	SharedPtr<Falcon::Texture>& rp)
{
	rp.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) Texture(
		pData,
		uDataWidth,
		uDataHeight,
		uStride,
		bIsFullOccluder));
}

void Renderer::UnPack(Falcon::PackerTree2D::NodeID nodeID)
{
	m_pDrawer->UnPack(nodeID);
}

void Renderer::InternalCommitActiveViewport()
{
	// Cache the active viewport.
	Viewport const activeViewport = GetActiveViewport();

	UInt32 const u = m_pDrawerState->m_vViewports.GetSize();
	m_pDrawerState->m_vViewports.PushBack(activeViewport);
	m_pState->m_Buffer.IssueGeneric(CommandType::kViewportChange, u);
}

void Renderer::InternalHandlePendingPurge()
{
	// One way or another, when this function is called,
	// we release our texture scratch.
	m_vTextureScratch.Clear();

	// Early out if nothing to do - we only fall through
	// if we're in the purge textures state.
	if (PurgeState::kPurgeTextures != m_ePendingPurge)
	{
		// Now in the inactive state.
		m_ePendingPurge = PurgeState::kInactive;
		return;
	}

	// Now in the wait for reacquire state.
	m_ePendingPurge = PurgeState::kWaitForReacquire;

	// Locally grab SeoulEngine texture references
	// so they are not released prematurely. We want to hold them
	// for a single frame to allow the next draw frame
	// a chance to reacquire textures (holding the SeoulEngine
	// references does not prevent hot loading or patcher reloading,
	// the underlying system handles that).
	m_vTextureScratch.Clear();
	auto const& list = m_pDrawerState->m_rState.m_pCache->GetList();
	for (auto p = list.GetHeadGlobal(); nullptr != p; p = p->GetNextGlobal())
	{
		auto pTexture(static_cast<Texture*>(p->m_pOriginalTexture.GetPtr()));
		if (pTexture != nullptr)
		{
			m_vTextureScratch.PushBack(pTexture->GetTextureContentHandle());
		}
	}

	// Now purge all cache state.
	m_pDrawerState->m_rState.m_pCache->Purge();
}

#if SEOUL_HOT_LOADING
void Renderer::OnFileChange(Content::ChangeEvent* pFileChangeEvent)
{
	// Don't insert entries if hot loading is suppressed.
	if (Content::LoadManager::Get()->IsHotLoadingSuppressed())
	{
		return;
	}

	// Prepare to hot load/refresh the file.
	FilePath const filePath = pFileChangeEvent->m_New;
	if (IsTextureFileType(filePath.GetType()))
	{
		(void)m_tHotLoading.Insert(filePath, false);
	}
}

void Renderer::OnFileLoadComplete(FilePath filePath)
{
	if (m_tHotLoading.HasValue(filePath))
	{
		m_tHotLoading.Overwrite(filePath, true);
	}
}
#endif // /#if SEOUL_HOT_LOADING

} // namespace Seoul::UI
