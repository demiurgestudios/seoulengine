/**
 * \file GamePatcher.cpp
 * \brief Screens and logic involved in the patching process.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ApplicationJson.h"
#include "DownloadablePackageFileSystem.h"
#include "Engine.h"
#include "FalconMovieClipInstance.h"
#include "FileManager.h"
#include "FileManagerRemap.h"
#include "GameAnalytics.h"
#include "GameAuthData.h"
#include "GameAuthManager.h"
#include "GameAutomation.h"
#include "GameClient.h"
#include "GameConfigManager.h"
#include "EventsManager.h"
#include "GameMain.h"
#include "GamePatcher.h"
#include "GamePaths.h"
#include "GameScriptManager.h"
#include "GameScriptManagerVmCreateJob.h"
#include "HTTPManager.h"
#include "HTTPResponse.h"
#include "LocManager.h"
#include "PatchablePackageFileSystem.h"
#include "Prereqs.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#include "ScriptFunctionInvoker.h"
#include "SettingsManager.h"
#include "UIManager.h"
#include "UIRenderer.h"

#if SEOUL_LOGGING_ENABLED
#include "ReflectionSerialize.h"
#endif // /#if SEOUL_LOGGING_ENABLED

namespace Seoul
{

/** Optional, download package file system that must be initialized once *_Config.sar initialization is complete. */
CheckedPtr<DownloadablePackageFileSystem> g_pDownloadableContentPackageFileSystem;

#if SEOUL_UNIT_TESTS
/** Test only hook for simulating restart conditions of the patcher. */
static Game::PatcherState s_eUnitTesting_SimulateRestartOnComplete = Game::PatcherState::COUNT;
void UnitTestingHook_SetGamePatcherSimulateRestartState(Game::PatcherState eState)
{
	s_eUnitTesting_SimulateRestartOnComplete = eState;
}
#endif // /#if SEOUL_UNIT_TESTS

namespace Game
{

static HString const kGameLoaded("GameLoaded");
static HString const kPatcherFriendly("PatcherFriendly");
static HString const kPendingSoftReboot("PendingSoftReboot");
static HString const ksMainScriptFileName("MainScriptFileName");
static HString const kSeoulIsFullyInitialized("SeoulIsFullyInitialized");

static const Float kfApplyProgressWeight = 0.10f;
static const Float kfLoadProgressWeight = 0.1f;
static const Float kfScriptProgressWeight = 0.80f;

/**
 * @return FilePath to the script main entry point.
 */
static inline String GetScriptMainFilePath()
{
	String sReturn;
	(void)GetApplicationJsonValue(ksMainScriptFileName, sReturn);
	return sReturn;
}

/** Populates settings to instantiate ScriptUI, from MainSettings and the global GetScriptMainFilePath(). */
inline static ScriptManagerSettings GetScriptUISettings(const MainSettings& settings)
{
	ScriptManagerSettings returnSettings;
	returnSettings.m_ScriptErrorHandler = settings.m_ScriptErrorHandler;
	returnSettings.m_InstantiatorOverride = settings.m_InstantiatorOverride;
	returnSettings.m_sMainScriptFileName = GetScriptMainFilePath();
	return returnSettings;
}

/** Utility, handles applying a patch off main thread, to avoid hitches. */
class PatcherApplyJob SEOUL_SEALED : public Jobs::Job
{
public:
	PatcherApplyJob()
		: m_uTotalSize(0u)
		, m_uConfigProgress(0u)
		, m_uContentProgress(0u)
		, m_bConfigSuccess(false)
		, m_bContentSuccess(false)
		, m_bRemapSuccess(false)
		, m_bConfigWriteFailure(false)
		, m_bContentWriteFailure(false)
	{
	}

	~PatcherApplyJob()
	{
		WaitUntilJobIsNotRunning();
	}

	typedef HashTable<HString, PatcherDisplayStat, MemoryBudgets::Game> ApplySubStats;

	/** Swap out stats gathered during apply processing. */
	void AcquireStats(ApplySubStats& rt)
	{
		// Accumulate into.
		for (auto const& pair : m_tApplySubStats)
		{
			auto p = rt.Find(pair.First);
			if (nullptr == p)
			{
				auto const e = rt.Insert(pair.First, PatcherDisplayStat());
				SEOUL_ASSERT(e.Second);
				p = &e.First->Second;
			}

			p->m_fTimeSecs += pair.Second.m_fTimeSecs;
			p->m_uCount += pair.Second.m_uCount;
		}

		// Zero out.
		m_tApplySubStats.Clear();
	}

	/** @return True if either patch file is failing to write (typically due to out of disk space). */
	Bool IsExperiencingWriteFailure() const
	{
		return m_bConfigWriteFailure || m_bContentWriteFailure;
	}

	/** @return True if the config package was successfully updated, false otherwise. */
	Bool GetConfigSuccess() const
	{
		return m_bConfigSuccess;
	}

	/** @return True if the content package was successfully updated, false otherwise. */
	Bool GetContentSuccess() const
	{
		return m_bContentSuccess;
	}

	/** @return True if the remap table was successfully applied, false otherwise. */
	Bool GetRemapSuccess() const
	{
		return m_bRemapSuccess;
	}

	/** @return The total download progress - only greater than zero when downloading is active. */
	UInt64 GetTotalProgress() const
	{
		return (m_uConfigProgress + m_uContentProgress);
	}

	/** @return The total download size - only greater than zero when downloading is active/necessary. */
	UInt64 GetTotalSize() const
	{
		return (m_uTotalSize);
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(PatcherApplyJob);

#	define SEOUL_MARK_TIME(name) { \
		static const HString kSymbol##name(#name); \
		auto const iTime = SeoulTime::GetGameTimeInTicks(); \
		auto const iDelta = (iTime - iLastTime); \
		SEOUL_LOG_ENGINE("GamePatcher (apply_%s): %.2f ms", #name, SeoulTime::ConvertTicksToMilliseconds(iDelta)); \
		OnState(kSymbol##name, iDelta); \
		iLastTime = iTime; }

	void OnState(HString name, Int64 iTicks)
	{
		auto p = m_tApplySubStats.Find(name);
		if (nullptr == p)
		{
			auto const e = m_tApplySubStats.Insert(name, PatcherDisplayStat());
			SEOUL_ASSERT(e.Second);
			p = &e.First->Second;
		}

		p->m_fTimeSecs += (Float)SeoulTime::ConvertTicksToSeconds(iTicks);
		++p->m_uCount;
	}

	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE
	{
		auto iLastTime = SeoulTime::GetGameTimeInTicks();

		SEOUL_MARK_TIME(start);

		// Failure by default.
		m_bConfigSuccess = false;
		m_bContentSuccess = false;
		m_bRemapSuccess = false;

		CheckedPtr<PatchablePackageFileSystem> pConfig = Main::Get()->GetConfigUpdatePackageFileSystem();
		CheckedPtr<PatchablePackageFileSystem> pContent = Main::Get()->GetContentUpdatePackageFileSystem();

		// Return with an error if we failed to acquire auth data.
		AuthData data;
		if (!AuthManager::Get()->GetAuthData(data))
		{
			reNextState = Jobs::State::kError;
			return;
		}

		// We are a low priority job waiting on other
		// work for the remainder of this block.
		{
			Jobs::ScopedQuantum scope(*this, Jobs::Quantum::kWaitingForDependency);

			// Config and content systems, set URLs.
			if (pConfig) { pConfig->SetURL(data.m_RefreshData.m_sConfigUpdateUrl); }
			if (pContent) { pContent->SetURL(data.m_RefreshData.m_sContentUpdateUrl); }

			SEOUL_MARK_TIME(set_url);

			// Bulk of .sar population.
			{
				// Wait for initialization to complete.
				while (pConfig && pConfig->IsInitializing())
				{
					m_bConfigWriteFailure = pConfig->HasExperiencedWriteFailure();
					Jobs::Manager::Get()->YieldThreadTime();
				}
				m_bConfigWriteFailure = false;

				SEOUL_MARK_TIME(sar_config_init);

				while (pContent && pContent->IsInitializing())
				{
					m_bContentWriteFailure = pContent->HasExperiencedWriteFailure();
					Jobs::Manager::Get()->YieldThreadTime();
				}
				m_bContentWriteFailure = false;

				SEOUL_MARK_TIME(sar_content_init);

				// Gather content files to download.
				PatchablePackageFileSystem::Files vContentFiles;
				if (pContent)
				{
					IPackageFileSystem::FileTable tFiles;
					if (pContent->GetFileTable(tFiles) && !tFiles.IsEmpty())
					{
						vContentFiles.Reserve(tFiles.GetSize());

						auto const iBegin = tFiles.Begin();
						auto const iEnd = tFiles.End();
						for (auto i = iBegin; iEnd != i; ++i)
						{
							auto const filePath = i->First;
							auto const eType = filePath.GetType();

							if (eType != FileType::kTexture0 &&
								eType != FileType::kTexture1 &&
								eType != FileType::kTexture2)
							{
								vContentFiles.PushBack(filePath);
							}
						}
					}
				}

				SEOUL_MARK_TIME(build_content_list);

				m_uTotalSize = 0u;
				m_uTotalSize += GetTotalSize(pConfig, PatchablePackageFileSystem::Files());
				m_uTotalSize += GetTotalSize(pContent, vContentFiles);
				m_uConfigProgress = 0u;
				m_uContentProgress = 0u;

				SEOUL_MARK_TIME(calculate_total_size);

				// Config, fetch all.
				if (pConfig)
				{
					m_bConfigSuccess = pConfig->Fetch(
						PatchablePackageFileSystem::Files(),
						SEOUL_BIND_DELEGATE(&DownloadProgressUtilStatic, (void*)&m_uConfigProgress));
				}
				else
				{
					// Config is successful if it doesn't exist.
					m_bConfigSuccess = true;
				}

				SEOUL_MARK_TIME(sar_config_fetch);

				// Content, prefetch all, exception texture mip levels lower than the last,
				// and audio banks. Nop if no files - passing an empty vector means "fetch all",
				// which is not what we want.
				if (pContent && !vContentFiles.IsEmpty())
				{
					m_bContentSuccess = pContent->Fetch(
						vContentFiles,
						SEOUL_BIND_DELEGATE(&DownloadProgressUtilStatic, (void*)&m_uContentProgress));
				}
				else
				{
					m_bContentSuccess = true;
				}
			}

			SEOUL_MARK_TIME(sar_content_fetch);
		}

		// Result tracking.
		if (m_bConfigSuccess && m_bContentSuccess)
		{
			m_bRemapSuccess = LoadAndApplyFileManagerRemap();
		}
		else
		{
			m_bRemapSuccess = false;
		}

		SEOUL_MARK_TIME(apply_remap);

		reNextState = Jobs::State::kComplete;
	}

#undef SEOUL_MARK_TIME
#undef SEOUL_MARK_TIME_

	/** Binding against a progress variable, used to track .sar population/download progress. */
	static void DownloadProgressUtilStatic(void* p, UInt64 /*zDownloadSizeInBytes*/, UInt64 zDownloadSoFarInBytes)
	{
		auto& r = *((UInt64*)p);
		r = zDownloadSoFarInBytes;
	}

	/** Utility, accumulates the total (compressed) size of all listed files (or all files, if v is empty). */
	static UInt64 GetTotalSize(CheckedPtr<PatchablePackageFileSystem> p, const PatchablePackageFileSystem::Files& v)
	{
		// Acquire package file table
		IPackageFileSystem::FileTable t;
		if (!p || !p->GetFileTable(t))
		{
			return 0u;
		}

		// Accumulate.
		UInt64 uReturn = 0u;

		// All files.
		if (v.IsEmpty())
		{
			auto const iBegin = t.Begin();
			auto const iEnd = t.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				uReturn += i->Second.m_Entry.m_uCompressedFileSize;
			}
		}
		// Limited list of files.
		else
		{
			auto const iBegin = v.Begin();
			auto const iEnd = v.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				auto p = t.Find(*i);
				if (nullptr == p)
				{
					continue;
				}

				uReturn += p->m_Entry.m_uCompressedFileSize;
			}
		}

		return uReturn;
	}

	static Bool LoadAndApplyFileManagerRemap()
	{
		AuthData data;
		if (!AuthManager::Get()->GetAuthData(data))
		{
			SEOUL_WARN("Remap configuration file could not be applied, no auth data.\n");
			return false;
		}

		auto const iBegin = data.m_RefreshData.m_vRemapConfigs.Begin();
		auto const iEnd = data.m_RefreshData.m_vRemapConfigs.End();
		FileManagerRemap::RemapTable tRemap;
		for (auto i = iBegin; iEnd != i; ++i)
		{
			auto pSettings = SettingsManager::Get()->WaitForSettings(*i);
			if (!pSettings.IsValid())
			{
				SEOUL_WARN("Failed loading remap configuration file: %s\n", i->CStr());
				return false;
			}

			if (!FileManagerRemap::Merge(*pSettings, pSettings->GetRootNode(), tRemap))
			{
				SEOUL_WARN("Failed merging values from remap configuration file: %s\n", i->CStr());
				return false;
			}
		}

		// Apply the remap and return success.
		FileManager::Get()->ConfigureRemap(tRemap, FileManagerRemap::ComputeHash(iBegin, iEnd));
		return true;
	}

	ApplySubStats m_tApplySubStats;
	UInt64 m_uTotalSize;
	UInt64 m_uConfigProgress;
	UInt64 m_uContentProgress;
	Bool m_bConfigSuccess;
	Bool m_bContentSuccess;
	Bool m_bRemapSuccess;
	Atomic32Value<Bool> m_bConfigWriteFailure;
	Atomic32Value<Bool> m_bContentWriteFailure;

	SEOUL_DISABLE_COPY(PatcherApplyJob);
}; // class Game::PatcherApplyJob

Bool Patcher::StayOnLoadingScreen{};

} // namespace Game

SEOUL_BEGIN_TYPE(Game::Patcher, TypeFlags::kDisableCopy)
	SEOUL_PARENT(UI::Movie)
	SEOUL_METHOD(OnPatcherStatusFirstRender)
	SEOUL_PROPERTY_N("PrecacheUrls", m_vPrecacheUrls)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_CMDLINE_PROPERTY(StayOnLoadingScreen, "StayOnLoadingScreen")
		SEOUL_ATTRIBUTE(NotRequired)
SEOUL_END_TYPE()

namespace Game
{

Patcher::Patcher()
	: m_vPrecacheUrls()
	, m_StartUptime(Engine::Get()->GetUptime())
	, m_LastStateChangeUptime(m_StartUptime)
	, m_fElapsedDisplayTimeInSeconds(0.0f)
	, m_pApplyJob()
	, m_pGameConfigManagerLoadJob()
#if SEOUL_WITH_GAME_PERSISTENCE
	, m_pGamePersistenceManagerLoadJob()
#endif
	, m_CachedUrls(0)
	, m_pVmCreateJob()
	, m_ContentPending()
	, m_fApplyProgress(0.0f)
	, m_fLoadProgress(0.0f)
	, m_fScriptProgress(0.0f)
	, m_eState(PatcherState::kGDPRCheck)
	, m_bPatcherStatusLoading(true)
	, m_bSentDiskWriteFailureAnalytics(false)
{
	SEOUL_LOG_ENGINE("GamePatcher()");

	Analytics::OnPatcherOpen();

	Content::LoadManager::Get()->BeginHotLoadSuppress();
	SettingsManager::Get()->BeginUnloadSuppress();
}

Patcher::~Patcher()
{
	// Terminate the script load job, if it is still running.
	if (m_pVmCreateJob.IsValid())
	{
		// Make sure we don't leave a dangling VM create job.
		if (m_pVmCreateJob->IsJobRunning())
		{
			// TODO: This is to make sure a startup cloud load doesn't block
			// VM creation. This sort of thing is ugly, but it does happen occasionally
			// and I don't have a better way to resolve it right now.
			if (Client::Get())
			{
				Client::Get()->CancelPendingRequests(); SEOUL_TEARDOWN_TRACE();
			}
			m_pVmCreateJob->RaiseInterrupt(); SEOUL_TEARDOWN_TRACE();
			m_pVmCreateJob->WaitUntilJobIsNotRunning(); SEOUL_TEARDOWN_TRACE();
		}

		// Need to make sure the VM itself is released now or it can linger
		// passed when it is expected/supposed to (it will otherwise be
		// released with the Jobs::Manager release the Job, which can be
		// a few ticks in the future). Call with no receive since we're just
		// dropping the reference entirely.
		m_pVmCreateJob->TakeOwnershipOfVm();
		m_pVmCreateJob.Reset();
	}

#if SEOUL_WITH_GAME_PERSISTENCE
	// Terminate the persistence load job if it is still running.
	if (m_pGamePersistenceManagerLoadJob.IsValid())
	{
		m_pGamePersistenceManagerLoadJob->WaitUntilJobIsNotRunning(); SEOUL_TEARDOWN_TRACE();
		m_pGamePersistenceManagerLoadJob.Reset(); SEOUL_TEARDOWN_TRACE();
	}
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE

	// Terminate the config load job if it is still running.
	if (m_pGameConfigManagerLoadJob.IsValid())
	{
		m_pGameConfigManagerLoadJob->WaitUntilJobIsNotRunning(); SEOUL_TEARDOWN_TRACE();
		m_pGameConfigManagerLoadJob.Reset(); SEOUL_TEARDOWN_TRACE();
	}

	// Terminate the apply job if it is still running.
	if (m_pApplyJob.IsValid())
	{
		m_pApplyJob->WaitUntilJobIsNotRunning(); SEOUL_TEARDOWN_TRACE();
		m_pApplyJob.Reset(); SEOUL_TEARDOWN_TRACE();
	}

	SettingsManager::Get()->EndUnloadSuppress();
	Content::LoadManager::Get()->EndHotLoadSuppress();

	// Report times.
	auto const patcherUptime = (Engine::Get()->GetUptime() - m_StartUptime);
	SEOUL_LOG_ENGINE("~GamePatcher(): (%.2f s)", patcherUptime.GetSecondsAsDouble());

	// Update analytics about the display time of the patcher.
	Analytics::OnPatcherClose(patcherUptime, m_fElapsedDisplayTimeInSeconds, m_Stats);

	// Also report patcher times to the automation system, it may log warnings when certain
	// thresholds are exceeded.
	if (Automation::Get())
	{
		Automation::Get()->OnPatcherClose(
			m_fElapsedDisplayTimeInSeconds,
			m_Stats);
	}
}

/** @return Total operation progress - range is [0, 1]. */
Float Patcher::GetProgress() const
{
	return (
		kfApplyProgressWeight * m_fApplyProgress +
		kfLoadProgressWeight * m_fLoadProgress +
		kfScriptProgressWeight * m_fScriptProgress);
}

void Patcher::OnPatcherStatusFirstRender()
{
	m_bPatcherStatusLoading = false;
}

void Patcher::SetState(PatcherState eState)
{
	if (m_eState != eState)
	{
#if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS
		// Additional warning if hit kRestarting, except when we're
		// coming from the insufficient disk space state, since
		// we have no control over mobile devices in our automated
		// testing.
		if (PatcherState::kRestarting == eState &&
			PatcherState::kInsufficientDiskSpace != m_eState &&
			PatcherState::kInsufficientDiskSpacePatchApply != m_eState)
#else
		// Additional warning if hit kRestarting - kRestarting is
		// always an exceptional case and one we want to yell about
		// in testing and most other contexts.
		if (PatcherState::kRestarting == eState)
#endif
		{
			SEOUL_WARN("Unexpected GamePatcher in kRestarting state.");
		}

		auto const uptime = Engine::Get()->GetUptime();
		SEOUL_LOG_ENGINE("GamePatcher: %s -> %s (%.2f s)",
			EnumToString<PatcherState>(m_eState),
			EnumToString<PatcherState>(eState),
			(uptime - m_LastStateChangeUptime).GetSecondsAsDouble());

		m_LastStateChangeUptime = uptime;
		m_eState = eState;

#if SEOUL_UNIT_TESTS
		// Unit testing functionality - if s_UnitTesting_SimulateRestartOnComplete
		// is anything other than Game::PatcherState::COUNT, we force
		// the state to kRestarting after completing the specified state. We
		// then clear the variable to allow the patcher to continue.
		if (PatcherState::COUNT != s_eUnitTesting_SimulateRestartOnComplete &&
			eState == s_eUnitTesting_SimulateRestartOnComplete /* previous state is target. */)
		{
			// Clear.
			s_eUnitTesting_SimulateRestartOnComplete = PatcherState::COUNT;

			// Force to restart state.
			m_eState = PatcherState::kRestarting;
		}
#endif // /#if SEOUL_UNIT_TESTS
	}
}

HTTP::CallbackResult Patcher::OnPrecacheUrl(HTTP::Result eResult, HTTP::Response* pResponse)
{
	if (eResult != HTTP::Result::kSuccess || pResponse->GetStatus() >= 500)
	{
		return HTTP::CallbackResult::kNeedsResend;
	}

	auto pPatcher = Patcher::Get();
	if (pPatcher)
	{
		++pPatcher->m_CachedUrls;
	}

	return HTTP::CallbackResult::kSuccess;
}

/** Utility, used as part of the kRestarting state. */
template <typename T>
static inline Bool TryReset(SharedPtr<T>& rp)
{
	if (rp.IsValid())
	{
		if (rp->IsJobRunning())
		{
			return false;
		}

		rp.Reset();
	}

	return true;
}
static inline Bool TryResetVmCreateJob(SharedPtr<ScriptManagerVmCreateJob>& rp)
{
	if (rp.IsValid())
	{
		if (rp->IsJobRunning())
		{
			return false;
		}

		// Need to make sure the VM itself is released now or it can linger
		// passed when it is expected/supposed to (it will otherwise be
		// released with the Jobs::Manager release the Job, which can be
		// a few ticks in the future). Call with no receive since we're just
		// dropping the reference entirely.
		rp->TakeOwnershipOfVm();
		rp.Reset();
	}

	return true;
}

void Patcher::OnTick(RenderPass& rPass, Float fDeltaTimeInSeconds)
{
#if SEOUL_ENABLE_CHEATS
	if (StayOnLoadingScreen)
	{
		return;
	}
#endif

	auto const eStartingState = m_eState;
	auto const statAction(MakeScopedAction([](){},
	[&]()
	{
		// Accumulate display stats.
		{
			// Track auth login time.
			AuthData authData;
			if (AuthManager::Get()->GetAuthData(authData))
			{
				m_Stats.m_AuthLoginRequest = authData.m_RequestStats;
			}

			// Track downloader stats.
			if (g_pDownloadableContentPackageFileSystem)
			{
				g_pDownloadableContentPackageFileSystem->GetStats(m_Stats.m_AdditionalStats);
			}
			{
				auto pConfig(Main::Get()->GetConfigUpdatePackageFileSystem());
				if (pConfig) { pConfig->GetStats(m_Stats.m_ConfigStats); }
			}
			{
				auto pContent(Main::Get()->GetContentUpdatePackageFileSystem());
				if (pContent) { pContent->GetStats(m_Stats.m_ContentStats); }
			}

			// Status tied to state.
			auto& state = m_Stats.m_aPerState[(UInt32)eStartingState];
			if (eStartingState != m_eState)
			{
				// Increment previous state once we advance.
				++state.m_uCount;
			}

			// Accumulate time.
			auto const fUnfixedDeltaTimeInSeconds = Engine::Get()->GetUnfixedSecondsInTick();
			state.m_fTimeSecs += fUnfixedDeltaTimeInSeconds;
			m_fElapsedDisplayTimeInSeconds += fUnfixedDeltaTimeInSeconds;
		}
	}));

	UI::Movie::OnTick(rPass, fDeltaTimeInSeconds);

	// Don't process any patcher state while the patcher progress is still loading.
	if (m_bPatcherStatusLoading)
	{
		return;
	}

	// Handle patching operations.
	//
	// If there is a pending auth conflict to resolve, it must be resolved
	// before the patcher can proceed.
	if (AuthManager::Get()->HasAuthConflict())
	{
		// If not in kInitial or kRestarting, go to kRestarting.
		if (PatcherState::kInitial != m_eState &&
			PatcherState::kRestarting != m_eState)
		{
			SetState(PatcherState::kRestarting);
		}

		// If an auth conflict is present, wait forever
		// in the initial state until it is resolved.
		if (PatcherState::kInitial == m_eState)
		{
			return;
		}
	}
	// If we have no auth data, we are only allowed in kGDPRCheck, kInitial
	// or kWaitForPatchApplyConditions
	else if (!AuthManager::Get()->HasAuthData())
	{
		// If not kGDPRCheck, kInitial, kWaitForAuth, kWaitForPatchApplyConditions, or
		// kRestarting, force to kRestarting.
		if (PatcherState::kGDPRCheck != m_eState &&
			PatcherState::kInitial != m_eState &&
			PatcherState::kWaitForAuth != m_eState &&
			PatcherState::kWaitForPatchApplyConditions != m_eState &&
			PatcherState::kRestarting != m_eState)
		{
			SetState(PatcherState::kRestarting);
		}
	}

	// Cache package pointers.
	CheckedPtr<PatchablePackageFileSystem> pConfigUpdatePackageFileSystem = Main::Get()->GetConfigUpdatePackageFileSystem();
	CheckedPtr<PatchablePackageFileSystem> pContentUpdatePackageFileSystem = Main::Get()->GetContentUpdatePackageFileSystem();

	switch (m_eState)
	{
	case PatcherState::kGDPRCheck:
		{
			// Check if they have accepted, if they have, go to kInitial, otherwise keep waiting
			if (Engine::Get()->GetGDPRAccepted())
			{
				SetState(PatcherState::kInitial);
			}
		}
		break;
	case PatcherState::kInitial:
		{
			// Check if all state machines are in their default state. If not, warn about it,
			// but then force the issue with a GotoState().
			Bool bReady = true;
			auto const& vStack = UI::Manager::Get()->GetStack();
			for (auto i = vStack.Begin(); vStack.End() != i; ++i)
			{
				auto pMachine(i->m_pMachine);
				HString const activeIdentifier = pMachine->GetActiveStateIdentifier();
				HString const defaultIdentifier = pMachine->GetDefaultStateIdentifier();

				if (activeIdentifier != defaultIdentifier)
				{
					// Check if the active state has the "PatcherFriendly" special exemption.
					if (pMachine->GetActiveState().IsValid())
					{
						DataStore const* pConfig = nullptr;
						DataNode config;
						if (pMachine->GetActiveState()->GetConfiguration(pConfig, config))
						{
							DataNode inner;
							Bool bPatcherFriendly = false;
							if (pConfig->GetValueFromTable(config, kPatcherFriendly, inner) &&
								pConfig->AsBoolean(inner, bPatcherFriendly) &&
								bPatcherFriendly)
							{
								// Special exemption from patcher forcing.
								continue;
							}
						}
					}

					SEOUL_WARN("State machine \"%s\" is not in its default state \"%s\" on the patcher screen. "
						"The patcher will now force the state machine to its default state. This might introduce bugs. "
						"Please add the global GameLoaded negative transition to the state machine definition so it "
						"returns to its default state during patching.",
						pMachine->GetName().CStr(),
						defaultIdentifier.CStr());

					UI::Manager::Get()->GotoState(pMachine->GetName(), defaultIdentifier);
					bReady = false;
				}
			}

			// Wait for state machines to enter their default states.
			if (!bReady)
			{
				return;
			}

			// Tell Game::Main to enter the pre-game tier.
			Main::Get()->PatcherFriend_ShutdownGame();

			// Do this early so that a patcher restart can be triggered.
			UI::Manager::Get()->SetCondition(kPendingSoftReboot, false);

			// Enter the kWaitForAuth state.
			SetState(PatcherState::kWaitForAuth);
		}
		break;

	case PatcherState::kWaitForAuth:
		{
			// If we don't have auth data yet, or auth data download is pending, wait.
			if (!AuthManager::Get()->HasAuthData() || AuthManager::Get()->IsRequestPending())
			{
				return;
			}

			// Enter the kWaitForRequiredVersion state.
			SetState(PatcherState::kWaitForRequiredVersion);
		}
		break;

	case PatcherState::kWaitForRequiredVersion:
		{
			// Don't allow us to continue if a required version update is pending.
			{
				AuthData data;
				if (!AuthManager::Get()->GetAuthData(data) ||
					!data.m_RefreshData.m_VersionRequired.CheckCurrentBuild())
				{
					return;
				}
			}

			// Enter the kWaitForPatchApplyConditions state.
			SetState(PatcherState::kWaitForPatchApplyConditions);
		}
		break;

	case PatcherState::kWaitForPatchApplyConditions:
		{
			// Don't continue with patching if the downloadable package file system is still initializing.
			if (g_pDownloadableContentPackageFileSystem &&
				!g_pDownloadableContentPackageFileSystem->IsInitialized())
			{
				// Make sure we're reporting insufficient disk space errors for the content download.
				if (g_pDownloadableContentPackageFileSystem->HasExperiencedWriteFailure())
				{
					// Report the write failure to analytics if we have not yet done so.
					if (!m_bSentDiskWriteFailureAnalytics)
					{
						m_bSentDiskWriteFailureAnalytics = true;
						Analytics::OnDiskWriteError();
					}

					SetState(PatcherState::kInsufficientDiskSpace);
				}

				return;
			}

			// If the settings cache is still in flight, don't advance to the kPatchApply
			// state. This should only occur if a patch is started rapidly after another
			// or is restarted mid patch.
			if (SettingsManager::Get()->AreSettingsLoading())
			{
				return;
			}

			// If "sensitive" content is loading, don't advnace to kPatchApply. This applies
			// to certain content types that must be loaded together at the same version
			// (switching files in the middle of this process would generate an error).
			if (Content::LoadManager::Get()->IsSensitiveContentLoading())
			{
				return;
			}

			// For debugging, log the auth body we got back from the server.
#if SEOUL_LOGGING_ENABLED
			{
				AuthData data;
				if (AuthManager::Get()->GetAuthData(data))
				{
					String sOut;
					(void)Reflection::SerializeToString(
						&data,
						sOut,
						true,
						0,
						true);
					SEOUL_LOG_ENGINE("GamePatcher (AuthData): %s", sOut.CStr());
				}
			}
#endif // /#if SEOUL_LOGGING_ENABLED

			// Create the patch apply job and start it.
			m_pApplyJob.Reset(SEOUL_NEW(MemoryBudgets::Game) PatcherApplyJob);
			m_pApplyJob->StartJob();

			// If we get here, the DownloadablePackageFileSystem is good to go, so continue to the patch apply
			// state.
			SetState(PatcherState::kPatchApply);
		}
		break;

	case PatcherState::kInsufficientDiskSpace:
		{
			// Don't continue with patching if the downloadable package file system is still initializing.
			if (g_pDownloadableContentPackageFileSystem &&
				!g_pDownloadableContentPackageFileSystem->IsInitialized() &&
				g_pDownloadableContentPackageFileSystem->HasExperiencedWriteFailure())
			{
				return;
			}

			// Otherwise, restart.
			SetState(PatcherState::kRestarting);
		}
		break;

	case PatcherState::kInsufficientDiskSpacePatchApply:
		{
			// Wait until the apply job has completed.
			if (m_pApplyJob.IsValid() &&
				m_pApplyJob->IsJobRunning())
			{
				// Job is still experiencing a write failure.
				if (m_pApplyJob->IsExperiencingWriteFailure())
				{
					return;
				}
			}

			// Otherwise, return to kPatchApply.
			SetState(PatcherState::kPatchApply);
		}
		break;

	case PatcherState::kPatchApply:
		{
			// Wait until the apply job has completed.
			if (m_pApplyJob.IsValid() &&
				m_pApplyJob->IsJobRunning())
			{
				// Switch to the insufficient disk space state on write failure.
				if (m_pApplyJob->IsExperiencingWriteFailure())
				{
					SetState(PatcherState::kInsufficientDiskSpacePatchApply);
				}

				// Update progress.
				if (m_pApplyJob->GetTotalSize() > 0u)
				{
					Float const fValue = (Float)((Double)m_pApplyJob->GetTotalProgress() / (Double)m_pApplyJob->GetTotalSize());
					m_fApplyProgress = Clamp(Max(m_fApplyProgress, fValue), 0.0f, 1.0f);
				}

				return;
			}

			// Done done.
			m_fApplyProgress = 1.0f;

			// Cache result values.
			Bool const bConfigSuccess = (m_pApplyJob.IsValid() ? m_pApplyJob->GetConfigSuccess() : false);
			Bool const bContentSuccess = (m_pApplyJob.IsValid() ? m_pApplyJob->GetContentSuccess() : false);
			Bool const bRemapSuccess = (m_pApplyJob.IsValid() ? m_pApplyJob->GetRemapSuccess() : false);

			// Acquire stats.
			m_pApplyJob->AcquireStats(m_Stats.m_tApplySubStats);

			// Reset the apply job.
			m_pApplyJob.Reset();

			// TODO: We may want to do something different here. For now, on failure, just clear
			// all remaps and yell loudly in developer builds.
			if (!bRemapSuccess)
			{
				{
					FileManagerRemap::RemapTable tEmpty;
					FileManager::Get()->ConfigureRemap(tEmpty, 0u);
				}

				// Yell.
				SEOUL_WARN(
					"FileManager remapping (likely, A/B testing), has been reset due to a failure to load config files. "
					"More data is probably available in the log.");
			}

			// Trigger a convent reload and then wait for it to complete.
			if (bConfigSuccess && bContentSuccess)
			{
				// Kick the UI::Manager texture cache.
				UI::Manager::Get()->GetRenderer().PurgeTextureCache();

				// Switch to the kWaitingForContentReload state.
				SetState(PatcherState::kWaitingForTextureCachePurge);
			}
			// Error with either update, revert both and start over.
			else
			{
				// Revert the config package if defined.
				if (pConfigUpdatePackageFileSystem)
				{
					pConfigUpdatePackageFileSystem->SetURL(String());
				}

				// Revert the content update package if defined.
				if (pContentUpdatePackageFileSystem)
				{
					pContentUpdatePackageFileSystem->SetURL(String());
				}

				// Issue an unload and then reload of all content to make sure loaded
				// state is in sync with patch state.
				m_ContentPending.Clear();
				Content::LoadManager::Get()->UnloadAll();
				Content::LoadManager::Get()->Reload(m_ContentPending);
				m_Stats.m_uReloadedFiles += m_ContentPending.m_vReloaded.GetSize();

				// Switch to the kWaitingForContentReloadAfterError state.
				SetState(PatcherState::kWaitingForContentReloadAfterError);
			}
		}
		return;

		// Wait for a texture purge to complete and when complete,
		// reload content.
	case PatcherState::kWaitingForTextureCachePurge:
		{
			// Stay in state while purge is still pending.
			if (UI::Manager::Get()->GetRenderer().IsTexturePurgePending())
			{
				return;
			}

			// Stay in state while loads are still active.
			if (Content::LoadManager::Get()->HasActiveLoads())
			{
				return;
			}

			// Issue an unload and then reload of all content.
			Content::LoadManager::Get()->UnloadAll();

			// Reinitialize the loc system prior to content reload, to ensure prefetched
			// movies get updated loc data.
			LocManager::Get()->ReInit();

			// Reload content.
			Content::LoadManager::Get()->Reload(m_ContentPending);
			m_Stats.m_uReloadedFiles += m_ContentPending.m_vReloaded.GetSize();

			// Switch to the kWaitingForContentReload state.
			SetState(PatcherState::kWaitingForContentReload);
		}
		return;

		// Wait for reloaded content to finish loading.
	case PatcherState::kWaitingForContentReload:
		{
			// If any pending files are still loading, don't advance to the next state.
			UInt32 uToReload = 0u;
			UInt32 uReloaded = 0u;
			m_ContentPending.GetProgress(uToReload, uReloaded);
			if (uReloaded < uToReload)
			{
				Float const fValue = (Float)uReloaded / (Float)uToReload;
				m_fLoadProgress = Clamp(Max(m_fLoadProgress, fValue), 0.0f, 1.0f);
				return;
			}
			m_fLoadProgress = 1.0f;

			// When reload has completed, leave m_ContentPending
			// populated so it can be used in kGameInitialized
			// to signal the UI::Manager to refresh its configuration.

			// Otherwise, start the app's ConfigManager loading and switch
			// to the kWaitingForGameConfigManager state.
			m_pGameConfigManagerLoadJob.Reset(SEOUL_NEW(MemoryBudgets::Config) ConfigManagerLoadJob(Main::Get()->GetSettings().m_ConfigManagerType));
			m_pGameConfigManagerLoadJob->StartJob();
			SetState(PatcherState::kWaitingForGameConfigManager);
		}
		return;

		// Wait for the Game::ConfigManager to load.
	case PatcherState::kWaitingForGameConfigManager:
		{
			// If the Game::ConfigManager is still loading, wait.
			if (m_pGameConfigManagerLoadJob->IsJobRunning())
			{
				return;
			}

			// Check job status - on error, restart the patcher.
			if (m_pGameConfigManagerLoadJob->GetJobState() == Jobs::State::kError)
			{
				m_pGameConfigManagerLoadJob.Reset();

				// Restart.
				SetState(PatcherState::kRestarting);

				// Refresh auth data, in case there was a server misconfiguration and the URLs have changed.
				AuthManager::Get()->Refresh();
				return;
			}

			// Otherwise, create the config manager, then reset
			// the config data job.
			Main::Get()->PatcherFriend_AcquireConfigManager(m_pGameConfigManagerLoadJob->GetConfigManager());
			m_pGameConfigManagerLoadJob.Reset();

#if SEOUL_WITH_GAME_PERSISTENCE
			// Start loading the persistence manager.
			m_pGamePersistenceManagerLoadJob.Reset(SEOUL_NEW(MemoryBudgets::Persistence) PersistenceManagerLoadJob(Main::Get()->GetSettings().m_PersistenceManagerSettings));
			m_pGamePersistenceManagerLoadJob->StartJob();
			SetState(kWaitingForGamePersistenceManager);
#else
			SetState(PatcherState::kWaitingForPrecacheUrls);
#endif
		}
		return;

#if SEOUL_WITH_GAME_PERSISTENCE
		// Wait for the Game::PersistenceManager to load.
	case kWaitingForGamePersistenceManager:
		{
			// If the Game::PersistenceManager is still loading, wait.
			if (m_pGamePersistenceManagerLoadJob->IsJobRunning())
			{
				return;
			}

			// Check job status - on error, restart the patcher.
			if (m_pGamePersistenceManagerLoadJob->GetJobState() == Jobs::State::kError)
			{
				m_pGamePersistenceManagerLoadJob.Reset();

				// Restart.
				SetState(kRestarting);

				// Refresh auth data, in case there was a server misconfiguration and the URLs have changed.
				AuthManager::Get()->Refresh();
				return;
			}

			// Otherwise, create the persistence manager, then reset
			// the persistence manager job.
			Main::Get()->PatcherFriend_AcquirePersistenceManager(m_pGamePersistenceManagerLoadJob->GetPersistenceManager());
			m_pGamePersistenceManagerLoadJob.Reset();

			// Now kick off URL precaching.
			if (!Main::Get()->GetServerBaseURL().IsEmpty())
			{
				m_CachedUrls.Reset();
				for (auto i = m_vPrecacheUrls.Begin(); m_vPrecacheUrls.End() != i; ++i)
				{
					// Cache the URL, and wrap the callback for caching.
					auto const sURL(Main::Get()->GetServerBaseURL() + *i);
					auto callback = Client::Get()->WrapCallbackForCache(SEOUL_BIND_DELEGATE(&OnPrecacheUrl), sURL);

					// Issue the request.
					auto& r = Client::Get()->CreateRequest(
						sURL,
						callback,
						HTTP::Method::ksGet,
						true,
						false);
					r.Start();
				}
			}
			SetState(kWaitingForPrecacheUrls);
		}
		return;
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE

		// Waiting for URL precache to complete.
	case PatcherState::kWaitingForPrecacheUrls:
		{
			// Not done yet.
			if (!Main::Get()->GetServerBaseURL().IsEmpty() && m_vPrecacheUrls.GetSize() > (UInt32)m_CachedUrls)
			{
				return;
			}

			// Leftover from previous run, allow to complete.
			if (m_pVmCreateJob.IsValid())
			{
				if (m_pVmCreateJob->IsJobRunning())
				{
					return;
				}

				// Need to make sure the VM itself is released now or it can linger
				// passed when it is expected/supposed to (it will otherwise be
				// released with the Jobs::Manager release the Job, which can be
				// a few ticks in the future). Call with no receive since we're just
				// dropping the reference entirely.
				m_pVmCreateJob->TakeOwnershipOfVm();
				m_pVmCreateJob.Reset();
			}

			// Pre-initialize game.
			Main::Get()->PatcherFriend_PreInitializeScript();

			// Start loading the script manager's initial VM.
			m_pVmCreateJob.Reset(SEOUL_NEW(MemoryBudgets::Scripting) ScriptManagerVmCreateJob(GetScriptUISettings(Main::Get()->GetSettings()), false));
			m_pVmCreateJob->StartJob();
			SetState(PatcherState::kWaitingForGameScriptManager);
		}
		return;

		// Wait for the Vm to reload.
	case PatcherState::kWaitingForGameScriptManager:
		{
			// If the script VM is still loading, wait.
			if (m_pVmCreateJob->IsJobRunning())
			{
				// Update progress.
				Atomic32Type steps;
				Atomic32Type progress;
				m_pVmCreateJob->GetProgress(steps, progress);
				if (steps > 0)
				{
					Float const fValue = (Float)progress / (Float)steps;
					m_fScriptProgress = Clamp(Max(m_fScriptProgress, fValue), 0.0f, 1.0f);
				}

				return;
			}

			// Check job status - on error, restart the patcher.
			if (m_pVmCreateJob->GetJobState() == Jobs::State::kError)
			{
				// Need to make sure the VM itself is released now or it can linger
				// passed when it is expected/supposed to (it will otherwise be
				// released with the Jobs::Manager release the Job, which can be
				// a few ticks in the future). Call with no receive since we're just
				// dropping the reference entirely.
				m_pVmCreateJob->TakeOwnershipOfVm();
				m_pVmCreateJob.Reset();

				// Restart.
				SetState(PatcherState::kRestarting);

				// Refresh auth data, in case there was a server misconfiguration and the URLs have changed.
				AuthManager::Get()->Refresh();
				return;
			}

			// Done.
			m_fScriptProgress = 1.0f;

			// Otherwise, create the script manager, then reset
			// the script manager job.
			Main::Get()->PatcherFriend_AcquireScriptManagerVm(
				m_pVmCreateJob->GetSettings(),
				m_pVmCreateJob->TakeOwnershipOfVm());
			m_pVmCreateJob.Reset();

			// Otherwise, switch to the kGameInitialize state.
			SetState(PatcherState::kGameInitialize);
		}
		return;

		// Wait for reloaded content to finish loading, then kick off
		// a new *.sar download and return to the kInitial state.
	case PatcherState::kWaitingForContentReloadAfterError:
		{
			// If any pending files are still loading, don't advance to the next state.
			if (m_ContentPending.IsLoading())
			{
				return;
			}

			// Restart.
			SetState(PatcherState::kRestarting);

			// Refresh auth data, in case there was a server misconfiguration and the URLs have changed.
			AuthManager::Get()->Refresh();
		}
		return;

		// Perform the actual singleton/system reboot.
	case PatcherState::kGameInitialize:
		{
			// Don't allow us into the game if a required version update is pending.
			AuthData data;
			if (!AuthManager::Get()->GetAuthData(data) ||
				!data.m_RefreshData.m_VersionRequired.CheckCurrentBuild())
			{
				return;
			}

			// Give the script environment one last chance to hold
			// for completion.
			if (ScriptManager::Get() &&
				ScriptManager::Get()->GetVm().IsValid())
			{
				Script::FunctionInvoker invoker(*ScriptManager::Get()->GetVm(), kSeoulIsFullyInitialized);
				if (invoker.IsValid())
				{
					if (invoker.TryInvoke())
					{
						Bool bResult = true;
						if (invoker.GetBoolean(0, bResult) && !bResult)
						{
							return;
						}
					}
				}
			}

			// Tell the UI::Manager about any file changes.
			for (auto const& p : m_ContentPending.m_vReloaded)
			{
				UI::Manager::Get()->ApplyFileChange(p->GetFilePath());
			}

			// Finish.
			Main::Get()->PatcherFriend_PostInitializeScript();
			UI::Manager::Get()->SetCondition(kGameLoaded, true);

			// Done.
			SetState(PatcherState::kDone);
		}
		return;

		// Nop - just re-assert that the game should be loaded.
		// We need to leave the "Patcher" state via this
		// condition if we've settled into the kDone state.
	case PatcherState::kDone:
		UI::Manager::Get()->SetCondition(kGameLoaded, true);
		UI::Manager::Get()->SetCondition(kPendingSoftReboot, false);
		return;

		// Not typical - designed to be robust and cleanup
		// from any interior state, placing the patcher
		// back into its kInitial State.
	case PatcherState::kRestarting:
		if (!TryResetVmCreateJob(m_pVmCreateJob)) { return; }
#if SEOUL_WITH_GAME_PERSISTENCE
		if (!TryReset(m_pGamePersistenceManagerLoadJob)) { return; }
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE
		if (!TryReset(m_pGameConfigManagerLoadJob)) { return; }
		if (!TryReset(m_pApplyJob)) { return; }

		// Now cleanup any simple state and return to kInitial.
		m_fScriptProgress = 0.0f;
		m_fLoadProgress = 0.0f;
		m_fApplyProgress = 0.0f;
		m_ContentPending.Clear();
		m_CachedUrls.Reset();

		// Return to the initial state.
		SetState(PatcherState::kInitial);
		return;
	case PatcherState::COUNT: break;		// COUNT is a delimiter, not a valid part of the state machine
	};
}

} // namespace Game

} // namespace Seoul
