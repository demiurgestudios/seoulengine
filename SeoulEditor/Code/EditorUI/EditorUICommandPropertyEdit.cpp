/**
 * \file EditorUICommandPropertyEdit.cpp
 * \brief Subclass of UndoAction for mutations via generic
 * reflection properties.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EditorUICommandPropertyEdit.h"
#include "ReflectionDefine.h"
#include "ReflectionProperty.h"

namespace Seoul
{

SEOUL_TYPE(EditorUI::IPropertyChangeBinder, TypeFlags::kDisableNew);

namespace EditorUI
{

// TODO: Break this out.
template <typename T, int MEMORY_BUDGETS>
static inline Bool Equals(
	const Vector<T, MEMORY_BUDGETS>& vA,
	const Vector<T, MEMORY_BUDGETS>& vB)
{
	typedef Vector<T, MEMORY_BUDGETS> VectorType;

	if (vA.GetSize() != vB.GetSize())
	{
		return false;
	}

	typename VectorType::ConstIterator iB = vB.Begin();
	for (typename VectorType::ConstIterator iA = vA.Begin(); vA.End() != iA; ++iA, ++iB)
	{
		if (*iA != *iB)
		{
			return false;
		}
	}

	return true;
}

static String ToDescription(
	IPropertyChangeBinder* pBinder,
	const PropertyUtil::Path& vPath)
{
	// TODO: Update return to include the value.
	String sReturn("Edit " + pBinder->GetDescription());
	for (UInt32 i = 0u; i < vPath.GetSize(); ++i)
	{
		sReturn.Append('.');
		if (vPath[i].m_Id.IsEmpty())
		{
			sReturn.Append(String::Printf("%u", vPath[i].m_uId));
		}
		else
		{
			sReturn.Append(String(vPath[i].m_Id));
		}
	}
	return sReturn;
}

CommandPropertyEdit::CommandPropertyEdit(
	IPropertyChangeBinder* pBinder,
	const PropertyUtil::Path& vPath,
	const IPropertyChangeBinder::Values& vOldValues,
	const IPropertyChangeBinder::Values& vNewValues)
	: DevUI::Command(false)
	, m_pBinder(pBinder)
	, m_vPath(vPath)
	, m_vOldValues(vOldValues)
	, m_vNewValues(vNewValues)
	, m_sDescription(ToDescription(pBinder, vPath))
{
}

CommandPropertyEdit::~CommandPropertyEdit()
{
}

void CommandPropertyEdit::Do()
{
	if (m_vNewValues.GetSize() == 1u)
	{
		m_pBinder->SetValue(m_vPath, m_vNewValues.Front());
	}
	else
	{
		m_pBinder->SetValues(m_vPath, m_vNewValues);
	}
}

const String& CommandPropertyEdit::GetDescription() const
{
	return m_sDescription;
}

UInt32 CommandPropertyEdit::GetSizeInBytes() const
{
	auto const uOldSizeInBytes = (UInt32)(m_vOldValues.IsEmpty() ? 0u : m_vOldValues.Front().GetTypeInfo().GetSizeInBytes());
	auto const uNewSizeInBytes = (UInt32)(m_vNewValues.IsEmpty() ? 0u : m_vNewValues.Front().GetTypeInfo().GetSizeInBytes());

	// TODO: Calculate size of the values themselves.
	return (UInt32)(
		sizeof(*this) +
		m_pBinder->GetSizeInBytes() +
		m_vPath.GetCapacityInBytes() +
		m_vOldValues.GetSize() * uOldSizeInBytes +
		m_vNewValues.GetSize() * uNewSizeInBytes +
		m_sDescription.GetCapacity());
}

void CommandPropertyEdit::Undo()
{
	m_pBinder->SetValues(m_vPath, m_vOldValues);
}

Bool CommandPropertyEdit::DoMerge(DevUI::Command const* pCommand)
{
	CommandPropertyEdit const* p = DynamicCast<CommandPropertyEdit const*>(pCommand);

	// Can only merge CommandPropertyEdit commands.
	if (nullptr == p)
	{
		return false;
	}

	// Binders must be equal.
	if (!m_pBinder->Equals(p->m_pBinder.Get()))
	{
		return false;
	}

	// Path must be equal.
	if (!Equals(m_vPath, p->m_vPath))
	{
		return false;
	}

	// Merge the new value.
	m_vNewValues = p->m_vNewValues;
	return true;
}

} // namespace EditorUI

SEOUL_BEGIN_TYPE(EditorUI::CommandPropertyEdit, TypeFlags::kDisableNew)
	SEOUL_PARENT(DevUI::Command)
SEOUL_END_TYPE()

} // namespace Seoul
