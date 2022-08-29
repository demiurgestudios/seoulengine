/**
 * \file DevUIUtil.cpp
 * \brief Miscellaneous shared functions of DevUI code.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIImGui.h"
#include "DevUIUtil.h"
#include "ToString.h"

#if SEOUL_ENABLE_DEV_UI

namespace Seoul::DevUI::Util
{

void TextDataStore(const DataStore& store, const DataNode& node)
{
	using namespace ImGui;

	Vector<TextDataStoreEntry, MemoryBudgets::DevUI> vEntries;

	if (node.IsArray())
	{
		UInt32 u = 0u;
		SEOUL_VERIFY(store.GetArrayCount(node, u));
		for (UInt32 i = 0u; i < u; ++i)
		{
			DataNode value;
			SEOUL_VERIFY(store.GetValueFromArray(node, i, value));

			TextDataStoreEntry entry;
			entry.m_sKey = ToString(i);
			entry.m_Value = value;

			vEntries.PushBack(entry);
		}
	}
	else if (node.IsTable())
	{
		auto const iBegin = store.TableBegin(node);
		auto const iEnd = store.TableEnd(node);
		for (auto i = iBegin; iEnd != i; ++i)
		{
			HString const key(i->First);
			DataNode const value(i->Second);

			TextDataStoreEntry entry;
			entry.m_sKey = String(key);
			entry.m_Value = value;

			vEntries.PushBack(entry);
		}
	}

	QuickSort(vEntries.Begin(), vEntries.End());

	String s;
	for (auto i = vEntries.Begin(); vEntries.End() != i; ++i)
	{
		if (i->m_Value.IsArray() || i->m_Value.IsTable())
		{
			if (TreeNode(i->m_sKey.CStr()))
			{
				TextDataStore(store, i->m_Value);
				TreePop();
			}
		}
		else
		{
			store.ToString(i->m_Value, s);
			ValueText(i->m_sKey.CStr(), "%s", s.CStr());
		}
	}
}

void ValueText(Byte const* sPrefix, Byte const* sFormat, ...)
{
	va_list argList;
	va_start(argList, sFormat);
	ImGui::Bullet();
	ImGui::SameLine();
	ImGui::Text("%s:", sPrefix);
	ImGui::SameLine();
	ImGui::TextV(sFormat, argList);
	va_end(argList);
}

} // namespace Seoul::DevUI::Util

#endif // /SEOUL_ENABLE_DEV_UI
