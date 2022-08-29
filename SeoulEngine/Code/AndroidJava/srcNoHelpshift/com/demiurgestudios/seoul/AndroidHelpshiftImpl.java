/**
 * \file AndroidHelpshiftImpl.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

// Null variation - used on projects that don't include HelpShift.
public class AndroidHelpshiftImpl
{
	public AndroidHelpshiftImpl(final AndroidNativeActivity activity)
	{
	}

	public void HelpShiftInitialize(
		final String sUserName,
		final String sUserID,
		final String sHelpShiftKey,
		final String sHelpShiftDomain,
		final String sHelpShiftID)
	{
	}

	public void HelpShiftShutdown()
	{
	}

	public boolean HelpShiftOpenUrl(
		final String[] asCustomIssueFields,
		final String[] asCustomMetadata,
		final String sURL)
	{
		return false;
	}

	public boolean HelpShiftShowHelp(
		final String[] asCustomIssueFields,
		final String[] asCustomMetadata)
	{
		return false;
	}
}
