/**
 * \file Animation3DData.h
 * \brief Binds runtime data ptr into the common animation
 * framework.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION3D_DATA_H
#define ANIMATION3D_DATA_H

#include "Animation3DDataDefinition.h"
#include "AnimationIData.h"

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul::Animation3D
{

class Data SEOUL_SEALED : public Animation::IData
{
public:
	Data(const Animation3DDataContentHandle& hData);
	~Data();

	virtual void AcquireInstance() SEOUL_OVERRIDE;
	virtual IData* Clone() const SEOUL_OVERRIDE;
	virtual Atomic32Type GetTotalLoadsCount() const SEOUL_OVERRIDE;
	virtual Bool HasInstance() const SEOUL_OVERRIDE;
	virtual Bool IsLoading() const SEOUL_OVERRIDE;
	virtual void ReleaseInstance() SEOUL_OVERRIDE;

	const Animation3DDataContentHandle& GetHandle() const
	{
		return m_hData;
	}
	const SharedPtr<DataDefinition const>& GetPtr() const
	{
		return m_pData;
	}

private:
	SEOUL_DISABLE_COPY(Data);

	Animation3DDataContentHandle const m_hData;
	SharedPtr<DataDefinition const> m_pData;
}; // class Data

} // namespace Seoul::Animation3D

#endif // /SEOUL_WITH_ANIMATION_3D

#endif // include guard
