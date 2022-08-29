/**
 * \file ReflectionAttributes.h
 * \brief Standard set of reflection attributes - provided for convenience and
 * to standardize some common attributes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_ATTRIBUTES_H
#define REFLECTION_ATTRIBUTES_H

#include "Delegate.h"
#include "ReflectionAny.h"
#include "ReflectionAttribute.h"
namespace Seoul { namespace Reflection { class Enum; } }

namespace Seoul::Reflection
{

namespace Attributes
{

/**
 * Used to bind a native class to a script class
 */
class ScriptClass SEOUL_SEALED : public Attribute
{
public:
	template <size_t zStringArrayLengthInBytes>
	ScriptClass(Byte const (&sClassName)[zStringArrayLengthInBytes], Bool bEmit = false)
		: m_ClassName(CStringLiteral(sClassName))
		, m_bEmit(bEmit)
	{
	}

	explicit ScriptClass(Bool bEmit = false)
		: m_ClassName()
		, m_bEmit(bEmit)
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("ScriptClass");
		return kId;
	}

	const HString m_ClassName;
	const Bool m_bEmit;
};

/**
 * Used to ignore the error when doing a generic deserialize for data which has no properties.
 * The error is there to catch cases when a custom deserialize should have been set, but wasn't,
 * however there are times when we actually want to deserialize objects with no properties
 * (eg. the components list on IggyUIMovie).
 */
class AllowNoProperties SEOUL_SEALED : public Attribute
{
public:
	AllowNoProperties()
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("AllowNoProperties");
		return kId;
	}
};

/**
 * Used to specify a human friendly category for an attribute or method - likely
 * useful for an editor or in-game visualization tool.
 */
class Category SEOUL_SEALED : public Attribute
{
public:
	template <size_t zStringArrayLengthInBytes>
	Category(Byte const (&sCategoryName)[zStringArrayLengthInBytes])
		: m_CategoryName(CStringLiteral(sCategoryName))
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("Category");
		return kId;
	}

	HString m_CategoryName;
};

/**
 * In a serialization context, defines a method that should be called to handle
 * serialization of a property instead of the standard serialization by member properties.
 *
 * The deserialization method is expected to have the following signature:
 * - Bool MyMethod(Reflection::SerializeContext* pContext, DataStore const* pDataStore, const DataNode& value)
 *
 * The serialization method is expected to have the following signature:
 * - Bool MyMethod(Reflection::SerializeContext* pContext, HString propertyName, DataStore* pDataStore, const DataNode& table)
 */
class CustomSerializeProperty SEOUL_SEALED : public Attribute
{
public:
	template <size_t zDeserializeStringArrayLengthInBytes>
	CustomSerializeProperty(
		Byte const (&sDeserializeMethodName)[zDeserializeStringArrayLengthInBytes])
		: m_DeserializeMethodName(CStringLiteral(sDeserializeMethodName))
		, m_SerializeMethodName()
	{
	}

	template <size_t zDeserializeStringArrayLengthInBytes, size_t zSerializeStringArrayLengthInBytes>
	CustomSerializeProperty(
		Byte const (&sDeserializeMethodName)[zDeserializeStringArrayLengthInBytes],
		Byte const (&sSerializeMethodName)[zSerializeStringArrayLengthInBytes])
		: m_DeserializeMethodName(CStringLiteral(sDeserializeMethodName))
		, m_SerializeMethodName(CStringLiteral(sSerializeMethodName))
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("CustomSerializeProperty");
		return kId;
	}

	HString m_DeserializeMethodName;
	HString m_SerializeMethodName;
}; // class CustomSerializeProperty

/**
 * Disable the "unknown property" check during deserialization for
 * this type. Useful if you know that a target type is intentionally
 * only a subset of the expected properties in a deserialized
 * input.
 */
class DisableReflectionCheck SEOUL_SEALED : public Attribute
{
public:
	DisableReflectionCheck()
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("DisableReflectionCheck");
		return kId;
	}
}; // class DisableReflectionCheck

/**
 * In a serialization context, defines a field that should be set to true if the property is deserialized
 *
 * The property is expected to be a Bool
 */
class IfDeserializedSetTrue SEOUL_SEALED : public Attribute
{
public:
	template <size_t zFieldStringArrayLengthInBytes>
	IfDeserializedSetTrue(
		Byte const (&sFieldToSetName)[zFieldStringArrayLengthInBytes])
		: m_FieldToSetName(CStringLiteral(sFieldToSetName))
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("IfDeserializedSetTrue");
		return kId;
	}

	HString m_FieldToSetName;
}; // class IfDeserializedSetTrue

class CustomSerializeType SEOUL_SEALED : public Attribute
{
public:
	typedef Bool (*CustomDeserialize)(
		SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& table,
		const Reflection::WeakAny& objectThis,
		Bool bSkipPostSerialize);

	typedef Bool (*CustomSerializeToArray)(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& array,
		UInt32 uIndex,
		const Reflection::WeakAny& objectThis,
		Bool bSkipPostSerialize);

	typedef Bool (*CustomSerializeToTable)(
		SerializeContext& rContext,
		DataStore& rDataStore,
		const DataNode& table,
		HString key,
		const Reflection::WeakAny& objectThis,
		Bool bSkipPostSerialize);

	CustomSerializeType(
		CustomDeserialize customDeserialize,
		CustomSerializeToArray customSerializeToArray = nullptr,
		CustomSerializeToTable customSerializeToTable = nullptr)
		: m_CustomDeserialize(customDeserialize)
		, m_CustomSerializeToArray(customSerializeToArray)
		, m_CustomSerializeToTable(customSerializeToTable)
	{
		// Sanity checks - either both are nullptr, or both are non-null
		SEOUL_ASSERT(
			(nullptr == m_CustomSerializeToArray && nullptr == m_CustomSerializeToTable) ||
			(nullptr != m_CustomSerializeToArray && nullptr != m_CustomSerializeToTable));
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("CustomSerializeType");
		return kId;
	}

	CustomDeserialize m_CustomDeserialize;
	CustomSerializeToArray m_CustomSerializeToArray;
	CustomSerializeToTable m_CustomSerializeToTable;
}; // class CustomSerializeType

class Deprecated SEOUL_SEALED : public Attribute
{
public:
	Deprecated()
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("Deprecated");
		return kId;
	}
}; // class Deprecated

/**
 * Used to define a human readable description for a property, method, or type.
 */
class Description SEOUL_SEALED : public Attribute
{
public:
	template <size_t zStringArrayLengthInBytes>
	Description(Byte const (&sDescriptionText)[zStringArrayLengthInBytes])
		: m_DescriptionText(CStringLiteral(sDescriptionText))
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("Description");
		return kId;
	}

	HString m_DescriptionText;
};

/**
 * Used to specify a human friendly display name for an attribute or method - likely
 * useful for an editor or in-game visualization tool.
 */
class DisplayName SEOUL_SEALED : public Attribute
{
public:
	template <size_t zStringArrayLengthInBytes>
	DisplayName(Byte const (&sDisplayName)[zStringArrayLengthInBytes])
		: m_DisplayName(CStringLiteral(sDisplayName))
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("DisplayName");
		return kId;
	}

	HString m_DisplayName;
};

/** Equivalent to do not serialize, but hides the property or field from an Editor environment. */
class DoNotEdit SEOUL_SEALED : public Attribute
{
public:
	DoNotEdit()
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("DoNotEdit");
		return kId;
	}
}; // class DoNotEdit

class DoNotSerialize SEOUL_SEALED : public Attribute
{
public:
	DoNotSerialize()
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("DoNotSerialize");
		return kId;
	}
}; // class DoNotSerialize

class DoNotSerializeIfEqualToSimpleType SEOUL_SEALED : public Attribute
{
public:
	DoNotSerializeIfEqualToSimpleType(const Reflection::Any& value)
		: m_Value(value)
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("DoNotSerializeIfEqualToSimpleType");
		return kId;
	}

	Reflection::Any m_Value;

	Bool Equals(const Property& property, const WeakAny& objectThis, SerializeError& error) const;
};

class DoNotSerializeIf SEOUL_SEALED : public Attribute
{
public:
	template <size_t zDoNotSerializeIfStringArrayLengthInBytes>
	DoNotSerializeIf(
		Byte const (&sDoNotSerializeIfMethodName)[zDoNotSerializeIfStringArrayLengthInBytes])
		: m_DoNotSerializeIfMethodName(CStringLiteral(sDoNotSerializeIfMethodName))
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("DoNotSerializeIf");
		return kId;
	}

	HString m_DoNotSerializeIfMethodName;
};

class EditorButton SEOUL_SEALED : public Attribute
{
public:
	template <size_t zStringArrayLengthInBytes>
	EditorButton(Byte const (&sPropertyName)[zStringArrayLengthInBytes])
		: m_PropertyName(CStringLiteral(sPropertyName))
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("EditorButton");
		return kId;
	}

	HString const m_PropertyName;
}; // class EditorButton

class EditorHide SEOUL_SEALED : public Attribute
{
public:
	EditorHide()
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("EditorHide");
		return kId;
	}
}; // class EditorHide

/**
 * In applicable contexts, indicates that an instance
 * of this type should start expanded in a tree or
 * property view.
 */
class EditorDefaultExpanded SEOUL_SEALED : public Attribute
{
public:
	EditorDefaultExpanded()
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("EditorDefaultExpanded");
		return kId;
	}
}; // class EditorDefaultExpanded

/**
 * Used by FilePath style properties to determine
 * the extension and starting directory if
 * a FilePath field does not already have an
 * existing value.
 */
class EditorFileSpec SEOUL_SEALED : public Attribute
{
public:
	EditorFileSpec(GameDirectory eDirectory, FileType eFileType)
		: m_eDirectory(eDirectory)
		, m_eFileType(eFileType)
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("EditorFileSpec");
		return kId;
	}

	GameDirectory const m_eDirectory;
	FileType const m_eFileType;
}; // class EditorFileSpec


/**
 * Marks a function as a persistence data migrator from one version to another
 * Use in combination with a PersistenceDataMigrationTest attribute.
 */
class PersistenceDataMigration SEOUL_SEALED : public Attribute
{
public:
	PersistenceDataMigration(Int32 iFromVersion, Int32 iToVersion)
		: m_iFromVersion(iFromVersion)
		, m_iToVersion(iToVersion)
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("PersistenceDataMigration");
		return kId;
	}

	Int32 m_iFromVersion;
	Int32 m_iToVersion;
}; // class PersistenceDataMigration

 /**
 * Marks a function as a unit test for a persistence data migrator.
 * This attribute is required for any function with a PersistenceDataMigration attribute.
 */
class PersistenceDataMigrationTest SEOUL_SEALED : public Attribute
{
public:
	PersistenceDataMigrationTest(const Byte* sBefore, const Byte* sAfter)
		: m_sBefore(sBefore)
		, m_sAfter(sAfter)
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("PersistenceDataMigrationTest");
		return kId;
	}

	const Byte* m_sBefore;
	const Byte* m_sAfter;
}; // class PersistenceDataMigrationTest

/**
 * In a deserialization context, this tells the serializer/deserializer that
 * the concrete base class of this Type to instantiate will be specified
 * with a key-value pair in the input DataStore table. The key to lookup
 * is the member variable m_Key.
 */
class PolymorphicKey SEOUL_SEALED : public Attribute
{
public:
	template <size_t zStringArrayLengthInBytes>
	PolymorphicKey(Byte const (&sKey)[zStringArrayLengthInBytes], MemoryBudgets::Type eMemoryBudgetsType = MemoryBudgets::Reflection)
		: m_Key(CStringLiteral(sKey))
		, m_Default()
		, m_eMemoryBudgetsType(eMemoryBudgetsType)
	{
	}

	template <size_t zStringArrayLengthInBytes0, size_t zStringArrayLengthInBytes1>
	PolymorphicKey(Byte const (&sKey)[zStringArrayLengthInBytes0], Byte const (&sDefault)[zStringArrayLengthInBytes1], MemoryBudgets::Type eMemoryBudgetsType = MemoryBudgets::Reflection)
		: m_Key(CStringLiteral(sKey))
		, m_Default(CStringLiteral(sDefault))
		, m_eMemoryBudgetsType(eMemoryBudgetsType)
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("PolymorphicKey");
		return kId;
	}

	HString const m_Key;
	HString const m_Default;
	MemoryBudgets::Type const m_eMemoryBudgetsType;
}; // class PolymorphicKey;

/**
 * In a serialization context, defines a method that should be called after a type has
 * been deserialized.
 *
 * The method is expected to have the following signature:
 * - Bool MyMethod(SerializeContext* pContext)
 */
class PostSerializeType SEOUL_SEALED : public Attribute
{
public:
	template <size_t zDeserializeStringArrayLengthInBytes>
	PostSerializeType(
		Byte const (&sDeserializeMethodName)[zDeserializeStringArrayLengthInBytes])
		: m_DeserializeMethodName(CStringLiteral(sDeserializeMethodName))
		, m_SerializeMethodName()
	{
	}

	template <size_t zDeserializeStringArrayLengthInBytes, size_t zSerializeStringArrayLengthInBytes>
	PostSerializeType(
		Byte const (&sDeserializeMethodName)[zDeserializeStringArrayLengthInBytes],
		Byte const (&sSerializeMethodName)[zSerializeStringArrayLengthInBytes])
		: m_DeserializeMethodName(CStringLiteral(sDeserializeMethodName))
		, m_SerializeMethodName(CStringLiteral(sSerializeMethodName))
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("PostSerializeType");
		return kId;
	}

	HString m_DeserializeMethodName;
	HString m_SerializeMethodName;
}; // class PostSerializeType

/**
 * Used to indicate that a property is not required in a serialization context. Unless
 * this attribute is specified, a property that does not have an input value in a DataStore
 * during deserialization will be considered a deserialization error.
 */
class NotRequired SEOUL_SEALED : public Attribute
{
public:
	NotRequired()
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("NotRequired");
		return kId;
	}
};

class PointerLike SEOUL_SEALED : public Attribute
{
public:
	typedef WeakAny (*GetPtrDelegate)(const WeakAny& objectThis);

	PointerLike(GetPtrDelegate getPtrDelegate)
		: m_GetPtrDelegate(getPtrDelegate)
	{
		// Sanity checks - must not be nullptr.
		SEOUL_ASSERT(nullptr != m_GetPtrDelegate);
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("PointerLike");
		return kId;
	}

	GetPtrDelegate m_GetPtrDelegate;
}; // class PointerLike

/**
 * Used to define a longer, addendum section for editor
 * and command-line args.
 */
class Remarks SEOUL_SEALED : public Attribute
{
public:
	Remarks(Byte const* s)
		: m_sRemarks(s)
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("Remarks");
		return kId;
	}

	const HString m_sRemarks;
};

/**
 * When attached to a type, script bindings will wrap the type emit
 * in this preprocessor definition in script.
 */
class ScriptPreprocessorDirective SEOUL_SEALED : public Attribute
{
public:
	template <size_t z>
	ScriptPreprocessorDirective(Byte const (&sName)[z])
		: m_Name(CStringLiteral(sName))
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("ScriptPreprocessorDirective");
		return kId;
	}

	HString const m_Name;
}; // class ScriptPreprocessorDirective

/**
 * Used to override the signature written to generated script interface
 * files.
 */
class ScriptSignature SEOUL_SEALED : public Attribute
{
public:
	template <size_t z0>
	ScriptSignature(Byte const (&sReturn)[z0])
		: m_Return(CStringLiteral(sReturn))
		, m_Args()
	{
	}

	template <size_t z0, size_t z1>
	ScriptSignature(Byte const (&sReturn)[z0], Byte const (&sArgs)[z1])
		: m_Return(CStringLiteral(sReturn))
		, m_Args(CStringLiteral(sArgs))
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("ScriptSignature");
		return kId;
	}

	HString const m_Return;
	HString const m_Args;
}; // class ScriptSignature

/**
 * The same usage as a unit test, but for performance measurements.
 * Test fixtures will run BenchmarkTest methods several times and use the best
 * measurement as the final result.
 */
class BenchmarkTest SEOUL_SEALED : public Attribute
{
public:
	BenchmarkTest()
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("BenchmarkTest");
		return kId;
	}
}; // class BenchmarkTest

/**
 * Associate with a class to identify it as a unit test fixture. When unit tests are run,
 * an instance of the class will be instantiated with Reflection::Type::New() and all methods
 * of that instance will be invoked.
 *
 * Associate with a method to identify it as an individual unit test. If the method is static,
 * it will just be run. If it is a member function, an instance of the method's class will
 * be instantiated and the method will be run.
 */
class UnitTest SEOUL_SEALED : public Attribute
{
public:
	enum Flag
	{
		kNone = 0,

		/**
		 * If defined for a type UnitTest attribute, the type will be destroyed and recreated
		 * after each test, instead of once before all tests in that type and once after.
		 */
		kInstantiateForEach = (1 << 0),
	};

	UnitTest(UInt32 uFlags = 0u)
		: m_uFlags(uFlags)
	{
	}

	UInt32 GetFlags() const
	{
		return m_uFlags;
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("UnitTest");
		return kId;
	}

	Bool InstantiateForEach() const { return (0u != (kInstantiateForEach & m_uFlags)); }

private:
	UInt32 m_uFlags;
};

/**
 * Associate with a class to identify it as an integration test fixture. When integration tests are run,
 * an instance of the class will be instantiated with Reflection::Type::New() and all methods
 * of that instance will be invoked.
 *
 * Unlike unit tests, it cannot be used on individual methods.
 */
class IntegrationTest SEOUL_SEALED : public Attribute
{
public:
	IntegrationTest()
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("IntegrationTest");
		return kId;
	}

private:
};

/**
 * When associated with a type, an instance of that type will be instantiated
 * by DevUIViewCommands and its methods will be used as DevUIViewCommands commands.
 */
class CommandsInstance SEOUL_SEALED : public Attribute
{
public:
	CommandsInstance()
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("CommandsInstance");
		return kId;
	}
}; // class CommandsInstance

/**
 * For developer ui/console commands that can be disabled temporarily or
 * permanently at runtime. Function is expected to return a non-empty
 * HString when the command is disabled with a human readable explanation
 * for why it is disabled (which in DevUI will replace the standard tooltip).
 */
class CommandIsDisabled SEOUL_SEALED : public Attribute
{
public:
	typedef HString(*IsDisabledFunc)();
	CommandIsDisabled(IsDisabledFunc isDisabledFunc)
		: m_IsDisabledFunc(isDisabledFunc)
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("CommandIsDisabled");
		return kId;
	}

	IsDisabledFunc const m_IsDisabledFunc;
}; // class CommandIsDisabled

/**
 * Require a button press to activate the command in the DevUI command view.
 *
 * By default, if a command takes no arguments, it will be displayed as a button press.
 * Otherwise, the command is called on any change to the arguments, and no button is
 * displayed.
 *
 * This attribute can be used to override that default and require a button
 * press to activate it, even if it takes arguments.
 */
class CommandNeedsButton SEOUL_SEALED : public Attribute
{
public:
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("CommandNeedsButton");
		return kId;
	}
}; // class CommandNeedsButton

/**
  * Use extra wide text boxes in the DevUI command view.
  *
  * Some commands have arguments that are too long to fit into the default text box size.
  * Setting this attribute will cause the command to draw with wider text fields.
  *
  */
class ExtraWideInput SEOUL_SEALED : public Attribute
{
public:
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("ExtraWideInput");
		return kId;
	}
}; // class ExtraWideInput

/**
 * In some contexts, this attribute is used to advertise that a
 * type behaves like an enum and has a limited (but possibly dynamic)
 * set of values.
 */
class EnumLike SEOUL_ABSTRACT : public Attribute
{
public:
	typedef Vector<HString, MemoryBudgets::Reflection> Names;

	virtual void GetNames(Names& rvNames) const = 0;
	virtual void NameToValue(HString name, Reflection::Any& rValue) const = 0;
	virtual void ValueToName(const Reflection::Any& value, HString& rName) const = 0;

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("EnumLike");
		return kId;
	}
}; // class EnumLike

/**
 * Sets the default value of the associated method argument
 * or property. Currently, used in the developer DevUI for method
 * arguments.
 */
class DefaultValue SEOUL_SEALED : public Attribute
{
public:
	template <size_t size>
	DefaultValue(Byte const (&value)[size])
		: m_DefaultValue(HString(CStringLiteral(value)))
	{

	}

	DefaultValue(const Reflection::Any& defaultValue)
		: m_DefaultValue(defaultValue)
	{

	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("DefaultValue");
		return kId;
	}

	Reflection::Any m_DefaultValue;
}; // class DefaultValue

/**
 * Equivalent to DefaultValue, but used to get the
 * current value in dynamic contexts. Used by the DevUI
 * for method arguments.
 */
class GetCurrentValue SEOUL_SEALED : public Attribute
{
public:
	typedef Reflection::Any (*GetValueFunc)();

	GetCurrentValue(GetValueFunc func)
		: m_Func(func)
	{

	}

	Reflection::Any Get() const
	{
		return m_Func();
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("GetCurrentValue");
		return kId;
	}

private:
	GetValueFunc m_Func;
}; // class GetCurrentValue

class Range SEOUL_SEALED : public Attribute
{
public:
	Range(const Reflection::Any& min, const Reflection::Any& max)
		: m_Min(min)
		, m_Max(max)
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("Range");
		return kId;
	}

	Reflection::Any m_Min;
	Reflection::Any m_Max;
}; // class Range

/**
 * Properties tagged with this attribute are command-line arguments -
 * populated by args passed to the literal command-line or extracted
 * from environment variables>
 *
 * Properties with this attribute must be static members of their
 * enclosing class. Currently, command-line args support only field
 * properties (if you tag a property that is e.g. a getter/setter pair
 * with the CommandLineArg attribute, it will fail to parse from the
 * command-line).
 *
 * For a fairly comprehensive usage example of implementing command-line
 * arguments in a tool, see Main.cpp in the SeoulTools -> JsonFormatter project.
 *
 * Also note that named command-line arguments can be consumed both from the
 * the literal command-line (e.g. -input filename for an arg named "input") as
 * well as the environment with the "SEOUL_ENV_" prefix (e.g. the -input argument
 * will be consumed from the environment as SEOUL_ENV_INPUT=filename.
 */
class CommandLineArg SEOUL_SEALED : public Attribute
{
public:
	/** Variation to specify a named argument (e.g.) -input filename
	 *  @param[in] sName The named argument id (e.g.) "input" for -input
	 *  @param[in] bRequired When specified, CommandLineArgs::Parse() will return false
	 *             if this argument is not passed to the command-line.
	 *  @param[in] bPrefix When true, this arg is a "prefix" arg, e.g. -DFoo=Bar. Prefix
	 *             args can be specified multiple times, e.g. -DFoo=Bar, -DFoo2=Bar2. The
	 *             property to which a prefix arg is attached must be a table, and the table
	 *             will be filled with the key-value pairs of the one-to-many prefix args. */
	template <size_t zName>
	CommandLineArg(Byte const (&sName)[zName], Bool bRequired = false, Bool bPrefix = false)
		: m_Name(CStringLiteral(sName))
		, m_iPosition(-1)
		, m_bRequired(bRequired)
		, m_bPrefix(bPrefix)
		, m_bTerminator(false)
		, m_ValueLabel()
	{
	}

	/** Variation to specify a named argument (e.g.) -input filename
	 *  that also includes a label for the value of the arg (e.g.) sLabel = "filename"
	 *  of -input filename.
	 *
	 *  @param[in] sName The named argument id (e.g.) "input" for -input
	 *  @param[in] sLabel Human readable label of the arg's value (e.g.) "filename" for -input filename.
	 *  @param[in] bRequired When specified, CommandLineArgs::Parse() will return false
	 *             if this argument is not passed to the command-line.
	 *  @param[in] bPrefix When true, this arg is a "prefix" arg, e.g. -DFoo=Bar. Prefix
	 *             args can be specified multiple times, e.g. -DFoo=Bar, -DFoo2=Bar2. The
	 *             property to which a prefix arg is attached must be a table, and the table
	 *             will be filled with the key-value pairs of the one-to-many prefix args. */
	template <size_t zName, size_t zLabel>
	CommandLineArg(Byte const (&sName)[zName], Byte const (&sLabel)[zLabel], Bool bRequired = false, Bool bPrefix = false)
		: m_Name(CStringLiteral(sName))
		, m_iPosition(-1)
		, m_bRequired(bRequired)
		, m_bPrefix(bPrefix)
		, m_bTerminator(false)
		, m_ValueLabel(CStringLiteral(sLabel))
	{
	}

	/** Variation to specify a positional argument (e.g.) -input filename
	 *  that also includes a label for the value of the arg (e.g.) sLabel = "filename"
	 *  of -input filename.
	 *
	 *  @param[in] sName The named argument id (e.g.) "input" for -input
	 *  @param[in] sLabel Human readable label of the arg's value (e.g.) "filename" for -input filename.
	 *  @param[in] bRequired When specified, CommandLineArgs::Parse() will return false
	 *             if this argument is not passed to the command-line.
	 *  @param[in] bPrefix When true, this arg is a "prefix" arg, e.g. -DFoo=Bar. Prefix
	 *             args can be specified multiple times, e.g. -DFoo=Bar, -DFoo2=Bar2. The
	 *             property to which a prefix arg is attached must be a table, and the table
	 *             will be filled with the key-value pairs of the one-to-many prefix args.
	 *  @param[in] bTerminator When true, this positional arg is the last arg that will be consumed
	 *             by command-line processing. Remaining args are assumed to be "pass-through" (e.g.
	 *             for a child process to be spawned) and will be ignored. */
	template <size_t zStringArrayLengthInBytes>
	CommandLineArg(Int iPosition, Byte const (&sLabel)[zStringArrayLengthInBytes], Bool bRequired = false, Bool bPrefix = false, Bool bTerminator = false)
		: m_Name()
		, m_iPosition(iPosition)
		, m_bRequired(bRequired)
		, m_bPrefix(bPrefix)
		, m_bTerminator(bTerminator)
		, m_ValueLabel(CStringLiteral(sLabel))
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("CommandLineArg");
		return kId;
	}

	const HString m_Name;
	const Int m_iPosition;
	const Bool m_bRequired;
	const Bool m_bPrefix;
	const Bool m_bTerminator;
	const HString m_ValueLabel;
};

/** Convenience macro for the very common pattery of:
 *  SEOUL_PROPERTY(prop)
 *  	SEOUL_ATTRIBUTE(CommandLineArg, ...) */
#define SEOUL_CMDLINE_PROPERTY(prop, ...) \
	SEOUL_PROPERTY(prop) \
		SEOUL_ATTRIBUTE(CommandLineArg, ##__VA_ARGS__)

/** Useful for suppressing command-line arguments that are engine levels in certain
 * tools, etc. in which they do not apply. Use at application level with the type
 * to suppress. */
class DisableCommandLineArgs SEOUL_SEALED : public Attribute
{
public:
	/** Give the type which contains arguments to suppress, e.g. "EngineCommandLineArgs"
	 *
	 *  See CookerMain.cpp in the SeoulTools -> Cooker project for a usage example. */
	template <size_t z>
	DisableCommandLineArgs(Byte const (&s)[z])
		: m_TypeName(CStringLiteral(s))
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("DisableCommandLineArgs");
		return kId;
	}

	HString const m_TypeName;
};

} // namespace Attributes

} // namespace Seoul::Reflection

#endif // include guard
