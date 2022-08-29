/*
	ScriptEngineCommerceManager.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class ScriptEngineCommerceManager
	{
		public abstract void PurchaseItem(string a0);
		[Pure] public abstract string GetItemPrice(string a0);
		[Pure] public abstract (string, double) FormatPrice(double iAmountInSmallestUnits, string sCurrencyName);
		[Pure] public abstract bool HasAllItemInfo();
		public abstract void OnItemPurchaseFinalized(string sItemID, string sFirstPartyTransactionID);
		[Pure] public abstract string GetCommercePlatformId();
		[Pure] public abstract SlimCS.Table GetProductInfo(string sItemID);
	}
}
