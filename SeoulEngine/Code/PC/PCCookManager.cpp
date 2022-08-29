/**
 * \file PCCookManager.cpp
 * \brief Specialization of CookManager on the PC - handles cooking by
 * delegating cooking tasks to cooker applications in SeoulEngine's Tools
 * folder.
 *
 * \warning PCCookManager does not handle disabling/enabling cooking
 * for ship builds or other cases - if you want to completely disable
 * cooking, do not create a PCCookManager, conditionally create
 * a NullCookManager in these cases.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CookDatabase.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "JobsManager.h"
#include "Logger.h"
#include "Path.h"
#include "PCCookManager.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#include "SeoulProcess.h"

namespace Seoul
{

static Byte const* kCookerExeFilename = "Cooker.exe";

/**
 * Simple binder class that reroutes standard error and output from the cooking process to the log.
 */
class StandardRedirect SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(StandardRedirect);

	StandardRedirect(String* psStandardErrorEchoOut = nullptr)
		: m_psStandardErrorEchoOut(psStandardErrorEchoOut)
	{
	}

	~StandardRedirect()
	{
		// Flush any remaining output.
		Flush(m_vStandardOutput, false);
		Flush(m_vStandardError, true);
	}

	/**
	 * Should be bound to the standard output delegate of a Process - gathers output until
	 * a newline character is seen, at which point the gathered output is output to the log.
	 */
	void StandardOutput(Byte const* s)
	{
		Gather(s, m_vStandardOutput, false);
	}

	/**
	 * Should be bound to the standard error delegate of a Process - gathers output until
	 * a newline character is seen, at which point the gathered output is output to the log.
	 */
	void StandardError(Byte const* s)
	{
		Gather(s, m_vStandardError, true);
	}

private:
	/**
	 * If rvBuffer is not empty, write out its contents to the log and clear it.
	 */
	void Flush(Vector<Byte>& rvBuffer, Bool bStandardError)
	{
		if (!rvBuffer.IsEmpty())
		{
			// Null terminate the string.
			rvBuffer.PushBack('\0');
			SEOUL_LOG_COOKING("%s\n", rvBuffer.Get(0));
			if (m_psStandardErrorEchoOut)
			{
				m_psStandardErrorEchoOut->Append(rvBuffer.Get(0), rvBuffer.GetSize() - 1);
				m_psStandardErrorEchoOut->Append('\n');
			}
			rvBuffer.Clear();
		}
	}

	/**
	 * Append the characters in sInput until a null terminate is seen.
	 */
	void Gather(Byte const* sInput, Vector<Byte>& rvBuffer, Bool bStandardError)
	{
		// Keep going until we have no more string to process.
		while (sInput && '\0' != *sInput)
		{
			// Get the current character, then advance the buffer pointer.
			Byte c = *sInput++;
			switch (c)
			{
			// Newline indicates end of line by itself, so we need to flush.
			case '\n':
				Flush(rvBuffer, bStandardError);
				break;
			// Carriage return indicates end of line, and may be followed by a newline.
			case '\r':
				Flush(rvBuffer, bStandardError);
				// If the next character is a newline, just skip it - this is the Windows end-of-line convention.
				if ('\n' == *sInput)
				{
					sInput++;
				}
				break;
			default:
				// Any other characters should be appended to the buffer.
				rvBuffer.PushBack(c);
				break;
			};
		}
	}

	Vector<Byte> m_vStandardOutput;
	Vector<Byte> m_vStandardError;
	String* m_psStandardErrorEchoOut;
}; // class StandardRedirect

/**
 * Helper function that, given source sourceFilePath and
 * cooked cookedFilePath FilePaths, attempts to construct
 * absolute paths rsSourceFilename and rsCookedFilename
 * that respect the case of the source path on disk. This function should
 * always succeed, unless an os/filesystem specific reason
 * prevents the case aware file path from being constructed.
 */
static Bool InternalStaticConstructCaseAwareAbsoluteFilenameStrings(
	FilePath filePath,
	String& rsSourceFilename,
	String& rsCookedFilename)
{
	// Use the GetExactPathName() function to get the case aware
	// path of sourceFilePath.
	rsSourceFilename = Path::GetExactPathName(
		filePath.GetAbsoluteFilenameInSource());

	// Cache the string root paths of the source and cooked files - for example,
	// the Data\Content or Source\ folders, with absolute roots.
	const String& sCookedRootPath = GameDirectoryToString(filePath.GetDirectory());
	const String& sSourceRootPath = GamePaths::Get()->GetSourceDir();

	// Construct the absolute, case aware output filename path for the cooked file
	// by combining the cooked root path with the relative, case aware source path.
	rsCookedFilename = Path::Combine(
		sCookedRootPath,
		rsSourceFilename.Substring(sSourceRootPath.GetSize()));

	// Finally, replace the extension of the source file with the extension of the cooked file,
	// converted to all uppercase.
	rsCookedFilename = Path::ReplaceExtension(rsCookedFilename, FileTypeToCookedExtension(filePath.GetType()).ToLowerASCII());

	return true;
}

PCCookManager::PCCookManager()
	: m_Mutex()
	, m_pCookDatabase(SEOUL_NEW(MemoryBudgets::Cooking) CookDatabase(keCurrentPlatform, true))
	, m_CurrentlyCooking()
	, m_bIssuedMissingCookerExecutableLog(false)
{
}

PCCookManager::~PCCookManager()
{
}

void PCCookManager::GetDependents(FilePath filePath, Dependents& rv) const
{
	m_pCookDatabase->GetDependents(filePath, rv);
}

CookManager::CookResult PCCookManager::DoCook(FilePath filePath, Bool bOnlyIfNeeded)
{
	if (SupportsCooking(filePath.GetType()))
	{
		return GenericCook(filePath, bOnlyIfNeeded, kCookerExeFilename);
	}

	return kErrorCannotCookFileType;
}

CookManager::CookResult PCCookManager::GenericCook(
	FilePath filePath,
	Bool bOnlyIfNeeded,
	Byte const* sCookerExeFilename)
{
	// If requested, check the cooking database to determine if we need
	// to cook the file.
	if (bOnlyIfNeeded)
	{
		if (m_pCookDatabase->CheckUpToDate(filePath))
		{
			return kUpToDate;
		}
	}

	// If the cooker executable is not found, SEOUL_WARN about it.
	FilePath cookerFilePath = FilePath::CreateToolsBinFilePath(sCookerExeFilename);
	if (!FileManager::Get()->Exists(cookerFilePath))
	{
		// This is a SEOUL_WARN case - basically, we have a source file, need to cook
		// an output file, but have no executable.
		SEOUL_WARN("CookManager: Cooker executable %s was not found but cooking is required!\n",
			cookerFilePath.GetAbsoluteFilename().CStr());

		return kErrorMissingCookerSupport;
	}

	// Execute the cooker process.
	{
		// Set the currently cooking FilePath, then setup a scoped action that will
		// reset it back to FilePath once this scope is left.
		SynchronizedCurrentlyCookingSet(filePath);
		auto const clearCurrentlyCooking(MakeScopedAction([](){}, SEOUL_BIND_DELEGATE(&PCCookManager::ClearCurrentlyCooking, this)));

		// Arguments for the SeoulEngine Cooker.exe app - run Cooker.exe without arguments for more information.
		//
		// Start with additional arguments.
		Process::ProcessArguments vArguments;

		// Standard arguments.
		vArguments.PushBack("-cooker_version");
		vArguments.PushBack(ToString(CookDatabase::GetCookerVersion()));
		vArguments.PushBack("-data_version");
		vArguments.PushBack(ToString(CookDatabase::GetDataVersion(filePath.GetType())));
		vArguments.PushBack("-local");
		vArguments.PushBack("-out_file");
		vArguments.PushBack(filePath.GetAbsoluteFilename());
		vArguments.PushBack("-platform");
		vArguments.PushBack(GetCurrentPlatformName());

		// In non-ship builds, pass "-debug_only", which
		// affects certain cook paths (e.g. script projects).
#if !SEOUL_SHIP
		vArguments.PushBack("-debug_only");
#endif // /#if !SEOUL_SHIP

		// Start the cooking process.
		String sStandardErrorEchoOut;
		ScopedPtr<StandardRedirect> pOutputRedirect(SEOUL_NEW(MemoryBudgets::Cooking) StandardRedirect(&sStandardErrorEchoOut));
		Int32 iResult = -1;
		{
			Process cookProcess(
				cookerFilePath.GetAbsoluteFilename(),
				vArguments,
				SEOUL_BIND_DELEGATE(&StandardRedirect::StandardOutput, pOutputRedirect.Get()),
				SEOUL_BIND_DELEGATE(&StandardRedirect::StandardError, pOutputRedirect.Get()));

			if (!cookProcess.Start())
			{
				SEOUL_WARN("FAILED: %s", filePath.GetRelativeFilenameInSource().CStr());
				if (!sStandardErrorEchoOut.IsEmpty())
				{
					SEOUL_WARN("%s", sStandardErrorEchoOut.CStr());
				}

				return kErrorCookingFailed;
			}

			// Wait for the cooking process to finish.
			while (cookProcess.CheckRunning())
			{
				// Shutdown requested, kill the process.
				if (FileManager::Get()->HasNetworkFileIOShutdown())
				{
					cookProcess.Kill(1);
				}

				Jobs::Manager::Get()->YieldThreadTime();
			}

			iResult = cookProcess.GetReturnValue();
		}

		// Flush the standard error and output capture.
		pOutputRedirect.Reset();

		// If the cooking process failed, cooking failed.
		if (0 != iResult)
		{
			SEOUL_WARN("FAILED: %s", filePath.GetRelativeFilenameInSource().CStr());
			if (!sStandardErrorEchoOut.IsEmpty())
			{
				SEOUL_WARN("%s", sStandardErrorEchoOut.CStr());
			}

			return kErrorCookingFailed;
		}
	}

	return kSuccess;
}

/**
 * Utility, synchronizes access to the m_CurrentlyCooking value in
 * a way that yields time to the Jobs::Manager. Use to make cooking a
 * serialized operation while avoiding deadlocks around yields to
 * the Jobs::Manager.
 */
void PCCookManager::SynchronizedCurrentlyCookingSet(FilePath newFilePath)
{
	// Force cooking to be serialized - we handle that in this unorthodox way to
	// account for the Jobs::Manager::Get()->Yield() below, which can result in this
	// method being reentrant on the same thread.
	m_Mutex.Lock();
	volatile Bool bIsValid = m_CurrentlyCooking.IsValid();
	while (bIsValid)
	{
		m_Mutex.Unlock();
		(void)Jobs::Manager::Get()->YieldThreadTime();
		m_Mutex.Lock();
		bIsValid = m_CurrentlyCooking.IsValid();
	}

	// Set the currently cooking FilePath, then setup a scoped action that will
	// reset it back to FilePath once this scope is left.
	m_CurrentlyCooking = newFilePath;
	m_Mutex.Unlock();
}

} // namespace Seoul
