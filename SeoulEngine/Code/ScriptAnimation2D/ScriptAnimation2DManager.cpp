/**
 * \file ScriptAnimation2DManager.cpp
 * \brief Script binding for Animation2DManager
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "ReflectionDefine.h"
#include "ScriptAnimation2DManager.h"
#include "ScriptFunctionInterface.h"

#if SEOUL_WITH_ANIMATION_2D
#include "Animation2DManager.h"
#endif

namespace Seoul
{

#if SEOUL_WITH_ANIMATION_2D

class ScriptAnimation2DQuery SEOUL_SEALED
{
public:
	void Construct(FilePath filePath)
	{
		m_FilePath = filePath;
		m_hData = Animation2D::Manager::Get()->GetData(m_FilePath);
	}

	void GetAnimationEvents(Script::FunctionInterface* pInterface) const
	{
		auto const p(m_hData.GetPtr());
		if (!p.IsValid())
		{
			pInterface->PushReturnNil();
			return;
		}

		pInterface->PushReturnAsTable(p->GetEvents());
	}

	UInt32 GetSlotCount() const
	{
		auto const p(m_hData.GetPtr());
		if (!p.IsValid())
		{
			return 0u;
		}

		return p->GetSlots().GetSize();
	}

	Bool HasEvent(HString eventName) const
	{
		// If still loading, return true.
		if (m_hData.IsLoading())
		{
			return true;
		}

		// Resolve and query.
		auto p(m_hData.GetPtr());
		if (!p.IsValid())
		{
			// Return false if an invalid network.
			return false;
		}

		return p->GetEvents().HasValue(eventName);
	}

	Bool IsReady() const
	{
		return m_hData.IsPtrValid();
	}

private:
	FilePath m_FilePath;
	Animation2DDataContentHandle m_hData;
}; // class ScriptAnimation2DQuery

SEOUL_BEGIN_TYPE(ScriptAnimation2DQuery)
	SEOUL_METHOD(GetAnimationEvents)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "SlimCS.Table<string, SlimCS.Table>")
	SEOUL_METHOD(GetSlotCount)
	SEOUL_METHOD(HasEvent)
	SEOUL_METHOD(IsReady)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(ScriptAnimation2DManager)
	SEOUL_METHOD(GetQuery)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "Native.ScriptAnimation2DQuery", "FilePath filePath")
SEOUL_END_TYPE()

void ScriptAnimation2DManager::GetQuery(Script::FunctionInterface* pInterface) const
{
	FilePath filePath;
	if (!pInterface->GetFilePath(1, filePath))
	{
		pInterface->RaiseError(1, "expected file path.");
		return;
	}

	auto pQuery = pInterface->PushReturnUserData<ScriptAnimation2DQuery>();
	if (nullptr == pQuery)
	{
		pInterface->RaiseError(1, "failed instantiating query.");
		return;
	}

	pQuery->Construct(filePath);
}

#endif // /#if SEOUL_WITH_ANIMATION_2D

} // namespace Seoul
