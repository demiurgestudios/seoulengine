/**
 * \file UILetterboxPass.h
 * \brief A poseable used to wrap UILetterbox.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_LETTERBOX_PASS_H
#define UI_LETTERBOX_PASS_H

#include "IPoseable.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
namespace Seoul { class DataStoreTableUtil; }
namespace Seoul { class RenderPass; }
namespace Seoul { namespace UI { class Letterbox; } }

namespace Seoul::UI
{

class LetterboxPass SEOUL_SEALED : public IPoseable
{
public:
	static IPoseable* SpawnUILetterboxPass(
		const DataStoreTableUtil& configSection,
		Bool& rbRenderPassOwnsPoseableObject);

	LetterboxPass(const DataStoreTableUtil& configSection);
	~LetterboxPass();

	// IPoseable overrides
	virtual void Pose(
		Float fDeltaTime,
		RenderPass& rPass,
		IPoseable* pParent = nullptr) SEOUL_OVERRIDE;
	// /IPoseable overrides

private:
	ScopedPtr<Letterbox> m_pLetterbox;

	SEOUL_DISABLE_COPY(LetterboxPass);
}; // class UI::LetterboxPass

} // namespace Seoul::UI

#endif // include guard
