/**
 * \file VideoEncoder.h
 * \brief Utility for writing encoded video streams of various codecs
 * and PCM encoded audio streams into an AVI container.
 *
 * \sa https://msdn.microsoft.com/en-us/library/windows/desktop/dd318189(v=vs.85).aspx
 * \sa https://msdn.microsoft.com/en-us/library/windows/desktop/dd388641%28v=vs.85%29.aspx
 * \sa https://msdn.microsoft.com/en-us/library/ms783421.aspx
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef VIDEO_ENCODER_H
#define VIDEO_ENCODER_H

#include "Prereqs.h"
#include "ScopedPtr.h"
#include "SeoulString.h"
#include "Vector.h"
#include "VideoCodec.h"
namespace Seoul { class SyncFile; }

namespace Seoul::Video
{

class Encoder SEOUL_SEALED
{
public:
	typedef Int16 SoundSampleType;

	Encoder(
		Codec eCodec,
		const String& sOutput,
		UInt32 uFPS,
		UInt32 uWidth,
		UInt32 uHeight,
		Bool bAudio,
		UInt32 uSamplingRate,
		UInt32 uChannels);
	~Encoder();

	Bool AddVideoFrame(
		void const* pData,
		UInt32 uDataInBytes);
	Bool AddAudioVideoFrame(
		void const* pVideoData,
		UInt32 uVideoDataInBytes,
		Float const* pfAudioSamples,
		UInt32 uAudioSizeInSamples);
	Bool Finish();

	UInt32 GetChannels() const
	{
		return m_uChannels;
	}

	Codec GetCodec() const
	{
		return m_eCodec;
	}

	UInt32 GetFrameSampleCount() const
	{
		auto const fSamplingRate = (Double)((Int32)m_uSamplingRate);
		auto const fFPS = (Double)((Int32)m_uFPS);
		auto const f = (fSamplingRate / fFPS);

		return (UInt32)((Int32)Ceil(f));
	}

	const String& GetOutputFilename() const
	{
		return m_sOutput;
	}

private:
	typedef Vector<SoundSampleType, MemoryBudgets::Video> Scratch;

	Codec const m_eCodec;
	String const m_sOutput;
	UInt32 const m_uFPS;
	UInt32 const m_uWidth;
	UInt32 const m_uHeight;
	Bool const m_bAudio;
	UInt32 const m_uSamplingRate;
	UInt32 const m_uChannels;
	Int64 m_iMovieDataStart;
	UInt32 m_uAudioSizeInBytes;
	UInt32 m_uVideoFrames;
	UInt32 m_uTotalDataSizeInBytes;
	UInt32 m_uMaxAudioBytesPerFrame;
	UInt32 m_uMaxTotalBytesPerFrame;
	UInt32 m_uMaxVideoBytesPerFrame;
	Scratch m_vSoundScratch;
	ScopedPtr<SyncFile> m_pFile;
	Bool m_bOk;

	void CheckFile();
	void WriteHeader();

	struct FrameEntry SEOUL_SEALED
	{
		FrameEntry(UInt32 uType, UInt32 uOffset, UInt32 uSize)
			: m_uTypeFourCC(uType)
			, m_uOffsetInBytes(uOffset)
			, m_uSizeInBytes(uSize)
		{
		}

		FrameEntry(const FrameEntry&) = default;

		UInt32 m_uTypeFourCC{};
		UInt32 m_uOffsetInBytes{};
		UInt32 m_uSizeInBytes{};
	};
	typedef Vector<FrameEntry, MemoryBudgets::Video> Frames;
	Frames m_vFrames;

	UInt32 Align(UInt32 uBytes);

	Int64 Pos();

	void WriteAudioSamples(
		Float const* pfSamples,
		UInt32 uSizeInSamples);
	void WriteVideoFrame(
		void const* pData,
		UInt32 uDataInBytes);

	template <typename T>
	void Write(const T& v);
	void Write(void const* p, UInt32 u);

	SEOUL_DISABLE_COPY(Encoder);
}; // class Encoder

} // namespace Seoul::Video

#endif // include guard
