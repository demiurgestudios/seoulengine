/**
 * \file NullPlatformEngine.h
 * \brief Specialization of Engine for platform independent
 * contexts (headless mode and tools).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef NULL_PLATFORM_ENGINE_H
#define NULL_PLATFORM_ENGINE_H

#include "Engine.h"
#include "GenericAnalyticsManager.h"
#include "SaveLoadManagerSettings.h"
#include "SeoulString.h"
namespace Seoul { class GenericInMemorySaveApiSharedMemory; }
namespace Seoul { class RenderDevice; }

namespace Seoul
{

/** Settings used to configure the behavior of a null (headless) Engine instance. */
struct NullPlatformEngineSettings SEOUL_SEALED
{
	NullPlatformEngineSettings();
	NullPlatformEngineSettings(const NullPlatformEngineSettings&);
	NullPlatformEngineSettings& operator=(const NullPlatformEngineSettings&);
	~NullPlatformEngineSettings();

	/** Settings for the SaveLoadManager. */
	SaveLoadManagerSettings m_SaveLoadManagerSettings;

	/** Add a GenericKeyboard instance to the Input system. */
	Bool m_bEnableGenericKeyboardInput;

	/** Add a GenericMouse instance to the Input system. */
	Bool m_bEnableGenericMouseInput;

	/**
	 * By default, NullPlatformEngine uses a NullSaveApi
	 * for servicing requests. If true, an GenericInMemorySaveApi
	 * is used instead. Useful for volatile storage for, e.g.,
	 * automated testing.
	 */
	Bool m_bEnableSaveApi;

	/**
	 * If false (the default), device identifier and
	 * storage are volatile and do not persistent between
	 * runs of the app. Otherwise, if true, a persistent
	 * device identifier is generated and a standard GenericStorage()
	 * instance is used to persistent save and identifier data.
	 */
	Bool m_bPersistent;

	/** Fixed height of the NullRenderDevice viewport. */
	Int32 m_iViewportHeight;

	/** Fixed width of the NullRenderDevice viewport. */
	Int32 m_iViewportWidth;

	/** Settings for Analytics, including API key and device information. */
	GenericAnalyticsManagerSettings m_AnalyticsSettings;

	/** Optional - if defined, this shared memory is used instead of a new store. */
	SharedPtr<GenericInMemorySaveApiSharedMemory> m_pSharedMemory;

	/** Optional - if defined, overrides the base directory for the application. */
	String m_sBaseDirectoryPath;

	/** Optional - enable custom render device creation, useful for headless devices with actual rendering capabilities. */
	RenderDevice* (*m_CreateRenderDevice)(Int32 iViewportWidth, Int32 iViewportHeight);

	/** Optional - enable custom sound device creation. */
	Sound::Manager* (*m_CreateSoundManager)();

	/** If true, GDPR is accepted by default. */
	Bool m_bDefaultGDPRAccepted;
}; // struct NullPlatformEngineSettings


class NullPlatformEngine SEOUL_SEALED : public Engine
{
public:
	static CheckedPtr<NullPlatformEngine> Get()
	{
		if (Engine::Get() && Engine::Get()->GetType() == EngineType::kNull)
		{
			return (NullPlatformEngine*)Engine::Get().Get();
		}
		else
		{
			return CheckedPtr<NullPlatformEngine>();
		}
	}

	NullPlatformEngine(const NullPlatformEngineSettings& settings);
	~NullPlatformEngine();

	virtual EngineType GetType() const SEOUL_OVERRIDE { return EngineType::kNull; }

	virtual void Initialize() SEOUL_OVERRIDE;
	virtual void Shutdown() SEOUL_OVERRIDE;

	// Manual refresh of Uptime.
	virtual void RefreshUptime() SEOUL_OVERRIDE SEOUL_SEALED;

	virtual Bool HasFocus() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool Tick() SEOUL_OVERRIDE;

	virtual SaveApi* CreateSaveApi() SEOUL_OVERRIDE;

	virtual Bool UpdatePlatformUUID(const String& sPlatformUUID) SEOUL_OVERRIDE;

	const NullPlatformEngineSettings& GetSettings() const
	{
		return m_Settings;
	}

	/**
	* Asks for application quit. Not supported on all platforms.
	*/
	virtual Bool PostNativeQuitMessage() SEOUL_OVERRIDE
	{
		m_bQuit = true;
		return true;
	}

	virtual void SetGDPRAccepted(Bool bAccepted) SEOUL_OVERRIDE { m_bGDPRAccepted = bAccepted; }
	virtual Bool GetGDPRAccepted() const SEOUL_OVERRIDE { return m_bGDPRAccepted; }

private:
	friend struct NullPlatformEngineInternal;

	virtual AnalyticsManager* InternalCreateAnalyticsManager() SEOUL_OVERRIDE;
	virtual Sound::Manager* InternalCreateSoundManager() SEOUL_OVERRIDE;

	NullPlatformEngineSettings const m_Settings;
	SharedPtr<GenericInMemorySaveApiSharedMemory> m_pSharedMemory;
	ScopedPtr<RenderDevice> m_pRenderDevice;
	WorldTime m_AppStartUTCTime;
	TimeInterval m_Uptime;
	Bool m_bQuit{false};
	Bool m_bGDPRAccepted = false;

	SEOUL_DISABLE_COPY(NullPlatformEngine);
}; // class NullPlatformEngine

} // namespace Seoul

#endif // include guard
