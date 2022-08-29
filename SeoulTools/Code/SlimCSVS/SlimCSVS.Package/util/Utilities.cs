// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace SlimCSVS.Package.util
{
	public static class Utilities
	{
		#region Private members
		const int MAX_PATH = 260;

		[DllImport("Shlwapi.dll", CharSet = CharSet.Auto)]
		static extern int PathRelativePathTo(
			[Out] StringBuilder pszPath,
			[In] string pszFrom,
			[In] FileAttributes dwAttrFrom,
			[In] string pszTo,
			[In] FileAttributes dwAttrTo);

		/// <summary>
		/// Internal utility for endian swapping a 64-bit float.
		/// </summary>
		[StructLayout(LayoutKind.Explicit, Size = 8)]
		struct DoubleEndianSwapUtil
		{
			[FieldOffset(0)]
			public UInt64 m_uUInt64;
			[FieldOffset(0)]
			public Double m_fDouble;
		}

		/// <summary>
		/// Internal utility for endian swapping a 32-bit float.
		/// </summary>
		[StructLayout(LayoutKind.Explicit, Size = 4)]
		struct SingleEndianSwapUtil
		{
			[FieldOffset(0)]
			public UInt32 m_uUInt32;
			[FieldOffset(0)]
			public Single m_fSingle;
		}
		#endregion

		/// <summary>
		/// Performs an endian swap on a 16-bit unsigned integer value.
		/// </summary>
		public static UInt16 EndianSwap(UInt16 v)
		{
			return (UInt16)(
				((v & 0xFFu) << 8) |
				((v & 0xFF00u) >> 8));
		}

		/// <summary>
		/// Performs an endian swap on a 32-bit floating point value.
		/// </summary>
		public static Single EndianSwap(Single f)
		{
			SingleEndianSwapUtil util = new SingleEndianSwapUtil()
			{
				m_fSingle = f
			};

			util.m_uUInt32 = EndianSwap(util.m_uUInt32);

			return util.m_fSingle;
		}

		/// <summary>
		/// Performs an endian swap on a 32-bit unsigned integer value.
		/// </summary>
		public static UInt32 EndianSwap(UInt32 v)
		{
			return
				((v & 0x000000FFu) << 24) |
				((v & 0x0000FF00u) << 8) |
				((v & 0x00FF0000u) >> 8) |
				((v & 0xFF000000u) >> 24);
		}

		/// <summary>
		/// Performs an endian swap on a 32-bit floating point value.
		/// </summary>
		public static Double EndianSwap(Double f)
		{
			DoubleEndianSwapUtil util = new DoubleEndianSwapUtil()
			{
				m_fDouble = f
			};

			util.m_uUInt64 = EndianSwap(util.m_uUInt64);

			return util.m_fDouble;
		}

		/// <summary>
		/// Performs an endian swap on a 64-bit unsigned integer value.
		/// </summary>
		public static UInt64 EndianSwap(UInt64 v)
		{
			return
				((v & 0x00000000000000FFul) << 56) |
				((v & 0x000000000000FF00ul) << 40) |
				((v & 0x0000000000FF0000ul) << 24) |
				((v & 0x00000000FF000000ul) << 8) |
				((v & 0x000000FF00000000ul) >> 8) |
				((v & 0x0000FF0000000000ul) >> 24) |
				((v & 0x00FF000000000000ul) >> 40) |
				((v & 0xFF00000000000000ul) >> 56);
		}

		/// <summary>
		/// Mix iMixin into an existing rolling iHash.
		/// </summary>
		public static int IncrementalHash(int iHash, int iMixin)
		{
			return (int)(iHash ^ (iMixin + 0x9e3779b9 + (iHash << 6) + (iHash >> 2)));
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

		// Import of the "natural" sorting function used in Windows explorer.
		[DllImport("shlwapi.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
		public static extern int StrCmpLogicalW(string x, string y);
	}
}
