// Equivalent to GameDirectory::Enum in SeoulEngine FilePath.h.
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

namespace CoreLib
{
	/// <summary>
	/// Enum values used to identify a directory accessible to
	/// Seoul Engine. Equivalent to GameDirectory::Enum in SeoulEngine
	/// FilePath.h
	/// </summary>
	public enum GameDirectory
	{
		// NOTE: Take care when changing these - these values must be kept in-sync with
		// equivalent values in SeoulEngine, FilePath.h.
		Unknown,
		Config,
		Content,
		Log,
		Save,
		ToolsBin,
		Videos,
	}; // enum GameDirectory
} // namespace CoreLib.
