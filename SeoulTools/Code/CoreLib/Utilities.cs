// Static class with miscellaneous utilities and functionality.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;

namespace CoreLib
{
	// Consistent callback used to report errors from cooker utility functions.
	public delegate void DebugMessageWriteLine(string sMessageLine);

	/// <summary>
	/// Internal helper class - used by PerforceClient (in particular) to wrap
	/// output from a Process execute when capturing standard output and standard error.
	/// </summary>
	public sealed class MessageCapture
	{
		#region Non-public members
		DebugMessageWriteLine m_Messages;
		#endregion

		public MessageCapture(DebugMessageWriteLine messages)
		{
			m_Messages = messages;
		}

		/// <summary>
		/// Delegate to bind to the standard output and error overrides.
		/// </summary>
		public void ReceiveMessage(object sender, DataReceivedEventArgs args)
		{
			if (null != m_Messages)
			{
				lock (m_Messages)
				{
					if (!string.IsNullOrEmpty(args.Data))
					{
						m_Messages(args.Data);
					}
				}
			}
		}
	} // class MessageCapture

	public static class Utilities
	{
		#region Non-public members
		const int MAX_PATH = 260;
		const string ksSeoulEngineLeadingDotSlash = "./";
		const string ksSeoulEngineContentFilePathPrefix = "content://";
		const string ksWindowsLeadingDotSlash = ".\\";

		#region SeoulUtil.dll bindings
		const string kSeoulUtilDLL = "SeoulUtil.dll";
		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern bool Seoul_LZ4Compress(
			[In] IntPtr pInputData,
			[In] UInt32 zInputDataSizeInBytes,
			[Out] out IntPtr ppOutputData,
			[Out] out UInt32 pzOutputDataSizeInBytes);
		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern void Seoul_ReleaseLZ4CompressedData([In] IntPtr pData);

		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern bool Seoul_LZ4Decompress(
			[In] IntPtr pInputData,
			[In] UInt32 zInputDataSizeInBytes,
			[Out] out IntPtr ppOutputData,
			[Out] out UInt32 pzOutputDataSizeInBytes);
		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern void Seoul_ReleaseLZ4DecompressedData([In] IntPtr pData);

		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern UInt32 Seoul_GetCrc32(
			[In] UInt32 uCrc32,
			[In] IntPtr pInputData,
			[In] UInt32 zInputDataSizeInBytes);

		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern UInt64 Seoul_GetFileSize([In] IntPtr sFilename);
		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern UInt64 Seoul_GetModifiedTime([In] IntPtr sFilename);
		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern bool Seoul_SetModifiedTime([In] IntPtr sFilename, [In] UInt64 uModifiedTime);

		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern bool Seoul_ZSTDCompress(
			[In] IntPtr pInputData,
			[In] UInt32 zInputDataSizeInBytes,
			[Out] out IntPtr ppOutputData,
			[Out] out UInt32 pzOutputDataSizeInBytes);
		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern void Seoul_ReleaseZSTDCompressedData([In] IntPtr pData);

		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern bool Seoul_ZSTDDecompress(
			[In] IntPtr pInputData,
			[In] UInt32 zInputDataSizeInBytes,
			[Out] out IntPtr ppOutputData,
			[Out] out UInt32 pzOutputDataSizeInBytes);
		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern void Seoul_ReleaseZSTDDecompressedData([In] IntPtr pData);
		#endregion

		#region Shlwapi.dll bindings
		[DllImport("Shlwapi.dll", CharSet = CharSet.Auto)]
		static extern int PathRelativePathTo(
			[Out] StringBuilder pszPath,
			[In] string pszFrom,
			[In] FileAttributes dwAttrFrom,
			[In] string pszTo,
			[In] FileAttributes dwAttrTo);
		#endregion

		/// <summary>
		/// Utility, gets s as a NULL terminated UTF8 byte sequence for SeoulEngine
		/// </summary>
		static GCHandle AllocUTF8(string s)
		{
			// Get the character length in bytes.
			int iLengthInBytes = Encoding.UTF8.GetByteCount(s);

			// Allocate a buffer +1 for the NULL terminator.
			byte[] aNullTerminatedBytes = new byte[iLengthInBytes + 1];

			// Get and null terminate the string data.
			Encoding.UTF8.GetBytes(s, 0, s.Length, aNullTerminatedBytes, 0);
			aNullTerminatedBytes[iLengthInBytes] = 0;

			// Pin the buffer.
			return GCHandle.Alloc(aNullTerminatedBytes, GCHandleType.Pinned);
		}

		/// <summary>
		/// Internal utility for endian swapping a 64-bit float
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
		/// Internal utility for endian swapping a 32-bit float
		/// </summary>
		[StructLayout(LayoutKind.Explicit, Size = 4)]
		struct SingleEndianSwapUtil
		{
			[FieldOffset(0)]
			public UInt32 m_uUInt32;
			[FieldOffset(0)]
			public Single m_fSingle;
		}

		/// <summary>
		/// Releases an allocation by AllocUTF8
		/// </summary>
		static void FreeUTF8(ref GCHandle gchandle)
		{
			gchandle.Free();
			gchandle = default(GCHandle);
		}

		/// <summary>
		/// Shared utility
		/// </summary>
		static string CleanSeoulEngineRelativeFilename(string sRelativeFilename)
		{
			// Make the extension lowercase.
			sRelativeFilename = Path.ChangeExtension(sRelativeFilename, Path.GetExtension(sRelativeFilename).ToLower());

			// Convert '\\' separators to '/' separators.
			sRelativeFilename = sRelativeFilename.Replace('\\', '/');

			// Remove leading "./" if now present.
			if (sRelativeFilename.StartsWith(ksSeoulEngineLeadingDotSlash))
			{
				sRelativeFilename = sRelativeFilename.Substring(ksSeoulEngineLeadingDotSlash.Length);
			}

			return sRelativeFilename;
		}

		/// <summary>
		/// Final path conversions for a relative path to a SeoulEngine Content FilePath
		/// </summary>
		static string RelativeToSeoulEngineContentFilePath(string sRelativeFilename)
		{
			sRelativeFilename = CleanSeoulEngineRelativeFilename(sRelativeFilename);

			// Prepend the content:// specifier and return.
			string sSeoulEngineContentFilePath = ksSeoulEngineContentFilePathPrefix + sRelativeFilename;
			return sSeoulEngineContentFilePath;
		}
		#endregion

		/// <summary>
		/// Generic equality query with a tolerance window
		/// </summary>
		public static bool AboutEqual(float fA, float fB, float fEpsilon)
		{
			return (Math.Abs(fA - fB) < fEpsilon);
		}

		/// <summary>
		/// Generic equality to zero query with a tolerance window
		/// </summary>
		public static bool AboutZero(float fA, float fEpsilon)
		{
			return (Math.Abs(fA) < fEpsilon);
		}

		/// <summary>
		/// Marshal raw data in aData to a struct type T
		/// </summary>
		public static T ByteArrayTo<T>(byte[] aData) where T : struct
		{
			GCHandle handle = GCHandle.Alloc(aData, GCHandleType.Pinned);
			T ret = (T)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(T));
			handle.Free();

			return ret;
		}

		/// <summary>
		/// Generic Clamp - NaN safe (will return min if value is NaN)
		/// </summary>
		public static T Clamp<T>(T value, T min, T max)
			where T : IComparable<T>
		{
			return Min(Max(value, min), max);
		}

		/// <summary>
		/// Create any parent directories for the directory sDirectory.
		/// </summary>
		/// <returns>
		/// True if the directory path was created successfully (or already exists),
		/// false otherwise.
		/// </returns>
		public static bool CreateDirectoryDependencies(string sDirectory)
		{
			if (!Directory.Exists(sDirectory))
			{
				if (CreateDirectoryDependencies(Path.GetDirectoryName(sDirectory)))
				{
					try
					{
						DirectoryInfo info = Directory.CreateDirectory(sDirectory);

						return (info.Exists);
					}
					catch (Exception)
					{
						return false;
					}
				}

				return false;
			}

			return true;
		}

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
		/// Given an array of command-line arguments, generates a string
		/// that will be properly escaped for the Windows command-line.
		/// </summary>
		public static string FormatArgumentsForCommandLine(params string[] asArguments)
		{
			StringBuilder result = new StringBuilder();
			for (int i = 0; i < asArguments.Length; ++i)
			{
				if (0 != i)
				{
					result.Append(' ');
				}

				result.Append(FormatArgumentForCommandLine(asArguments[i]));
			}

			return result.ToString();
		}

		/// <summary>
		/// Escapes the argument for the Windows command-line
		/// </summary>
		public static string FormatArgumentForCommandLine(string sArgument)
		{
			StringBuilder result = new StringBuilder();

			bool bRequiresQuotes = false;
			int nPreceedingBackslashes = 0;
			for (int i = 0; i < sArgument.Length; ++i)
			{
				// If the current element is a double quote, we need to handle it specially.
				if (sArgument[i] == '"')
				{
					// Little tricky at first glance - this implements the following formatting rules:
					// - Backslashes are interpreted literally, unless they immediately precede a double quotation mark.
					// - If an even number of backslashes is followed by a double quotation mark, one backslash is placed
					//   in the argv array for every pair of backslashes, and the double quotation mark is interpreted as
					//   a string delimiter.
					// - If an odd number of backslashes is followed by a double quotation mark, one backslash is
					//   placed in the argv array for every pair of backslashes, and the double quotation mark is
					//   "escaped" by the remaining backslash, causing a literal double quotation mark (") to be placed
					//   in argv.
					//
					// In our case, we're going the other way - we're generating a command-line that, when converted by
					// the above rules into an argv array, we want the exact input argument sArgument to be restored. So,
					// reading between the lines a bit, the only case we need to handle specially is when a (") is present.
					// Further, since we want to get the exact same result, we always want to double the number of
					// backslashes and then add one more for quotes present in the input. For example:
					//
					// sArgument = "Hello World" . \"Hello World\" (zero backslashes * 2 + 1)
					// sArgument = \"Hello World"\ . \\\"Hello World\"\ (1 backslash * 2 + 1, zero backslashes * 2 + 1)
					result.Append('\\', nPreceedingBackslashes + 1);
					result.Append('"');

					nPreceedingBackslashes = 0;
				}
				// Otherwise, just append the character and potentially count backslashes in preparation
				// for a quote.
				else
				{
					// If the character is a backslash, increment the backslash count.
					if (sArgument[i] == '\\')
					{
						++nPreceedingBackslashes;
					}
					// Otherwise...
					else
					{
						// ...reset the backslash count.
						nPreceedingBackslashes = 0;

						// If this character is a tab or a space (the 2 argument delimiters according to docs), then we will need
						// to quote this entire argument for it to be interpreted correctly as a single command-line argument
						// instead of multiple.
						if (sArgument[i] == '\t' || sArgument[i] == ' ')
						{
							bRequiresQuotes = true;
						}
					}

					// Append this character.
					result.Append(sArgument[i]);
				}
			}

			// If we need to quote the entire argument, do so now.
			if (bRequiresQuotes)
			{
				// Leading double quote.
				result.Insert(0, '"');

				// See the comment above for the bulk of why this happens - for the very last quote, we *want* it to be interpreted
				// as a string delimiter (instead of passed through literally), so we need to double the number of preceeding
				// backslashes before adding the quote (we don't add 1 in this case, because that would case the quote to be escaped
				// and passed through literally, instead of acting as a string delimiter).
				result.Append('\\', nPreceedingBackslashes);
				result.Append('"');
			}

			return result.ToString();
		}

		/// <returns>
		/// A 32-bit CRC value for the block of data described by pData and zSizeInBytes.
		/// </returns>
		public static UInt32 GetCrc32(UInt32 uCrc32, byte[] aData)
		{
			GCHandle pinnedData = default(GCHandle);
			try
			{
				pinnedData = GCHandle.Alloc(
					aData,
					GCHandleType.Pinned);

				return Seoul_GetCrc32(
					uCrc32,
					pinnedData.AddrOfPinnedObject(),
					(UInt32)aData.Length);
			}
			finally
			{
				pinnedData.Free();
			}
		}

		/// <summary>
		/// Syntactic sugar for starting a Crc32 computation.
		/// </summary>
		public static UInt32 GetCrc32(byte[] aData)
		{
			return GetCrc32(0xFFFFFFFF, aData);
		}

		/// <summary>
		/// Ask the filesystem for the exact path name of sPath. Potentially
		/// expensive, but returns an exact path with case preserved.
		/// </summary>
		public static string GetExactPathName(string sPath)
		{
			if (!File.Exists(sPath) && !Directory.Exists(sPath))
			{
				return sPath;
			}

			DirectoryInfo info = new DirectoryInfo(sPath);
			if (null != info.Parent)
			{
				string sParentPath = GetExactPathName(info.Parent.FullName);
				string sChildPath = info.Parent.GetFileSystemInfos(info.Name)[0].Name;
				return Path.Combine(sParentPath, sChildPath);
			}
			else
			{
				// This is the drive letter - the canonical version is uppercase.
				return info.Name.ToUpper();
			}
		}

		/// <returns>
		/// An array of all files (in all subdirectories) of sPath, with
		/// extension sExtension.
		///
		/// \pre sExtension is an extension, including the leading '.' character.
		///
		/// This method is identical to Directory.GetFiles(), except that
		/// it guarantees that the extension of returned files exactly matches sPattern,
		/// if sPattern is a terminating pattern (i.e. "*.txt").
		///
		/// Directory.GetFiles() can return files whose extension is a superset of
		/// the specific extension - for example, ".txt" will return files that end
		/// in ".txt_files".
		/// </returns>
		public static string[] GetFiles(string sPath, string sPattern, SearchOption eSearchOption)
		{
			// Get the list of files using Directory.GetFiles().
			string[] aReturn = Directory.GetFiles(sPath, sPattern, eSearchOption);

			string sExtension = sPattern;
			if (sExtension.Length > 0 && sExtension[0] == '*')
			{
				sExtension = sExtension.Substring(1);
			}

			if (sExtension.IndexOf('*') < 0 && sExtension.IndexOf('?') < 0)
			{
				// Cache the number of results.
				int iCount = aReturn.Length;

				// For each result, if the extension of the result does not exactly match
				// the input extension, remove the entry by swapping it with the last entry.
				for (int i = 0; i < iCount; ++i)
				{
					if (0 != String.Compare(sExtension, Path.GetExtension(aReturn[i]), true))
					{
						Swap(ref aReturn[i], ref aReturn[iCount - 1]);
						--i;
						--iCount;
					}
				}

				// Remove any swapped out entries.
				Array.Resize(ref aReturn, iCount);
			}

			// Return the result array.
			return aReturn;
		}

		/// <returns>
		/// The file size in bytes of sAbsoluteFilename.
		/// </returns>
		public static UInt64 GetFileSize(string sAbsoluteFilename)
		{
			GCHandle pinnedAbsoluteFilename = default(GCHandle);
			try
			{
				pinnedAbsoluteFilename = AllocUTF8(sAbsoluteFilename);
				return Seoul_GetFileSize(pinnedAbsoluteFilename.AddrOfPinnedObject());
			}
			finally
			{
				FreeUTF8(ref pinnedAbsoluteFilename);
			}
		}

		/// <returns>
		/// The Unix epoch based modified timestamp of sAbsoluteFilename.
		///
		/// This timestamp will exactly match the timestamp returned from the equivalent
		/// SeoulEngine function GetModifiedTime().
		/// </returns>
		public static UInt64 GetModifiedTime(string sAbsoluteFilename)
		{
			GCHandle pinnedAbsoluteFilename = default(GCHandle);
			try
			{
				pinnedAbsoluteFilename = AllocUTF8(sAbsoluteFilename);
				return Seoul_GetModifiedTime(pinnedAbsoluteFilename.AddrOfPinnedObject());
			}
			finally
			{
				FreeUTF8(ref pinnedAbsoluteFilename);
			}
		}

		/// <summary>
		/// Mix iMixin into an existing rolling iHash
		/// </summary>
		public static int IncrementalHash(int iHash, int iMixin)
		{
			return (int)(iHash ^ (iMixin + 0x9e3779b9 + (iHash << 6) + (iHash >> 2)));
		}

		/// <returns>
		/// True if v is a power of 2, false otherwise.
		/// </returns>
		public static bool IsPowerOfTwo(int v)
		{
			return (0 == (v & (v - 1)));
		}

		/// <summary>
		/// Compress aInputData with LZ4 - includesa  SeoulEngine specific header
		/// </summary>
		public static byte[] LZ4Compress(byte[] aInputData)
		{
			GCHandle pinnedInputData = default(GCHandle);
			IntPtr pOutputData = IntPtr.Zero;
			try
			{
				pinnedInputData = GCHandle.Alloc(
					aInputData,
					GCHandleType.Pinned);

				UInt32 zOutputDataSizeInBytes = 0;
				if (!Seoul_LZ4Compress(
					pinnedInputData.AddrOfPinnedObject(),
					(UInt32)aInputData.Length,
					out pOutputData,
					out zOutputDataSizeInBytes))
				{
					return null;
				}

				byte[] aReturn = new byte[(int)zOutputDataSizeInBytes];
				Marshal.Copy(pOutputData, aReturn, 0, aReturn.Length);
				return aReturn;
			}
			finally
			{
				Seoul_ReleaseLZ4CompressedData(pOutputData);
				pinnedInputData.Free();
			}
		}

		/// <summary>
		/// Decompress aInputData as appropriate - supports
		/// data from LZ4Compress.
		/// </summary>
		public static byte[] LZ4Decompress(byte[] aInputData)
		{
			GCHandle pinnedInputData = default(GCHandle);
			IntPtr pOutputData = IntPtr.Zero;
			try
			{
				pinnedInputData = GCHandle.Alloc(
					aInputData,
					GCHandleType.Pinned);

				UInt32 zOutputDataSizeInBytes = 0;
				if (!Seoul_LZ4Decompress(
					pinnedInputData.AddrOfPinnedObject(),
					(UInt32)aInputData.Length,
					out pOutputData,
					out zOutputDataSizeInBytes))
				{
					return null;
				}

				byte[] aReturn = new byte[(int)zOutputDataSizeInBytes];
				Marshal.Copy(pOutputData, aReturn, 0, aReturn.Length);
				return aReturn;
			}
			finally
			{
				Seoul_ReleaseLZ4DecompressedData(pOutputData);
				pinnedInputData.Free();
			}
		}

		/// <summary>
		/// Suite of generic min/max routines
		/// </summary>
		public static T Max<T>(T a, T b)
			where T : IComparable<T>
		{
			return ((a.CompareTo(b) > 0) ? a : b);
		}

		/// <summary>
		/// Suite of generic min/max routines
		/// </summary>
		public static T Max<T>(T a, T b, T c)
			where T : IComparable<T>
		{
			return Max(a, Max(b, c));
		}

		/// <summary>
		/// Suite of generic min/max routines
		/// </summary>
		public static T Max<T>(T a, T b, T c, T d)
			where T : IComparable<T>
		{
			return Max(a, b, Max(c, d));
		}

		/// <summary>
		/// Suite of generic min/max routines
		/// </summary>
		public static T Min<T>(T a, T b)
			where T : IComparable<T>
		{
			return ((a.CompareTo(b) < 0) ? a : b);
		}

		/// <summary>
		/// Suite of generic min/max routines
		/// </summary>
		public static T Min<T>(T a, T b, T c)
			where T : IComparable<T>
		{
			return Min(a, Min(b, c));
		}

		/// <summary>
		/// Suite of generic min/max routines
		/// </summary>
		public static T Min<T>(T a, T b, T c, T d)
			where T : IComparable<T>
		{
			return Min(a, b, Min(c, d));
		}

		/// <summary>
		/// Return an unsigned integer that is the power of 2
		/// which is closest and greater than or equal to v.
		/// </summary>
		public static int NextPowerOfTwo(int v)
		{
			// Handle the edge case of u == 0.
			if (0 == v)
			{
				return 1;
			}

			uint u = (uint)v;

			// Subtract first to handle values that are already power of 2.
			u--;
			u |= (u >> 1);
			u |= (u >> 2);
			u |= (u >> 4);
			u |= (u >> 8);
			u |= (u >> 16);
			u++;

			return (int)u;
		}

		/// <summary>
		/// Utility that attempts to put sPath into a canonical form.
		/// </summary>
		/// <remarks>
		/// Path normalization is useful/necessary to resolve two
		/// potentially distinct string paths to the same file on disk.
		///
		/// The returned path is absolute when possible.
		/// </remarks>
		public static string NormalizePath(string sPath)
		{
			// Uri does most of the heavy lifting of generating a canonized
			// path.
			Uri uri = new Uri(sPath);

			// Initial result - Path.GetFullPath() returns
			// an absolute, resolve path to files on disk.
			string sReturn = Path.GetFullPath(uri.LocalPath);

			// Remove the trailing directory separator if present,
			// to maintain consistency.
			sReturn = sReturn.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);

			return sReturn;
		}

		/// <summary>
		/// Convenience wrapper around PathRelativePathTo().
		/// </summary>
		public static string PathRelativePathTo(
			string sAbsoluteDirectory,
			string sAbsoluteFilename)
		{
			// Special case handling.
			if (string.IsNullOrEmpty(sAbsoluteFilename))
			{
				return string.Empty;
			}

			StringBuilder relativePath = new StringBuilder(MAX_PATH);
			if (0 == PathRelativePathTo(
				relativePath,
				sAbsoluteDirectory,
				FileAttributes.Directory,
				sAbsoluteFilename,
				FileAttributes.Normal))
			{
				return sAbsoluteFilename;
			}
			else
			{

				string sRelativeFilename = relativePath.ToString();

				// Remove leading "./" if now present.
				if (sRelativeFilename.StartsWith(ksWindowsLeadingDotSlash))
				{
					sRelativeFilename = sRelativeFilename.Substring(ksWindowsLeadingDotSlash.Length);
				}

				return sRelativeFilename;
			}
		}

		/// <summary>
		/// Return an unsigned integer that is the power of 2
		/// which is closest and less than or equal to x.
		/// </summary>
		public static int PreviousPowerOfTwo(int v)
		{
			return NextPowerOfTwo(v + 1) / 2;
		}

		/// <summary>
		/// Convenience, rough equivalent of a propertyof() keyword,
		/// if it existed in the language.
		/// </summary>
		public static PropertyInfo PropertyOf<T>(Expression<Func<T>> expression)
		{
			MemberExpression body = (MemberExpression)expression.Body;
			PropertyInfo pi = body.Member as PropertyInfo;
			if (null == pi)
			{
				throw new ArgumentException("expected property", "expression");
			}

			return pi;
		}

		/// <summary>
		/// Generic utility for providing List&amp;lt;&amp;gt;.Remove() style functionality for an array
		/// </summary>
		public static void Remove<T>(ref T[] a, T val)
		{
			int iIndexOf = Array.IndexOf<T>(a, val);
			if (iIndexOf >= 0)
			{
				RemoveAt<T>(ref a, iIndexOf);
			}
		}

		/// <summary>
		/// Generic utility for providing List&amp;lt;&amp;gt;.RemoveAt() style functionality for an array
		/// </summary>
		public static void RemoveAt<T>(ref T[] a, int iIndex)
		{
			for (int i = iIndex; i < a.Length - 1; i++)
			{
				// Move elements forward.
				a[i] = a[i + 1];
			}

			// Resize the array.
			Array.Resize(ref a, a.Length - 1);
		}

		/// <summary>
		/// Return the full path of sPath with any trailing extension (i.e. ".txt") removed.
		/// </summary>
		public static string RemoveExtension(string sPath)
		{
			return Path.Combine(Path.GetDirectoryName(sPath), Path.GetFileNameWithoutExtension(sPath));
		}

		/// <summary>
		/// Return a full path sRelativeFile starting at the containing directory
		/// of sFromFile.
		/// </summary>
		/// <remarks>
		/// Utility to handle a common operation of resolving a path that is relative
		/// to a file. sFromFile *must* be a file path (e.g. C:\Foo\Bar.txt), *not*
		/// the containing directory of the file (e.g. C:\Foo) or the returned
		/// results will be incorrect.
		/// </remarks>
		public static string ResolveFileRelative(string sFromFile, string sRelativeFile)
		{
			return NormalizePath(Path.Combine(
				Path.GetDirectoryName(sFromFile),
				sRelativeFile));
		}

		/// <returns>
		/// zValue adjusted so that it fulfills zAlignment and
		/// is &amp;gt;= the initial value of zValue.
		/// </returns>
		public static Int64 RoundUpToAlignment(Int64 zValue, Int64 zAlignment)
		{
			if (zAlignment > 0)
			{
				Int64 zModulo = (zValue % zAlignment);
				Int64 zReturn = (0 == zModulo)
					? zValue
					: (zValue + zAlignment - zModulo);

				return zReturn;
			}
			else
			{
				return zValue;
			}
		}

		/// <returns>
		/// zValue adjusted so that it fulfills zAlignment and
		/// is &amp;gt;= the initial value of zValue.
		/// </returns>
		public static UInt32 RoundUpToAlignment(UInt32 zValue, UInt32 zAlignment)
		{
			if (zAlignment > 0u)
			{
				UInt32 zModulo = (zValue % zAlignment);
				UInt32 zReturn = (0u == zModulo)
					? zValue
					: (zValue + zAlignment - zModulo);

				return zReturn;
			}
			else
			{
				return zValue;
			}
		}

		/// <summary>
		/// Utility, executes a command-line process synchronously.
		/// </summary>
		/// <returns>
		/// Success if the process returned 0 as its return code, false otherwise.
		///
		/// If sStdOutput or sStdError are not null, they will be populated
		/// with any output from the process, whether the process returns 0 or not.
		/// </returns>
		public static bool RunCommandLineProcess(
			string sFilename,
			string sArguments,
			DebugMessageWriteLine stdOutput,
			DebugMessageWriteLine stdError,
			Dictionary<string, string> environmentVariablesToSet = null,
			string sWorkingDirectory = null,
			int iTimeoutInMilliseconds = -1)
		{
			bool bReturn = false;
			try
			{
				// Setup a process to run the cook.
				ProcessStartInfo startInfo = new ProcessStartInfo();
				startInfo.Arguments = sArguments;
				startInfo.CreateNoWindow = true;
				startInfo.ErrorDialog = false;
				startInfo.FileName = sFilename;
				startInfo.UseShellExecute = false;
				startInfo.WindowStyle = ProcessWindowStyle.Hidden;
				startInfo.RedirectStandardOutput = true;
				startInfo.RedirectStandardError = true;

				// Set working directory.
				if (null != sWorkingDirectory)
				{
					startInfo.WorkingDirectory = sWorkingDirectory;
				}

				// Add environment variables.
				if (null != environmentVariablesToSet)
				{
					foreach (KeyValuePair<string, string> e in environmentVariablesToSet)
					{
						string sValue = e.Value;
						if (startInfo.EnvironmentVariables.ContainsKey(e.Key))
						{
							sValue = sValue + ";" + startInfo.EnvironmentVariables[e.Key];
							startInfo.EnvironmentVariables.Remove(e.Key);
						}

						startInfo.EnvironmentVariables.Add(e.Key, sValue);
					}
				}

				// Create the process.
				Process process = Process.Start(startInfo);

				// Create the message captures.
				MessageCapture stdOutMessageCapture = new MessageCapture(stdOutput);
				MessageCapture stdErrorMessageCapture = new MessageCapture(stdError);

				process.OutputDataReceived += new DataReceivedEventHandler(
					stdOutMessageCapture.ReceiveMessage);
				process.ErrorDataReceived += new DataReceivedEventHandler(
					stdErrorMessageCapture.ReceiveMessage);

				// Run the process and wait for it to complete.
				process.BeginOutputReadLine();
				process.BeginErrorReadLine();

				if (iTimeoutInMilliseconds < 0)
				{
					process.WaitForExit();

					// Cache the return value, based on whether the cooker returns 0 or not.
					bReturn = (0 == process.ExitCode);
				}
				else
				{
					if (!process.WaitForExit(iTimeoutInMilliseconds))
					{
						bReturn = false;
						process.Kill();
					}
					else
					{
						bReturn = (0 == process.ExitCode);
					}
				}

				process.Close();
				process.Dispose();
			}
			// Any exceptions are considered a failure.
			catch (Exception e)
			{
				if (null != stdError)
				{
					stdError(e.Message);
				}
				bReturn = false;
			}

			return bReturn;
		}

		/// <summary>
		/// Invoke dispose on disposable, if not null, and clear to its default
		/// </summary>
		public static void SafeDispose<T>(ref T disposable)
			where T : IDisposable
		{
			if (null != disposable)
			{
				disposable.Dispose();
			}

			disposable = default(T);
		}

		/// <summary>
		/// Update the modified time of sAbsoluteFilename to match
		/// uModifiedTime, which is expected to come from GetModifiedTime() or
		/// an equivalent Unix epoch based time value.
		/// </summary>
		public static bool SetModifiedTime(string sAbsoluteFilename, UInt64 uModifiedTime)
		{
			GCHandle pinnedAbsoluteFilename = default(GCHandle);
			try
			{
				pinnedAbsoluteFilename = AllocUTF8(sAbsoluteFilename);
				return Seoul_SetModifiedTime(pinnedAbsoluteFilename.AddrOfPinnedObject(), uModifiedTime);
			}
			finally
			{
				FreeUTF8(ref pinnedAbsoluteFilename);
			}
		}

		/// <summary>
		/// General purpose swap implementation (a exchanged for b)
		/// </summary>
		public static void Swap<T>(ref T a, ref T b)
		{
			T tmp = a;
			a = b;
			b = tmp;
		}

		/// <summary>
		/// Marshal a struct type T to raw bytes into aData
		/// </summary>
		public static void ToByteArray<T>(T val, byte[] aData) where T : struct
		{
			int iSizeT = Marshal.SizeOf(typeof(T));
			IntPtr ptr = Marshal.AllocHGlobal(iSizeT);
			Marshal.StructureToPtr(val, ptr, true);
			Marshal.Copy(ptr, aData, 0, iSizeT);
			Marshal.FreeHGlobal(ptr);
		}

		/// <summary>
		/// Convert an absolute path to a SeoulEngine content path
		/// </summary>
		public static string ToSeoulEngineContentFilePath(
			string sAbsoluteSourceDirectory,
			string sAbsoluteFilename)
		{
			// Special case handling.
			if (string.IsNullOrEmpty(sAbsoluteFilename))
			{
				return string.Empty;
			}

			StringBuilder relativePath = new StringBuilder(MAX_PATH);
			if (0 == PathRelativePathTo(
				relativePath,
				sAbsoluteSourceDirectory,
				FileAttributes.Directory,
				sAbsoluteFilename,
				FileAttributes.Normal))
			{
				return RelativeToSeoulEngineContentFilePath(sAbsoluteFilename);
			}
			else
			{
				return RelativeToSeoulEngineContentFilePath(relativePath.ToString());
			}
		}

		/// <summary>
		/// Compress aInputData with ZSTD - includesa  SeoulEngine specific header
		/// </summary>
		public static byte[] ZSTDCompress(byte[] aInputData)
		{
			GCHandle pinnedInputData = default(GCHandle);
			IntPtr pOutputData = IntPtr.Zero;
			try
			{
				pinnedInputData = GCHandle.Alloc(
					aInputData,
					GCHandleType.Pinned);

				UInt32 zOutputDataSizeInBytes = 0;
				if (!Seoul_ZSTDCompress(
					pinnedInputData.AddrOfPinnedObject(),
					(UInt32)aInputData.Length,
					out pOutputData,
					out zOutputDataSizeInBytes))
				{
					return null;
				}

				byte[] aReturn = new byte[(int)zOutputDataSizeInBytes];
				Marshal.Copy(pOutputData, aReturn, 0, aReturn.Length);
				return aReturn;
			}
			finally
			{
				Seoul_ReleaseZSTDCompressedData(pOutputData);
				pinnedInputData.Free();
			}
		}

		/// <summary>
		/// Decompress aInputData as appropriate - supports
		/// data from ZSTDCompress.
		/// </summary>
		public static byte[] ZSTDDecompress(byte[] aInputData)
		{
			GCHandle pinnedInputData = default(GCHandle);
			IntPtr pOutputData = IntPtr.Zero;
			try
			{
				pinnedInputData = GCHandle.Alloc(
					aInputData,
					GCHandleType.Pinned);

				UInt32 zOutputDataSizeInBytes = 0;
				if (!Seoul_ZSTDDecompress(
					pinnedInputData.AddrOfPinnedObject(),
					(UInt32)aInputData.Length,
					out pOutputData,
					out zOutputDataSizeInBytes))
				{
					return null;
				}

				byte[] aReturn = new byte[(int)zOutputDataSizeInBytes];
				Marshal.Copy(pOutputData, aReturn, 0, aReturn.Length);
				return aReturn;
			}
			finally
			{
				Seoul_ReleaseZSTDDecompressedData(pOutputData);
				pinnedInputData.Free();
			}
		}
	}
} // namespace CoreLib
