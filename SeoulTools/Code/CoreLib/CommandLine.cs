// Parses an array of commandline arguments into a key-value pair directory.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

// Note: this code adapted from
// a C# version at: http://www.codeproject.com/KB/recipes/command_line.aspx

using System;
using System.Collections.Specialized;
using System.Text.RegularExpressions;

namespace CoreLib
{
	public sealed class CommandLine
	{
		#region Non-public members
		StringDictionary m_tParameters = new StringDictionary();
		#endregion

		public CommandLine(string[] aArgs)
		{
			Regex isParam = new Regex("^-{1,2}|^/", RegexOptions.IgnoreCase | RegexOptions.Compiled);
			Regex splitter = new Regex("^-{1,2}|^/|=|:", RegexOptions.IgnoreCase | RegexOptions.Compiled);
			Regex remover = new Regex("^['\"]?(.*?)['\"]?$", RegexOptions.IgnoreCase | RegexOptions.Compiled);

			string sParam = null;
			string[] aParts;

			// Valid parameter forms:
			// {-,/,--}param{ ,=,:}((",')value(",'))
			// Examples:
			// -param1 value1
			// --param2
			// /param3:"Test-:-work"
			// /param4=happy
			// -param5 '--=nice=--'

			foreach (string sTxt in aArgs)
			{
				// Look for new parameters (-,/ or --) and a
				// possible enclosed value (=,:)
				if (isParam.IsMatch(sTxt))
				{
					aParts = splitter.Split(sTxt, 3);
				}
				else
				{
					aParts = new string[] { sTxt };
				}

				switch (aParts.Length)
				{
					// Found a value (for the last parameter
					// found (space separator))
				case 1:
					if (sParam != null)
					{
						if (!m_tParameters.ContainsKey(sParam))
						{
							aParts[0] = remover.Replace(aParts[0], "$1");
							m_tParameters.Add(sParam, aParts[0]);
						}
						sParam = null;
					}
					// else Error: no parameter waiting for a value (skipped)
					break;

					// Found just a parameter
				case 2:
					// The last parameter is still waiting with no value, so set its value to "true"
					if (sParam != null)
					{
						if (!m_tParameters.ContainsKey(sParam))
						{
							m_tParameters.Add(sParam, "true");
						}
					}
					sParam = aParts[1];
					break;

					// Parameter with enclosed value
				case 3:
					// The last parameter is still waiting with no value, so set its value to "true"
					if (sParam != null)
					{
						if (!m_tParameters.ContainsKey(sParam))
						{
							m_tParameters.Add(sParam, "true");
						}
					}
					sParam = aParts[1];

					// Remove possible enclosing characters (",')

					if (!m_tParameters.ContainsKey(sParam))
					{
						aParts[2] = remover.Replace(aParts[2], "$1");
						m_tParameters.Add(sParam, aParts[2]);
					}

					sParam = null;
					break;
				}
			};

			// In case a parameter is still waiting
			if (sParam != null)
			{
				if (!m_tParameters.ContainsKey(sParam))
				{
					m_tParameters.Add(sParam, "true");
				}
			}
		}

		public string this[string sKey]
		{
			get
			{
				return m_tParameters[sKey];
			}

			set
			{
				m_tParameters[sKey] = value;
			}
		}
	} // class CommandLine
} // namespace CoreLib
