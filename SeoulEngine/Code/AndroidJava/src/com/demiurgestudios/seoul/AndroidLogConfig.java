/**
 * \file AndroidLogConfig.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

public final class AndroidLogConfig
{
	// Important to use boolean vs. Boolean here - Boolean is a class ref type
	// and does not facilitate compile time optimizations. boolean is a value type
	// and will enable compile time optimizations (if BuildConfig.DEBUG is false,
	// if (ENABLED) will be ellided by the Java compiler at compiler time).
	public final static boolean ENABLED = BuildConfig.DEBUG;

	public final static String TAG = "Seoul";
}
