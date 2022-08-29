/**
 * \file RenderDevice.cpp
 * \brief Abstract base class for platform-specific device implementations.
 * RenderDevice and the other graphics objects in HAL provide
 * a platform-independent layer on top of platform-dependent graphics hardware
 * functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FromString.h"
#include "GamePaths.h"
#include "RenderDevice.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionDefine.h"
#include "SettingsManager.h"

namespace Seoul
{

/** Constants used to extract render device configuration values from application.json. */
static const HString ksApplication("Application");
static const HString ksFullscreen("Fullscreen");
static const HString ksFullscreenHeight("FullscreenHeight");
static const HString ksFullscreenOnMaximize("FullscreenOnMaximize");
static const HString ksFullscreenWidth("FullscreenWidth");
static const HString ksViewportHeight("ViewportHeight");
static const HString ksViewportWidth("ViewportWidth");
static const HString ksViewportX("ViewportX");
static const HString ksViewportY("ViewportY");
#if !SEOUL_SHIP
static const HString ksVirtualizedDesktop("VirtualizedDesktop");
#endif // /!SEOUL_SHIP
static const HString ksVsync("Vsync");
static const HString ksWindowedFullscreen("WindowedFullscreen");

SEOUL_BEGIN_ENUM(PrimitiveType)
	SEOUL_ENUM_N("None", PrimitiveType::kNone)
	SEOUL_ENUM_N("PointList", PrimitiveType::kPointList)
	SEOUL_ENUM_N("LineList", PrimitiveType::kLineList)
	SEOUL_ENUM_N("LineStrip", PrimitiveType::kLineStrip)
	SEOUL_ENUM_N("TriangleList", PrimitiveType::kTriangleList)
SEOUL_END_ENUM()

SEOUL_BEGIN_TYPE(Viewport)
	SEOUL_PROPERTY_N("TargetWidth", m_iTargetWidth)
	SEOUL_PROPERTY_N("TargetHeight", m_iTargetHeight)
	SEOUL_PROPERTY_N("ViewportX", m_iViewportX)
	SEOUL_PROPERTY_N("ViewportY", m_iViewportY)
	SEOUL_PROPERTY_N("ViewportWidth", m_iViewportWidth)
	SEOUL_PROPERTY_N("ViewportHeight", m_iViewportHeight)
SEOUL_END_TYPE()

RenderDevice::RenderDevice()
	: m_Caps()
	, m_GraphicsParameters()
	, m_eBackBufferDepthStencilFormat(DepthStencilFormat::kInvalid)
	, m_eBackBufferPixelFormat(PixelFormat::kInvalid)
	, m_eCompatible32bit4colorRenderTargetFormat(PixelFormat::kA8R8G8B8)
	, m_iDesiredVsyncInterval(0)
	, m_iPresentMarkerInTicks(-1)
	, m_iPresentDeltaInTicks(0)
	, m_sPlaceholderEmptyTitle()
{
	InternalInitializeGraphicsParameters();
}

RenderDevice::~RenderDevice()
{
}

/**
 * Not supported on all platforms. When supported, returns the human readable
 * title string of the application's main window.
 */
const String& RenderDevice::GetOsWindowTitle() const
{
	return m_sPlaceholderEmptyTitle;
}

/**
 * Shared utility function, should be implemented in most graphics drivers. Used to
 * avoid stalling in the driver during vsync when there is useful work the render thread could be doing.
 */
void RenderDevice::InternalPrePresent()
{
	// Instead of just immediately blocking in the Present() call when vsync is enabled,
	// let the Jobs::Manager do some work.
	auto const refresh = GetDisplayRefreshRate();
	auto const iVsyncInterval = GetVsyncInterval();
	if (iVsyncInterval > 0 &&
		m_iPresentMarkerInTicks > 0 &&
		!refresh.IsZero())
	{
		// Give a little bit of slop so we don't overshoot the interval.
		Int64 const iMargin = SeoulTime::ConvertMillisecondsToTicks(1.0);

		// Compute the time we expect to be remaining in the frame.
		Double const fDisplayHz = refresh.ToHz();
		Double const fTargetHz = fDisplayHz / (Double)iVsyncInterval;
		Int64 const iFrameTargetTicks = SeoulTime::ConvertMillisecondsToTicks(1000.0 / fTargetHz);
		Int64 const iDeltaTicks = (SeoulTime::GetGameTimeInTicks() - m_iPresentMarkerInTicks);
		Int64 const iDeltaInFrameTicks = (iDeltaTicks % iFrameTargetTicks);
		Int64 const iRemainingTicks = Max((Int64)0, (iFrameTargetTicks - iDeltaInFrameTicks) - iMargin);

		// Potentially fill up the refresh interval with render thread work.
		Int64 const iStartTicks = SeoulTime::GetGameTimeInTicks();
		while (SeoulTime::GetGameTimeInTicks() - iStartTicks < iRemainingTicks)
		{
			// Give the Jobs::Manager time, unless it has no work to do.
			if (!Jobs::Manager::Get()->YieldThreadTime())
			{
				break;
			}

			// Wait hint indicates another thread wants us to finish
			// frame processing, so early out.
			if (0 != m_WaitHint)
			{
				break;
			}
		}
	}
}

/**
 * Shared utility function, should be implemented in most graphics drivers. Used to
 * avoid stalling in the driver during vsync when there is useful work the render thread could be doing.
 */
void RenderDevice::InternalPostPresent()
{
	// Mark the start of a new vsync interval.
	auto const iPrev = m_iPresentMarkerInTicks;
	m_iPresentMarkerInTicks = SeoulTime::GetGameTimeInTicks();
	if (iPrev >= 0)
	{
		m_iPresentDeltaInTicks = (m_iPresentMarkerInTicks - iPrev);
	}
}

/**
 * Does the initial application window setup. Loads
 *  some setup variables from JSON files as well.
 */
void RenderDevice::InternalInitializeGraphicsParameters()
{
	m_GraphicsParameters = GraphicsParameters();

	// Override the window rectangle if it is set in the .json file
	SharedPtr<DataStore> pDataStore(SettingsManager::Get()->WaitForSettings(
		GamePaths::Get()->GetApplicationJsonFilePath()));
	if (!pDataStore.IsValid())
	{
		// Early out if no data.
		return;
	}

	Bool bVsync = false;
	DataStoreTableUtil applicationSection(*pDataStore, ksApplication);
#if !SEOUL_SHIP
	applicationSection.GetValue(ksVirtualizedDesktop, m_GraphicsParameters.m_bVirtualizedDesktop);
#endif // /!SEOUL_SHIP
	applicationSection.GetValue(ksWindowedFullscreen, m_GraphicsParameters.m_bWindowedFullscreen);
	applicationSection.GetValue(ksViewportWidth, m_GraphicsParameters.m_iWindowViewportWidth);
	applicationSection.GetValue(ksViewportHeight, m_GraphicsParameters.m_iWindowViewportHeight);
	applicationSection.GetValue(ksViewportX, m_GraphicsParameters.m_iWindowViewportX);
	applicationSection.GetValue(ksViewportY, m_GraphicsParameters.m_iWindowViewportY);
	applicationSection.GetValue(ksFullscreenWidth, m_GraphicsParameters.m_iFullscreenWidth);
	applicationSection.GetValue(ksFullscreenHeight, m_GraphicsParameters.m_iFullscreenHeight);
	applicationSection.GetValue(ksVsync, bVsync);
	applicationSection.GetValue(ksFullscreenOnMaximize, m_GraphicsParameters.m_bFullscreenOnMaximize);
	applicationSection.GetValue(ksFullscreen, m_GraphicsParameters.m_bStartFullscreen);

	// Copy through vsync interval.
	m_GraphicsParameters.m_iVsyncInterval = (bVsync ? 1 : 0);
	m_iDesiredVsyncInterval = m_GraphicsParameters.m_iVsyncInterval;
}

} // namespace Seoul
