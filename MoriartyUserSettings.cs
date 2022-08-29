/**
 * \file MoriartyUserSettings.cs
 * \brief Specialization of UserSettings in the Moriarty codebase for the App. This file
 * will be dynamically compiled by Moriarty.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Text;
using System.Xml;
using System.Xml.Serialization;

namespace Moriarty.Settings
{
	public sealed class MoriartyUserSettings : UserSettings
	{
		public new static Type[] GetAdditionalSerializerTypes()
		{
			return UI.MoriartyCollectionEditor.GetInstancableTypes(typeof(LaunchTargetSettings));
		}

		public MoriartyUserSettings()
		{
		}

		[Browsable(false)]
		[XmlIgnore]
		public override List<string> LaunchTargetCommandlineArgumentsList
		{
			get
			{
				List<string> args = base.LaunchTargetCommandlineArgumentsList;

				// Automated test.
				if (AutomationScript)
				{
					args.Add("-automation_script=DevOnly/AutomatedTests/BasicAutomation.lua");
				}

				return args;
			}
		}

		[Category("Launch Options")]
		[DefaultValue(false)]
		[Description("If true, launch the build running the basic automation test.")]
		public bool AutomationScript { get; set; }
	}
} // namespace Moriarty
