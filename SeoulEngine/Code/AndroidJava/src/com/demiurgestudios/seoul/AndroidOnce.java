/**
 * \file AndroidOnce.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import java.lang.Runnable;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Utility class, runs a Runnable once and only
 * once. If run, the block code is run immediately
 * in the caller scope.
 */
public final class AndroidOnce
{
	AtomicBoolean m_b = new AtomicBoolean();

	public void Call(Runnable f)
	{
		if (m_b.get())
		{
			return;
		}

		if (m_b.compareAndSet(false, true))
		{
			f.run();
		}
	}
}
