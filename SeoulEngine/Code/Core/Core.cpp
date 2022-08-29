/**
 * \file Core.cpp
 * \brief Primary header file for the core singleton. Contains
 * shared functionality that needs runtime initialization
 * (e.g. map file support for stack traces).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Core.h"
#include "CoreSettings.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "Logger.h"
#include "MoriartyClient.h"
#include "PackageFileSystem.h"
#include "Path.h"
#include "Platform.h"
#include "SeoulSocket.h"
#include "SeoulMath.h"
#include "SeoulTime.h"
#include "StringUtil.h"

#if SEOUL_PLATFORM_WINDOWS
// None
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_LINUX
#include <execinfo.h>
#if SEOUL_PLATFORM_IOS
#include "IOSUtil.h"
#endif // /#if SEOUL_PLATFORM_IOS
#elif SEOUL_PLATFORM_ANDROID
#include <android/log.h>
#include <unwind.h>
#endif

#if SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
/**
 * Wide string variation of vscwprintf for platforms
 * that don't support it.
 */
int vscwprintf(wchar_t const* sFormat, va_list argList)
{
	using namespace Seoul;

	static const Int kHalfInitialBufferSize = 512;
	Int iBufferSize = kHalfInitialBufferSize;

	ScopedArray<wchar_t> buffer;
	Int iResult = -1;
	do
	{
		iBufferSize *= 2;
		buffer.Reset(SEOUL_NEW(MemoryBudgets::Strings) wchar_t[iBufferSize]);
		iResult = vswprintf(buffer.Get(), iBufferSize, sFormat, argList);
	} while (iResult < 0);

	return iResult;
}
#endif // /#if SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX

/**
 * Seoul engine namespace
 *
 * Seoul engine namespace.  All of the classes, functions, objects, etc. of
 * the Seoul engine are defined within this namespace.  Any game built on the
 * Seoul engine should use a different namespace.
 */
namespace Seoul
{

/** Names of the platforms, e.g. "PC" */
const Byte* kaPlatformNames[(UInt32)Platform::SEOUL_PLATFORM_COUNT] =
{
	"PC",
	"IOS",
	"Android",
	"Linux",
};

/** Macros names used for effect compiling, e.g. "SEOUL_PLATFORM_WINDOWS" */
const Byte* kaPlatformMacroNames[(UInt32)Platform::SEOUL_PLATFORM_COUNT] =
{
	"SEOUL_PLATFORM_WINDOWS",
	"SEOUL_PLATFORM_IOS",
	"SEOUL_PLATFORM_ANDROID",
	"SEOUL_PLATFORM_LINUX",
};

/**
 * Gets the name of the current platform, e.g. "PC"
 */
const Byte* GetCurrentPlatformName()
{
	return kaPlatformNames[(UInt32)keCurrentPlatform];
}

#if SEOUL_LOGGING_ENABLED
inline static void OpenLogFile(const String& sLogBaseName)
{
	String sFullLogFilename = GamePaths::Get()->GetLogDir();
	if (!sLogBaseName.IsEmpty())
	{
		sFullLogFilename = Path::Combine(sFullLogFilename, sLogBaseName);
	}
	else
	{
		sFullLogFilename.Append("Gamelog.txt");
	}

	Logger::GetSingleton().OpenFile(
		sFullLogFilename.CStr(),
		true);
}
#endif // /#if SEOUL_LOGGING_ENABLED

#if SEOUL_ENABLE_STACK_TRACES

#if SEOUL_PLATFORM_WINDOWS
typedef USHORT (WINAPI* CaptureStackBackTraceSignature)(
  __in       ULONG FramesToSkip,
  __in       ULONG FramesToCapture,
  __out      PVOID *BackTrace,
  __out_opt  PULONG BackTraceHash);

struct CaptureCallStackHelper
{
	CaptureCallStackHelper()
	{
		m_hKernel32Dll = LoadLibraryA("kernel32.dll");
		if (m_hKernel32Dll != nullptr)
		{
			m_pCaptureStackBackTrace = (CaptureStackBackTraceSignature)GetProcAddress(m_hKernel32Dll, "RtlCaptureStackBackTrace");
		}
		else
		{
			m_pCaptureStackBackTrace = nullptr;
		}
	}

	~CaptureCallStackHelper()
	{
		if (m_hKernel32Dll != nullptr)
		{
			SEOUL_VERIFY(FreeLibrary(m_hKernel32Dll));
		}
	}

	UInt32 GetCurrentCallStack(
		UInt32 nNumberOfEntriesToSkip,
		UInt32 nMaxNumberOfAddressesToGet,
		size_t* pOutBuffer)
	{
		if (m_pCaptureStackBackTrace == nullptr)
			return 0;

		UInt32 nFrames = (*m_pCaptureStackBackTrace)(
			nNumberOfEntriesToSkip,
			nMaxNumberOfAddressesToGet,
			(PVOID*)pOutBuffer,
			nullptr);

		if (0u == nFrames)
		{
			// On Windows Server 2003 and XP, FramesToSkip + FramesToCapture must
			// be less than 63
			nMaxNumberOfAddressesToGet = Min(nMaxNumberOfAddressesToGet, 62u - Min(62u, nNumberOfEntriesToSkip));

			nFrames = (*m_pCaptureStackBackTrace)(
						nNumberOfEntriesToSkip,
						nMaxNumberOfAddressesToGet,
						(PVOID*)pOutBuffer,
						nullptr);
		}

		return nFrames;
	}

private:
	HMODULE m_hKernel32Dll;
	CaptureStackBackTraceSignature m_pCaptureStackBackTrace;
};

#elif SEOUL_PLATFORM_ANDROID // /#if SEOUL_PLATFORM_WINDOWS

class AndroidStackTraceHelper SEOUL_SEALED
{
public:
	AndroidStackTraceHelper(size_t* pOutBuffer, Int iFrameCount, Int iFramesToSkip)
		: m_iDepth(0)
		, m_pOutBuffer(pOutBuffer)
		, m_iFrameCount(iFrameCount)
		, m_iFramesToSkip(iFramesToSkip)
	{
	}

	Bool AddFrame(size_t zFrameAddress)
	{
		Int iOffset = (m_iDepth - m_iFramesToSkip);
		++m_iDepth;

		if (nOffset >= 0 && iOffset < m_iFrameCount)
		{
			m_pOutBuffer[iOffset] = zFrameAddress;
			return true;
		}

		return false;
	}

	Int GetFrameCount() const
	{
		return Min(m_iFrameCount, Max(m_iDepth - m_iFramesToSkip, 0));
	}

private:
	Int m_iDepth;
	size_t* m_pOutBuffer;
	Int m_iFrameCount;
	Int m_iFramesToSkip;
}; // struct AndroidStackTraceHelper

_Unwind_Reason_Code InternalAndroidStackTraceFunctionHelper(_Unwind_Context* pContext, void* pUserData)
{
	AndroidStackTraceHelper& rHelper = *reinterpret_cast<AndroidStackTraceHelper*>(pUserData);
	rHelper.AddFrame(_Unwind_GetIP(pContext));
	return _URC_NO_REASON;
}

static Int InternalStaticAndroidBacktrace(size_t* pOutBuffer, Int nOutBufferCount, Int nFramesToSkip)
{
	AndroidStackTraceHelper helper(pOutBuffer, nOutBufferCount, nFramesToSkip);
	_Unwind_Backtrace(&InternalAndroidStackTraceFunctionHelper, &helper);
	return helper.GetFrameCount();
}

#endif

#endif // /SEOUL_ENABLE_STACK_TRACES

// Extra number of stack frames to skip when getting a stack trace
#if SEOUL_ENABLE_STACK_TRACES
#if SEOUL_PLATFORM_WINDOWS
static const int kExtraStackFramesToSkip = 3;
#elif SEOUL_PLATFORM_IOS
static const int kExtraStackFramesToSkip = 2;
#elif SEOUL_PLATFORM_ANDROID
static const int kExtraStackFramesToSkip = 2;
#elif SEOUL_PLATFORM_LINUX
static const int kExtraStackFramesToSkip = 2;
#else
#error "Define for your platform"
#endif
#endif // /#if SEOUL_ENABLE_STACK_TRACES

/**
 * Get the current call stack on the current platform.
 * @return The number of call stack entries returned.
 */
inline UInt32 GetCurrentCallStack(
	UInt32 nNumberOfEntriesToSkip,
	UInt32 nMaxNumberOfAddressesToGet,
	size_t* pOutBuffer)
{
#if SEOUL_ENABLE_STACK_TRACES
	nNumberOfEntriesToSkip += kExtraStackFramesToSkip;

#if SEOUL_PLATFORM_WINDOWS
	static CaptureCallStackHelper s_Helper;
	return s_Helper.GetCurrentCallStack(
		nNumberOfEntriesToSkip,
		nMaxNumberOfAddressesToGet,
		pOutBuffer);
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_LINUX
	SEOUL_STATIC_ASSERT(sizeof(size_t) == sizeof(void*));
	void** ppOut = (void**)pOutBuffer;
	UInt32 nReturnCount = backtrace(ppOut, nMaxNumberOfAddressesToGet);

	const UInt32 nNumberToSkip = Min(
		nReturnCount,
		nNumberOfEntriesToSkip);
	nReturnCount -= nNumberToSkip;

	if (nNumberToSkip > 0u)
	{
		memmove(pOutBuffer, pOutBuffer + nNumberToSkip, nReturnCount * sizeof(size_t));
	}

	return nReturnCount;
#elif SEOUL_PLATFORM_ANDROID
	Int nReturnCount = InternalStaticAndroidBacktrace(pOutBuffer, nMaxNumberOfAddressesToGet, nNumberOfEntriesToSkip);
	return (nReturnCount >= 0 ? (UInt32)nReturnCount : 0u);
#else
#error "Define for this platform."
#endif

#else // !SEOUL_ENABLE_STACK_TRACES
	return 0u;
#endif // /!SEOUL_ENABLE_STACK_TRACES
}

namespace PlatformPrint
{

/**
 * Platform-dependent implementation for sending log messages to a debug
 * window (i.e. MSVC debug console), may be a NOP on some platforms.
 */
void PrintString(Type eType, const String& sMessage)
{
	PrintString(eType, sMessage.CStr());
}

/**
 * Platform-dependent implementation for sending log messages to a debug
 * window (i.e. MSVC debug console), may be a NOP on some platforms.
 */
void PrintString(Type eType, const Byte *sMessage)
{
#if SEOUL_PLATFORM_WINDOWS || SEOUL_PLATFORM_LINUX
	fprintf(stdout, "%s\n", sMessage);
	fflush(stdout);
#elif SEOUL_PLATFORM_IOS
	IOSPrintDebugString(sMessage);
#elif SEOUL_PLATFORM_ANDROID
	auto eOutType = ANDROID_LOG_INFO;
	switch (eType)
	{
	case Type::kError: eOutType = ANDROID_LOG_ERROR; break;
	case Type::kFailure: eOutType = ANDROID_LOG_FATAL; break;
	case Type::kWarning: eOutType = ANDROID_LOG_WARN; break;

	case Type::kInfo: // fall-through
	default:
		eOutType = ANDROID_LOG_INFO;
		break;
	};

	__android_log_write(eOutType, "Seoul", sMessage);
#else
#error "Define for this platform."
#endif
}

/**
 * Platform-dependent implementation for sending printf-style formatted
 * log messages to a debug window (i.e. MSVC debug console), may be a NOP on
 * some platforms.
 */
void PrintStringFormatted(Type eType, const Byte *sFormat, ...)
{
	va_list args;
	va_start(args, sFormat);
	PrintStringFormattedV(eType, sFormat, args);
	va_end(args);
}

/**
 * Platform-dependent implementation for sending printf-style formatted
 * log messages to a debug window (i.e. MSVC debug console), may be a NOP on
 * some platforms.
 */
void PrintStringFormattedV(Type eType, const Byte *sFormat, va_list args)
{
	PrintString(eType, String::VPrintf(sFormat, args));
}

void PrintStringMultiline(Type eType, const Byte *sPrefix, const String& sIn)
{
	UInt32 uStartIndex = 0u;
	UInt32 uEndIndex = sIn.Find('\n', uStartIndex);

	// Log each entry, split on newlines.
	String const sPrefixStr(sPrefix);
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

		PrintString(eType, sPrefixStr + sMessage);

		// Find the next entry, may be finished if the log was only 1 line.
		uStartIndex = (uEndIndex == String::NPos ? sIn.GetSize() : uEndIndex + 1u);
		uEndIndex = sIn.Find('\n', uStartIndex);
	}
}

void PrintStringMultiline(Type eType, const Byte *sPrefix, const Byte *sMessage)
{
	PrintStringMultiline(eType, sPrefix, String(sMessage));
}

void PrintStringFormattedMultiline(Type eType, const Byte *sPrefix, const Byte* sFormat, ...)
{
	va_list args;
	va_start(args, sFormat);
	PrintStringFormattedVMultiline(eType, sPrefix, sFormat, args);
	va_end(args);
}

void PrintStringFormattedVMultiline(Type eType, const Byte *sPrefix, const Byte* sFormat, va_list args)
{
	PrintStringMultiline(eType, sPrefix, String::VPrintf(sFormat, args));
}

/**
 * Platform-dependent implementation for sending log messages to a debug
 * window (i.e. MSVC debug console), may be a NOP on some platforms.
 */
void PrintDebugString(Type eType, const String& sMessage)
{
	PrintDebugString(eType, sMessage.CStr());
}

/**
 * Platform-dependent implementation for sending log messages to a debug
 * window (i.e. MSVC debug console), may be a NOP on some platforms.
 */
void PrintDebugString(Type eType, const Byte *sMessage)
{
#if !SEOUL_SHIP
#if SEOUL_PLATFORM_WINDOWS
	::OutputDebugStringA(sMessage);
#elif SEOUL_PLATFORM_IOS
	IOSPrintDebugString(sMessage);
#elif SEOUL_PLATFORM_ANDROID
	auto eOutType = ANDROID_LOG_INFO;
	switch (eType)
	{
	case Type::kError: eOutType = ANDROID_LOG_ERROR; break;
	case Type::kFailure: eOutType = ANDROID_LOG_FATAL; break;
	case Type::kWarning: eOutType = ANDROID_LOG_WARN; break;

	case Type::kInfo: // fall-through
	default:
		eOutType = ANDROID_LOG_INFO;
		break;
	};

	__android_log_write(eOutType, "Seoul", sMessage);
#elif SEOUL_PLATFORM_LINUX
	// TODO: Nop - redundant with basic log channel.
#else
#error "Define for this platform."
#endif
#endif
}

/**
 * Platform-dependent implementation for sending printf-style formatted
 * log messages to a debug window (i.e. MSVC debug console), may be a NOP on
 * some platforms.
 */
void PrintDebugStringFormatted(Type eType, const Byte *sFormat, ...)
{
	va_list args;
	va_start(args, sFormat);
	PrintDebugStringFormattedV(eType, sFormat, args);
	va_end(args);
}

/**
 * Platform-dependent implementation for sending printf-style formatted
 * log messages to a debug window (i.e. MSVC debug console), may be a NOP on
 * some platforms.
 */
void PrintDebugStringFormattedV(Type eType, const Byte *sFormat, va_list args)
{
	PrintDebugString(eType, String::VPrintf(sFormat, args));
}

} // namespace PlatformPrint

/** Whether or not the current build is running automated tests. */
Bool g_bRunningAutomatedTests = false;

/**
 * Whether or not we are running unit tests
 *
 * Whether or not we are running unit tests.  This variable should not be
 * used frequently, since we generally want the same behavior while running
 * unit tests and while playing a game.  It should only be used in very
 * special cases (e.g. what to do when an assertion fails).
 */
Bool g_bRunningUnitTests = false;

/** Whether the current build is headless. This is implicitly true if g_bRunningUnitTests is set to true. */
Bool g_bHeadless = false;

void Core::Initialize(const CoreSettings& settings)
{
	// Make sure our compiler macros for endianness match up with what our
	// actual endianness is at runtime.  Exactly one of SEOUL_LITTLE_ENDIAN and
	// SEOUL_BIG_ENDIAN should be defined to 1.
	SEOUL_STATIC_ASSERT(SEOUL_LITTLE_ENDIAN ^ SEOUL_BIG_ENDIAN);
#if SEOUL_LITTLE_ENDIAN
	SEOUL_ASSERT_NO_LOG(IsSystemLittleEndian() && !IsSystemBigEndian());
#else
	SEOUL_ASSERT_NO_LOG(!IsSystemLittleEndian() && IsSystemBigEndian());
#endif

	// game path setup
	GamePaths::Initialize(settings.m_GamePathsSettings);
	SEOUL_ASSERT_NO_LOG(GamePaths::Get().IsValid());

	// Initialize the global file manager.
	FileManager::Initialize();

#if SEOUL_ENABLE_MEMORY_TOOLING
	// Set the memory leaks filename.
	String sMemoryLeaksFilename = String("MemoryLeaks_") + GetCurrentTimeString() + ".txt";

	sMemoryLeaksFilename = Path::Combine(GamePaths::Get()->GetLogDir(), sMemoryLeaksFilename);
	MemoryManager::SetMemoryLeaksFilename(sMemoryLeaksFilename);
#endif // /#if SEOUL_ENABLE_MEMORY_TOOLING

	// setup the logger
#if SEOUL_LOGGING_ENABLED
	Bool bSuccess = true;
	if (settings.m_bLoadLoggerConfigurationFile)
	{
		bSuccess = Seoul::Logger::GetSingleton().LoadConfiguration();
	}

	// open the log file
	if (settings.m_bOpenLogFile)
	{
		OpenLogFile(settings.m_sLogName);
	}

	if (!bSuccess)
	{
		SEOUL_WARN("Failed loading logger configuration file.\n");
	}
#endif

#if SEOUL_WITH_MORIARTY
	// Construct MoriartyClient singleton (the Engine is responsible for
	// connecting it to the server)
	Socket::StaticInitialize();
	SEOUL_NEW(MemoryBudgets::Developer) MoriartyClient;
#endif
}

void Core::ShutDown()
{
#if SEOUL_ENABLE_STACK_TRACES
	// Force a load completion to avoid complications on shutdown.
	IMapFile* pMapFile = GetMapFile();
	if (nullptr != pMapFile)
	{
		pMapFile->WaitUntilLoaded();
	}
#endif

#if SEOUL_WITH_MORIARTY
	// Delete MoriartyClient singleton
	SEOUL_DELETE MoriartyClient::Get();
	Socket::StaticShutdown();
#endif

	// Initialize the global file manager.
	FileManager::ShutDown();

	// game path shutdown
	SEOUL_ASSERT(GamePaths::Get().IsValid());
	GamePaths::ShutDown();
}

#if SEOUL_ENABLE_STACK_TRACES
/**
 * Global map file pointer - can be used to resolve
 * addresses captured with a stack trace to human readable names.
 */
static IMapFile* s_pMapFile = nullptr;

/**
 * Populate pOutBuffer with addresses in the stack,
 * up to rnMaxNumberOfAddressesToGet.
 *
 * \pre pOutBuffer must be non-nullptr and must point to a contiguous
 * array of type size_t large enough to hold rnMaxNumberOfAddressesToGet
 * size_t addresses.
 *
 * @param[in] nMaxNumberOfAddressesToGet The max number of addresses
 * to place in pOutBuffer.
 * @param[out] pOutBuffer The buffer to populate with addresses.
 *
 * @return The actual number of addresses in pOutBuffer.
 */
UInt32 Core::GetCurrentCallStack(
	UInt32 nNumberOfEntriesToSkip,
	UInt32 nMaxNumberOfAddressesToGet,
	size_t* pOutBuffer)
{
	return Seoul::GetCurrentCallStack(
		nNumberOfEntriesToSkip,
		nMaxNumberOfAddressesToGet,
		pOutBuffer);
}

/**
 * Gets the current stack trace as a string and writes it to sBuffer
 *
 * Gets the current stack trace, resolves addresses to function names,
 * and demangles C++ symbol names.
 *
 * @param[out] sBuffer Buffer to write the stack trace into
 * @param[in]  zSize Size of sBuffer, in bytes.  At most zSize bytes of
 *     data will be written, including the terminating null.
 */
void Core::GetStackTraceString(Byte *sBuffer, size_t zSize)
{
	size_t aCallStack[64];
	UInt32 nFrames = GetCurrentCallStack(1, SEOUL_ARRAY_COUNT(aCallStack), aCallStack);
	PrintStackTraceToBuffer(sBuffer, zSize, aCallStack, nFrames);
}

/**
 * Prints the given stack trace to the given buffer
 *
 * Takes the given stack trace, resolves addresses to function names, and
 * demangles C++ symbol names.
 *
 * @param[out] sBuffer Buffer to write the stack trace into
 * @param[in]  zSize Size of sBuffer, in bytes.  At most zSize bytes of
 *     data will be written, including the terminating null.
 * @param[in]  aCallStack Array of call stack addresses to resolve
 * @param[in]  nFrames number of call stack addresses to resolve
 */
void Core::PrintStackTraceToBuffer(
	Byte *sBuffer,
	size_t zSize,
	Byte const* sPerLinePrefix,
	const size_t *aCallStack,
	UInt32 nFrames)
{
	if (nFrames == 0)
	{
		STRNCPY(sBuffer, "<Stack trace unavailable>\n", zSize);
	}
	else
	{
		IMapFile *pMapFile = GetMapFile();
		Byte sFunctionName[256];

		if (pMapFile != nullptr)
			pMapFile->WaitUntilLoaded();

		for (UInt32 i = 0; i < nFrames; i++)
		{
			if (pMapFile != nullptr)
			{
				pMapFile->ResolveFunctionAddress(aCallStack[i], sFunctionName, sizeof(sFunctionName));
			}
			else
			{
				SNPRINTF(sFunctionName, sizeof(sFunctionName), "0x%p", (void*)aCallStack[i]);
			}

			Int nBytesWritten = SNPRINTF(sBuffer, zSize, "%s%s\n", sPerLinePrefix, sFunctionName);

			// Check for truncation (different implementations of snprintf
			// indicate this differently)
			if (nBytesWritten < 0 || (size_t)nBytesWritten > zSize)
			{
				break;
			}

			sBuffer += nBytesWritten;
			zSize -= nBytesWritten;
		}
	}
}

/**
 * @return The currently set map file, or nullptr if none is set. The map
 * file can be used to resolve addresses acquired through a stack
 * capture to human readable function names.
 */
IMapFile* Core::GetMapFile()
{
	return s_pMapFile;
}

/**
 * Update the active map file.
 */
void Core::SetMapFile(IMapFile* pMapFile)
{
	s_pMapFile = pMapFile;
}
#endif // /#if SEOUL_ENABLE_STACK_TRACES

/**
 * @return True if the current environment is 64-bits, otherwise return false.
 */
Bool IsOperatingSystem64Bits()
{
	// If a 64-bit executable, always true
#if SEOUL_PLATFORM_64
	return true;
	// Otherwise, if a Windows build, test, otherwise always false.
#elif SEOUL_PLATFORM_32

#	if SEOUL_PLATFORM_WINDOWS
		typedef BOOL (WINAPI *IsWow64ProcessFunc)(HANDLE, PBOOL);
		IsWow64ProcessFunc pFunc = (IsWow64ProcessFunc)::GetProcAddress(
			::GetModuleHandleW(L"kernel32"),
			"IsWow64Process");

		if (nullptr != pFunc)
		{
			BOOL bIsWow64 = FALSE;
			if (FALSE != pFunc(::GetCurrentProcess(), &bIsWow64))
			{
				return (FALSE != bIsWow64);
			}
		}
#	endif

	return false;
#endif // /SEOUL_PLATFORM_32
}

/** Global boolean used to check whether we've entered the main app function or not. */
Bool g_bInMain = false;

} // namespace Seoul
