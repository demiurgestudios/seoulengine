/**
 * \file GameConfigManager.h
 * \brief Game::ConfigManager is (roughly) the concrete equivalent to
 * SettingsManager.
 *
 * While all data in SettingsManager is in a dynamic DataStore
 * structure, ConfigManager is expected to contain a concrete,
 * C++ hierarchy of data objects.
 *
 * As such, an App must define a concrete subclass of GameConfigManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_CONFIG_MANAGER_H
#define GAME_CONFIG_MANAGER_H

#include "Delegate.h"
#include "JobsJob.h"
#include "Prereqs.h"
#include "ReflectionAttribute.h"
#include "ReflectionWeakAny.h"
#include "Singleton.h"
namespace Seoul { namespace Game { class ConfigManager; } }

namespace Seoul
{

namespace Reflection
{

namespace Attributes
{

/**
 * Attribute to override the file or directory name of a config Property.
 *
 * By default, ConfigData Properties look for a file (or directory,
 * in the case of ManyToOne Properties) that match the property name.
 *
 * This attribute can be used to override the name.
 *
 * \example
 *
 * Note that this attribute can, in particular, be used to support deeper
 * hierarchies for ManyToOne Properties. For example:
 *     [ConfigName("Objects/Explosives")]
 *     public Dictionary<string, Explosive> Explosives
 *
 * Would allow a ConfigData Property to map to the folder Resources/Config/Global/Objects/Explosives
 * instead of the default, which would be Resources/Config/Global/Explosives
 * </example>
 */
class ConfigName SEOUL_SEALED : public Attribute
{
public:
	template <size_t zStringArrayLengthInBytes>
	ConfigName(Byte const (&sName)[zStringArrayLengthInBytes])
		: m_Name(CStringLiteral(sName))
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("ConfigName");
		return kId;
	}

	HString m_Name;
}; // class ConfigName

/**
 * Attribute to associate with config collections of ConfigData.
 *
 * When specified on a Property of ConfigData, the Property must
 * be a Dictionary<string, value-type>. The name of the property will
 * be used as a foldername instead of a filename, and all matching
 * files will be serialized as values into the Dictionary.
 */
class ManyToOne SEOUL_SEALED : public Attribute
{
public:
	ManyToOne()
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("ManyToOne");
		return kId;
	}
}; // class ManyToOne

/**
 * Attribute used to construct the concrete ConfigManager
 * subclass.
 *
 * This is a workaround for the lack of New<>() with arguments
 * support in the reflection system.
 */
class CreateConfigManager SEOUL_SEALED : public Attribute
{
public:
	typedef CheckedPtr<Game::ConfigManager> (*CreateConfigManagerFunc)(const Reflection::WeakAny& pConfigData);

	CreateConfigManager(CreateConfigManagerFunc pCreateConfigManager)
		: m_pCreateConfigManager(pCreateConfigManager)
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("CreateConfigManager");
		return kId;
	}

	CreateConfigManagerFunc const m_pCreateConfigManager;
}; // class CreateConfigManager

/**
 * Attribute to put on the concrete ConfigManager subclass,
 * defines the type of the root config data object used
 * by the class.
 */
class RootConfigDataType SEOUL_SEALED : public Attribute
{
public:
	template <size_t zStringArrayLengthInBytes>
	RootConfigDataType(Byte const (&sName)[zStringArrayLengthInBytes])
		: m_Name(CStringLiteral(sName))
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("RootConfigDataType");
		return kId;
	}

	HString m_Name;
}; // class RootConfigDataType

} // namespace Attribute

} // namespace Reflection

namespace Game
{

/**
 * Abstract Game::ConfigManager base class.
 */
class ConfigManager SEOUL_ABSTRACT : public Singleton<ConfigManager>
{
public:
	virtual ~ConfigManager();

protected:
	ConfigManager();

private:
	SEOUL_DISABLE_COPY(ConfigManager);
}; // class Game::ConfigManager

/**
 * Utility to asynchronously load a root config data
 * object, later used to construct the concrete
 * ConfigManager.
 */
class ConfigManagerLoadJob SEOUL_SEALED : public Jobs::Job
{
public:
	ConfigManagerLoadJob(const Reflection::Type& configManagerType);
	~ConfigManagerLoadJob();

	ScopedPtr<ConfigManager>& GetConfigManager()
	{
		return m_pConfigManager;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(ConfigManagerLoadJob);

	const Reflection::Type& m_ConfigManagerType;
	ScopedPtr<ConfigManager> m_pConfigManager;

	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE;

	SEOUL_DISABLE_COPY(ConfigManagerLoadJob);
}; // class Game::ConfigManagerLoadJob

class NullConfigData SEOUL_SEALED
{
public:
	NullConfigData();
	~NullConfigData();

private:
	SEOUL_DISABLE_COPY(NullConfigData);
}; // class NullConfigData

class NullConfigManager SEOUL_SEALED : public ConfigManager
{
public:
	static CheckedPtr<ConfigManager> CreateNullConfigManager(const Reflection::WeakAny& pConfigData);

	static CheckedPtr<NullConfigManager> Get()
	{
		return (NullConfigManager*)ConfigManager::Get().Get();
	}

	static CheckedPtr<const NullConfigManager> GetConst()
	{
		return (NullConfigManager*)ConfigManager::Get().Get();
	}

	~NullConfigManager();

	/**
	 * Global root of ConfigData. Specific data from specific .json resources
	 * are member properties of this root instance.
	 */
	const NullConfigData& GetConfigData() const
	{
		return *m_pConfigData;
	}

private:
	NullConfigManager(CheckedPtr<NullConfigData> pConfigData);

	ScopedPtr<NullConfigData const> m_pConfigData;

	SEOUL_DISABLE_COPY(NullConfigManager);
}; // class NullConfigManager

} // namespace Game

} // namespace Seoul

#endif // include guard
