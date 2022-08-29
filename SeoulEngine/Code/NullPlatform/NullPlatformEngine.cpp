/**
 * \file NullPlatformEngine.cpp
 * \brief Specialization of Engine for platform independent
 * contexts (headless mode and tools).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BuildChangelistPublic.h"
#include "CoreSettings.h"
#include "Delegate.h"
#include "EncryptAES.h"
#include "GenericAnalyticsManager.h"
#include "GenericInMemorySaveApi.h"
#include "GenericInput.h"
#include "JobsFunction.h"
#include "GenericSaveApi.h"
#include "InputManager.h"
#include "Logger.h"
#include "Mutex.h"
#include "NullGraphicsDevice.h"
#include "NullSaveApi.h"
#include "NullPlatformEngine.h"
#include "PlatformData.h"
#include "ReflectionDefine.h"
#include "ReflectionUtil.h"
#include "SeoulUUID.h"
#include "StringUtil.h"

namespace Seoul
{

NullPlatformEngineSettings::NullPlatformEngineSettings()
	: m_SaveLoadManagerSettings()
	, m_bEnableGenericKeyboardInput(false)
	, m_bEnableGenericMouseInput(false)
	, m_bEnableSaveApi(false)
	, m_bPersistent(false)
	, m_iViewportHeight(600)
	, m_iViewportWidth(800)
	, m_AnalyticsSettings()
	, m_pSharedMemory()
	, m_sBaseDirectoryPath()
	, m_CreateRenderDevice(nullptr)
	, m_CreateSoundManager(nullptr)
	, m_bDefaultGDPRAccepted(true)
{
}

NullPlatformEngineSettings::NullPlatformEngineSettings(const NullPlatformEngineSettings&) = default;
NullPlatformEngineSettings& NullPlatformEngineSettings::operator=(const NullPlatformEngineSettings&) = default;

NullPlatformEngineSettings::~NullPlatformEngineSettings()
{
}

class NullPlatformEngineInputDeviceEnumerator SEOUL_SEALED : public InputDeviceEnumerator
{
public:
	NullPlatformEngineInputDeviceEnumerator(const NullPlatformEngineSettings& settings)
		: m_Settings(settings)
	{
	}

	~NullPlatformEngineInputDeviceEnumerator()
	{
	}

	virtual void EnumerateDevices(InputDevices& rvDevices) SEOUL_OVERRIDE
	{
		// Only add a keyboard handler if specified.
		if (m_Settings.m_bEnableGenericKeyboardInput)
		{
			rvDevices.PushBack(SEOUL_NEW(MemoryBudgets::Input) GenericKeyboard());
		}

		// Only add a mouse handler if specified.
		if (m_Settings.m_bEnableGenericMouseInput)
		{
			rvDevices.PushBack(SEOUL_NEW(MemoryBudgets::Input) GenericMouse());
		}
	}

private:
	NullPlatformEngineSettings const m_Settings;

	SEOUL_DISABLE_COPY(NullPlatformEngineInputDeviceEnumerator);
}; // class NullPlatformEngineInputDeviceEnumerator

struct NullPlatformEngineInternal SEOUL_SEALED
{
	static void InitializeNullGraphicsDevice(NullPlatformEngine* pNullPlatform)
	{
		if (pNullPlatform->m_Settings.m_CreateRenderDevice)
		{
			pNullPlatform->m_pRenderDevice.Reset(pNullPlatform->m_Settings.m_CreateRenderDevice(
				pNullPlatform->m_Settings.m_iViewportWidth,
				pNullPlatform->m_Settings.m_iViewportHeight));
		}

		if (!pNullPlatform->m_pRenderDevice.IsValid())
		{
			pNullPlatform->m_pRenderDevice.Reset(SEOUL_NEW(MemoryBudgets::Rendering) NullGraphicsDevice(
				pNullPlatform->m_Settings.m_iViewportWidth,
				pNullPlatform->m_Settings.m_iViewportHeight));
		}
	}

	static void DestroyNullGraphicsDevice(NullPlatformEngine* pNullPlatform)
	{
		pNullPlatform->m_pRenderDevice.Reset();
	}
};

struct NullPlatformEngineState
{
	String m_sUniqueDeviceIdentifier;
}; // struct NullPlatformEngineState
SEOUL_BEGIN_TYPE(NullPlatformEngineState)
	SEOUL_PROPERTY_N("UUID", m_sUniqueDeviceIdentifier)
SEOUL_END_TYPE()

NullPlatformEngine::NullPlatformEngine(const NullPlatformEngineSettings& settings)
	: Engine()
	, m_Settings(settings)
	, m_pSharedMemory()
	, m_pRenderDevice()
	, m_AppStartUTCTime(WorldTime::GetUTCTime())
{
	// Initialize.
	m_bGDPRAccepted = m_Settings.m_bDefaultGDPRAccepted;

	// Populate and some other simple data bits.
	{
		Lock lock(*m_pPlatformDataMutex);
		m_pPlatformData->m_iAppVersionCode = g_iBuildChangelist;
		m_pPlatformData->m_sAdvertisingId = UUID::GenerateV4().ToString();
		m_pPlatformData->m_sAppVersionName = g_sBuildChangelistStr;
		m_pPlatformData->m_sPlatformUUID = UUID::GenerateV4().ToString();
	}

	// Create a shared memory object if we're going to need it.
	if (m_Settings.m_bEnableSaveApi && !m_Settings.m_bPersistent)
	{
		if (settings.m_pSharedMemory.IsValid())
		{
			m_pSharedMemory = settings.m_pSharedMemory;
		}
		else
		{
			m_pSharedMemory.Reset(SEOUL_NEW(MemoryBudgets::Io) GenericInMemorySaveApiSharedMemory);
		}
	}

	m_iStartUptimeInMilliseconds = 0;
	m_iUptimeInMilliseconds = m_iStartUptimeInMilliseconds;
}

NullPlatformEngine::~NullPlatformEngine()
{
	// Release shared memory.
	m_pSharedMemory.Reset();
}

void NullPlatformEngine::Initialize()
{
	CoreSettings settings;
	settings.m_bLoadLoggerConfigurationFile = false;
	settings.m_bOpenLogFile = false;
	settings.m_GamePathsSettings.m_sBaseDirectoryPath = m_Settings.m_sBaseDirectoryPath;
	InternalPreRenderDeviceInitialization(
		settings,
		m_Settings.m_SaveLoadManagerSettings);

	// If persistent, restore the UUID.
	if (m_Settings.m_bPersistent)
	{
		auto const filePath(FilePath::CreateSaveFilePath("null_platform_engine.json"));

		NullPlatformEngineState state;
		if (!Reflection::LoadObject(filePath, &state))
		{
			// Use the generated ID if not restored.
			state.m_sUniqueDeviceIdentifier = GetPlatformUUID();

			// Commit updated ID.
			if (!Reflection::SaveObject(&state, filePath))
			{
				SEOUL_WARN("NullPlatformEngine failed saving config state.");
			}
		}
		else
		{
			Lock lock(*m_pPlatformDataMutex);
			m_pPlatformData->m_sPlatformUUID = state.m_sUniqueDeviceIdentifier;
		}
	}

	Jobs::AwaitFunction(
		GetRenderThreadId(),
		&NullPlatformEngineInternal::InitializeNullGraphicsDevice,
		this);

	Engine::InternalPostRenderDeviceInitialization();

	NullPlatformEngineInputDeviceEnumerator inputDeviceEnumerator(m_Settings);
	InputManager::Get()->EnumerateInputDevices(&inputDeviceEnumerator);

	Engine::InternalPostInitialization();
}

void NullPlatformEngine::Shutdown()
{
	// Perform basic first step shutdown tasks in engine
	Engine::InternalPreShutdown();

	Engine::InternalPreRenderDeviceShutdown();

	// Destroy the render device
	Jobs::AwaitFunction(
		GetRenderThreadId(),
		&NullPlatformEngineInternal::DestroyNullGraphicsDevice,
		this);

	Engine::InternalPostRenderDeviceShutdown();
}

void NullPlatformEngine::RefreshUptime()
{
	Lock lock(*m_pUptimeMutex);
	m_iUptimeInMilliseconds = (WorldTime::GetUTCTime() - m_AppStartUTCTime).GetMicroseconds() / (Int64)1000;
}

Bool NullPlatformEngine::Tick()
{
	Engine::InternalBeginTick();
	Engine::InternalEndTick();

	return !m_bQuit;
}

Bool NullPlatformEngine::UpdatePlatformUUID(const String& sPlatformUUID)
{
	// Don't allow an empty UUID
	if (sPlatformUUID.IsEmpty())
	{
		return false;
	}

	Lock lock(*m_pPlatformDataMutex);
	m_pPlatformData->m_sPlatformUUID = sPlatformUUID;
	return true;
}

SaveApi* NullPlatformEngine::CreateSaveApi()
{
	if (m_Settings.m_bEnableSaveApi)
	{
		if (m_Settings.m_bPersistent)
		{
			return SEOUL_NEW(MemoryBudgets::Saving) GenericSaveApi;
		}
		else
		{
			return SEOUL_NEW(MemoryBudgets::Saving) GenericInMemorySaveApi(m_pSharedMemory);
		}
	}
	else
	{
		return SEOUL_NEW(MemoryBudgets::Saving) NullSaveApi();
	}
}

AnalyticsManager* NullPlatformEngine::InternalCreateAnalyticsManager()
{
	return CreateGenericAnalyticsManager(m_Settings.m_AnalyticsSettings);
}

Sound::Manager* NullPlatformEngine::InternalCreateSoundManager()
{
	if (m_Settings.m_CreateSoundManager)
	{
		auto p = m_Settings.m_CreateSoundManager();
		if (p)
		{
			return p;
		}
	}

	return Engine::InternalCreateSoundManager();
}

} // namespace Seoul
