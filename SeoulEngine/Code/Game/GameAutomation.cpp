/**
 * \file GameAutomation.cpp
 * \brief Global Singletons that owns a Lua script VM and functionality
 * for automation and testing of a game application. Developer
 * functionality only.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BuildChangelistPublic.h"
#include "BuildVersion.h"
#include "CrashManager.h"
#include "DownloadablePackageFileSystem.h"
#include "Engine.h"
#include "FalconMovieClipInstance.h"
#include "GameAutomation.h"
#include "GameClient.h"
#include "EventsManager.h"
#include "GameMain.h"
#include "GamePatcherStatus.h"
#include "GameScriptManager.h"
#include "HTTPManager.h"
#include "JobsFunction.h"
#include "JobsJob.h"
#include "Logger.h"
#include "NTPClient.h"
#include "PatchablePackageFileSystem.h"
#include "Path.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ReflectionIntegrationTestRunner.h"
#include "RenderDevice.h"
#include "Renderer.h"
#include "SaveLoadManager.h"
#include "ScopedAction.h"
#include "ScriptFunctionInterface.h"
#include "ScriptFunctionInvoker.h"
#include "ScriptUIInstance.h"
#include "ScriptVm.h"
#include "SeoulProfiler.h"
#include "TextureManager.h"
#include "UIManager.h"
#include "UnitTesting.h"
#include "VmStats.h"

#if SEOUL_UNIT_TESTS && SEOUL_PLATFORM_ANDROID
#include "ReflectionUnitTestRunner.h"
#endif

namespace Seoul
{

/** Optional, download package file system that must be initialized once *_Config.sar initialization is complete. */
extern CheckedPtr<DownloadablePackageFileSystem> g_pDownloadableContentPackageFileSystem;

namespace Game
{

// TODO: Enable on Android only to diagnose low memory usage
// conditions.
#if SEOUL_AUTO_TESTS && SEOUL_PLATFORM_ANDROID && SEOUL_LOGGING_ENABLED
#define SEOUL_REPORT_PROCESS_MEM_USAGE 1
#else
#define SEOUL_REPORT_PROCESS_MEM_USAGE 0
#endif

#if SEOUL_REPORT_PROCESS_MEM_USAGE
static void ProcessMemUsageLog()
{
	size_t zWorkingSet = 0;
	size_t zPrivate = 0;
	if (Engine::Get()->QueryProcessMemoryUsage(zWorkingSet, zPrivate))
	{
		SEOUL_LOG_AUTOMATION("Process memory usage:");
		SEOUL_LOG_AUTOMATION("- Working Set: %zu", zWorkingSet);
		SEOUL_LOG_AUTOMATION("- Private: %zu", zPrivate);
	}
}
#endif // /SEOUL_REPORT_PROCESS_MEM_USAGE

/** Global script entry points in the automation script. */
static const HString kFrame("Frame");
static const HString kInitialize("Initialize");
static const HString kOnUIStateChange("OnUIStateChange");
static const HString kPreShutdown("PreShutdown");
static const HString kPreTick("PreTick");
static const HString kPostTick("PostTick");
static const HString kRenderSynchronize("Render.Synchronize");
static const HString kSetGlobalState("SetGlobalState");
static const HString kGameScreens("GameScreens");

/** How frequently we check NTP vs. server time estimate. */
static const TimeInterval kServerTimeCheckInterval = TimeInterval::FromSecondsDouble(300.0);

#if SEOUL_LOGGING_ENABLED
/**
 * Hook for print() output from Lua.
 */
void AutomationLuaLog(Byte const* sTextLine)
{
	SEOUL_LOG_SCRIPT("%s", sTextLine);
}
#endif // /#if SEOUL_LOGGING_ENABLED

#if SEOUL_ENABLE_MEMORY_TOOLING
#if SEOUL_LOGGING_ENABLED
/**
 * Utility, commit remaining text in rs, up to (and excluding)
 * a required newline terminator.
 */
static void LogMemoryDetailsFlush(String& rs)
{
	auto u = rs.Find('\n');
	while (String::NPos != u)
	{
		if (0 == u)
		{
			LogMessage(LoggerChannel::Core, "\n");
		}
		else
		{
			LogMessage(LoggerChannel::Core, "%.*s", u, rs.CStr());
		}
		rs.Assign(rs.CStr() + u + 1);
		u = rs.Find('\n');
	}
}

/**
 * Bind printf style printing into the logger for printing memory details.
 */
static void LogMemoryDetailsPrintfLike(void* userData, const Byte* sFormat, ...)
{
	String& rs = *((String*)userData);
	va_list args;
	va_start(args, sFormat);
	rs.Append(String::VPrintf(sFormat, args));
	va_end(args);

	LogMemoryDetailsFlush(rs);
}
#endif // /SEOUL_LOGGING_ENABLED
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

namespace
{

void TimeCheck()
{
	static const Double kfMaxDeltaInSeconds = 15.0;

	NTPClientSettings settings;
	settings.m_sHostname = "pool.ntp.org";
	NTPClient client(settings);

	WorldTime ntpTime;
	if (client.SyncQueryTime(ntpTime))
	{
		WorldTime serverTime;
		{
			ClientLifespanLock lock;
			if (!Client::Get())
			{
				return;
			}

			serverTime = Client::Get()->GetCurrentServerTime();
		}

		auto const delta = (serverTime - ntpTime);
		auto const fDeltaInSeconds = Abs(delta.GetSecondsAsDouble());
		if (fDeltaInSeconds > kfMaxDeltaInSeconds)
		{
			SEOUL_WARN("Server/NTP time delta is above acceptable threshold at: %.3f seconds", fDeltaInSeconds);
			SEOUL_WARN("Server time (UTC microseconds): %" PRIu64, serverTime.GetMicroseconds());
			SEOUL_WARN("NTP time (UTC microseconds): %" PRIu64, ntpTime.GetMicroseconds());
		}

		// Accumulate.
		Automation::AccumulateServerTimeDeltaInMilliseconds((Atomic32Type)Abs(
			delta.GetMicroseconds() / WorldTime::kMillisecondsToMicroseconds));
	}
}

} // namespace

class AutomationScriptObject SEOUL_SEALED
{
public:
	void BroadcastEvent(Script::FunctionInterface* pInterface) const
	{
		HString sEvent;
		if (!pInterface->GetString(1u, sEvent))
		{
			pInterface->RaiseError(1u, "invalid event name, must be convertible to string.");
			return;
		}

		Int const iArguments = pInterface->GetArgumentCount() - 2;
		Reflection::MethodArguments aArguments;
		if (iArguments < 0 ||
			(UInt32)iArguments > aArguments.GetSize())
		{
			pInterface->RaiseError(-1, "too many arguments to BroadcastEvent, got %d, max of %u",
				iArguments,
				aArguments.GetSize());
			return;
		}

		for (Int i = 0; i < iArguments; ++i)
		{
			if (!pInterface->GetAny((UInt32)(i + 2), TypeId<void>(), aArguments[i]))
			{
				pInterface->RaiseError(i + 2, "invalid argument, must be convertible to Seoul::Reflection::Any.");
				return;
			}
		}

		UI::Manager::Get()->BroadcastEvent(sEvent, aArguments, iArguments);
	}

	void BroadcastEventTo(Script::FunctionInterface* pInterface)
	{
		HString sTargetType;
		if (!pInterface->GetString(1u, sTargetType))
		{
			pInterface->RaiseError(1u, "invalid target type, must be convertible to string.");
			return;
		}

		HString sEvent;
		if (!pInterface->GetString(2u, sEvent))
		{
			pInterface->RaiseError(2u, "invalid event name, must be convertible to string.");
			return;
		}

		Int const iArguments = pInterface->GetArgumentCount() - 3;
		Reflection::MethodArguments aArguments;
		if (iArguments < 0 ||
			(UInt32)iArguments > aArguments.GetSize())
		{
			pInterface->RaiseError(-1, "too many arguments to BroadcastEvent, got %d, max of %u",
				iArguments,
				aArguments.GetSize());
			return;
		}

		for (Int i = 0; i < iArguments; ++i)
		{
			if (!pInterface->GetAny((UInt32)(i + 3), TypeId<void>(), aArguments[i]))
			{
				pInterface->RaiseError(i + 3, "invalid argument, must be convertible to Seoul::Reflection::Any.");
				return;
			}
		}

		Bool const bReturn = UI::Manager::Get()->BroadcastEventTo(sTargetType, sEvent, aArguments, iArguments);
		pInterface->PushReturnBoolean(bReturn);
	}

	void EnableServerTimeChecking(Bool bServerTimeChecking)
	{
		Automation::EnableServerTimeChecking(bServerTimeChecking);
	}

	Double GetUptimeInSeconds() const
	{
		return Engine::Get()->GetUptime().GetSecondsAsDouble();
	}

	Bool IsSaveLoadManagerFirstTimeTestingComplete() const
	{
#if SEOUL_UNIT_TESTS
		if (SaveLoadManager::Get())
		{
			return SaveLoadManager::Get()->IsFirstTimeTestingComplete();
		}
		else
#endif // /#if SEOUL_UNIT_TESTS
		{
			return true;
		}
	}

	HStringStats GetHStringStats() const
	{
		return HString::GetHStringStats();
	}

	void LogAllHStrings() const
	{
		HString::LogAllHStrings();
	}

	UI::Manager::HitPoints GetHitPoints(UInt8 uInputMask)
	{
		auto& r = Automation::Get()->m_vHitPoints;
		r.Clear();
		UI::Manager::Get()->GetHitPoints(uInputMask, r);
		return r;
	}

#if SEOUL_UNIT_TESTS
	void RunIntegrationTests(String sOptionalTestName)
	{
		UnitTesting::RunIntegrationTests(sOptionalTestName);
	}
#endif

	static void GatherName(Falcon::Instance const* p, String& rs)
	{
		if (nullptr == p)
		{
			return;
		}

		p->GatherFullName(rs);
	}

	String GetHitPointLongName(const UI::HitPoint& point) const
	{
		auto& r = Automation::Get()->m_vHitPoints;
		auto i = r.Find(point);
		if (r.End() == i || !i->m_pInstance.IsValid())
		{
			return String(point.m_Id);
		}

		String s;
		GatherName(i->m_pInstance.GetPtr(), s);
		return s;
	}

	HashTable<String, UInt32, MemoryBudgets::Developer> GetRequestedMemoryUsageBuckets()
	{
		HashTable<String, UInt32, MemoryBudgets::Developer> tReturn;

#if SEOUL_ENABLE_MEMORY_TOOLING
		for (Int i = MemoryBudgets::FirstType; i <= MemoryBudgets::LastType; ++i)
		{
			(void)tReturn.Overwrite(
				MemoryBudgets::ToString((MemoryBudgets::Type)i),
				MemoryManager::GetUsageInBytes((MemoryBudgets::Type)i));
		}
#endif // /#if SEOUL_ENABLE_MEMORY_TOOLING

		if (TextureManager::Get())
		{
			UInt32 uTextureMemoryUsage = 0u;
			(void)TextureManager::Get()->GetTextureMemoryUsageInBytes(uTextureMemoryUsage);

			(void)tReturn.Overwrite(
				"Textures",
				uTextureMemoryUsage);
		}

		return tReturn;
	}

#if SEOUL_ENABLE_MEMORY_TOOLING
	/** Min size to exclude, filter out some "noise". */
	static const ptrdiff_t kiMinScriptBucketSizeInBytes = 1024;

	static void GetScriptImpl(void* pUserData, Byte const* sName, ptrdiff_t iSizeInBytes, Int32 iLine)
	{
		if (iSizeInBytes <= kiMinScriptBucketSizeInBytes)
		{
			return;
		}

		auto& rt = *((HashTable<String, UInt32, MemoryBudgets::Developer>*)pUserData);
		rt.Insert(String::Printf("%s(%d)", sName, iLine), (UInt32)iSizeInBytes);
	}
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

	HashTable<String, UInt32, MemoryBudgets::Developer> GetScriptMemoryUsageBuckets()
	{
		HashTable<String, UInt32, MemoryBudgets::Developer> tReturn;

#if SEOUL_ENABLE_MEMORY_TOOLING
		if (ScriptManager::Get())
		{
			auto pVm(ScriptManager::Get()->GetVm());
			if (pVm.IsValid())
			{
				pVm->QueryMemoryProfilingData(SEOUL_BIND_DELEGATE(&GetScriptImpl, &tReturn));
			}
		}
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

		return tReturn;
	}

	Int64 GetTotalMemoryUsageInBytes() const
	{
#if SEOUL_ENABLE_MEMORY_TOOLING
		return MemoryManager::GetTotalUsageInBytes();
#else
		return 0;
#endif // /#if SEOUL_ENABLE_MEMORY_TOOLING
	}

	VmStats GetVmStats() const
	{
		return g_VmStats;
	}

	void LogGlobalUIScriptNodes(Bool bWarn) const
	{
#if !SEOUL_SHIP
		auto const sCode = (bWarn
			? R"__string__tag__(
				CoreUtilities.VisitTables(function (tbl, path)
					if tbl.m_udNativeInstance then
						CoreNative.Warn('- ' .. table.concat(path, '.'))
						return false
					end
					return true
				end)
				)__string__tag__"

			: R"__string__tag__(
				CoreUtilities.VisitTables(function (tbl, path)
					if tbl.m_udNativeInstance then
						CoreNative.Log('- ' .. table.concat(path, '.'))
						return false
					end
					return true
				end)
				)__string__tag__");

		if (ScriptManager::Get())
		{
			auto pVm(ScriptManager::Get()->GetVm());
			if (pVm.IsValid())
			{
				(void)pVm->RunCode(sCode);
			}
		}
#endif // /#if !SEOUL_SHIP
	}

	void LogInstanceCountsPerMovie() const
	{
#if !SEOUL_SHIP
		ScriptUIInstance::DebugLogInstanceCountsPerMovie();
#endif // /#if !SEOUL_SHIP
	}

	Int64 GetCurrentClientWorldTimeInMilliseconds() const
	{
		return WorldTime::GetUTCTime().GetMicroseconds() / WorldTime::kMillisecondsToMicroseconds;
	}

	Int64 GetCurrentServerWorldTimeInMilliseconds() const
	{
		return Client::Get()->GetCurrentServerTime().GetMicroseconds() / WorldTime::kMillisecondsToMicroseconds;
	}

	String GetCurrentISO8601DateTimeUTCString() const
	{
		return WorldTime::GetUTCTime().ToISO8601DateTimeUTCString();
	}

	Bool GetUICondition(HString name)
	{
		return UI::Manager::Get()->GetCondition(name);
	}

	UI::Manager::Conditions GetUIConditions() const
	{
		UI::Manager::Conditions t;
		UI::Manager::Get()->GetConditions(t);
		return t;
	}

	Vector<String> GetUIInputWhitelist() const
	{
#if !SEOUL_SHIP
		return UI::Manager::Get()->DebugGetInputWhitelistPaths();
#else
		return Vector<String>();
#endif
	}

	void GotoUIState(HString stateMachineName, HString stateName)
	{
		UI::Manager::Get()->GotoState(stateMachineName, stateName);
	}

	void Log(const String& s) const
	{
		SEOUL_LOG_AUTOMATION("%s", s.CStr());
	}

	void LogMemoryDetails(MemoryBudgets::Type eType)
	{
#if SEOUL_ENABLE_MEMORY_TOOLING
#if SEOUL_LOGGING_ENABLED
		String s;
		MemoryManager::PrintMemoryDetails(eType, LogMemoryDetailsPrintfLike, &s);
		// Any remaining, just send to the logger. It will automatically break
		// any remaining newlines, and then any trailing fragment will be emitted.
		if (!s.IsEmpty())
		{
			LogMessage(LoggerChannel::Core, "%s", s.CStr());
		}
#endif // /SEOUL_LOGGING_ENABLED
#endif // /#if SEOUL_ENABLE_MEMORY_TOOLING
	}

	void ManuallyInjectBindingEvent(HString bindingName) const
	{
		InputManager::Get()->ManuallyInjectBindingEvent(bindingName);
	}

	void QueueLeftMouseButtonEvent(Bool bPressed) const
	{
		InputManager::Get()->QueueMouseButtonEvent(InputButton::MouseLeftButton, bPressed);
	}

	void QueueMouseMoveEvent(Int32 iX, Int32 iY) const
	{
		InputManager::Get()->QueueMouseMoveEvent(Point2DInt(iX, iY));
	}

	void SendUITrigger(HString triggerName) const
	{
		UI::Manager::Get()->TriggerTransition(triggerName);
	}

	void SetEnablePerfTesting(Bool bEnable) const
	{
		Automation::Get()->SetEnablePerfTesting(bEnable);
	}

	void SetUICondition(HString name, Bool bValue) const
	{
		UI::Manager::Get()->SetCondition(name, bValue);
	}

	void Warn(const String& s) const
	{
#if SEOUL_LOGGING_ENABLED
		SEOUL_WARN("%s", s.CStr());
#else
		PlatformPrint::PrintStringMultiline(PlatformPrint::Type::kWarning, "Warning: ", s);
		Automation::Get()->IncrementAdditionalWarningCount();
#endif // /#if SEOUL_LOGGING_ENABLED
	}

	AutomationScriptObject()
	{
	}

	~AutomationScriptObject()
	{
	}

private:
	SEOUL_DISABLE_COPY(AutomationScriptObject);
}; // class Game::AutomationScriptObject

} // namespace Game

SEOUL_BEGIN_TYPE(Game::AutomationScriptObject, TypeFlags::kDisableCopy)
	SEOUL_METHOD(BroadcastEvent)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "string sEvent, params object[] aArgs")
	SEOUL_METHOD(BroadcastEventTo)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "string sEvent, string sTarget, params object[] aArgs")
	SEOUL_METHOD(EnableServerTimeChecking)
	SEOUL_METHOD(GetUptimeInSeconds)
	SEOUL_METHOD(IsSaveLoadManagerFirstTimeTestingComplete)
	SEOUL_METHOD(LogAllHStrings)
	SEOUL_METHOD(GetHStringStats)
	SEOUL_METHOD(GetHitPoints)
	SEOUL_METHOD(GetHitPointLongName)
	SEOUL_METHOD(GetRequestedMemoryUsageBuckets)
	SEOUL_METHOD(GetScriptMemoryUsageBuckets)
	SEOUL_METHOD(GetTotalMemoryUsageInBytes)
	SEOUL_METHOD(GetVmStats)
	SEOUL_METHOD(GetCurrentClientWorldTimeInMilliseconds)
	SEOUL_METHOD(GetCurrentServerWorldTimeInMilliseconds)
	SEOUL_METHOD(GetCurrentISO8601DateTimeUTCString)
	SEOUL_METHOD(GetUICondition)
	SEOUL_METHOD(GetUIConditions)
	SEOUL_METHOD(GetUIInputWhitelist)
	SEOUL_METHOD(GotoUIState)
	SEOUL_METHOD(Log)
	SEOUL_METHOD(LogGlobalUIScriptNodes)
	SEOUL_METHOD(LogInstanceCountsPerMovie)
	SEOUL_METHOD(LogMemoryDetails)
	SEOUL_METHOD(ManuallyInjectBindingEvent)
	SEOUL_METHOD(QueueLeftMouseButtonEvent)
	SEOUL_METHOD(QueueMouseMoveEvent)
#if SEOUL_UNIT_TESTS
	SEOUL_METHOD(RunIntegrationTests)
#endif
	SEOUL_METHOD(SendUITrigger)
	SEOUL_METHOD(SetEnablePerfTesting)
	SEOUL_METHOD(SetUICondition)
	SEOUL_METHOD(Warn)
SEOUL_END_TYPE()

namespace Game
{

TimeInterval Automation::s_LastServerTimeCheckUptime{};
Atomic32 Automation::s_MaxServerTimeDeltaInMilliseconds{};

/**
 * Ensure that we're at least running the memory manager unit test on Android
 * at all times, since the implementation is delicate on Android (see MemoryManager.cpp
 * for more information).
 */
static void InternalPerformBasicMemoryManagerCheck()
{
	// TODO: Not a great spot for this but currently convenient.
	// Make sure we stress the memory manager a bit in automated tests.
#if SEOUL_UNIT_TESTS && SEOUL_PLATFORM_ANDROID
	if (g_bRunningAutomatedTests)
	{
		SEOUL_ASSERT(UnitTesting::RunUnitTests("MemoryManagerTest.TestGetAllocationSizeAndAlignment"));
	}
#endif // /SEOUL_UNIT_TESTS
}

Automation::Automation(const AutomationSettings& settings)
	: m_AdditionalWarningCount(0)
	, m_pVm()
	, m_Settings(settings)
	, m_bEnablePerfTesting(false)
	, m_bIsEnabled(true)
	, m_uLongFrames(0u)
	, m_uTotalFrames(0u)
	, m_FirstHeartbeatUptime(Engine::Get()->GetUptime())
	, m_LastHeartbeatUptime(m_FirstHeartbeatUptime)
{
	SEOUL_ASSERT(IsMainThread());

	// Enable teardown tracing to help diagnose
	// shutdown hangs.
	SEOUL_TEARDOWN_TRACE_ENABLE(true);

	// TODO: Not a great spot for this but currently convenient.
	InternalPerformBasicMemoryManagerCheck();

	// Force initialize.
	s_LastServerTimeCheckUptime = TimeInterval{};

	// Register the UI state change handler
	Events::Manager::Get()->RegisterCallback(
		UI::StateChangeEventId,
		SEOUL_BIND_DELEGATE(&Automation::OnUIStateChange, this));

	// Apply settings immediately.
	InternalApplyAutomatedTestingMode();

	// Load the automation VM. Synchronously so we can
	// ensure we're loaded early in the startup flow.
	InternalLoadVm();
}

Automation::~Automation()
{
	SEOUL_ASSERT(IsMainThread());

	// Cleanup the VM.
	m_pVm.Reset(); SEOUL_TEARDOWN_TRACE();

	// Unregister the UI state change handler
	Events::Manager::Get()->UnregisterCallback(
		UI::StateChangeEventId,
		SEOUL_BIND_DELEGATE(&Automation::OnUIStateChange, this));

	// Record maximum delta (not a warning, for information purposes only).
	if (IsServerTimeCheckingEnabled() && s_MaxServerTimeDeltaInMilliseconds > 0)
	{
		SEOUL_LOG_AUTOMATION("Max server/ntp time delta: %.3f seconds",
			(Double)s_MaxServerTimeDeltaInMilliseconds / (Double)WorldTime::kSecondsToMilliseconds);
	}

	// Final heartbeat.
	SEOUL_LOG_AUTOMATION("Shutdown heartbeat, ran for %.2f minute(s)",
		(Engine::Get()->GetUptime() - m_FirstHeartbeatUptime).GetSecondsAsDouble() / 60.0);

	if (m_uLongFrames > 0u)
	{
		PlatformPrint::PrintStringFormatted(PlatformPrint::Type::kWarning, "Warning: LONG FRAMES: %" PRIu64 " (%.2f%%)", m_uLongFrames, ((Double)m_uLongFrames / (Double)m_uTotalFrames) * 100.0);
	}

	SEOUL_TEARDOWN_TRACE();
}

// Call during game shutdown, after clearing the UI
// but before destroying the game scripting environment.
Bool Automation::PreShutdown()
{
	SEOUL_ASSERT(IsMainThread());

	// Cleanup hit points prior to shutdown.
	m_vHitPoints.Clear();

	// If main script execution failed, return false immediately.
	if (!m_pVm.IsValid())
	{
		return false;
	}

	// Call PreShutdown in the script VM.
	Bool bReturn = true;
	{
		Script::FunctionInvoker invoker(*m_pVm, kPreShutdown);
		if (invoker.IsValid())
		{
			if (!invoker.TryInvoke())
			{
				SEOUL_WARN("Game automation failure, PreShutdown execution failure.");
				bReturn = false;
			}
			else
			{
				(void)invoker.GetBoolean(0, bReturn);
			}
		}
	}

	// Return the returned invoker value.
	return bReturn;
}

Bool Automation::PreTick()
{
	using namespace Reflection;

	SEOUL_ASSERT(IsMainThread());

	// Potentially apply automated testing settings.
	InternalApplyAutomatedTestingMode();

	if (InputManager::Get()->WasBindingPressed("AutomationDisableToggle"))
	{
		m_bIsEnabled = !m_bIsEnabled;
	}

	// Don't run the automation if it's disabled
	if (!m_bIsEnabled)
	{
		return true;
	}

	// If main script execution failed, return false immediately.
	if (!m_pVm.IsValid())
	{
		return false;
	}

	// Incremental garbage collection.
	{
		m_pVm->StepGarbageCollector();
	}

	// Call PreTick in the script VM.
	Bool bReturn = true;
	{
		Script::FunctionInvoker invoker(*m_pVm, kPreTick);
		if (invoker.IsValid())
		{
			if (!invoker.TryInvoke())
			{
				SEOUL_WARN("Game automation failure, PreTick execution failure.");
				bReturn = false;
			}
			else
			{
				(void)invoker.GetBoolean(0, bReturn);
			}
		}
	}

	// Return the returned invoker value.
	return bReturn;
}

Bool Automation::PostTick()
{
	using namespace Reflection;

	SEOUL_ASSERT(IsMainThread());

	// Cleanup cached nodes on post tick exit.
	auto const scoped(MakeScopedAction([]() {}, [&]() { m_vHitPoints.Clear(); }));

	// Don't run the automation if it's disabled
	if (!m_bIsEnabled)
	{
		return true;
	}

	// Potentially apply automated testing settings.
	InternalApplyAutomatedTestingMode();

	// Perform performance testing now, if enabled.
	InternalApplyPerformanceTesting();

	// Perform version checking now, if enabled.
	InternalRunClChecks();

	// Perform save load manager checking, if enabled.
	InternalRunSaveLoadManagerChecks();

	// If main script execution failed, return false immediately.
	if (!m_pVm.IsValid())
	{
		return false;
	}

	// Call PostTick in the script VM.
	Bool bReturn = true;
	{
		Script::FunctionInvoker invoker(*m_pVm, kPostTick);
		if (invoker.IsValid())
		{
			if (!invoker.TryInvoke())
			{
				SEOUL_WARN("Game automation failure, PostTick execution failure.");
				bReturn = false;
			}
			else
			{
				(void)invoker.GetBoolean(0, bReturn);
			}
		}
	}

	// If running ok, and enabled, kick off a server time check.
	if (IsServerTimeCheckingEnabled())
	{
		auto const current = Engine::Get()->GetUptime();
		if (current - s_LastServerTimeCheckUptime >= kServerTimeCheckInterval)
		{
			s_LastServerTimeCheckUptime = current;

			// If we don't expect a server time yet, skip the check.
			if (m_bExpectServerTime)
			{
				// If we expect a server time but don't have one yet, warn
				// about this specifically.
				if (!Client::Get()->HasCurrentServerTime())
				{
					SEOUL_WARN("Server time is expected but has not yet been initialized.");
				}
				// Else, run the time check.
				else
				{
					Jobs::AsyncFunction(&TimeCheck);
				}
			}
		}
	}

	// Log a heartbeat once every 15 seconds
	{
		auto const uptime = Engine::Get()->GetUptime();
		auto const delta = (uptime - m_LastHeartbeatUptime);
		if (delta >= TimeInterval::FromSecondsInt64(15))
		{
			SEOUL_LOG_AUTOMATION("Heartbeat, running for %.2f minute(s)",
				(uptime - m_FirstHeartbeatUptime).GetSecondsAsDouble() / 60.0);
			m_LastHeartbeatUptime = uptime;

#if SEOUL_REPORT_PROCESS_MEM_USAGE
			ProcessMemUsageLog();
#endif // /SEOUL_REPORT_PROCESS_MEM_USAGE
		}
	}

	// Return the returned invoker value.
	return bReturn;
}

/** Accumulate a server/ntp delta time sample. */
void Automation::AccumulateServerTimeDeltaInMilliseconds(Atomic32Type milliseconds)
{
	while (true)
	{
		auto current = (Atomic32Type)s_MaxServerTimeDeltaInMilliseconds;
		auto max = Max(current, milliseconds);
		if (max == current)
		{
			break;
		}

		if (current == s_MaxServerTimeDeltaInMilliseconds.CompareAndSet(max, current))
		{
			break;
		}
	}
}

/**
 * Debugging feature, enable periodic server time checks
 * using an NTP client.
 */
void Automation::EnableServerTimeChecking(Bool bEnable)
{
	if (IsServerTimeCheckingEnabled() != bEnable)
	{
		s_LastServerTimeCheckUptime = (bEnable ? Engine::Get()->GetUptime() : TimeInterval{});
	}
}

/** Part of OnPatcherClose reporting. */
namespace
{

struct StringSorter SEOUL_SEALED
{
	template <typename V>
	Bool operator()(const Pair<HString, V>& a, const Pair<HString, V>& b) const
	{
		return (strcmp(a.First.CStr(), b.First.CStr()) < 0);
	}
}; // struct StringSorter

} // namespace anonymous

template <typename V>
static Vector< Pair<HString, V> > Unfold(const HashTable<HString, V, MemoryBudgets::Io>& t)
{
	Vector< Pair<HString, V> > v;
	for (auto const& pair : t)
	{
		v.PushBack(MakePair(pair.First, pair.Second));
	}

	StringSorter sorter;
	QuickSort(v.Begin(), v.End(), sorter);

	return v;
}

static void PrintSubStats(Byte const* sLabel, const PatcherDisplayStats::ApplySubStats& t)
{
	for (auto const& pair : t)
	{
		PlatformPrint::PrintStringFormatted(PlatformPrint::Type::kInfo, "%s_%s: (%u, %f s", sLabel, pair.First.CStr(), pair.Second.m_uCount, pair.Second.m_fTimeSecs);
	}
}

static void PrintDownloaderData(Byte const* sLabel, const DownloadablePackageFileSystemStats& stats)
{
	PlatformPrint::PrintStringFormatted(PlatformPrint::Type::kInfo, "Sar '%s' Events", sLabel);
	auto const vEvents(Unfold(stats.m_tEvents));
	for (auto const& pair : vEvents)
	{
		PlatformPrint::PrintStringFormatted(PlatformPrint::Type::kInfo, "%s: %u", pair.First.CStr(), pair.Second);
	}

	PlatformPrint::PrintStringFormatted(PlatformPrint::Type::kInfo, "Sar '%s' Times", sLabel);
	auto const vTimes(Unfold(stats.m_tTimes));
	for (auto const& pair : vTimes)
	{
		PlatformPrint::PrintStringFormatted(PlatformPrint::Type::kInfo, "%s: %f s",
			pair.First.CStr(),
			SeoulTime::ConvertTicksToSeconds(pair.Second));
	}
}

static void PrintRequestData(Byte const* sLabel, const HTTP::Stats& stats)
{
	PlatformPrint::PrintStringFormatted(
		PlatformPrint::Type::kInfo,
		"%s Request: (%u resends, delay: %2.f ms, lookup: %.2f ms, connect: %2.f ms, appconnect: %2.f ms, pretransfer: %2.f ms, redirect: %.2f ms, starttransfer: %2.f ms, totalrequest: %.2f ms, overall: %.2f ms, %.2f B/s down, %2.f B/s up, %u http fails, %u net fails, request id \"%s\")",
		sLabel,
		stats.m_uResends,
		(stats.m_fApiDelaySecs * 1000.0),
		(stats.m_fLookupSecs * 1000.0),
		(stats.m_fConnectSecs * 1000.0),
		(stats.m_fAppConnectSecs * 1000.0),
		(stats.m_fPreTransferSecs * 1000.0),
		(stats.m_fRedirectSecs * 1000.0),
		(stats.m_fStartTransferSecs * 1000.0),
		(stats.m_fTotalRequestSecs * 1000.0),
		(stats.m_fOverallSecs * 1000.0),
		stats.m_fAverageDownloadSpeedBytesPerSec,
		stats.m_fAverageUploadSpeedBytesPerSec,
		stats.m_uHttpFailures,
		stats.m_uNetworkFailures,
		stats.m_sRequestTraceId.CStr());
}

/** Hook for reporting patcher times. */
void Automation::OnPatcherClose(
	Float32 fPatcherDisplayTimeInSeconds,
	const PatcherDisplayStats& stats)
{
	// Early out if no testing or not enabled.
	if (!m_bIsEnabled)
	{
		return;
	}

	// Now we expect a server time.
	m_bExpectServerTime = true;

	// Check if any state took more than a threshold to complete
	// (except for game script initialization, which can take a very
	// long time on device in developer builds due to developer overhead
	// and the sleepiness of mobile devices).
	static const Float kfThresholdInSeconds = 5.0;
	Float fMaxTimeSeconds = 0.0f;
	for (UInt32 i = 0u; i < stats.m_aPerState.GetSize(); ++i)
	{
		// Ignore this state, can be very expensive on mobile due
		// to developer build overhead and unpredictable sleep
		// behavior of mobile devices.
		if (PatcherState::kWaitingForGameScriptManager == (PatcherState)i)
		{
			continue;
		}

		fMaxTimeSeconds = Max(fMaxTimeSeconds, stats.m_aPerState[i].m_fTimeSecs);
	}

	if (fMaxTimeSeconds > kfThresholdInSeconds)
	{
		PlatformPrint::PrintStringFormatted(
			PlatformPrint::Type::kInfo,
			"Patcher took %.2f seconds:",
			fPatcherDisplayTimeInSeconds);

		for (UInt32 iState = 0u; iState < stats.m_aPerState.GetSize(); ++iState)
		{
			auto const& e = stats.m_aPerState[iState];
			PlatformPrint::PrintStringFormatted(
				PlatformPrint::Type::kInfo,
				"Patcher Step (%s): (%.2f seconds, %u times)",
				EnumToString<PatcherState>(iState),
				e.m_fTimeSecs,
				e.m_uCount);
		}

		// Reloaded files count.
		PlatformPrint::PrintDebugStringFormatted(
			PlatformPrint::Type::kInfo,
			"Patcher Reloaded Files: %u",
			stats.m_uReloadedFiles);

		// Request data.
		PrintRequestData("Auth Login", stats.m_AuthLoginRequest);

		// Also max stats.
		{
			String sURL;
			HTTP::Stats maxStats;
			HTTP::Manager::Get()->GetMaxRequestStats(sURL, maxStats);
			auto uPos = sURL.FindLast("/");
			if (String::NPos != uPos) { sURL = sURL.Substring(uPos + 1u); }

			PrintRequestData(sURL.CStr(), maxStats);
		}

		// Patch data.
		PrintSubStats("apply_stat_", stats.m_tApplySubStats);

		// Downloader data.
		PrintDownloaderData("AdditionalSar", stats.m_AdditionalStats);
		PrintDownloaderData("ConfigSar", stats.m_ConfigStats);
		PrintDownloaderData("ContentSar", stats.m_ContentStats);
	}
}

Bool Automation::SetGlobalState(HString key, const Reflection::Any& anyValue)
{
	// Nothing more to do in this body if the VM is still not valid.
	if (!m_pVm.IsValid())
	{
		return false;
	}

	// Call SetGlobalState in the script VM.
	Script::FunctionInvoker invoker(*m_pVm, kSetGlobalState);
	if (!invoker.IsValid())
	{
		return false;
	}

	invoker.PushString(key);
	invoker.PushAny(anyValue);
	return invoker.TryInvoke();
}

void Automation::OnUIStateChange(
	HString stateMachineId,
	HString previousStateId,
	HString nextStateId)
{
	// Nothing to do if the VM is still loading.
	if (!m_pVm.IsValid())
	{
		return;
	}

	Script::FunctionInvoker invoker(*m_pVm, kOnUIStateChange);
	if (!invoker.IsValid())
	{
		return;
	}

	invoker.PushString(stateMachineId);
	invoker.PushString(previousStateId);
	invoker.PushString(nextStateId);
	if (!invoker.TryInvoke())
	{
		SEOUL_WARN("Game automation failure, failed invocation of OnUIStateChange: %s, %s, %s",
			stateMachineId.CStr(),
			previousStateId.CStr(),
			nextStateId.CStr());
		return;
	}
}

void Automation::InternalApplyAutomatedTestingMode()
{
	// If testing, configure as such.
	if (m_Settings.m_bAutomatedTesting)
	{
#if SEOUL_ENABLE_MEMORY_TOOLING
		// Enable verbose leak detection, if available.
		MemoryManager::SetVerboseMemoryLeakDetectionEnabled(true);
#endif // /#if SEOUL_ENABLE_MEMORY_TOOLING

		// 60.0 is our ideal target frame time. NOTE: This used to be set
		// to 30.0 as a realistic max frame time, but this resulted in the unit
		// test not reproducing bugs that could easily be reproduced when the
		// game was running at 60.0 FPS. Likely, we should either start varying
		// this value in a range, or adjust the test so it can be specified
		// and run the game at those various test values.
#if !(SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS)
		static const Float32 kfAutomatedTestingFixedDeltaTime = (Float32)(1.0 / 60.0);
#endif // /#if !(SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS)

		// Enable all logger channels.
#if SEOUL_LOGGING_ENABLED
		Logger::GetSingleton().EnableAllChannels(true);

		// Enable verbose logging.
		HTTP::Manager::Get()->EnableVerboseHttp2Logging(true);

		// Warn about blocking loads.
		Content::LoadManager::Get()->SetEnableBlockingLoadCheck(true);
#endif // /#if SEOUL_LOGGING_ENABLED

		// Update the Engine's fixed delta time value.
#if !(SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS)
		{
			Engine::Get()->SetFixedSecondsInTick(kfAutomatedTestingFixedDeltaTime);
		}
#endif // /#if !(SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS)
	}
}

static inline HString GetUIGameScreensState(HString name)
{
	auto const& v = UI::Manager::Get()->GetStack();
	for (auto const& e : v)
	{
		if (e.m_pMachine.IsValid() &&
			e.m_pMachine->GetName() == name)
		{
			return e.m_ActiveStateId;
		}
	}

	return HString();
}

void Automation::InternalApplyPerformanceTesting()
{
	// Early out if not enabled.
	if (!m_bEnablePerfTesting)
	{
		return;
	}

	// Threshold and checking based on environment.
#if SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_IOS
	// TODO: Setting this very high as we've found
	// perf. testing on device farm devices to be unreliable
	// due to sleeping, etc.
	static const Double kfThresholdMs = 500.0;
#else
	static const Double kfThresholdMs = 8.0;
#endif

	auto const iTime = SEOUL_PROF_TICKS(kFrame);

	// TODO: I don't like "hiding" problems, but we've too much
	// noise at the moment from GPU spikes. Revisit.
	auto const iSyncTime = SEOUL_PROF_TICKS(kRenderSynchronize);

	++m_uTotalFrames;
	auto const fAdjustedFrameTimeMs = SeoulTime::ConvertTicksToMilliseconds(
		Max(iTime - iSyncTime, (Int64)0));
	auto const iInterval = RenderDevice::Get()->GetVsyncInterval();
	if (fAdjustedFrameTimeMs > kfThresholdMs)
	{
		++m_uLongFrames;
		auto const stateName(GetUIGameScreensState(kGameScreens));
		PlatformPrint::PrintStringFormatted(
			PlatformPrint::Type::kWarning,
			"Warning: Frame %s(%u): (%d, %.2f ms)",
			stateName.CStr(),
			Engine::Get()->GetFrameCount(),
			iInterval,
			fAdjustedFrameTimeMs);
		SEOUL_PROF_LOG_CURRENT(kFrame);
	}
}

void Automation::InternalLoadVm()
{
	Script::VmSettings settings;
	settings.SetStandardBasePaths();
	settings.m_ErrorHandler = SEOUL_BIND_DELEGATE(&CrashManager::DefaultErrorHandler);
#if SEOUL_LOGGING_ENABLED
	settings.m_StandardOutput = SEOUL_BIND_DELEGATE(AutomationLuaLog);
#endif // /#if SEOUL_LOGGING_ENABLED
	settings.m_uInitialGcStepSize = 32u;
	settings.m_uMinGcStepSize = 16u;
	settings.m_uMaxGcStepSize = 64u;
	settings.m_sVmName = "GameAutomation";
	m_pVm.Reset(SEOUL_NEW(MemoryBudgets::Scripting) Script::Vm(settings));

	if (!m_pVm->RunScript(m_Settings.m_sMainScriptFileName, false))
	{
		// NOTE: If you are getting into this block, one potential explanation
		// is that you have a native class exposed to scripts that has not been
		// linked into the current build. If not, add the SEOUL_LINK_ME macro
		// somewhere appropriate (search for existing uses).
		SEOUL_WARN("Game automation failure, failed running main script: %s",
			m_Settings.m_sMainScriptFileName.CStr());
		m_pVm.Reset();
		return;
	}

	// Binder utility.
	auto bindFunc = [&]()
	{
		Script::FunctionInvoker invoker(*m_pVm, kInitialize);
		if (!invoker.IsValid())
		{
			return true;
		}

		(void)invoker.PushUserData<AutomationScriptObject>();
		return invoker.TryInvoke();
	};

	if (!bindFunc())
	{
		SEOUL_WARN("Game automation failure, failed initialization.");
		m_pVm.Reset();
		return;
	}
}

static inline void CheckPackageVersioning(const IPackageFileSystem& pkg)
{
	// Skip CL check if a local build.
	if (g_iBuildChangelist != 0 &&
		pkg.GetBuildChangelist() != g_iBuildChangelist)
	{
		SEOUL_WARN("[GameAutomation]: %s CL%u != Build CL%d",
			pkg.GetAbsolutePackageFilename().CStr(),
			pkg.GetBuildChangelist(),
			g_iBuildChangelist);
	}
	if (pkg.GetBuildVersionMajor() != BUILD_VERSION_MAJOR)
	{
		SEOUL_WARN("[GameAutomation]: %s Version %u != Build Version %d",
			pkg.GetAbsolutePackageFilename().CStr(),
			pkg.GetBuildVersionMajor(),
			BUILD_VERSION_MAJOR);
	}
}

/** Apply validation of various CLs to catch mismatch. */
void Automation::InternalRunClChecks()
{
	// Wait for downloadable system to be ready before checking.
	if (g_pDownloadableContentPackageFileSystem &&
		!g_pDownloadableContentPackageFileSystem->IsInitialized())
	{
		return;
	}

	// Perform checks now.
	m_ClCheckOnce.Call([&]()
	{
		// Verify config.
		auto pConfig(Main::Get()->GetConfigUpdatePackageFileSystem());
		if (pConfig) { CheckPackageVersioning(*pConfig); }

		SEOUL_LOG_AUTOMATION("Config build CL %d", (pConfig.IsValid() ? pConfig->GetBuildChangelist() : 0));

		// Downloadable.
		if (g_pDownloadableContentPackageFileSystem)
		{
			CheckPackageVersioning(*g_pDownloadableContentPackageFileSystem);
		}
	});
}

void Automation::InternalRunSaveLoadManagerChecks()
{
	// Ten saves backed up implies we're falling behind.
	//
	// Double each time to track ever expanding queue.
	static Atomic32Type s_MaxQueueCount = 10;

	if (SaveLoadManager::Get())
	{
		auto const count = SaveLoadManager::Get()->GetWorkQueueCount();
		if (count > s_MaxQueueCount)
		{
			SEOUL_WARN("[GameAutomation]: SaveLoadManager is over max queue size of %u at %u entries.",
				(UInt32)s_MaxQueueCount,
				(UInt32)count);

			// Double size for future checks.
			s_MaxQueueCount *= 2;
		}
	}
}

} // namespace Game

} // namespace Seoul
