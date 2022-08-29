/**
 * \file FalconTextChunk.cpp
 * \brief The smallest unit of text data that can be submitted
 * to the Falcon render backend.
 *
 * Each text chunk has size, style, and character data that fully
 * defines its contents. A single text chunk can have only one style
 * and one size. Style and size changes require the generation of
 * a new text chunk.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconGlobalConfig.h"
#include "FalconTextChunk.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_SPEC_TEMPLATE_TYPE(CheckedPtr<Falcon::TextEffectSettings>)
SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, CheckedPtr<Falcon::TextEffectSettings>, 50, DefaultHashTableKeyTraits<HString>>)

SEOUL_BEGIN_ENUM(Falcon::TextEffectDetailMode)
	SEOUL_ENUM_N("Word", Falcon::TextEffectDetailMode::Word)
	SEOUL_ENUM_N("Character", Falcon::TextEffectDetailMode::Character)
SEOUL_END_ENUM()

SEOUL_BEGIN_ENUM(Falcon::TextEffectDetailStretchMode)
	SEOUL_ENUM_N("Stretch", Falcon::TextEffectDetailStretchMode::Stretch)
	SEOUL_ENUM_N("FitWidth", Falcon::TextEffectDetailStretchMode::FitWidth)
	SEOUL_ENUM_N("FitHeight", Falcon::TextEffectDetailStretchMode::FitHeight)
SEOUL_END_ENUM()

SEOUL_BEGIN_TYPE(Falcon::TextEffectSettings, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("Color", m_pTextColor)
	SEOUL_PROPERTY_N("ColorTop", m_pTextColorTop)
	SEOUL_PROPERTY_N("ColorBottom", m_pTextColorBottom)
	SEOUL_PROPERTY_N("ShadowOffset", m_vShadowOffset)
	SEOUL_PROPERTY_N("ShadowColor", m_ShadowColor)
	SEOUL_PROPERTY_N("ShadowBlur", m_uShadowBlur)
	SEOUL_PROPERTY_N("ShadowOutlineWidth", m_uShadowOutlineWidth)
	SEOUL_PROPERTY_N("ShadowEnable", m_bShadowEnable)
	SEOUL_PROPERTY_N("ExtraOutlineOffset", m_vExtraOutlineOffset)
	SEOUL_PROPERTY_N("ExtraOutlineColor", m_ExtraOutlineColor)
	SEOUL_PROPERTY_N("ExtraOutlineBlur", m_uExtraOutlineBlur)
	SEOUL_PROPERTY_N("ExtraOutlineWidth", m_uExtraOutlineWidth)
	SEOUL_PROPERTY_N("ExtraOutlineEnable", m_bExtraOutlineEnable)
	SEOUL_PROPERTY_N("DetailMode", m_eDetailMode)
	SEOUL_PROPERTY_N("DetailStretchMode", m_eDetailStretchMode)
	SEOUL_PROPERTY_N("DetailOffset", m_vDetailOffset)
	SEOUL_PROPERTY_N("DetailFilePath", m_DetailFilePath)
	SEOUL_PROPERTY_N("DetailSpeed", m_vDetailSpeed)
	SEOUL_PROPERTY_N("Detail", m_bDetail)
SEOUL_END_TYPE()

namespace Falcon
{

/**
 * Vertex bounds when rendering - use for rendering bounding
 * box.
 */
Rectangle TextChunk::ComputeRenderBounds() const
{
	auto const fBorder = (Float)kiRadiusSDF * (m_Format.GetTextHeight() / kfGlyphHeightSDF);

	auto ret = ComputeGlyphBounds();
	ret.Expand(fBorder);

	// Also, if the text chunk has settings configured, need to expand the render rectangle
	// by any outline/extra outline offset (e.g. the outline has the same render dimensions
	// as the base, so it only enlarges the rectangle if there is an offset).
	if (!m_Format.m_TextEffectSettings.IsEmpty())
	{
		if (auto pSettings = g_Config.m_pGetTextEffectSettings(m_Format.m_TextEffectSettings))
		{
			// X
			if (pSettings->m_bShadowEnable)
			{
				// X
				if (pSettings->m_vShadowOffset.X > 0.0f)
				{
					ret.m_fRight += pSettings->m_vShadowOffset.X;
				}
				else if (pSettings->m_vShadowOffset.X < 0.0f)
				{
					ret.m_fLeft += pSettings->m_vShadowOffset.X;
				}
				// Y
				if (pSettings->m_vShadowOffset.Y > 0.0f)
				{
					ret.m_fBottom += pSettings->m_vShadowOffset.Y;
				}
				else if (pSettings->m_vShadowOffset.Y < 0.0f)
				{
					ret.m_fTop += pSettings->m_vShadowOffset.Y;
				}
			}
			if (pSettings->m_bExtraOutlineEnable)
			{
				// X
				if (pSettings->m_vExtraOutlineOffset.X > 0.0f)
				{
					ret.m_fRight += pSettings->m_vExtraOutlineOffset.X;
				}
				else if (pSettings->m_vExtraOutlineOffset.X < 0.0f)
				{
					ret.m_fLeft += pSettings->m_vExtraOutlineOffset.X;
				}
				// Y
				if (pSettings->m_vExtraOutlineOffset.Y > 0.0f)
				{
					ret.m_fBottom += pSettings->m_vExtraOutlineOffset.Y;
				}
				else if (pSettings->m_vExtraOutlineOffset.Y < 0.0f)
				{
					ret.m_fTop += pSettings->m_vExtraOutlineOffset.Y;
				}
			}
		}
	}

	return ret;
}

} // namespace Falcon

} // namespace Seoul
