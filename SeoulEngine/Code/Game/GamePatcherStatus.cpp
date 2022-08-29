/**
 * \file GamePatcherStatus.cpp
 * \brief Screen framework for displaying progress and status
 * of the patching process.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DownloadablePackageFileSystem.h"
#include "Engine.h"
#include "FalconMovieClipInstance.h"
#include "GameAnalytics.h"
#include "GameAutomation.h"
#include "GameAuthManager.h"
#include "GameMain.h"
#include "GamePatcher.h"
#include "GamePatcherStatus.h"
#include "PatchablePackageFileSystem.h"
#include "ReflectionDefine.h"
#include "UIHitShapeInstance.h"
#include "UIManager.h"

namespace Seoul
{

namespace Game
{

static const HString kAuthenticatingToken("UI_Patcher_Authenticating");
static const HString kCloudSync("UI_Patcher_CloudSync");
static const HString kErrorToken("UI_Patcher_Error");
static const HString kFullScreenClipper("FullScreenClipper");
static const HString kInitialToken("UI_Patcher_Initial");
static const HString kInsufficientDiskSpaceToken("UI_Patcher_InsufficientDiskSpace");
static const HString kLoadToken("UI_Patcher_Load");
static const HString kLoadConfigToken("UI_Patcher_LoadConfig");
static const HString kLoadContentToken("UI_Patcher_LoadContent");
static const HString kNoConnectionToken("UI_Patcher_NoConnection");
static const HString kOnPatcherStatusFirstRender("OnPatcherStatusFirstRender");
static const HString kPatchApplyToken("UI_Patcher_Apply");
static const HString kPatcherVisible("PatcherVisible");
static const HString kPrecacheUrls("UI_Patcher_PrecacheUrls");

// TODO: Move this into a config variable.

/** Minimum display time of a particular patching state. */
static const Double kfMinDisplayTimeInSeconds = 0.5;

} // namespace Game

SEOUL_BEGIN_TYPE(Game::PatcherStatus, TypeFlags::kDisableCopy)
	SEOUL_PARENT(UI::Movie)
	SEOUL_PROPERTY_N("MinimumDisplayTimeInSeconds", m_fMinimumDisplayTimeInSeconds)
		SEOUL_ATTRIBUTE(NotRequired)
SEOUL_END_TYPE()

/** Optional, download package file system that must be initialized once *_Config.sar initialization is complete. */
extern CheckedPtr<DownloadablePackageFileSystem> g_pDownloadableContentPackageFileSystem;

namespace Game
{

#if SEOUL_ENABLE_CHEATS
Bool PatcherStatus::s_bDevOnlyDisableMinimumDisplay = false;
#endif // /#if SEOUL_ENABLE_CHEATS

PatcherStatus::PatcherStatus()
	: m_bScrollOut(false)
	, m_fElapsedDisplayTimeInSeconds(0.0f)
	, m_fMinimumDisplayTimeInSeconds(0.0f)
	, m_Tracking()
	, m_bFirstRender(false)
{
	// Mark as visible.
	UI::Manager::Get()->SetCondition(kPatcherVisible, true);
}

PatcherStatus::~PatcherStatus()
{
}

void PatcherStatus::OnConstructMovie(HString movieTypeName)
{
	UI::Movie::OnConstructMovie(movieTypeName);

	// Make sure we're clipped to the screen bounds.
	AddFullScreenClipper();
}

void PatcherStatus::OnLinkClicked(
	const String& sLinkInfo,
	const String& sLinkType,
	const SharedPtr<Falcon::MovieClipInstance>& pInstance)
{
	UI::Movie::OnLinkClicked(sLinkInfo, sLinkType, pInstance);

	if (sLinkType.IsEmpty())
	{
		Engine::Get()->OpenURL(sLinkInfo);
	}
}

void PatcherStatus::OnPose(
	RenderPass& rPass,
	UI::Renderer& rRenderer)
{
	UI::Movie::OnPose(rPass, rRenderer);

	// Check if we've begun rendering in full and if so,
	// broadcast an event for the Game::Patcher screen.
	if (!m_bFirstRender)
	{
		// Broadcast our render event.
		UI::Manager::Get()->BroadcastEvent(kOnPatcherStatusFirstRender);
		m_bFirstRender = true;
	}
}

void PatcherStatus::OnTick(RenderPass& rPass, Float fDeltaTimeInSeconds)
{
	UI::Movie::OnTick(rPass, fDeltaTimeInSeconds);

	// Accumulate display time.
	m_fElapsedDisplayTimeInSeconds += fDeltaTimeInSeconds;

	auto const pGamePatcher = Patcher::GetConst();
	auto const eState = (pGamePatcher.IsValid() ? pGamePatcher->GetState() : PatcherState::kDone);
	auto const fProgress = (pGamePatcher.IsValid() ? pGamePatcher->GetProgress() : 1.0f);
	Bool const bIsConnected = Main::Get()->IsConnectedToNetwork();

	Bool bRequiredVersionUpdate = false;
	AuthData data;
	if (AuthManager::Get()->GetAuthData(data))
	{
		bRequiredVersionUpdate = !data.m_RefreshData.m_VersionRequired.CheckCurrentBuild();
	}

	auto fMinimumDisplayTimeInSeconds = m_fMinimumDisplayTimeInSeconds;

	// If disabled, set to 0.
#if SEOUL_ENABLE_CHEATS
	if (s_bDevOnlyDisableMinimumDisplay) { fMinimumDisplayTimeInSeconds = 0.0f; }
#endif // /#if SEOUL_ENABLE_CHEATS

	// Check for fade out conditions - the status screen goes away once
	// we've hit 100% progress and we're in the done state.
	if (m_Tracking.GetDisplayProgress() >= 1.0f &&
		PatcherState::kDone == m_Tracking.GetLastState() &&
		m_fElapsedDisplayTimeInSeconds >= fMinimumDisplayTimeInSeconds)
	{
		SetScrollOut(true);
	}

	// Progress tracking.
	Float const fDisplay = m_Tracking.ApplyProgress(fDeltaTimeInSeconds, fProgress);
	HandleProgress(fDisplay);

	// Handle scroll out.
	auto const bScrolling = PerformScrollOut(fDeltaTimeInSeconds);

	// Check for dismiss condition - the screen fully dismisses once it has faded out.
	if (GetScrollOut() && !bScrolling)
	{
		UI::Manager::Get()->SetCondition(kPatcherVisible, false);
	}

	// Special case, force server down message to take priority.
	if (!Main::Get()->GetServerDownMessage().IsEmpty())
	{
		HandleServerDown(Main::Get()->GetServerDownMessage());
		return;
	}

	// Nothing to do if on the same state.
	if (!m_Tracking.UpdateState(eState, bIsConnected, bRequiredVersionUpdate))
	{
		return;
	}

	HandleUpdateRequired(bRequiredVersionUpdate);

	// Special cases - not connected or required version.
	if (bRequiredVersionUpdate)
	{
		return;
	}
	else if (!bIsConnected)
	{
		HandleSetText(kNoConnectionToken);
		return;
	}

	switch (eState)
	{
	case PatcherState::kGDPRCheck: // fall-through
	case PatcherState::kInitial: // fall-through
	case PatcherState::kWaitForAuth: HandleSetText(kAuthenticatingToken);
	case PatcherState::kWaitForRequiredVersion:
	case PatcherState::kWaitForPatchApplyConditions:
		HandleSetText(kInitialToken);
		break;
	case PatcherState::kInsufficientDiskSpace: // fall-through
	case PatcherState::kInsufficientDiskSpacePatchApply: HandleSetText(kInsufficientDiskSpaceToken); break;
	case PatcherState::kPatchApply: HandleSetText(kPatchApplyToken); break;
	case PatcherState::kWaitingForTextureCachePurge: // fall-through
	case PatcherState::kWaitingForContentReload:
		HandleSetText(kLoadToken);
		break;
	case PatcherState::kWaitingForGameConfigManager: HandleSetText(kLoadConfigToken); break;
#if SEOUL_WITH_GAME_PERSISTENCE
	case PatcherState::kWaitingForGamePersistenceManager: HandleSetText(kCloudSync); break;
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE
	case PatcherState::kWaitingForGameScriptManager: HandleSetText(kLoadContentToken); break;
	case PatcherState::kWaitingForPrecacheUrls: HandleSetText(kPrecacheUrls); break;

		// These states hold existing message.
	case PatcherState::kGameInitialize:
	case PatcherState::kDone:
	case PatcherState::kRestarting:
		break;

	case PatcherState::kWaitingForContentReloadAfterError: HandleSetText(kErrorToken); break;
	case PatcherState::COUNT: break; // COUNT is a delimiter, not a valid part of the state machine
	};
}

// NOTE: This functionality is in large part copy and pasted
// from the script definition of MovieScroller.cs and
// utility functions in ScriptUIMovieClipInstance.cpp. This is
// because patching exists without game code (and no script VM)
// and must remain isolated.

/**
* Utility - shared by AddFullScreenClipper and AddNativeChildHitShapeFullScreen,
* returns a full screen sized bounds centered at the origin.
*/
static inline Falcon::Rectangle GetCenteredFullScreenBounds(const UI::Movie& movie)
{
	// Generate the bounds from the viewport.
	const Falcon::Rectangle bounds = movie.ViewportToWorldBounds();

	// Recenter - we always want the bounds centered around (0, 0, 0)
	// so that is scrolled properly.
	auto const fWidth = bounds.GetWidth();
	auto const fHeight = bounds.GetHeight();
	auto const vCenter = bounds.GetCenter();
	return Falcon::Rectangle::Create(
		vCenter.X - fWidth * 0.5f,
		vCenter.X + fWidth * 0.5f,
		vCenter.Y - fHeight * 0.5f,
		vCenter.Y + fHeight * 0.5f);
}

void PatcherStatus::AddFullScreenClipper()
{
	// Signed 16-bit max value.
	static const HString kDefaultMovieClipClassName("MovieClip");
	static const UInt16 kuMaxClipDepth = 32767;

	SharedPtr<Falcon::MovieClipInstance> pInstance;
	if (!GetRootMovieClip(pInstance))
	{
		return;
	}

	// Clipper must go first, so check for an existing element. If already
	// a clipper, nothing to do. If not a clipper, check its depth - if
	// a depth of 0, we need to push back all existing children to make
	// room for the clipper.
	//
	// Depth of 0 is special - Flash timelines always place children
	// at a depth of at least 1, but Falcon code is fine with usage of
	// 0 depth. As such, we use this "reserved" depth to place the clipper
	// in front of all other children under normal usage circumstances.
	// This avoids the need to push back elements (and also of movie clip
	// timelines in the root fighting with this runtime change).
	SharedPtr<Falcon::Instance> pChild;
	if (pInstance->GetChildAt(0, pChild))
	{
		// Check if already a clipper.
		if (pChild->GetType() == Falcon::InstanceType::kMovieClip)
		{
			auto pMovieClipChild = (Falcon::MovieClipInstance*)pChild.GetPtr();
			if (pMovieClipChild->GetScissorClip() &&
				pMovieClipChild->GetClipDepth() == kuMaxClipDepth)
			{
				// This is already a clipper, we're done.
				goto done;
			}
		}

		// One way or another, we need to insert a clipper, so
		// check depth - if 0, we need to push back all existing elements.
		if (pChild->GetDepthInParent() <= 0)
		{
			// Push back all children by 1 depth value so the clipper is first.
			pInstance->IncreaseAllChildDepthByOne();
		}
	}

	// If we get here, generate a clipper MovieClip.
	{
		SharedPtr<Falcon::MovieClipInstance> pClipper(SEOUL_NEW(MemoryBudgets::Falcon) Falcon::MovieClipInstance(SharedPtr<Falcon::MovieClipDefinition>(
			SEOUL_NEW(MemoryBudgets::Falcon) Falcon::MovieClipDefinition(kDefaultMovieClipClassName))));

		// Clipper shape is a hit shape with viewport bounds.
		// Generate the bounds from the viewport.
		auto const bounds = GetCenteredFullScreenBounds(*this);

		// Generate the hit shape that will size the clipper.
		SharedPtr<UI::HitShapeInstance> pChildInstance(SEOUL_NEW(MemoryBudgets::UIRuntime) UI::HitShapeInstance(
			bounds));

		// Set the clipper's hit shape.
		pClipper->SetChildAtDepth(*this, 1u, pChildInstance);

		// The clipper has a max clip depth and is a scissor clip for perf.
		pClipper->SetClipDepth(kuMaxClipDepth);
		pClipper->SetScissorClip(true);
		pClipper->SetName(kFullScreenClipper);

		// Now insert the clipper itself - place at depth 0
		// to give it special placement in front of everything else.
		pInstance->SetChildAtDepth(*this, 0u, pClipper);
	}

done:
	return;
}

Bool PatcherStatus::PerformScrollOut(Float fDeltaTimeInSeconds)
{
	static const Float32 kfAutoScrollDecelerationStrength = 1.2f;
	static const Float32 kfAutoScrollSpeedFactor = 2.0f / 15.0f;
	static const Float32 kfAutoScrollDecelerateDistanceFactor = 8.0f / 15.0f;
	static const Float32 kfSnapThresholdFactor = 0.002f;

	SharedPtr<Falcon::MovieClipInstance> pRoot;
	if (!GetRootMovieClip(pRoot))
	{
		return false;
	}

	if (!m_bScrollOut)
	{
		pRoot->SetPositionX(0.0f);
		return false;
	}

	auto const bounds = ViewportToWorldBounds();
	auto fBaseX = pRoot->GetPosition().X;
	auto const fWidth = bounds.GetWidth();
	auto const fTarget = -fWidth;

	// Total distance left to auto scroll.
	auto const fDistance = Abs(fTarget - fBaseX);

	// Adjustment to apply - constants were tuned at 60 FPS, so we
	// rescale delta time by that value.
	auto fAdjust = kfAutoScrollSpeedFactor * fWidth * (fDeltaTimeInSeconds * 60.0f);

	// We start decelerating when we're closer than this distance.
	auto const fSlowdownDistance = kfAutoScrollDecelerateDistanceFactor * fWidth;

	// Apply slodown.
	if (fDistance <= fSlowdownDistance)
	{
		auto fSlowdown = (fDistance / fSlowdownDistance);
		fSlowdown = Pow(fSlowdown, kfAutoScrollDecelerationStrength);
		fAdjust *= fSlowdown;
	}

	// Swap the directionality of adjustment as needed.
	if (fBaseX > fTarget) { fAdjust = -fAdjust; }

	// Apply.
	fBaseX += fAdjust;

	// If we've either gone passed, or if we're within
	// a threshold, snap to target.
	auto bReturn = true;
	if ((fBaseX <= fTarget && fAdjust < 0.0) ||
		(fBaseX >= fTarget && fAdjust > 0.0) ||
		Abs(fBaseX - fTarget) <= kfSnapThresholdFactor * fWidth)
	{
		fBaseX = fTarget;
		bReturn = false;
	}

	// Done, set new position.
	pRoot->SetPositionX(fBaseX);
	return bReturn;
}

// END NOTE:

/** Apply a progress update - applies interpolation and smoothing. */
Float PatcherStatus::StateTracking::ApplyProgress(Float fDeltaTimeInSeconds, Float fProgress)
{
	// TODO: Move this into a config variable.
	static const Float kfMaxChangePerSecond = 1.0f;

	m_fTargetProgress = Clamp(Max(m_fTargetProgress, fProgress), 0.0f, 1.0f);

	if (fDeltaTimeInSeconds > fEpsilon)
	{
		Float const fDelta = Min((m_fTargetProgress - m_fDisplayProgress) / fDeltaTimeInSeconds, kfMaxChangePerSecond) * fDeltaTimeInSeconds;
		m_fDisplayProgress = Clamp(m_fDisplayProgress + fDelta, 0.0f, 1.0f);
	}

	return m_fDisplayProgress;
}

/** Attempt to update the current state - returns true on successful update, false otherwise. */
Bool PatcherStatus::StateTracking::UpdateState(
	PatcherState eState,
	Bool bIsConnected,
	Bool bRequiredVersionUpdate)
{
	auto const iTime = SeoulTime::GetGameTimeInTicks();
	auto const fDelta = SeoulTime::ConvertTicksToSeconds(iTime - m_iStateChangeTimeInTicks);
	if (m_iStateChangeTimeInTicks < 0 ||
		((m_eLastState != eState || m_bIsConnected != bIsConnected || m_bRequiredVersionUpdate != bRequiredVersionUpdate)
		&& fDelta >= kfMinDisplayTimeInSeconds))
	{
		m_eLastState = eState;
		m_bIsConnected = bIsConnected;
		m_bRequiredVersionUpdate = bRequiredVersionUpdate;
		m_iStateChangeTimeInTicks = iTime;
		return true;
	}

	return false;
}

} // namespace Game

} // namespace Seoul
