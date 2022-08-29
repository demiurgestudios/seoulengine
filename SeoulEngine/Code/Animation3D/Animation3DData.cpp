/**
* \file Animation3DData.cpp
* \brief Binds runtime data ptr into the common animation
* framework.
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#include "Animation3DClipDefinition.h"
#include "Animation3DData.h"

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul::Animation3D
{

Data::Data(const Animation3DDataContentHandle& hData)
	: m_hData(hData)
	, m_pData()
{
}

Data::~Data()
{
}

void Data::AcquireInstance()
{
	m_pData = m_hData.GetPtr();
}

Animation::IData* Data::Clone() const
{
	return SEOUL_NEW(MemoryBudgets::Animation3D) Data(m_hData);
}

Atomic32Type Data::GetTotalLoadsCount() const
{
	return m_hData.GetTotalLoadsCount();
}

Bool Data::HasInstance() const
{
	return m_pData.IsValid();
}

Bool Data::IsLoading() const
{
	return m_hData.IsLoading();
}

void Data::ReleaseInstance()
{
	m_pData.Reset();
}

} // namespace Seoul::Animation3D

#endif // /#if SEOUL_WITH_ANIMATION_3D
