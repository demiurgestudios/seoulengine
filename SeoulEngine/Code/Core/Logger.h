/**
 * \file Logger.h
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

#pragma once
#ifndef LOGGER_H
#define LOGGER_H

#include "HashSet.h"
#include "Mutex.h"
#include "Prereqs.h"
#include "SeoulHString.h"
#include "Vector.h"
namespace Seoul { class WorldTime; }

namespace Seoul
{

// Returns a date+time string using the same function that Logger
// uses when tagging log entries or the log filename.
String GetCurrentTimeString(Bool bIncludeYearMonthDay = true);

/**
 * Predefined logging channels
 *
 * These are the predefined logging channel values.  Log messages should use
 * either one of these, or a value between MinGameChannel (inclusive) and
 * MaxChannel (exclusive).
 *
 * These values must be kept in sync with the array
 * in GetPredefinedChannelName().
 */
enum class LoggerChannel
{
	Default = 0,

	Analytics,
	Animation,
	Assertion,
	Audio,
	AudioEvents,
	Auth,
	Automation,
	Chat,
	Commerce,
	Cooking,
	Core,
	Engine,
	FailedGotoLabel,
	FileIO,
	HTTP,
	Input,
	Loading,
	Localization,
	LocalizationWarning,
	Network,
	Notification,
	Performance,
	Physics,
	Render,
	Script,
	Server,
	State,
	Tracking,
	TransformsErrors,
	TransformsWarnings,
	TriggersAndEvents,
	UI,
	UIDebug,
	UnitTest,
	Video,
	Warning,

	// Not a real channel -- this is the first value that the game code
	// can use for non-predefined engine channels
	MinGameChannel,

	MaxChannel = 128
};

#if SEOUL_LOGGING_ENABLED

/**
 * The class that handles logging data
 *
 * This class handles logging data to a log file or other output device and
 * provides fine controls over what data is output by means of logging levels
 * and channels.
 */
class Logger SEOUL_SEALED
{
public:
	static Logger& GetSingleton();

	static const String& GetPredefinedChannelName(LoggerChannel eChannel);

	// Not ideal but a convenience place for this.
	static Bool IsTeardownTraceEnabled();
	static void SetTeardownTraceEnabled(Bool bEnabled);

	// Destructor - cleans up
	~Logger();

	// Load the logger's configuration file
	Bool LoadConfiguration();

	// Call each engine frame tick. Used to reset the warning message box popup
	// suppression that engages to avoid blocking the game on runaway SEOUL_WARN()s.
	void OnFrame();

	// Opens log to the given file
	void OpenFile(const String & sFilename, Bool bAppendDateToFilename);

	// Opens log to the given file object (e.g. stdout)
	void OpenStream(FILE *pStream);

	// Logs a printf-style formatted message at the given logging level to the
	// given channel.
	//
	// NOTE: W.R.T format attribute, member functions have an implicit
	// 'this' so counting starts at 2 for explicit arguments.
	void LogMessage(LoggerChannel eChannel, const Byte *sFormat, va_list arguments) SEOUL_PRINTF_FORMAT_ATTRIBUTE(3, 0);

	// Get the total number of defined channels.
	UInt32 GetChannelCount() const
	{
		return (UInt32)LoggerChannel::MinGameChannel + m_vGameChannelNames.GetSize();
	}

	// Enables or disables all channels.
	void EnableAllChannels(Bool bEnable);

	// Enables or disables the given channel
	void EnableChannel(LoggerChannel eChannel, Bool bEnable);

	// Enable or disables the display of channel name
	void EnableChannelName(LoggerChannel eChannel, Bool bEnable);

	// Checks if the given channel is enabled
	Bool IsChannelEnabled(LoggerChannel eChannel) const;

	// Checks if the given channel has name displayed enabled.
	Bool IsChannelNamedEnabled(LoggerChannel eChannel) const;

	/**
	 * @return True if timestamps will be added to logger lines, false otherwise.
	 */
	Bool GetOutputTimestamps() const
	{
		return m_bOutputTimestamps;
	}

	/** Enable or disable timestamps in log output. */
	void SetOutputTimestamps(Bool bOutputTimestamps)
	{
		m_bOutputTimestamps = bOutputTimestamps;
	}

	/** @return The total number of warnings since program startup. */
	Atomic32Type GetWarningCount() const
	{
		return m_WarningCount;
	}

	/**
	 * Allow outside entities to register for log events. If an entity has "handled" the event,
	 * it should return true. This indicates to the Logger not to (e.g.) display a message box
	 * for warnings or other critical events.
	 */
	typedef Bool (*LoggerCallback)(const String& sLine, const WorldTime& timestamp, LoggerChannel eChannel);
	void RegisterCallback(LoggerCallback callback);
	void UnregisterCallback(LoggerCallback callback);

	// Gets the name of the given channel (predefined or game-defined)
	String GetChannelName(LoggerChannel eChannel) const;

	// Gets the channel with the given name, or (LoggerChannel)-1
	LoggerChannel GetChannelFromName(const String& sChannelName) const;

	// Sets the names of game-specific channels.  These correspond to the
	// channels in the range [MinGameChannel, MinGameChannel + uNumChannels).
	void SetGameChannelNames(String const* paChannelNames, UInt32 uNumChannels);

	// Animated console progress bar for tools.
	void AdvanceProgress(
		HString type,
		Float fTimeInSeconds,
		Float fPercentage,
		UInt32 uActiveTasks,
		UInt32 uTotalTasks);
	void ClearProgress();
	void CompleteProgress(
		HString type,
		Float fTimeInSeconds,
		Bool bSuccess);

	// Special access when unit tests are running.
#if SEOUL_UNIT_TESTS
	void UnitTesting_ClearSuppressedLogging();
	void UnitTesting_EmitSuppressedLogging(Byte const* sPrefix = "");
#endif // /#if SEOUL_UNIT_TESTS

private:
	// Constructor - opens log to standard out
	Logger();

	SEOUL_DISABLE_COPY(Logger);

	// NOTE: W.R.T format attribute, member functions have an implicit
	// 'this' so counting starts at 2 for explicit arguments.
	void InternalLogMessage(Bool bCache, LoggerChannel eChannel, const Byte *sFormat, va_list arguments) SEOUL_PRINTF_FORMAT_ATTRIBUTE(4, 0);

	// Clear progress if progress is being logged to the associated stream.
	void ClearProgressIfOn(FILE* stream);

	// Close the output stream if it needs to be closed.
	void CloseOutputStream();

	// Prints a preamble to the log
	void LogPreamble();

	// Store for additional channel names.
	typedef Vector<String, MemoryBudgets::Strings> GameChannelNames;
	GameChannelNames m_vGameChannelNames;

	typedef HashSet<void*, MemoryBudgets::Strings> Callbacks;
	Callbacks m_Callbacks;

	Int64 m_iPerformanceTick;

	// Number of warnings that have been received since program start.
	Atomic32Type m_WarningCount;

	// Number of message boxes displayed since the last call to OnFrame().
	UInt32 m_uPerFrameMessageBoxCount;

	// Current output stream being used for the log
	FILE *m_OutputStream;

	// Output stream used for the console window
	FILE *m_ConsoleStream;

	// Output for tools to display an animated/progressive progress bar.
	FILE* m_ProgressStream;
	Int64 m_iLastProgressTicks;
	HString m_LastProgressType;
	Int32 m_iProgress;

	// Mutex for output stream
	Mutex m_Mutex;

	// Whether or not we need to close mfOutputStream
	Bool m_bCloseOutputStream;

	// Whether timestamps are included in logger output or not.
	Bool m_bOutputTimestamps;

	// Bit vector of enabled channels
	FixedArray<UInt32, (UInt32)LoggerChannel::MaxChannel / sizeof(UInt32)> m_abChannelsEnabled;
	FixedArray<UInt32, (UInt32)LoggerChannel::MaxChannel / sizeof(UInt32)> m_abIncludeChannelName;

	// Make sure our bit vector the right number of bits
	SEOUL_STATIC_ASSERT((UInt32)LoggerChannel::MaxChannel % sizeof(UInt32) == 0);

	// Special access when unit tests are running.
#if SEOUL_UNIT_TESTS
	Vector<String> m_vUnitTesting_SuppressedLogging;
#endif // /#if SEOUL_UNIT_TESTS
};

/**
 * Logs a printf-style formatted message at the given logging level to the
 * given channel.
 */
inline void Logger::LogMessage(LoggerChannel eChannel, const Byte *sFormat, va_list arguments)
{
	InternalLogMessage(false, eChannel, sFormat, arguments);
}

/**
 * Helper function - allows the global Logger to be
 * called with variadic arguments.
 */
inline void LogMessage(LoggerChannel eChannel, const Byte *sFormat, ...) SEOUL_PRINTF_FORMAT_ATTRIBUTE(2, 3);
inline void LogMessage(LoggerChannel eChannel, const Byte *sFormat, ...)
{
	va_list arguments;
	va_start(arguments, sFormat);
	Logger::GetSingleton().LogMessage(eChannel, sFormat, arguments);
	va_end(arguments);
}

// Debug function to log a stacktrace at the given point of execution.
void LogStackTrace();

/**
 * Logs a message to the default log on the default channel.
 *
 * @param[in] sFormat  The message format string
 * @param[in] ...      Message arguments
 */
#define SEOUL_LOG(sFormat, ...)   Seoul::LogMessage(Seoul::LoggerChannel::Default, (sFormat), ##__VA_ARGS__)

/**
 * Utility variation of SEOUL_LOG for which the message body is a stack trace at the current point of execution.
 */
#define SEOUL_LOG_STACKTRACE() Seoul::LogStackTrace()

// This should only be called internally by the assertion macro
#define SEOUL_LOG_ASSERTION(sFormat, ...)   Seoul::LogMessage(Seoul::LoggerChannel::Assertion, (sFormat), ##__VA_ARGS__)

// Logs the warning message and displays a dialog box
#define SEOUL_WARN(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Warning, (sFormat), ##__VA_ARGS__)

// Shortcut macros to make it easier to log to a specific channel
#define SEOUL_LOG_ANALYTICS(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Analytics, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_ANIMATION(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Animation, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_AUDIO(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Audio, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_AUDIO_EVENTS(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::AudioEvents, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_AUTH(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Auth, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_AUTOMATION(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Automation, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_CHAT(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Chat, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_COMMERCE(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Commerce, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_COOKING(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Cooking, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_CORE(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Core, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_ENGINE(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Engine, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_FAILEDGOTOLABEL(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::FailedGotoLabel, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_FILEIO(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::FileIO, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_HTTP(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::HTTP, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_INPUT(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Input, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_LOADING(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Loading, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_LOCALIZATION(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Localization, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_LOCALIZATION_WARNING(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::LocalizationWarning, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_NETWORK(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Network, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_NOTIFICATION(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Notification, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_PERFORMANCE(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Performance, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_PHYSICS(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Physics, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_RENDER(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Render, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_SCRIPT(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Script, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_SERVER(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Server, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_STATE(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::State, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_TRACKING(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::Tracking, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_TRANSFORMS_ERRORS(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::TransformsErrors, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_TRANSFORMS_WARNINGS(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::TransformsWarnings, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_TRIGGERS_AND_EVENTS(sFormat, ...)   Seoul::LogMessage(Seoul::LoggerChannel::TriggersAndEvents, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_UI(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::UI, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_UI_DEBUG(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::UIDebug, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_UNIT_TEST(sFormat, ...) Seoul::LogMessage(Seoul::LoggerChannel::UnitTest, (sFormat), ##__VA_ARGS__)
#define SEOUL_LOG_VIDEO(sFormat, ...)   Seoul::LogMessage(Seoul::LoggerChannel::Video, (sFormat), ##__VA_ARGS__)

// Used for identification and tracking of shutdown progression,
// debugging from device farm and other automated builds.
#define SEOUL_TEARDOWN_TRACE() (Seoul::Logger::IsTeardownTraceEnabled() && (SEOUL_LOG("TEARDOWN: %s(%d)", __FUNCTION__, __LINE__), 0))
#define SEOUL_TEARDOWN_TRACE_ENABLE(enable) (Seoul::Logger::SetTeardownTraceEnabled(enable))

#else  // !SEOUL_LOGGING_ENABLED

// Expand macros to no-ops (must result in expressions of type void so that we
// can do things like: foo ? SEOUL_LOG("bar") : baz )
#define SEOUL_LOG(sFormat, ...) ((void)0)
#define SEOUL_LOG_STACKTRACE() ((void)0)
#define SEOUL_LOG_ASSERTION(sFormat, ...) ((void)0)
#define SEOUL_WARN(sFormat, ...) ((void)0)
#define SEOUL_LOG_ANALYTICS(sFormat, ...) ((void)0)
#define SEOUL_LOG_ANIMATION(sFormat, ...) ((void)0)
#define SEOUL_LOG_AUDIO(sFormat, ...) ((void)0)
#define SEOUL_LOG_AUDIO_EVENTS(sFormat, ...) ((void)0)
#define SEOUL_LOG_AUTH(sFormat, ...) ((void)0)
#define SEOUL_LOG_AUTOMATION(sFormat, ...) ((void)0)
#define SEOUL_LOG_CHAT(sFormat, ...) ((void)0)
#define SEOUL_LOG_COMMERCE(sFormat, ...) ((void)0)
#define SEOUL_LOG_COOKING(sFormat, ...) ((void)0)
#define SEOUL_LOG_CORE(sFormat, ...) ((void)0)
#define SEOUL_LOG_ENGINE(sFormat, ...) ((void)0)
#define SEOUL_LOG_FAILEDGOTOLABEL(sFormat, ...) ((void)0)
#define SEOUL_LOG_FILEIO(sFormat, ...) ((void)0)
#define SEOUL_LOG_HTTP(sFormat, ...) ((void)0)
#define SEOUL_LOG_INPUT(sFormat, ...) ((void)0)
#define SEOUL_LOG_LOADING(sFormat, ...) ((void)0)
#define SEOUL_LOG_LOCALIZATION(sFormat, ...) ((void)0)
#define SEOUL_LOG_LOCALIZATION_WARNING(sFormat, ...) ((void)0)
#define SEOUL_LOG_NETWORK(sFormat, ...) ((void)0)
#define SEOUL_LOG_NOTIFICATION(sFormat, ...) ((void)0)
#define SEOUL_LOG_PERFORMANCE(sFormat, ...) ((void)0)
#define SEOUL_LOG_PHYSICS(sFormat, ...) ((void)0)
#define SEOUL_LOG_RENDER(sFormat, ...) ((void)0)
#define SEOUL_LOG_SCRIPT(sFormat, ...) ((void)0)
#define SEOUL_LOG_SERVER(sFormat, ...) ((void)0)
#define SEOUL_LOG_STATE(sFormat, ...) ((void)0)
#define SEOUL_LOG_TRACKING(sFormat, ...) ((void)0)
#define SEOUL_LOG_TRANSFORMS_ERRORS(sFormat, ...) ((void)0)
#define SEOUL_LOG_TRANSFORMS_WARNINGS(sFormat, ...) ((void)0)
#define SEOUL_LOG_TRIGGERS_AND_EVENTS(sFormat, ...) ((void)0)
#define SEOUL_LOG_UI(sFormat, ...) ((void)0)
#define SEOUL_LOG_UI_DEBUG(sFormat, ...) ((void)0)
#define SEOUL_LOG_UNIT_TEST(sFormat, ...) ((void)0)
#define SEOUL_LOG_VIDEO(sFormat, ...) ((void)0)
#define SEOUL_TEARDOWN_TRACE() ((void)0)
#define SEOUL_TEARDOWN_TRACE_ENABLE(enable) ((void)0)

#endif  // !SEOUL_LOGGING_ENABLED

} // namespace Seoul

#endif // include guard
