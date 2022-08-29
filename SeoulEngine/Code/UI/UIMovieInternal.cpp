/**
 * \file UIMovieInternal.cpp
 * \brief Internal class, owned by UI::Movie, handles
 * some low-level details of wrapping a Falcon scene graph.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "Engine.h"
#include "FalconFCNFile.h"
#include "FalconMovieClipInstance.h"
#include "LocManager.h"
#include "Logger.h"
#include "UIAdvanceInterfaceDeferredDispatch.h"
#include "UIManager.h"
#include "UIMovie.h"
#include "UIMovieInternal.h"
#include "UIRenderer.h"

namespace Seoul::UI
{

MovieInternal::MovieInternal(FilePath movieFilePath, HString typeName)
	: m_hFCNFileData()
	, m_pRoot()
	, m_pAdvanceInterface()
	, m_FilePath(movieFilePath)
	, m_fAccumulatedFrameTime(0.0f)
	, m_TypeName(typeName)
{
}

void MovieInternal::DispatchEvents(Falcon::AdvanceInterface& rAdvanceInterface)
{
	m_pAdvanceInterface->SetInterface(&rAdvanceInterface);
	m_pAdvanceInterface->DispatchEvents();
	m_pAdvanceInterface->SetInterface(nullptr);
}

void MovieInternal::Initialize()
{
	InternalInitializeMovieFromFilePath();
}

MovieInternal::~MovieInternal()
{
}

void MovieInternal::Advance(Falcon::AdvanceInterface& rAdvanceInterface)
{
	if (m_pRoot.IsValid())
	{
		m_pAdvanceInterface->SetInterface(&rAdvanceInterface);
		{
			m_pRoot->Advance(*m_pAdvanceInterface);
		}
		{
			m_pAdvanceInterface->DispatchEvents();
		}
		m_pAdvanceInterface->SetInterface(nullptr);
	}
}

void MovieInternal::Advance(
	Falcon::AdvanceInterface& rAdvanceInterface,
	Float fDeltaTimeInSeconds)
{
	// Tolerance used to avoid error build up in the accumulation buffer
	// and to allow a bit of undershoot. Currently set to 1 ms pending
	// further testing and refinement.
	static const Float kfAccumulationSlopInSeconds = (Float)(1.0 / 1000.0);

	SharedPtr<Falcon::FCNFile> pFile(GetFCNFile());
	Float const fFramesPerSecond = (pFile.IsValid() ? pFile->GetFramesPerSecond() : 30.0f);
	Float const fTargetFrameTimeInSeconds = (1.0f / fFramesPerSecond);
	m_fAccumulatedFrameTime += fDeltaTimeInSeconds;
	while (m_fAccumulatedFrameTime + kfAccumulationSlopInSeconds >= fTargetFrameTimeInSeconds)
	{
		Advance(rAdvanceInterface);
		m_fAccumulatedFrameTime -= fTargetFrameTimeInSeconds;
	}

	// If our accumulated time after processing is at or
	// below kfAccumulationSlopInSeconds, just zero it out.
	if (m_fAccumulatedFrameTime <= kfAccumulationSlopInSeconds)
	{
		m_fAccumulatedFrameTime = 0.0f;
	}
}

void MovieInternal::Pose(
	Movie* pMovie,
	Renderer& rRenderer)
{
	SharedPtr<Falcon::FCNFile> pFile(GetFCNFile());
	if (m_pRoot.IsValid() && pFile.IsValid())
	{
		// Sanity check/regression for a bug - due to incorrect ordering
		// of actions in PrePose(), it used to be possible for
		// rendering to be called on frame0 of a movie, prior to
		// dispatch of the initialization events. In short,
		// the advance interface must be fully processed prior to
		// render invocation, every frame, to guarantee a movie
		// has been fully initialized before it is displayed.
		SEOUL_ASSERT(!m_pAdvanceInterface->HasEventsToDispatch());

		rRenderer.BeginMovie(pMovie, pFile->GetBounds());
		rRenderer.PoseRoot(m_pRoot);
		rRenderer.EndMovie(pMovie->FlushesDeferredDraw());
	}
}

#if SEOUL_ENABLE_CHEATS
/**
 * Developer only method, performs a render pass to visualize input hit testable
 * areas.
 */
void MovieInternal::PoseInputVisualization(
	const InputWhitelist& inputWhitelist,
	Movie* pMovie,
	UInt8 uInputMask,
	Renderer& rRenderer)
{
	if (!m_pRoot.IsValid())
	{
		return;
	}

	SharedPtr<Falcon::FCNFile> pFile(GetFCNFile());
	if (!pFile.IsValid())
	{
		return;
	}

	rRenderer.BeginMovie(pMovie, pFile->GetBounds());
	rRenderer.PoseInputVisualization(inputWhitelist, uInputMask, m_pRoot);
	rRenderer.EndMovie(pMovie->FlushesDeferredDraw());
}
#endif // /#if SEOUL_ENABLE_CHEATS

void MovieInternal::InternalInitializeEmptyMovie()
{
	m_hFCNFileData.Reset();
	m_pRoot.Reset(SEOUL_NEW(MemoryBudgets::Falcon) Falcon::MovieClipInstance(SharedPtr<Falcon::MovieClipDefinition>(
		SEOUL_NEW(MemoryBudgets::Falcon) Falcon::MovieClipDefinition(m_TypeName))));
	m_pAdvanceInterface.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) AdvanceInterfaceDeferredDispatch);
}

void MovieInternal::InternalInitializeMovieFromFilePath()
{
	// If no FilePath, just initialize an empty movie.
	if (!m_FilePath.IsValid())
	{
		InternalInitializeEmptyMovie();
		return;
	}

	// Acquire the content handle to the movie's FCN data and wait
	// for it to finish loading (it is the responsibility of the UI system
	// to prefetch movies in order to avoid a busy wait here).
	Content::Handle<FCNFileData> hFCNFileData = Manager::Get()->GetFCNFileData(m_FilePath);
	{
		Content::LoadManager::Get()->WaitUntilLoadIsFinished(hFCNFileData);
	}
	SharedPtr<FCNFileData> pFileData(hFCNFileData.GetPtr());

	// If we ended up with no data, warn about the error,
	// initialize an empty movie and return.
	if (!pFileData.IsValid())
	{
		SEOUL_WARN("%s: failed to initialize, FCN file %s failed to load or does not exist.",
			m_TypeName.CStr(),
			m_FilePath.CStr());

		InternalInitializeEmptyMovie();
		return;
	}

	// Setup our variables and instances.
	m_hFCNFileData = hFCNFileData;
	pFileData->CloneTo(m_pRoot, m_pAdvanceInterface);
}

} // namespace Seoul::UI
