/**
 * \file DevUIViewLog.cpp
 * \brief In-game log display.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIImGui.h"
#include "DevUIRoot.h"
#include "DevUIViewLog.h"
#include "Engine.h"
#include "HashFunctions.h"
#include "ReflectionDefine.h"

#if (SEOUL_ENABLE_DEV_UI && SEOUL_LOGGING_ENABLED)

namespace Seoul
{

SEOUL_BEGIN_TYPE(DevUI::ViewLog, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(DisplayName, "Log")
	SEOUL_PARENT(DevUI::View)
SEOUL_END_TYPE()

namespace DevUI
{

static const Int32 kiMaxImportantCount = 50;
static const Int32 kiMaxUnimportantCount = 50;
static const UInt32 kuMaxLogMessageSize = 256u;
static const Double kfImportantShowTimeInSeconds = 1.5;

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

static void SetImportant(FixedArray<Bool, (UInt32)LoggerChannel::MaxChannel>& r)
{
	r[(UInt32)LoggerChannel::Assertion] = true;
	r[(UInt32)LoggerChannel::Warning] = true;
}

ViewLog::ViewLog()
	: m_abChannelsEnabled(false)
	, m_tDuplicates()
	, m_lLogBuffer()
	, m_iImportantCount(0)
	, m_Mutex()
	, m_bImportantShow(false)
	, m_fImportantTimeRemainingInSeconds(-1.0)
{
	// Enable a few important channels.
	SetImportant(m_abChannelsEnabled);

	// Set ourselves as the warning handler.
	Logger::GetSingleton().RegisterCallback(&ViewLog::OnLogMessageStatic);
}

ViewLog::~ViewLog()
{
	// Done as the warning handler.
	Logger::GetSingleton().UnregisterCallback(&ViewLog::OnLogMessageStatic);
}

void ViewLog::DoPrePose(Controller& rController, RenderPass& rPass)
{
	using namespace ImGui;

	static const ImVec4 kOld(0.5f, 0.5f, 0.5f, 1);
	static const ImVec4 kNew(1, 1, 1, 1);

	// Center the window if in important mode.
	if (m_fImportantTimeRemainingInSeconds >= 0.0)
	{
		auto const vCenter(ImVec2(
			(GetIO().DisplaySize.x - GetWindowSize().x) * 0.5f,
			(GetIO().DisplaySize.y - GetWindowSize().y) * 0.5f));
		SetWindowPos(vCenter);
	}

	// Guarantee exclusive access to the buffer.
	Lock lock(m_Mutex);

	// Cache logger reference for channel names.
	auto const& logger = Logger::GetSingleton();

	// Copy and paste if supported.
	if (Engine::Get()->SupportsClipboard())
	{
		if (Button("Copy To Clipboard", ImVec2(0, 0), !m_lLogBuffer.IsEmpty()))
		{
			String s;
			for (auto const& e : m_lLogBuffer)
			{
				s.Append(String::Printf("%s: %s: %s",
					e.m_Timestamp.ToLocalTimeString(false).CStr(),
					logger.GetChannelName(e.m_eChannel).CStr(), e.m_sLine.CStr()));
			}

			(void)Engine::Get()->WriteToClipboard(s);
		}
		SameLine();
	}

	// Important messages only.
	{
		Bool bImportant = true;
		for (UInt32 i = 0u; i < m_abChannelsEnabled.GetSize(); ++i)
		{
			if (m_abChannelsEnabled[i] && !IsImportant((LoggerChannel)i))
			{
				bImportant = false;
				break;
			}
		}

		PushStyleColor(ImGuiCol_Button, bImportant
			? GetStyle().Colors[ImGuiCol_PlotHistogramHovered]
			: GetStyle().Colors[ImGuiCol_Button]);
		if (Button("Important Only"))
		{
			bImportant = !bImportant;
			if (bImportant)
			{
				m_abChannelsEnabled.Fill(false);
				SetImportant(m_abChannelsEnabled);
			}
			else
			{
				m_abChannelsEnabled.Fill(true);
			}

		}
		PopStyleColor();
		SameLine();
	}

	// Clear.
	if (Button("Clear", ImVec2(0, 0), !m_lLogBuffer.IsEmpty()))
	{
		m_tDuplicates.Clear();
		m_lLogBuffer.Clear();
		m_iImportantCount = 0;
	}

	// Narrow for mobile.
	if (Root::Get()->IsMobile())
	{
		Columns(1);
	}
	else
	{
		Columns(4);
	}

	// Heading.
	Separator();
	if (!Root::Get()->IsMobile())
	{
		Text("Time"); SetColumnOffset(1, GetColumnMinX() + 100); NextColumn();
		Text("Channel"); SetColumnOffset(2, GetColumnMinX() + 200); NextColumn();
		Text("Message"); SetColumnOffset(3, GetColumnMaxX() - 40); NextColumn();
		Text("#"); NextColumn();
		Separator();
	}

	// Two passes - archived and not.
	Bool bArchived = false;
	for (Int32 i = 0; i < 2; ++i)
	{
		for (auto& e : m_lLogBuffer)
		{
			// Skip disabled channels.
			if (!m_abChannelsEnabled[(UInt32)e.m_eChannel])
			{
				continue;
			}

			// Skip archived mismatch.
			if (e.m_bArchived != bArchived)
			{
				continue;
			}

			e.m_bViewed = true;

			PushStyleColor(ImGuiCol_Text, (e.m_bArchived ? kOld : kNew));

			if (!Root::Get()->IsMobile())
			{
				tm local;
				if (e.m_Timestamp.ConvertToLocalTime(local))
				{
					Text("%02d:%02d:%02d %s",
						(local.tm_hour >= 13 ? local.tm_hour - 12 : local.tm_hour),
						local.tm_min,
						local.tm_sec,
						(local.tm_hour >= 12 ? "PM" : "AM"));
				}
				else
				{
					Text("-");
				}
				NextColumn();
				Text("%s", logger.GetChannelName(e.m_eChannel).CStr()); NextColumn();
			}
			TextWrapped("%s", e.m_sLine.CStr());
			if (!Root::Get()->IsMobile())
			{
				NextColumn();
				Text("%u", e.m_uCount); NextColumn();
			}

			PopStyleColor();

			Separator();
		}

		// Now archived.
		bArchived = true;
	}
	Columns(1);
}

void ViewLog::DoPrePoseAlways(Controller& rController, RenderPass& rPass, Bool bVisible)
{
	if (m_fImportantTimeRemainingInSeconds >= 0.0)
	{
		m_fImportantTimeRemainingInSeconds -= Engine::Get()->DevOnlyGetRawSecondsInTick();
		if (m_fImportantTimeRemainingInSeconds < 0.0)
		{
			m_fImportantTimeRemainingInSeconds = -1.0;
		}
	}

	if (m_bImportantShow)
	{
		auto const bAlwaysShowViews = (Root::Get()->AlwaysShowViews());
		auto const bWasOpen =
			IsOpen() &&
			(bAlwaysShowViews || (Root::Get()->IsMainMenuVisible()));

		// If on mobile, only perform this handling if
		// the main menu is *not* visible, since we
		// don't want to steal away from the active view.
		if (!Root::Get()->IsMobile() ||
			(!bAlwaysShowViews && !Root::Get()->IsMainMenuVisible()))
		{
			m_bImportantShow = false;

			SetOpen(true);
			if (!bAlwaysShowViews)
			{
				if (Root::Get())
				{
					Root::Get()->SetMainMenuVisible(true);
				}
			}

			// Reset the timer if we were forced to open the log menu.
			if (!bWasOpen)
			{
				m_fImportantTimeRemainingInSeconds = kfImportantShowTimeInSeconds;
			}
		}
	}

	// If not visible, archive any messages previously viewed.
	if (!bVisible)
	{
		Lock lock(m_Mutex);
		for (auto& e : m_lLogBuffer)
		{
			if (e.m_bViewed) { e.m_bArchived = true; }
		}
	}
}

Bool ViewLog::IsCloseable() const
{
	if (m_fImportantTimeRemainingInSeconds >= 0.0)
	{
		return false;
	}

	return true;
}

UInt32 ViewLog::GetFlags() const
{
	UInt32 uFlags = ImGuiWindowFlags_HorizontalScrollbar;
	if (m_fImportantTimeRemainingInSeconds >= 0.0)
	{
		uFlags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
	}

	return uFlags;
}

Bool ViewLog::OnLogMessage(const String& sInLine, const WorldTime& timestamp, LoggerChannel eChannel)
{
	// Local assign for truncation support.
	String sLineTruncated;
	if (sInLine.GetSize() > kuMaxLogMessageSize)
	{
		sLineTruncated = sInLine.Substring(0u, kuMaxLogMessageSize);
	}
	else
	{
		sLineTruncated = sInLine;
	}

	// Compute hash on the line.
	UInt32 uHash = GetHash(sLineTruncated);
	IncrementalHash(uHash, (Int32)eChannel);

	// Lock our mutex for the duration of processing.
	{
		Lock lock(m_Mutex);

		auto i = m_lLogBuffer.Insert(m_lLogBuffer.End(), Message(sLineTruncated, timestamp, eChannel, uHash));
		auto bUpdateImportant = IsImportant(eChannel);

		// Check for a duplicate.
		{
			HashableLogBufferIterator iHashIter(i);
			auto pairInserted = m_tDuplicates.Insert(iHashIter);
			if (!pairInserted.Second)
			{
				// Already found -- increment count, update timestamp, reset archive and visibility flags.
				pairInserted.First->m_Iter->m_Timestamp = timestamp;
				pairInserted.First->m_Iter->m_uCount++;
				pairInserted.First->m_Iter->m_bArchived = false;
				pairInserted.First->m_Iter->m_bViewed = false;
				m_lLogBuffer.PopBack();
				// Removed, don't update, one way or another.
				bUpdateImportant = false;
			}
		}

		// Update.
		if (bUpdateImportant)
		{
			++m_iImportantCount;
		}

		// Prune to stay below our max
		Int32 iUnimportantCount = Max((Int32)m_lLogBuffer.GetSize() - m_iImportantCount, 0);
		for (auto i = m_lLogBuffer.Begin(); m_lLogBuffer.End() != i; )
		{
			// A termination case - maxes are ok.
			if (iUnimportantCount <= kiMaxUnimportantCount &&
				m_iImportantCount <= kiMaxImportantCount)
			{
				break;
			}

			// Update based on whether this is an important or unimportant entry.
			if (IsImportant(i->m_eChannel))
			{
				if (m_iImportantCount > kiMaxImportantCount)
				{
					// Erase the entry.
					SEOUL_VERIFY(m_tDuplicates.Erase(HashableLogBufferIterator{ i }));
					i = m_lLogBuffer.Erase(i);
					--m_iImportantCount;
					continue;
				}
			}
			else
			{
				if (iUnimportantCount > kiMaxUnimportantCount)
				{
					// Erase the entry.
					SEOUL_VERIFY(m_tDuplicates.Erase(HashableLogBufferIterator{ i }));
					i = m_lLogBuffer.Erase(i);
					--iUnimportantCount;
					continue;
				}
			}

			// Advance.
			++i;
		}
	}

	// IMPORTANT: Call comes from any thread, so take care what
	// you call from here on.
	if (IsImportant(eChannel))
	{
		m_bImportantShow = true;
	}

	// Always handled.
	return true;
}

} // namespace DevUI

} // namespace Seoul

#endif // /(SEOUL_ENABLE_DEV_UI && SEOUL_LOGGING_ENABLED)
