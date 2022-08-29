// Encapsulates a relative path with game directory info. Equivalent
// to FilePath in the SeoulEngine codebase, but with some differences to
// support its usage in Moriarty.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Moriarty.Utilities
{
	public struct FilePath
	{
		/// <summary>
		/// Type code of the file, maps to a source or cooked file extension
		/// </summary>
		public CoreLib.FileType m_eFileType;

		/// <summary>
		/// Type code of the file, maps to directory in the game project's file hierarchy
		/// </summary>
		public CoreLib.GameDirectory m_eGameDirectory;

		/// <summary>
		/// Relative path of the file, excluding extension
		/// </summary>
		public string m_sRelativePathWithoutExtension;

		/// <returns>
		/// Human readable representation of this FilePath
		/// </returns>
		public override string ToString()
		{
			return m_eGameDirectory.ToString() + ":" +
				m_sRelativePathWithoutExtension +
				FileUtility.FileTypeToCookedExtension(m_eFileType);
		}
	}
} // namespace Moriarty.Utilities
