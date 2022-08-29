/**
 * \file AndroidCommerceProductType.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

/**
 * Product Type.
 *
 * NOTE: Keep this in sync with the CommerceManager::ProductType enum
 */
public enum AndroidCommerceProductType
{
	/** Consumable IAP */
	kProductTypeConsumable,
	/** DLC / Entitlement */
	kProductTypeDLC,
	/** Subscription IAP */
	kProductTypeSubscription,
}
