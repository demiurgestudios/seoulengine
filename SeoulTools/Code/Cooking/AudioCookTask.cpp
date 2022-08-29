/**
 * \file AudioCookTask.cpp
 * \brief Cooking tasks for cooking FMOD project .fbp files into runtime
 * SeoulEngine .fev files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BaseCookTask.h"
#include "Compress.h"
#include "CookDatabase.h"
#include "CookPriority.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "ICookContext.h"
#include "Logger.h"
#include "Path.h"
#include "Prereqs.h"
#include "ReflectionDefine.h"
#include "SccIClient.h"
#include "ScopedAction.h"
#include "SeoulPugiXml.h"
#include "SeoulProcess.h"
#include "SoundUtil.h"
#include "StackOrHeapArray.h"

namespace Seoul
{

namespace Cooking
{

typedef HashTable<String, Pair<String, String>, MemoryBudgets::Audio> EventFolders;
typedef Vector<FilePath, MemoryBudgets::Cooking> Files;
typedef HashTable<String, Files, MemoryBudgets::Cooking> Events;

namespace
{

struct Sorter SEOUL_SEALED
{
	Bool operator()(const FilePath& a, const FilePath& b) const
	{
		return (STRCMP_CASE_INSENSITIVE(a.CStr(), b.CStr()) < 0);
	}
};

} // namespace anonymous

static inline Bool PadToAlignment(SyncFile& r, Int64 iAlignment)
{
	Int64 iPosition = 0;
	if (!r.GetCurrentPositionIndicator(iPosition))
	{
		SEOUL_LOG_COOKING("%s: failed getting position indicator for alignment padding.", r.GetAbsoluteFilename().CStr());
		return false;
	}

	Int64 const iNewPosition = RoundUpToAlignment(iPosition, iAlignment);
	Int64 const iDiff = (iNewPosition - iPosition);
	if (iDiff <= 0)
	{
		return true;
	}

	StackOrHeapArray<Byte, 16, MemoryBudgets::Cooking> aPadding((UInt32)iDiff);
	aPadding.Fill((Byte)0);
	if (aPadding.GetSizeInBytes() != r.WriteRawData(aPadding.Data(), aPadding.GetSizeInBytes()))
	{
		SEOUL_LOG_COOKING("%s: failed writing %u bytes for alignment padding.", r.GetAbsoluteFilename().CStr(), aPadding.GetSize());
		return false;
	}

	return true;
}

class AudioCookTask SEOUL_SEALED : public BaseCookTask
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(AudioCookTask);

	AudioCookTask()
	{
	}

	virtual Bool CanCook(FilePath filePath) const SEOUL_OVERRIDE
	{
		return (filePath.GetType() == FileType::kSoundProject);
	}

	virtual Bool CookAllOutOfDateContent(ICookContext& rContext) SEOUL_OVERRIDE
	{
		ContentFiles v;
		if (!DefaultOutOfDateCook(rContext, FileType::kSoundProject, v))
		{
			return false;
		}

		return true;
	}

	virtual Int32 GetPriority() const SEOUL_OVERRIDE
	{
		return CookPriority::kAudio;
	}

	virtual Bool ValidateContentEnvironment(ICookContext& rContext) SEOUL_OVERRIDE
	{
		return (OverlappingBanksResult::kNoneOverlapping == HasOverlappingSoundBanks(rContext));
	}

private:
	SEOUL_DISABLE_COPY(AudioCookTask);

	HashTable<FilePath, FilePath, MemoryBudgets::Cooking> m_tBanksToFSPROFiles;

	virtual Bool GetSources(ICookContext& rContext, FilePath filePath, Sources& rv) const SEOUL_OVERRIDE
	{
		auto const sBase(Path::GetDirectoryName(filePath.GetAbsoluteFilenameInSource()));
		auto const sMetadata(Path::Combine(sBase, "Metadata"));

		// Starting with FMOD studio, the project output is
		// dependent on its source, as well as any .xml files
		// in a folder named "Metadata" next to the project source file.
		//
		// We also add any .wav files next to the .proj file.
		Vector<String> vsXml;
		if (!FileManager::Get()->GetDirectoryListing(
			sMetadata,
			vsXml,
			false,
			true,
			FileTypeToSourceExtension(FileType::kXml)))
		{
			SEOUL_LOG_COOKING("%s: failed enumerating Metadata directory to get FMOD project .xml sources list.",
				filePath.CStr());
			return false;
		}

		Vector<String> vsWav;
		if (!FileManager::Get()->GetDirectoryListing(
			sBase,
			vsWav,
			false,
			true,
			FileTypeToSourceExtension(FileType::kWav)))
		{
			SEOUL_LOG_COOKING("%s: failed enumerating root directory to get FMOD project .wav sources list.",
				filePath.CStr());
			return false;
		}

		rv.Reserve(vsXml.GetSize() + vsWav.GetSize() + 2u);

		// Project file.
		rv.PushBack(CookSource{ filePath, false });

		// We add the metadata path itself as a directory source.
		{
			auto dirPath = FilePath::CreateContentFilePath(sMetadata);
			dirPath.SetType(FileType::kXml);
			rv.PushBack(CookSource{ dirPath, true });
		}

		// All XML files.
		for (auto const& s : vsXml)
		{
			rv.PushBack(CookSource{ FilePath::CreateContentFilePath(s), false });
		}

		// All WAV files.
		for (auto const& s : vsWav)
		{
			rv.PushBack(CookSource{ FilePath::CreateContentFilePath(s), false });
		}

		// Also, bank files are siblings.
		Files vFiles;
		Events tEvents;
		if (GetSoundBankDependenciesOfFSPRO(rContext, filePath, vFiles, tEvents))
		{
			HashSet<FilePath, MemoryBudgets::Cooking> uniques;

			// Master banks.
			for (auto const& bank : vFiles)
			{
				if (uniques.Insert(bank).Second)
				{
					rv.PushBack(CookSource{ bank, false, false, true });
				}
			}

			// Event banks.
			for (auto const& pair : tEvents)
			{
				for (auto const& bank : pair.Second)
				{
					if (uniques.Insert(bank).Second)
					{
						rv.PushBack(CookSource{ bank, false, false, true });
					}
				}
			}
		}

		return true;
	}

	/**
	 * @return A string that can be passed to the FMOD Studio command-line utility
	 * to specify the build platform ePlatform.
	 */
	void GetFmodStudioClPlatformArgument(Platform ePlatform, ProcessArguments& rv) const
	{
		// IMPORTANT: We use the "Mobile" platform for all platforms for the following reasons:
		// - this makes it easier for audio to quickly check how large output data will be
		//   when developing on PC.

		rv.PushBack("-platforms");
		rv.PushBack("Mobile");
	}

	/**
	 * FMOD studio has some restrictions that force us to write output to a staging
	 * area, then move it into the final output location.
	 *
	 * This functions returns the staging area for the specified platform,
	 * based on the input project file.
	 */
	String GetFmodStudioStagingPath(Platform ePlatform, FilePath filePath) const
	{
		// IMPORTANT: We use the "Mobile" platform for all platforms for the following reasons:
		// - this makes it easier for audio to quickly check how large output data will be
		//   when developing on PC.

		auto const sBase(Path::GetDirectoryName(filePath.GetAbsoluteFilenameInSource()));
		return Path::Combine(sBase, "Staging", "Mobile");
	}

	/** Utility, deletes all files in the given directory. */
	Bool DeleteAll(const String& sPath) const
	{
		Vector<String> vs;
		if (!FileManager::Get()->GetDirectoryListing(
			sPath,
			vs,
			false,
			true,
			String()))
		{
			SEOUL_LOG_COOKING("Failed enumerating %s for AudioCook staging delete all.", sPath.CStr());
			return false;
		}

		for (auto const& s : vs)
		{
			if (!FileManager::Get()->Delete(s))
			{
				SEOUL_LOG_COOKING("Failed deleting %s from AudioCook staging area.", s.CStr());
				return false;
			}
		}

		return true;
	}

	/** Utility, move all audio files from the input to the output. */
	Bool MoveAllAudioOutput(const String& sIn, const String& sOut)
	{
		Vector<String> vs;
		if (!FileManager::Get()->GetDirectoryListing(
			sIn,
			vs,
			false,
			false,
			String()))
		{
			SEOUL_LOG_COOKING("Failed enumerating %s for AudioCook move from staging area.", sIn.CStr());
			return false;
		}

		// Enumerate, filter unsupported types, then attempt move.
		UInt32 const uRelative = sIn.GetSize() + (sIn.EndsWith(Path::DirectorySeparatorChar()) ? 0 : 1);
		for (auto const& s : vs)
		{
			auto const eType = ExtensionToFileType(Path::GetExtension(s));
			if (eType != FileType::kSoundBank && eType != FileType::kSoundProject)
			{
				continue;
			}

			// Generate output file.
			auto const sOutFile(Path::Combine(sOut, s.Substring(uRelative)));

			// Move to output.
			if (!FileManager::Get()->CreateDirPath(Path::GetDirectoryName(sOutFile)))
			{
				SEOUL_LOG_COOKING("Failed creating dependent directories for bank %s move of AudioCook.", sOutFile.CStr());
				return false;
			}

			// Cleanup and move into place.
			(void)FileManager::Get()->Delete(sOutFile);
			if (!FileManager::Get()->Rename(s, sOutFile))
			{
				SEOUL_LOG_COOKING("Failed moving %s from staging to output path %s as part of AudioCook.", s.CStr(), sOutFile.CStr());
				return false;
			}
		}

		return true;
	}

	/**
	 * A cooker string to execute fmod_designercl.exe to cook the input FSPRO
	 * file sInputFile to an output FEV file sOutputFile.
	 */
	ProcessArguments GetFmodStudioClArguments(ICookContext& rContext, FilePath filePath) const
	{
		// TODO: This doesn't actually respect the filename of the requested output file sOutputFile, only
		// the path, since fmod_designercl.exe does not appear to have this option. This is not a problem for us
		// in practice since we always cook from a file to a file of the same name with
		// a different extension.
		ProcessArguments v;
		v.PushBack("-build");
		GetFmodStudioClPlatformArgument(rContext.GetPlatform(), v);
		v.PushBack(filePath.GetAbsoluteFilenameInSource());
		return v;
	}

	Vector<FilePath, MemoryBudgets::Cooking> Convert(ICookContext& rContext, const Files& vFiles, const Events& tEvents) const
	{
		auto const ePlatform = rContext.GetPlatform();

		HashSet<FilePath, MemoryBudgets::Cooking> set;
		Vector<FilePath, MemoryBudgets::Cooking> v;
		v.Reserve(vFiles.GetSize());
		for (auto const& filePath : vFiles)
		{
			if (set.Insert(filePath).Second)
			{
				v.PushBack(filePath);
			}
		}
		for (auto const& evt : tEvents)
		{
			for (auto const& dep : evt.Second)
			{
				if (set.Insert(dep).Second)
				{
					v.PushBack(dep);
				}
			}
		}
		return v;
	}

	static String GetWorkspacePath(FilePath filePath)
	{
		return Path::Combine(
			Path::GetDirectoryName(filePath.GetAbsoluteFilenameInSource()),
			R"(Metadata\Workspace.xml)");
	}

	Bool HasBuiltBanksSeparateBankPerAsset(ICookContext& rContext, FilePath filePath) const
	{
		String const sWorkspacePath(GetWorkspacePath(filePath));

		pugi::xml_document root;
		pugi::xml_parse_result result = root.load_file(
			sWorkspacePath.CStr(),
			pugi::parse_default,
			pugi::encoding_utf8);

		// Check and return failure on error.
		if (result.status != pugi::status_ok)
		{
			return false;
		}

		return root.root().select_node("objects/object[@class='Workspace']/property[@name='builtBanksSeparateBankPerAsset']/value").node().text().as_bool();
	}

	virtual Bool InternalCook(ICookContext& rContext, FilePath filePath) SEOUL_OVERRIDE
	{
		// Verify that the <property name="builtBanksSeparateBankPerAsset"> is set - as of v02.01.08
		// of FMOD studio, this is a "secret" property added by FMOD at Demiurge request which
		// generates a separate .bank file for each audio file (.wav file typically). It is not recognized
		// formally by the editor and in particular, will be stripped on data updates.
		if (!HasBuiltBanksSeparateBankPerAsset(rContext, filePath))
		{
			String const sWorkspacePath(GetWorkspacePath(filePath));
			SEOUL_LOG_COOKING("%s: Workspace.xml does not contain the <property name=\"builtBanksSeparateBankPerAsset\"> property.", filePath.CStr());
			SEOUL_LOG_COOKING("This is a \"secret\" property added by FMOD at Demiurge request which");
			SEOUL_LOG_COOKING("generates a separate .bank file for each audio file, which the SeoulEngine runtime depends on.");
			SEOUL_LOG_COOKING("To resolve this error, please add the following property to the the file \"%s\", under the <object class=\"Workspace\"...> node:", sWorkspacePath.CStr());
			SEOUL_LOG_COOKING(R"(<property name="builtBanksSeparateBankPerAsset">)");
			SEOUL_LOG_COOKING(R"(	<value>true</value>)");
			SEOUL_LOG_COOKING(R"(</property>)");
			return false;
		}

		// Get dependencies.
		Files vFiles;
		Events tEvents;
		if (!GetSoundBankDependenciesOfFSPRO(rContext, filePath, vFiles, tEvents))
		{
			return false;
		}

		auto const vBankFiles(Convert(rContext, vFiles, tEvents));
		auto const sBase(Path::GetDirectoryName(filePath.GetAbsoluteFilename()));

		// Acquire the path to fmodstudiocl.exe- expected to be located in the same
		// directory as the Cooker.exe executable.
		auto const sFullPath(Path::Combine(rContext.GetToolsDirectory(), "FMODStudio\\fmodstudiocl.exe"));

		// Clean the staging area.
		auto const sStaging(GetFmodStudioStagingPath(rContext.GetPlatform(), filePath));

		// Make sure the staging area exists.
		if (!FileManager::Get()->IsDirectory(sStaging))
		{
			if (!FileManager::Get()->CreateDirPath(sStaging))
			{
				SEOUL_LOG_COOKING("%s: failed creating staging area %s for AudioCook task.", filePath.CStr(), sStaging.CStr());
				return false;
			}
		}

		// Clean the staging area.
		if (!DeleteAll(sStaging))
		{
			return false;
		}

		// Run the cook
		Bool const bReturn = RunCommandLineProcess(
			String(),
			sFullPath,
			GetFmodStudioClArguments(rContext, filePath),
			false,
			true);

		// If we were successful, finish further processing of the project:
		// - prepend the list of bank dependencies.
		if (bReturn)
		{
			// Move staging to output.
			if (!MoveAllAudioOutput(sStaging, Path::GetDirectoryName(filePath.GetAbsoluteFilename())))
			{
				return false;
			}

			// Obfuscate any .strings banks.
			for (auto const& f : vFiles)
			{
				if (SoundUtil::IsStringsBank(f))
				{
					void* p = nullptr;
					UInt32 u = 0u;
					if (!SoundUtil::ReadAllAndObfuscate(f, p, u))
					{
						SEOUL_LOG_COOKING("%s: bank dependency %s could not be read for obfuscation.",
							filePath.CStr(),
							f.CStr());
						return false;
					}

					auto const b = AtomicWriteFinalOutput(rContext, p, u, f);
					MemoryManager::Deallocate(p);
					p = nullptr;
					u = 0u;

					if (!b)
					{
						SEOUL_LOG_COOKING("%s: bank dependency %s could not be written after obsfucation.",
							filePath.CStr(),
							f.CStr());
						return false;
					}
				}
			}

			// Verify that all dependencies exist after move.
			for (auto const& f : vFiles)
			{
				if (!FileManager::Get()->Exists(f))
				{
					SEOUL_LOG_COOKING("%s: bank dependency %s does not exist after cooking.",
						filePath.CStr(),
						f.CStr());
					return false;
				}
			}
			for (auto const& dep : tEvents)
			{
				for (auto const& f : dep.Second)
				{
					if (!FileManager::Get()->Exists(f))
					{
						SEOUL_LOG_COOKING("%s: bank dependency %s does not exist after cooking.",
							filePath.CStr(),
							f.CStr());
						return false;
					}
				}
			}

			// Commit the data.
			auto const sFilename(filePath.GetAbsoluteFilename());

			StreamBuffer buffer;

			// Write the data.
			buffer.WriteLittleEndian32(vFiles.GetSize());
			for (auto const& e : vFiles) { buffer.Write(Path::GetFileName(e.GetRelativeFilename())); }
			buffer.WriteLittleEndian32(tEvents.GetSize());
			for (auto const& e : tEvents)
			{
				buffer.Write(e.First);
				buffer.WriteLittleEndian32(e.Second.GetSize());
				for (auto const& dep : e.Second)
				{
					buffer.Write(Path::GetFileName(String(dep.GetRelativeFilename())));
				}
			}

			void* p = nullptr;
			UInt32 u = 0u;
			auto const deferred(MakeDeferredAction([&]()
				{
					MemoryManager::Deallocate(p);
					p = nullptr;
					u = 0u;
				}));

			if (!ZSTDCompress(
				buffer.GetBuffer(),
				buffer.GetTotalDataSizeInBytes(),
				p,
				u))
			{
				SEOUL_LOG_COOKING("%s: failed compression output sound project data.", filePath.CStr());
				return false;
			}

			if (!AtomicWriteFinalOutput(rContext, p, u, filePath))
			{
				return false;
			}
		}

		return bReturn;
	}

	static Bool GetMasterBanksOfFSPRO(
		ICookContext& rContext,
		FilePath filePath,
		Files& rvOut)
	{
		String const sBankPath(Path::Combine(Path::GetDirectoryName(filePath.GetAbsoluteFilenameInSource()), "Metadata", "Bank"));

		// List all the bank files - in a folder named Metadata/Bank
		// relative to the sInputFile.
		Vector<String> vs;
		if (FileManager::Get()->IsDirectory(sBankPath) &&
			!FileManager::Get()->GetDirectoryListing(
			sBankPath,
			vs,
			false,
			true,
			".xml"))
		{
			SEOUL_LOG_COOKING("Failed listing .xml files for sound project dependency check.");
			return false;
		}

		// Now read the bank name from each.
		auto const sBase(Path::GetDirectoryName(filePath.GetAbsoluteFilenameInSource()));
		Files vOut;
		for (auto const& s : vs)
		{
			pugi::xml_document root;
			pugi::xml_parse_result result = root.load_file(
				s.CStr(),
				pugi::parse_default | pugi::parse_fragment | pugi::parse_ws_pcdata,
				pugi::encoding_utf8);

			// Check and return failure on error.
			if (result.status != pugi::status_ok)
			{
				SEOUL_LOG_COOKING("%s: XML parsing of %s (audio dependency) failed.",
					filePath.CStr(),
					s.CStr());
				return false;
			}

			// Troll the XML data for bank names.
			for (auto const& parent : root.children("objects"))
			{
				for (auto const& child : parent.children("object"))
				{
					auto sClassName = child.attribute("class").as_string();
					if (nullptr == sClassName)
					{
						continue;
					}

					Bool bMasterBank = (0 == strcmp(sClassName, "MasterBank"));
					// New as of v2.01.08 - no "MasterBank" class anymore, instead, a subnode:
					// <property name="isMasterBank">
					//   <value>true</value>
					// </property>
					bMasterBank = bMasterBank || child.select_node("property[@name='isMasterBank']/value").node().text().as_bool();
					// Process bank.
					if (bMasterBank || 0 == strcmp(sClassName, "Bank"))
					{
						for (auto const& prop : child.children("property"))
						{
							auto const name = prop.attribute("name");
							if (!name.empty() &&
								0 == strcmp(name.as_string(), "name") &&
								!prop.child("value").empty())
							{
								String sOut(Path::Combine(sBase, prop.child("value").text().as_string()));

								auto baseFilePath = FilePath::CreateContentFilePath(sOut);
								baseFilePath.SetType(FileType::kSoundBank);
								vOut.PushBack(baseFilePath);
								if (bMasterBank)
								{
									auto filePath = baseFilePath;

									// Also track the strings bank off master.
									filePath.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename(
										filePath.GetRelativeFilenameWithoutExtension().ToString() + ".strings"));
									vOut.PushBack(filePath);

									// And the ".assets" bank, which is the directory of the individual
									// banks generated for each asset.
									filePath = baseFilePath;
									filePath.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename(
										filePath.GetRelativeFilenameWithoutExtension().ToString() + ".assets"));
									vOut.PushBack(filePath);
								}
								goto done;
							}
						}
					}
				}
			}

			// Empty block - escape hatch from nested loop above.
		done:
			{}
		}

		// This must never be empty - if it is, there's a bug or a corrupt file.
		if (vOut.IsEmpty() && !vs.IsEmpty())
		{
			SEOUL_LOG_COOKING("%s: 0 bank dependencies found, bug or corrupt FMOD data.",
				filePath.CStr());
			return false;
		}

		rvOut.Swap(vOut);
		return true;
	}

	/**
	 * Build a lookup table of event folders (parent
	 * containers of sound events in the FMOD Studio
	 * editor).
	 */
	static Bool GetEventFolders(
		ICookContext& rContext,
		FilePath filePath,
		EventFolders& rtFolders)
	{
		String const sEventFolder(Path::Combine(Path::GetDirectoryName(filePath.GetAbsoluteFilenameInSource()), "Metadata", "EventFolder"));

		// List all the event folder files - in a folder named Metadata/EventFolder
		// relative to the sInputFile.
		Vector<String> vs;
		if (FileManager::Get()->IsDirectory(sEventFolder) &&
			!FileManager::Get()->GetDirectoryListing(
			sEventFolder,
			vs,
			false,
			false,
			".xml"))
		{
			SEOUL_LOG_COOKING("Failed listing .xml files for gathering event folders.");
			return false;
		}

		EventFolders tFolders;
		for (auto const& s : vs)
		{
			pugi::xml_document root;
			pugi::xml_parse_result result = root.load_file(
				s.CStr(),
				pugi::parse_default | pugi::parse_fragment | pugi::parse_ws_pcdata,
				pugi::encoding_utf8);

			// Check and return failure on error.
			if (result.status != pugi::status_ok)
			{
				SEOUL_LOG_COOKING("%s: XML parsing of %s (audio dependency) failed.",
					filePath.CStr(),
					s.CStr());
				return false;
			}

			// Troll the XML data for event folders.
			for (auto const& parent : root.children("objects"))
			{
				for (auto const& child : parent.children("object"))
				{
					auto sClassName = child.attribute("class").as_string();
					if (nullptr == sClassName)
					{
						continue;
					}

					// Event folder entry.
					if (0 == strcmp("EventFolder", sClassName))
					{
						// Lookup id and any parenting.
						auto const sChildId = child.attribute("id").as_string();
						auto const sChildName = child.select_node("property[@name='name']/value").node().text().as_string();
						auto const parent = child.select_node("relationship[@name='folder']");
						if (parent)
						{
							tFolders.Insert(sChildId, MakePair(sChildName, parent.node().child("destination").text().as_string()));
						}
						else
						{
							tFolders.Insert(sChildId, MakePair(sChildName, String()));
						}
					}
				}
			}
		}

		rtFolders.Swap(tFolders);
		return true;
	}

	/**
	 * Utility, stores relationships that can establish
	 * sound dependencies to be resolved before returning.
	 */
	struct EventData SEOUL_SEALED
	{
		String m_sPath;
		HashTable<String, String, MemoryBudgets::Audio> m_tEventSounds;
		HashTable<String, HashSet<String, MemoryBudgets::Audio>, MemoryBudgets::Audio> m_tMultiSounds;
		HashTable<String, FilePath, MemoryBudgets::Audio> m_tSingleSounds;

		HashSet<FilePath, MemoryBudgets::Audio> m_Deps;
	};

	/**
	 * Given an event data, resolve any dependent event sounds. Call last, once the event
	 * sound dependencies have been otherwise resolved.
	 */
	static void ResolveEvent(HashTable<String, EventData, MemoryBudgets::Audio>& rt, EventData& r)
	{
		for (auto const& e : r.m_tEventSounds)
		{
			auto pDep = rt.Find(e.Second);
			if (!pDep->m_tEventSounds.IsEmpty())
			{
				ResolveEvent(rt, *pDep);
			}

			for (auto const& filePath : pDep->m_Deps)
			{
				r.m_Deps.Insert(filePath);
			}
		}
		r.m_tEventSounds.Clear();
	};

	static Bool GetEventBanksOfFSPRO(
		ICookContext& rContext,
		FilePath filePath,
		Events& rtEvents)
	{
		// Get folders for processing.
		EventFolders tFolders;
		if (!GetEventFolders(rContext, filePath, tFolders))
		{
			SEOUL_LOG_COOKING("%s: failed gathering event folders for dependency scan", filePath.CStr());
			return false;
		}

		String const sEventFolder(Path::Combine(Path::GetDirectoryName(filePath.GetAbsoluteFilenameInSource()), "Metadata", "Event"));

		// List all the events files - in a folder named Metadata/Event
		// relative to the sInputFile.
		Vector<String> vs;
		if (FileManager::Get()->IsDirectory(sEventFolder) &&
			!FileManager::Get()->GetDirectoryListing(
			sEventFolder,
			vs,
			false,
			false,
			".xml"))
		{
			SEOUL_LOG_COOKING("Failed listing .xml event files for reverse bank lookup.");
			return false;
		}


		// Now gather asset banks and event dependency mappings.
		HashTable<String, EventData, MemoryBudgets::Audio> tEventData;
		auto const sBase(Path::GetDirectoryName(filePath.GetAbsoluteFilenameInSource()));
		for (auto const& s : vs)
		{
			pugi::xml_document root;
			pugi::xml_parse_result result = root.load_file(
				s.CStr(),
				pugi::parse_default | pugi::parse_fragment | pugi::parse_ws_pcdata,
				pugi::encoding_utf8);

			// Check and return failure on error.
			if (result.status != pugi::status_ok)
			{
				SEOUL_LOG_COOKING("%s: XML parsing of %s (audio dependency) failed.",
					filePath.CStr(),
					s.CStr());
				return false;
			}

			// Gather of bank files directory a dependency from the bank.
			String sEventId;
			EventData data;

			// Troll the XML data for sound dependencies to establish
			// event banks.
			for (auto const& parent : root.children("objects"))
			{
				for (auto const& child : parent.children("object"))
				{
					auto sClassName = child.attribute("class").as_string();
					if (nullptr == sClassName)
					{
						continue;
					}

					// Event details - gather name.
					if (0 == strcmp(sClassName, "Event"))
					{
						auto const sId = child.attribute("id").value();
						auto const sEventName = child.select_node("property[@name='name']/value").node().text().as_string();

						// Start forming the full path to the event.
						String sPath = sEventName;

						// Get the folder of the event, if it has one.
						auto folder = child.select_node("relationship[@name='folder']");
						String sFolder;
						if (folder)
						{
							sFolder = folder.node().child("destination").text().as_string();
						}

						// Iterate and prepend.
						while (!sFolder.IsEmpty())
						{
							auto pPair = tFolders.Find(sFolder);
							if (nullptr == pPair)
							{
								break;
							}

							sPath = pPair->First + "/" + sPath;
							sFolder = pPair->Second;
						}

						// Store.
						sEventId = sId;
						data.m_sPath = sPath;
					}
					// SingleSound case.
					else if (0 == strcmp(sClassName, "SingleSound"))
					{
						auto asset = child.select_node("relationship[@name='audioFile']/destination").node().text().as_string();
						data.m_tSingleSounds.Insert(
							child.attribute("id").as_string(),
							FilePath::CreateContentFilePath(Path::Combine(sBase, String(asset) + ".asset.bank")));
					}
					// EventSound case.
					else if (0 == strcmp(sClassName, "EventSound"))
					{
						auto asset = child.select_node("relationship[@name='event']/destination").node().text().as_string();
						data.m_tEventSounds.Insert(
							child.attribute("id").as_string(),
							asset);
					}
					// MultiSound case.
					else if (0 == strcmp(sClassName, "MultiSound"))
					{
						auto const e = data.m_tMultiSounds.Insert(
							child.attribute("id").as_string(),
							HashSet<String, MemoryBudgets::Audio>());
						for (auto const& node : child.select_nodes("relationship[@name='sounds']/destination"))
						{
							e.First->Second.Insert(String(node.node().text().as_string()));
						}
					}
					// SoundScatterer case.
					else if (0 == strcmp(sClassName, "SoundScatterer"))
					{
						SEOUL_LOG_COOKING("Unimplemented SoundScatterer type for event '%s'", data.m_sPath.CStr());
						return false;
					}
				}
			}

			// Now accumulate.
			auto const e = tEventData.Insert(sEventId, data);
			if (!e.Second)
			{
				SEOUL_LOG_COOKING("Event ID collision: '%s' already at path '%s', "
					"also '%s' at path '%s'",
					e.First->First.CStr(), e.First->Second.m_sPath.CStr(),
					sEventId.CStr(), data.m_sPath.CStr());
				return false;
			}
		}

		// Resolve dependencies.

		// Single sounds.
		HashTable<String, FilePath, MemoryBudgets::Audio> globalSingleSet;
		for (auto pair : tEventData)
		{
			auto& data = pair.Second;
			for (auto const& e : data.m_tSingleSounds)
			{
				data.m_Deps.Insert(e.Second);
				globalSingleSet.Insert(e.First, e.Second);
			}
		}
		// Multi sounds.
		for (auto pair : tEventData)
		{
			auto& data = pair.Second;
			for (auto const& e : data.m_tMultiSounds)
			{
				for (auto const& s : e.Second)
				{
					data.m_Deps.Insert(*globalSingleSet.Find(s));
				}
			}
		}
		// Dependent events.
		for (auto pair : tEventData)
		{
			auto& data = pair.Second;
			ResolveEvent(tEventData, data);
		}

		// Populate.
		rtEvents.Clear();
		for (auto const& pair : tEventData)
		{
			Files vFiles;
			for (auto const& e : pair.Second.m_Deps)
			{
				vFiles.PushBack(e);
			}
			rtEvents.Insert(pair.Second.m_sPath, vFiles);
		}

		return true;
	}

	/**
	 * @return A list of .bank dependencies of a source FMOD .fspro file and
	 * a mapping of events (by base name, e.g. Menu/Music *not* event:/Menu/Music)
	 * to their dependency banks).
	 */
	static Bool GetSoundBankDependenciesOfFSPRO(
		ICookContext& rContext,
		FilePath filePath,
		Files& rvFiles,
		Events& rtEvents)
	{
		Files vFiles;
		if (!GetMasterBanksOfFSPRO(rContext, filePath, vFiles)) { return false; }

		Events tEvents;
		if (!GetEventBanksOfFSPRO(rContext, filePath, tEvents)) { return false; }

		// Deterministic order.
		Sorter sorter;
		QuickSort(vFiles.Begin(), vFiles.End(), sorter);
		for (auto e : tEvents)
		{
			QuickSort(e.Second.Begin(), e.Second.End(), sorter);
		}

		rvFiles.Swap(vFiles);
		rtEvents.Swap(tEvents);
		return true;
	}

	enum class OverlappingBanksResult
	{
		kNoneOverlapping,

		kSomeOverlapping,

		kOperationFailed,
	};

	/**
	 * Utility used to verify that multiple FSPRO files do not generate sound
	 * banks which overlap - this causes headaches and runtime errors, so this
	 * allows for a proactive catch of the case during cook.
	 */
	OverlappingBanksResult HasOverlappingSoundBanks(ICookContext& rContext)
	{
		Vector<String> vs;
		if (!FileManager::Get()->GetDirectoryListing(
			GamePaths::Get()->GetSourceDir(),
			vs,
			false,
			true,
			FileTypeToSourceExtension(FileType::kSoundProject)))
		{
			SEOUL_LOG_COOKING("Failed listing %s files for detecting overlapping sound banks.",
				FileTypeToSourceExtension(FileType::kSoundProject).CStr());
			return OverlappingBanksResult::kOperationFailed;
		}

		OverlappingBanksResult eResult = OverlappingBanksResult::kNoneOverlapping;
		HashTable<FilePath, FilePath, MemoryBudgets::Cooking> tBanksToFSPROFiles;
		for (auto const& sFSPROFile : vs)
		{
			auto const filePath = FilePath::CreateContentFilePath(sFSPROFile);
			auto const sBaseDirectory = Path::GetDirectoryName(sFSPROFile);
			Files vSoundBanks;
			Events tEvents;
			if (!GetSoundBankDependenciesOfFSPRO(rContext, filePath, vSoundBanks, tEvents))
			{
				SEOUL_LOG_COOKING("Failed gathering dependencies for %s", filePath.CStr());
				return OverlappingBanksResult::kOperationFailed;
			}

			HashSet<FilePath, MemoryBudgets::Cooking> set;
			for (auto const& bankFilePath : vSoundBanks)
			{
				if (!set.Insert(bankFilePath).Second)
				{
					continue;
				}

				if (tBanksToFSPROFiles.HasValue(bankFilePath))
				{
					eResult = OverlappingBanksResult::kSomeOverlapping;
					SEOUL_LOG_COOKING("FMOD .fspro file \"%s\" has sound bank dependency "
						"\"%s\" which conflicts with FMOD .fspro file \"%s\" which also has "
						"sound bank dependency \"%s\". This will result in "
						"errors at runtime. All FMOD projects must generate sound "
						"banks which have unique file paths from the sound banks "
						"of all other FMOD projects.",
						sFSPROFile.CStr(),
						bankFilePath.CStr(),
						tBanksToFSPROFiles.Find(bankFilePath)->CStr(),
						bankFilePath.CStr());
				}
				else
				{
					SEOUL_VERIFY(tBanksToFSPROFiles.Insert(bankFilePath, filePath).Second);
				}
			}
			for (auto const& evt : tEvents)
			{
				for (auto const& bankFilePath : evt.Second)
				{
					if (!set.Insert(bankFilePath).Second)
					{
						continue;
					}

					if (tBanksToFSPROFiles.HasValue(bankFilePath))
					{
						eResult = OverlappingBanksResult::kSomeOverlapping;
						SEOUL_LOG_COOKING("FMOD .fspro file \"%s\" has sound bank dependency "
							"\"%s\" which conflicts with FMOD .fspro file \"%s\" which also has "
							"sound bank dependency \"%s\". This will result in "
							"errors at runtime. All FMOD projects must generate sound "
							"banks which have unique file paths from the sound banks "
							"of all other FMOD projects.",
							sFSPROFile.CStr(),
							bankFilePath.CStr(),
							tBanksToFSPROFiles.Find(bankFilePath)->CStr(),
							bankFilePath.CStr());
					}
					else
					{
						SEOUL_VERIFY(tBanksToFSPROFiles.Insert(bankFilePath, filePath).Second);
					}
				}
			}
		}

		// Update our cached mapping.
		m_tBanksToFSPROFiles.Swap(tBanksToFSPROFiles);
		return eResult;
	}
}; // class AudioCookTask

} // namespace Cooking

SEOUL_BEGIN_TYPE(Cooking::AudioCookTask, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Cooking::BaseCookTask)
SEOUL_END_TYPE()

} // namespace Seoul
