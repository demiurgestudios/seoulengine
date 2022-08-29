/**
 * \file AnalyticsManager.cpp
 * \brief Global singleton, exposes functionality to track
 * game events, revenue, and other analytics.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnalyticsManager.h"
#include "EventsManager.h"
#include "Logger.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ReflectionUtil.h"
#include "StackOrHeapArray.h"
#include "StringUtil.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(AnalyticsManagerType)
	SEOUL_ENUM_N("Null", AnalyticsManagerType::kNull)
	SEOUL_ENUM_N("Mixpanel", AnalyticsManagerType::kMixpanel)
SEOUL_END_ENUM()

SEOUL_BEGIN_TYPE(AnalyticsEvent, TypeFlags::kDisableCopy)
	SEOUL_PROPERTY_N("Name", m_sName)
	SEOUL_PROPERTY_N_EXT("Properties", m_Properties, PropertyFlags::kDisableGet | PropertyFlags::kDisableSet)
		SEOUL_ATTRIBUTE(CustomSerializeProperty, "DeserializeProperties", "SerializeProperties")
	SEOUL_PROPERTY_N("OnceToken", m_sOnceToken)

	SEOUL_METHOD(DeserializeProperties)
	SEOUL_METHOD(SerializeProperties)
SEOUL_END_TYPE()

SEOUL_BEGIN_ENUM(AnalyticsProfileUpdateOp)
	SEOUL_ENUM_N("Unknown", AnalyticsProfileUpdateOp::kUnknown)
	SEOUL_ENUM_N("Add", AnalyticsProfileUpdateOp::kAdd)
	SEOUL_ENUM_N("Append", AnalyticsProfileUpdateOp::kAppend)
	SEOUL_ENUM_N("Remove", AnalyticsProfileUpdateOp::kRemove)
	SEOUL_ENUM_N("Set", AnalyticsProfileUpdateOp::kSet)
	SEOUL_ENUM_N("SetOnce", AnalyticsProfileUpdateOp::kSetOnce)
	SEOUL_ENUM_N("Union", AnalyticsProfileUpdateOp::kUnion)
	SEOUL_ENUM_N("Unset", AnalyticsProfileUpdateOp::kUnset)
SEOUL_END_ENUM()

SEOUL_BEGIN_TYPE(AnalyticsProfileUpdate, TypeFlags::kDisableCopy)
	SEOUL_PROPERTY_N("Op", m_eOp)
	SEOUL_PROPERTY_N_EXT("Updates", m_Updates, PropertyFlags::kDisableGet | PropertyFlags::kDisableSet)
		SEOUL_ATTRIBUTE(CustomSerializeProperty, "DeserializeUpdates", "SerializeUpdates")

	SEOUL_METHOD(DeserializeUpdates)
	SEOUL_METHOD(SerializeUpdates)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(AnalyticsSessionChangeEvent)
	SEOUL_PROPERTY_N("SessionStart", m_bSessionStart)
	SEOUL_PROPERTY_N("SessionUUID", m_SessionUUID)
	SEOUL_PROPERTY_N("TimeStamp", m_TimeStamp)
	SEOUL_PROPERTY_N("Duration", m_Duration)
SEOUL_END_TYPE()

AnalyticsManager::~AnalyticsManager()
{
}

AnalyticsManager::AnalyticsManager()
	: m_Mutex()
	, m_sUserID()
	, m_bSandboxed(false)
{
}

Bool AnalyticsEvent::DeserializeProperties(Reflection::SerializeContext* pContext, DataStore const* pDataStore, const DataNode& value)
{
	if (value.IsArray())
	{
		m_Properties.MakeArray();
	}
	else
	{
		m_Properties.MakeTable();
	}
	return m_Properties.DeepCopy(
		*pDataStore,
		value,
		m_Properties.GetRootNode());
}

Bool AnalyticsEvent::SerializeProperties(Reflection::SerializeContext* pContext, HString propertyName, DataStore* pDataStore, const DataNode& table) const
{
	if (m_Properties.GetRootNode().IsArray())
	{
		if (!pDataStore->SetArrayToTable(table, propertyName))
		{
			return false;
		}
	}
	else
	{
		// Not an array - must be a table
		if (!pDataStore->SetTableToTable(table, propertyName))
		{
			return false;
		}
	}

	// Done if m_Properties is null.
	if (m_Properties.GetRootNode().IsNull())
	{
		return true;
	}

	DataNode toTable;
	SEOUL_VERIFY(pDataStore->GetValueFromTable(table, propertyName, toTable));

	return pDataStore->DeepCopy(
		m_Properties,
		m_Properties.GetRootNode(),
		toTable);
}

Bool AnalyticsProfileUpdate::DeserializeUpdates(Reflection::SerializeContext* pContext, DataStore const* pDataStore, const DataNode& value)
{
	if (value.IsArray())
	{
		m_Updates.MakeArray();
	}
	else
	{
		m_Updates.MakeTable();
	}
	return m_Updates.DeepCopy(
		*pDataStore,
		value,
		m_Updates.GetRootNode());
}

Bool AnalyticsProfileUpdate::SerializeUpdates(Reflection::SerializeContext* pContext, HString propertyName, DataStore* pDataStore, const DataNode& table) const
{
	if (m_Updates.GetRootNode().IsArray())
	{
		if (!pDataStore->SetArrayToTable(table, propertyName))
		{
			return false;
		}
	}
	else
	{
		// Not an array - must be a table
		if (!pDataStore->SetTableToTable(table, propertyName))
		{
			return false;
		}
	}

	// Done if m_Updates is null.
	if (m_Updates.GetRootNode().IsNull())
	{
		return true;
	}

	DataNode toTable;
	SEOUL_VERIFY(pDataStore->GetValueFromTable(table, propertyName, toTable));

	return pDataStore->DeepCopy(
		m_Updates,
		m_Updates.GetRootNode(),
		toTable);
}

void NullAnalyticsManager::DoEnterBackground()
{
	DoTrackEvent(AnalyticsEvent("DoEnterBackground"), WorldTime());
}

void NullAnalyticsManager::DoLeaveBackground()
{
	DoTrackEvent(AnalyticsEvent("DoLeaveBackground"), WorldTime());
}

/**
 * NullAnalyticsManager implements DoTrackEvent() by logging
 * the event to the analytics log channel with 1 frame of call stack info.
 */
void NullAnalyticsManager::DoTrackEvent(const AnalyticsEvent& evt, const WorldTime& /*timestamp*/)
{
	Byte sStackInfo[512u] = "<call stack unavailable>";
	size_t aCallStack[3u];

	(void)sStackInfo;
	(void)aCallStack;

#if SEOUL_ENABLE_STACK_TRACES
	UInt32 const nCallStack = Core::GetCurrentCallStack(2, SEOUL_ARRAY_COUNT(aCallStack), aCallStack);
	if (nCallStack > 0u)
	{
		Core::PrintStackTraceToBuffer(sStackInfo, sizeof(sStackInfo), "    ", aCallStack, nCallStack);
	}
#endif // /!SEOUL_ENABLE_STACK_TRACES

#if SEOUL_LOGGING_ENABLED
	String sProperties;
	evt.GetProperties().ToString(evt.GetProperties().GetRootNode(), sProperties, false, 0, true);

	SEOUL_LOG_ANALYTICS("[NullAnalyticsManager]: %s %s\n%s",
		evt.GetName().CStr(),
		sProperties.CStr(),
		sStackInfo);
#endif
}

/**
 * NullAnalyticsManager implements DoTrackSessionEnd() by logging
 * the event to the analytics log channel with 1 frame of call stack info.
 */
void NullAnalyticsManager::DoTrackSessionEnd(const WorldTime& /*timestamp*/)
{
	Byte sStackInfo[512u] = "<call stack unavailable>";
	size_t aCallStack[3u];

	(void)sStackInfo;
	(void)aCallStack;

#if SEOUL_ENABLE_STACK_TRACES
	UInt32 const nCallStack = Core::GetCurrentCallStack(2, SEOUL_ARRAY_COUNT(aCallStack), aCallStack);
	if (nCallStack > 0u)
	{
		Core::PrintStackTraceToBuffer(sStackInfo, sizeof(sStackInfo), "    ", aCallStack, nCallStack);
	}
#endif // /!SEOUL_ENABLE_STACK_TRACES

#if SEOUL_LOGGING_ENABLED
	SEOUL_LOG_ANALYTICS("[NullAnalyticsManager]: SessionEnd\n%s",
		sStackInfo);
#endif
}

/**
 * NullAnalyticsManager implements DoTrackSessionStart() by logging
 * the event to the analytics log channel with 1 frame of call stack info.
 */
void NullAnalyticsManager::DoTrackSessionStart(const WorldTime& timestamp)
{
	Byte sStackInfo[512u] = "<call stack unavailable>";
	size_t aCallStack[3u];

	(void)sStackInfo;
	(void)aCallStack;

#if SEOUL_ENABLE_STACK_TRACES
	UInt32 const nCallStack = Core::GetCurrentCallStack(2, SEOUL_ARRAY_COUNT(aCallStack), aCallStack);
	if (nCallStack > 0u)
	{
		Core::PrintStackTraceToBuffer(sStackInfo, sizeof(sStackInfo), "    ", aCallStack, nCallStack);
	}
#endif // /!SEOUL_ENABLE_STACK_TRACES

#if SEOUL_LOGGING_ENABLED
	SEOUL_LOG_ANALYTICS("[NullAnalyticsManager]: SessionStart\n%s",
		sStackInfo);
#endif
}

void NullAnalyticsManager::DoUpdateProfile(
	const AnalyticsProfileUpdate& update,
	const WorldTime& /*timestamp*/)
{
	Byte sStackInfo[512u] = "<call stack unavailable>";
	size_t aCallStack[3u];

	(void)sStackInfo;
	(void)aCallStack;

#if SEOUL_ENABLE_STACK_TRACES
	UInt32 const nCallStack = Core::GetCurrentCallStack(2, SEOUL_ARRAY_COUNT(aCallStack), aCallStack);
	if (nCallStack > 0u)
	{
		Core::PrintStackTraceToBuffer(sStackInfo, sizeof(sStackInfo), "    ", aCallStack, nCallStack);
	}
#endif // /!SEOUL_ENABLE_STACK_TRACES

#if SEOUL_LOGGING_ENABLED
	String sUpdates;
	update.GetUpdates().ToString(update.GetUpdates().GetRootNode(), sUpdates, false, 0, true);

	SEOUL_LOG_ANALYTICS("[NullAnalyticsManager]: %s(%s) at \n%s",
		EnumToString<AnalyticsProfileUpdateOp>(update.GetOp()),
		sUpdates.CStr(),
		sStackInfo);
#endif
}

Bool NullAnalyticsManager::ShouldSetInSandboxProfileProperty()
{
	return false;
}

} // namespace Seoul
