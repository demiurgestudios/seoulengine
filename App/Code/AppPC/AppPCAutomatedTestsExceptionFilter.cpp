/**
 * \file AppPCAutomatedTestsExceptionFilter.cpp
 * \brief Defines a utility for print a stack trace when an
 * unhandled x64 exception occurs.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Platform.h"
#include "Prereqs.h"
#include <ImageHlp.h>

namespace Seoul
{

#if SEOUL_AUTO_TESTS || SEOUL_UNIT_TESTS

/** Crash message buffer - don't want to allocate memory, and don't want a buffer this big on the stack. */
static Byte aCrashReasonBuffer[4096];

Int AutomatedTestsExceptionFilter(
	DWORD uExceptionCode,
	LPEXCEPTION_POINTERS pExceptionInfo)
{
#if SEOUL_PLATFORM_64
	DWORD const uMachineType = IMAGE_FILE_MACHINE_AMD64;
#else
	DWORD const uMachineType = IMAGE_FILE_MACHINE_I386;
#endif

	CONTEXT* pContext = pExceptionInfo->ContextRecord;

	size_t aCallStack[64];
	memset(aCallStack, 0, sizeof(aCallStack));

	STACKFRAME frame;
	memset(&frame, 0, sizeof(frame));
#if SEOUL_PLATFORM_64
    frame.AddrPC.Offset = pContext->Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Offset = pContext->Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = pContext->Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
#else
    frame.AddrPC.Offset = pContext->Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Offset = pContext->Esp;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = pContext->Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
#endif

	UInt32 uFrameOffset = 0u;
	while (uFrameOffset < SEOUL_ARRAY_COUNT(aCallStack) &&
		FALSE != ::StackWalk(
		uMachineType,
		::GetCurrentProcess(),
		::GetCurrentThread(),
		&frame,
		pContext,
		nullptr,
		::SymFunctionTableAccess,
		::SymGetModuleBase,
		nullptr))
	{
		aCallStack[uFrameOffset++] = (size_t)frame.AddrPC.Offset;
	}

	if (uFrameOffset > 0u)
	{
#if SEOUL_ENABLE_STACK_TRACES
		memset(aCrashReasonBuffer, 0, sizeof(aCrashReasonBuffer));
		Core::PrintStackTraceToBuffer(
			aCrashReasonBuffer,
			sizeof(aCrashReasonBuffer),
			"Crash: ",
			aCallStack,
			uFrameOffset);

		fprintf(
			stderr,
			"\nCrash:\nCrash: Unhandled x64 Exception (likely null pointer dereference or heap corruption)\n%s\n\n",
			aCrashReasonBuffer);
#else
		fprintf(
			stderr,
			"\nCrash:\nCrash: Unhandled x64 Exception (likely null pointer dereference or heap corruption)\n");
#endif
	}

	// Execute the __except handler
	return EXCEPTION_EXECUTE_HANDLER;
}

#endif // /#if SEOUL_AUTO_TESTS || SEOUL_UNIT_TESTS

} // namespace Seoul
