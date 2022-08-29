/**
 * \file SccPerforceClient.cpp
 * \brief Specialization of the abstract source control client interface
 * for Perforce source control.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#define SEOUL_ENABLE_CTLMGR 1
#include "FileManager.h"
#include "Platform.h"
#include "ReflectionUtil.h"
#include "SccPerforceClient.h"
#include "SeoulProcess.h"
#include "StringUtil.h"
#include "ToString.h"

#if SEOUL_PLATFORM_WINDOWS
#	include <Shlobj.h>
#endif // /#if SEOUL_PLATFORM_WINDOWS

namespace Seoul::Scc
{

/** Convenience. */
static const String ksEol(SEOUL_EOL);

/**
 * @return A string modifier code that can be passed as an argument to the P4 commandline
 * to set various file type modifiers on a file.
 */
static inline Byte const* ToModifierCode(FileTypeOptions::FileTypeModifier eModifier)
{
	switch (eModifier)
	{
	case FileTypeOptions::kNone: return "";
	case FileTypeOptions::kAlwaysWriteable: return "w";
	case FileTypeOptions::kArchiveTriggerRequired: return "X";
	case FileTypeOptions::kExclusiveOpen: return "l";
	case FileTypeOptions::kExecuteBit: return "x";
	case FileTypeOptions::kPreserveModificationTime: return "m";
	case FileTypeOptions::kRcsKeywordExpansion: return "k";
	case FileTypeOptions::kStoreCompressedVersionOfEachRevision: return "C";
	case FileTypeOptions::kStoreDeltasInRcsFormat: return "D";
	case FileTypeOptions::kStoreUncompressedVersionOfEachRevision: return "F";
	default:
		SEOUL_FAIL("Out-of-sync enum.");
		return "";
	};
}

/** Utility to find the P4 executable name. */
static String GetPerforcePath()
{
#if SEOUL_PLATFORM_WINDOWS
	WCHAR aBuffer[MAX_PATH] = { 0 };
	auto const aToTry =
	{
		CSIDL_PROGRAM_FILES,
		CSIDL_PROGRAM_FILESX86,
	};

	// Look for the p4 executable in a few possible paths.
	for (auto e : aToTry)
	{
		if (SUCCEEDED(::SHGetFolderPathW(nullptr, e, nullptr, SHGFP_TYPE_CURRENT, aBuffer)))
		{
			auto const s(Path::Combine(WCharTToUTF8(aBuffer), "Perforce", "p4.exe"));
			if (FileManager::Get()->Exists(s))
			{
				return s;
			}
		}
	}
#endif // /#if SEOUL_PLATFORM_WINDOWS

	return "p4";
}

PerforceClient::PerforceClient(const PerforceClientParameters& parameters)
	: m_sP4(GetPerforcePath())
	, m_Parameters(parameters)
	, m_iActiveChangelist(parameters.m_iP4Changelist)
{
}

PerforceClient::~PerforceClient()
{
}

/**
 * @return An argument directing the P4 command at a specific changelist number.
 */
void PerforceClient::GetChangelistArgument(ProcessArguments& rv) const
{
	if (m_iActiveChangelist >= 0)
	{
		rv.PushBack("-c");
		rv.PushBack(ToString(m_iActiveChangelist));
	}
}

/**
 * @return An argument adding file type options to the P4 command.
 */
void PerforceClient::GetFileTypeArgument(
	const FileTypeOptions& fileTypeOptions,
	ProcessArguments& rv) const
{
	// If modifiers are set, append a '+' to start the description of the modifiers.
	auto sType = String(EnumToString<FileTypeOptions::BaseFileType>(fileTypeOptions.m_eBaseFileType)).ToLowerASCII();
	if (fileTypeOptions.m_iModifiers != FileTypeOptions::kNone ||
		fileTypeOptions.m_eNumberOfRevisions != FileTypeOptions::kUnlimited)
	{
		sType.Append("+");
	}

	// Append modifier codes
	auto const& v = EnumOf<FileTypeOptions::FileTypeModifier>().GetValues();
	for (auto const e : v)
	{
		if (((Int)e & fileTypeOptions.m_iModifiers) == (Int)e)
		{
			sType.Append(ToModifierCode((FileTypeOptions::FileTypeModifier)e));
		}
	}

	// Append revision limit code, if revisions are limited.
	if (fileTypeOptions.m_eNumberOfRevisions != FileTypeOptions::kUnlimited)
	{
		sType.Append("S");
		sType.Append(ToString((int)fileTypeOptions.m_eNumberOfRevisions));
	}

	if (!sType.IsEmpty())
	{
		// Append the '-t' argument and the base file type.
		rv.PushBack("-t");
		rv.PushBack(sType);
	}
}

/**
 * @return A string containing arguments to the P4 command-line process that
 * specify (optionally) file input using a standard input, and (if specified at
 * construction), client workspace, username, port, and password.
 */
PerforceClient::ProcessArguments PerforceClient::GetStandardArguments(Bool bNeedsStandardInput) const
{
	ProcessArguments vsReturn;

	// If needed, tell the client to read filenames (line by line) from standard input.
	if (bNeedsStandardInput)
	{
		vsReturn.PushBack("-x");
		vsReturn.PushBack("-");
	}

	// If defined, pass a workspace name to P4.
	if (!m_Parameters.m_sP4ClientWorkspace.IsEmpty())
	{
		vsReturn.PushBack("-c");
		vsReturn.PushBack(m_Parameters.m_sP4ClientWorkspace);
	}

	// If defined, pass a user name to P4.
	if (!m_Parameters.m_sP4User.IsEmpty())
	{
		vsReturn.PushBack("-u");
		vsReturn.PushBack(m_Parameters.m_sP4User);
	}

	// If defined, pass a port (i.e. breakout:1683) to P4.
	if (!m_Parameters.m_sP4Port.IsEmpty())
	{
		vsReturn.PushBack("-p");
		vsReturn.PushBack(m_Parameters.m_sP4Port);
	}

	// If defined, pass a password to P4.
	if (!m_Parameters.m_sP4Password.IsEmpty())
	{
		vsReturn.PushBack("-P");
		vsReturn.PushBack(m_Parameters.m_sP4Password);
	}

	return vsReturn;
}

namespace
{

struct InputBinder SEOUL_SEALED
{
	SEOUL_DELEGATE_TARGET(InputBinder);

	InputBinder(IClient::FileIterator iBegin, IClient::FileIterator iEnd)
		: m_iBegin(iBegin)
		, m_iEnd(iEnd)
		, m_i(iBegin)
		, m_u(0u)
	{
	}

	IClient::FileIterator const m_iBegin;
	IClient::FileIterator const m_iEnd;
	IClient::FileIterator m_i;
	UInt32 m_u;

	Process::InputDelegate Bind()
	{
		if (m_iBegin == m_iEnd)
		{
			return Process::InputDelegate();
		}
		else
		{
			return SEOUL_BIND_DELEGATE(&InputBinder::ProduceInput, this);
		}
	}

	Bool ProduceInput(Byte* pOutput, UInt32 uBuffer, UInt32& ruOut)
	{
		// Sanity handling.
		if (0u == uBuffer)
		{
			return false;
		}

		// Output all, done.
		if (m_iEnd == m_i)
		{
			return false;
		}

		// Get current string.
		auto const& s = *m_i;
		auto const uSize = s.GetSize();

		// Check if we've reached the end, if so, output a newline
		// and return immediately.
		if (m_u == uSize)
		{
			// Terminate.
			memcpy(pOutput, ksEol.CStr(), ksEol.GetSize());
			ruOut = ksEol.GetSize();

			// Advance.
			m_u = 0u;
			++m_i;
			return true;
		}

		// Compute next chunk to copy.
		UInt32 const u = Min(uSize - m_u, uBuffer);
		if (u > 0u)
		{
			memcpy(pOutput, s.CStr() + m_u, u);
			m_u += u;
		}

		ruOut = u;
		return true;
	}
}; // struct InputBinder

struct OutputBinder SEOUL_ABSTRACT
{
	SEOUL_DELEGATE_TARGET(OutputBinder);

	OutputBinder(const IClient::ErrorOutDelegate& error)
		: m_Error(error)
		, m_bError(false)
	{
	}

	virtual ~OutputBinder()
	{
	}

	virtual IClient::ErrorOutDelegate Bind() = 0;

	IClient::ErrorOutDelegate const m_Error;
	Bool m_bError;
}; // struct OutputBinder

struct StdErrBinder SEOUL_SEALED : OutputBinder
{
	StdErrBinder(const IClient::ErrorOutDelegate& error)
		: OutputBinder(error)
	{
	}

	virtual IClient::ErrorOutDelegate Bind() SEOUL_OVERRIDE
	{
		if (!m_Error.IsValid())
		{
			return SEOUL_BIND_DELEGATE(&StdErrBinder::CheckOutput, this);
		}
		else
		{
			return SEOUL_BIND_DELEGATE(&StdErrBinder::PassThroughOutput, this);
		}
	}

	void PassThroughOutput(Byte const* s)
	{
		CheckOutput(s);

		// This is a spurious message in our use cases.
		if (nullptr == strstr(s, "file(s) not on client"))
		{
			m_Error(s);
		}
	}

	void CheckOutput(Byte const* s)
	{
		// Nop if already found an error.
		if (m_bError)
		{
			return;
		}

		for (auto const& sErr : { "use reopen" })
		{
			if (nullptr != strstr(s, sErr))
			{
				m_bError = true;
				break;
			}
		}
	}
};

struct StdOutBinder SEOUL_SEALED : OutputBinder
{
	StdOutBinder()
		: OutputBinder(IClient::ErrorOutDelegate())
	{
	}

	virtual IClient::ErrorOutDelegate Bind() SEOUL_OVERRIDE
	{
		return SEOUL_BIND_DELEGATE(&StdOutBinder::CheckOutput, this);
	}

	void CheckOutput(Byte const* s)
	{
		// Nop if already found an error.
		if (m_bError)
		{
			return;
		}

		for (auto const& sErr : { "exclusive file already opened" })
		{
			if (nullptr != strstr(s, sErr))
			{
				m_bError = true;
				break;
			}
		}
	}
};

} // namespace anonymous

/**
 * Generic function that attempts to run a P4 command.
 *
 * @param[in] sCommand - the base command, for example "add" or "sync".
 * @param[in] bNeedsChangelistArgument If true, and if an explicit changelist is set, it will be passed to this command.
 * @param[in] iBegin If defined, start of the list of files will be sent to the command via standard input.
 * @param[in] iEnd If defined, start of the list of files will be sent to the command via standard input.
 * @param[in] fileTypeOptions File type settings to apply to the operation. Possibly optional.
 * @param[out] errorOut Will be sent any notifications or errors after the command has been run.
 *
 * @return True if the command completed successfully, false otherwise. "Success" is determined based
 * on the return value from P4 - as a result, some types of failures (i.e. "open for edit failed because
 * file is not in depot) are a success, as far as the client is concerned.
 */
Bool PerforceClient::RunCommand(
	std::initializer_list<Byte const*> asCommands,
	Bool bNeedsChangelistArgument,
	FileIterator iBegin,
	FileIterator iEnd,
	const FileTypeOptions& fileTypeOptions,
	const ErrorOutDelegate& errorOut)
{
	// Need to pass files in via standard input if defined.
	Bool const bNeedsStandardInput = (iBegin != iEnd);

	// Need to add a file type argument if fileTypeOptions is defined.
	Bool const bNeedsFileType = (fileTypeOptions.HasOptions());

	// Build the commandline argument string.
	auto vsArguments = GetStandardArguments(bNeedsStandardInput);
	for (auto const sCommand : asCommands)
	{
		vsArguments.PushBack(sCommand);
	}

	// Add the changelist argument if needed.
	if (bNeedsChangelistArgument)
	{
		GetChangelistArgument(vsArguments);
	}

	// Add the file type argument if needed.
	if (bNeedsFileType)
	{
		GetFileTypeArgument(fileTypeOptions, vsArguments);
	}

	InputBinder binder(iBegin, iEnd);
	StdOutBinder outputBinder;
	StdErrBinder errorBinder(errorOut);

	// Result tracking.
	Int32 iResult = -1;
	{
		// Construct a Process to execut the p4 commandline.
		Process process(
			m_sP4,
			vsArguments,
			outputBinder.Bind(),
			errorBinder.Bind(),
			binder.Bind());

		if (!process.Start())
		{
			if (errorOut) { errorOut("P4 process failed to start, likely failed to find p4 client binary."); }
			return false;
		}

		auto const iTimeout = (m_Parameters.m_iTimeoutInSeconds < 0 ? -1 : (Int32)(m_Parameters.m_iTimeoutInSeconds * WorldTime::kSecondsToMilliseconds));
		iResult = process.WaitUntilProcessIsNotRunning(iTimeout);
	}

	// Check result.
	if (0 != iResult || outputBinder.m_bError || errorBinder.m_bError)
	{
		if (errorOut)
		{
			errorOut(String::Printf("P4 process arguments: %s", ToString(vsArguments, " ").CStr()).CStr());
		}

		if (iResult < 0)
		{
			if (errorOut) { errorOut(String::Printf("P4 process returned non-zero exit code: %d (timeout)", iResult).CStr()); }
		}
		else if (iResult > 0)
		{
			if (errorOut) { errorOut(String::Printf("P4 process returned non-zero exit code: %d", iResult).CStr()); }
		}
		else
		{
			if (outputBinder.m_bError)
			{
				if (errorOut) { errorOut("P4 process produced error string to stdout."); }
			}
			else
			{
				if (errorOut) { errorOut("P4 process produced error string to stderr."); }
			}
		}

		return false;
	}
	else
	{
		return true;
	}
}

} // namespace Seoul::Scc
