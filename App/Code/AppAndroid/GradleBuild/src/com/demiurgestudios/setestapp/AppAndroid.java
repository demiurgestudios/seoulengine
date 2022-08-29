/**
 * \file AppAndroid.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.setestapp;

import com.demiurgestudios.seoul.AndroidNativeActivity;

public class AppAndroid extends AndroidNativeActivity
{
	public AppAndroid()
	{
		Configure(true, AppAndroid.class.getSimpleName());
	}
}
