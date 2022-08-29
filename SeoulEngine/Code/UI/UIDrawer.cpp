/**
 * \file UIDrawer.cpp
 * \brief Component of UI::Renderer, handles direct interactions with
 * the render backend.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EffectManager.h"
#include "FalconRenderDrawer.h"
#include "FalconTextureCache.h"
#if SEOUL_ENABLE_CHEATS
#include "Engine.h"
#include "LocManager.h"
#endif // /#if SEOUL_ENABLE_CHEATS
#include "Logger.h"
#include "ReflectionEnum.h"
#include "RenderCommandStreamBuilder.h"
#include "RenderDevice.h"
#include "RenderPass.h"
#include "ScopedAction.h"
#include "UIDrawer.h"
#include "UITexture.h"

namespace Seoul
{

namespace
{

struct Techniques SEOUL_SEALED
{
	HString const kAllFeaturesOverdraw;
	HString const kAlphaShapeOverdraw;
	HString const kColorMultiplyOverdraw;
	HString const kColorMultiplyAddOverdraw;
	HString const kDetailOverdraw;

	HString const kAllFeaturesSecondaryTexture;
	HString const kAllFeatures;
	HString const kAllFeaturesDetailSecondaryTexture;
	HString const kAllFeaturesDetail;
	HString const kAlphaShapeSecondaryTexture;
	HString const kAlphaShape;
	HString const kColorMultiply;
	HString const kColorMultiplyAdd;
	HString const kColorMultiplySecondaryTexture;
	HString const kColorMultiplyAddSecondaryTexture;
	HString const kShadowTwoPass;
	HString const kShadowTwoPassSecondaryTexture;

	HString const kExtendedColorAlphaShape;
	HString const kExtendedColorAlphaShapeSecondaryTexture;
}; // struct Techniques

static const Techniques kTechniques2D =
{
	HString("seoul_RenderAllFeaturesOverdraw2D"),
	HString("seoul_RenderAlphaShapeOverdraw2D"),
	HString("seoul_RenderColorMultiplyOverdraw2D"),
	HString("seoul_RenderColorMultiplyAddOverdraw2D"),
	HString("seoul_RenderAllFeaturesDetailOverdraw2D"),

	HString("seoul_RenderAllFeaturesSecondaryTexture2D"),
	HString("seoul_RenderAllFeatures2D"),
	HString("seoul_RenderAllFeaturesDetailSecondaryTexture2D"),
	HString("seoul_RenderAllFeaturesDetail2D"),
	HString("seoul_RenderAlphaShapeSecondaryTexture2D"),
	HString("seoul_RenderAlphaShape2D"),
	HString("seoul_RenderColorMultiply2D"),
	HString("seoul_RenderColorMultiplyAdd2D"),
	HString("seoul_RenderColorMultiplySecondaryTexture2D"),
	HString("seoul_RenderColorMultiplyAddSecondaryTexture2D"),
	HString("seoul_RenderShadowTwoPass2D"),
	HString("seoul_RenderShadowTwoPassSecondaryTexture2D"),

	HString("seoul_RenderColorAlphaShape2D"),
	HString("seoul_RenderColorAlphaShapeSecondaryTexture2D"),
};

static const Techniques kTechniques3D =
{
	HString("seoul_RenderAllFeaturesOverdraw3D"),
	HString("seoul_RenderAlphaShapeOverdraw3D"),
	HString("seoul_RenderColorMultiplyOverdraw3D"),
	HString("seoul_RenderColorMultiplyAddOverdraw3D"),
	HString("seoul_RenderAllFeaturesDetailOverdraw3D"),

	HString("seoul_RenderAllFeaturesSecondaryTexture3D"),
	HString("seoul_RenderAllFeatures3D"),
	HString("seoul_RenderAllFeaturesDetailSecondaryTexture3D"),
	HString("seoul_RenderAllFeaturesDetail3D"),
	HString("seoul_RenderAlphaShapeSecondaryTexture3D"),
	HString("seoul_RenderAlphaShape3D"),
	HString("seoul_RenderColorMultiply3D"),
	HString("seoul_RenderColorMultiplyAdd3D"),
	HString("seoul_RenderColorMultiplySecondaryTexture3D"),
	HString("seoul_RenderColorMultiplyAddSecondaryTexture3D"),
	HString("seoul_RenderShadowTwoPass3D"),
	HString("seoul_RenderShadowTwoPassSecondaryTexture3D"),

	HString("seoul_RenderColorAlphaShape3D"),
	HString("seoul_RenderColorAlphaShapeSecondaryTexture3D"),
};

} // namespace

static const HString kPackTechnique("seoul_Pack");
static const HString kPerspective("seoul_Perspective");
static const HString kShadowAccumulateState("seoul_ShadowAccumulateState");
static const HString kShadowApplyState("seoul_ShadowApplyState");
static const HString kStateTechnique_Default("seoul_State");
#if SEOUL_ENABLE_CHEATS
static const HString kInputVisualizationStateTechnique("seoul_InputVisualizationState");
#endif // /#if SEOUL_ENABLE_CHEATS
static const HString kColorTexture("seoul_Texture");
static const HString kDetailTexture("seoul_Detail");
static const HString kViewProjectionTransform("seoul_ViewProjectionUI");

static const HString kRenderAllFeaturesDetail2D("seoul_RenderAllFeaturesDetail2D");

static VertexElement const* GetUIDrawerVertexElements2D()
{
	static const VertexElement s_kaElements[] =
	{
		// Position (in stream 0)
		{ 0,                            // stream
		  0,                            // offset
		  VertexElement::TypeFloat2,    // type
		  VertexElement::MethodDefault, // method
		  VertexElement::UsagePosition, // usage
		  0u },                         // usage index

		// Color0 (in stream 0)
		{ 0,                            // stream
		  8,                            // offset
		  VertexElement::TypeColor,     // type
		  VertexElement::MethodDefault, // method
		  VertexElement::UsageColor,    // usage
		  0u },	                        // usage index

		// Color1 (in stream 0)
		{ 0,                            // stream
		  12,                           // offset
		  VertexElement::TypeColor,     // type
		  VertexElement::MethodDefault, // method
		  VertexElement::UsageColor,    // usage
		  1u },                         // usage index

		// TexCoords (in stream 0)
		{ 0,                            // stream
		  16,                           // offset
		  VertexElement::TypeFloat4,    // type
		  VertexElement::MethodDefault, // method
		  VertexElement::UsageTexcoord, // usage
		  0u },                         // usage index

		VertexElementEnd
	};

	return s_kaElements;
}

static VertexElement const* GetUIDrawerVertexElements3D()
{
	static const VertexElement s_kaElements[] =
	{
		// Position (in stream 0)
		{ 0,                            // stream
		  0,                            // offset
		  VertexElement::TypeFloat2,    // type
		  VertexElement::MethodDefault, // method
		  VertexElement::UsagePosition, // usage
		  0u },                         // usage index

		// Color0 (in stream 0)
		{ 0,                            // stream
		  8,                            // offset
		  VertexElement::TypeColor,     // type
		  VertexElement::MethodDefault, // method
		  VertexElement::UsageColor,    // usage
		  0u },	                        // usage index

		// Color1 (in stream 0)
		{ 0,                            // stream
		  12,                           // offset
		  VertexElement::TypeColor,     // type
		  VertexElement::MethodDefault, // method
		  VertexElement::UsageColor,    // usage
		  1u },                         // usage index

		// TexCoords (in stream 0)
		{ 0,                            // stream
		  16,                           // offset
		  VertexElement::TypeFloat4,    // type
		  VertexElement::MethodDefault, // method
		  VertexElement::UsageTexcoord, // usage
		  0u },                         // usage index

		// Depth Term (in stream 1)
		{ 1,                            // stream
		  0,                            // offset
		  VertexElement::TypeFloat1,    // type
		  VertexElement::MethodDefault, // method
		  VertexElement::UsageTexcoord, // usage
		  1u },                         // usage index

		VertexElementEnd
	};

	return s_kaElements;
}

namespace UI
{

DrawerState::DrawerState(Falcon::Render::State& rState)
	: m_rState(rState)
	, m_vCustomDraws()
	, m_vVector4Ds()
	, m_vViewports()
{
}

DrawerState::~DrawerState()
{
}

/**
 * Reset the buffered portions of state to its default.
 */
void DrawerState::Reset()
{
	m_rState.m_Buffer.Reset();
	m_vCustomDraws.Clear();
	m_vVector4Ds.Clear();
	m_vViewports.Clear();
}

/** Build our array to map extended blend mode indices into state technique name. */
static inline Drawer::ExtendedBlendModeTechniques GetExtendedBlendModeTechniques()
{
	using namespace Falcon::Render::Feature;

	Drawer::ExtendedBlendModeTechniques a;
	for (UInt32 i = 0u; i < a.GetSize(); ++i)
	{
		auto const iValue = (Int)((i + (EXTENDED_MIN >> EXTENDED_SHIFT)) << EXTENDED_SHIFT);

		HString baseName;
		SEOUL_VERIFY(
			EnumOf<Falcon::Render::Feature::Enum>().TryGetName(iValue, baseName));

		// Prepend prefix for technique name.
		a[i] = HString(String("seoul_State_") + String(baseName));
	}

	return a;
}

Drawer::Drawer(const DrawerSettings& settings /*= UI::DrawerSettings()*/)
	: m_aExtendedBlendModeTechniques(GetExtendedBlendModeTechniques())
	, m_Settings(settings)
	, m_pDrawer(SEOUL_NEW(MemoryBudgets::Falcon) Falcon::Render::Drawer)
	, m_StateEffect(EffectManager::Get()->GetEffect(settings.m_StateEffectFilePath))
	, m_RenderEffect(EffectManager::Get()->GetEffect(settings.m_EffectFilePath))
	, m_PackEffect(EffectManager::Get()->GetEffect(settings.m_PackEffectFilePath))
	, m_pDrawEffect(&m_RenderEffect)
	, m_pIndices(RenderDevice::Get()->CreateDynamicIndexBuffer(sizeof(UInt16) * settings.m_uIndexBufferSizeInIndices, IndexBufferDataFormat::kIndex16))
	, m_pVertices(RenderDevice::Get()->CreateDynamicVertexBuffer(sizeof(Falcon::ShapeVertex) * settings.m_uVertexBufferSizeInVertices, sizeof(Falcon::ShapeVertex)))
	, m_pDepths3D(RenderDevice::Get()->CreateDynamicVertexBuffer(sizeof(Float32) * settings.m_uVertexBufferSizeInVertices, sizeof(Float32)))
	, m_pVertexFormat2D(RenderDevice::Get()->CreateVertexFormat(GetUIDrawerVertexElements2D()))
	, m_pVertexFormat3D(RenderDevice::Get()->CreateVertexFormat(GetUIDrawerVertexElements3D()))
	, m_pActiveVertexFormat()
	, m_bTwoPassShadows(false)
	, m_bInTwoPassShadowRender(false)
	, m_vPackOps()
	, m_vPackAcquired()
	, m_tPackNodes()
	, m_LastPackTargetReset(0)
	, m_pState()
	, m_pBuilder()
	, m_pPass()
	, m_SolidFill()
#if SEOUL_ENABLE_CHEATS
	, m_eRendererMode(Falcon::Render::Mode::kDefault)
#endif // /#if SEOUL_ENABLE_CHEATS
{
}

Drawer::~Drawer()
{
}

static inline Bool Acquire(Drawer::EffectUtil& a, Drawer::EffectUtil& b, Drawer::EffectUtil& c, RenderCommandStreamBuilder& r)
{
	if (a.Acquire(r))
	{
		if (b.Acquire(r))
		{
			if (c.Acquire(r)) { return true; }
			else
			{
				b.Release();
				a.Release();
				return false;
			}
		}
		else
		{
			a.Release();
			return false;
		}
	}

	return false;
}

void Drawer::InternalPerformDraw()
{
	using namespace Falcon;
	using namespace Falcon::Render;

	// Handle debug scanning if enabled loc token viz is enabled.
#if SEOUL_ENABLE_CHEATS
	if (LocManager::Get()->DebugOnlyShowTokens())
	{
		m_pDrawer->SetDebugScanning(true);
		m_pDrawer->SetDebugScanningOffset(
			m_pDrawer->GetDebugScanningOffset() + (Engine::Get()->GetSecondsInTick() * 25.0f));

		// Clamp m_fDebugScanningOffset to a reasonable max.
		if (m_pDrawer->GetDebugScanningOffset() > (FlIntMax / 2))
		{
			m_pDrawer->SetDebugScanningOffset(0.0f);
		}
	}
	else
	{
		m_pDrawer->SetDebugScanning(false);
		m_pDrawer->SetDebugScanningOffset(0.0f);
	}
#endif // /#if SEOUL_ENABLE_CHEATS

	// Acquire and check the state effect.
	if (!Acquire(m_StateEffect, m_RenderEffect, m_PackEffect, *m_pBuilder)) { return; }
	// Validate in developer builds. Happens once at startup, but needs to wait
	// until after we've successfully acquired all effects.
#if !SEOUL_SHIP
	ValidateEffects();
#endif

	// Reset vertex format.
	m_pActiveVertexFormat.Reset();

	// Pre step, necessary to restore nodes on resolution changes/reset
	// events. Render targets are usually volatile and their contents
	// do not persist across resist events. Do this
	// *after* setting up the surface and builder.
	InternalCheckPackNodes();

	// Begin effect pass for the frame.
	InternalSetupVertexFormat(false);
	InternalSetupStateTechnique(Falcon::Render::Features());
	m_pBuilder->SetIndices(m_pIndices.GetPtr());
	m_pBuilder->SetVertices(0, m_pVertices.GetPtr(), 0, sizeof(Falcon::ShapeVertex));
	m_pBuilder->SetVertices(1, m_pDepths3D.GetPtr(), 0, sizeof(Float32));

	// Resolve the solid fill texture reference.
	(void)m_pState->m_rState.m_pCache->ResolveTextureReference(1.0f, FilePath(), m_SolidFill, true);

	// Start draw submission.
	m_pDrawer->Begin(m_pState->m_rState);

	// Commands here.
	Int32 iClip = -1;
	for (auto i = m_pState->m_rState.m_Buffer.Begin(); m_pState->m_rState.m_Buffer.End() != i; ++i)
	{
		auto const& cmd = *i;

		switch ((CommandType)cmd.m_uType)
		{
		case CommandType::kBeginInputVisualization:
#if SEOUL_ENABLE_CHEATS
			InternalBeginInputVisualizationMode();
#endif // /#if SEOUL_ENABLE_CHEATS
			break;
		case CommandType::kBeginPlanarShadows:
			m_pDrawer->BeginPlanarShadows();
			InternalBeginPlanarShadows();
			break;
		case CommandType::kBeginScissorClip:
			m_pDrawer->Flush();
			m_pBuilder->SetScissor(true, InternalWorldToScissorViewport(
				m_pState->m_rState.m_Buffer.GetRectangle(cmd.m_u)));
			break;
		case CommandType::kCustomDraw:
			InternalBeginCustomDraw();
			m_pState->m_vCustomDraws[cmd.m_u](*m_pPass, *m_pBuilder);
			InternalEndCustomDraw();
			break;
		case CommandType::kEndInputVisualization:
#if SEOUL_ENABLE_CHEATS
			InternalEndInputVisualizationMode();
#endif // /#if SEOUL_ENABLE_CHEATS
			break;
		case CommandType::kEndPlanarShadows:
			InternalEndPlanarShadows();
			m_pDrawer->EndPlanarShadows();
			break;
		case CommandType::kEndScissorClip:
			m_pDrawer->Flush();
			{
				auto const rect = m_pState->m_rState.m_Buffer.GetRectangle(cmd.m_u);
				if (rect.GetWidth() == 0.0f)
				{
					m_pBuilder->SetScissor(true, ToClearSafeScissor(m_pBuilder->GetCurrentViewport()));
				}
				else
				{
					m_pBuilder->SetScissor(true, InternalWorldToScissorViewport(rect));
				}
			}
			break;
		case CommandType::kPose:
			{
				// TODO: If pose.m_fDepth3D != to the depth of the clip
				// shape, clipping results will be incorrect. For homogenous
				// depths (all vertices in consideration for the clip shape
				// and for the mesh to be clipped have the same 3D depth),
				// we can solve this by reprojection the vertices based
				// on the depth differences, to place the clipping shape
				// in the depth plane of the clipped shape.

				auto const& pose = m_pState->m_rState.m_Buffer.GetPose(cmd.m_u);

				if (pose.m_iClip != iClip)
				{
					if (pose.m_iClip >= 0)
					{
						m_pState->m_rState.m_Buffer.GetClipCapture(pose.m_iClip)->Overwrite(
							*m_pState->m_rState.m_pClipStack);
					}
					else
					{
						m_pState->m_rState.m_pClipStack->Clear();
					}

					iClip = pose.m_iClip;
				}

				m_pDrawer->SetDepth3D(pose.m_fDepth3D);
				m_pDrawer->SetPlanarShadowPosition(pose.m_vShadowPlaneWorldPosition);

				pose.m_pRenderable->Draw(
					*m_pDrawer,
					pose.m_WorldRectanglePreClip,
					pose.m_mWorld,
					pose.m_cxWorld,
					pose.m_TextureReference,
					pose.m_iSubRenderableId);

#if SEOUL_ENABLE_CHEATS
				// If occlusion mode is enabled, draw it now.
				if ((Falcon::Render::Mode::kWorldBounds == m_eRendererMode && !pose.m_WorldRectangle.IsZero()) ||
					(Falcon::Render::Mode::kOcclusion == m_eRendererMode && !pose.m_WorldOcclusionRectangle.IsZero()))
				{
					static const RGBA kColor = RGBA::Create(118, 0, 143, 127);

					auto const& bounds = (Falcon::Render::Mode::kWorldBounds == m_eRendererMode
						? pose.m_WorldRectangle
						: pose.m_WorldOcclusionRectangle);

					FixedArray<ShapeVertex, 4> aVertices;
					aVertices[0] = ShapeVertex::Create(bounds.m_fLeft, bounds.m_fTop, kColor, RGBA::TransparentBlack());
					aVertices[1] = ShapeVertex::Create(bounds.m_fLeft, bounds.m_fBottom, kColor, RGBA::TransparentBlack());
					aVertices[2] = ShapeVertex::Create(bounds.m_fRight, bounds.m_fBottom, kColor, RGBA::TransparentBlack());
					aVertices[3] = ShapeVertex::Create(bounds.m_fRight, bounds.m_fTop, kColor, RGBA::TransparentBlack());

					// Both the world and occlusion rectangles have already
					// been projected, so we need to set depth to 0.0f for
					// this draw submission to avoid doubling up
					// the projection effect.
					auto const fDepth3D = m_pState->m_rState.m_fRawDepth3D;
					m_pState->m_rState.m_fRawDepth3D = 0.0f;
					m_pDrawer->DrawTriangleList(
						pose.m_WorldRectanglePreClip,
						m_SolidFill,
						Matrix2x3::Identity(),
						ColorTransformWithAlpha::Identity(),
						aVertices.Data(),
						4u,
						TriangleListDescription::kQuadList,
						Render::Feature::kColorMultiply);
					m_pState->m_rState.m_fRawDepth3D = fDepth3D;
				}
#endif // /#if SEOUL_ENABLE_CHEATS
			}
			break;
		case CommandType::kPoseInputVisualization:
			{
				auto const& pose = m_pState->m_rState.m_Buffer.GetPoseIV(cmd.m_u);

				if (pose.m_iClip != iClip)
				{
					if (pose.m_iClip >= 0)
					{
						m_pState->m_rState.m_Buffer.GetClipCapture(pose.m_iClip)->Overwrite(
							*m_pState->m_rState.m_pClipStack);
					}
					else
					{
						m_pState->m_rState.m_pClipStack->Clear();
					}

					iClip = pose.m_iClip;
				}

				auto const& bounds = pose.m_InputBounds;

				FixedArray<ShapeVertex, 4> aVertices;
				aVertices[0] = ShapeVertex::Create(bounds.m_fLeft, bounds.m_fTop, RGBA::White(), RGBA::TransparentBlack());
				aVertices[1] = ShapeVertex::Create(bounds.m_fLeft, bounds.m_fBottom, RGBA::White(), RGBA::TransparentBlack());
				aVertices[2] = ShapeVertex::Create(bounds.m_fRight, bounds.m_fBottom, RGBA::White(), RGBA::TransparentBlack());
				aVertices[3] = ShapeVertex::Create(bounds.m_fRight, bounds.m_fTop, RGBA::White(), RGBA::TransparentBlack());

				m_pDrawer->SetDepth3D(pose.m_fDepth3D);
				m_pDrawer->DrawTriangleList(
					pose.m_WorldRectanglePreClip,
					pose.m_TextureReference,
					pose.m_mWorld,
					pose.m_cxWorld,
					aVertices.Data(),
					4u,
					TriangleListDescription::kQuadList,
					Render::Feature::kColorMultiply);
			}
			break;
		case CommandType::kViewportChange:
			InternalCommitActiveViewport(m_pState->m_vViewports[cmd.m_u]);
			break;
		case CommandType::kViewProjectionChange:
			InternalCommitViewProjection(m_pState->m_vVector4Ds[cmd.m_u]);
			break;
		case CommandType::kWorldCullChange:
			{
				auto const& worldCull = m_pState->m_rState.m_Buffer.GetWorldCull(cmd.m_u);
				m_pState->m_rState.m_WorldCullRectangle = worldCull.m_WorldCullRectangle;
				m_pState->m_rState.m_fWorldWidthToScreenWidth = worldCull.m_fWorldWidthToScreenWidth;
				m_pState->m_rState.m_fWorldHeightToScreenHeight = worldCull.m_fWorldHeightToScreenHeight;
				m_pState->m_rState.m_fWorldCullScreenArea = worldCull.m_WorldCullRectangle.GetWidth() * worldCull.m_WorldCullRectangle.GetHeight();
				m_pState->m_rState.m_fMaxCostInBatchFromOverfill = ((Double)m_pState->m_rState.m_fWorldCullScreenArea * kfMaxCostInBatchFromOverfillFactor);
			}
			break;

		case CommandType::kUnknown: // fall-through
		default:
			break;
		};
	}

	// Done.
	m_pDrawer->End();

	// Reset solid fill texture reference (this releases our strong pointer
	// to the underlying texture as well).
	m_SolidFill = Falcon::TextureReference();

	// Unset techniques.
	m_pDrawEffect->SetActiveTechnique();
	m_StateEffect.SetActiveTechnique();

	// Unset vertex format.
	m_pActiveVertexFormat.Reset();

	// Now process any pending pack operations.
	InternalProcessPackOps();

	// Release buffers so they don't spill into other
	// rendering. Particularly stream 1.
	m_pBuilder->SetVertices(1, nullptr, 0, 0);
	m_pBuilder->SetVertices(0, nullptr, 0, 0);
	m_pBuilder->SetIndices(nullptr);

	// Release acquired effects.
	m_PackEffect.Release();
	m_RenderEffect.Release();
	m_StateEffect.Release();
}

void Drawer::InternalBeginCustomDraw()
{
	m_pDrawer->Flush();

	// Unset techniques.
	m_pDrawEffect->SetActiveTechnique();
	m_StateEffect.SetActiveTechnique();

	// Unset vertex format.
	m_pActiveVertexFormat.Reset();
}

void Drawer::InternalEndCustomDraw()
{
	InternalSetupVertexFormat(false);
	InternalSetupStateTechnique(Falcon::Render::Features());

	m_pBuilder->SetIndices(m_pIndices.GetPtr());
	m_pBuilder->SetVertices(0, m_pVertices.GetPtr(), 0, sizeof(Falcon::ShapeVertex));
	m_pBuilder->SetVertices(1, m_pDepths3D.GetPtr(), 0, sizeof(Float32));
}

#if SEOUL_ENABLE_CHEATS
/** Developer functionality, used for rendering input visualizatino hit rectangles. */
void Drawer::InternalBeginInputVisualizationMode()
{
	// Flush prior to mode changes.
	m_pDrawer->Flush();

	// Reset the active technique.
	m_pDrawEffect->SetActiveTechnique();
	// Set new state technique.
	m_StateEffect.SetActiveTechnique(true, kInputVisualizationStateTechnique);
}

void Drawer::InternalEndInputVisualizationMode()
{
	// Flush prior to mode changes.
	m_pDrawer->Flush();

	// Reset the active technique.
	m_pDrawEffect->SetActiveTechnique();
	// Set new state technique.
	m_StateEffect.SetActiveTechnique(false, kStateTechnique_Default);
}
#endif // /#if SEOUL_ENABLE_CHEATS

void Drawer::InternalBeginPlanarShadows()
{
	// Nothing to do if we can't support two-pass shadows, or
	// if the current render mode does not support them.
	if (!m_bTwoPassShadows ||
#if SEOUL_ENABLE_CHEATS
		!SupportsTwoPassShadow(m_eRendererMode) ||
#endif // /#if SEOUL_ENABLE_CHEATS
		m_pState->m_rState.m_pStage3DSettings->m_Shadow.GetDebugForceOnePassRendering())
	{
		return;
	}

	// Flush prior to mode changes.
	m_pDrawer->Flush();

	// Reset the active technique.
	m_pDrawEffect->SetActiveTechnique();
	// Set new state technique.
	m_StateEffect.SetActiveTechnique(true, kShadowAccumulateState);

	// Now rendering for two-pass shadows.
	m_bInTwoPassShadowRender = true;
}

void Drawer::InternalEndPlanarShadows()
{
	// Nothing to do if we can't support two-pass shadows, or
	// if the current render mode does not support them.
	if (!m_bTwoPassShadows ||
#if SEOUL_ENABLE_CHEATS
		!SupportsTwoPassShadow(m_eRendererMode) ||
#endif // /#if SEOUL_ENABLE_CHEATS
		m_pState->m_rState.m_pStage3DSettings->m_Shadow.GetDebugForceOnePassRendering())
	{
		return;
	}

	// Flush prior to mode changes.
	m_pDrawer->Flush();

	// No longer rendering for two pass shadows.
	m_bInTwoPassShadowRender = false;

	{
		// Reset the active technique.
		m_pDrawEffect->SetActiveTechnique();
		// Stop the shadow accumulation state effect and start the state effect used
		// for applying the accumulated shadow.
		m_StateEffect.SetActiveTechnique(true, kShadowApplyState);

		// Rectangle to render.
		auto const& r = m_pDrawer->GetPlanarShadowBounds();

		// TODO: Should make this tighter fitting so it doesn't
		// have as much unnecessary overdraw.

		// Setup indices and vertices.
		FixedArray<UInt16, 6u> aIndices;
		aIndices[0] = 0;
		aIndices[1] = 1;
		aIndices[2] = 2;
		aIndices[3] = 0;
		aIndices[4] = 2;
		aIndices[5] = 3;

		FixedArray<Falcon::ShapeVertex, 4u> aVertices;
		aVertices[0].m_vP = Vector2D(r.m_fLeft, r.m_fTop);
		aVertices[1].m_vP = Vector2D(r.m_fLeft, r.m_fBottom);
		aVertices[2].m_vP = Vector2D(r.m_fRight, r.m_fBottom);
		aVertices[3].m_vP = Vector2D(r.m_fRight, r.m_fTop);
		for (auto i = aVertices.Begin(); aVertices.End() != i; ++i)
		{
			i->m_ColorMultiply = RGBA::Create(0, 0, 0, 255);
		}

		// Resolve the solid fill texture.
		Falcon::TextureReference solidFill;
		(void)m_pState->m_rState.m_pCache->ResolveTextureReference(1.0f, FilePath(), solidFill, true);

		// Render the shadow application quad.
		Falcon::Render::Features features;
		features.SetColorMultiply();
		DrawTriangleListRI(
			solidFill.m_pTexture,
			SharedPtr<Falcon::Texture>(),
			aIndices.Data(),
			6u,
			nullptr,
			aVertices.Data(),
			4u,
			features);
	}

	// Reset the active technique.
	m_pDrawEffect->SetActiveTechnique();
	// Stop the shadow application state effect and start the state effect used
	// for normal rendering.
	m_StateEffect.SetActiveTechnique(false, kStateTechnique_Default);
}

void Drawer::DrawTriangleListRI(
	const SharedPtr<Falcon::Texture>& pColorTex,
	const SharedPtr<Falcon::Texture>& pDetailTex,
	UInt16 const* pIndices,
	UInt32 uIndexCount,
	Float32 const* pDepths3D,
	Falcon::ShapeVertex const* pVertices,
	UInt32 uVertexCount,
	const Falcon::Render::Features& features)
{
	// Early out if nothing to draw.
	if (0 == uIndexCount || 0 == uVertexCount)
	{
		return;
	}

	// Check and reconfigure our rendering vertex format
	// and effect technique before performing the render operation.
	Bool const b3D = (nullptr != pDepths3D);
	InternalSetupVertexFormat(b3D);
	InternalSetupStateTechnique(features);
	InternalSetupEffectTechnique(features, pColorTex, b3D);

	// Color can be the invalid handle if not explicitly set.
	auto const hColorTexture = (pColorTex.IsValid()
		? ((Texture*)pColorTex.GetPtr())->GetTextureContentHandle()
		: TextureContentHandle());

	// Detail always resolves to the solid fill texture. This allows
	// batches to include draw operations that do not use the detail
	// texture (the detail will be set to solid white).
	auto const hDetailTexture = (pDetailTex.IsValid()
		? ((Texture*)pDetailTex.GetPtr())->GetTextureContentHandle()
		: (m_SolidFill.m_pTexture.IsValid() ? ((Texture*)m_SolidFill.m_pTexture.GetPtr())->GetTextureContentHandle() : TextureContentHandle()));

	m_pDrawEffect->SetColorTexture(hColorTexture);
	m_pDrawEffect->SetDetailTexture(hDetailTexture);

	// Indices.
	{
		UInt32 const uSizeInBytes = (UInt32)(sizeof(UInt16) * Min(uIndexCount, m_Settings.m_uIndexBufferSizeInIndices));
#if !SEOUL_SHIP
		if (m_Settings.m_uIndexBufferSizeInIndices < uIndexCount)
		{
			SEOUL_WARN("UIDrawer: Out of index buffer space, have %u indices, need %u indices", (UInt32)m_Settings.m_uIndexBufferSizeInIndices, (UInt32)uIndexCount);
		}
#endif // /#if !SEOUL_SHIP
		void* pOutIndices = m_pBuilder->LockIndexBuffer(m_pIndices.GetPtr(), uSizeInBytes);
		memcpy(pOutIndices, pIndices, uSizeInBytes);
		m_pBuilder->UnlockIndexBuffer(m_pIndices.GetPtr());
	}

	// Depths - optiona.
	if (b3D)
	{
		UInt32 const uSizeInBytes = (UInt32)(sizeof(Float32) * Min(uVertexCount, m_Settings.m_uVertexBufferSizeInVertices));
		void* pOutDepths = m_pBuilder->LockVertexBuffer(m_pDepths3D.GetPtr(), uSizeInBytes);
		memcpy(pOutDepths, pDepths3D, uSizeInBytes);
		m_pBuilder->UnlockVertexBuffer(m_pDepths3D.GetPtr());
	}

	// Vertices.
	{
		UInt32 const uSizeInBytes = (UInt32)(sizeof(Falcon::ShapeVertex) * Min(uVertexCount, m_Settings.m_uVertexBufferSizeInVertices));
#if !SEOUL_SHIP
		if (m_Settings.m_uVertexBufferSizeInVertices < uVertexCount)
		{
			SEOUL_WARN("UIDrawer: Out of vertex buffer space, have %u vertices, need %u vertices", (UInt32)m_Settings.m_uVertexBufferSizeInVertices, (UInt32)uVertexCount);
		}
#endif // /#if !SEOUL_SHIP
		void* pOutVertices = m_pBuilder->LockVertexBuffer(m_pVertices.GetPtr(), uSizeInBytes);
		memcpy(pOutVertices, pVertices, uSizeInBytes);
		m_pBuilder->UnlockVertexBuffer(m_pVertices.GetPtr());
	}

	m_pBuilder->DrawIndexedPrimitive(
		PrimitiveType::kTriangleList,
		0,
		0,
		Min(uVertexCount, m_Settings.m_uVertexBufferSizeInVertices),
		0,
		Min(uIndexCount, m_Settings.m_uIndexBufferSizeInIndices) / 3);
}

void Drawer::ProcessDraw(
	DrawerState& rState,
	CheckedPtr<RenderCommandStreamBuilder> pBuilder,
	CheckedPtr<RenderPass> pPass)
{
	m_pState = &rState;
	m_pBuilder = pBuilder;
	m_pPass = pPass;

	// Update our two pass shadowing settings based on current device state.
	m_bTwoPassShadows =
		RenderDevice::Get()->GetCaps().m_bBlendMinMax &&
		RenderDevice::Get()->GetCaps().m_bBackBufferWithAlpha;

#if SEOUL_ENABLE_CHEATS
	m_pDrawer->SetMode(m_eRendererMode);
#endif // /#if SEOUL_ENABLE_CHEATS

	InternalPerformDraw();

	// Flush buffers.
	m_pState->Reset();

	m_pPass.Reset();
	m_pBuilder.Reset();
	m_pState.Reset();
}

void Drawer::UnPack(Falcon::PackerTree2D::NodeID nodeID)
{
	// Always remove from the pack nodes table.
	Bool const bErased = m_tPackNodes.Erase(nodeID);
	(void)bErased;

	// Also remove any pending entries, if they exist.
	Bool bPending = false;
	for (auto i = m_vPackOps.Begin(); m_vPackOps.End() != i; )
	{
		if (i->m_NodeID == nodeID)
		{
			i = m_vPackOps.Erase(i);
			bPending = true;
		}
		else
		{
			++i;
		}
	}

	// One or the other must have been true.
	SEOUL_ASSERT(bPending || bErased);
}

#if SEOUL_ENABLE_CHEATS
Bool Drawer::GetDebugEnableOverfillOptimizer() const
{
	return m_pDrawer->GetDebugEnableOverfillOptimizer();
}

void Drawer::SetDebugEnableOverfillOptimizer(Bool bEnable)
{
	m_pDrawer->SetDebugEnableOverfillOptimizer(bEnable);
}
#endif // /#if SEOUL_ENABLE_CHEATS

void Drawer::InternalCommitActiveViewport(const Viewport& activeViewport)
{
	// Flush before making changes.
	m_pDrawer->Flush();

	// Set the viewport to the device.
	m_pBuilder->SetCurrentViewport(activeViewport);
	m_pBuilder->SetScissor(true, ToClearSafeScissor(activeViewport));
}

void Drawer::InternalCommitViewProjection(const Vector4D& vViewProjection)
{
	// Flush before making changes.
	m_pDrawer->Flush();

	// Apply the view projection to state.
	m_pState->m_rState.m_vViewProjectionTransform = vViewProjection;

	// TODO: This code is assuming perspective factor does not
	// change over the course of a frame. Enforce this.

	// Commit the new view projection transform.
	m_pDrawEffect->SetVector4DParameter(kViewProjectionTransform, vViewProjection);
	m_pDrawEffect->SetVector4DParameter(kPerspective, Vector4D(m_pState->m_rState.GetPerspectiveFactor(), 0.0f, 0.0f, 0.0f));
}

/**
 * Called on BeginFrame(). On reset events, restores the
 * current table of packed nodes to the packing texture.
 */
void Drawer::InternalCheckPackNodes()
{
	// Cache the render target.
	auto pTarget(((AtlasTexture*)m_pState->m_rState.m_pCache->GetPackerTexture().GetPtr())->GetTarget());

	// Check for target reset events. When these occur, we must reinsert all currently
	// registered pack nodes.
	if (pTarget->GetResetCount() == m_LastPackTargetReset)
	{
		return;
	}

	// Unset ready - all packed nodes are no longer packed for a frame.
	for (auto p = m_pState->m_rState.m_pCache->GetList().GetHeadPacked(); nullptr != p; p = p->GetNextPacked())
	{
		// This packing is no longer ready for render.
		p->SetPackReady(false);
	}

	// Insert all existing nodes as ops, then rerun them.
	{
		auto const iBegin = m_tPackNodes.Begin();
		auto const iEnd = m_tPackNodes.End();
		m_vPackOps.Reserve(m_tPackNodes.GetSize());
		for (auto i = iBegin; iEnd != i; ++i)
		{
			m_vPackOps.PushBack(i->Second);
		}
	}

	// Clear and re-run.
	m_tPackNodes.Clear();
	InternalProcessPackOps();

	// Done, and now up-to-date.
	m_LastPackTargetReset = pTarget->GetResetCount();
}

/**
 * Standard process - pending packs are added to the pack
 * texture once per frame, on end of frame.
 */
void Drawer::InternalProcessPackOps()
{
	// Early out if no ops to process.
	if (m_vPackOps.IsEmpty())
	{
		return;
	}

	// Set the pack effect as active - restore on terminate.
	auto const scoped(MakeScopedAction([&]() { m_pDrawEffect = &m_PackEffect; }, [&]() { m_pDrawEffect = &m_RenderEffect; }));

	// Check acquire all textures, make sure they're valid.
	UInt32 const uOps = m_vPackOps.GetSize();
	m_vPackAcquired.Clear();
	m_vPackAcquired.Resize(uOps);
	for (UInt32 i = 0u; i < uOps; ++i)
	{
		auto const& e = m_vPackOps[i];
		m_vPackAcquired[i] = ((Texture*)e.m_pSource.GetPtr())->GetTextureContentHandle().GetPtr();
		if (!m_vPackAcquired[i].IsValid() || e.m_pSource->IsLoading())
		{
			// Failed to acquire a texture, clear the acquire set
			// and restore the effect.
			m_vPackAcquired.Clear();
			return;
		}
	}

	// Cache the depth and targets.
	auto pDepth(((AtlasTexture*)m_pState->m_rState.m_pCache->GetPackerTexture().GetPtr())->GetDepth());
	auto pTarget(((AtlasTexture*)m_pState->m_rState.m_pCache->GetPackerTexture().GetPtr())->GetTarget());

	// TODO: Use RenderSurface2D here or refactor RenderSurface2D
	// so it can be used here.

	// Configure the surface - setup the viewport for the target.
	m_pBuilder->SelectDepthStencilSurface(pDepth.GetPtr());
	m_pBuilder->SelectRenderTarget(pTarget.GetPtr());
	m_pBuilder->CommitRenderSurface();
	auto const viewport(Viewport::Create(
		pTarget->GetWidth(),
		pTarget->GetHeight(),
		0,
		0,
		pTarget->GetWidth(),
		pTarget->GetHeight()));
	m_pBuilder->SetCurrentViewport(viewport);
	m_pBuilder->SetScissor(true, viewport);

	// Start the packing effect.
	InternalSetupVertexFormat(false);
	m_StateEffect.SetActiveTechnique(); // Pack sets state for packing, disable state effect.
	m_pDrawEffect->SetActiveTechnique(true, kPackTechnique);

	m_pBuilder->SetIndices(m_pIndices.GetPtr());
	m_pBuilder->SetVertices(0, m_pVertices.GetPtr(), 0, sizeof(Falcon::ShapeVertex));
	m_pBuilder->SetVertices(1, m_pDepths3D.GetPtr(), 0, sizeof(Float32));

	// Setup indices - these will always be the same.
	FixedArray<UInt16, 6> aIndices;
	aIndices[0] = 0; aIndices[1] = 1; aIndices[2] = 2;
	aIndices[3] = 0; aIndices[4] = 2; aIndices[5] = 3;

	// Vertices, setup per draw.
	FixedArray<Falcon::ShapeVertex, 4> aVertices;

	// Cache the target dimensions, shared across all calculations.
	UInt32 const uTargetWidth = (UInt32)pTarget->GetWidth();
	UInt32 const uTargetHeight = (UInt32)pTarget->GetHeight();
	for (UInt32 i = 0u; i < uOps; ++i)
	{
		// Get the entry from both lists.
		auto const& pTexture = m_vPackAcquired[i];
		auto const& e = m_vPackOps[i];

		// Compute the source we're copying from and convenience variable
		// for padding.
		Int32 const iVisibleWidth = (e.m_SourceRectangle.m_iRight - e.m_SourceRectangle.m_iLeft);
		Int32 const iVisibleHeight = (e.m_SourceRectangle.m_iBottom - e.m_SourceRectangle.m_iTop);
		Int32 const iPad = Falcon::TexturePacker::kBorder;

		// The quad is oversized by 1 pixel on each side for padding. Otherwise, it
		// maps the visible subregion we're copying to the appropriately sized
		// rectangle in the atlus texture.
		aVertices[0].m_vP.X = (Float)(e.m_Destination.X - iPad) / (Float)uTargetWidth;
		aVertices[0].m_vP.Y = (Float)(e.m_Destination.Y - iPad) / (Float)uTargetHeight;
		aVertices[0].m_vT.X = (Float)(e.m_SourceRectangle.m_iLeft - iPad) / (Float)pTexture->GetWidth();
		aVertices[0].m_vT.Y = (Float)(e.m_SourceRectangle.m_iTop - iPad) / (Float)pTexture->GetHeight();
		aVertices[1].m_vP.X = (Float)(e.m_Destination.X - iPad) / (Float)uTargetWidth;
		aVertices[1].m_vP.Y = (Float)(e.m_Destination.Y + iVisibleHeight + iPad) / (Float)uTargetHeight;
		aVertices[1].m_vT.X = (Float)(e.m_SourceRectangle.m_iLeft - iPad) / (Float)pTexture->GetWidth();
		aVertices[1].m_vT.Y = (Float)(e.m_SourceRectangle.m_iBottom + iPad) / (Float)pTexture->GetHeight();
		aVertices[2].m_vP.X = (Float)(e.m_Destination.X + iVisibleWidth + iPad) / (Float)uTargetWidth;
		aVertices[2].m_vP.Y = (Float)(e.m_Destination.Y + iVisibleHeight + iPad) / (Float)uTargetHeight;
		aVertices[2].m_vT.X = (Float)(e.m_SourceRectangle.m_iRight + iPad) / (Float)pTexture->GetWidth();
		aVertices[2].m_vT.Y = (Float)(e.m_SourceRectangle.m_iBottom + iPad) / (Float)pTexture->GetHeight();
		aVertices[3].m_vP.X = (Float)(e.m_Destination.X + iVisibleWidth + iPad) / (Float)uTargetWidth;
		aVertices[3].m_vP.Y = (Float)(e.m_Destination.Y - iPad) / (Float)uTargetHeight;
		aVertices[3].m_vT.X = (Float)(e.m_SourceRectangle.m_iRight + iPad) / (Float)pTexture->GetWidth();
		aVertices[3].m_vT.Y = (Float)(e.m_SourceRectangle.m_iTop - iPad) / (Float)pTexture->GetHeight();

		// Draw the quad to commit it to the atlas.
		DrawTriangleListRI(
			e.m_pSource,
			SharedPtr<Falcon::Texture>(),
			aIndices.Data(),
			aIndices.GetSize(),
			nullptr,
			aVertices.Data(),
			aVertices.GetSize(),
			Falcon::Render::Features());

		// Now that we're done, add it to our tracking tree.
		SEOUL_VERIFY(m_tPackNodes.Insert(e.m_NodeID, e).Second);
	}

	// Done with the effect, terminate it.
	m_pDrawEffect->SetActiveTechnique();

	// Restore the surface for the overall render pass.
	m_pPass->GetSurface()->Select(*m_pBuilder);

	// Cleanup our tracking buffers.
	m_vPackOps.Clear();
	m_vPackAcquired.Clear();

	// Make all valid pack nodes as ready.
	for (auto p = m_pState->m_rState.m_pCache->GetList().GetHeadPacked(); nullptr != p; p = p->GetNextPacked())
	{
		if (m_tPackNodes.HasValue(p->GetPackedNodeID()))
		{
			// This packing is now ready for render.
			p->SetPackReady(true);
		}
	}
}

/** Given features that describe an extended blend mode, map that to the appropriate state effect technique name. */
HString Drawer::GetExtendedBlendModeTechniqueName(
	const Falcon::Render::Features& features) const
{
	SEOUL_ASSERT(features.NeedsExtendedBlendMode());

	auto const uIndex = Falcon::Render::Feature::ExtendedToIndex(features.GetBits());
	return m_aExtendedBlendModeTechniques[uIndex];
}

void Drawer::InternalSetupStateTechnique(
	const Falcon::Render::Features& features)
{
	// Ignore if packing.
	if (&m_PackEffect == m_pDrawEffect)
	{
		return;
	}

	// Ignore if feature locked. Feature locking occurs
	// in certain passes (e.g. render-to-texture packing),
	// which locks the render effect to the global effect
	// for that mode.
	if (m_StateEffect.FeatureLocked())
	{
		return;
	}

	// Always default to the standard technique.
	HString targetTechnique = kStateTechnique_Default;

	// If an extended mode, map.
	if (features.NeedsExtendedBlendMode())
	{
		// Resolve the technique.
		targetTechnique = GetExtendedBlendModeTechniqueName(features);
	}

	// Update.
	m_StateEffect.SetActiveTechnique(false, targetTechnique);
}

void Drawer::InternalSetupEffectTechnique(
	const Falcon::Render::Features& features,
	const SharedPtr<Falcon::Texture>& pColorTex,
	Bool b3D)
{
	// Early out if packing, the technique does not change.
	auto const active = m_pDrawEffect->GetActiveTechnique();
	if (active == kPackTechnique)
	{
		return;
	}

	// Techniques to use.
	auto const& t = (b3D ? kTechniques3D : kTechniques2D);

	// Technique we will use for rendering.
	HString desiredEffectTechnique;

#if SEOUL_ENABLE_CHEATS
	// If overdraw mode is enabled, use the overdraw technique.
	if (Falcon::Render::Mode::kOverdraw == m_eRendererMode)
	{
		// Detail texture, most expensive.
		if (features.NeedsDetail())
		{
			desiredEffectTechnique = t.kDetailOverdraw;
		}
		// Alpha shape, variations (either most expensive, or explicitly alpha
		// shape only).
		else if (features.NeedsAlphaShape())
		{
			if (features.NeedsColorAdd())
			{
				desiredEffectTechnique = t.kAllFeaturesOverdraw;
			}
			else
			{
				desiredEffectTechnique = t.kAlphaShapeOverdraw;
			}
		}
		// Typical cases.
		else if (features.NeedsColorAdd())
		{
			desiredEffectTechnique = t.kColorMultiplyAddOverdraw;
		}
		else
		{
			desiredEffectTechnique = t.kColorMultiplyOverdraw;
		}
	}
	// Otherwise, figure out the desired technique based on a number of factors.
	else
#endif // /#if SEOUL_ENABLE_CHEATS
	{
		// Default to the full (most expensive) technique.
		desiredEffectTechnique = t.kColorMultiplyAdd;

		// Determine if the texture that will be use for rendering
		// requires its secondary texture to render correctly. If not,
		// we use a less costly shader to render.
		Bool bSecondary = false;
		{
			auto const hTexture = (pColorTex.IsValid()
				? ((Texture*)pColorTex.GetPtr())->GetTextureContentHandle()
				: TextureContentHandle());
			SharedPtr<BaseTexture> pTexture(hTexture.GetPtr());
			if (pTexture.IsValid())
			{
				bSecondary = pTexture->NeedsSecondaryTexture();
			}
		}

		// Select the appropriate effect technique based on the
		// renderer material necessary to properly render the batch
		// as provided by Falcon.

		// Detail is first, since it is the "super" technique - it includes
		// all features of all other techniques.
		if (features.NeedsDetail())
		{
			if (bSecondary)
			{
				desiredEffectTechnique = t.kAllFeaturesDetailSecondaryTexture;
			}
			else
			{
				desiredEffectTechnique = t.kAllFeaturesDetail;
			}
		}
		// Next comes alpha shape, which can vary based on other required
		// features.
		else if (features.NeedsAlphaShape())
		{
			// This is the most expensive option. The draw call has
			// a mix of additive color and alpha shape styling
			// (not in the same vertex, as the two are mutually exclusive,
			// but across several vertices).
			if (features.NeedsColorAdd())
			{
				if (bSecondary)
				{
					desiredEffectTechnique = t.kAllFeaturesSecondaryTexture;
				}
				else
				{
					desiredEffectTechnique = t.kAllFeatures;
				}
			}
			// This is the standard alpha shape shader, which can handle
			// both alpha shaping and standard alpha blending.
			else
			{
				if (bSecondary)
				{
					desiredEffectTechnique = t.kAlphaShapeSecondaryTexture;
				}
				else
				{
					desiredEffectTechnique = t.kAlphaShape;
				}
			}
		}
		// This batch needs a shader that supports the additive color
		// term but not alpha shaping.
		else if (features.NeedsColorAdd())
		{
			if (bSecondary)
			{
				desiredEffectTechnique = t.kColorMultiplyAddSecondaryTexture;
			}
			else
			{
				desiredEffectTechnique = t.kColorMultiplyAdd;
			}
		}
		// This is the simplest shader option - color multiply or just
		// color only.
		else
		{
			// We don't currently separate kColor and kColorMultiply - on most hardware,
			// a texture read + alu is the same cycles/cost as just a texture read.
			if (bSecondary)
			{
				desiredEffectTechnique = t.kColorMultiplySecondaryTexture;
			}
			else
			{
				desiredEffectTechnique = t.kColorMultiply;
			}
		}

		// Overide when rendering twopass shadows.
		if (m_bInTwoPassShadowRender)
		{
			if (bSecondary)
			{
				desiredEffectTechnique = t.kShadowTwoPassSecondaryTexture;
			}
			else
			{
				desiredEffectTechnique = t.kShadowTwoPass;
			}
		}

		// Extended blend modes are always just color
		// multiply as the basic effect technique, with
		// the exception of kExtended_ColorAlphaShape
		if (features.NeedsExtendedBlendMode())
		{
			if (features.NeedsExtendedColorAlphaShape())
			{
				if (bSecondary)
				{
					desiredEffectTechnique = t.kExtendedColorAlphaShapeSecondaryTexture;
				}
				else
				{
					desiredEffectTechnique = t.kExtendedColorAlphaShape;
				}
			}
			else if (bSecondary)
			{
				desiredEffectTechnique = t.kColorMultiplySecondaryTexture;
			}
			else
			{
				desiredEffectTechnique = t.kColorMultiply;
			}
		}
	}

	// Update.
	m_pDrawEffect->SetActiveTechnique(false, desiredEffectTechnique);
}

void Drawer::InternalSetupVertexFormat(Bool b3D)
{
	if (b3D)
	{
		if (m_pActiveVertexFormat != m_pVertexFormat3D)
		{
			m_pBuilder->UseVertexFormat(m_pVertexFormat3D.GetPtr());
			m_pActiveVertexFormat = m_pVertexFormat3D;
		}
	}
	else
	{
		if (m_pActiveVertexFormat != m_pVertexFormat2D)
		{
			m_pBuilder->UseVertexFormat(m_pVertexFormat2D.GetPtr());
			m_pActiveVertexFormat = m_pVertexFormat2D;
		}
	}
}

Viewport Drawer::InternalWorldToScissorViewport(const Falcon::Rectangle& world) const
{
	// Cache scaling.
	auto const fX = m_pState->m_rState.m_fWorldWidthToScreenWidth;
	auto const fY = m_pState->m_rState.m_fWorldHeightToScreenHeight;
	auto const worldRect = m_pState->m_rState.m_WorldCullRectangle;

	// Start with the rendering viewport.
	auto const render = m_pBuilder->GetCurrentViewport();

	// Generate a clamp rectangle to simplify clamping.
	auto const clampRect = Falcon::Rectangle::Create(
		(Float)render.m_iViewportX,
		(Float)(render.m_iViewportX + render.m_iViewportWidth),
		(Float)render.m_iViewportY,
		(Float)(render.m_iViewportY + render.m_iViewportHeight));

	// Rescale.
	auto const rect = Falcon::Rectangle::Create(
		Clamp((world.m_fLeft - worldRect.m_fLeft) * fX + clampRect.m_fLeft, clampRect.m_fLeft, clampRect.m_fRight),
		Clamp((world.m_fRight - worldRect.m_fLeft) * fX + clampRect.m_fLeft, clampRect.m_fLeft, clampRect.m_fRight),
		Clamp((world.m_fTop - worldRect.m_fTop) * fY + clampRect.m_fTop, clampRect.m_fTop, clampRect.m_fBottom),
		Clamp((world.m_fBottom - worldRect.m_fTop) * fY + clampRect.m_fTop, clampRect.m_fTop, clampRect.m_fBottom));

	// Generate - initialize with the render viewport, then update.
	auto ret = render;
	ret.m_iViewportX  = Max(ret.m_iViewportX, (Int32)Floor(rect.m_fLeft));
	ret.m_iViewportY = Max(ret.m_iViewportY, (Int32)Floor(rect.m_fTop));
	ret.m_iViewportWidth = Min(ret.m_iViewportWidth, (Int32)Ceil(rect.GetWidth()));
	ret.m_iViewportHeight = Min(ret.m_iViewportHeight, (Int32)Ceil(rect.GetHeight()));
	return ret;
}

Drawer::EffectUtil::EffectUtil(const EffectContentHandle& h)
	: m_hEffect(h)
{
}

Drawer::EffectUtil::~EffectUtil()
{
}

Bool Drawer::EffectUtil::Acquire(RenderCommandStreamBuilder& r)
{
	SEOUL_ASSERT(!m_pAcquired.IsValid());
	m_pAcquired = m_hEffect.GetPtr();

	if (m_pAcquired.IsValid() && BaseGraphicsObject::kDestroyed == m_pAcquired->GetState())
	{
		m_pAcquired.Reset();
		return false;
	}

	if (m_pAcquired.IsValid())
	{
		m_p = &r;
		return true;
	}

	return false;
}

void Drawer::EffectUtil::Release()
{
	SEOUL_ASSERT(m_pAcquired.IsValid());

	SetActiveTechnique(false, HString());
	SEOUL_ASSERT(m_ActiveTechnique.IsEmpty());

	m_p.Reset();
	m_pAcquired.Reset();
}

void Drawer::EffectUtil::SetActiveTechnique(Bool bFeatureLocked, HString name)
{
	SEOUL_ASSERT(m_pAcquired.IsValid());
	SEOUL_ASSERT(m_p.IsValid());

	// Always record feature locking.
	m_bFeatureLocked = bFeatureLocked;

	// If we're already on this technique, return immediately.
	if (m_ActiveTechnique == name)
	{
		return;
	}

	// Terminate any existing technique.
	if (!m_ActiveTechnique.IsEmpty())
	{
		// Make sure we clear the active textures before finalizing.
		if (m_bHasDetailTexture)
		{
			m_p->SetTextureParameter(m_pAcquired, kDetailTexture, TextureContentHandle());
			m_bHasDetailTexture = false;
		}
		if (m_bHasColorTexture)
		{
			m_p->SetTextureParameter(m_pAcquired, kColorTexture, TextureContentHandle());
			m_bHasColorTexture = false;
		}

		m_p->CommitEffectPass(m_pAcquired, m_Pass);
		m_p->EndEffectPass(m_pAcquired, m_Pass);
		m_Pass = EffectPass();
		m_p->EndEffect(m_pAcquired);
		m_ActiveTechnique = HString();
	}

	// Assign.
	m_ActiveTechnique = name;

	// Start if not empty.
	if (!m_ActiveTechnique.IsEmpty())
	{
		m_Pass = m_p->BeginEffect(m_pAcquired, m_ActiveTechnique);
		m_p->BeginEffectPass(m_pAcquired, m_Pass);
	}
}

/**
 * Update the color texture - common. This is the main texture used by
 * all drawing operations of Falcon.
 */
void Drawer::EffectUtil::SetColorTexture(const TextureContentHandle& hTexture)
{
	SEOUL_ASSERT(m_pAcquired.IsValid());
	SEOUL_ASSERT(m_p.IsValid());

	m_p->SetTextureParameter(m_pAcquired, kColorTexture, hTexture);
	m_bHasColorTexture = hTexture.IsInternalPtrValid();

	if (m_Pass.IsValid())
	{
		m_p->CommitEffectPass(m_pAcquired, m_Pass);
	}
}

/**
 * Detail texture - this is a secondary texture, with wrap mode set to "repeat",
 * that is modulated against the base texture to provide additional surface
 * variation. Currently used for face texturing on text.
 */
void Drawer::EffectUtil::SetDetailTexture(const TextureContentHandle& hTexture)
{
	SEOUL_ASSERT(m_pAcquired.IsValid());
	SEOUL_ASSERT(m_p.IsValid());

	m_p->SetTextureParameter(m_pAcquired, kDetailTexture, hTexture);
	m_bHasDetailTexture = hTexture.IsInternalPtrValid();

	if (m_Pass.IsValid())
	{
		m_p->CommitEffectPass(m_pAcquired, m_Pass);
	}
}

void Drawer::EffectUtil::SetVector4DParameter(HString name, const Vector4D& v, Bool bCommit /*= false*/)
{
	SEOUL_ASSERT(m_pAcquired.IsValid());
	SEOUL_ASSERT(m_p.IsValid());

	m_p->SetVector4DParameter(m_pAcquired, name, v);
	if (bCommit && m_Pass.IsValid())
	{
		m_p->CommitEffectPass(m_pAcquired, m_Pass);
	}
}

#if !SEOUL_SHIP
/** Validate that effects have the necessary techniques and parameters. */
void Drawer::ValidateEffects()
{
	if (m_bValidated) { return; }

	m_bValidated = true; // Done.

	// Check that each effect has expected techniques.

	// State effect
	{
		SEOUL_ASSERT(m_StateEffect.GetAcquired()->HasTechniqueWithName(kShadowAccumulateState));
		SEOUL_ASSERT(m_StateEffect.GetAcquired()->HasTechniqueWithName(kShadowApplyState));
		SEOUL_ASSERT(m_StateEffect.GetAcquired()->HasTechniqueWithName(kStateTechnique_Default));
#if SEOUL_ENABLE_CHEATS
		SEOUL_ASSERT(m_StateEffect.GetAcquired()->HasTechniqueWithName(kInputVisualizationStateTechnique));
#endif

		for (auto const& e : m_aExtendedBlendModeTechniques)
		{
			SEOUL_ASSERT_MESSAGE(m_StateEffect.GetAcquired()->HasTechniqueWithName(e), String::Printf("Missing technique '%s'", e.CStr()).CStr());
		}
	}

	// Draw effect.
	SEOUL_STATIC_ASSERT(sizeof(kTechniques2D) % sizeof(HString) == 0);
	for (HString const* p = (HString const*)&kTechniques2D, *pEnd = p + (sizeof(kTechniques2D) / sizeof(HString)); p < pEnd; ++p)
	{
		SEOUL_ASSERT_MESSAGE(m_RenderEffect.GetAcquired()->HasTechniqueWithName(*p), String::Printf("Missing technique '%s'", p->CStr()).CStr());
	}
	SEOUL_STATIC_ASSERT(sizeof(kTechniques3D) % sizeof(HString) == 0);
	for (HString const* p = (HString const*)&kTechniques3D, *pEnd = p + (sizeof(kTechniques3D) / sizeof(HString)); p < pEnd; ++p)
	{
		SEOUL_ASSERT_MESSAGE(m_RenderEffect.GetAcquired()->HasTechniqueWithName(*p), String::Printf("Missing technique '%s'", p->CStr()).CStr());
	}

	// Pack effect.
	SEOUL_ASSERT(m_PackEffect.GetAcquired()->HasTechniqueWithName(kPackTechnique));
}
#endif

} // namespace UI

} // namespace Seoul
