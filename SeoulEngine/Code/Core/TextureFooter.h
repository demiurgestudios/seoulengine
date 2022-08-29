/**
 * \file TextureFooter.h
 * \brief Part of the SeoulEngine DDS container spec for
 * storing texture data. Metadata footer.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef TEXTURE_FOOTER_H
#define TEXTURE_FOOTER_H

#include "Prereqs.h"

namespace Seoul
{

/** Current signature of the TextureFooter. */
static const UInt32 kTextureFooterSignature = 0x5E0F7E37;

/** Current version of the TextureFooter. */
static const UInt32 kTextureFooterVersion = 3u;

/** TextureFooter, structure at the end of texture file data. */
SEOUL_DEFINE_PACKED_STRUCT(struct TextureFooter
	{
		static TextureFooter Default()
		{
			TextureFooter ret;
			ret.m_uSignature = kTextureFooterSignature;
			ret.m_uVersion = kTextureFooterVersion;
			ret.m_fTexcoordsScaleU = 1.0f;
			ret.m_fTexcoordsScaleV = 1.0f;
			ret.m_fOcclusionRegionScaleU = 0.0f;
			ret.m_fOcclusionRegionScaleV = 0.0f;
			ret.m_fOcclusionRegionOffsetU = 0.0f;
			ret.m_fOcclusionRegionOffsetV = 0.0f;
			ret.m_fVisibleRegionScaleU = 1.0f;
			ret.m_fVisibleRegionScaleV = 1.0f;
			ret.m_fVisibleRegionOffsetU = 0.0f;
			ret.m_fVisibleRegionOffsetV = 0.0f;
			return ret;
		}

		UInt32 m_uSignature;
		UInt32 m_uVersion;
		Float m_fTexcoordsScaleU;
		Float m_fTexcoordsScaleV;
		Float m_fVisibleRegionScaleU;
		Float m_fVisibleRegionScaleV;
		Float m_fVisibleRegionOffsetU;
		Float m_fVisibleRegionOffsetV;
		Float m_fOcclusionRegionScaleU;
		Float m_fOcclusionRegionScaleV;
		Float m_fOcclusionRegionOffsetU;
		Float m_fOcclusionRegionOffsetV;
	});

} // namespace Seoul

#endif // include guard
