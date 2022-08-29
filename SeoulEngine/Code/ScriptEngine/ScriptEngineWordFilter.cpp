/**
 * \file ScriptEngineWordFilter.cpp
 * \brief Binder instance for exposing HTTP functionality into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

// TODO: Fix this up reference, ScriptEngine should not be dependent on the Game project.
#include "ReflectionDefine.h"
#include "ScriptEngineWordFilter.h"
#include "ScriptFunctionInterface.h"
#include "ScriptVm.h"
#include "WordFilter.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngineWordFilter, TypeFlags::kDisableCopy)
	SEOUL_METHOD(Construct)
	SEOUL_METHOD(FilterString)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "string sInput")
SEOUL_END_TYPE()

ScriptEngineWordFilter::ScriptEngineWordFilter()
	: m_pFilter(SEOUL_NEW(MemoryBudgets::Scripting) WordFilter)
{
}

ScriptEngineWordFilter::~ScriptEngineWordFilter()
{
	SafeDelete(m_pFilter);
}

static const HString kBlacklist("Blacklist");
static const HString kConfiguration("Configuration");
static const HString kDefaultSubstitution("DefaultSubstitution");
static const HString kKnownWords("KnownWords");
static const HString kSubstitutions("Substitutions");
static const HString kWhitelist("Whitelist");

void ScriptEngineWordFilter::Construct(Script::FunctionInterface *pInterface)
{
	String sFilename;
	FilePath configPath;
	if (pInterface->GetString(1u, sFilename))
	{
		configPath = FilePath::CreateConfigFilePath(sFilename);
	}
	else if (!pInterface->GetFilePath(1u, configPath))
	{
		pInterface->RaiseError(1, "Missing configuration path.");
		return;
	}

	DataStore dataStore;
	if (!DataStoreParser::FromFile(configPath, dataStore))
	{
		pInterface->RaiseError("Failed to parse configuration file %s", configPath.GetAbsoluteFilename().CStr());
		return;
	}

	// Configuration.
	// NOTE: Configuration load must always be first, since it potentially affects
	// the behavior of other Load* calls.
	DataNode value;
	if (dataStore.GetValueFromTable(dataStore.GetRootNode(), kConfiguration, value))
	{
		if (!m_pFilter->LoadConfiguration(dataStore, value))
		{
			pInterface->RaiseError("Failed to load Configuration section of %s", configPath.GetAbsoluteFilename().CStr());
			return;
		}
	}

	// Lists.
	DataNode blacklistNode;
	DataNode knownWordsNode;
	DataNode whitelistNode;
	(void)dataStore.GetValueFromTable(dataStore.GetRootNode(), kBlacklist, blacklistNode);
	(void)dataStore.GetValueFromTable(dataStore.GetRootNode(), kKnownWords, knownWordsNode);
	(void)dataStore.GetValueFromTable(dataStore.GetRootNode(), kWhitelist, whitelistNode);
	if (!blacklistNode.IsNull() ||
		!knownWordsNode.IsNull() ||
		!whitelistNode.IsNull())
	{
		if (!m_pFilter->LoadLists(dataStore, blacklistNode, knownWordsNode, whitelistNode))
		{
			pInterface->RaiseError("Failed to load Blacklist/KnownWords/Whitelist sections of %s", configPath.GetAbsoluteFilename().CStr());
			return;
		}
	}

	// Subsitutions
	if (dataStore.GetValueFromTable(dataStore.GetRootNode(), kDefaultSubstitution, value))
	{
		String sDefaultSubstitution;
		if (!dataStore.AsString(value, sDefaultSubstitution))
		{
			pInterface->RaiseError("Failed to load DefaultSubstitution from %s", configPath.GetAbsoluteFilename().CStr());
			return;
		}
		else
		{
			m_pFilter->SetDefaultSubstitution(sDefaultSubstitution);
		}
	}

	if (dataStore.GetValueFromTable(dataStore.GetRootNode(), kSubstitutions, value))
	{
		if (!m_pFilter->LoadSubstitutionTable(dataStore, value))
		{
			pInterface->RaiseError("Failed to load Substitutions from %s", configPath.GetAbsoluteFilename().CStr());
			return;
		}
	}

	// Success.
	return;
}

void ScriptEngineWordFilter::FilterString(Script::FunctionInterface *pInterface)
{
	if (nullptr == m_pFilter)
	{
		pInterface->RaiseError("ScriptEngineWordFilter not initialized");
		return;
	}

	String sInput;
	if (!pInterface->GetString(1, sInput))
	{
		pInterface->RaiseError(1, "Missing input");
		return;
	}

	(void)m_pFilter->FilterString(sInput);
	pInterface->PushReturnString(sInput);
}

} // namespace Seoul
