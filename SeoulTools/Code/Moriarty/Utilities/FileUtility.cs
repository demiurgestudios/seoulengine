// Miscellaneous methods for working with file paths and file types.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace Moriarty.Utilities
{
	public static class FileUtility
	{
		/// <returns>
		/// The CoreLib.FileType that corresponds to a file extensions sExtension,
		/// or kUnknown if the extension is not recognized.
		///
		/// sExtension must be an all lowercase, cleaned (no enclosing white space)
		/// extension with a leading '.'
		/// </returns>
		public static CoreLib.FileType ExtensionToFileType(string sExtension)
		{
			switch (sExtension)
			{
				case ".avi": return CoreLib.FileType.Video;
				case ".bank": return CoreLib.FileType.SoundBank;
				case ".cs": return CoreLib.FileType.Cs;
				case ".csp": return CoreLib.FileType.ScriptProject;
				case ".csproj": return CoreLib.FileType.ScriptProject;
				case ".csv": return CoreLib.FileType.Csv;
				case ".dat": return CoreLib.FileType.SaveGame;
				case ".exe": return CoreLib.FileType.Exe;
				case ".fbx": return CoreLib.FileType.SceneAsset;
				case ".fcn": return CoreLib.FileType.UIMovie;
				case ".fspro": return CoreLib.FileType.SoundProject;
				case ".fev": return CoreLib.FileType.SoundProject;
				case ".fx": return CoreLib.FileType.Effect;
				case ".fxb": return CoreLib.FileType.FxBank;
				case ".fxc": return CoreLib.FileType.Effect;
				case ".fxh": return CoreLib.FileType.EffectHeader;
				case ".fxh_marker": return CoreLib.FileType.EffectHeader;
				case ".html": return CoreLib.FileType.Html;
				case ".json": return CoreLib.FileType.Json;
				case ".lbc": return CoreLib.FileType.Script;
				case ".lua": return CoreLib.FileType.Script;
				case ".pb": return CoreLib.FileType.Protobuf;
				case ".pem": return CoreLib.FileType.PEMCertificate;
				case ".png": return CoreLib.FileType.Texture0;
				case ".prefab": return CoreLib.FileType.ScenePrefab;
				case ".proto": return CoreLib.FileType.Protobuf;
				case ".saf": return CoreLib.FileType.Animation2D;
				case ".sff": return CoreLib.FileType.Font;
				case ".sif0": return CoreLib.FileType.Texture0;
				case ".sif1": return CoreLib.FileType.Texture1;
				case ".sif2": return CoreLib.FileType.Texture2;
				case ".sif3": return CoreLib.FileType.Texture3;
				case ".sif4": return CoreLib.FileType.Texture4;
				case ".ssa": return CoreLib.FileType.SceneAsset;
				case ".son": return CoreLib.FileType.Animation2D;
				case ".spf": return CoreLib.FileType.ScenePrefab;
				case ".swf": return CoreLib.FileType.UIMovie;
				case ".ttf": return CoreLib.FileType.Font;
				case ".txt": return CoreLib.FileType.Text;
				case ".xfx": return CoreLib.FileType.FxBank;
				case ".wav": return CoreLib.FileType.Wav;
				case ".xml": return CoreLib.FileType.Xml;
				default:
					return CoreLib.FileType.Unknown;
			}
		}

		/// <returns>
		/// eType as a file extension string for files in a project's
		/// Content*\ folder.
		///
		/// Must be kept in-sync with the equivalent function in SeoulEngine's
		/// FilePath.h
		/// </returns>
		public static string FileTypeToCookedExtension(CoreLib.FileType eType)
		{
			switch (eType)
			{
				case CoreLib.FileType.Unknown: return string.Empty;
				case CoreLib.FileType.Animation2D: return ".saf";
				case CoreLib.FileType.Cs: return ".cs";
				case CoreLib.FileType.Csv: return ".csv";
				case CoreLib.FileType.Effect: return ".fxc";
				case CoreLib.FileType.EffectHeader: return ".fxh_marker";
				case CoreLib.FileType.Exe: return ".exe";
				case CoreLib.FileType.Font: return ".sff";
				case CoreLib.FileType.FxBank: return ".fxb";
				case CoreLib.FileType.Html: return ".html";
				case CoreLib.FileType.Json: return ".json";
				case CoreLib.FileType.PEMCertificate: return ".pem";
				case CoreLib.FileType.Protobuf: return ".pb";
				case CoreLib.FileType.SaveGame: return ".dat";
				case CoreLib.FileType.SceneAsset: return ".ssa";
				case CoreLib.FileType.ScenePrefab: return ".spf";
				case CoreLib.FileType.Script: return ".lbc";
				case CoreLib.FileType.ScriptProject: return ".csp";
				case CoreLib.FileType.SoundBank: return ".bank";
				case CoreLib.FileType.SoundProject: return ".fev";
				case CoreLib.FileType.Texture0: return ".sif0";
				case CoreLib.FileType.Texture1: return ".sif1";
				case CoreLib.FileType.Texture2: return ".sif2";
				case CoreLib.FileType.Texture3: return ".sif3";
				case CoreLib.FileType.Texture4: return ".sif4";
				case CoreLib.FileType.Text: return ".txt";
				case CoreLib.FileType.UIMovie: return ".fcn";
				case CoreLib.FileType.Video: return ".avi";
				case CoreLib.FileType.Wav: return ".wav";
				case CoreLib.FileType.Xml: return ".xml";
				default:
					return string.Empty;
			}
		}

		/// <returns>
		/// eType as a file extension string for files in a project's
		/// Source\ folder.
		///
		/// Must be kept in-sync with the equivalent function in SeoulEngine's
		/// FilePath.h
		/// </returns>
		public static string FileTypeToSourceExtension(CoreLib.FileType eType)
		{
			switch (eType)
			{
				case CoreLib.FileType.Unknown: return string.Empty;
				case CoreLib.FileType.Animation2D: return ".son";
				case CoreLib.FileType.Cs: return ".cs";
				case CoreLib.FileType.Csv: return ".csv";
				case CoreLib.FileType.Effect: return ".fx";
				case CoreLib.FileType.EffectHeader: return ".fxh";
				case CoreLib.FileType.Exe: return ".exe";
				case CoreLib.FileType.Font: return ".ttf";
				case CoreLib.FileType.FxBank: return ".xfx";
				case CoreLib.FileType.Html: return ".html";
				case CoreLib.FileType.Json: return ".json";
				case CoreLib.FileType.PEMCertificate: return ".pem";
				case CoreLib.FileType.Protobuf: return ".proto";
				case CoreLib.FileType.SaveGame: return ".dat";
				case CoreLib.FileType.ScenePrefab: return ".prefab";
				case CoreLib.FileType.Script: return ".lua";
				case CoreLib.FileType.ScriptProject: return ".csproj";
				case CoreLib.FileType.SoundBank: return string.Empty;
				case CoreLib.FileType.SoundProject: return ".fspro";
				case CoreLib.FileType.Text: return ".txt";
				case CoreLib.FileType.Texture0: return ".png";
				case CoreLib.FileType.Texture1: return ".png";
				case CoreLib.FileType.Texture2: return ".png";
				case CoreLib.FileType.Texture3: return ".png";
				case CoreLib.FileType.Texture4: return ".png";
				case CoreLib.FileType.UIMovie: return ".swf";
				case CoreLib.FileType.Video: return ".avi";
				case CoreLib.FileType.Wav: return ".wav";
				case CoreLib.FileType.Xml: return ".xml";
				default:
					return string.Empty;
			}
		}

		/// <summary>
		/// Gets the extension type used for executables on the given platform
		/// (e.g. ".exe" on Windows)
		/// </summary>
		public static string GetPlatformExecutableExtension(CoreLib.Platform ePlatform)
		{
			switch (ePlatform)
			{
			case CoreLib.Platform.Android: return ".apk";
			case CoreLib.Platform.IOS: return ".ipa";
			default: throw new ArgumentException("Invalid platform");
			}
		}

		/// <returns>
		/// The full path of sPath with any trailing extension (i.e. ".txt") removed.
		/// </returns>
		public static string RemoveExtension(string sPath)
		{
			return Path.Combine(Path.GetDirectoryName(sPath), Path.GetFileNameWithoutExtension(sPath));
		}

		/// <returns>
		/// The path sPath simplified to its most direct form. For example,
		/// D:\\Foo\\Bar\\..\\Foo\\ would be converted to D:\\Foo\\Foo\\ .
		///
		/// If sPath is a relative path, it will be converted to an absolute path,
		/// using the current working directory.
		/// </returns>
		public static string ResolveAndSimplifyPath(string sPath)
		{
			// For sanity sake, we absorb exceptions on Path.GetFullPath() here,
			// as it can throw a number of exceptions for malformed paths.
			try
			{
				return Path.GetFullPath(sPath);
			}
			catch (Exception)
			{
				// If the full path failed, return the original unmodified path.
				return sPath;
			}
		}
	}
} // namespace Moriarty.Utilities
