/**
 * \file FalconBitmapDefinition.cpp
 * \brief Defines either on-disk or in-memory image
 * data to be cached and rendered by the Falcon
 * backend.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Color.h"
#include "FalconBitmapDefinition.h"
#include "FalconBitmapInstance.h"
#include "FalconRenderPoser.h"
#include "HashFunctions.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "SeoulString.h"

namespace Seoul
{

SEOUL_SPEC_TEMPLATE_TYPE(SharedPtr<Falcon::BitmapDefinition const>)
SEOUL_BEGIN_TYPE(Falcon::BitmapDefinition, TypeFlags::kDisableNew)
	SEOUL_PROPERTY_N("FilePath", m_FilePath)
	SEOUL_PROPERTY_N("Height", m_uHeight)
	SEOUL_PROPERTY_N("Width", m_uWidth)
	SEOUL_PROPERTY_N("VisibleRectangle", m_VisibleRectangle)
	SEOUL_PROPERTY_N("IsFullOccluder", m_bIsFullOccluder)
	SEOUL_PROPERTY_N("CanPack", m_bCanPack)
	SEOUL_PROPERTY_N("Preload", m_bPreload)
SEOUL_END_TYPE()

namespace Falcon
{

static RGBA SampleGradient(
	const Gradient& gradient,
	UInt32 uRatio)
{
	if (gradient.m_vGradientRecords.IsEmpty())
	{
		return RGBA::TransparentBlack();
	}

	if (uRatio < gradient.m_vGradientRecords[0].m_uRatio)
	{
		return gradient.m_vGradientRecords[0].m_Color;
	}

	for (GradientRecords::SizeType i = 1; i < gradient.m_vGradientRecords.GetSize(); ++i)
	{
		if (gradient.m_vGradientRecords[i].m_uRatio >= uRatio)
		{
			const GradientRecord& record0 = gradient.m_vGradientRecords[i - 1];
			const GradientRecord& record1 = gradient.m_vGradientRecords[i];

			Float fLerpAlpha = 0.0f;
			if (record0.m_uRatio != record1.m_uRatio)
			{
				fLerpAlpha = (uRatio - record0.m_uRatio) / (Float)(record1.m_uRatio - record0.m_uRatio);
			}

			return Lerp(record0.m_Color, record1.m_Color, fLerpAlpha);
		}
	}

	return gradient.m_vGradientRecords.Back().m_Color;
}

BitmapDefinition::BitmapDefinition(
	FillStyleType eGradientType,
	const Gradient& gradient,
	Bool bCanPack)
	: Definition(DefinitionType::kBitmap, 0)
	, m_FilePath()
	, m_uHeight(0)
	, m_uWidth(0)
	, m_pData(nullptr)
	, m_VisibleRectangle()
	, m_bIsFullOccluder(false)
	, m_bCanPack(bCanPack)
	, m_bPreload(false)
{
	// Initially true - set to false if we encounter any
	// transparent pixels.
	m_bIsFullOccluder = true;

	if (FillStyleType::kLinearGradientFill == eGradientType)
	{
		m_uHeight = 1;
		m_uWidth = 256;
		m_pData = (UInt8*)MemoryManager::Allocate(m_uWidth * 4, MemoryBudgets::Falcon);

		for (UInt32 i = 0; i < m_uWidth; ++i)
		{
			RGBA const rgba = PremultiplyAlpha(SampleGradient(gradient, i));

			UInt32 const uOutIndex = (i * 4);
			m_pData[uOutIndex+0] = rgba.m_R;
			m_pData[uOutIndex+1] = rgba.m_G;
			m_pData[uOutIndex+2] = rgba.m_B;
			m_pData[uOutIndex+3] = rgba.m_A;

			// Can occlude if the fill color's alpha is
			// at or above the 8-bit occlusion threshold.
			m_bIsFullOccluder = (m_bIsFullOccluder && (rgba.m_A >= ku8bitColorOcclusionThreshold));
		}
	}
	else
	{
		m_uHeight = 64;
		m_uWidth = 64;
		m_pData = (UInt8*)MemoryManager::Allocate(m_uWidth * m_uHeight * 4, MemoryBudgets::Falcon);

		Float const fRadius = (m_uHeight - 1) / 2.0f;
		for (UInt32 j = 0; j < m_uHeight; ++j)
		{
			for (UInt32 i = 0; i < m_uWidth; ++i)
			{
				Vector2D const v(Vector2D((i - fRadius) / fRadius, (j - fRadius) / fRadius));
				UInt32 const uRatio = Seoul::Min((UInt32)Floor(255.5f * v.Length()), 255u);

				RGBA const rgba = PremultiplyAlpha(SampleGradient(gradient, uRatio));

				UInt32 const uOutIndex = (j * m_uWidth * 4) + (i * 4);
				m_pData[uOutIndex+0] = rgba.m_R;
				m_pData[uOutIndex+1] = rgba.m_G;
				m_pData[uOutIndex+2] = rgba.m_B;
				m_pData[uOutIndex+3] = rgba.m_A;

				// Can occlude if the fill color's alpha is
				// at or above the 8-bit occlusion threshold.
				m_bIsFullOccluder = (m_bIsFullOccluder && (rgba.m_A >= ku8bitColorOcclusionThreshold));
			}
		}
	}

	// Now fill in visible rectangle.
	m_VisibleRectangle = Rectangle::Create(0, (Float32)m_uWidth, 0, (Float32)m_uHeight);

	String const sIdentifier(String::Printf("gradient_image_%ux%u_0x%08X",
		m_uWidth,
		m_uHeight,
		Seoul::GetHash((const char*)m_pData, (m_uWidth * m_uHeight * 4))));

	// TODO: Verify that this psuedo path won't be a problem.
	m_FilePath.SetDirectory(GameDirectory::kUnknown);
	m_FilePath.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename(sIdentifier));
	m_FilePath.SetType(FileType::kTexture0);
}

BitmapDefinition::BitmapDefinition(UInt32 uWidth, UInt32 uHeight, UInt8* pData, Bool bIsFullOccluder)
	: Definition(DefinitionType::kBitmap, 0)
	, m_FilePath()
	, m_uHeight(uHeight)
	, m_uWidth(uWidth)
	, m_pData(pData)
	, m_VisibleRectangle(Rectangle::Create(0, (Float32)uWidth, 0, (Float32)uHeight))
	, m_bIsFullOccluder(bIsFullOccluder)
	, m_bCanPack(true)
	, m_bPreload(false)
{
	String const sIdentifier(String::Printf("raw_image_%ux%u_0x%08X",
		m_uWidth,
		m_uHeight,
		Seoul::GetHash((const char*)m_pData, (m_uWidth * m_uHeight * 4))));

	// TODO: Verify that this psuedo path won't be a problem.
	m_FilePath.SetDirectory(GameDirectory::kUnknown);
	m_FilePath.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename(sIdentifier));
	m_FilePath.SetType(FileType::kTexture0);
}

void BitmapDefinition::DoCreateInstance(SharedPtr<Instance>& rp) const
{
	rp.Reset(SEOUL_NEW(MemoryBudgets::Falcon) BitmapInstance(SharedPtr<BitmapDefinition const>(this)));
}

} // namespace Falcon

} // namespace Seoul
