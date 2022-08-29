/**
 * \file AndroidCommerceApi.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import java.util.List;

/**
 * Base class for various backend specific
 * implementations of commerce on Android.
 *
 * Composite member of AndroidCommerceManager.
 */
public abstract class AndroidCommerceApi
{
	// Shutdown API.
	public abstract void Dispose();

	// Core public API.
	public abstract void ConsumeItem(final String sProductID, final String sTransactionID);
	public abstract void AcknowledgeItem(final String sProductID, final String sTransactionID);
	public abstract void PurchaseItem(final String sProductID);
	public abstract void RefreshProductInfo(final String[] skus);

	// Additional API - used in special circumstances.
	public abstract PurchaseData GetPurchaseData(final String sProductID);

	/**
	 * Used by an owner (currently AndroidCommerceManager) to instantiate
	 * a selected backend API.
	 */
	public interface Factory
	{
		AndroidCommerceApi Create(final Listener listener);
	}

	/**
	 * Listener API used to dispatch result events to an interesting
	 * party (currently, the AndroidCommerceManager that owns the API
	 * instance).
	 */
	public interface Listener
	{
		void OnConsumeComplete(final AndroidCommerceResult eResult, final String sProductID);
		void OnAcknowledgeComplete(final AndroidCommerceResult eResult, final String sProductID);
		void OnInventoryUpdated(final PurchaseData[] purchaseData);
		void OnReceivedCompletedConsumableTransactions(final PurchaseData[] purchaseData);
		void OnPurchaseComplete(final AndroidCommerceResult eResult, final List<PurchaseData> purchaseData);
		void OnRefreshProductInfoComplete(final boolean bSuccess, final ProductInfo[] info);
	}

	/**
	 * Describes an SKU - name, description, and price. Used for presenting purchasing
	 * information to a user.
	 */
	public final class ProductInfo
	{
		public final AndroidCommerceProductType Type;
		public final String ProductID;
		public final String Name;
		public final String Description;
		public final String PriceString;
		public final String CurrencyCode;
		public final long PriceMicros;

		public ProductInfo(
			final AndroidCommerceProductType eType,
			final String sProductID,
			final String sName,
			final String sDescription,
			final String sPriceString,
			final String sCurrencyCode,
			final long lPriceMicros)
		{
			Type = eType;
			ProductID = sProductID;
			Name = sName;
			Description = sDescription;
			PriceString = sPriceString;
			CurrencyCode = sCurrencyCode;
			PriceMicros = lPriceMicros;
		}
	}

	/**
	 * Describes a single purchase made by a user, success or failure.
	 */
	public final class PurchaseData
	{
		public final String ProductID;
		public final String ReceiptData;
		public final String TransactionID;
		public final String PurchaseToken;

		public PurchaseData(
			final String sProductID,
			final String sReceiptData,
			final String sTransactionID,
			final String sPurchaseToken)
		{
			ProductID = sProductID;
			ReceiptData = sReceiptData;
			TransactionID = sTransactionID;
			PurchaseToken = sPurchaseToken;
		}
	}
}
