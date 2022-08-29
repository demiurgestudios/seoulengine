/**
 * \file UILetterboxPass.cpp
 * \brief A poseable used to wrap UILetterbox.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDataStoreTableUtil.h"
#include "RenderCommandStreamBuilder.h"
#include "RenderPass.h"
#include "UILetterbox.h"
#include "UILetterboxPass.h"

namespace Seoul::UI
{

IPoseable* LetterboxPass::SpawnUILetterboxPass(
	const DataStoreTableUtil& configSection,
	Bool& rbRenderPassOwnsPoseableObject)
{
	auto pReturn = SEOUL_NEW(MemoryBudgets::Rendering) UI::LetterboxPass(configSection);
	rbRenderPassOwnsPoseableObject = true;
	return pReturn;
}

static const HString ksLetterboxingEnabledOnPC("LetterboxingEnabledOnPC");
static const HString ksLetterboxingEffect("LetterboxingEffect");
static const HString ksLetterboxingBaseTop("LetterboxingBaseTop");
static const HString ksPillarboxingBaseLeft("PillarboxingBaseLeft");

LetterboxPass::LetterboxPass(const DataStoreTableUtil& configSection)
{
	LetterboxSettings settings;
	(void)configSection.GetValue(ksLetterboxingEnabledOnPC, settings.m_bLetterboxingEnabledOnPC);
	(void)configSection.GetValue(ksLetterboxingEffect, settings.m_EffectFilePath);
	(void)configSection.GetValue(ksLetterboxingBaseTop, settings.m_LetterFilePath);
	(void)configSection.GetValue(ksPillarboxingBaseLeft, settings.m_PillarFilePath);
	m_pLetterbox.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) Letterbox(settings));
}

LetterboxPass::~LetterboxPass()
{
}

void LetterboxPass::Pose(
	Float fDeltaTime,
	RenderPass& rPass,
	IPoseable* pParent)
{
	RenderCommandStreamBuilder& rBuilder = *rPass.GetRenderCommandStreamBuilder();
	BeginPass(rBuilder, rPass);
	m_pLetterbox->Draw(rBuilder, rPass);
	EndPass(rBuilder, rPass);
}

} // namespace Seoul::UI
