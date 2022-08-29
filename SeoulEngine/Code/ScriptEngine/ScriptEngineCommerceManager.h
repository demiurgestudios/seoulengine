/**
 * \file ScriptEngineCommerceManager.h
 * \brief Binder instance for exposing a CommerceManager instance into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_COMMERCE_MANAGER_H
#define SCRIPT_ENGINE_COMMERCE_MANAGER_H

#include "Prereqs.h"
#include "EngineVirtuals.h"
#include "CommerceManager.h"
namespace Seoul { namespace Script { class FunctionInterface; } }

namespace Seoul
{

/** Binder, wraps a CommerceManager instance and exposes functionality to a script VM. */
class ScriptEngineCommerceManager SEOUL_SEALED
{
public:
	ScriptEngineCommerceManager();
	~ScriptEngineCommerceManager();

	void PurchaseItem(HString sItemID);
	String GetItemPrice(HString sItemID) const;
	void FormatPrice(Script::FunctionInterface* pInterface) const;
	Bool HasAllItemInfo() const;
	void OnItemPurchaseFinalized(HString sItemID, const String& sFirstPartyTransactionID);
	// Gets the name of the platform the CommerceManager uses for its microtransaction products.
	HString GetCommercePlatformId() const;
	void GetProductInfo(Script::FunctionInterface* pInterface) const;

private:
	SEOUL_DISABLE_COPY(ScriptEngineCommerceManager);
}; // class ScriptEngineCommerceManager

} // namespace Seoul

#endif // include guard
