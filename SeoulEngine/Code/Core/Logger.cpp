/**
 * \file Logger.cpp
 * \brief SeoulEngine log management. Global singleton that provides
 * real-time and persistent logging functionality for development and
 * debug builds.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BuildChangelistPublic.h"
#include "BuildVersion.h"
#include "Core.h"
#include "DataStore.h"
#include "DataStoreParser.h"
#include "Directory.h"
#include "GamePaths.h"
#include "Logger.h"
#include "LoggerInternal.h"
#include "MoriartyClient.h"
#include "Mutex.h"
#include "Path.h"
#include "SeoulUtil.h"

#include <errno.h>
#include <stdarg.h>
#include <string.h>

#if defined(_MSC_VER)
#	include <io.h>
#	define SEOUL_FILENO _fileno
#	define SEOUL_ISATTY _isatty
#else
#	include <unistd.h>
#	define SEOUL_FILENO fileno
#	define SEOUL_ISATTY isatty
#endif

namespace Seoul
{

#if SEOUL_LOGGING_ENABLED

/* Text for animated portion of progress. */
static Byte const* ksProgressSpinner = "|/-\\";
/* Output to clear a progress line. */
static Byte const* ksClearLineString = "\r                                                               \r";

/** @return A formatted string of the given world time. */
static inline String GetTimeString(const WorldTime& worldTime, Bool bIncludeYearMonthDay)
{
	return worldTime.ToLocalTimeString(bIncludeYearMonthDay);
}

/**
 * @return A formatted string of the current time (and year month and day
 * if bIncludeYearMonthDay is true) that is safe to use in
 * filenames.
 */
String GetCurrentTimeString(Bool bIncludeYearMonthDay)
{
	return GetTimeString(WorldTime::GetUTCTime(), bIncludeYearMonthDay);
}

/** Special values used when processing log.json. */
static const HString ksAll("All");
static const HString ksChannels("Channels");

/**
 * Maximum number of message boxes that will be displayed per-frame.
 * Does not suppress logging associated with the message, only the
 * message box popu.
 */
static const UInt32 knPerFrameMessageBoxLimit = 4u;

/**
 * @return The singleton global instance of Logger used by
 * the entire system.
 */
Logger& Logger::GetSingleton()
{
	static Logger s_Logger;
	return s_Logger;
}

/**
 * Return a human readable name for the
 * predefined channel eChannel.
 */
const String& Logger::GetPredefinedChannelName(LoggerChannel eChannel)
{
	/**
	 * Array of predefined channel names
	 *
	 * If the channel name gets printed out in the message prefix, the predefined
	 * channels are printed as these strings here; user channels are printed out as
	 * just "Channel X".
	 */
	static const String s_asChannelNames[] =
	{
		"Default",

		"Analytics",
		"Animation",
		"Assertion",
		"Audio",
		"AudioEvents",
		"Auth",
		"Automation",
		"Chat",
		"Commerce",
		"Cooking",
		"Core",
		"Engine",
		"FailedGotoLabel",
		"FileIO",
		"HTTP",
		"Input",
		"Loading",
		"Localization",
		"LocalizationWarning",
		"Network",
		"Notification",
		"Performance",
		"Physics",
		"Render",
		"Script",
		"Server",
		"State",
		"Tracking",
		"TransformsErrors",
		"TransformsWarnings",
		"TriggersAndEvents",
		"UI",
		"UIDebug",
		"UnitTest",
		"Video",
		"Warning",
	};

	SEOUL_STATIC_ASSERT(SEOUL_ARRAY_COUNT(s_asChannelNames) == (size_t)LoggerChannel::MinGameChannel);

	SEOUL_ASSERT((Int)eChannel >= 0 && (UInt32)eChannel < SEOUL_ARRAY_COUNT(s_asChannelNames));
	return s_asChannelNames[(UInt32)eChannel];
}

// Not ideal but a convenience place for this.
static Atomic32Value<Bool> s_bTeardownTrace(false);
Bool Logger::IsTeardownTraceEnabled()
{
	return s_bTeardownTrace;
}

void Logger::SetTeardownTraceEnabled(Bool bEnabled)
{
	s_bTeardownTrace = bEnabled;
}

Logger::Logger()
	: m_vGameChannelNames()
	, m_Callbacks()
	, m_iPerformanceTick(-1)
	, m_WarningCount(0u)
	, m_uPerFrameMessageBoxCount(0u)
	, m_OutputStream(stdout)
	, m_ConsoleStream(nullptr)
	, m_ProgressStream(0 != SEOUL_ISATTY(SEOUL_FILENO(stdout)) ? stdout : (0 != SEOUL_ISATTY(SEOUL_FILENO(stderr)) ? stderr : nullptr))
	, m_iLastProgressTicks(-1)
	, m_LastProgressType()
	, m_iProgress(0)
	, m_bCloseOutputStream(false)
	, m_bOutputTimestamps(true)
{
	// Enable all channels by default - otherwise, SEOUL_LOG statements
	// that occur before an explicit configuration load may
	// not be logged.
	m_abChannelsEnabled.Fill(UIntMax);
	m_abIncludeChannelName.Fill(UIntMax);

	// Disable name display of the default channel.
	EnableChannelName(LoggerChannel::Default, false);
}

Logger::~Logger()
{
	CloseOutputStream();
}

/**
 * Setup the logger from the logger's configuration file
 */
Bool Logger::LoadConfiguration()
{
	// Cache the path to log.json.
	FilePath const filePath = GamePaths::Get()->GetLogJsonFilePath();

	// Force load log.json - if this false, return immediately with a failure.
	DataStore dataStore;
	if (!DataStoreParser::FromFile(filePath, dataStore, DataStoreParserFlags::kLogParseErrors))
	{
		return false;
	}

	// Clear out channels.
	m_abChannelsEnabled.Fill(0);

	// Lookup the channels table in log.json
	DataNode channels;
	(void)dataStore.GetValueFromTable(dataStore.GetRootNode(), ksChannels, channels);

	// Iterate all key-value pairs in the channels table.
	auto const iBegin = dataStore.TableBegin(channels);
	auto const iEnd = dataStore.TableEnd(channels);
	for (auto i = iBegin; iEnd != i; ++i)
	{
		// If the channel is "All", enable all channels.
		if (ksAll == i->First)
		{
			m_abChannelsEnabled.Fill(UIntMax);
		}
		else
		{
			// Otherwise, lookup the channel name from the key of the key-value
			// pair in the channels table, and then enable it based on the value
			// (we don't need to explicitly disable the channel, since all channels
			// are disabled by default, above).
			LoggerChannel const eChannel = GetChannelFromName(String(i->First));
			Bool bValue = false;
			if (-1 != (Int)eChannel && dataStore.AsBoolean(i->Second, bValue))
			{
				EnableChannel(eChannel, bValue);
			}
		}
	}

	return true;
}

// Call each engine frame tick. Used to reset the warning message box popup
// suppression that engages to avoid blocking the game on runaway SEOUL_WARN()s.
void Logger::OnFrame()
{
	Lock lock(m_Mutex);
	m_uPerFrameMessageBoxCount = 0u;
}

/**
 * Opens the log to the given filename
 *
 * Opens the log to the given filename.  If an error occurs (e.g. bad filename
 * or file is located in a read-only directory), the program is aborted via
 * SEOUL_ASSERT.  After opening the log, a preamble containing useful information
 * is printed to the log.  If bAppendDateToFilename is true, then the
 * filename is modified by appending the current date and time to it, before
 * the file extension (e.g. "log.txt" becomes "log-20080722130614.txt"), and
 * the original filename is hard linked to the new filename.  This ensures that
 * the log filename is unique and does not overwrite an old log.
 *
 * @param[in] sFilename Filename to open the log to
 * @param[in] bAppendDateToFilename Whether or not to create a unique filename
 *            by appending the current date and time
 */
void Logger::OpenFile(
	const String& sFilename,
	Bool bAppendDateToFilename)
{
	CloseOutputStream();

	String sActualFilename;
	if (bAppendDateToFilename)
	{
		sActualFilename = Path::GetPathWithoutExtension(sFilename);
		const String sExtension = Path::GetExtension(sFilename);

		// Append current date+time and extension to filename
		sActualFilename.Append("-" + GetCurrentTimeString(true));
		sActualFilename.Append(sExtension);
	}
	else
	{
		sActualFilename = sFilename;
	}

	// Create the Log directory if it doesn't exist yet
	Directory::CreateDirPath(Path::GetDirectoryName(sActualFilename));

	LoggerDetail::OpenLogStream(sActualFilename, m_OutputStream);
	int nError = errno;

	if (bAppendDateToFilename)
	{
		LoggerDetail::CreateHardLink(sFilename, sActualFilename);
	}

	LogPreamble();

	if (m_OutputStream != nullptr)
	{
		m_bCloseOutputStream = true;
	}
	else
	{
		m_OutputStream = stdout;
		m_bCloseOutputStream = false;
#if SEOUL_PLATFORM_WINDOWS
		char sError[256];
		strerror_s(sError, SEOUL_ARRAY_COUNT(sError), nError);
		SEOUL_WARN("Failed opening log file \"%s\", logging will be disabled.  Error %d (%s)",
			 sActualFilename.CStr(), nError, sError);
#else
		SEOUL_WARN("Failed opening log file \"%s\", logging will be disabled.  Error %d (%s)",
			 sActualFilename.CStr(), nError, strerror(nError));
#endif
	}
}

/**
 * Opens the log to an arbitrary output stream
 *
 * Opens the log to the given output stream, which can be standard out, a file
 * stream, a string stream, etc.  After
 * opening the log, a preamble containing useful information is printed to the
 * log.
 *
 * @param[in] pStream Stream to open the log to
 */
void Logger::OpenStream(FILE* pStream)
{
	CloseOutputStream();
	m_OutputStream = pStream;

	LogPreamble();
}

/**
 * Register a custom logger callback.
 */
void Logger::RegisterCallback(LoggerCallback callback)
{
	Lock lock(m_Mutex);
	(void)m_Callbacks.Insert((void*)callback);
}

/**
 * Unregister a previously registered logger callback.
 */
void Logger::UnregisterCallback(LoggerCallback callback)
{
	Lock lock(m_Mutex);
	(void)m_Callbacks.Erase((void*)callback);
}

/**
 * Logs a formatted message to the log
 *
 * Formats the given message printf-style and then logs it at the given logging
 * level to the given channel.  The message is only logged if:
 *   (1) The logging level is greater than or equal to the current logging level
 *       (so that the log does not get filled up with unimportant or
 *       uninteresting messages), and
 *   (2) The given channel is enabled.
 *
 * This method is thread-safe.
 *
 * @param[in] eLevel   The logging level to log the message at
 * @param[in] eChannel The channel to log the message to
 * @param[in] sFormat  Format string of the message to be logged
 * @param[in] ...      Arguments for the format string
 */
void Logger::InternalLogMessage(Bool bCache, LoggerChannel eChannel, Byte const* sFormat, va_list arguments)
{
	// When unit testing is enabled, all logging is suppressed, except messages
	// to the unit testing channel.
#if SEOUL_UNIT_TESTS
	if (g_bRunningUnitTests)
	{
		if (LoggerChannel::UnitTest != eChannel)
		{
			Lock lock(m_Mutex);

			// Always track warning contribution to count, even if the channel is being suppressed.
			if (LoggerChannel::LocalizationWarning == eChannel ||
				LoggerChannel::Warning == eChannel)
			{
				++m_WarningCount;
			}

			// Store the entire string.
			va_list argumentsCopy;
			VA_COPY(argumentsCopy, arguments);
			m_vUnitTesting_SuppressedLogging.PushBack(
				GetChannelName(eChannel) + ": " + String::VPrintf(sFormat, argumentsCopy));
			va_end(argumentsCopy);
			return;
		}
	}
#endif // /#if SEOUL_UNIT_TESTS

	// Always track warning contribution to count, even if the channel is disabled.
	if (LoggerChannel::LocalizationWarning == eChannel ||
		LoggerChannel::Warning == eChannel)
	{
		++m_WarningCount;
	}

	// Turn this function into a no-op when logging is disabled
	// Check to see if message should be logged
	if (!IsChannelEnabled(eChannel))
	{
		return;
	}

	// Store the entire string.
	String sIn(String::VPrintf(sFormat, arguments));

	// Check if this message is cached and if so, do not log it.
	Bool bLogging = true;

	// Cache the current time stamp.
	WorldTime const now(WorldTime::GetUTCTime());

	// If logging.
	if (bLogging)
	{
		// Send to moriarty, if enabled.
#if SEOUL_WITH_MORIARTY
		if (MoriartyClient::Get() && MoriartyClient::Get()->IsConnected())
		{
			MoriartyClient::Get()->LogMessage(sIn);
		}
#endif

		// Cache the time string.
		String const sNow(GetTimeString(now, false));

		UInt32 uStartIndex = 0u;
		UInt32 uEndIndex = sIn.Find('\n', uStartIndex);

		// Track whether a custom callback handled this message or not.
		Bool bHandled = false;

		// Log each entry, split on newlines.
		while (uStartIndex < sIn.GetSize())
		{
			// If we did not find a newline, then the entry is the entire string,
			// otherwise it is the substring including the newline.
			String sMessage((String::NPos == uEndIndex)
				? sIn.Substring(uStartIndex)
				: sIn.Substring(uStartIndex, (uEndIndex - uStartIndex) + 1u));

			// Trim all trailing white space, then append the newline terminator
			// for the current platform.
			{
				auto const uLastNonSpace = sMessage.FindLastNotOf(" \t\r\n\f");
				if (String::NPos != uLastNonSpace)
				{
					sMessage.ShortenTo(uLastNonSpace + 1u);
				}

				// Don't use SEOUL_EOL here - any output through stdout or stderr
				// will automatically convert \n to \r\n, which will produce
				// \r\rn if we prematurely add \r\n to the output. This is because
				// those streams wre opened with "w" instead of "wb".
				//
				// Note that this is *not* true of our file IO (through e.g. SyncFile),
				// which always opens with "wb".
				//
				// See discussion: https://github.com/ninja-build/ninja/issues/773
				sMessage.Append('\n');
			}

			String sFullMessage;

			if (m_bOutputTimestamps)
			{
				sFullMessage.Append(sNow);
				sFullMessage.Append(": ");
			}

			// Log channel name if enabled.
			if (IsChannelNamedEnabled(eChannel))
			{
				sFullMessage.Append(GetChannelName(eChannel));
				sFullMessage.Append(": ");
			}

			sFullMessage.Append(sMessage);

			// Ensure thread-safety
			{
				Lock lock(m_Mutex);

				// Performance channel, add a delta from last marker.
				if (LoggerChannel::Performance == eChannel)
				{
					auto const iTicks = SeoulTime::GetGameTimeInTicks();
					if (m_iPerformanceTick >= 0)
					{
						sFullMessage.PopBack(); // Remove the trailing '\n'.
						sFullMessage.Append(String::Printf(" (%.2f ms)", SeoulTime::ConvertTicksToMilliseconds(iTicks - m_iPerformanceTick)));
						sFullMessage.Append('\n');
					}
					m_iPerformanceTick = iTicks;
				}

				// Dispatch to callbacks. Copy locally in case an unregister
				// event occurs inside the callback.
				{
					auto const callbacks = m_Callbacks;
					auto const iBegin = callbacks.Begin();
					auto const iEnd = callbacks.End();
					for (auto i = iBegin; iEnd != i; ++i)
					{
						auto callback = (LoggerCallback)(*i);
						bHandled = callback(sMessage, now, eChannel) || bHandled;
					}
				}

				// Make sure progress isn't being displayed before we log.
				ClearProgressIfOn(m_OutputStream);

				// Write to the output stream.
				fputs(sFullMessage.CStr(), m_OutputStream);

				// Flush message to disk
				fflush(m_OutputStream);

				// Always log to debugging channel.
				{
					auto eOutType = PlatformPrint::Type::kInfo;
					switch (eChannel)
					{
						case LoggerChannel::Assertion: eOutType = PlatformPrint::Type::kFailure; break;
						case LoggerChannel::LocalizationWarning: eOutType = PlatformPrint::Type::kWarning; break;
						case LoggerChannel::Warning: eOutType = PlatformPrint::Type::kWarning; break;
						default:
							eOutType = PlatformPrint::Type::kInfo;
							break;
					};

					PlatformPrint::PrintDebugString(eOutType, sFullMessage);
				}

				// Additional output to debugger and console, if specified.
				if (m_ConsoleStream != nullptr)
				{
					ClearProgressIfOn(m_ConsoleStream);
					fputs(sFullMessage.CStr(), m_ConsoleStream);
				}
			}

			// Find the next entry, may be finished if the log was only 1 line.
			uStartIndex = (uEndIndex == String::NPos ? sIn.GetSize() : uEndIndex + 1u);
			uEndIndex = sIn.Find('\n', uStartIndex);
		}

		// If this is a warning message, show a dialog box, unless
		// we're running unit tests or in headless mode.
		if (eChannel == LoggerChannel::Warning && !g_bRunningAutomatedTests && !g_bRunningUnitTests && !g_bHeadless)
		{
			++m_uPerFrameMessageBoxCount;

			// Custom handler, nop.
			if (bHandled)
			{
				// Nop
			}
			// If we've hit the message box limit for the first time,
			// display a warning indicating as such.
			else if (knPerFrameMessageBoxLimit == m_uPerFrameMessageBoxCount)
			{
				ShowMessageBox(
					"Per-frame message box limit has been reached. Check the log for additional warnings.");
			}
			// Otherwise, display the warning if we're still under the limit.
			else if (m_uPerFrameMessageBoxCount < knPerFrameMessageBoxLimit)
			{
				ShowMessageBox(sIn);
			}
		}
	}
}

/**
 * Clear progress if progress is being logged to the associated stream.
 */
void Logger::ClearProgressIfOn(FILE* stream)
{
	if (m_ProgressStream == stream)
	{
		ClearProgress();
	}
}

/**
 * Close the output stream if it needs to be closed.
 */
void Logger::CloseOutputStream()
{
	if (m_bCloseOutputStream)
	{
		(void)fclose(m_OutputStream);
		m_OutputStream = stdout;
		m_bCloseOutputStream = false;
	}
}

/**
 * Prints out a preamble to the log.
 *
 * Prints out a preamble to the log after it is opened.  The preamble contains
 * useful information such as the build number and the current date and time.
 */
void Logger::LogPreamble()
{
	char message[1024];
	SNPRINTF(message, sizeof(message),
		"Seoul Engine %s.v%s.%s\n"
		"Log opened at %s\n"
		"--------------------------------\n",
		SEOUL_BUILD_CONFIG_STR, BUILD_VERSION_STR, g_ksBuildChangelistStrFixed,
		GetCurrentTimeString(true).CStr());

	if (m_OutputStream != nullptr)
	{
		ClearProgressIfOn(m_OutputStream);
		fputs(message, m_OutputStream);
		fflush(m_OutputStream);
	}

	PlatformPrint::PrintDebugString(PlatformPrint::Type::kInfo, message);
	if (m_ConsoleStream != nullptr)
	{
		ClearProgressIfOn(m_ConsoleStream);
		fputs(message, m_ConsoleStream);
	}
}

/**
 * Enables or disables all channels.
 *
 * @param[in] bEnable true if we are enabling all channels, false otherwise.
 */
void Logger::EnableAllChannels(Bool bEnable)
{
	m_abChannelsEnabled.Fill((bEnable ? UIntMax : 0));
}

/**
 * Enables or disables a channel
 *
 * Enables or disables the given channel.  If bEnable is true, enables
 * the given channel.  If bEnable is false, disables the given channel.
 *
 * @param[in] eChannel The channel to enable or disable
 * @param[in] bEnable  true if we are enabling the channel, false if we
 *                     are disabling the channel
 */
void Logger::EnableChannel(LoggerChannel eChannel, Bool bEnable)
{
	// In Debug build, fail-fast
	SEOUL_ASSERT((Int)eChannel >= 0 && (Int)eChannel < (Int)LoggerChannel::MaxChannel);

	if (bEnable)
	{
		// Set bit in bit vector
		m_abChannelsEnabled[(Int)eChannel >> 5] |= (1 << ((Int)eChannel & 0x1F));
	}
	else
	{
		if (eChannel != LoggerChannel::Default)  // The default channel must never be disabled
		{
			// Clear bit in bit vector
			m_abChannelsEnabled[(Int)eChannel >> 5] &= ~(1 << ((Int)eChannel & 0x1F));
		}
	}
}

/**
 * Enables or disables a channel name display.
 */
void Logger::EnableChannelName(LoggerChannel eChannel, Bool bEnable)
{
	// In Debug build, fail-fast
	SEOUL_ASSERT((Int32)eChannel >= 0 && (Int32)eChannel < (Int32)LoggerChannel::MaxChannel);

	if (bEnable)
	{
		// Set bit in bit vector
		m_abIncludeChannelName[(Int32)eChannel >> 5] |= (1 << ((Int32)eChannel & 0x1F));
	}
	else
	{
		// Clear bit in bit vector
		m_abIncludeChannelName[(Int32)eChannel >> 5] &= ~(1 << ((Int32)eChannel & 0x1F));
	}
}

/**
 * Tests if a channel is currently enabled
 *
 * Tests whether or not the given channel is currently enabled.
 *
 * @param[in] eChannel The channel to test
 *
 * @return true if eChannel is currently enabled, false if
 *         eChannel is currently disabled
 */
Bool Logger::IsChannelEnabled(LoggerChannel eChannel) const
{
	// In Debug build, fail-fast
	SEOUL_ASSERT((Int32)eChannel >= 0 && (Int32)eChannel < (Int32)LoggerChannel::MaxChannel);

	return ((m_abChannelsEnabled[(Int32)eChannel >> 5] & (1 << ((Int32)eChannel & 0x1F))) != 0);
}

/**
 * Checks if the given channel has name displayed enabled.
 */
Bool Logger::IsChannelNamedEnabled(LoggerChannel eChannel) const
{
	// In Debug build, fail-fast
	SEOUL_ASSERT((Int32)eChannel >= 0 && (Int32)eChannel < (Int32)LoggerChannel::MaxChannel);

	return ((m_abIncludeChannelName[(Int32)eChannel >> 5] & (1 << ((Int32)eChannel & 0x1F))) != 0);
}

/**
 * Gets the name of the given channel
 */
String Logger::GetChannelName(LoggerChannel eChannel) const
{
	SEOUL_ASSERT((Int32)eChannel >= 0 && (Int32)eChannel < (Int32)LoggerChannel::MaxChannel);
	if ((Int32)eChannel < (Int32)LoggerChannel::MinGameChannel)
	{
		return GetPredefinedChannelName(eChannel);
	}
	else if ((Int32)eChannel < (Int32)LoggerChannel::MinGameChannel + (Int32)m_vGameChannelNames.GetSize())
	{
		return m_vGameChannelNames[(Int32)eChannel - (Int32)LoggerChannel::MinGameChannel];
	}
	else
	{
		return String::Printf("Channel%d", (Int32)eChannel);
	}
}

/**
 * Gets the channel with the given name, or (LoggerChannel::Enum)-1
 */
LoggerChannel Logger::GetChannelFromName(const String& sChannelName) const
{
	Int32 iChannel;

	// Iterate over all predefined channels with names, try to find a match
	for (iChannel = 0; iChannel < (Int32)LoggerChannel::MinGameChannel; iChannel++)
	{
		if (sChannelName.CompareASCIICaseInsensitive(GetPredefinedChannelName((LoggerChannel)iChannel)) == 0)
		{
			return (LoggerChannel)iChannel;
		}
	}

	// If it wasn't a predefined channel, try a game channel
	for (iChannel = (Int32)LoggerChannel::MinGameChannel; iChannel < (Int32)LoggerChannel::MinGameChannel + (Int32)m_vGameChannelNames.GetSize(); iChannel++)
	{
		if (sChannelName.CompareASCIICaseInsensitive(m_vGameChannelNames[iChannel - (Int32)LoggerChannel::MinGameChannel]) == 0)
		{
			return (LoggerChannel)iChannel;
		}
	}

	// Not found
	return (LoggerChannel)-1;
}

/**
* Sets the names of game-specific channels.  These correspond to the channels
* in the range [MinGameChannel, MinGameChannel + uNumChannels).
*/
void Logger::SetGameChannelNames(String const* paChannelNames, UInt32 uNumChannels)
{
	SEOUL_ASSERT_NO_LOG(uNumChannels <= (UInt32)((Int32)LoggerChannel::MaxChannel - (Int32)LoggerChannel::MinGameChannel));

	Lock lock(m_Mutex);

	if (0u == uNumChannels)
	{
		m_vGameChannelNames.Clear();
	}
	else
	{
		m_vGameChannelNames.Assign(paChannelNames, paChannelNames + uNumChannels);
	}
}

void Logger::AdvanceProgress(
	HString type,
	Float fTimeInSeconds,
	Float fPercentage,
	UInt32 uActiveTasks,
	UInt32 uTotalTasks)
{
	Lock lock(m_Mutex);

	if (nullptr == m_ProgressStream)
	{
		return;
	}

	auto const iTicks = SeoulTime::GetGameTimeInTicks();
	if (type != m_LastProgressType ||
		m_iLastProgressTicks < 0 ||
		SeoulTime::ConvertTicksToMilliseconds(iTicks - m_iLastProgressTicks) >= 100.0)
	{
		m_iLastProgressTicks = iTicks;
	}
	else
	{
		return;
	}

	if (!ksProgressSpinner[m_iProgress]) { m_iProgress = 0; }
	ClearProgress();
	fprintf(m_ProgressStream, "%s: %.2f s (%u/%u): %.2f%%: %c",
		type.CStr(),
		fTimeInSeconds,
		uActiveTasks,
		uTotalTasks,
		Clamp(100.0f * fPercentage, 0.0f, 100.0f),
		ksProgressSpinner[m_iProgress++]);
	if ((stderr) != m_ProgressStream)
	{
		fflush(m_ProgressStream);
	}
	m_LastProgressType = type;
}

void Logger::ClearProgress()
{
	Lock lock(m_Mutex);

	if (nullptr == m_ProgressStream)
	{
		return;
	}

	if (m_LastProgressType.IsEmpty())
	{
		return;
	}

	fwrite(ksClearLineString, 1, strlen(ksClearLineString), m_ProgressStream);
	m_LastProgressType = HString();
}

void Logger::CompleteProgress(
	HString type,
	Float fTimeInSeconds,
	Bool bSuccess)
{
	Lock lock(m_Mutex);

	if (nullptr == m_ProgressStream)
	{
		return;
	}

	// "archive" the progress.
	ClearProgress();
	fprintf(m_ProgressStream, "%s: %s (%.2f s)\n", type.CStr(), bSuccess ? "OK" : "FAIL", fTimeInSeconds);
	if ((stderr) != m_ProgressStream)
	{
		fflush(m_ProgressStream);
	}
}

// Special access when unit tests are running.
#if SEOUL_UNIT_TESTS
void Logger::UnitTesting_ClearSuppressedLogging()
{
	Lock lock(m_Mutex);

	Vector<String> vs;
	vs.Swap(m_vUnitTesting_SuppressedLogging);
}

void Logger::UnitTesting_EmitSuppressedLogging(Byte const* sPrefix /*= ""*/)
{
	Lock lock(m_Mutex);

	for (auto const& s : m_vUnitTesting_SuppressedLogging)
	{
		PlatformPrint::PrintStringFormatted(PlatformPrint::Type::kError, "%s%s", sPrefix, s.CStr());
	}

	Vector<String> vs;
	vs.Swap(m_vUnitTesting_SuppressedLogging);
}
#endif // /#if SEOUL_UNIT_TESTS

/**
 * Debug function to print a stacktrace at the given point of execution.
 */
void LogStackTrace()
{
	// Use static buffer here to avoid dynamic memory. Since this is designed for
	// debugging, it should be able to run even if the system cannot allocate memory
	static Byte sBuffer[6144];
#if SEOUL_ENABLE_STACK_TRACES
	// Capture the callstack, if available

	SNPRINTF(sBuffer, SEOUL_ARRAY_COUNT(sBuffer), "Stack Trace:\n");
	Core::GetStackTraceString(sBuffer + strlen(sBuffer), SEOUL_ARRAY_COUNT(sBuffer) - strlen(sBuffer));
#else
	STRNCAT(sBuffer, "\n<Stack trace unavailable>\n", SEOUL_ARRAY_COUNT(sBuffer));
#endif

	LogMessage(LoggerChannel::Default, "%s", sBuffer);
}

#endif // /#if SEOUL_LOGGING_ENABLED

} // namespace Seoul
