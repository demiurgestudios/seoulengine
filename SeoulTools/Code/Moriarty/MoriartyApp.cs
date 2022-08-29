// Root entry point for the Moriarty GUI application - very little
// happens here, this is simply the point from which everything originates.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace Moriarty
{
	static class MoriartyApp
	{
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main()
		{
			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault(false);
			Application.Run(new Moriarty.UI.MoriartyMainForm());
		}
	}
} // namespace Moriarty
