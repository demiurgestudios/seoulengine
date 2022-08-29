// MoriartyManager is the global manager for all connections in the current GUI
// application. MoriartyManager handles global preferences as well as acting as
// a choke point for any operations (i.e. cooking) that cannot be safely multi-threaded
// across multiple connections.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.CSharp;
using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;

using Moriarty.Commands;
using Moriarty.Settings;
using Moriarty.Utilities;

namespace Moriarty
{
	/// <summary>
	/// Root class that owns all connections and also serves as a choke point/global manager
	/// for operations that can only occur one at a time across all Moriarty connections
	/// (cooking, file change events).
	/// </summary>
	public sealed class MoriartyManager : IDisposable
	{
		#region Private members
		ContentChangeManager m_ContentChangeManager = null;
		MoriartyConnectionManager m_ConnectionManager = null;
		CookManager m_CookManager = null;
		SettingsOwner<ProjectSettings> m_ProjectSettings = null;
		SettingsOwner<UserSettings> m_UserSettings = null;
		Logger m_Log = null;

		/// <returns>
		/// The Type that should be used for UserSettings. By default, maps
		/// to the base UserSettings type. If a project specific UserSettings definition
		/// has been provided and it successfully compiles, that Type will be used
		/// instead.
		/// </returns>
		Type GetOrCompileUserSettingsType()
		{
			// Default to the stock UserSettings type.
			Type userSettingsType = typeof(UserSettings);

			// If the user settings class and file for the project have been defined,
			// attempt to compile the file and use that type instead.
			if (!string.IsNullOrEmpty(m_ProjectSettings.Settings.UserSettingsClass) &&
				!string.IsNullOrEmpty(m_ProjectSettings.Settings.UserSettingsClassFile))
			{
				try
				{
					// Get the absolute filename to the C# code file.
					string sAbsoluteFilename = FileUtility.ResolveAndSimplifyPath(
						Path.Combine(Path.GetDirectoryName(m_ProjectSettings.Filename), m_ProjectSettings.Settings.UserSettingsClassFile));

					// Construct a compiler provider.
					using (CSharpCodeProvider provider = new CSharpCodeProvider())
					{
						// Setup to compile in memory, and add the standard assemblies, including
						// the current assembly.
						CompilerParameters parameters = new CompilerParameters()
						{
							GenerateInMemory = true,
						};
						parameters.ReferencedAssemblies.Add(Assembly.GetExecutingAssembly().Location);

						foreach (string s in m_ProjectSettings.Settings.UserSettingsReferencedAssemblies)
						{
							parameters.ReferencedAssemblies.Add(s);
						}

						// Trigger the compile.
						CompilerResults results = provider.CompileAssemblyFromFile(parameters, new string[] { sAbsoluteFilename });

						// If the compile succeeded and the type is valid, use it instead.
						if (null != results && !results.Errors.HasErrors)
						{
							Type newUserSettingsType = results.CompiledAssembly.GetType(
								m_ProjectSettings.Settings.UserSettingsClass);

							if (null != newUserSettingsType &&
								newUserSettingsType.IsSubclassOf(typeof(UserSettings)))
							{
								userSettingsType = newUserSettingsType;
							}
							// Otherwise, log that the type is not what we expect.
							else
							{
								m_Log.LogMessage("Failed compiling custom UserSettings class, type is undefined or is not a subclass of UserSettings");
							}
						}
						// If the compiler failed with errors, log the errors.
						else if (null != results)
						{
							m_Log.LogMessage("Failed compiling custom UserSettings class: ");
							foreach (CompilerError error in results.Errors)
							{
								m_Log.LogMessage("\t" + error.ToString());
							}
						}
						// Otherwise, just log that the compile failed.
						else
						{
							m_Log.LogMessage("Failed compiling custom UserSettings class.");
						}
					}
				}
				// If the compiler crashed, log the exception.
				catch (Exception e)
				{
					m_Log.LogMessage("Failed compiling custom UserSettings class: " + e.Message);
				}
			}

			return userSettingsType;
		}
		#endregion

		public MoriartyManager(string sAbsoluteProjectSettingsFilename)
		{
			// Get the project settings.
			m_ProjectSettings = new SettingsOwner<ProjectSettings>(typeof(ProjectSettings), sAbsoluteProjectSettingsFilename);

			// Now create a manager log - we need the project settings to define the target directory
			// for log files.
			m_Log = new Logger(Path.Combine(
				GetGameDirectoryPath(CoreLib.Platform.Unknown, CoreLib.GameDirectory.Log),
				"MoriartyManager.txt"));

			// User config file is stored in their local application folder, under
			// the "Moriarty" folder, under a subfolder that matches the project name.
			string sAbsoluteUserSettingsFilename = FileUtility.ResolveAndSimplifyPath(Path.Combine(
				Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
				"Demiurge Studios",
				Path.Combine(Path.Combine("Moriarty", m_ProjectSettings.Settings.ProjectName), "Settings.xml")));

			// If the user settings file does not exist, and the project defines a default, copy it to the
			// destination.
			try
			{
				// If the project defines a default user settings file and the user doesn't
				// already have a settings file, try to copy the default to the user's
				// settings file folder.
				if (!string.IsNullOrEmpty(m_ProjectSettings.Settings.DefaultUserSettingsFilename) &&
					!File.Exists(sAbsoluteUserSettingsFilename))
				{
					// Get the absolute path to the default user settings file.
					string sAbsoluteDefaultUserSettingsFilename = FileUtility.ResolveAndSimplifyPath(Path.Combine(
						Path.GetDirectoryName(m_ProjectSettings.Filename),
						m_ProjectSettings.Settings.DefaultUserSettingsFilename));

					// Make any parent directories, if necessary
					Directory.CreateDirectory(Path.GetDirectoryName(sAbsoluteUserSettingsFilename));

					// Don't overwrite - if for some reason that causes an error, it's an actual problem,
					// since we only do this step if the user's settings file does not already exists.
					File.Copy(sAbsoluteDefaultUserSettingsFilename, sAbsoluteUserSettingsFilename, false);

					// Make sure the settings file is writable
					FileInfo userSettingsFileInfo = new FileInfo(sAbsoluteUserSettingsFilename);
					userSettingsFileInfo.IsReadOnly = false;
				}
			}
			// Any errors, log and continue.
			catch (Exception e)
			{
				m_Log.LogMessage("UserSettings does not exist, but attempting to use the default UserSettings file resulted in an error: " + e.Message);
			}

			// Instantiate the user settings instance - this will either be an instance
			// of the base UserSettings class, or a project specific subclass that will
			// be compiled by GetOrCompileUserSettingsType().
			m_UserSettings = new SettingsOwner<UserSettings>(
				GetOrCompileUserSettingsType(),
				sAbsoluteUserSettingsFilename);

			// Instantiate the remaining aggregates used by MoriartyManager.
			m_ContentChangeManager = new ContentChangeManager(this);
			m_ConnectionManager = new MoriartyConnectionManager(
				this,
				m_ProjectSettings.Settings.Port);
			m_CookManager = new CookManager(this);
		}

		public void Dispose()
		{
			m_CookManager.Dispose();
			m_CookManager = null;

			m_ConnectionManager.Dispose();
			m_ConnectionManager = null;

			m_ContentChangeManager.Dispose();
			m_ContentChangeManager = null;

			m_Log.Dispose();

			m_ProjectSettings.Save();
			m_UserSettings.Save();
		}

		/// <summary>
		/// See also: ProjectSettings.GetGameDirectoryPat
		/// </summary>
		public string GetGameDirectoryPath(
			CoreLib.Platform ePlatform,
			CoreLib.GameDirectory eGameDirectory)
		{
			return ProjectSettings.Settings.GetGameDirectoryPath(ePlatform, eGameDirectory);
		}

		/// <summary>
		/// See also: ProjectSettings.GetGameDirectoryPathInSourc
		/// </summary>
		public string GetGameDirectoryPathInSource(
			CoreLib.Platform ePlatform,
			CoreLib.GameDirectory eGameDirectory)
		{
			return ProjectSettings.Settings.GetGameDirectoryPathInSource(ePlatform, eGameDirectory);
		}

		/// <summary>
		/// See also: ProjectSettings.GetAbsoluteFilenam
		/// </summary>
		public string GetAbsoluteFilename(
			CoreLib.Platform ePlatform,
			FilePath filePath)
		{
			return ProjectSettings.Settings.GetAbsoluteFilename(ePlatform, filePath);
		}

		/// <summary>
		/// See also: ProjectSettings.GetAbsoluteFilenameInSourc
		/// </summary>
		public string GetAbsoluteFilenameInSource(
			CoreLib.Platform ePlatform,
			FilePath filePath)
		{
			return ProjectSettings.Settings.GetAbsoluteFilenameInSource(ePlatform, filePath);
		}

		public MoriartyConnectionManager ConnectionManager
		{
			get
			{
				return m_ConnectionManager;
			}
		}

		public CookManager CookManager
		{
			get
			{
				return m_CookManager;
			}
		}

		public ContentChangeManager ContentChangeManager
		{
			get
			{
				return m_ContentChangeManager;
			}
		}

		/// <returns>
		/// The Logger instance that can be used for major errors or events that are
		/// not associated with a specific connection.
		/// </returns>
		public Logger Log
		{
			get
			{
				return m_Log;
			}
		}

		public SettingsOwner<ProjectSettings> ProjectSettings
		{
			get
			{
				return m_ProjectSettings;
			}
		}

		public SettingsOwner<UserSettings> UserSettings
		{
			get
			{
				return m_UserSettings;
			}
		}

		/// <returns>
		/// True if a platform supports Run, false otherwise
		/// </returns>
		public bool CanLaunch(CoreLib.Platform ePlatform)
		{
			switch (ePlatform)
			{
				case CoreLib.Platform.Android:
					return true;
				default:
					return false;
			}
		}

		/// <summary>
		/// Attempt to run the current game project based on settings.
		/// </summary>
		/// <returns>
		/// True if the target was launched successfully, false otherwise.
		/// </returns>
		public bool LaunchGame(LaunchTargetSettings settings)
		{
			m_Log.LogMessage("Starting launch of " + settings.Platform.ToString() + " target \"" + settings.Name + "\". Launch is in progress...");

			switch (settings.Platform)
			{
			case CoreLib.Platform.Android:
				try
				{
					AndroidLaunchTargetSettings androidSettings = (AndroidLaunchTargetSettings)settings;

					// Launc the APK
					AndroidUtilities.LaunchAPK(
						m_ProjectSettings.Settings.AndroidPackageName,
						m_ProjectSettings.Settings.AndroidActivityName,
						UserSettings.Settings.LaunchTargetCommandlineArgumentsList,
						UserSettings.Settings.WaitForDebugger);

					// Indicate success.
					m_Log.LogMessage("Done launching Android target \"" + settings.Name + "\". If " +
						"failed, please make sure you have installed a Developer build of the game.");

					return true;
				}
				catch (Exception e)
				{
					m_Log.LogMessage("Failed launching Android target \"" + settings.Name + "\", error: " + e.Message);
				}
				break;

			default:
				m_Log.LogMessage("Failed launching target \"" + settings.Name + "\", unsupported platform \"" + settings.Platform.ToString() + "\".");
				break;
			}

			return false;
		}

		/// <summary>
		/// Get the base name of the game's executable, based on platform.
		/// </summary>
		private string GetGameExecutableName(CoreLib.Platform ePlatform)
		{
			return ProjectSettings.Settings.GetExecutableNameWithExtension(ePlatform);
		}

		/// <summary>
		/// Gets the path to the game data folder.
		/// </summary>
		private string GetGameDataFolderPath(CoreLib.Platform ePlatform)
		{
			return FileUtility.ResolveAndSimplifyPath(Path.Combine(
				ProjectSettings.Settings.ProjectDirectoryPath,
				"Data"));
		}
	}
} // namespace Moriarty
