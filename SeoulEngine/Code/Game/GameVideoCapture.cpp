/**
 * \file GameVideoCapture.cpp
 * \brief UI::Movie that performs video capture. Insert into
 * the UI setup at the point where you wish to capture
 * (e.g. below developer UI).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "GameAutomation.h"
#include "GamePaths.h"
#include "GameVideoCapture.h"
#include "Path.h"
#include "ReflectionDefine.h"
#include "RenderCommandStreamBuilder.h"
#include "UIContext.h"
#include "UIRenderer.h"
#include "VideoCapture.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(Game::VideoCapture, TypeFlags::kDisableCopy)
	SEOUL_PARENT(UI::Movie)
	SEOUL_METHOD(EventHandlerStartVideoCapture)
	SEOUL_METHOD(EventHandlerStopVideoCapture)
SEOUL_END_TYPE()

namespace Game
{

static const HString kGameCapturingVideo("GameCapturingVideo");

VideoCapture::VideoCapture()
	: m_pVideoCapture()
{
}

VideoCapture::~VideoCapture()
{
	if (Automation::Get())
	{
		Automation::Get()->SetGlobalState(kGameCapturingVideo, false);
	}
}

void VideoCapture::OnPose(RenderPass& rPass, UI::Renderer& rRenderer)
{
	SEOUL_ASSERT(IsMainThread());

	// Use the existing viewport.
	auto const viewport = rRenderer.GetActiveViewport();

	// Start rendering this movie - necessary, even though we
	// don't perform any rendering. We need the render hook.
	auto const stageBounds(Falcon::Rectangle::Create(
		0,
		(Float32)viewport.m_iViewportWidth,
		0,
		(Float32)viewport.m_iViewportHeight));

	// Start this movie rendering in the renderer.
	rRenderer.BeginMovie(this, stageBounds);

	// Enqueue custom renderer context to handle ImGui rendering during
	// buffer generation.
	rRenderer.PoseCustomDraw(SEOUL_BIND_DELEGATE(&VideoCapture::InternalRender, this));

	// Done with movie.
	rRenderer.EndMovie();
}

void VideoCapture::InternalRender(
	RenderPass& rPass,
	RenderCommandStreamBuilder& rBuilder)
{
	SEOUL_ASSERT(IsMainThread());

	if (Automation::Get())
	{
		auto const bCapturing = (m_pVideoCapture.IsValid() && m_pVideoCapture->GetState() != Video::CaptureState::kStopped);
		Automation::Get()->SetGlobalState(kGameCapturingVideo, bCapturing);
	}

	if (m_pVideoCapture.IsValid())
	{
		// If capturing video, submit the capture command now.
		if (m_pVideoCapture->GetState() == Video::CaptureState::kRecording)
		{
			auto const viewport = rBuilder.GetCurrentViewport();
			auto const rect = Rectangle2DInt(
				viewport.m_iViewportX,
				viewport.m_iViewportY,
				viewport.m_iViewportX + viewport.m_iViewportWidth,
				viewport.m_iViewportY + viewport.m_iViewportHeight);

			m_pVideoCapture->OnRenderFrame(rBuilder, rect);
		}
		// Otherwise, perform maintenance.
		else
		{
			m_pVideoCapture->Poll();
			if (m_pVideoCapture->GetState() == Video::CaptureState::kStopped)
			{
				m_pVideoCapture.Reset();
			}
		}
	}
}

String VideoCapture::GenerateVideoCapturePath() const
{
	auto const now = WorldTime::GetUTCTime();

	tm local;
	if (now.ConvertToLocalTime(local))
	{
		return Path::Combine(
			GamePaths::Get()->GetVideosDir(),
			String::Printf("VideoCapture %04d-%02d-%02d %02d-%02d-%02d",
				local.tm_year + 1900,
				local.tm_mon + 1,
				local.tm_mday,
				local.tm_hour,
				local.tm_min,
				local.tm_sec) + FileTypeToSourceExtension(FileType::kVideo));
	}
	else
	{
		return Path::Combine(
			GamePaths::Get()->GetVideosDir(),
			String::Printf("VideoCapture %" PRIi64, now.GetMicroseconds()) + FileTypeToSourceExtension(FileType::kVideo));
	}
}

void VideoCapture::EventHandlerStartVideoCapture(
	Video::Codec eCodec,
	const String& sPath,
	Bool bWithAudio,
	UInt32 uQuality)
{
	// Can't start unless if running already.
	if (m_pVideoCapture.IsValid() &&
		m_pVideoCapture->GetState() != Video::CaptureState::kStopped)
	{
		return;
	}

	// In the event we're clearing a stopped capture, reset first.
	m_pVideoCapture.Reset();

	// TODO: Should limit this to the actual root viewport.

	// Now instantiate a new capture unit.
	auto const viewport = UI::g_UIContext.GetRootViewport();
	m_pVideoCapture.Reset(SEOUL_NEW(MemoryBudgets::Video) Video::Capture(
		eCodec,
		Path::GetExactPathName(sPath),
		(UInt32)viewport.m_iTargetWidth,
		(UInt32)viewport.m_iTargetHeight,
		bWithAudio,
		uQuality));

	Automation::Get()->SetGlobalState(kGameCapturingVideo, true);
}

void VideoCapture::EventHandlerStopVideoCapture()
{
	// Stop the capture if it is valid.
	if (m_pVideoCapture.IsValid())
	{
		m_pVideoCapture->Stop();
	}
}

} // namespace Game

} // namespace Seoul
