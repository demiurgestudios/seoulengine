/**
 * \file GameVideoCapture.h
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

#pragma once
#ifndef GAME_VIDEO_CAPTURE_H
#define GAME_VIDEO_CAPTURE_H

#include "Delegate.h"
#include "UIMovie.h"
#include "VideoCodec.h"
namespace Seoul { namespace Video { class Capture; } }

namespace Seoul::Game
{

class VideoCapture SEOUL_SEALED : public UI::Movie
{
public:
	SEOUL_DELEGATE_TARGET(VideoCapture);
	SEOUL_REFLECTION_POLYMORPHIC(VideoCapture);

	VideoCapture();
	~VideoCapture();

private:
	ScopedPtr<Video::Capture> m_pVideoCapture;

	// UI::Movie overrides.
	virtual void OnPose(RenderPass& rPass, UI::Renderer& rRenderer) SEOUL_OVERRIDE;
	// /UIMovie overrides.

	// Implementation.
	void InternalRender(RenderPass& rPass, RenderCommandStreamBuilder& rBuilder);

	// Video capture utils.
	String GenerateVideoCapturePath() const;

	// Video capture control hooks.
	void EventHandlerStartVideoCapture(
		Video::Codec eCodec,
		const String& sPath,
		Bool bWithAudio,
		UInt32 uQuality);
	void EventHandlerStopVideoCapture();

	SEOUL_DISABLE_COPY(VideoCapture);
	SEOUL_REFLECTION_FRIENDSHIP(VideoCapture);
}; // class Game::VideoCapture

} // namespace Seoul::Game

#endif // include guard
