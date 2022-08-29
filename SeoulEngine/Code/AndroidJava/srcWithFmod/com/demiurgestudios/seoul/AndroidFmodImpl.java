/**
 * \file AndroidFmodImpl.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.content.Context;

// Implementation against "new" FMOD (FMOD Studio and FMOD Low Level)
public final class AndroidFmodImpl
{
	public static final String kLowLevelLibName = "fmod";
	public static final String kHighLevelLibName = "fmodstudio";

	public AndroidFmodImpl()
	{
	}

	public void OnCreate(Context context)
	{
		org.fmod.FMOD.init(context);
	}

	public void OnDestroy(Context context)
	{
		org.fmod.FMOD.close();
	}

	public void Start(Context context)
	{
	}

	public void Stop(Context context)
	{
	}
}
