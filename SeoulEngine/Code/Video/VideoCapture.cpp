/**
 * \file VideoCapture.cpp
 * \brief Utility that binds various systems (rendering and audio)
 * into a utility for capturing game content into a video
 * using VideoEncoder.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "JobsJob.h"
#include "Logger.h"
#include "RenderCommandStreamBuilder.h"
#include "RenderDevice.h"
#include "SoundManager.h"
#include "VideoCapture.h"
#include "VideoEncoder.h"

namespace Seoul::Video
{

class CaptureJob SEOUL_SEALED : public Jobs::Job
{
public:
	CaptureJob(
		Codec eCodec,
		String sOutFilename,
		UInt32 uWidth,
		UInt32 uHeight,
		Bool bSound,
		UInt32 uQuality,
		UInt32 uSamplingRate,
		UInt32 uChannels)
		: m_bSound(bSound)
		, m_uQuality(uQuality)
		, m_Encoder(
			eCodec,
			sOutFilename,
			(UInt32)((Int32)RenderDevice::Get()->GetDisplayRefreshRate().ToHz()),
			uWidth,
			uHeight,
			bSound,
			uSamplingRate,
			uChannels)
	{
	}

	~CaptureJob()
	{
		WaitUntilJobIsNotRunning();

		{
			Lock lock(m_Mutex);
			for (auto const e : m_tVideoFrames)
			{
				MemoryManager::Deallocate(e.Second.m_p);
				e.Second.m_p = nullptr;
			}
			m_tVideoFrames.Clear();
		}
	}

	Bool const m_bSound;
	UInt32 const m_uQuality;
	Atomic32 m_SoundProcessed;
	Atomic32 m_VideoProcessed;
	Atomic32 m_VideoSubmitted;
	Atomic32 m_SoundReceived;
	Atomic32 m_VideoReceived;
	Atomic32Value<Bool> m_bDone;
	Encoder m_Encoder;
	typedef Vector<Float, MemoryBudgets::Video> SoundSamples;
	SoundSamples m_vSoundSamples;

	struct VideoFrame SEOUL_SEALED
	{
		void* m_p{};
		UInt32 m_u{};
	};
	typedef HashTable< UInt32, VideoFrame, MemoryBudgets::Video > VideoFrames;
	VideoFrames m_tVideoFrames;
	typedef HashTable< UInt32, SharedPtr<Sound::SampleData>, MemoryBudgets::Video > SoundsSamples;
	SoundsSamples m_tSoundSamples;
	Mutex m_Mutex;

private:
	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE
	{
		while (
			!m_bDone ||
			m_VideoReceived < m_VideoSubmitted ||
			m_VideoProcessed < m_VideoReceived)
		{
			{
				Jobs::ScopedQuantum scope(*this, Jobs::Quantum::kWaitingForDependency);
				while (m_VideoReceived <= m_VideoProcessed)
				{
					Jobs::Manager::Get()->YieldThreadTime();
				}
			}

			while (m_VideoProcessed < m_VideoReceived)
			{
				VideoFrame frame;
				Bool bHasFrame = false;
				{
					Lock lock(m_Mutex);
					bHasFrame = m_tVideoFrames.GetValue(m_VideoProcessed, frame);
					if (bHasFrame)
					{
						m_tVideoFrames.Erase(m_VideoProcessed);
					}
				}

				if (!bHasFrame)
				{
					Jobs::ScopedQuantum scope(*this, Jobs::Quantum::kWaitingForDependency);
					while (!bHasFrame)
					{
						{
							Lock lock(m_Mutex);
							bHasFrame = m_tVideoFrames.GetValue(m_VideoProcessed, frame);
							if (bHasFrame)
							{
								m_tVideoFrames.Erase(m_VideoProcessed);
								break;
							}
						}

						Jobs::Manager::Get()->YieldThreadTime();
					}
				}

				InternalProcessVideo(frame);
				++m_VideoProcessed;
			}
		}

		reNextState = Jobs::State::kComplete;
	}

	Bool InternalFillSoundBuffer(const SharedPtr<Sound::SampleData>& pData)
	{
		// Commit the frame.
		if (pData->GetSizeInSamples() > 0u)
		{
			auto const p = pData->GetData();
			m_vSoundSamples.Append(p, p + pData->GetSizeInSamples() * pData->GetChannels());
		}

		return true;
	}

	Bool InternalFillSoundBuffer()
	{
		// Maximum time in ticks that we will wait for any trailing audio
		// samples.
		Int64 const kiMaxWaitTimeInTicks = SeoulTime::ConvertMillisecondsToTicks(500.0);
		if (m_SoundProcessed >= m_SoundReceived)
		{
			auto const iStart = SeoulTime::GetGameTimeInTicks();
			Jobs::ScopedQuantum scope(*this, Jobs::Quantum::kWaitingForDependency);
			while (m_SoundProcessed >= m_SoundReceived)
			{
				// If we're done, and we've exceeded the wait
				// time, we don't wait for any more samples.
				if (m_bDone && SeoulTime::GetGameTimeInTicks() - iStart > kiMaxWaitTimeInTicks)
				{
					return false;
				}

				Jobs::Manager::Get()->YieldThreadTime();
			}
		}

		SharedPtr<Sound::SampleData> pData;
		{
			Lock lock(m_Mutex);
			if (m_tSoundSamples.GetValue(m_SoundProcessed, pData))
			{
				m_tSoundSamples.Erase(m_SoundProcessed);
			}
		}

		if (!pData.IsValid())
		{
			Jobs::ScopedQuantum scope(*this, Jobs::Quantum::kWaitingForDependency);
			while (!pData.IsValid())
			{
				{
					Lock lock(m_Mutex);
					if (m_tSoundSamples.GetValue(m_SoundProcessed, pData))
					{
						m_tSoundSamples.Erase(m_SoundProcessed);
						break;
					}
				}
				Jobs::Manager::Get()->YieldThreadTime();
			}
		}

		if (InternalFillSoundBuffer(pData))
		{
			++m_SoundProcessed;
			return true;
		}

		return false;
	}

	void InternalProcessVideo(VideoFrame& frame)
	{
		// Commit the frame - if processing audio, need to also
		// include enough samples for 1 frame of sound.
		if (m_bSound)
		{
			auto const uSamples = m_Encoder.GetFrameSampleCount();
			while (m_vSoundSamples.GetSize() < uSamples * m_Encoder.GetChannels())
			{
				if (!InternalFillSoundBuffer())
				{
					break;
				}
			}

			auto const uData = Min(uSamples, m_vSoundSamples.GetSize() / m_Encoder.GetChannels());
			if (!m_Encoder.AddAudioVideoFrame(
				frame.m_p,
				frame.m_u,
				m_vSoundSamples.Data(),
				uData))
			{
				SEOUL_WARN("%s: failed encoding frame %u",
					m_Encoder.GetOutputFilename().CStr(),
					(UInt32)m_VideoProcessed);
			}

			m_vSoundSamples.Erase(
				m_vSoundSamples.Begin(),
				m_vSoundSamples.Begin() + (uData * m_Encoder.GetChannels()));
		}
		else
		{
			if (!m_Encoder.AddVideoFrame(frame.m_p, frame.m_u))
			{
				SEOUL_WARN("%s: failed encoding frame %u",
					m_Encoder.GetOutputFilename().CStr(),
					(UInt32)m_VideoProcessed);
			}
		}

		MemoryManager::Deallocate(frame.m_p);
		frame.m_p = nullptr;
		frame.m_u = 0u;
	}

	SEOUL_DISABLE_COPY(CaptureJob);
	SEOUL_REFERENCE_COUNTED_SUBCLASS(CaptureJob);
}; // class CaptureJob

class CaptureGrabSound SEOUL_SEALED : public Sound::ICapture
{
public:
	CaptureGrabSound(const SharedPtr<CaptureJob>& pJob)
		: m_pJob(pJob)
	{
	}

	~CaptureGrabSound()
	{
	}

	virtual void OnSamples(const SharedPtr<Sound::SampleData>& pData) SEOUL_OVERRIDE
	{
		Lock lock(m_pJob->m_Mutex);
		SEOUL_VERIFY(m_pJob->m_tSoundSamples.Insert((UInt32)pData->GetFrame(), pData).Second);
		++m_pJob->m_SoundReceived;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(CaptureGrabSound);
	SEOUL_DISABLE_COPY(CaptureGrabSound);

	SharedPtr<CaptureJob> m_pJob;
}; // class CaptureGrabSound

class CaptureGrabFrame : public IGrabFrame
{
public:
	CaptureGrabFrame(const SharedPtr<CaptureJob>& pJob)
		: m_pJob(pJob)
	{
	}

	~CaptureGrabFrame()
	{
	}

	virtual void OnGrabFrame(
		UInt32 uFrame,
		const SharedPtr<IFrameData>& pFrameData,
		Bool bSuccess) SEOUL_OVERRIDE
	{
		if (!bSuccess)
		{
			return;
		}

		void* p = nullptr;
		UInt32 u = 0u;

		switch (m_pJob->m_Encoder.GetCodec())
		{
		case Codec::kLossless:
			{
				auto const uWidth = pFrameData->GetFrameWidth();
				auto const uHeight = pFrameData->GetFrameHeight();
				auto const uInPitch = pFrameData->GetPitch();
				auto const uOutPitch = (UInt32)RoundUpToAlignment(uWidth * 3u, 4u);
				u = (uOutPitch * uHeight);
				p = MemoryManager::Allocate(u, MemoryBudgets::TBD);
				memset(p, 0, u);

				UInt8 const* pIn = (UInt8 const*)pFrameData->GetData();
				UInt8* pOut = (UInt8*)p;
				for (UInt32 y = 0u; y < uHeight; ++y)
				{
					for (UInt32 x = 0u; x < uWidth; ++x)
					{
						auto const uBaseIn = (y * uInPitch + (x * 4u));
						auto const uBaseOut = (u - ((y + 1u) * uOutPitch)) + (x * 3u);

						pOut[uBaseOut + 2u] = pIn[uBaseIn + 0u];
						pOut[uBaseOut + 1u] = pIn[uBaseIn + 1u];
						pOut[uBaseOut + 0u] = pIn[uBaseIn + 2u];
					}
				}
			}
			break;

		default:
			SEOUL_FAIL("Out-of-sync enum.");
			return;
		}

		{
			Lock lock(m_pJob->m_Mutex);
			CaptureJob::VideoFrame frame;
			frame.m_p = p;
			frame.m_u = u;
			SEOUL_VERIFY(m_pJob->m_tVideoFrames.Insert(uFrame, frame).Second);
		}

		m_pJob->m_VideoReceived++;
		m_pJob.Reset();
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(CaptureGrabFrame);
	SEOUL_DISABLE_COPY(CaptureGrabFrame);

	SharedPtr<CaptureJob> m_pJob;
}; // class CaptureGrabFrame

Capture::Capture(
	Codec eCodec,
	const String& sOutputFilename,
	UInt32 uWidth,
	UInt32 uHeight,
	Bool bCaptureSound,
	UInt32 uQuality)
	: m_pCaptureJob()
	, m_pCaptureSound()
	, m_eState(CaptureState::kRecording)
	, m_bCaptureSound(bCaptureSound)
{
	UInt32 uSamplingRate = 0u;
	UInt32 uChannels = 0u;
	if (bCaptureSound)
	{
		if (!Sound::Manager::Get()->GetMasterAttributes(uSamplingRate, uChannels))
		{
			// TODO: Warn.
			bCaptureSound = false;
		}
	}

	m_pCaptureJob.Reset(SEOUL_NEW(MemoryBudgets::Video) CaptureJob(
		eCodec,
		sOutputFilename,
		uWidth,
		uHeight,
		bCaptureSound,
		uQuality,
		uSamplingRate,
		uChannels));
	m_pCaptureJob->StartJob();
}

Capture::~Capture()
{
}

void Capture::OnRenderFrame(RenderCommandStreamBuilder& rBuilder, const Rectangle2DInt& rect)
{
	if (CaptureState::kRecording != m_eState)
	{
		return;
	}

	SharedPtr<CaptureGrabFrame> pCallback(SEOUL_NEW(MemoryBudgets::Video) CaptureGrabFrame(m_pCaptureJob));
	rBuilder.GrabBackBufferFrame((UInt32)(m_pCaptureJob->m_VideoSubmitted), rect, pCallback);
	++(m_pCaptureJob->m_VideoSubmitted);

	if (m_bCaptureSound && !m_pCaptureSound.IsValid())
	{
		m_pCaptureSound.Reset(SEOUL_NEW(MemoryBudgets::Video) CaptureGrabSound(m_pCaptureJob));
		Sound::Manager::Get()->RegisterSoundCapture(m_pCaptureSound);
	}
}

void Capture::Poll()
{
	if (CaptureState::kStopping != m_eState)
	{
		return;
	}

	if (!m_pCaptureJob->IsJobRunning())
	{
		m_pCaptureSound.Reset();
		m_pCaptureJob.Reset();
		m_eState = CaptureState::kStopped;
	}
}

void Capture::Stop()
{
	if (CaptureState::kRecording != m_eState)
	{
		return;
	}

	m_pCaptureJob->m_bDone = true;
	m_eState = CaptureState::kStopping;
}

} // namespace Seoul::Video
