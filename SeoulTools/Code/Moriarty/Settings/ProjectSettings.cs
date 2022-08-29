// ProjectSettings contains all project specific configuration for Moriarty - this file
// is expected to be configured globally for all users for the current development project.
// Most likely, it is checked into Perforce and downloaded with the Moriarty application.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Drawing.Design;
using System.Text;
using System.Xml.Serialization;

using Moriarty.Utilities;

namespace Moriarty.Settings
{
	public sealed class ProjectSettings : Settings
	{
		#region Private members
		int m_iPort = 0;

		string m_sDefaultUserSettingsFilename = string.Empty;
		string m_sAndroidActivityName = string.Empty;
		string m_sAndroidPackageName = string.Empty;
		string m_sExecutableNameAndroid = string.Empty;
		string m_sExecutableNameIOS = string.Empty;
		string m_sProjectName = string.Empty;
		string m_sProjectRoot = string.Empty;
		string m_sUserSettingsClassFile = string.Empty;
		string m_sUserSettingsClass = string.Empty;
		List<string> m_UserSettingsReferencedAssemblies = new List<string>();

		/// <summary>
		/// Utility structure, uniquely identifies a GameDirectory as a combination
		/// of an active platform and a game directory.
		/// </summary>
		struct GameDirectoryKey
		{
			public GameDirectoryKey(CoreLib.Platform ePlatform, CoreLib.GameDirectory eGameDirectory)
			{
				// Platform only matters for the content directory.
				if (CoreLib.GameDirectory.Content == eGameDirectory)
				{
					m_ePlatform = ePlatform;
				}
				else
				{
					m_ePlatform = CoreLib.Platform.Unknown;
				}
				m_eGameDirectory = eGameDirectory;
			}

			public CoreLib.Platform m_ePlatform;
			public CoreLib.GameDirectory m_eGameDirectory;
		}
		string m_sProjectDirectoryPath = string.Empty;
		Dictionary<GameDirectoryKey, string> m_GameDirectories = new Dictionary<GameDirectoryKey, string>();
		string m_sSourceDirectoryPath = string.Empty;

		/// <summary>
		/// Apply some normalization to directory paths so they have fully expected qualities.
		/// </summary>
		string NormalizeDirectoryPath(string s)
		{
			if (!s.EndsWith("\\"))
			{
				s = s + "\\";
			}

			return s;
		}

		/// <summary>
		/// Insert entries for all the various game directories that may be requested.
		/// </summary>
		void RefreshGameDirectories()
		{
			m_GameDirectories.Clear();

			// If we don't have a project name yet, don't set any directories
			if (string.IsNullOrEmpty(ProjectRoot))
			{
				return;
			}

			// Cache the path to the root project directory.
			m_sProjectDirectoryPath = NormalizeDirectoryPath(FileUtility.ResolveAndSimplifyPath(Path.Combine(
				Path.GetDirectoryName(Filename),
				ProjectRoot + "\\")));

			// Source directory
			m_sSourceDirectoryPath = NormalizeDirectoryPath(Path.Combine(m_sProjectDirectoryPath, "Source\\"));

			// Config directory
			m_GameDirectories.Add(
				new GameDirectoryKey(CoreLib.Platform.Unknown, CoreLib.GameDirectory.Config),
				NormalizeDirectoryPath(Path.Combine(m_sProjectDirectoryPath, "Data\\Config\\")));

			// Platform content directories
			foreach (CoreLib.Platform ePlatform in Enum.GetValues(typeof(CoreLib.Platform)))
			{
				if (ePlatform == CoreLib.Platform.Unknown)
				{
					continue;
				}

				m_GameDirectories.Add(
					new GameDirectoryKey(ePlatform, CoreLib.GameDirectory.Content),
					NormalizeDirectoryPath(Path.Combine(m_sProjectDirectoryPath, "Data\\Content" + CoreLib.PlatformUtility.ToString(ePlatform) + "\\")));
			}

			// Log directory
			m_GameDirectories.Add(
				new GameDirectoryKey(CoreLib.Platform.Unknown, CoreLib.GameDirectory.Log),
				NormalizeDirectoryPath(Path.Combine(m_sProjectDirectoryPath, "Data\\Log\\")));

            // Save directory
            m_GameDirectories.Add(
                new GameDirectoryKey(CoreLib.Platform.Unknown, CoreLib.GameDirectory.Save),
                NormalizeDirectoryPath(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + "\\"));


            // Tools directory
            string sToolsRelativePath = "SeoulTools\\Binaries\\PC\\Developer\\x64\\";

			m_GameDirectories.Add(
				new GameDirectoryKey(CoreLib.Platform.Unknown, CoreLib.GameDirectory.ToolsBin),
				NormalizeDirectoryPath(FileUtility.ResolveAndSimplifyPath(Path.Combine(
					Path.GetDirectoryName(Filename),
					sToolsRelativePath))));
		}
		#endregion

		public ProjectSettings()
		{
		}

		/// <returns>
		/// The absolute filename of the file that is associated with this ProjectSettings
		/// object.
		/// </returns>
		[XmlIgnore]
		public override string Filename
		{
			get
			{
				return base.Filename;
			}

			set
			{
				base.Filename = value;
				RefreshGameDirectories();
			}
		}

		[Category("Connection")]
		[DefaultValue(0)]
		[Description("Port number used to listen for incoming connections.")]
		public int Port { get { return m_iPort; } set { m_iPort = value; } }

		[Category("Project")]
		[DefaultValue("")]
		[Description("Filename (relative to the Moriarty project settings file) of the default user settings xml file - used if the user does not have a settings file.")]
		public string DefaultUserSettingsFilename { get { return m_sDefaultUserSettingsFilename; } set { m_sDefaultUserSettingsFilename = value; } }

		[Category("Project")]
		[DefaultValue("")]
		[Description("Activity name of the project. Used when launching the game for the Android platform. Example: com.demiurgestudios.SeoulApp.SeoulAppRoot")]
		public string AndroidActivityName { get { return m_sAndroidActivityName; } set { m_sAndroidActivityName = value; } }

		[Category("Project")]
		[DefaultValue("")]
		[Description("Package name of the project. Used when launching the game for the Android platform. Example: com.demiurgestudios.SeoulApp")]
		public string AndroidPackageName { get { return m_sAndroidPackageName; } set { m_sAndroidPackageName = value; } }

		[Category("Project")]
		[DefaultValue("")]
		[Description("Base name of the executable for the project. Used to launch the game for the Android platform.")]
		public string ExecutableNameAndroid { get { return m_sExecutableNameAndroid; } set { m_sExecutableNameAndroid = value; } }

		[Category("Project")]
		[DefaultValue("")]
		[Description("Base name of the executable for the project. Used to launch the game for the IOS platform.")]
		public string ExecutableNameIOS { get { return m_sExecutableNameIOS; } set { m_sExecutableNameIOS = value; } }

		[Category("Project")]
		[DefaultValue("")]
		[Description("Project name. Used for settings and display.")]
		public string ProjectName { get { return m_sProjectName; } set { m_sProjectName = value; } }

		[Category("Project")]
		[DefaultValue("")]
		[Description("Root folder. Used to locate the project directory relative to the Moriarty project settings file.")]
		public string ProjectRoot { get { return m_sProjectRoot; } set { m_sProjectRoot = value; } }

		[Category("UserSettings")]
		[DefaultValue("")]
		[Description("Filename (relative to the Moriarty project settings file) that contains the C# class definition for the project's UserSettings subclass.")]
		public string UserSettingsClassFile { get { return m_sUserSettingsClassFile; } set { m_sUserSettingsClassFile = value; } }

		[Category("UserSettings")]
		[DefaultValue("")]
		[Description("Typename of the UserSettings subclass to be used for this project.")]
		public string UserSettingsClass { get { return m_sUserSettingsClass; } set { m_sUserSettingsClass = value; }  }

		[Category("UserSettings")]
		[Description("List of filenames of assemblies that must be referenced to compile UserSettingsClassFile.")]
		public List<string> UserSettingsReferencedAssemblies { get { return m_UserSettingsReferencedAssemblies; } set { m_UserSettingsReferencedAssemblies = value; } }

		/// <returns>
		/// An absolute path to the ProjectDirectory described by this ProjectSettings.
		/// </returns>
		public string ProjectDirectoryPath
		{
			get
			{
				return m_sProjectDirectoryPath;
			}
		}

		/// <returns>
		/// The executable name including extension for ePlatform.
		/// </returns>
		public string GetExecutableNameWithExtension(CoreLib.Platform ePlatform)
		{
			switch (ePlatform)
			{
				case CoreLib.Platform.Android:
					return m_sExecutableNameAndroid + FileUtility.GetPlatformExecutableExtension(ePlatform);
				case CoreLib.Platform.IOS:
					return m_sExecutableNameIOS + FileUtility.GetPlatformExecutableExtension(ePlatform);
				default:
					throw new Exception("Unknown enum \"" + Enum.GetName(typeof(CoreLib.Platform), ePlatform) + ".\"");
			}
		}

		/// <returns>
		/// An absolute path to the directory described by ePlatform and eGameDirectory.
		/// </returns>
		public string GetGameDirectoryPath(
			CoreLib.Platform ePlatform,
			CoreLib.GameDirectory eGameDirectory)
		{
			string sReturn = string.Empty;
			GameDirectoryKey key = new GameDirectoryKey(ePlatform, eGameDirectory);

			m_GameDirectories.TryGetValue(key, out sReturn);

			// If the key is not in the dictionary, then sReturn gets set to
			// null, so we normalize to the empty string in that case
			return (sReturn != null ? sReturn : "");
		}

		/// <returns>
		/// An absolute path to the directory described by ePlatform and eGameDirectory,
		/// as a Source/ content path.
		/// </returns>
		public string GetGameDirectoryPathInSource(
			CoreLib.Platform ePlatform,
			CoreLib.GameDirectory eGameDirectory)
		{
			// The Source directory path is always the same.
			return m_sSourceDirectoryPath;
		}

		/// <returns>
		/// An absolute filename string to the file or directory described
		/// by ePlatform and filePath, based on this ProjectSettings,
		/// assuming the filename points at a cooked or platform independent folder.
		/// </returns>
		public string GetAbsoluteFilename(
			CoreLib.Platform ePlatform,
			FilePath filePath)
		{
			string sFilename = Path.Combine(
				GetGameDirectoryPath(ePlatform, filePath.m_eGameDirectory),
				filePath.m_sRelativePathWithoutExtension);

			if (filePath.m_eFileType != CoreLib.FileType.Unknown)
			{
				sFilename = Path.ChangeExtension(
					sFilename,
					FileUtility.FileTypeToCookedExtension(filePath.m_eFileType));
			}

			return FileUtility.ResolveAndSimplifyPath(sFilename);
		}

		/// <returns>
		/// An absolute filename string to the file or directory described
		/// by ePlatform and filePath, based on this ProjectSettings,
		/// assuming the filename points at a Source/ content folder.
		/// </returns>
		public string GetAbsoluteFilenameInSource(
			CoreLib.Platform ePlatform,
			FilePath filePath)
		{
			// The initial path is always this combine of directory, relative filename, and extension.
			string sFilename = Path.Combine(
				GetGameDirectoryPathInSource(ePlatform, filePath.m_eGameDirectory),
				filePath.m_sRelativePathWithoutExtension);

			if (filePath.m_eFileType != CoreLib.FileType.Unknown)
			{
				sFilename = Path.ChangeExtension(
					sFilename,
					FileUtility.FileTypeToSourceExtension(filePath.m_eFileType));
			}

			return FileUtility.ResolveAndSimplifyPath(sFilename);
		}
	}
} // namespace Moriarty.Settings
