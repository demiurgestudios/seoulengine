/**
 * \file UIMovieInternal.h
 * \brief Internal class, owned by UI::Movie, handles
 * some low-level details of wrapping a Falcon scene graph.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_MOVIE_INTERNAL_H
#define UI_MOVIE_INTERNAL_H

#include "FalconAdvanceInterface.h"
#include "JobsJob.h"
#include "Prereqs.h"
#include "UIData.h"
namespace Seoul { namespace Falcon { class MovieClipInstance; } }

namespace Seoul
{

// Forward declarations
class RenderSurface2D;

namespace UI
{

class AdvanceInterfaceDeferredDispatch; 
class MovieInternalLoadJob;
class Renderer;

class MovieInternal SEOUL_SEALED
{
public:
	MovieInternal(FilePath movieFilePath, HString typeName);
	~MovieInternal();

	// Used in a few special cases, particularly initialization.
	// Normally events are dispatched as part of advance.
	void DispatchEvents(Falcon::AdvanceInterface& rAdvanceInterface);
	AdvanceInterfaceDeferredDispatch& GetDeferredDispatch() { return *m_pAdvanceInterface; }
	void Initialize();

	/** @return The Seoul UI::Data handle associated with this movie. */
	const Content::Handle<FCNFileData>& GetFCNFileData() const
	{
		return m_hFCNFileData;
	}

	/** @return The Falcon FCNFile data associated with this movie, if valid. */
	SharedPtr<Falcon::FCNFile> GetFCNFile() const
	{
		SharedPtr<FCNFileData> pFCNFileData(GetFCNFileData().GetPtr());
		if (pFCNFileData.IsValid())
		{
			return pFCNFileData->GetFCNFile();
		}

		return SharedPtr<Falcon::FCNFile>();
	}

	const SharedPtr<Falcon::MovieClipInstance>& GetRoot() const
	{
		return m_pRoot;
	}

	// Call once to perform a manual (single step) advance.
	void Advance(Falcon::AdvanceInterface& rAdvanceInterface);

	// Call once per frame to check and potentially advance the Falcon movie.
	void Advance(Falcon::AdvanceInterface& rAdvanceInterface, Float fDeltaTimeInSeconds);

	// Display the current frame state of the Falcon movie.
	void Pose(
		Movie* pMovie,
		Renderer& rRenderer);

#if SEOUL_ENABLE_CHEATS
	// Developer only method, performs a render pass to visualize input hit testable
	// areas.
	typedef HashSet< SharedPtr<Falcon::MovieClipInstance>, MemoryBudgets::UIData > InputWhitelist;
	void PoseInputVisualization(
		const InputWhitelist& inputWhitelist,
		Movie* pMovie,
		UInt8 uMask,
		Renderer& rRenderer);
#endif // /#if SEOUL_ENABLE_CHEATS

private:
	Content::Handle<FCNFileData> m_hFCNFileData;
	SharedPtr<Falcon::MovieClipInstance> m_pRoot;
	ScopedPtr<AdvanceInterfaceDeferredDispatch> m_pAdvanceInterface;
	FilePath m_FilePath;
	Float m_fAccumulatedFrameTime;
	HString m_TypeName;

	void InternalInitializeEmptyMovie();
	void InternalInitializeMovieFromFilePath();

	SEOUL_DISABLE_COPY(MovieInternal);
}; // class UI::MovieInternal

} // namespace UI

} // namespace Seoul

#endif // include guard
