// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace SlimCSLib
{
	public static class FileUtility
	{
		#region Non-public members
		const int MAX_PATH = 260;

		[DllImport("Shlwapi.dll", CharSet = CharSet.Auto)]
		static extern int PathRelativePathTo(
			[Out] StringBuilder pszPath,
			[In] string pszFrom,
			[In] FileAttributes dwAttrFrom,
			[In] string pszTo,
			[In] FileAttributes dwAttrTo);
		#endregion

		/// <summary>
		/// Create sDirectory, including any necessary parent directories.
		/// </summary>
		public static void CreateDirectoryAll(string sDirectory)
		{
			// Sanity handling on recursion.
			if (string.IsNullOrWhiteSpace(sDirectory))
			{
				return;
			}

			// Early out for the easy case.
			if (Directory.Exists(sDirectory))
			{
				return;
			}

			// Recursively create parents.
			CreateDirectoryAll(Path.GetDirectoryName(sDirectory));

			// Now create the directory itself.
			var info = Directory.CreateDirectory(sDirectory);

			// Report failure.
			if (!info.Exists)
			{
				throw new DirectoryNotFoundException($"failed creating \"{sDirectory}\".");
			}
		}

		/// <summary>
		/// Deletes sPath and all its contents.
		/// </summary>
		/// <param name="sPath">The directory path to delete.</param>
		public static void ForceDeleteDirectory(string sPath)
		{
			// Early out.
			if (!Directory.Exists(sPath))
			{
				return;
			}

			// Exception if path is not a directory.
			if ((new FileInfo(sPath).Attributes & FileAttributes.Directory) != FileAttributes.Directory)
			{
				throw new ArgumentException($"\"{sPath}\" is not a directory.");
			}

			// First, delete all files in the destination directory.
			{
				var asFiles = Directory.GetFiles(sPath);
				foreach (var s in asFiles)
				{
					File.SetAttributes(s, FileAttributes.Normal);
					File.Delete(s);
				}
			}

			// Now recursively delete any directories in the destination directory.
			{
				var asDirectories = Directory.GetDirectories(sPath);
				foreach (var s in asDirectories)
				{
					ForceDeleteDirectory(s);
				}
			}

			// Termination condition, delete the target directory.
			Directory.Delete(sPath, false);
		}

		/// <summary>
		/// Utility that attempts to put sPath into a canonical form.
		///
		/// Path normalization is useful/necessary to resolve two
		/// potentially distinct string paths to the same file on disk.
		///
		/// The returned path is absolute when possible.
		/// </summary>
		public static string NormalizePath(string sPath)
		{
			// Uri does most of the heavy lifting of generating a canonized
			// path.
			var uri = new Uri(sPath);

			// Initial result - Path.GetFullPath() returns
			// an absolute, resolve path to files on disk.
			var sReturn = Path.GetFullPath(uri.LocalPath);

			// Remove the trailing directory separator if present,
			// to maintain consistency.
			sReturn = sReturn.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);

			return sReturn;
		}

		/// <summary>
		/// Given a directory and a path, returns the canonical relative
		/// path from that directory to the path.
		/// </summary>
		/// <param name="sDirectory">Directory to make relative to.</param>
		/// <param name="sPath">Path to resolve.</param>
		/// <returns>The relative path.</returns>
		public static string PathRelativeTo(string sDirectory, string sPath)
		{
			// Use the system utility to resolve the relative path.
			var relativePath = new StringBuilder(MAX_PATH);
			if (0 == PathRelativePathTo(
				relativePath,
				sDirectory,
				FileAttributes.Directory,
				sPath,
				FileAttributes.Normal))
			{
				throw new ArgumentException("Could not generate a relative path from \"" +
					sDirectory + "\" to \"" + sPath + "\".");
			}

			// Get result.
			var sReturn = relativePath.ToString();

			// Trim any leading .\ if it exists.
			string sPrefix = "." + Path.DirectorySeparatorChar.ToString();
			if (sReturn.StartsWith(sPrefix))
			{
				return sReturn.Substring(sPrefix.Length);
			}
			else
			{
				return sReturn;
			}
		}

		/// <summary>
		/// Return the full path of sPath with any trailing extension (i.e. ".txt") removed.
		/// </summary>
		public static string RemoveExtension(string sPath)
		{
			return Path.Combine(Path.GetDirectoryName(sPath), Path.GetFileNameWithoutExtension(sPath));
		}
	}
}
