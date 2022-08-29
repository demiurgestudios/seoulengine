/**
 * \file AndroidSamsungImpl.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

// Null variation - used on projects that don't include Samsung support.
public final class AndroidSamsungImpl
{
	public static AndroidCommerceApi CreateCommerceManager(
		final AndroidNativeActivity activity,
		final AndroidCommerceApi.Listener listener,
		final boolean bDebugEnabled)
	{
		return new AndroidCommerceGooglePlayApi(activity, listener, bDebugEnabled);
	}
}
