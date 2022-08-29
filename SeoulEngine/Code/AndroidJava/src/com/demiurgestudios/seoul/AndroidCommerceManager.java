/**
 * \file AndroidCommerceManager.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

public final class AndroidCommerceManager implements AndroidCommerceApi.Listener
{
	public static native void NativeSetProductsInfo(boolean bSuccess, AndroidCommerceApi.ProductInfo[] arr);
	public static native void NativeTransactionCompleted(String sProductID, String sTransactionID, String sReceiptData, String sPurchaseToken, int eResult);
	public static native void NativeInventoryUpdated(AndroidCommerceApi.PurchaseData[] arr);

	/** Describes the set of tasks that can be enqueued in the commerce manager. */
	enum ETaskKind
	{
		/** Initial task to create the API on the UI thread. */
		Create,

		/** Remove an already purchased consumable item from the user's inventory. */
		Consume,

		/** Acknowledge a subscription item from from the user's inventory. */
		Acknowledge,

		/** Start the purchase flow. */
		Purchase,

		/** Query for updated product info given a list of skus. */
		Refresh,

		/** Destruction even - all other events suppressed after this call. */
		Dispose,
	}

	/** Enqueued work for the commerce system. */
	final class Task
	{
		public final ETaskKind Kind;
		public final Object Data;

		public Task(final ETaskKind eTaskKind, final Object data)
		{
			Kind = eTaskKind;
			Data = data;
		}
	}

	/** Parameters for the Consume / Acknowledge ETaskKind */
	final class FinishTaskParams
	{
		public final String ProductID;
		public final String TransactionID;

		public FinishTaskParams(final String sProductID, final String sTransactionID)
		{
			ProductID = sProductID;
			TransactionID = sTransactionID;
		}
	}

	final LinkedList<Task> m_Tasks = new LinkedList<Task>();
	final Activity m_Activity;
	final boolean m_bDebugEnabled;
	AndroidCommerceApi m_Api = null;
	Task m_TaskInProgress = null;
	boolean m_bDisposed = false;

	/** Logging utility. */
	void LogMessage(String sMessage)
	{
		// Early out if not logging.
		if (!m_bDebugEnabled)
		{
			return;
		}

		if (AndroidLogConfig.ENABLED)
		{
			Log.d(AndroidLogConfig.TAG, "[AndroidCommerceManager]: " + sMessage);
		}
	}

	/**
	 * Called to pull a new task off the task list - nop if a task is already in progress.
	 */
	synchronized void poll()
	{
		if (AndroidLogConfig.ENABLED) { LogMessage("poll:"); }

		// Already a task in progress, nothing to do.
		if (null != m_TaskInProgress)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("poll: task is already running, doing nothing."); }
			return;
		}

		// Grab the next task and try to run it.
		m_TaskInProgress = m_Tasks.poll();
		runTaskInProgress();
	}

	/**
	 * Run the current task in progress - expected to be called once only
	 * for a given task.
	 */
	synchronized void runTaskInProgress()
	{
		// For synchronization.
		final AndroidCommerceManager self = this;

		if (AndroidLogConfig.ENABLED) { LogMessage("runTaskInProgress:"); }

		// No task, nothing to do.
		if (null == m_TaskInProgress)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("runTaskInProgress: no task to run, doing nothing."); }
			return;
		}

		if (AndroidLogConfig.ENABLED) { LogMessage("runTaskInProgress: running next task: " + m_TaskInProgress.Kind.toString()); }

		// Run the task on the UI thread.
		m_Activity.runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				synchronized (self)
				{
					// Check again - make sure the task did not complete before we
					// had a chance to run.
					if (null == m_TaskInProgress)
					{
						return;
					}

					// Don't access m_TaskInProgress after this switch,
					// since it may have been cleared by a callback.
					final Task task = m_TaskInProgress;
					switch (m_TaskInProgress.Kind)
					{
					case Create:
						// Construct the API immediately.
						m_Api = ((AndroidCommerceApi.Factory)task.Data).Create(self);

						// Immediately check and process the next task, if there is one.
						m_TaskInProgress = null;
						poll();
						break;
					case Consume:
						m_Api.ConsumeItem(((FinishTaskParams)task.Data).ProductID, ((FinishTaskParams)task.Data).TransactionID);
						break;
					case Acknowledge:
						m_Api.AcknowledgeItem(((FinishTaskParams)task.Data).ProductID, ((FinishTaskParams)task.Data).TransactionID);
						break;
					case Purchase:
						m_Api.PurchaseItem((String)task.Data);
						break;
					case Refresh:
						m_Api.RefreshProductInfo((String[])task.Data);
						break;
					case Dispose:
						m_Api.Dispose();
						break;
					}
				}
			}
		});
	}

	// AndroidCommerceApi.Listener overrides
	public synchronized void OnConsumeComplete(final AndroidCommerceResult eResult, final String sProductID)
	{
		Finish(ETaskKind.Consume, "OnConsumeComplete", eResult, sProductID);
	}

	public synchronized void OnAcknowledgeComplete(final AndroidCommerceResult eResult, final String sProductID)
	{
		Finish(ETaskKind.Acknowledge, "OnAcknowledgeComplete", eResult, sProductID);
	}

	public synchronized void OnInventoryUpdated(AndroidCommerceApi.PurchaseData[] purchases)
	{
		if (null == purchases) { purchases = new AndroidCommerceApi.PurchaseData[0]; } // Sanity.

		if (AndroidLogConfig.ENABLED) { LogMessage("OnInventoryUpdated: " + String.valueOf(purchases.length) + " inventory results."); }

		// Report inventory
		NativeInventoryUpdated(purchases);

		// An inventory event never interacts with the task list, but we use
		// it as an opportunity to run the next task, if there is one.
		if (null == m_TaskInProgress)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("OnInventoryUpdated: polling for next task."); }
			poll();
		}
	}

	public synchronized void OnReceivedCompletedConsumableTransactions(AndroidCommerceApi.PurchaseData[] purchases)
	{
		if (null == purchases) { purchases = new AndroidCommerceApi.PurchaseData[0]; } // Sanity.

		if (AndroidLogConfig.ENABLED) { LogMessage("OnReceivedCompletedConsumableTransactions: " + String.valueOf(purchases.length) + " inventory results."); }

		// Report purchases - always successful when OnPendingInventory() has been called.
		final int iResult = AndroidCommerceResult.kResultSuccess.ordinal();
		for (AndroidCommerceApi.PurchaseData purchase : purchases)
		{
			// Report.
			NativeTransactionCompleted(
				purchase.ProductID,
				purchase.TransactionID,
				purchase.ReceiptData,
				purchase.PurchaseToken,
				iResult);
		}

		// This event never interacts with the task list, but we use
		// it as an opportunity to run the next task, if there is one.
		if (null == m_TaskInProgress)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("OnReceivedCompletedConsumableTransactions: polling for next task."); }
			poll();
		}
	}

	public synchronized void OnPurchaseComplete(final AndroidCommerceResult eResult, final List<AndroidCommerceApi.PurchaseData> purchaseData)
	{
		if (AndroidLogConfig.ENABLED) { LogMessage("OnPurchaseComplete:"); }

		// Report success or failure. Also check if we have the pending
		// id in the list of purchase data.
		final int iResult = eResult.ordinal();
		final String sPendingProductID = ((null != m_TaskInProgress && m_TaskInProgress.Kind == ETaskKind.Purchase)
			? ((String)m_TaskInProgress.Data)
			: "");

		// Report purchases explicitly provided.
		boolean bReportedPending = false;
		if (null != purchaseData)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("OnPurchaseComplete: reporting " + String.valueOf(purchaseData.size()) + " explicit purchase data with result '" + eResult.toString() + "'."); }
			for (AndroidCommerceApi.PurchaseData purchase : purchaseData)
			{
				// Report.
				NativeTransactionCompleted(
					purchase.ProductID,
					purchase.TransactionID,
					purchase.ReceiptData,
					purchase.PurchaseToken,
					iResult);

				// Check if we found the pending.
				if (purchase.ProductID.equals(sPendingProductID))
				{
					bReportedPending = true;
				}
			}
		}

		// If we get here and haven't yet reported the pending id
		// (and we have a pending id), report it now.
		if (!bReportedPending && null != sPendingProductID && !sPendingProductID.isEmpty())
		{
			bReportedPending = true;
			if (AndroidLogConfig.ENABLED) { LogMessage("OnPurchaseComplete: reporting pending product '" + sPendingProductID + "' separately with result '" + eResult.toString() + "', was not in explicit results."); }

			// If the purchase was successful but we need to report it separately, try
			// to query for the purchase details from the API inventory.
			final AndroidCommerceApi.PurchaseData pendingData = m_Api.GetPurchaseData(sPendingProductID);
			if (null != pendingData)
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("OnPurchaseComplete: pending data found for implicit purchase '" + sPendingProductID + "': '" + pendingData.TransactionID + "'."); }

				// Report.
				NativeTransactionCompleted(
					sPendingProductID,
					pendingData.TransactionID,
					pendingData.ReceiptData,
					pendingData.PurchaseToken,
					iResult);
			}
			else
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("OnPurchaseComplete: no pending data found for implicit purchase '" + sPendingProductID + "', reporting without transaction ID or receipt data."); }

				// We must not report kResultSuccess if we have no transaction ID or receipt data, so we convert to PlatformSpecificError5.
				if (eResult == AndroidCommerceResult.kResultSuccess)
				{
					if (AndroidLogConfig.ENABLED) { LogMessage("OnPurchaseComplete: WARNING: success for implicit purchase '" + sPendingProductID + "' but no purchase data found, converting to failure!"); }

					// Report.
					NativeTransactionCompleted(
						sPendingProductID,
						"",
						"",
						"",
						AndroidCommerceResult.PlatformSpecificError5.ordinal());
				}
				else
				{
					// Report.
					NativeTransactionCompleted(
						sPendingProductID,
						"",
						"",
						"",
						iResult);
				}
			}
		}
		else if (!bReportedPending)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("OnPurchaseComplete: pending product not found and not reported, no callback to native with this result '" + eResult.toString() + "'."); }
		}

		// Now resolve the task. We always resolve a purchase task.
		if (null != m_TaskInProgress && m_TaskInProgress.Kind == ETaskKind.Purchase)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("OnPurchaseComplete: completed purchase, running next task."); }
			m_TaskInProgress = null;
			poll();
		}
		// An unexpected purchase event is not entirely unexpected (it can be triggered
		// by interrupted purchases). The only action to take here is to schedule a new
		// task if we don't have an active one.
		else
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("OnPurchaseComplete: unexpected purchase completion event, checking for new task to schedule."); }

			// If no expected test, poll for a new one.
			if (null == m_TaskInProgress)
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("OnPurchaseComplete: after unexpected completion, polling for next task."); }
				poll();
			}
		}
	}

	public synchronized void OnRefreshProductInfoComplete(final boolean bSuccess, AndroidCommerceApi.ProductInfo[] info)
	{
		if (null == info) { info = new AndroidCommerceApi.ProductInfo[0]; } // Sanity.

		if (AndroidLogConfig.ENABLED) { LogMessage("OnRefreshProductInfoComplete: result '" + (bSuccess ? "success" : "failure") + "'."); }
		// Report success or failure.
		NativeSetProductsInfo(bSuccess, info);

		// Now resolve next task.
		if (null != m_TaskInProgress && m_TaskInProgress.Kind == ETaskKind.Refresh)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("OnRefreshProductInfoComplete: complete, running next task."); }
			m_TaskInProgress = null;
			poll();
		}
		// Otherwise, run what appears to be a different head task.
		else
		{
			// If no expected test, poll for a new one.
			if (null == m_TaskInProgress)
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("OnRefreshProductInfoComplete: unexpected, completed, checking for new task to schedule."); }
				poll();
			}
		}
	}
	// /AndroidCommerceApi.Listener

	public synchronized void Finish(ETaskKind taskKind, String sLogName, final AndroidCommerceResult eResult, final String sProductID)
	{
		// Now resolve the task.
		if (null != m_TaskInProgress &&
			m_TaskInProgress.Kind == taskKind &&
			((FinishTaskParams)m_TaskInProgress.Data).ProductID.equals(sProductID))
		{
			if (AndroidLogConfig.ENABLED) { LogMessage(sLogName + " complete '" + sProductID + "', result '" + String.valueOf(eResult) + "'."); }

			// On temporary failure, try again.
			if (eResult == AndroidCommerceResult.kResultNetworkError)
			{
				if (AndroidLogConfig.ENABLED) { LogMessage(sLogName + " failure of '" + sProductID + "' due to (assumed) temporary network error, retrying."); }
				runTaskInProgress();
			}
			// Permanent failure or success, consume and queue next task.
			else
			{
				if (AndroidLogConfig.ENABLED) { LogMessage(sLogName + " success or permanent failure of '" + sProductID + "', running next task."); }
				m_TaskInProgress = null;
				poll();
			}
		}
		// Somehow we got a consumption event but that wasn't the task we were waiting for.
		// This is unexpected, but we handle it by just running the next task (if there is one).
		else
		{
			if (AndroidLogConfig.ENABLED)
			{
				LogMessage(sLogName + " unexpected completion of '" + sProductID + "', result '" + String.valueOf(eResult) +
					"', but this was not the task we were expecting. Expected task is: " + (null != m_TaskInProgress ? String.valueOf(m_TaskInProgress.Kind) : "<null>"));
			}

			// If no expected test, poll for a new one.
			if (null == m_TaskInProgress)
			{
				LogMessage(sLogName + " after unexpected completion, polling for next task.");
				poll();
			}
		}
	}

	public AndroidCommerceManager(final Activity activity, final AndroidCommerceApi.Factory apiFactory, final boolean bDebugEnabled)
	{
		m_Activity = activity;
		m_bDebugEnabled = bDebugEnabled;

		// Kick the task to create the API.
		m_Tasks.add(new Task(ETaskKind.Create, apiFactory));
		// Kick it.
		poll();
	}

	/**
	 * Call when this CommerceManager is about to be released. After
	 * a call to Dispose(), no further API can be called on this
	 * CommerceManager.
	 */
	public synchronized void Dispose()
	{
		// To handle this correctly, we:
		// - set a boolean so the public api is now ignored.
		// - immediately clear the task queue.
		// - add Dispose as the only action.
		//
		// This allows the running task to complete but
		// does not attempt to process any further tasks.
		m_bDisposed = true;
		m_Tasks.clear();
		m_Tasks.add(new Task(ETaskKind.Dispose, null));
	}

	/**
	 * Call to remove a purchased item from the user's inventory.
	 *
	 * For consumable IAPs, this is called after the user has
	 * been granted their rewards and those rewards have been
	 * committed to persistent storage.
	 */
	public synchronized void ConsumeItem(final String sProductID, final String sTransactionID)
	{
		try
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("ConsumeItem: " + sProductID); }

			// Nop if disposed.
			if (m_bDisposed)
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("ConsumeItem: Disposed"); }
				return;
			}

			// Enqueue for processing.
			m_Tasks.add(new Task(ETaskKind.Consume, new FinishTaskParams(sProductID, sTransactionID)));
			poll();
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	public synchronized void AcknowledgeItem(final String sProductID, final String sTransactionID)
	{
		try
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("AcknowledgeItem: " + sProductID); }

			// Nop if disposed.
			if (m_bDisposed)
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("AcknowledgeItem: Disposed"); }
				return;
			}

			// Enqueue for processing.
			m_Tasks.add(new Task(ETaskKind.Acknowledge, new FinishTaskParams(sProductID, sTransactionID)));
			poll();
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	/**
	 * Call to start the purchase flow for an item.
	 */
	public synchronized void PurchaseItem(final String sProductID)
	{
		try
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("PurchaseItem: " + sProductID); }

			// Nop if disposed.
			if (m_bDisposed)
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("PurchaseItem: Disposed"); }
				return;
			}

			// Enqueue for processing.
			m_Tasks.add(new Task(ETaskKind.Purchase, sProductID));
			poll();
		}
		catch (Exception e)
		{
			e.printStackTrace();
			NativeTransactionCompleted(sProductID, "", "", "", AndroidCommerceResult.kPlatformSpecificError1.ordinal());
		}
	}

	/**
	 * Call to refresh the product information for a given list of skus.
	 */
	public synchronized void RefreshProductInfo(final String[] skus)
	{
		try
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("RefreshProductInfo:"); }

			// Nop if disposed.
			if (m_bDisposed)
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("RefreshProductInfo: Disposed."); }
				return;
			}

			// Enqueue for processing.
			m_Tasks.add(new Task(ETaskKind.Refresh, skus));
			poll();
		}
		catch (Exception e)
		{
			e.printStackTrace();
			NativeSetProductsInfo(false, new AndroidCommerceApi.ProductInfo[0]);
		}
	}
}
