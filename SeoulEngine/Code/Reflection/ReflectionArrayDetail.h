/**
 * \file ReflectionArrayDetail.h
 * \brief Types used to construct subclasses of Reflection::Array that
 * defines array behavior for various types exposed through the
 * reflection system.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_ARRAY_DETAIL_H
#define REFLECTION_ARRAY_DETAIL_H

#include "ReflectionArray.h"
#include "ReflectionType.h"
#include "ReflectionTypeDetail.h"

namespace Seoul::Reflection
{

namespace ArrayDetail
{

template <typename T>
struct ArrayIsResizable SEOUL_SEALED
{
private:
	typedef typename T::SizeType SizeType;
	typedef typename T::ConstReference ConstReference;

	typedef Byte True[1];
	typedef Byte False[2];

	template <typename U, U MEMBER_FUNC> struct Tester {};

	template <typename U>
	static True& TestIt(Tester< void (U::*)(SizeType, ConstReference), &U::Resize >*);

	template <typename U>
	static False& TestIt(...);

public:
	static const Bool Value = (sizeof(TestIt<T>(nullptr)) == sizeof(True));
}; // struct ArrayIsResizable

template <typename T, Bool B_ARRAY_IS_RESIZABLE>
struct ArrayResizer
{
	static Bool TryResize(T& rArray, UInt32 zNewSize)
	{
		rArray.Resize(zNewSize);
		return true;
	}
};

template <typename T>
struct ArrayResizer<T, false>
{
	static Bool TryResize(T&, UInt32)
	{
		return false;
	}
};

template <typename T>
class ArrayT SEOUL_SEALED : public Seoul::Reflection::Array
{
public:
	/** Carry through the ValueType of the array we represent. */
	typedef typename T::ValueType ValueType;

	/** Bypass some polymorphism when handling individual values. */
	typedef typename TypeTDiscovery<ValueType>::Type Handler;

	ArrayT()
		: Array((ArrayIsResizable<T>::Value ? ArrayFlags::kResizable : ArrayFlags::kNone))
	{
	}

	~ArrayT()
	{
	}

	/** @return the TypeInfo of elements of this Array. */
	virtual const TypeInfo& GetElementTypeInfo() const SEOUL_OVERRIDE
	{
		return TypeId<typename T::ValueType>();
	}

	/** Attempt to get a copy of the element at uIndex. */
	virtual Bool TryGet(const WeakAny& arrayPointer, UInt32 uIndex, Any& rValue) const SEOUL_OVERRIDE
	{
		T const* pArray = nullptr;
		if (PointerCast(arrayPointer, pArray))
		{
			SEOUL_ASSERT(nullptr != pArray);
			if (uIndex < pArray->GetSize())
			{
				typename T::ValueType const* pElement = pArray->Get(uIndex);
				SEOUL_ASSERT(nullptr != pElement);
				rValue = *pElement;
				return true;
			}
		}

		return false;
	}

	/** Attempt to assign a read-write pointer to the element at uIndex to rValue. */
	virtual Bool TryGetElementPtr(const WeakAny& arrayPointer, UInt32 uIndex, WeakAny& rValue) const SEOUL_OVERRIDE
	{
		T* pArray = nullptr;
		if (PointerCast(arrayPointer, pArray))
		{
			SEOUL_ASSERT(nullptr != pArray);
			if (uIndex < pArray->GetSize())
			{
				typename T::ValueType* pElement = pArray->Get(uIndex);
				SEOUL_ASSERT(nullptr != pElement);
				rValue = pElement;
				return true;
			}
		}

		return false;
	}

	/** Attempt to assign a read-only pointer to the element at uIndex to rValue. */
	virtual Bool TryGetElementConstPtr(const WeakAny& arrayPointer, UInt32 uIndex, WeakAny& rValue) const SEOUL_OVERRIDE
	{
		T const* pArray = nullptr;
		if (PointerCast(arrayPointer, pArray))
		{
			SEOUL_ASSERT(nullptr != pArray);
			if (uIndex < pArray->GetSize())
			{
				typename T::ValueType const* pElement = pArray->Get(uIndex);
				SEOUL_ASSERT(nullptr != pElement);
				rValue = pElement;
				return true;
			}
		}

		return false;
	}

	/** Attempt to retrieve the size of arrayPointer. */
	virtual Bool TryGetSize(const WeakAny& arrayPointer, UInt32& rzSize) const SEOUL_OVERRIDE
	{
		T const* pArray = nullptr;
		if (PointerCast(arrayPointer, pArray))
		{
			SEOUL_ASSERT(nullptr != pArray);
			rzSize = pArray->GetSize();
			return true;
		}

		return false;
	}

	/** Attempt to resize arrayPointer to zSize, return success or failure. */
	virtual Bool TryResize(const WeakAny& arrayPointer, UInt32 zNewSize) const SEOUL_OVERRIDE
	{
		T* pArray = nullptr;
		if (PointerCast(arrayPointer, pArray))
		{
			SEOUL_ASSERT(nullptr != pArray);
			return ArrayResizer<T, ArrayIsResizable<T>::Value>::TryResize(*pArray, zNewSize);
		}

		return false;
	}

	/**
	 * Attempt to update the element at uIndex to value.
	 *
	 * @return True if the value was successfully updated, false otherwise.
	 */
	virtual Bool TrySet(const WeakAny& arrayPointer, UInt32 uIndex, const WeakAny& value) const SEOUL_OVERRIDE
	{
		T* pArray = nullptr;
		if (PointerCast(arrayPointer, pArray))
		{
			SEOUL_ASSERT(nullptr != pArray);
			if (uIndex < pArray->GetSize())
			{
				return TypeConstruct(value, *pArray->Get(uIndex));
			}
		}

		return false;
	}

	/**
	 * Attempt to serialize the array data in dataNode into
	 * objectThis, assuming objectThis is of type T.
	 *
	 * @return True if deserialization was successful, false otherwise. If this method
	 * returns false, an error was reported through rContext HandleError(), which returned
	 * false.
	 */
	virtual Bool TryDeserialize(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& array,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false) const SEOUL_OVERRIDE
	{
		// Get the array object - if this fails, we must fail, as there is nothing more to do.
		T* pArray = nullptr;
		if (!PointerCast(objectThis, pArray))
		{
			return false;
		}
		SEOUL_ASSERT(nullptr != pArray);

		// Get the size of the input array.
		UInt32 uArrayCount = 0u;
		if (!dataStore.GetArrayCount(array, uArrayCount))
		{
			if (!rContext.HandleError(SerializeError::kDataNodeIsNotArray))
			{
				return false;
			}
		}

		// Remove all existing entries from the array - attempt to resize to 0,
		// if this fails, assign a default value to the existing array.
		if (!ArrayResizer<T, ArrayIsResizable<T>::Value>::TryResize(*pArray, 0))
		{
			pArray->Fill(ValueType());
		}

		// Check and match the array object to the input size.
		if (pArray->GetSize() != uArrayCount &&
			!ArrayResizer<T, ArrayIsResizable<T>::Value>::TryResize(*pArray, uArrayCount))
		{
			if (!rContext.HandleError(SerializeError::kFailedSizingObjectArray))
			{
				return false;
			}
		}

		// Deserialize each element of the array.
		UInt32 uOut = 0u;
		for (UInt32 i = 0u; i < uArrayCount; ++i)
		{
			// Get the value from the array - this must alwasy succeed in this context.
			DataNode elementValue;
			SEOUL_VERIFY(dataStore.GetValueFromArray(array, i, elementValue));

			SerializeContextScope scope(rContext, elementValue, GetElementTypeInfo(), i);

			// If deserialization of the element fails, fail deserialization.
			if (!Handler::DirectDeserialize(
				rContext,
				dataStore,
				elementValue,
				*pArray->Get(uOut),
				bSkipPostSerialize))
			{
				if (!rContext.HandleError(SerializeError::kFailedSettingValueToArray))
				{
					return false;
				}
				// Skip the element, by not advancing iOut.
				else
				{
					continue;
				}
			}

			// Out and in kept in-sync.
			uOut++;
		}

		// Cleanup array if iOut does not match uArrayCount after deserialization.
		if (uOut != uArrayCount)
		{
			if (pArray->GetSize() != uOut &&
				!ArrayResizer<T, ArrayIsResizable<T>::Value>::TryResize(*pArray, uOut))
			{
				if (!rContext.HandleError(SerializeError::kFailedSizingObjectArray))
				{
					return false;
				}
			}
		}

		return true;
	}

	// Attempt to serialize the state of objectThis into the array dataNode in rDataStore, assuming objectThis is an array.
	virtual Bool TrySerialize(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& array,
		const WeakAny& objectThis,
		Bool bSkipPostSerialize = false) const SEOUL_OVERRIDE
	{
		// Get the array object - if this fails, we must fail, as there is nothing more to do.
		T const* pArray = nullptr;
		if (!PointerCast(objectThis, pArray))
		{
			return false;
		}
		SEOUL_ASSERT(nullptr != pArray);

		// Get the size of the input array.
		UInt32 uArrayCount = pArray->GetSize();

		// Match the array object to the input size.
		if (!rDataStore.ResizeArray(array, uArrayCount))
		{
			if (!rContext.HandleError(SerializeError::kFailedSizingObjectArray))
			{
				return false;
			}
		}

		// Serialize each element of the array.
		for (UInt32 i = 0u; i < uArrayCount; ++i)
		{
			SerializeContextScope scope(rContext, DataNode(), GetElementTypeInfo(), i);

			// If serialization of the element fails, fail serialization.
			if (!Handler::DirectSerializeToArray(
				rContext,
				rDataStore,
				array,
				i,
				*pArray->Get(i),
				bSkipPostSerialize))
			{
				return false;
			}
		}

		return true;
	}

	virtual void FromScript(lua_State* pVm, Int iOffset, const WeakAny& objectThis) const SEOUL_OVERRIDE
	{
		// Get the array object - if this fails, we must fail, as there is nothing more to do.
		T* pArray = nullptr;
		if (!PointerCast(objectThis, pArray))
		{
			// TODO: I think I can assert here, as the caller
			// will always ensure this succeeds.
			return;
		}
		SEOUL_ASSERT(nullptr != pArray);

		// TODO: Evaluate whether it would be faster to avoid this
		// call (which is O(log n)) and instead use the PushBack() semantics
		// of the array (if those are supported).

		// Get the entire array count.
		UInt32 uArrayCount = (UInt32)lua_rawlen(pVm, iOffset);

		// Remove all existing entries from the array - attempt to resize to 0,
		// if this fails, assign a default value to the existing array.
		if (!ArrayResizer<T, ArrayIsResizable<T>::Value>::TryResize(*pArray, 0))
		{
			pArray->Fill(ValueType());
		}

		// Attempt to match the array size to the target.
		ArrayResizer<T, ArrayIsResizable<T>::Value>::TryResize(*pArray, uArrayCount);

		// One way or the other, match sizes.
		uArrayCount = pArray->GetSize();

		// Cache the reflection type of the value we'll use for processing.
		auto const& type = TypeOf<ValueType>();
		for (UInt32 i = 0u; i < uArrayCount; ++i)
		{
			// Push the value onto the script stack.
			lua_rawgeti(pVm, iOffset, (int)(i + 1));
			
			// Process the value into the array.
			type.FromScript(pVm, -1, pArray->Get(i));
			
			// Pop the value.
			lua_pop(pVm, 1);
		}
	}

	virtual void ToScript(lua_State* pVm, const WeakAny& objectThis) const SEOUL_OVERRIDE
	{
		// Get the array object - if this fails, we must fail, as there is nothing more to do.
		T const* pArray = nullptr;
		if (!PointerCast(objectThis, pArray))
		{
			// TODO: I think I can assert here, as the caller
			// will always ensure this succeeds.
			lua_pushnil(pVm);
			return;
		}
		SEOUL_ASSERT(nullptr != pArray);

		// Cache the array size.
		auto const uSize = pArray->GetSize();

		// Cache the reflection type of the value we will use
		// for processing.
		auto const& type = TypeOf<ValueType>();
		
		// Create a table to populate on the stack. Pre-allocated
		// with a sufficiently sized array portion.
		lua_createtable(pVm, uSize, 0);
		for (UInt32 i = 0u; i < uSize; ++i)
		{
			// Process the array element.
			type.ToScript(pVm, pArray->Get(i));

			// Commit the value to the array on the script stack. This
			// also pops the value off the stack.
			lua_rawseti(pVm, -2, (int)(i + 1));
		}
	}

private:
	SEOUL_DISABLE_COPY(ArrayT);
}; // class ArrayT

} // namespace ArrayDetail

} // namespace Seoul::Reflection

#endif // include guard
