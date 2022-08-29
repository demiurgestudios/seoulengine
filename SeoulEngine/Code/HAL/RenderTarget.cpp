/**
 * \file RenderTarget.cpp
 * \brief Represents a two dimensional color target on the GPU.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "ReflectionDataStoreTableUtil.h"
#include "RenderDevice.h"
#include "RenderTarget.h"

namespace Seoul
{

/** Constants used to extract configuration values from a render target configuration section. */
static const HString ksFormat("Format");
static const HString ksHeight("Height");
static const HString ksHeightFactor("HeightFactor");
static const HString ksHeightProportionalToBackBuffer("HeightProportionalToBackBuffer");
static const HString ksInputOutput("InputOutput");
static const HString ksLevels("Levels");
static const HString ksSameFormatAsBackBuffer("SameFormatAsBackBuffer");
static const HString ksThresholdHeightFactor("ThresholdHeightFactor");
static const HString ksThresholdWidthFactor("ThresholdWidthFactor");
static const HString ksWidth("Width");
static const HString ksWidthFactor("WidthFactor");
static const HString ksWidthProportionalToBackBuffer("WidthProportionalToBackBuffer");
static const HString ksWidthTimesHeightThreshold("WidthTimesHeightThreshold");

static const Float kMaxWidthHeightFactor = 16.0f;

RenderTarget* RenderTarget::s_pActiveRenderTarget = nullptr;

/**
 * Constructs a RenderTarget from an JSON file section
 * describing its settings.
 */
RenderTarget::RenderTarget(const DataStoreTableUtil& configSettings)
	: m_Flags(0u)
	, m_Levels(1u)
	, m_Width(0u)
	, m_Height(0u)
	, m_WidthTimesHeightThreshold(0u)
	, m_fThresholdWidthFactor(0.0f)
	, m_fThresholdHeightFactor(0.0f)
	, m_eFormat(PixelFormat::kInvalid)
	, m_ResetCount(0)
{
	// If we don't have a levels setting in the JSON file, default
	// to a one level render target.
	if (!configSettings.GetValue<UInt16>(ksLevels, m_Levels))
	{
		m_Levels = 1u;
	}

	// If we don't have a format entry in the JSON file, set
	// this RenderTarget's format as invalid.
	Bool bMatchBackBuffer = false;
	if (!configSettings.GetValue(ksFormat, m_eFormat))
	{
		if (configSettings.GetValue(ksSameFormatAsBackBuffer, bMatchBackBuffer) && bMatchBackBuffer)
		{
			m_eFormat = RenderDevice::Get()->GetBackBufferPixelFormat();
		}
		else
		{
			m_eFormat = PixelFormat::kInvalid;
		}
	}

	// Validate and warn about an invalid format.
	if (PixelFormat::kInvalid == m_eFormat)
	{
		SEOUL_WARN("RenderTarget (%s) does not have a valid \"Format\" entry.",
			configSettings.GetName().CStr());
	}

	// Check if our width and/or height are proportional to the backbuffer.
	Bool bWidthProportionalToBackBuffer = false;
	Bool bHeightProportionalToBackBuffer = false;

	configSettings.GetValue<Bool>(
		ksWidthProportionalToBackBuffer,
		bWidthProportionalToBackBuffer);

	configSettings.GetValue<Bool>(
		ksHeightProportionalToBackBuffer,
		bHeightProportionalToBackBuffer);

	// If we have proportional width, grab the factor from the JSON file.
	// Proportioanl width means that our width will be computed as
	// floor(BackBufferWidth * (our width factor))
	if (bWidthProportionalToBackBuffer)
	{
		m_Flags |= kTakeWidthFromBackBuffer;
		if (!configSettings.GetValue<Float>(ksWidthFactor, m_fWidthFactor))
		{
			SEOUL_WARN("RenderTarget (%s) is defined as having a width "
				"proportional to the back buffer but its definition does "
				"not contain a \"WidthFactor\" entry.",
				configSettings.GetName().CStr());

			m_fWidthFactor = fEpsilon;
		}

		if (m_fWidthFactor < fEpsilon || m_fWidthFactor > kMaxWidthHeightFactor)
		{
			SEOUL_WARN("RenderTarget (%s) has an out-of-range width factor.",
				configSettings.GetName().CStr());

			m_fWidthFactor = Clamp(m_fWidthFactor, fEpsilon, kMaxWidthHeightFactor);
		}
	}
	// Otherwise, grab the fixed width in pixels.
	else
	{
		if (!configSettings.GetValue<UInt32>(ksWidth, m_Width))
		{
			SEOUL_WARN("RenderTarget (%s) is defined as having a fixed width "
				"but its definition does "
				"not contain a \"Width\" entry.",
				configSettings.GetName().CStr());

			m_Width = 0u;
		}
	}

	// If we have proportional height, grab the factor from the JSON file.
	// Proportioanl height means that our height will be computed as
	// floor(BackBufferHeight * (our height factor))
	if (bHeightProportionalToBackBuffer)
	{
		m_Flags |= kTakeHeightFromBackBuffer;
		if (!configSettings.GetValue<Float>(ksHeightFactor, m_fHeightFactor))
		{
			SEOUL_WARN("RenderTarget (%s) is defined as having a height "
				"proportional to the back buffer but its definition does "
				"not contain a \"HeightFactor\" entry.",
				configSettings.GetName().CStr());

			m_fHeightFactor = fEpsilon;
		}

		if (m_fHeightFactor < fEpsilon || m_fHeightFactor > kMaxWidthHeightFactor)
		{
			SEOUL_WARN("RenderTarget (%s) has an out-of-range height factor.",
				configSettings.GetName().CStr());

			m_fHeightFactor = Clamp(m_fHeightFactor, fEpsilon, kMaxWidthHeightFactor);
		}
	}
	// Otherwise, grab the fixed height in pixels.
	else
	{
		if (!configSettings.GetValue<UInt32>(ksHeight, m_Height))
		{
			SEOUL_WARN("RenderTarget (%s) is defined as having a fixed height "
				"but its definition does "
				"not contain a \"Height\" entry.",
				configSettings.GetName().CStr());

			m_Height = 0u;
		}
	}

	// If one of the dimensions of this render target is relative to the back
	// buffer but the total calculated (width * height) is less than this
	// threshold, then the width and height will be set exactly equal to
	// the backbuffer.
	if (!configSettings.GetValue<UInt32>(ksWidthTimesHeightThreshold, m_WidthTimesHeightThreshold))
	{
		m_WidthTimesHeightThreshold = 0u;
	}

	if (!configSettings.GetValue<Float32>(ksThresholdWidthFactor, m_fThresholdWidthFactor))
	{
		m_fThresholdWidthFactor = 0.0f;
	}

	if (!configSettings.GetValue<Float32>(ksThresholdHeightFactor, m_fThresholdHeightFactor))
	{
		m_fThresholdHeightFactor = 0.0f;
	}

	// Input-output - if true, this render target will be configured such
	// that it can be used as both input (as a texture parameter) and
	// output (as a target surface) during the same draw operation.
	Bool bSimultaneousInputOutput;
	if (!configSettings.GetValue<Bool>(ksInputOutput, bSimultaneousInputOutput))
	{
		bSimultaneousInputOutput = false;
	}

	if (bSimultaneousInputOutput)
	{
		m_Flags |= kSimultaneousInputOutput;
	}
}

RenderTarget::~RenderTarget()
{
	// It is the responsibility of the implementation to make
	// sure a set surface is not destroyed.
	SEOUL_ASSERT(this != s_pActiveRenderTarget);

	// It is the responsibility of the subclass to un-reset itself
	// on destruction if the graphics object was ever created.
	SEOUL_ASSERT(GetState() == kCreated || GetState() == kDestroyed);
}

void RenderTarget::OnReset()
{
	++m_ResetCount;
	BaseTexture::OnReset();
}

/**
 * Recalculate the width and height of this RenderTarget.
 * The actual width and height may change if this RenderTarget
 * is defined relative to the back buffer.
 */
void RenderTarget::InternalRefreshWidthAndHeight()
{
	// If this RenderTarget has proportional width or depth,
	// then that width or depth is proportional to the BackBuffer
	// viewport width and depth, not the full BackBuffer dimensions.
	const Viewport& viewport = RenderDevice::Get()->GetBackBufferViewport();
	const Int32 iBackBufferWidth = viewport.m_iTargetWidth;
	const Int32 iBackBufferHeight = viewport.m_iTargetHeight;
	Int32 iWidth = (IsWidthProportionalToBackBuffer())
		? Max((Int32)(iBackBufferWidth * m_fWidthFactor), 1)
		: (Int32)m_Width;
	Int32 iHeight = (IsHeightProportionalToBackBuffer())
		? Max((Int32)(iBackBufferHeight * m_fHeightFactor), 1)
		: (Int32)m_Height;

	// If the width * height threshold is lower than the given threshold,
	// set relative dimensions to be exactly equal to the back buffer.
	Int32 const iWidthTimesHeight = (iBackBufferWidth * iBackBufferHeight);
	if (iWidthTimesHeight < (Int32)m_WidthTimesHeightThreshold)
	{
		if (IsWidthProportionalToBackBuffer())
		{
			iWidth = Max((Int32)(iBackBufferWidth * m_fThresholdWidthFactor), 1);
		}

		if (IsHeightProportionalToBackBuffer())
		{
			iHeight = Max((Int32)(iBackBufferHeight * m_fThresholdHeightFactor), 1);
		}
	}

	// Assign the final width and height to the base BaseTexture members -
	// these are the values that are actually publically accessible.
	m_iWidth = iWidth;
	m_iHeight = iHeight;
}

} // namespace Seoul
