/**
 * \file EditorUIViewLog.cpp
 * \brief Log pane for the editor.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIImGui.h"
#include "EditorUILogBuffer.h"
#include "EditorUIViewLog.h"
#include "Engine.h"

namespace Seoul::EditorUI
{

static inline Bool IsImportant(LoggerChannel eChannel)
{
	switch (eChannel)
	{
	case LoggerChannel::Assertion:
	case LoggerChannel::Warning:
		return true;
	default:
		return false;
	};
}

ViewLog::ViewLog()
	: m_LastUpdateCount(0)
{
}

ViewLog::~ViewLog()
{
}

void ViewLog::DoPrePose(DevUI::Controller& rController, RenderPass& rPass)
{
#if SEOUL_LOGGING_ENABLED
	using namespace ImGui;

	// Guarantee exclusive access to the buffer.
	LogBufferLock lock;

	// Early out if nothing to display.
	auto const& lLogBuffer = lock.GetLogBuffer();
	if (lLogBuffer.IsEmpty())
	{
		return;
	}

	// Cache iterators to buffer.
	auto const iBegin = lLogBuffer.Begin();
	auto const iEnd = lLogBuffer.End();

	// Cache logger reference for channel names.
	auto const& logger = Logger::GetSingleton();

	// Copy and paste if supported.
	if (Engine::Get()->SupportsClipboard())
	{
		if (Button("Copy To Clipboard"))
		{
			String s;
			for (auto i = iBegin; iEnd != i; ++i)
			{
				s.Append(String::Printf("%s: %s: %s", i->m_Timestamp.ToLocalTimeString(false).CStr(), logger.GetChannelName(i->m_eChannel).CStr(), i->m_sLine.CStr()));
			}

			(void)Engine::Get()->WriteToClipboard(s);
		}
	}

	if (BeginTable("Log", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable))
	{
		// Heading.
		TableSetupColumn("Time", 0, 0.5f);
		TableSetupColumn("Channel", 0, 0.5f);
		TableSetupColumn("Message", 0, 5.0f);
		TableSetupColumn("#", 0, 0.2f);
		TableHeadersRow();

		// Entries.
		for (auto i = iBegin; iEnd != i; ++i)
		{
			TableNextRow();

			// Red row if an important (warning/error) row.
			Bool const bImportant = IsImportant(i->m_eChannel);
			if (bImportant) { PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1)); }

			// Time.
			{
				TableSetColumnIndex(0);

				tm localTime;
				if (i->m_Timestamp.ConvertToLocalTime(localTime))
				{
					auto const bPM = (localTime.tm_hour >= 12);
					auto iHour = (localTime.tm_hour % 12);
					iHour = (0 == iHour ? 12 : iHour);
					Text("%02d:%02d:%02d %s",
						iHour,
						localTime.tm_min,
						localTime.tm_sec,
						(bPM ? "PM" : "AM"));
				}
			}

			// Channel.
			{
				TableSetColumnIndex(1);
				Text("%s", logger.GetChannelName(i->m_eChannel).CStr());
			}

			// Message.
			{
				TableSetColumnIndex(2);
				TextWrapped("%s", i->m_sLine.CStr());
			}

			// Count.
			{
				TableSetColumnIndex(3);
				Text("%u", i->m_uCount);
			}

			if (bImportant) { PopStyleColor(); }
		}
		
		EndTable();
	}

	if (m_LastUpdateCount != lock.GetUpdateCount())
	{
		m_LastUpdateCount = lock.GetUpdateCount();
		SetScrollY(GetCursorPosY());
	}
#endif // /#if SEOUL_LOGGING_ENABLED
}

} // namespace Seoul::EditorUI
