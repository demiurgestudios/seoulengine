// C# wrapper around the native cooker database.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Runtime.InteropServices;

namespace CoreLib
{
	public sealed class CookDatabase : IDisposable
	{
		#region SeoulUtil.dll bindings
		const string kSeoulUtilDLL = "SeoulUtil.dll";
		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern bool Seoul_CookDatabaseCheckUpToDate(
			[In] IntPtr pCookDatabase,
			[In] string sFilename);

		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern IntPtr Seoul_CookDatabaseCreate(
			[In] int iPlatform);

		[DllImport(kSeoulUtilDLL, CallingConvention = CallingConvention.StdCall)]
		static extern void Seoul_CookDatabaseRelease(
			[In] IntPtr pCookDatabase);
		#endregion

		#region Private members
		readonly CoreLib.Platform m_ePlatform;
		IntPtr m_pCookDatabase;

		~CookDatabase()
		{
			Dispose();
		}
		#endregion

		public CookDatabase(CoreLib.Platform ePlatform)
		{
			m_ePlatform = ePlatform;
			m_pCookDatabase = Seoul_CookDatabaseCreate((int)ePlatform);
		}

		public void Dispose()
		{
			if (m_pCookDatabase != IntPtr.Zero)
			{
				var pCookDatabase = m_pCookDatabase;
				m_pCookDatabase = IntPtr.Zero;

				Seoul_CookDatabaseRelease(pCookDatabase);
			}
		}

		/// <summary>
		/// Returns true if the given file is up-to-date (does not require cooking),
		/// false otherwise.
		/// </summary>
		public bool IsUpToDate(string sAbsoluteFilename)
		{
			if (IntPtr.Zero == m_pCookDatabase)
			{
				return false;
			}

			return Seoul_CookDatabaseCheckUpToDate(
				m_pCookDatabase,
				sAbsoluteFilename);
		}
	} // class CookerDatabase
} // namespace CoreLib
