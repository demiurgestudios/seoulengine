// Extension to the EndPoint class and some subclasses,
// allows for filename safe ToString() functions.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace Moriarty.Extensions
{
	public static class EndPointExtensions
	{
		/// <returns>
		/// A filename-safe human readable representation of the connection in endPoint
		/// </returns>
		public static string ToFilenameString(this EndPoint endPoint)
		{
			string sReturn = endPoint.ToString();
			sReturn = sReturn.Replace('.', '_');
			sReturn = sReturn.Replace(':', '_');
			return sReturn;
		}
	}

	public static class IPEndPointExtensions
	{
		/// <returns>
		/// A filename-safe human readable representation of the connection in endPoint
		/// </returns>
		public static string ToFilenameString(this IPEndPoint endPoint)
		{
			string sReturn = endPoint.Address.ToString();
			sReturn = sReturn.Replace('.', '_');
			sReturn = sReturn.Replace(':', '_');
			return sReturn;
		}
	}
} // namespace Moriarty.Extensions
