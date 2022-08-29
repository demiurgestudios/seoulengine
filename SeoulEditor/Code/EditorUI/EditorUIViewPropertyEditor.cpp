/**
 * \file EditorUIViewPropertyEditor.cpp
 * \brief An editor view that displays
 * a tool for working with hierarchical data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DevUIController.h"
#include "DevUIImGui.h"
#include "EditorUIRoot.h"
#include "EditorUIDragSourceFilePath.h"
#include "EditorUIIControllerPropertyEditor.h"
#include "EditorUIUtil.h"
#include "EditorUIViewPropertyEditor.h"
#include "Engine.h"
#include "GamePaths.h"
#include "Quaternion.h"
#include "ReflectionArray.h"
#include "ReflectionAttribute.h"
#include "ReflectionAttributes.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionEnum.h"
#include "ReflectionMethod.h"
#include "ReflectionTable.h"
#include "ReflectionType.h"
#include "ScopedAction.h"
#include "Vector3D.h"

namespace Seoul::EditorUI
{

using namespace ViewPropertyEditorUtil;

static const HString kMethodEdit("Edit");

static inline Bool DragFloat(
	Float& f,
	const Reflection::AttributeCollection& attributes,
	ImGuiInputTextFlags eFlags)
{
	using namespace Reflection;
	using namespace Reflection::Attributes;

	Float fMin = 0.0f;
	Float fMax = 0.0f;

	if (auto pRange = attributes.GetAttribute<Range>())
	{
		(void)SimpleCast(pRange->m_Min, fMin);
		(void)SimpleCast(pRange->m_Max, fMax);
	}

	// TODO: Derive precisions and step from attributes.
	return ImGui::DragFloat("", &f, 1.0f, fMin, fMax, "%.3f", 0, eFlags);
}

template <size_t N>
static inline Bool DragFloatN(
	Float (&af)[N],
	const Reflection::AttributeCollection& attributes,
	ImGuiInputTextFlags (&aeFlags)[N])
{
	SEOUL_STATIC_ASSERT(N <= 4);

	static Byte const* kasLabels[] = { "X", "Y", "Z", "W" };

	using namespace Reflection;
	using namespace Reflection::Attributes;

	Float fMin = 0.0f;
	Float fMax = 0.0f;

	if (auto pRange = attributes.GetAttribute<Range>())
	{
		(void)SimpleCast(pRange->m_Min, fMin);
		(void)SimpleCast(pRange->m_Max, fMax);
	}

	// TODO: Derive precisions and step from attributes.
	return ImGui::DragFloatNEx(kasLabels, af, N, 1.0f, fMin, fMax, "%.3f", aeFlags);
}

// Special case handling of (read only) DataStore visualization.
static void ReadOnlyValue(const DataStore& ds, const DataNode& dn)
{
	using namespace ImGui;

	static const Float32 kfMinWidth = 100.0f; // TODO:

	switch (dn.GetType())
	{
	case DataNode::kArray:
	{
		UInt32 u = 0u;
		SEOUL_VERIFY(ds.GetArrayCount(dn, u));
		for (UInt32 i = 0u; i < u; ++i)
		{
			DataNode arrValue;
			SEOUL_VERIFY(ds.GetValueFromArray(dn, i, arrValue));

			if (arrValue.IsArray() || arrValue.IsTable())
			{
				if (TreeNodeEx(((void*)((size_t)i)), ImGuiTreeNodeFlags_SpanFullWidth, "%u", i))
				{
					ReadOnlyValue(ds, arrValue);
					TreePop();
				}
			}
			else
			{
				auto const fStart = GetCursorPosX();

				BeginGroup();
				PushID((Int32)i);
				AlignTextToFramePadding();
				Text("Element %d", (Int32)i);
				SameLine();
				SetCursorPosX(fStart + kfMinWidth);
				PushItemWidth(GetContentRegionAvail().x);

				ReadOnlyValue(ds, arrValue);

				PopItemWidth();
				PopID();
				EndGroup();
			}
		}
	}
	break;
	case DataNode::kBoolean:
	{
		Bool b = ds.AssumeBoolean(dn);
		ImGui::Checkbox("", &b, false);
	}
	break;

	case DataNode::kFilePath:
	{
		FilePath filePath;
		SEOUL_VERIFY(ds.AsFilePath(dn, filePath));
		ImGui::Text("%s", filePath.ToSerializedUrl().CStr());
	}
	break;

	case DataNode::kFloat31: ImGui::Text("%g", ds.AssumeFloat31(dn)); break;
	case DataNode::kFloat32: ImGui::Text("%g", ds.AssumeFloat32(dn)); break;

	case DataNode::kInt32Big: ImGui::Text("%d", ds.AssumeInt32Big(dn)); break;
	case DataNode::kInt32Small: ImGui::Text("%d", ds.AssumeInt32Small(dn)); break;
	case DataNode::kInt64: ImGui::Text(PRIi64, ds.AssumeInt64(dn)); break;

	case DataNode::kNull: ImGui::Text("<null>"); break;
	case DataNode::kString:
	{
		Byte const* s = nullptr;
		UInt32 u = 0u;
		SEOUL_VERIFY(ds.AsString(dn, s, u));
		ImGui::Text("%s", s);
	}
	break;
	case DataNode::kTable:
	{
		auto const iBegin = ds.TableBegin(dn);
		auto const iEnd = ds.TableEnd(dn);

		// Measure key width.
		Float32 fWidth = 0.0f;
		for (auto i = iBegin; iEnd != i; ++i)
		{
			fWidth = Max(fWidth, CalcTextSize(i->First.CStr()).x);
		}
		fWidth = Max(kfMinWidth, fWidth + 10.0f);

		// Now display key-value pairs.
		for (auto i = iBegin; iEnd != i; ++i)
		{
			auto const name = i->First;
			auto const value = i->Second;
			if (value.IsArray() || value.IsTable())
			{
				if (TreeNodeEx(i->First.CStr(), ImGuiTreeNodeFlags_SpanFullWidth))
				{
					ReadOnlyValue(ds, value);
					TreePop();
				}
			}
			else
			{
				auto const fStart = GetCursorPosX();

				BeginGroup();
				PushID(name.CStr());
				AlignTextToFramePadding();
				TextUnformatted(name.CStr(), nullptr, true);
				SameLine();
				SetCursorPosX(fStart + fWidth);
				PushItemWidth(GetContentRegionAvail().x);

				ReadOnlyValue(ds, value);

				PopItemWidth();
				PopID();
				EndGroup();
			}
		}
	}
	break;

	case DataNode::kUInt32: ImGui::Text("%u", ds.AssumeUInt32(dn)); break;
	case DataNode::kUInt64: ImGui::Text(PRIu64, ds.AssumeUInt64(dn)); break;

	case DataNode::kSpecialErase:
		// Nop
		break;

	default:
		SEOUL_FAIL("Out-of-sync enum.");
		break;
	};
}

static Bool ReadOnlyValue(DataStore const* pDataStore)
{
	if (nullptr == pDataStore)
	{
		ImGui::Text("<null>");
	}
	else
	{
		ReadOnlyValue(*pDataStore, pDataStore->GetRootNode());
	}

	return false;
}

ViewPropertyEditor::ViewPropertyEditor()
	: m_tValueTypes()
	, m_vStack()
{
	PopulateValueTypes();
}

ViewPropertyEditor::~ViewPropertyEditor()
{
}

void ViewPropertyEditor::DoPrePose(DevUI::Controller& rController, RenderPass& rPass)
{
	IControllerPropertyEditor* pController = DynamicCast<IControllerPropertyEditor*>(&rController);

	m_vStack.Clear();
	if (nullptr == pController ||
		!pController->GetPropertyTargets(m_vStack) ||
		m_vStack.IsEmpty())
	{
		return;
	}

	PropertyUtil::Path vPath;

	UInt32 const uSize = m_vStack.GetSize();
	Complex(*pController, vPath, m_vStack.Front().GetType(), 0u, uSize);
	SEOUL_ASSERT(uSize == m_vStack.GetSize());

	m_vStack.Clear();
}

Bool ViewPropertyEditor::Complex(
	IControllerPropertyEditor& rController,
	PropertyUtil::Path& rvPath,
	const Reflection::Type& type,
	UInt32 uBegin,
	UInt32 uEnd)
{
	using namespace Reflection;

	// Special handling for PointerLike complex objects.
	{
		Attributes::PointerLike const* pPointerLike = type.GetAttribute<Attributes::PointerLike>();
		if (nullptr != pPointerLike)
		{
			UInt32 const uInnerBegin = m_vStack.GetSize();
			for (UInt32 i = uBegin; i < uEnd; ++i)
			{
				WeakAny proxyObjectThis = pPointerLike->m_GetPtrDelegate(m_vStack[i]);
				if (proxyObjectThis.IsValid())
				{
					m_vStack.PushBack(proxyObjectThis);
				}
				else
				{
					m_vStack.Resize(uEnd);
					return false;
				}
			}

			Bool const bReturn = Complex(rController, rvPath, m_vStack[uInnerBegin].GetType(), uInnerBegin, m_vStack.GetSize());
			m_vStack.Resize(uEnd);
			return bReturn;
		}
	}

	// Special case for DataStore read-only value.
	if (type == TypeOf<DataStore>())
	{
		return ComplexDataStore(rController, rvPath, type, uBegin, uEnd);
	}

	// TODO: Array or table support.

	// Standard complex type.
	return ComplexType(rController, rvPath, type, uBegin, uEnd);
}

Bool ViewPropertyEditor::ComplexDataStore(
	IControllerPropertyEditor& rController,
	PropertyUtil::Path& rvPath,
	const Reflection::Type& type,
	UInt32 uBegin,
	UInt32 uEnd)
{
	// TODO: Multiple selection support for DataStore viz.
	if (uEnd - uBegin != 1u)
	{
		// Multiple values, just display a placeholder.
		ImGui::InputText("", nullptr, 0u, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_Indeterminate);
		return false;
	}
	else
	{
		// Single value display.
		auto const value = m_vStack[uBegin];
		if (value.GetTypeInfo() == TypeId<DataStore const*>())
		{
			ReadOnlyValue(value.Cast<DataStore const*>());
		}
		else if (value.GetTypeInfo() == TypeId<DataStore*>())
		{
			ReadOnlyValue(value.Cast<DataStore*>());
		}

		return false;
	}
}

Bool ViewPropertyEditor::ComplexType(
	IControllerPropertyEditor& rController,
	PropertyUtil::Path& rvPath,
	const Reflection::Type& type,
	UInt32 uBegin,
	UInt32 uEnd)
{
	using namespace Reflection;

	// TODO: Support button actions with multiple selection?
	// Buttons first.
	if (uBegin + 1u == uEnd)
	{
		Any context;
		if (rController.GetPropertyButtonContext(context))
		{
			MethodArguments aArguments;
			aArguments[0] = context;

			UInt32 const uMethods = type.GetMethodCount();
			for (UInt32 i = 0u; i < uMethods; ++i)
			{
				const Method& method = *type.GetMethod(i);
				if (auto pButtonAttr = method.GetAttributes().GetAttribute<Attributes::EditorButton>())
				{
					auto pProp = type.GetProperty(pButtonAttr->m_PropertyName);
					if (nullptr == pProp)
					{
						// TODO: Warn.
						continue;
					}

					if (ImGui::Button(method.GetName().CStr()))
					{
						Any oldValue;
						if (!pProp->TryGet(m_vStack[uBegin], oldValue))
						{
							// TODO: Warn.
							continue;
						}

						if (!method.TryInvoke(m_vStack[uBegin], aArguments))
						{
							// TODO: Warn.
						}

						Any newValue;
						if (!pProp->TryGet(m_vStack[uBegin], newValue))
						{
							// TODO: Warn.
							continue;
						}

						// TODO: The design of EditorButton means
						// that the value must have already been committed,
						// which also means we're committing it twice (and
						// also bypassing the controller on the initial
						// commit, which is never desired).

						// Commit the value.
						rvPath.PushBack(PropertyUtil::NumberOrHString(pProp->GetName()));
						rController.CommitPropertyEdit(
							rvPath,
							IControllerPropertyEditor::PropertyValues(1u, oldValue),
							IControllerPropertyEditor::PropertyValues(1u, newValue));
						rvPath.PopBack();
					}
				}
			}
		}
	}

	Bool bReturn = false;
	UInt32 const uProperties = type.GetPropertyCount();
	for (UInt32 i = 0u; i < uProperties; ++i)
	{
		const Property& prop = *type.GetProperty(i);
		bReturn = Prop(rController, rvPath, prop, uBegin, uEnd) || bReturn;
	}

	return bReturn;
}

Bool ViewPropertyEditor::Prop(
	IControllerPropertyEditor& rController,
	PropertyUtil::Path& rvPath,
	const Reflection::Property& prop,
	UInt32 uBegin,
	UInt32 uEnd)
{
	using namespace Reflection;

	if (!prop.CanGet())
	{
		return false;
	}

	// Hide from editor if requested.
	if (nullptr != prop.GetAttributes().GetAttribute<Attributes::DoNotEdit>())
	{
		return false;
	}

	rvPath.PushBack(PropertyUtil::NumberOrHString(prop.GetName()));

	Bool bChanged = false;
	ValueFunc valueFunc;
	if (GetValueFunc(prop, valueFunc))
	{
		bChanged = valueFunc(
			rController,
			rvPath,
			m_Storage,
			m_vStack.Begin() + uBegin,
			m_vStack.Begin() + uEnd,
			prop);
	}
	else
	{
		Bool bOk = true;
		UInt32 const uInnerBegin = m_vStack.GetSize();
		for (UInt32 i = uBegin; i < uEnd; ++i)
		{
			WeakAny nestedValuePtr;
			if (prop.TryGetPtr(m_vStack[i], nestedValuePtr) ||
				prop.TryGetConstPtr(m_vStack[i], nestedValuePtr))
			{
				m_vStack.PushBack(nestedValuePtr);
			}
			else
			{
				bOk = false;
				break;
			}
		}

		// Try to get values into scratch.
		Bool bUseScratch = false;
		UInt32 const uScratchStart = m_vScratch.GetSize();
		if (!bOk)
		{
			bUseScratch = true;
			bOk = true;
			m_vScratch.Resize(uScratchStart + uEnd - uBegin);
			for (UInt32 i = uBegin; i < uEnd; ++i)
			{
				if (prop.TryGet(m_vStack[i], m_vScratch[uScratchStart + i - uBegin]))
				{
					m_vStack.PushBack(m_vScratch[uScratchStart + i - uBegin].GetPointerToObject());
				}
				else
				{
					bOk = false;
					break;
				}
			}
		}

		if (bOk)
		{
			if (ImGui::TreeNodeEx(prop.GetName().CStr(), ImGuiTreeNodeFlags_SpanFullWidth))
			{
				bChanged = Complex(
					rController,
					rvPath,
					m_vStack[uInnerBegin].GetType(),
					uInnerBegin,
					m_vStack.GetSize());
				ImGui::TreePop();
			}

			// On mutation, set the values back.
			if (bChanged && bUseScratch)
			{
				for (UInt32 i = uBegin; i < uEnd; ++i)
				{
					(void)prop.TrySet(m_vStack[i], m_vScratch[uScratchStart + i - uBegin]);
				}
			}
		}

		m_vScratch.Resize(uScratchStart);
		m_vStack.Resize(uEnd);
	}

	rvPath.PopBack();
	return bChanged;
}

template <typename T>
inline static Bool EditInt(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue)
{
	using namespace Reflection;
	using namespace Reflection::Attributes;

	T iValue = (T)0;

	// rValue will be invalid if multi-selection
	// is active and has multiple values.
	ImGuiInputTextFlags eFlags = 0;
	if (rValue.IsValid())
	{
		iValue = rValue.Cast<T>();
	}
	else
	{
		eFlags |= ImGuiInputTextFlags_Indeterminate;
	}

	if (!prop.CanSet())
	{
		eFlags |= ImGuiInputTextFlags_ReadOnly;
	}

	Int iInOutValue = (Int)iValue;
	if (ImGui::InputInt("", &iInOutValue, 1, 100, eFlags))
	{
		rValue = (T)iInOutValue;
		return true;
	}

	return false;
}

template <typename T>
inline static Bool EditUInt(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue)
{
	using namespace Reflection;
	using namespace Reflection::Attributes;

	T uValue = (T)0;

	// rValue will be invalid if multi-selection
	// is active and has multiple values.
	ImGuiInputTextFlags eFlags = 0;
	if (rValue.IsValid())
	{
		uValue = rValue.Cast<T>();
	}
	else
	{
		eFlags |= ImGuiInputTextFlags_Indeterminate;
	}

	if (!prop.CanSet())
	{
		eFlags |= ImGuiInputTextFlags_ReadOnly;
	}

	UInt uInOutValue = (UInt)uValue;
	if (ImGui::InputUInt("", &uInOutValue, 1u, 100u, eFlags))
	{
		rValue = (T)uInOutValue;
		return true;
	}

	return false;
}

static Bool EnumLikeToCString(void* pData, Int iIndex, Byte const** psOut)
{
	auto const& v = *((Reflection::Attributes::EnumLike::Names const*)pData);

	if (iIndex >= 0 && (UInt32)iIndex < v.GetSize())
	{
		*psOut = v[iIndex].CStr();
		return true;
	}

	return false;
}

static Bool HStringToCString(void* pUserData, Int32 iIndex, Byte const** psOut)
{
	using namespace Reflection;
	const EnumNameVector& v = *((EnumNameVector const*)pUserData);
	if (iIndex >= 0 && iIndex < (Int32)v.GetSize())
	{
		*psOut = v[iIndex].CStr();
		return true;
	}

	return false;
}

// Equal functions.
static Bool EnumEqualFunc(
	const ViewPropertyEditor::Stack::ConstIterator& iBegin,
	const ViewPropertyEditor::Stack::ConstIterator& iEnd,
	const Reflection::Property& prop)
{
	using namespace Reflection;

	auto i = iBegin;

	Any anyValue;
	if (!prop.TryGet(*i, anyValue))
	{
		return false;
	}
	++i;

	Int32 iFirstValue = 0;
	SEOUL_VERIFY(SimpleCast(anyValue, iFirstValue));
	for (; iEnd != i; ++i)
	{
		if (!prop.TryGet(*i, anyValue))
		{
			return false;
		}

		Int32 iCurrentValue = 0;
		SEOUL_VERIFY(SimpleCast(anyValue, iCurrentValue));
		if (iCurrentValue != iFirstValue)
		{
			return false;
		}
	}

	return true;
}

static Bool EnumLikeEqualFunc(
	const ViewPropertyEditor::Stack::ConstIterator& iBegin,
	const ViewPropertyEditor::Stack::ConstIterator& iEnd,
	const Reflection::Property& prop)
{
	using namespace Reflection;

	auto i = iBegin;

	Any anyValue;
	if (!prop.TryGet(*i, anyValue))
	{
		return false;
	}
	++i;

	auto const& enumLike = *prop.GetAttributes().GetAttribute<Attributes::EnumLike>();

	HString firstValue;
	enumLike.ValueToName(anyValue, firstValue);
	for (; iEnd != i; ++i)
	{
		if (!prop.TryGet(*i, anyValue))
		{
			return false;
		}

		HString currentValue;
		enumLike.ValueToName(anyValue, currentValue);
		if (currentValue != firstValue)
		{
			return false;
		}
	}

	return true;
}

template <typename T>
static Bool ValueEqualFunc(
	const ViewPropertyEditor::Stack::ConstIterator& iBegin,
	const ViewPropertyEditor::Stack::ConstIterator& iEnd,
	const Reflection::Property& prop)
{
	using namespace Reflection;

	auto i = iBegin;

	Any anyValue;
	if (!prop.TryGet(*i, anyValue))
	{
		return false;
	}
	++i;

	T const firstValue = anyValue.Cast<T>();
	for (; iEnd != i; ++i)
	{
		if (!prop.TryGet(*i, anyValue))
		{
			return false;
		}

		T const currentValue = anyValue.Cast<T>();
		if (currentValue != firstValue)
		{
			return false;
		}
	}

	return true;
}

// Handlers for display/edit of value types.
static Bool EnumValueFuncImpl(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue)
{
	using namespace Reflection;

	Int32 iEnumValue = 0;

	// rValue will be invalid if multi-selection
	// is active and has multiple values.
	Bool bIndeterminate = false;
	if (rValue.IsValid())
	{
		SEOUL_VERIFY(SimpleCast(rValue, iEnumValue));
	}
	else
	{
		bIndeterminate = true;
	}

	const Reflection::Enum& e = *prop.GetMemberTypeInfo().GetType().TryGetEnum();
	const EnumValueVector& v = e.GetValues();
	const EnumNameVector& n = e.GetNames();

	Int32 iValue = 0;
	Vector<Byte const*, MemoryBudgets::Editor> vNames(v.GetSize());
	for (UInt32 i = 0u; i < vNames.GetSize(); ++i)
	{
		vNames[i] = n[i].CStr();
		if (v[i] == iEnumValue)
		{
			iValue = (Int32)i;
		}
	}

	Int32 const iPrevious = iValue;
	if (ImGui::Combo("", &iValue, &HStringToCString, (void*)&n, (Int32)n.GetSize(), -1, prop.CanSet(), bIndeterminate) &&
		iPrevious != iValue)
	{
		rValue = (Int32)v[iValue];
		return true;
	}

	return false;
}

static Bool EnumLikeValueFuncImpl(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue)
{
	using namespace ImGui;
	using namespace Reflection;

	auto const& enumLike = *prop.GetAttributes().GetAttribute<Attributes::EnumLike>();

	Attributes::EnumLike::Names v;
	enumLike.GetNames(v);

	// Get the current name.
	HString currentName;

	// rValue will be invalid if multi-selection
	// is active and has multiple values.
	Bool bIndeterminate = false;
	if (rValue.IsValid())
	{
		enumLike.ValueToName(rValue, currentName);
	}
	else
	{
		bIndeterminate = true;
	}

	auto value = rValue;
	if (currentName.IsEmpty() && !v.IsEmpty())
	{
		currentName = v.Front();
		enumLike.NameToValue(currentName, value);
	}

	// Find the offset into the values array for the value.
	Int iCurrent = 0;
	{
		auto i = v.Find(currentName);
		if (v.End() != i)
		{
			iCurrent = (Int)(i - v.Begin());
		}
		else if (!v.IsEmpty())
		{
			currentName = v.Front();
			enumLike.NameToValue(currentName, value);
			iCurrent = 0;
		}
	}

	// Present and update.
	Int32 const iPrevious = iCurrent;
	if (Combo("", &iCurrent, &EnumLikeToCString, (void*)&v, (Int)v.GetSize(), -1, prop.CanSet(), bIndeterminate) &&
		iCurrent != iPrevious)
	{
		if (iCurrent >= 0 && iCurrent < (Int32)v.GetSize())
		{
			enumLike.NameToValue(v[iCurrent], rValue);
			return true;
		}
	}

	return false;
}

template <typename T>
static Bool ValueFuncImpl(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue);

template <>
static Bool ValueFuncImpl<Bool>(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue)
{
	Bool bValue = false;

	// rValue will be invalid if multi-selection
	// is active and has multiple values.
	Bool bIndeterminate = false;
	if (rValue.IsValid())
	{
		bValue = rValue.Cast<Bool>();
	}
	else
	{
		bIndeterminate = true;
	}

	if (ImGui::Checkbox("", &bValue, prop.CanSet(), bIndeterminate))
	{
		rValue = bValue;
		return true;
	}

	return false;
}

template <>
static Bool ValueFuncImpl<FilePath>(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue)
{
	using namespace Reflection;
	using namespace Reflection::Attributes;

	FilePath oldFilePath;

	// rValue will be invalid if multi-selection
	// is active and has multiple values.
	Bool bIndeterminate = false;
	if (rValue.IsValid())
	{
		oldFilePath = rValue.Cast<FilePath>();
	}
	else
	{
		bIndeterminate = true;
	}

	auto const sDisplayName(oldFilePath.IsValid()
		? Path::GetFileName(oldFilePath.GetRelativeFilenameInSource())
		: String("..."));

	auto const* pFileSpec = prop.GetAttributes().GetAttribute<EditorFileSpec>();
	FileType eFileType = FileType::kUnknown;
	if (oldFilePath.GetType() != FileType::kUnknown)
	{
		eFileType = oldFilePath.GetType();
	}
	else if (nullptr != pFileSpec)
	{
		eFileType = pFileSpec->m_eFileType;
	}

	// Clicking the button opens a file selection browser.
	if (ImGui::Button(sDisplayName.CStr(), ImVec2(0, 0), prop.CanSet(), bIndeterminate))
	{
		const GameDirectory eDirectory = (pFileSpec ? pFileSpec->m_eDirectory : GameDirectory::kUnknown);
		FilePath newFilePath = oldFilePath;
		(void)Engine::Get()->DisplayFileDialogSingleSelection(
			newFilePath,
			Engine::FileDialogOp::kOpen,
			eFileType,
			eDirectory);

		if (newFilePath != oldFilePath)
		{
			rValue = newFilePath;
			return true;
		}
	}

	// Drag and drop handling.
	if (Root::Get()->IsItemDragAndDropTarget() &&
		Root::Get()->GetDragData().m_Data.IsOfType<DragSourceFilePath>())
	{
		// Get the file path, then check the type.
		auto const& droppedFilePath = Root::Get()->GetDragData().m_Data.Cast<DragSourceFilePath>().m_FilePath;

		if (droppedFilePath.GetType() == eFileType)
		{
			// On release, mark that we
			// have a category update to apply.
			if (ImGui::IsMouseReleased(0))
			{
				if (droppedFilePath != oldFilePath)
				{
					rValue = droppedFilePath;
					return true;
				}
			}
			// Otherwise, just mark that
			// we're a valid drop target.
			else
			{
				Root::Get()->MarkCanDrop();
			}
		}
	}

	return false;
}

template <>
static Bool ValueFuncImpl<Float32>(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue)
{
	Float32 fValue = 0.0f;

	// rValue will be invalid if multi-selection
	// is active and has multiple values.
	ImGuiInputTextFlags eFlags = 0;
	if (rValue.IsValid())
	{
		fValue = rValue.Cast<Float32>();
	}
	else
	{
		eFlags |= ImGuiInputTextFlags_Indeterminate;
	}

	if (!prop.CanSet())
	{
		eFlags |= ImGuiInputTextFlags_ReadOnly;
	}

	if (DragFloat(fValue, prop.GetAttributes(), eFlags))
	{
		rValue = fValue;
		return true;
	}

	return false;
}

template <>
static Bool ValueFuncImpl<Int8>(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue)
{
	return EditInt<Int8>(r, prop, rValue);
}

template <>
static Bool ValueFuncImpl<Int16>(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue)
{
	return EditInt<Int16>(r, prop, rValue);
}

template <>
static Bool ValueFuncImpl<Int32>(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue)
{
	return EditInt<Int32>(r, prop, rValue);
}

template <>
static Bool ValueFuncImpl<String>(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue)
{
	static const UInt32 kuOversize = 64u;

	String sValue;

	// rValue will be invalid if multi-selection
	// is active and has multiple values.
	ImGuiInputTextFlags eFlags = 0;
	if (rValue.IsValid())
	{
		sValue = rValue.Cast<String>();
	}
	else
	{
		eFlags |= ImGuiInputTextFlags_Indeterminate;
	}

	if (!prop.CanSet())
	{
		eFlags |= ImGuiInputTextFlags_ReadOnly;
	}

	// Space for additional characters, plus the null terminator.
	r.m_vText.Resize(sValue.GetSize() + 1u + kuOversize);
	if (sValue.GetSize() > 0u)
	{
		memcpy(r.m_vText.Data(), sValue.CStr(), sValue.GetSize());
	}
	r.m_vText[sValue.GetSize()] = '\0';

	if (ImGui::InputText("", r.m_vText.Data(), r.m_vText.GetSize(), eFlags))
	{
		rValue = String(r.m_vText.Data());
		return true;
	}

	return false;
}

template <>
static Bool ValueFuncImpl<HString>(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue)
{
	auto value = rValue;
	if (value.IsValid())
	{
		value = String(value.Cast<HString>());
	}

	auto const bReturn = ValueFuncImpl<String>(r, prop, value);

	if (bReturn)
	{
		if (value.IsValid())
		{
			value = HString(value.Cast<String>());
		}

		rValue = value;
	}

	return bReturn;
}

template <>
static Bool ValueFuncImpl<Byte const*>(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue)
{
	auto value = rValue;
	if (value.IsValid())
	{
		value = String(value.Cast<Byte const*>());
	}

	(void)ValueFuncImpl<String>(r, prop, value);

	return false;
}

template <>
static Bool ValueFuncImpl<UInt8>(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue)
{
	return EditUInt<UInt8>(r, prop, rValue);
}

template <>
static Bool ValueFuncImpl<UInt16>(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue)
{
	return EditUInt<UInt16>(r, prop, rValue);
}

template <>
static Bool ValueFuncImpl<UInt32>(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue)
{
	return EditUInt<UInt32>(r, prop, rValue);
}

/**
 * Value display implementation specific to N-dimensional
 * Vector types (e.g. Vector3D).
 */
template <typename T, UInt32 N>
static Bool ValueFuncImplVectorN(
	Storage& r,
	const Reflection::Property& prop,
	UInt32 uIndeterminant,
	const T& vValue,
	T& rvNewValue)
{
	// Gather set and indeterminate state into per-component flags.
	ImGuiInputTextFlags aeFlags[N];
	memset(aeFlags, 0, sizeof(aeFlags));
	for (UInt32 i = 0u; i < N; ++i)
	{
		aeFlags[i] |= (((1 << i) & uIndeterminant) == (1 << i)) ? ImGuiInputTextFlags_Indeterminate : 0u;
		aeFlags[i] |= !prop.CanSet() ? ImGuiInputTextFlags_ReadOnly : 0u;
	}

	// Gather values for display.
	Float afValue[N];
	for (UInt32 i = 0u; i < N; ++i)
	{
		afValue[i] = vValue[i];
	}

	// Display - on mutation, update output value.
	if (DragFloatN(afValue, prop.GetAttributes(), aeFlags))
	{
		T ret{};
		for (UInt32 i = 0u; i < N; ++i)
		{
			ret[i] = afValue[i];
		}
		rvNewValue = ret;
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Default wrapper for value mutation -
 * combines a value edit with an equality
 * to fully implement single (and multiple)
 * value editing.
 */
template <
	Bool valueFunc(Storage& r, const Reflection::Property& prop, Reflection::Any& rValue),
	Bool equalFunc(const ViewPropertyEditor::Stack::ConstIterator& iBegin, const ViewPropertyEditor::Stack::ConstIterator& iEnd, const Reflection::Property& prop)>
static Bool DefaultValueFunc(
	IControllerPropertyEditor& rController,
	PropertyUtil::Path& rvPath,
	ViewPropertyEditorUtil::Storage& r,
	const ViewPropertyEditor::Stack::ConstIterator& iBegin,
	const ViewPropertyEditor::Stack::ConstIterator& iEnd,
	const Reflection::Property& prop)
{
	using namespace Reflection;

	// Check if all values across all mutation
	// targets are equal - if so, get the value.
	Any value;
	if (equalFunc(iBegin, iEnd, prop))
	{
		SEOUL_VERIFY(prop.TryGet(*iBegin, value));
	}

	// Mutate the value - on mutation, update.
	if (valueFunc(r, prop, value))
	{
		// TODO: CommitPropertyEdit should handle
		// capture of the old value instead.

		IControllerPropertyEditor::PropertyValues vOldValues;
		vOldValues.Reserve((UInt32)(iEnd - iBegin));
		Reflection::Any oldValue;
		for (auto i = iBegin; iEnd != i; ++i)
		{
			SEOUL_VERIFY(prop.TryGet(*i, oldValue));
			vOldValues.PushBack(oldValue);
		}

		// Commit the new value.
		rController.CommitPropertyEdit(rvPath, vOldValues, IControllerPropertyEditor::PropertyValues(1u, value));
		return true;
	}
	else
	{
		return false;
	}
}

/** Specialization of value edit for N-tuple Vector types (e.g. Vector3D, Vector4D, etc.). */
template <typename T, UInt32 N>
static Bool VectorNValueFunc(
	IControllerPropertyEditor& rController,
	PropertyUtil::Path& rvPath,
	ViewPropertyEditorUtil::Storage& r,
	const ViewPropertyEditor::Stack::ConstIterator& iBegin,
	const ViewPropertyEditor::Stack::ConstIterator& iEnd,
	const Reflection::Property& prop)
{
	using namespace Reflection;

	// Used to determine if all indeterminant bits are set for this type.
	UInt32 uAllSet = 0u;
	for (UInt32 i = 0u; i < N; ++i)
	{
		uAllSet |= (1 << i);
	}

	// Track indeterminism for the type (if a component is
	// different between two editing targets).
	UInt32 uIndeterminant = 0u;
	T vValue{};

	auto i = iBegin;

	Any anyValue;
	if (!prop.TryGet(*i, anyValue))
	{
		// Immediatelly and fully indeterminant.
		uIndeterminant = UIntMax;
	}
	else
	{
		// Initial value, process further values.
		vValue = anyValue.Cast<T>();
		++i;

		for (; iEnd != i; ++i)
		{
			if (!prop.TryGet(*i, anyValue))
			{
				// Immediatelly and fully indeterminant.
				uIndeterminant = UIntMax;
				break;
			}

			// Check components, track indeterminism.
			T const vCurrent = anyValue.Cast<T>();
			for (UInt32 i = 0u; i < N; ++i)
			{
				if (vCurrent[i] != vValue[i])
				{
					uIndeterminant |= (1 << i);
				}
			}

			// Early out if we've already found that
			// all components have different values.
			if (uAllSet == (uAllSet & uIndeterminant))
			{
				break;
			}
		}
	}

	// Process the value - then check for change handling,
	// if changes were made.
	T vNewValue;
	if (!ValueFuncImplVectorN<T, N>(r, prop, uIndeterminant, vValue, vNewValue))
	{
		return false;
	}

	for (UInt32 i = 0u; i < N; ++i)
	{
		// Value didn't change, skip it.
		if (vNewValue[i] == vValue[i])
		{
			continue;
		}

		// TODO: CommitPropertyEdit should handle
		// capture of the old value instead.

		IControllerPropertyEditor::PropertyValues vOldValues;
		vOldValues.Reserve((UInt32)(iEnd - iBegin));
		Reflection::Any oldValue;
		for (auto j = iBegin; iEnd != j; ++j)
		{
			SEOUL_VERIFY(prop.TryGet(*j, oldValue));
			auto const v(oldValue.Cast<T>());
			vOldValues.PushBack(v[i]);
		}

		// Update the single component that changed.

		// TODO: Potentially brittle, but also
		// convenient and may be reasonable for us to require
		// it to be true.
		SEOUL_ASSERT(nullptr != TypeOf<T>().GetProperty(i));
		auto const name = TypeOf<T>().GetProperty(i)->GetName();
		rvPath.PushBack(name);
		rController.CommitPropertyEdit(rvPath, vOldValues, IControllerPropertyEditor::PropertyValues(1u, vNewValue[i]));
		rvPath.PopBack();
	}

	return true;
}

/**
 * Wraps all value edits in standard boilerplate.
 */
template <Bool valueFunc(
	IControllerPropertyEditor& rController,
	PropertyUtil::Path& rvPath,
	ViewPropertyEditorUtil::Storage& r,
	const ViewPropertyEditor::Stack::ConstIterator& iBegin,
	const ViewPropertyEditor::Stack::ConstIterator& iEnd,
	const Reflection::Property& prop)>
static Bool FuncBinder(
	IControllerPropertyEditor& rController,
	PropertyUtil::Path& rvPath,
	ViewPropertyEditorUtil::Storage& r,
	const ViewPropertyEditor::Stack::ConstIterator& iBegin,
	const ViewPropertyEditor::Stack::ConstIterator& iEnd,
	const Reflection::Property& prop)
{
	using namespace ImGui;
	using namespace Reflection;

	// Get the display name for the property.
	auto name = prop.GetName();
	if (auto pDisplayName = prop.GetAttributes().GetAttribute<Attributes::DisplayName>())
	{
		name = pDisplayName->m_DisplayName;
	}

	auto const iStart = GetCursorPosX();

	BeginGroup();
	PushID(name.CStr());
	AlignTextToFramePadding();
	TextUnformatted(name.CStr(), nullptr, true);
	SameLine();
	SetCursorPosX(iStart + 100); // TODO:
	PushItemWidth(GetContentRegionAvail().x);

	Bool const bReturn = valueFunc(rController, rvPath, r, iBegin, iEnd, prop);

	PopItemWidth();
	PopID();
	EndGroup();

	SetTooltipEx(prop.GetAttributes());
	return bReturn;
}
// /Handlers for display/edit of value types.

Bool ViewPropertyEditor::GetValueFunc(
	const Reflection::Property& prop,
	ValueFunc& rFunc) const
{
	// Property has EnumLike, so we need to treat it as such.
	if (prop.GetAttributes().HasAttribute<Reflection::Attributes::EnumLike>())
	{
		rFunc = FuncBinder< DefaultValueFunc<EnumLikeValueFuncImpl, EnumLikeEqualFunc> >;
		return true;
	}

	// Otherwise, type of the value is an Enum.
	auto const& typeInfo = prop.GetMemberTypeInfo();
	if (typeInfo.GetSimpleTypeInfo() == Reflection::SimpleTypeInfo::kEnum)
	{
		rFunc = FuncBinder< DefaultValueFunc<EnumValueFuncImpl, EnumEqualFunc> >;
		return true;
	}
	// Finally, default handling.
	else
	{
		return m_tValueTypes.GetValue(&typeInfo, rFunc);
	}
}

#define SEOUL_VALUE_TYPE(type) \
	SEOUL_VERIFY(m_tValueTypes.Insert(&TypeId<type>(), &FuncBinder< DefaultValueFunc< ValueFuncImpl<type>, ValueEqualFunc<type> > >).Second) \

void ViewPropertyEditor::PopulateValueTypes()
{
	SEOUL_VALUE_TYPE(Bool);
	SEOUL_VALUE_TYPE(Byte const*);
	SEOUL_VALUE_TYPE(FilePath);
	SEOUL_VALUE_TYPE(Float32);
	SEOUL_VALUE_TYPE(HString);
	SEOUL_VALUE_TYPE(Int8);
	SEOUL_VALUE_TYPE(Int16);
	SEOUL_VALUE_TYPE(Int32);
	SEOUL_VALUE_TYPE(String);
	SEOUL_VALUE_TYPE(UInt8);
	SEOUL_VALUE_TYPE(UInt16);
	SEOUL_VALUE_TYPE(UInt32);
	SEOUL_VERIFY(m_tValueTypes.Insert(&TypeId<Vector3D>(), &FuncBinder< VectorNValueFunc<Vector3D, 3> >).Second);
}
#undef SEOUL_VALUE_TYPE

} // namespace Seoul::EditorUI
