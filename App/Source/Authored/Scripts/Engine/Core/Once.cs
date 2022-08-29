// Utility that executes a function passed to it once and once only.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public sealed class Once
{
	#region Private members
	bool m_bDone;
	#endregion

	public void Do(Vfunc0 f)
	{
		if (m_bDone)
		{
			return;
		}
		m_bDone = true;

		f();
	}
}
