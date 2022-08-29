/**
 * \file GamePaths.h
 * \brief Global singleton reponsible for (on some platforms) discover
 * and tracking of various file paths, used for reading and writing
 * classes of files in a standard game application.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_PATHS_H
#define GAME_PATHS_H

#include "FilePath.h"
#include "FixedArray.h"
#include "SeoulString.h"
#include "SeoulTypes.h"
#include "Singleton.h"

namespace Seoul
{

// Fordward declarations
struct GamePathsSettings;

/**
 * This class provides getters & setters for the default game data folders
 *
 * This class provides getters & setters for the default game data folders.  It follows the
 * singleton pattern.
 *
 * This class needs to be initialized before the logger is initialized (and can't make use of the logger.)
 *
 * It stores (1) full paths to several data folders: config, content, log. (2) static
 * offsets from the base folder to those data folders, (3) the name of the binaries
 * folder, and (4) the location that the exe was launched from.
 */
class GamePaths SEOUL_SEALED : public Singleton<GamePaths>
{
private:
	// constructors
	GamePaths();

	// destructor
	~GamePaths();

	GamePaths& operator= (GamePaths const&) {return *this;}	// prevent assignment
	GamePaths(GamePaths const&) {} // prevent copy construction

	// initialization function which fills in all of the member variables based on the input path
	static void InternalGetBasePath(String& rsBasePath);
	void InitializeInternal(const GamePathsSettings& settings);
	// shutdown function
	void ShutDownInternal();

	Bool m_bInitialized; // initialized?

	// full path values to various game folders
	String m_sExeDir;      // full path to the exe directory
	String m_sBaseDir;     // full path to the base directory
	String m_sConfigDir;   // full path to the config directory
	String m_sContentDir;  // full path to the content directory
	String m_sSaveDir;     // full path to the platform specific save content directory
	String m_sSourceDir;   // full path to the source directory
	String m_sToolsBinDir; // full path to the tools binaries directory
	String m_sUserDir;     // full path to the user directory
	String m_sLogDir;      // full path to the log directory
	String m_sVideosDir;   // full path to the videos directory
	FixedArray<String, (UInt32)Platform::SEOUL_PLATFORM_COUNT> m_asContentDirs; // Platform specific content dirs.

	// file paths to standard application JSON files.
	FilePath m_ApplicationJsonFilePath;
	FilePath m_AudioJsonFilePath;
	FilePath m_GuiJsonFilePath;
	FilePath m_InputJsonFilePath;
	FilePath m_LogJsonFilePath;
	FilePath m_OnlineJsonFilePath;
	FilePath m_TrialJsonFilePath;
	FilePath m_UserConfigJsonFilePath;

public:
	// Must be called pre-Initialize() to have an effect - set the user config JSON filename for the current game.
	static void SetUserConfigJsonFileName(const char* sUserConfigJsonFileName);

	// Must be called pre-Initialize() to have an effect - set the relative directory to the user's
	// save folder for the current game.
	static void SetRelativeSaveDirPath(const char* sRelativeSaveDirPath);

	// singleton accessors
	static void Initialize(const GamePathsSettings& inSettings);
	static void ShutDown();

	/**
	 * @return True if GamePaths::Initialize() has been
	 * called, false otherwise.
	 */
	Bool IsInitialized() const
	{
		return m_bInitialized;
	}

	// Utility, since some platforms shared content.
	static Byte const* GetGeneratedContentDirName(Platform ePlatform);

	// access method for the base directory
	const String& GetBaseDir() const;

	// access method for the config directory
	const String& GetConfigDir() const;

	// access method for the content directory
	const String& GetContentDir() const;

	// access method for the content directory for an explicit platform.
	const String& GetContentDirForPlatform(Platform ePlatform) const;

	// access method for the save game content directory
	const String& GetSaveDir() const;

	// access method for the source directory
	const String& GetSourceDir() const;

	// access method for the tools directory
	const String& GetToolsBinDir() const;

	// access method for user directory
	const String& GetUserDir() const;

	// access method for the log directory
	const String& GetLogDir() const;

	// access method for the videos directory
	const String& GetVideosDir() const;

	// set method for the base directory
	void SetBaseDir(const String& sNewBaseDir);

	// set method for the config directory
	void SetConfigDir(const String& sNewConfigDir);

	// set method for the content directory
	void SetContentDir(const String& sNewContentDir);
	void SetContentDirForPlatform(const String& sNewContentDir, Platform ePlatform);

	// set method for the save game content directory
	void SetSaveDir(const String& sNewSaveDir);

	// set method for the source directory
	void SetSourceDir(const String& sNewSourceDir);

	// set method for the tools directory
	void SetToolsBinDir(const String& sNewToolsBinDir);

	// set method for the user directory
	void SetUserDir(const String& sNewUserDir);

	// set method for the log directory
	void SetLogDir(const String& sNewLogDir);

	// set method for the videos directory
	void SetVideosDir(const String& sNewVideosDir);

	FilePath GetApplicationJsonFilePath() const { return m_ApplicationJsonFilePath; }
	FilePath GetAudioJsonFilePath() const { return m_AudioJsonFilePath; }
	FilePath GetGuiJsonFilePath() const { return m_GuiJsonFilePath; }
	FilePath GetInputJsonFilePath() const { return m_InputJsonFilePath; }
	FilePath GetLogJsonFilePath() const { return m_LogJsonFilePath; }
	FilePath GetOnlineJsonFilePath() const { return m_OnlineJsonFilePath; }
	FilePath GetTrialJsonFilePath() const { return m_TrialJsonFilePath; }
	FilePath GetUserConfigJsonFilePath() const { return m_UserConfigJsonFilePath; }

	// static string data:
	static const char* s_sDefaultPath;		// the default path
	static const char* s_sBinaryDirName;	// the base-to-binaries path in the Seoul project
	static const char* s_sConfigDirName;	// the base-to-config path in the Seoul project
	static const char* s_sContentDirName;	// the base-to-content path in the Seoul project
	static const char* s_sSaveDirName; // the base-to-save-content path for save content
	static const char* s_sSourceDirName; // the base-to-source path in the Seoul project
	static const char* s_sToolsDirBinName;	 // the base-to-tools path in the Seoul project
	static const char* s_sUserDirName;		// the user path (%APPDATA%/whatever/...)
	static const char* s_sUserConfigJsonFileName; // The name of the json file used for config settings
	static const char* s_sLogDirName;		// the base-to-log path in the Seoul project
	static const char* s_sVideosDirName;		// the base-to-videos path in the Seoul project
	static const char* s_sStaticObjectDirName; // The name of the directory under all maps that the engine should search for static objects to load

};

} // namespace Seoul

#endif // include guard
