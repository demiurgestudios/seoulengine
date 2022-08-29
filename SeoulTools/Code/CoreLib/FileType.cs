// Equivalent to FileType in SeoulEngine FilePath.h.
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
	/// Equivalent to FileType in SeoulEngine FilePath.h
	/// </summary>
	public enum FileType
	{
		Unknown,

		Animation2D,

		Csv,

		Effect,
		EffectHeader,

		Exe,

		Font,

		FxBank,

		Html,

		Json,

		PEMCertificate,

		Protobuf,

		SaveGame,
		SceneAsset,
		ScenePrefab,

		Script,

		SoundBank,
		SoundProject,

		Texture0,
		Texture1,
		Texture2,
		Texture3,
		Texture4,

		Text,

		UIMovie,

		Wav,
		Xml,
		ScriptProject,
		Cs,
		Video,
	} // enum FileType
} // namespace CoreLib
