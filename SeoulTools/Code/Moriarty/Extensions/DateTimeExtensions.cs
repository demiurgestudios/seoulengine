// Extension to the DateTime class, allows for filename safe
// ToString() functions and conversion of a DateTime structure to
// a Unix-based timestamp.
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

namespace Moriarty.Extensions
{
	public static class DateTimeExtensions
	{
		/// <returns>
		/// A filename-safe human readable representation of the time in dateTime
		/// </returns>
		public static string ToFilenameString(this DateTime dateTime)
		{
			return
				dateTime.Year.ToString() + "-" +
				dateTime.Month.ToString() + "-" +
				dateTime.Day.ToString() + "-" +
				dateTime.Hour.ToString() + "_" +
				dateTime.Minute.ToString() + "_" +
				dateTime.Second.ToString();
		}
	}
} // namespace Moriarty.Extensions
