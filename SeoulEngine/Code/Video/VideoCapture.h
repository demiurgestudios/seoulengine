/**
 * \file VideoCapture.h
 * \brief Utility that binds various systems (rendering and audio)
 * into a utility for capturing game content into a video
 * using Video::Encoder.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H

#include "Prereqs.h"
#include "VideoCodec.h"
namespace Seoul { struct Rectangle2DInt; }
namespace Seoul { class RenderCommandStreamBuilder; }
namespace Seoul { namespace Video { class CaptureJob; } }
namespace Seoul { namespace Video { class Encoder; } }
namespace Seoul { namespace Video { class CaptureGrabSound; } }

namespace Seoul::Video
{

enum class CaptureState
{
	kRecording,
	kStopping,
	kStopped,
};

class Capture SEOUL_SEALED
{
public:
	Capture(
		Codec eCodec,
		const String& sOutputFilename,
		UInt32 uWidth,
		UInt32 uHeight,
		Bool bCaptureSound,
		UInt32 uQuality);
	~Capture();

	CaptureState GetState() const
	{
		return m_eState;
	}

	void OnRenderFrame(RenderCommandStreamBuilder& rBuilder, const Rectangle2DInt& rect);
	void Poll();

	void Stop();

private:
	SharedPtr<CaptureJob> m_pCaptureJob;
	SharedPtr<CaptureGrabSound> m_pCaptureSound;
	CaptureState m_eState;
	Bool const m_bCaptureSound;

	SEOUL_DISABLE_COPY(Capture);
}; // class Capture

} // namespace Seoul::Video

#endif // include guard
