/**
 * \file Animation3DClipInstance.cpp
 * \brief An instance of an Animation3D::Clip. Necessary for runtime
 * playback of the clip's animation timelines.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnimationEventInterface.h"
#include "Animation3DCache.h"
#include "Animation3DClipDefinition.h"
#include "Animation3DClipInstance.h"
#include "Animation3DDataDefinition.h"
#include "Animation3DDataInstance.h"
#include "SeoulMath.h"

#if SEOUL_WITH_ANIMATION_3D

// TODO: All evaluators that support blending should use the Animation3D::Cache.
// Once that is complete, additive blending is straightforward.

namespace Seoul::Animation3D
{

class IEvaluator SEOUL_ABSTRACT
{
public:
	IEvaluator()
	{
	}

	virtual ~IEvaluator()
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) = 0;

private:
	SEOUL_DISABLE_COPY(IEvaluator);
}; // class IEvaluator

class EventEvaluator SEOUL_SEALED : public IEvaluator
{
public:
	EventEvaluator(
		const KeyFramesEvent& v,
		Float fEventMixThreshold)
		: m_v(v)
		, m_fEventMixThreshold(fEventMixThreshold)
	{
	}

	Bool GetNextEventTime(HString sEventName, Float fStartTime, Float& fEventTime) const
	{
		// Find the starting index.
		UInt32 u = 0u;
		UInt32 const uEnd = m_v.GetSize();

		// Iterate and search for the start time
		for (; u < uEnd; ++u)
		{
			if (m_v[u].m_fTime > fStartTime)
			{
				break;
			}
		}

		// Iterate until we find a matching event name
		for (; u < uEnd; ++u)
		{
			if (m_v[u].m_Id == sEventName)
			{
				fEventTime = m_v[u].m_fTime;
				return true;
			}
		}

		// no match
		return false;
	}

	void EvaluateRange(DataInstance& r, Float fStartTime, Float fEndTime, Float fAlpha)
	{
		// Early out if we're below the mix threshold.
		if (fAlpha < m_fEventMixThreshold)
		{
			return;
		}

		// Early out if we don't have an evaluator.
		auto pEventInterface = r.GetEventInterface();
		if (!pEventInterface.IsValid())
		{
			return;
		}

		// Find the starting index.
		UInt32 u = 0u;
		UInt32 const uEnd = m_v.GetSize();

		// fStartTime == 0.0 and m_v.Front().m_fTime == 0.0f
		// is a special case. Normally, the evaluation range is (start, end], so that
		// we don't play the event at end twice (when, on the next evaluation,
		// end becomes start of the next range). However, since no time before 0.0 exists,
		// we must treat 0.0 as a special case and include it in the range.
		if (fStartTime != 0.0f || m_v.Front().m_fTime != 0.0f)
		{
			// Iterate and search for the normal start. Open range,
			// so m_v[u].m_fTime must be > fStartTime to begin evaluation
			// at it.
			for (; u < uEnd; ++u)
			{
				if (m_v[u].m_fTime > fStartTime)
				{
					break;
				}
			}
		}

		// Iterate until we hit the end, dispatch an event
		// at each frame.
		for (; u < uEnd; ++u)
		{
			// Closed range - we include an index u if its time is <= fEndTime.
			if (m_v[u].m_fTime > fEndTime)
			{
				break;
			}

			auto const& e = m_v[u];
			pEventInterface->DispatchEvent(e.m_Id, e.m_i, e.m_f, e.m_s);
		}
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// Nop
	}

private:
	const KeyFramesEvent& m_v;
	Float const m_fEventMixThreshold;

	SEOUL_DISABLE_COPY(EventEvaluator);
}; // class EventEvaluator

class KeyFrameEvaluator SEOUL_ABSTRACT : public IEvaluator
{
public:
	KeyFrameEvaluator()
		: m_uLastKeyFrame(0)
	{
	}

	virtual ~KeyFrameEvaluator()
	{
	}

	template <typename T>
	Float32 GetAlpha(
		Float fTime,
		const T& r0,
		const T& r1) const
	{
		return Clamp((fTime - r0.m_fTime) / (r1.m_fTime - r0.m_fTime), 0.0f, 1.0f);
	}

	template <typename T>
	Float32 GetFrames(
		const T& v,
		Float fTime,
		typename T::ValueType const*& pr0,
		typename T::ValueType const*& pr1)
	{
		SEOUL_ASSERT(!v.IsEmpty());

		if (v[m_uLastKeyFrame].m_fTime > fTime)
		{
			if (0u == m_uLastKeyFrame)
			{
				pr0 = v.Get(m_uLastKeyFrame);
				pr1 = pr0;

				return GetAlpha(fTime, *pr0, *pr1);
			}

			m_uLastKeyFrame = 0u;
		}

		UInt32 const uSize = v.GetSize();
		for (; m_uLastKeyFrame + 1u < uSize; ++m_uLastKeyFrame)
		{
			if (v[m_uLastKeyFrame + 1u].m_fTime > fTime)
			{
				pr0 = v.Get(m_uLastKeyFrame);
				pr1 = v.Get(m_uLastKeyFrame + 1u);

				return GetAlpha(fTime, *pr0, *pr1);
			}
		}

		pr0 = v.Get(m_uLastKeyFrame);
		pr1 = pr0;

		return GetAlpha(fTime, *pr0, *pr1);
	}

private:
	UInt32 m_uLastKeyFrame;

	SEOUL_DISABLE_COPY(KeyFrameEvaluator);
}; // class KeyFrameEvaluator

class RotationEvaluator SEOUL_SEALED : public KeyFrameEvaluator
{
public:
	RotationEvaluator(
		DataInstance& r,
		const KeyFramesRotation& v,
		Int16 iBone)
		: m_r(r)
		, m_v(v)
		, m_iBone(iBone)
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime) { return; }

		// Cache common values.
		auto& cache = m_r.GetCache();

		// Standard case, interpolate between frames.
		KeyFrameRotation const* pk0 = nullptr;
		KeyFrameRotation const* pk1 = nullptr;
		Float const fT = GetFrames(m_v, fTime, pk0, pk1);

		// Accumulate.
		cache.AccumRotation(m_iBone,
			fAlpha * Quaternion::Slerp(pk0->m_qRotation, pk1->m_qRotation, fT));
	}

private:
	DataInstance& m_r;
	const KeyFramesRotation& m_v;
	Int16 const m_iBone;

	SEOUL_DISABLE_COPY(RotationEvaluator);
}; // class RotationEvaluator

class ScaleEvaluator SEOUL_SEALED : public KeyFrameEvaluator
{
public:
	ScaleEvaluator(
		DataInstance& r,
		const KeyFrames3D& v,
		Int16 iBone)
		: m_r(r)
		, m_v(v)
		, m_iBone(iBone)
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime) { return; }

		// Cache common values.
		auto& cache = m_r.GetCache();

		// Standard case, interpolate between frames.
		KeyFrame3D const* pk0 = nullptr;
		KeyFrame3D const* pk1 = nullptr;
		Float const fT = GetFrames(m_v, fTime, pk0, pk1);

		// Accumulate.
		cache.AccumScale(m_iBone,
			fAlpha * Vector3D::Lerp(pk0->m_v, pk1->m_v, fT), fAlpha);
	}

private:
	DataInstance& m_r;
	const KeyFrames3D& m_v;
	Int16 const m_iBone;

	SEOUL_DISABLE_COPY(ScaleEvaluator);
}; // class ScaleEvaluator

class TranslationEvaluator SEOUL_SEALED : public KeyFrameEvaluator
{
public:
	TranslationEvaluator(
		DataInstance& r,
		const KeyFrames3D& v,
		Int16 iBone)
		: m_r(r)
		, m_v(v)
		, m_iBone(iBone)
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime) { return; }

		// Cache common values.
		auto& cache = m_r.GetCache();

		// Standard case, interpolate between frames.
		KeyFrame3D const* pk0 = nullptr;
		KeyFrame3D const* pk1 = nullptr;
		Float const fT = GetFrames(m_v, fTime, pk0, pk1);

		// Accumulate.
		cache.AccumPosition(m_iBone,
			fAlpha * Vector3D::Lerp(pk0->m_v, pk1->m_v, fT));
	}

private:
	DataInstance& m_r;
	const KeyFrames3D& m_v;
	Int16 const m_iBone;

	SEOUL_DISABLE_COPY(TranslationEvaluator);
}; // class TranslationEvaluator

ClipInstance::ClipInstance(DataInstance& r, const SharedPtr<ClipDefinition>& pClipDefinition, const Animation::ClipSettings& settings)
	: m_Settings(settings)
	, m_r(r)
	, m_pClipDefinition(pClipDefinition)
	, m_fMaxTime(0.0f)
	, m_vEvaluators()
	, m_pEventEvaluator()
{
	InternalConstructEvaluators();
}

ClipInstance::~ClipInstance()
{
	m_pEventEvaluator.Reset();
	SafeDeleteVector(m_vEvaluators);
}

Bool ClipInstance::GetNextEventTime(HString sEventName, Float fStartTime, Float& fEventTime) const
{
	if (m_pEventEvaluator.IsValid())
	{
		return m_pEventEvaluator->GetNextEventTime(sEventName, fStartTime, fEventTime);
	}

	return false;
}

/**
 * Used for event dispatch, pass a time range. Looping should be implemented
 * by passing all time ranges (where fPrevTime >= 0.0f and fTime <= GetMaxTime())
 * iteratively until the final time is reached.
 */
void ClipInstance::EvaluateRange(Float fStartTime, Float fEndTime, Float fAlpha)
{
	if (m_pEventEvaluator.IsValid())
	{
		m_pEventEvaluator->EvaluateRange(m_r, fStartTime, fEndTime, fAlpha);
	}
}

void ClipInstance::Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState)
{
	for (auto i = m_vEvaluators.Begin(); m_vEvaluators.End() != i; ++i)
	{
		(*i)->Evaluate(fTime, fAlpha, bBlendDiscreteState);
	}
}

void ClipInstance::InternalConstructEvaluators()
{
	SafeDeleteVector(m_vEvaluators);

	// Bones first.
	{
		auto const& t = m_pClipDefinition->GetBones();
		m_vEvaluators.Reserve(m_vEvaluators.GetSize() + (t.GetSize() * 3u));

		auto const iBegin = t.Begin();
		auto const iEnd = t.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			auto const& entry = i->Second;
			auto const iBone = i->First;

			// Skip entries if no bone is available. This supports retargeting.
			if (iBone < 0)
			{
				continue;
			}

			if (!entry.m_vRotation.IsEmpty())
			{
				m_fMaxTime = Max(m_fMaxTime, entry.m_vRotation.Back().m_fTime);
				m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation3D) RotationEvaluator(m_r, entry.m_vRotation, iBone));
			}
			if (!entry.m_vScale.IsEmpty())
			{
				m_fMaxTime = Max(m_fMaxTime, entry.m_vScale.Back().m_fTime);
				m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation3D) ScaleEvaluator(m_r, entry.m_vScale, iBone));
			}

			if (!entry.m_vTranslation.IsEmpty())
			{
				m_fMaxTime = Max(m_fMaxTime, entry.m_vTranslation.Back().m_fTime);
				m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation3D) TranslationEvaluator(m_r, entry.m_vTranslation, iBone));
			}
		}
	}

	// TODO: Events.
	/*
	{
		auto const& v = m_pClip->GetEvents();
		if (!v.IsEmpty())
		{
			m_fMaxTime = Max(m_fMaxTime, v.Back().m_fTime);

			auto pEventEvaluator(SEOUL_NEW(MemoryBudgets::Animation3D) EventEvaluator(m_r, v, m_Settings.m_fEventMixThreshold));
			m_vEvaluators.PushBack(pEventEvaluator);
			m_pEventEvaluator = pEventEvaluator;
		}
	}*/
}

} // namespace Seoul::Animation3D

#endif // /#if SEOUL_WITH_ANIMATION_3D
