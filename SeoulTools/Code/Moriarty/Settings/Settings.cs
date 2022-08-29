// Common base class and owner for settings in Moriarty (currently, UserSettings and ProjectSettings).
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
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Xml;
using System.Xml.Serialization;

namespace Moriarty.Settings
{
	/// <summary>
	/// Common base class fo settings objects (currently UserSettings and ProjectSettings).
	/// Managed by SettingsOwner.
	/// </summary>
	public abstract class Settings
	{
		#region Private members
		string m_sFilename = string.Empty;
		#endregion

		[Browsable(false)]
		[XmlIgnore]
		public virtual string Filename
		{
			get
			{
				return m_sFilename;
			}

			set
			{
				m_sFilename = value;
			}
		}
	}

	/// <summary>
	/// Concrete type used to manage a Settings instance. Loads the instance form disk, and provides
	/// a method to save it to disk.
	/// </summary>
	public sealed class SettingsOwner<T>
		where T : Settings
	{
		#region Private members
		static Type[] GetSerializerTypes(Type settingsType)
		{
			// If the most derived Type of settingsType implement the "GetAdditionalSerializerTypes" method,
			// add those to the list of types that the serializer must support.
			Type[] additionalTypes = new Type[0];
			try
			{
				MethodInfo methodInfo = settingsType.GetMethod("GetAdditionalSerializerTypes");
				if (null != methodInfo)
				{
					additionalTypes = (Type[])methodInfo.Invoke(null, null);
				}
			}
			catch (Exception)
			{
			}

			List<Type> ret = new List<Type>();
			ret.Add(settingsType);
			ret.AddRange(additionalTypes);
			return ret.ToArray();
		}

		XmlSerializer m_Serializer = null;
		string m_sAbsoluteFilename = string.Empty;
		Type m_SettingsType = null;
		T m_Settings = null;
		bool m_bCanSave = true;
		#endregion

		/// <summary>
		/// Construct this SettingsOwner with a conrete settings type settingsType
		/// and save/load filename sAbsoluteFilename.
		/// </summary>
		/// <remarks>
		/// \pre settingsType must be a instancable subclass of Moriarty.Settings or
		/// this constructor will throw an exception.
		/// </remarks>
		public SettingsOwner(Type settingsType, string sAbsoluteFilename)
		{
			m_Serializer = new XmlSerializer(settingsType, GetSerializerTypes(settingsType));
			m_sAbsoluteFilename = sAbsoluteFilename;
			m_SettingsType = settingsType;
			m_Settings = (T)Activator.CreateInstance(m_SettingsType);
			m_Settings.Filename = m_sAbsoluteFilename;
			Load();
		}

		/// <summary>
		/// The filename that will be used to Load()/Save() the settings owned by this
		/// SettingsOwner.
		/// </summary>
		public string Filename
		{
			get
			{
				return m_sAbsoluteFilename;
			}

			set
			{
				if (value != m_sAbsoluteFilename)
				{
					// Allow saving again.
					m_bCanSave = true;
					m_sAbsoluteFilename = value;
				}
			}
		}

		/// <returns>
		/// The settings object managed by this SettingsOwner&amp;lt;&amp;gt;
		/// </returns>
		public T Settings
		{
			get
			{
				return m_Settings;
			}
		}

		/// <summary>
		/// Attempt to load the current settings state from the absolute Filename
		/// registered at construction
		/// </summary>
		/// <returns>
		/// True if loading was successful, false otherwise.
		///
		/// Depending on the load failure, saving may be disabled, to prevent
		/// corruption of a file in an unknown state. To clear this error, change
		/// the Filename of this SettingsOwner.
		/// </returns>
		public bool Load()
		{
			bool bFileExistsButIsEmpty = false;
			try
			{
				using (FileStream fs = new FileStream(
					m_sAbsoluteFilename,
					FileMode.Open,
					FileAccess.Read))
				{
					bFileExistsButIsEmpty = (fs.Length == 0);
					m_Settings = (T)m_Serializer.Deserialize(fs);
					m_Settings.Filename = m_sAbsoluteFilename;
					return true;
				}
			}
			// FileNotFoundException is ignored.
			catch (FileNotFoundException)
			{
				// Ignore this exception - it just means the user doesn't have a
				// settings file, so we use the defaults.
			}
			// All other exceptions disable saving, to prevent corrupting the
			// settings file, unless the file is empty in which case we can
			// safely overwrite it.
			catch (Exception e)
			{
				// TODO: Log.
				Debug.Print(e.StackTrace);
				m_bCanSave = bFileExistsButIsEmpty;
			}

			return false;
		}

		/// <summary>
		/// Attempt to save the current settings state to the absolute Filename
		/// registered at construction
		/// </summary>
		/// <returns>
		/// True if saving was successful, false otherwise.
		///
		/// Saving may fail immediately if it was previously disabled by a Load failure.
		/// </returns>
		public bool Save()
		{
			// Fail immediately if saving has been disabled.
			if (!m_bCanSave)
			{
				return false;
			}

			try
			{
				if (CoreLib.Utilities.CreateDirectoryDependencies(Path.GetDirectoryName(m_sAbsoluteFilename)))
				{
					using (FileStream fs = new FileStream(
						m_sAbsoluteFilename,
						FileMode.Create,
						FileAccess.Write))
					{
						m_Serializer.Serialize(fs, m_Settings);
						return true;
					}
				}
			}
			catch (Exception)
			{
				// TODO: Pass this error along? Use a message box?
			}

			return false;
		}
	}
}
