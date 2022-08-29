/**
 * \file FalconRenderState.cpp
 * Shared state across Drawer, Poser, and Optimizer.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconClipper.h"
#include "FalconRenderState.h"
#include "FalconScalingGrid.h"
#include "FalconStage3DSettings.h"
#include "FalconTextureCache.h"

namespace Seoul::Falcon
{

namespace Render
{

State::State(const StateSettings& settings)
	: m_Settings(settings)
	, m_pCache(SEOUL_NEW(MemoryBudgets::Falcon) TextureCache(settings.m_pInterface, settings.m_CacheSettings))
	, m_pClipStack(SEOUL_NEW(MemoryBudgets::Falcon) ClipStack)
	, m_pStage3DSettings(SEOUL_NEW(MemoryBudgets::Falcon) Stage3DSettings)
	, m_Buffer()
	, m_iInPlanarShadowRender(0)
	, m_iInDeferredDrawingRender(0)
	, m_vViewProjectionTransform(1, 1, 0, 0)
	, m_WorldCullRectangle(Rectangle::Max())
	, m_fWorldWidthToScreenWidth(1.0f)
	, m_fWorldHeightToScreenHeight(1.0f)
	, m_fMaxCostInBatchFromOverfill(DoubleMax)
	, m_fWorldCullScreenArea(FloatMax)
	, m_fRawDepth3D(0.0f)
	, m_iIgnoreDepthProjection(0)
	, m_fPerspectiveFactorAdjustment(0.0f)
	, m_fStage3DTopY(0.0f)
	, m_fStage3DBottomY(0.0f)
{
}

State::~State()
{
}

/**
 * Expected to be called on completion of a Pose or Draw phase.
 * Resets phase specific state so it does not linger between
 * the two.
 */
void State::EndPhase()
{
	m_iInPlanarShadowRender = 0;
	m_iInDeferredDrawingRender = 0;
	m_fRawDepth3D = 0.0f;
	m_iIgnoreDepthProjection = 0;
	m_pClipStack->Clear();
}

Float State::GetPerspectiveFactor() const
{
	return Clamp(m_pStage3DSettings->m_Perspective.m_fFactor + m_fPerspectiveFactorAdjustment, 0.0f, 0.99f);
}

} // namespace Render

} // namespace Seoul::Falcon
