// UserSettings contains all the settings that are expected to change per Moriarty user.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.DirectoryServices.ActiveDirectory;
using System.Drawing.Design;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Xml.Serialization;
using Moriarty.Utilities;

namespace Moriarty.Settings
{
	/// <summary>
	/// Base class for the a settings object that describes the parameters
	/// necessary to launch a game for a particular platform.
	/// </summary>
	public abstract class LaunchTargetSettings
	{
		#region Private members
		string m_sName = "Unnamed Target";
		#endregion

		public override string ToString()
		{
			return m_sName;
		}

		[Category("Configuration")]
		[DefaultValue("Unnamed Target")]
		[Description("Name to identify this target in the list of targets.")]
		public string Name { get { return m_sName; } set { m_sName = value; } }

		[Category("Target")]
		[DefaultValue(CoreLib.Platform.Unknown)]
		[Description("The platform of the target, one of Seoul Engine's supported platforms.")]
		public abstract CoreLib.Platform Platform { get; }
	}

	/// <summary>
	/// Specialization of LaunchTargetSettings for target configurations for launching
	/// the Android build of the game. Can be further specialized for a project in its subclass
	/// of UserSettings.
	/// </summary>
	public class AndroidLaunchTargetSettings : LaunchTargetSettings
	{
		#region Private members
		/// <summary>
		/// If an AndroidLaunchTargetSettings is in use, kick of starting up the Adb server, to avoid
		/// unfriendly hangs when, for example, the user tries to get a device list.
		/// </summary>
		static AndroidLaunchTargetSettings()
		{
			ThreadPool.QueueUserWorkItem(new System.Threading.WaitCallback(delegate(object state)
				{
					try
					{
						AndroidUtilities.StartAdbServer();
					}
					catch (Exception)
					{
					}
				}));
		}
		#endregion

		public override CoreLib.Platform Platform { get { return CoreLib.Platform.Android; } }
	}

	/// <summary>
	/// Specialization of LaunchTargetSettings for target configurations for launching
	/// the iOS build of the game. Can be further specialized for a project in its subclass
	/// of UserSettings.
	/// </summary>
	public class IOSLaunchTargetSettings : LaunchTargetSettings
	{
		public override CoreLib.Platform Platform { get { return CoreLib.Platform.IOS; } }
	}

	/// <summary>
	/// Describes per-user configurable settings for the Moriarty server. This class
	/// can be further specialized in a per-project settings C# file that will be compiled
	/// by Moriarty on load. See MoriartyManager.
	/// </summary>
	public class UserSettings : Settings
	{
		#region Private members
		List<LaunchTargetSettings> m_LaunchTargets = new List<LaunchTargetSettings>();
		bool m_bConnectMoriarty = true;
		string m_sActiveTarget = string.Empty;
		string m_sAbsoluteITunesFilename = string.Empty;
		bool m_bConnectWithIP = false;
		bool m_bWaitForDebugger = false;
		#endregion

		/// <returns>
		/// An array of Reflection.Type objects that describe the types,
		/// in addition to the UserSettings subclass, that must be handlable by
		/// the XmlSerializer. Typically, this is the set of LaunchTargetSettings subclasses.
		/// </returns>
		public static Type[] GetAdditionalSerializerTypes()
		{
			return UI.MoriartyCollectionEditor.GetInstancableTypes(typeof(LaunchTargetSettings));
		}

		public UserSettings()
		{
			// Try a default path for iTunes, if it exists
			if (string.IsNullOrEmpty(m_sAbsoluteITunesFilename))
			{
				string sITunesPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), @"iTunes\iTunes.exe");
				if (File.Exists(sITunesPath))
				{
					m_sAbsoluteITunesFilename = sITunesPath;
				}
			}
		}

		/// <returns>
		/// An list of commandline arguments that will be passed
		/// to any targets when they are launched, if the target supports commandline arguments.
		/// Can be specialized by the project's UserSettings subclass.
		/// </returns>
		[Browsable(false)]
		[XmlIgnore]
		public virtual List<string> LaunchTargetCommandlineArgumentsList
		{
			get
			{
				List<string> args = new List<string>();

				// If enabled, pass the -moriarty_server argument to the target.
				if (ConnectMoriarty)
				{
					args.Add("-moriarty_server=" + GetConnectionAddress());
				}

				return args;
			}
		}

		/// <returns>
		/// The IP, machine name, or full host name, depending on
		/// settings and current machine state.
		/// </returns>
		protected string GetConnectionAddress()
		{
			string sReturn = string.Empty;
			if (ConnectWithIP)
			{
				sReturn = GetIPAddress();
			}
			else
			{
				try
				{
					sReturn = GetFullHostName();
				}
				catch (Exception)
				{
					sReturn = GetIPAddress();
				}
			}

			return sReturn;
		}

		/// <summary>
		/// Helper function to get the full host name (e.g.
		/// "tron.demiurge.local") of the local machine.
		/// </summary>
		static string GetFullHostName()
		{
			string sHostName = Environment.MachineName;

			Domain domain = Domain.GetComputerDomain();
			sHostName += "." + domain.Name;

			return sHostName;
		}

		/// <returns>
		/// Get the local machine's IPv4 address.
		/// </returns>
		static string GetIPAddress()
		{
			// Find the first network interface which has an IPv4 address and
			// isn't a loopback, tunnel, or VirtualBox interface
			NetworkInterface[] nics = NetworkInterface.GetAllNetworkInterfaces();
			foreach (NetworkInterface nic in nics)
			{
				if (nic.NetworkInterfaceType != NetworkInterfaceType.Loopback &&
					nic.NetworkInterfaceType != NetworkInterfaceType.Tunnel &&
					(nic.Name == null || !nic.Name.StartsWith("VirtualBox")) &&
					nic.Supports(NetworkInterfaceComponent.IPv4))
				{
					IPInterfaceProperties ipProps = nic.GetIPProperties();
					foreach (UnicastIPAddressInformation ipAddr in ipProps.UnicastAddresses)
					{
						if (ipAddr.Address.AddressFamily == AddressFamily.InterNetwork)
						{
							return ipAddr.Address.ToString();
						}
					}
				}
			}

			return string.Empty;
		}

		/// <returns>
		/// A string of whitespace-separated arguments that will be passed
		/// to any targets when they are launched, if the target supports commandline arguments.
		/// </returns>
		[Browsable(false)]
		[XmlIgnore]
		public string LaunchTargetCommandlineArguments
		{
			get
			{
				// Flatten the arguments list into a space-separated string
				StringBuilder sArgs = new StringBuilder();
				bool bIsFirstArg = true;
				List<string> launchTargetCommandlineArguments = LaunchTargetCommandlineArgumentsList;
				foreach (string sArg in launchTargetCommandlineArguments)
				{
					if (bIsFirstArg)
					{
						bIsFirstArg = false;
					}
					else
					{
						sArgs.Append(' ');
					}

					if (!sArg.Contains(' ') && !sArg.Contains('"') && !sArg.Contains('\\'))
					{
						sArgs.Append(sArg);
					}
					else
					{
						string sEscapedArg = sArg.Replace("\\", "\\\\").Replace("\"", "\\\"").Replace("'", "\\'");
						sArgs.Append('"').Append(sEscapedArg).Append('"');
					}
				}

				return sArgs.ToString();
			}
		}

		[Category("Targets")]
		[Description("The list of targets currently configured. Moriarty can run the game on these targets.")]
		[Editor(typeof(UI.MoriartyCollectionEditor), typeof(UITypeEditor))]
		public List<LaunchTargetSettings> LaunchTargets { get { return m_LaunchTargets; } set { m_LaunchTargets = value; } }

		[Category("Launch Options")]
		[DefaultValue(true)]
		[Description("If true, Moriarty will connect to the target after it launches.")]
		public bool ConnectMoriarty { get { return m_bConnectMoriarty; } set { m_bConnectMoriarty = value; } }

		[Category("Launch Options")]
		[DefaultValue(false)]
		[Description("If true, advertise this computer's IP instead of hostname for establishing a Moriarty connection. Set to true if you have trouble connecting with Moriarty by default.")]
		public bool ConnectWithIP { get { return m_bConnectWithIP; } set { m_bConnectWithIP = value; } }

		[Category("Launch Options")]
		[DefaultValue(false)]
		[Description("If true and if supported, the target will wait for a debugger to attach before continuing.")]
		public bool WaitForDebugger { get { return m_bWaitForDebugger; } set { m_bWaitForDebugger = value; } }

		[Browsable(false)]
		[Category("Launch Options")]
		[Description("The currently active target, this target will be launched when the run button is clicked.")]
		public string ActiveTarget { get { return m_sActiveTarget; } set { m_sActiveTarget = value; } }
	}
} // namespace Moriarty.Settings
