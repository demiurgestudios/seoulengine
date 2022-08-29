/**
 * \file Cooker.cpp
 * \brief Root instance to create to access SeoulEngine content
 * cooking facilities.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BaseCookTask.h"
#include "CookDatabase.h"
#include "Cooker.h"
#include "DataStoreParser.h"
#include "FileManager.h"
#include "ICookContext.h"
#include "JobsJob.h"
#include "GamePaths.h"
#include "JobsFunction.h"
#include "Logger.h"
#include "PackageCookConfig.h"
#include "Path.h"
#include "ReflectionDeserialize.h"
#include "ReflectionRegistry.h"
#include "ReflectionType.h"
#include "SccPerforceClient.h"
#include "SeoulTime.h"
#include "StringUtil.h"
#include "Vector.h"
#include <io.h>

namespace Seoul::Cooking
{

// TODO: Move this into FilePath?
static Bool CreateContentFilePathChecked(const String& s, FilePath& r)
{
	if (s.IsEmpty())
	{
		SEOUL_LOG_COOKING("Invalid empty FilePath.");
		return false;
	}

	// Validate extension.
	auto const sExtension(Path::GetExtension(s));
	if (FileType::kUnknown == ExtensionToFileType(sExtension))
	{
		SEOUL_LOG_COOKING("Path \"%s\" ends with invalid extension '%s'", s.CStr(), sExtension.CStr());
		return false;
	}

	// Initial conversion.
	auto const filePath(FilePath::CreateContentFilePath(s));
	if (!filePath.IsValid())
	{
		SEOUL_LOG_COOKING("Path \"%s\" conversion to SeoulEngine FilePath failed, unknown error.", s.CStr());
		return false;
	}

	// Now iterate the check string and make sure all characters are valid
	// for a Seoul file path.
	UniChar prevCh = (UniChar)0;
	auto const iBegin = filePath.GetRelativeFilenameWithoutExtension().CStr();
	auto const iEnd = filePath.GetRelativeFilenameWithoutExtension().CStr() + filePath.GetRelativeFilenameWithoutExtension().GetSizeInBytes();
	auto iBeginFile = strrchr(iBegin, (Byte)Path::kDirectorySeparatorChar);
	if (nullptr != iBeginFile) { ++iBeginFile; }

	for (auto i = iBegin; iEnd != i; ++i)
	{
		UniChar const c = *i;
		switch (c)
		{
			// Single slashes.
		case Path::kDirectorySeparatorChar:
			if (Path::kDirectorySeparatorChar == prevCh)
			{
				SEOUL_LOG_COOKING("Path \"%s\" contains a double slash.", s.CStr());
				return false;
			}
			break;

			// Only valid as first character (FMOD file paths).
		case '{':
			if (iBeginFile != i)
			{
				SEOUL_LOG_COOKING("Path \"%s\" contains '%c' but not at the start, this is invalid.", s.CStr(), c);
				return false;
			}
			break;

			// Only valid as last character (FMOD file paths).
		case '}':
			if (iEnd != i + 1 && *(i + 1) != '.')
			{
				SEOUL_LOG_COOKING("Path \"%s\" contains '%c' but not at the end, this is invalid.", s.CStr(), c);
				return false;
			}
			break;

			// Valid under limited circumstances.
		case '.':
		case '-':
		case ' ':
			// These characters are only allowed if
			// not the first character, not the
			// last character, and not before or after
			// another character of this class.
			if (iBegin == i || iBeginFile == i)
			{
				SEOUL_LOG_COOKING("Path \"%s\" starts with '%c', this is invalid.", s.CStr(), c);
				return false;
			}
			else if (iEnd == i + 1)
			{
				SEOUL_LOG_COOKING("Path \"%s\" ends with '%c', this is invalid.", s.CStr(), c);
				return false;
			}
			else
			{
				switch (prevCh)
				{
				case '.':
				case '_':
				case '-':
				case ' ':
					SEOUL_LOG_COOKING("Path \"%s\" contains sequence '%c%c', this is invalid.", s.CStr(), prevCh, c);
					return false;
				default:
					break;
				}
			}
			break;

			// Valid anywhere.
		case '_':
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':

		case 'A': case 'B': case 'C': case 'D': case 'E':
		case 'F': case 'G': case 'H': case 'I': case 'J':
		case 'K': case 'L': case 'M': case 'N': case 'O':
		case 'P': case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X': case 'Y':
		case 'Z':

		case 'a': case 'b': case 'c': case 'd': case 'e':
		case 'f': case 'g': case 'h': case 'i': case 'j':
		case 'k': case 'l': case 'm': case 'n': case 'o':
		case 'p': case 'q': case 'r': case 's': case 't':
		case 'u': case 'v': case 'w': case 'x': case 'y':
		case 'z':
			break;

			// Everything else is invalid.
		default:
			SEOUL_LOG_COOKING("Path \"%s\" contains invalid character '%c'", s.CStr(), c);
			return false;
		}

		prevCh = c;
	}

	r = filePath;
	return true;
}

typedef HashSet< FilePath, MemoryBudgets::Cooking > FilterSet;
typedef FixedArray< ICookContext::FilePaths, (UInt32)FileType::FILE_TYPE_COUNT > SourceFiles;

static Bool AmendSourceFiles(
	SourceFiles& av,
	FilterSet& set,
	String const* iBegin,
	String const* iEnd)
{
	for (auto i = iBegin; iEnd != i; ++i)
	{
		// Track s.
		auto const& s = *i;

		// Validate extension - skip unknown extensions.
		auto const sExtension(Path::GetExtension(s));
		auto const eType = ExtensionToFileType(sExtension);
		if (FileType::kUnknown == eType)
		{
			continue;
		}

		// Skip extensions that don't need cooking.
		if (!FileTypeNeedsCooking(eType))
		{
			continue;
		}

		FilePath filePath;
		if (!CreateContentFilePathChecked(s, filePath))
		{
			return false;
		}

		// Skip files already added.
		if (!set.Insert(filePath).Second)
		{
			continue;
		}

		// Add texture file types to all, otherwise add to one.
		if (IsTextureFileType(filePath.GetType()))
		{
			for (auto iType = (Int)FileType::LAST_TEXTURE_TYPE; iType >= (Int)FileType::FIRST_TEXTURE_TYPE; --iType)
			{
				filePath.SetType((FileType)iType);
				av[iType].PushBack(filePath);
			}
		}
		else
		{
			av[(UInt32)filePath.GetType()].PushBack(filePath);
		}
	}

	return true;
}

static Bool RemoveSourceFiles(
	SourceFiles& av,
	FilterSet& set,
	String const* iBegin,
	String const* iEnd)
{
	for (auto i = iBegin; iEnd != i; ++i)
	{
		// Track s.
		auto const& s = *i;

		// Validate extension - skip unknown extensions.
		auto const sExtension(Path::GetExtension(s));
		if (FileType::kUnknown == ExtensionToFileType(sExtension))
		{
			continue;
		}

		FilePath filePath;
		if (!CreateContentFilePathChecked(s, filePath))
		{
			return false;
		}

		// Skip files not in the set.
		if (!set.Erase(filePath))
		{
			continue;
		}

		// Add texture file types to all, otherwise add to one.
		if (IsTextureFileType(filePath.GetType()))
		{
			for (auto iType = (Int)FileType::LAST_TEXTURE_TYPE; iType >= (Int)FileType::FIRST_TEXTURE_TYPE; --iType)
			{
				filePath.SetType((FileType)iType);

				auto const iFind = av[iType].Find(filePath);
				if (av[iType].End() != iFind)
				{
					av[iType].Erase(iFind);
				}
			}
		}
		else
		{
			auto const eType = filePath.GetType();
			auto const iFind = av[(UInt32)eType].Find(filePath);
			if (av[(UInt32)eType].End() != iFind)
			{
				av[(UInt32)eType].Erase(iFind);
			}
		}
	}

	return true;
}

/** File used to enforce multiple exclusion between cookers. */
static const String ksCookerLockFile("CookerLock.txt");

/** Maximum amount of time we will wait for the exclusivity lock. */
static const Double kfMaxExclusivityWaitSeconds = 120.0;

/** Options for binary files managed by the cooker in source control */
static const Scc::FileTypeOptions kP4CookedFileTypeOptions(Scc::FileTypeOptions::Create(
	Scc::FileTypeOptions::kBinary,
	Scc::FileTypeOptions::kAlwaysWriteable |
	Scc::FileTypeOptions::kExclusiveOpen |
	Scc::FileTypeOptions::kPreserveModificationTime,
	Scc::FileTypeOptions::k4));
static const Scc::FileTypeOptions kP4CookedFileTypeOptionsLongLife(Scc::FileTypeOptions::Create(
	Scc::FileTypeOptions::kBinary,
	Scc::FileTypeOptions::kAlwaysWriteable |
	Scc::FileTypeOptions::kExclusiveOpen |
	Scc::FileTypeOptions::kPreserveModificationTime,
	Scc::FileTypeOptions::k128));

/**
 * Files generated by the cooker (output to Source/Generated*) are either
 * short lived (4 revisions) or long lived (128 revisions).
 */
static const Scc::FileTypeOptions kP4GeneratedFileTypeOptions(Scc::FileTypeOptions::Create(
	Scc::FileTypeOptions::kText,
	Scc::FileTypeOptions::kAlwaysWriteable |
	Scc::FileTypeOptions::kPreserveModificationTime,
	Scc::FileTypeOptions::k4));
static const Scc::FileTypeOptions kP4GeneratedFileTypeOptionsLongLife(Scc::FileTypeOptions::Create(
	Scc::FileTypeOptions::kText,
	Scc::FileTypeOptions::kAlwaysWriteable |
	Scc::FileTypeOptions::kPreserveModificationTime,
	Scc::FileTypeOptions::k128));

namespace
{

struct TaskSorter SEOUL_SEALED
{
	Bool operator()(BaseCookTask* a, BaseCookTask* b) const
	{
		return (a->GetPriority() < b->GetPriority());
	}
}; // struct TaskSorter

} // namespace anonymous

struct CookerState SEOUL_SEALED : public ICookContext
{
	typedef Vector<BaseCookTask*, MemoryBudgets::Cooking> Tasks;

	virtual void AdvanceProgress(HString type, Float fTimeInSeconds, Float fPercentage, UInt32 uActiveTasks, UInt32 uTotalTasks) SEOUL_OVERRIDE
	{
#if SEOUL_LOGGING_ENABLED
		Logger::GetSingleton().AdvanceProgress(type, fTimeInSeconds, fPercentage, uActiveTasks, uTotalTasks);
#endif
	}

	virtual Bool AmendSourceFiles(String const* iBegin, String const* iEnd) SEOUL_OVERRIDE
	{
		return Cooking::AmendSourceFiles(m_avSourceFiles, m_SourceFileFilters, iBegin, iEnd);
	}

	virtual void CompleteProgress(HString type, Float fTimeInSeconds, Bool bSuccess) SEOUL_OVERRIDE
	{
#if SEOUL_LOGGING_ENABLED
		Logger::GetSingleton().CompleteProgress(type, fTimeInSeconds, bSuccess);
#endif
	}

	virtual Bool GetCookDebugOnly() const SEOUL_OVERRIDE
	{
		return m_Settings.m_bDebugOnly;
	}

	virtual Bool GetForceCompressionDictGeneration() const SEOUL_OVERRIDE
	{
		return m_Settings.m_bForceGenCdict;
	}

	virtual CookDatabase& GetDatabase() SEOUL_OVERRIDE
	{
		return *m_pCookDatabase;
	}

	virtual CheckedPtr<PackageCookConfig> GetPackageCookConfig() const SEOUL_OVERRIDE
	{
		return m_pPackageCookConfig.Get();
	}

	virtual Platform GetPlatform() const SEOUL_OVERRIDE
	{
		return m_Settings.m_ePlatform;
	}

	virtual Scc::IClient& GetSourceControlClient() SEOUL_OVERRIDE
	{
		return *m_pSourceControlClient;
	}

	virtual const Scc::FileTypeOptions& GetSourceControlFileTypeOptions(
		Bool bNeedsExclusiveLock /* = true */,
		Bool bLongLife /*= false*/) const SEOUL_OVERRIDE
	{
		return (bNeedsExclusiveLock
			? (bLongLife ? kP4CookedFileTypeOptionsLongLife : kP4CookedFileTypeOptions)
			: (bLongLife ? kP4GeneratedFileTypeOptionsLongLife : kP4GeneratedFileTypeOptions));
	}

	virtual const FilePaths& GetSourceFilesOfType(FileType eType) const SEOUL_OVERRIDE
	{
		return m_avSourceFiles[(UInt32)eType];
	}

	virtual const String& GetToolsDirectory() const SEOUL_OVERRIDE
	{
		return m_sToolsDirectory;
	}

	virtual Bool RemoveSourceFiles(String const* iBegin, String const* iEnd) SEOUL_OVERRIDE
	{
		return Cooking::RemoveSourceFiles(m_avSourceFiles, m_SourceFileFilters, iBegin, iEnd);
	}

	CookerState(const CookerSettings& settings)
		: m_Settings(settings)
		, m_sToolsDirectory(Path::GetProcessDirectory())
	{
	}

	~CookerState()
	{
		SafeDeleteVector(m_vTasks);
		m_pCookDatabase.Reset();
		m_pSourceControlClient.Reset();
		m_pPackageCookConfig.Reset();

		// Cleanup the exclusivity lock.
		if (m_pLockFile.IsValid())
		{
			auto const sFile = m_pLockFile->GetAbsoluteFilename();
			m_pLockFile.Reset();

			(void)FileManager::Get()->Delete(sFile);
		}
	}

	CookerSettings const m_Settings;
	String const m_sToolsDirectory;
	ScopedPtr<SyncFile> m_pLockFile;
	ScopedPtr<PackageCookConfig> m_pPackageCookConfig;
	ScopedPtr<Scc::IClient> m_pSourceControlClient;
	ScopedPtr<CookDatabase> m_pCookDatabase;
	SourceFiles m_avSourceFiles;
	FilterSet m_SourceFileFilters;
	Tasks m_vTasks;
}; // struct CookerState

class CookerConstructJob SEOUL_SEALED : public Jobs::Job
{
public:
	CookerConstructJob(const CookerSettings& settings)
		: m_Settings(settings)
		, m_pState()
		, m_bCancel(false)
	{
	}

	~CookerConstructJob()
	{
	}

	Bool AcquireResults(ScopedPtr<CookerState>& rp)
	{
		if (IsJobRunning())
		{
			return false;
		}

		if (Jobs::State::kError == GetJobState())
		{
			return false;
		}

		if (m_bCancel)
		{
			return false;
		}

		if (!m_pState.IsValid())
		{
			return false;
		}

		rp.Swap(m_pState);
		return true;
	}

	void Cancel()
	{
		m_bCancel = true;
	}

private:
	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE
	{
		ScopedPtr<SyncFile> pLockFile;
		ScopedPtr<PackageCookConfig> pPackageCookConfig;
		ScopedPtr<Scc::IClient> pSourceControlClient;
		ScopedPtr<CookDatabase> pCookDatabase;
		SourceFiles avSourceFiles;
		FilterSet sourceFileFilters;
		CookerState::Tasks vTasks;

		if (m_bCancel || !AcquireLock(pLockFile))
		{
			reNextState = Jobs::State::kError;
			return;
		}

		if (m_bCancel || !ReadPackageCookConfig(pPackageCookConfig))
		{
			reNextState = Jobs::State::kError;
			return;
		}

		if (m_bCancel || !CreateSourceControlClient(pSourceControlClient))
		{
			reNextState = Jobs::State::kError;
			return;
		}

		if (m_bCancel || !CreateDatabase(pCookDatabase))
		{
			reNextState = Jobs::State::kError;
			return;
		}

		if (m_bCancel || !GatherSourceFiles(avSourceFiles, sourceFileFilters))
		{
			reNextState = Jobs::State::kError;
			return;
		}

		// Do this last for two reasons - one, we don't want
		// to allow tasks to exist until we've verified
		// the other two staps. Second, once vTasks has entry,
		// it must be cleaned up manually (SafeDeleteVector(v))
		// or it will leak, unless it is passed along to m_pState.
		if (m_bCancel || !GatherTasks(vTasks))
		{
			reNextState = Jobs::State::kError;
			return;
		}

		m_pState.Reset(SEOUL_NEW(MemoryBudgets::Cooking) CookerState(m_Settings));
		m_pState->m_pLockFile.Swap(pLockFile);
		m_pState->m_pPackageCookConfig.Swap(pPackageCookConfig);
		m_pState->m_pSourceControlClient.Swap(pSourceControlClient);
		m_pState->m_pCookDatabase.Swap(pCookDatabase);
		m_pState->m_avSourceFiles.Swap(avSourceFiles);
		m_pState->m_SourceFileFilters.Swap(sourceFileFilters);
		m_pState->m_vTasks.Swap(vTasks);

		reNextState = Jobs::State::kComplete;
	}

	/**
	 * Acquires the cooker exclusion lock. Provides multiple exclusion
	 * between multipl cookers.
	 */
	Bool AcquireLock(ScopedPtr<SyncFile>& rp) const
	{
		auto const sPath = Path::Combine(GamePaths::Get()->GetSourceDir(), ksCookerLockFile);

		// Make sure the output path exists for the lock file.
		auto const sLockDir(Path::GetDirectoryName(sPath));
		if (!FileManager::Get()->IsDirectory(sLockDir) &&
			!FileManager::Get()->CreateDirPath(sLockDir))
		{
			SEOUL_LOG_COOKING("Failed creating directory for cooker lock file: %s", sPath.CStr());
			return false;
		}

		auto const iStart = SeoulTime::GetGameTimeInTicks();

		Bool bDone = false;
		int iSleepCount = 0;
		while (!bDone)
		{
			// Early out if cancelled.
			if (m_bCancel)
			{
				return false;
			}

			auto const fElapsed = SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStart);
			if (fElapsed > kfMaxExclusivityWaitSeconds)
			{
				SEOUL_LOG_COOKING("Exclusivity lock failed after %f seconds.", fElapsed);
				return false;
			}

			ScopedPtr<SyncFile> pLockFile;
			if (FileManager::Get()->OpenFile(sPath, File::kWriteTruncate, pLockFile) &&
				pLockFile->CanWrite())
			{
				rp.Swap(pLockFile);
				return true;
			}

			Thread::Sleep(1000);
			if (iSleepCount % 5 == 0)
			{
				SEOUL_LOG_COOKING("Waiting for another Cooker to finish... (%f seconds)", fElapsed);
			}
			++iSleepCount;
		}

		return false;
	}

	/**
	 * Instantiate the source control client (or a null client if not defined).
	 */
	Bool CreateSourceControlClient(ScopedPtr<Scc::IClient>& rp) const
	{
		if (m_Settings.m_P4Parameters.IsValid())
		{
			rp.Reset(SEOUL_NEW(MemoryBudgets::Cooking) Scc::PerforceClient(m_Settings.m_P4Parameters));
		}
		else
		{
			rp.Reset(SEOUL_NEW(MemoryBudgets::Cooking) Scc::NullClient);
		}

		return true;
	}

	/**
	 * Instantiate the cooking database, used to check whether files are out
	 * of date or not.
	 */
	Bool CreateDatabase(ScopedPtr<CookDatabase>& rp) const
	{
		// No processing of one-to-one version data (which is global
		// and will trigger a re-fetch/cook of all one-to-one files
		// identified out-of-date, which we don't want to trigger
		// on single file processing).
		auto const bSingleFile = m_Settings.m_SingleCookPath.IsValid();
		rp.Reset(SEOUL_NEW(MemoryBudgets::Cooking) CookDatabase(m_Settings.m_ePlatform, !bSingleFile));
		return true;
	}

	/**
	 * Find all source files to use for this cooking session.
	 */
	Bool GatherSourceFiles(SourceFiles& rav, FilterSet& rFilters) const
	{
		SourceFiles av;
		FilterSet set;

		// Early out if single file cooking.
		if (m_Settings.m_SingleCookPath.IsValid())
		{
			rav.Swap(av);
			rFilters.Swap(set);
			return true;
		}

		Vector<String> vs;
		if (!FileManager::Get()->GetDirectoryListing(
			GamePaths::Get()->GetSourceDir(),
			vs,
			false,
			true))
		{
			SEOUL_LOG_COOKING("Failed enumerating source directory \"%s\".", GamePaths::Get()->GetSourceDir().CStr());
			return false;
		}

		// Now enumerate and accumulate.
		if (!AmendSourceFiles(av, set, vs.Begin(), vs.End()))
		{
			return false;
		}

		// Done.
		rav.Swap(av);
		rFilters.Swap(set);
		return true;
	}

	/**
	 * Find all cooking tasks and gather them into a sorted vector.
	 */
	Bool GatherTasks(CookerState::Tasks& rv) const
	{
		using namespace Reflection;

		CookerState::Tasks v;
		auto const& parentType = TypeOf<BaseCookTask>();
		auto const u = Registry::GetRegistry().GetTypeCount();
		for (UInt32 i = 0u; i < u; ++i)
		{
			// Early out if cancelled.
			if (m_bCancel)
			{
				SafeDeleteVector(v);
				return false;
			}

			// Get the type, early out if it does not inherit from BaseCookTask.
			auto const& type = *Registry::GetRegistry().GetType(i);
			if (!type.IsSubclassOf(parentType))
			{
				continue;
			}

			// Attempt to instantiate an instance of the task.
			auto pTask = type.New<BaseCookTask>(MemoryBudgets::Cooking);
			if (nullptr == pTask)
			{
				SEOUL_LOG_COOKING("Failed instantiating cook task of type \"%s\".", type.GetName().CStr());
				SafeDeleteVector(v);
				return false;
			}

			// Add the task.
			v.PushBack(pTask);
		}

		// Sort tasks.
		TaskSorter sorter;
		QuickSort(v.Begin(), v.End(), sorter);

		// Done.
		rv.Swap(v);
		return true;
	}

	Bool ReadPackageCookConfig(ScopedPtr<PackageCookConfig>& rp)
	{
		// Early out in the simple case.
		if (m_Settings.m_sPackageCookConfig.IsEmpty())
		{
			return true;
		}

		void* p = nullptr;
		UInt32 u = 0u;
		if (!FileManager::Get()->ReadAll(m_Settings.m_sPackageCookConfig, p, u, 0u, MemoryBudgets::Cooking))
		{
			SEOUL_LOG_COOKING("Failed reading package cooker config file \"%s\".", m_Settings.m_sPackageCookConfig.CStr());
			return false;
		}

		DataStore dataStore;
		if (!DataStoreParser::FromString(
			(Byte const*)p,
			u,
			dataStore,
			DataStoreParserFlags::kLogParseErrors))
		{
			SEOUL_LOG_COOKING("Parse error reading package cooker config file \"%s\".", m_Settings.m_sPackageCookConfig.CStr());
			MemoryManager::Deallocate(p);
			return false;
		}

		MemoryManager::Deallocate(p);
		p = nullptr;

		ScopedPtr<PackageCookConfig> pConfig(SEOUL_NEW(MemoryBudgets::Cooking) PackageCookConfig(m_Settings.m_sPackageCookConfig));
		Reflection::DefaultSerializeContext context(ContentKey(), dataStore, dataStore.GetRootNode(), TypeId<PackageCookConfig>());
		context.SetUserData(&m_Settings);
		if (!Seoul::Reflection::DeserializeObject(context, dataStore, dataStore.GetRootNode(), pConfig.Get()))
		{
			SEOUL_LOG_COOKING("Deserialization error on package cooker config file \"%s\".", m_Settings.m_sPackageCookConfig.CStr());
			return false;
		}

		rp.Swap(pConfig);
		return true;
	}

	CookerSettings const m_Settings;
	ScopedPtr<CookerState> m_pState;
	Atomic32Value<Bool> m_bCancel;

	SEOUL_DISABLE_COPY(CookerConstructJob);
	SEOUL_REFERENCE_COUNTED_SUBCLASS(CookerConstructJob);
}; // class CookerConstructJob

Cooker::Cooker(const CookerSettings& settings)
	: m_Settings(settings)
	, m_pConstructJob(SEOUL_NEW(MemoryBudgets::Cooking) CookerConstructJob(settings))
	, m_pState()
{
	m_pConstructJob->StartJob();
}

Cooker::~Cooker()
{
}

Bool Cooker::CookAllOutOfDateContent()
{
	// Cooker must have finished its asynchronous setup.
	if (!FinishConstruct())
	{
		return false;
	}

	// Validate content directory setting.
	if (!ValidateContentDir())
	{
		return false;
	}

	// Cooking environment must be satisfied.
	if (!ValidateContentEnvironment())
	{
		return false;
	}

	// Now we can run cooking tasks.
	for (auto p : m_pState->m_vTasks)
	{
		if (!p->CookAllOutOfDateContent(*m_pState))
		{
			return false;
		}
	}

	return true;
}

Bool Cooker::CookSingle()
{
	// Cooker must have finished its asynchronous setup.
	if (!FinishConstruct())
	{
		return false;
	}

	// Validate content directory setting.
	if (!ValidateContentDir())
	{
		return false;
	}

	// Now we can run cooking tasks. Stop at the
	// first task which successfully cooks the single file.
	// If none cook the file, return failure.
	for (auto p : m_pState->m_vTasks)
	{
		// Found the task to handle this cook.
		if (p->CanCook(m_Settings.m_SingleCookPath))
		{
			return p->CookSingle(*m_pState, m_Settings.m_SingleCookPath);
		}
	}

	// No successful cook, done with failure.
	SEOUL_LOG_COOKING("CookSingle: no cook task can to handle %s", m_Settings.m_SingleCookPath.GetAbsoluteFilename().CStr());
	return false;
}

Bool Cooker::FinishConstruct()
{
	if (m_pConstructJob.IsValid())
	{
		if (m_pConstructJob->IsJobRunning())
		{
			m_pConstructJob->WaitUntilJobIsNotRunning();
		}

		if (!m_pConstructJob->AcquireResults(m_pState))
		{
			m_pState.Reset();
		}
		m_pConstructJob.Reset();
	}

	return m_pState.IsValid();
}

Bool Cooker::ValidateContentDir() const
{
	Bool const bReturn = (GamePaths::Get()->GetContentDirForPlatform(m_Settings.m_ePlatform) == GamePaths::Get()->GetContentDir());
	if (!bReturn)
	{
		SEOUL_LOG_COOKING("Content directory %s is not the valid content directory for platform %s, it should "
			"be %s.",
			GamePaths::Get()->GetContentDir().CStr(),
			EnumToString<Platform>(m_Settings.m_ePlatform),
			GamePaths::Get()->GetContentDirForPlatform(m_Settings.m_ePlatform).CStr());
	}

	return bReturn;
}

Bool Cooker::ValidateContentEnvironment()
{
	auto const& v = m_pState->m_vTasks;
	for (auto p : v)
	{
		if (!p->ValidateContentEnvironment(*m_pState))
		{
			return false;
		}
	}

	return true;
}

} // namespace Seoul::Cooking
