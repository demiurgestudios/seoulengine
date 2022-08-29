-----------------------------------------------------------------------------------------------------------------------
Overview:
-----------------------------------------------------------------------------------------------------------------------
The Reflection project implements C++ reflection (http://en.wikipedia.org/wiki/Reflection_%28computer_programming%29)
for SeoulEngine. Reflection can be used for serialization, introspection, validation, replication, etc.

Where appropriate, Seoul's Reflection project follows the philosophy/API/approach of Microsoft's .NET reflection.
See the documentation for .NET reflection for a deeper understanding of Seoul's Reflection:
http://msdn.microsoft.com/en-us/library/ms173183(v=vs.80).aspx .

-----------------------------------------------------------------------------------------------------------------------
How To: Define Reflection for a Concrete Type
-----------------------------------------------------------------------------------------------------------------------
To make a class or struct reflectable, include "ReflectionDefine.h" in the class CPP file and define the following
block:
  SEOUL_BEGIN_TYPE(MyTypeName)
  SEOUL_END_TYPE()

where MyTypeName is the class you want to reflect. If the class is a subclass:
  SEOUL_BEGIN_TYPE(MyTypeName)
    SEOUL_PARENT(MyParentTypeName)
  SEOUL_END_TYPE()

where MyParentTypeName is the immediate parent of the class you want to reflect. Note that parents must be
reflectable as well.

If a class is meant to be polymorphic (it will be subclassed), be sure to have a virtual destructor. If
you don't want to use a default constructor, add this flag to the BEGIN macro:

  SEOUL_BEGIN_TYPE(MyTypeName, TypeFlags::kDisableNew)

- DO   : define the reflection block for a type in the type's CPP file.
- DON'T: include ReflectionDefine.h in header files, unless you're defining a templated type - avoid code bloat.
- DO   : define immediate parents of a type.
- DON'T: define distant parents of a type - while this will compile, it can result in strange behavior at runtime. Each
         type in a class hierarchy should only define its immediate parents - distant parents will be accessible
		 through the immediate parents.

-----------------------------------------------------------------------------------------------------------------------
How To: Add extra Secret Sauce to Polymorphic Types
-----------------------------------------------------------------------------------------------------------------------
Although you don't need to do anything more than the above to reflect a Type, even polymorphic Types, it is almost
always useful to add the SEOUL_POLYMORPHIC_GET_TYPE() macro to your polymorphic type declaration. To do so, include
"ReflectionDeclare.h" in the type's header and place SEOUL_POLYMORPHIC_GET_TYPE() in a public: block of the type:
  class MyTypeName
  {
  public:
    virtual ~MyTypeName()
	{
	}

	SEOUL_REFLECTION_POLYMORPHIC(MyTypeName)
  };

when included, this macro defines a virtual method virtual WeakAny GetReflectionThis() that,
when called, returns a WeakAny of a pointer of the most derived class of the object. This allows the specific
Reflection::Type of an object to be discovered at runtime through its base classes, as well as other operations.

- DO: use the SEOUL_REFLECTION_POLYMORPHIC() macro in polymorphic types, to allow Reflection::Type discovery.

-----------------------------------------------------------------------------------------------------------------------
How To: Define Reflection for a Templated Type
-----------------------------------------------------------------------------------------------------------------------
To make a templated class or struct reflectable, include "ReflectionDefine.h" in a header file and define the following
block:
  SEOUL_BEGIN_TEMPLATE_TYPE(
	MyTemplatedType,
	(T),
	(typename T),
	("MyTemplatedClass<%s>", SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(T)))
  SEOUL_END_TYPE()

where MyTemplatedType is the type name, T is the template argument, and typename T is the signature of the template.
Defining templated types is a bit more complicated, so here are the arguments one by one:
  - MyTemplatedType - type name, same as a concrete type.
  - (T) - the template argument of the type - in this case, we're reflecting MyTemplatedType<T>
  - (typename T) - the templated signature of the type - in this case, template <typename T> class MyTempaltedType; is
    the signature of the type we're reflecting.
  - ("MyTemplatedClass<%s>", SEOUL_GET_FULLY_QUALIFIED_TYPE_NAME(T))) - the final argument is an expression that when evaluated as the
    arguments to a printf() style function, should uniquely identify any specialization of the templated type. For
	example, if we specialized MyTempaltedClass on a Float, this expression would evaluate to
	"MyTemplatedClass<Float>". This is used to generate a unique global name for each specialization of a templated
	type.

Due to the nature of templated types, this definition block must be visible to any code that reflects on a variable
that specializes the type, so this block must be defined in a header file. To avoid code bloat, it makes sense to
define this block in a header file separate from the header file that defines the type being reflected, and then
separately include that file in CPP files that define reflection on variables of the type. In other words, define
templated type blocks in a header file that you then treat like "ReflectionDefine.h".

- DO: place templated type definitions in a unique header file that is only included when needed.
- DO: provide an expression that will generate a unique string name for each specialization of a templated type.

-----------------------------------------------------------------------------------------------------------------------
How To: Define Properties
-----------------------------------------------------------------------------------------------------------------------
The real power of reflection emerges when you flesh out type definitions. Properties are one way to do this:
  SEOUL_BEGIN_TYPE(MyTypeName)
    SEOUL_PARENT(MyParentTypeName)

	SEOUL_PROPERTY_N("Name", m_sName)
  SEOUL_END_TYPE()

where m_sName is a String member variable of MyTypeName and "Name" is the unique name that we want to use to access
this property via reflection. Properties can also be implemented using getter/setter pairs:
  SEOUL_BEGIN_TYPE(MyTypeName)
    SEOUL_PARENT(MyParentTypeName)

	SEOUL_PROPERTY_PAIR_N("Name", GetName, SetName)
  SEOUL_END_TYPE()

where GetName and SetName are member functions of MyTypeName. Other syntaxes are possible - see "ReflectionDefine.h"
for all the possibilities. As a rule, accessing a property that is a member variable is faster than accessing it
as a getter/setter pair - so prefer this when possible.

- DO: define properties to make reflection really shine.
- DO: prefer properties that are member variables over getter/setter pairs for efficiency.

-----------------------------------------------------------------------------------------------------------------------
How To: Reflect Private Members and Control Property Access Privileges
-----------------------------------------------------------------------------------------------------------------------
The previous example will fail to compile if MyTypeName::m_sName is a private member of MyTypeName. To give access
to private members to the reflection system, include "ReflectionDeclare.h" in your type's header file and place
SEOUL_REFLECTION_FRIENDSHIP() in a private: block of the type's declaration:
  class MyTypeName
  {
    ...

  private:
    SEOUL_REFLECTION_FRIENDSHIP(MyTypeName);
  };

Doing so gives full access to the Reflection system, which may not be desirable - for example, you may want to access
a property for getting but not setting, or only expose a property for "storing" (serialization) but not getting or
setting. To control this, you can specify flags to your property definition to control access privileges:
  SEOUL_BEGIN_TYPE(MyTypeName)
    SEOUL_PARENT(MyParentTypeName)

	SEOUL_PROPERTY_N("Name", m_sName, PropertyFlags::kDisableSet)
  SEOUL_END_TYPE()

In this case, even though we have full access to the m_sName property, we don't want write access to the variable
through the reflection system, so we disable it. See "ReflectionProperty.h", the PropertyFlags namespace for the
full list of flags.

- DO   : use property flags to control access to property data
- DO   : include "ReflectionDeclare.h" in your header files and use SEOUL_REFLECTION_FRIENDSHIP() to grant acces to
         private members, when needed.
- DON'T: buy me a pony - I've given up on this dream, long ago.

-----------------------------------------------------------------------------------------------------------------------
How To: Define Methods
-----------------------------------------------------------------------------------------------------------------------
Like properties, you can also reflect methods:
  SEOUL_BEGIN_TYPE(MyTypeName)
    SEOUL_PARENT(MyParentTypeName)

	SEOUL_METHOD(WickedCool)
  SEOUL_END_TYPE()

where WickedCool is a member function of MyTypeName.

Note that method overloading is not currently supported. Reflecting two methods of the same name will not produce an error,
but you will only be able to access the first method with that name via Type::GetMethod(HString).

- DON'T: expose 2 methods with the same string name in the same type, method overloading is not supported.

-----------------------------------------------------------------------------------------------------------------------
How To: Access Properties and Call Methods
-----------------------------------------------------------------------------------------------------------------------
So, you have your fancy Type reflection with your fancy methods and properties - what good is that? Glad you asked!
To get data out of a property:
  MyTypeName* pInstanceOfMyTypeName = SEOUL_NEW(MemoryBudgets::TBD) MyTypeName;
  Reflection::Any anyValue;

  if (TypeOf<MyTypeName>().GetProperty(HString("Name"))->TryGet(pInstanceOfMyTypeName, anyValue))
  {
    ... do stuff with anyValue ...
  }

To invoke a method:
  Reflection::Any unusedAnyReturnValue;
  MethodArguments aArguments;
  aArguments[0] = 1.0f;
  if (TypeOf<MyTypeName>().GetMethod(HString("WickedCool"))->TryInvoke(unusedAnyReturnValue, pInstanceOfMyTypeName, aArguments))
  {
    ... be happy that we were able to invoke "Wicked Cool"...
  }

So, why is this cool? So far, this doesn't seem to give us anything beyond accessing pInstanceOfMyTypeName->m_sName or
calling pInstanceOfMyTypeName->WickedCool() (and the observant among you may notice that it also adds HString
construction and what appears to be some searching and converting of values).

When this becomes powerful is when you do somethign like this, using the fancy GetReflectionThis() method we defined
previously:
  WeakAny reflectionThis = pInstanceOfMyTypeName->GetReflectionThis();
If you didn't define that function, you can just do this:
  WeakAny reflectionThis(this);
Then:
  const Reflection::Type& actualType = reflectionThis.GetTypeInfo().GetType();
  for (UInt32 i = 0; i < actualType.GetPropertyCount(); ++i)
  {
	Any anyValue;
	if (actualType.GetProperty(i)->TryGet(pInstanceOfMyTypeName, anyValue))
	{
	  ... do something with whatever is in anyValue, like pass it to something else, serialize it, or print it.
	}
  }

Or, taking it even further...
  for (UInt32 i = 0; i < Reflection::Registry::GetRegistry().GetTypeCount(); ++i)
  {
    WeakAny someType = Reflection::Registry::GetRegistry().GetType(i).New(MemoryBudgets::TBD);
	if (someType.IsValid())
	{
	  .. enumerate the properties of someType and do stuff with them.
	}
  }

The trick with Reflection is that you can enumerate, create, and inspect C++ types like any data structure,
without compile time knowledge of the type or even its interface. This allows you to perform dynamic operations
on the type (i.e. enumerate all properties of all Components of all Objects in the scene and print their value).

- DO   : Use Reflection to get/set properties and invoke methods of Types unknown at compile time.
- DON'T: Use Reflection to simply invoke methods or properties on Types you *do* know at compile time - the first
         example is just to keep it simple, in practice, you will almost never get property values or invoke methods
		 on a Type that you know at compile time, there is no benefit in doing so.
- DO   : Use Reflection to enumerate properties, parents, and methods - even when you know the Type at compile time.
         Reflection is very useful for implementing patterns such as "for each property, print the property" or
		 "for each property, set the property to its default value".

-----------------------------------------------------------------------------------------------------------------------
How To: Use Attributes and Add Attributes to Types, Properties, and Methods
-----------------------------------------------------------------------------------------------------------------------
Attributes are classes of any type that can be used to add metadata to your Types, Properties, and Methods. For example:

  SEOUL_BEGIN_TYPE(MyTypeName)
    SEOUL_PARENT(MyParentTypeName)

	SEOUL_METHOD(WickedCool)
	  SEOUL_ATTRIBUTE(Description, "WickedCool is simply the best member function you'll ever call!")
  SEOUL_END_TYPE()

An attribute is associated with the Type, Property, or Method that comes immediately before it - so if you want to
associate an attribute with the Type, those attributes must be before any properties or methods that you define.

To use an attribute:
  WeakAny reflectionThis = pInstanceOfMyTypeName->GetReflectionThis();
  const Reflection::Type& actualType = reflectionThis.GetTypeId().GetType();
  for (UInt32 i = 0; i < actualType.GetPropertyCount(); ++i)
  {
	if (actualType.GetProperty(i)->GetAttributeCollection().HasAttribute(HString("Description"))
	{
		Attributes::Description const* pDescription = actualType.GetProperty(i)->GetAttributeCollection().GetAttribute<
		    Attributes::Description>();

		... now do something with the description, like print it...
	}
  }

Attributes are likely most useful for developer tools - variable tweaking, tests, error reporting, formatting, etc.

DO   : Use Attributes to associate rich metadata with your reflection Types, Properties, and Methods.
DO   : Use the SEOUL_DEV_ONLY_ATTRIBUTE() macro for metadata that will *only* be used in non-Ship builds, to reduce
       memory overhead of Reflection in shipping builds.
DON'T: Associate 2 Attributes of the same type (i.e. Attributes::Description) to a single target, this is not
       supported.

-----------------------------------------------------------------------------------------------------------------------
How To: Define Reflection for Enums
-----------------------------------------------------------------------------------------------------------------------
You can define Reflection for an enum, which then allows you to easily convert the values to/from their string names:
  SEOUL_BEGIN_ENUM(MyFancySetting::Enum)
    SEOUL_ENUM_N("DoAwesomeStuff", MyFancySetting::kDoAwesomeStuff)
	SEOUL_ENUM_N("DoLessAwesomeStuff", MyFancySetting::kDoLessAwesomeStuff)
	SEOUL_ENUM_N("DoMostAwesomeStuff", MyFancySetting::kDoMostAwesomeStuff)
  SEOUL_END_ENUM()

Once defined, you can interact with Reflection::Enum like so:

  Any any;
  ... do stuff that assigns a value of an unknown type to any...

  if (any.GetTypeInfo().IsEnum())
  {
    const Reflection::Enum& enum = *any.GetTypeInfo().TryGetEnum();

    HString enumName;
    if (enum.TryGetName(any, enumName))
    {
      ... do something with the enum name, like print it out or serialize it to a text file ...
    }
  }

-----------------------------------------------------------------------------------------------------------------------
How To: Work With Arrays
-----------------------------------------------------------------------------------------------------------------------
A type is considered an array in the context of reflection if FulfillsArrayContract<T>::Value is true. This is true if:
- T::ValueType is defined to the type of elements contained in the array
- UInt32 T::GetSize() const is defined and returns the size of the array
- T::ValueType* T::Get(UInt32 uIndex) is defined and returns a pointer to the element at uIndex in the array
- T::ValueType const* T::Get(UInt32 uIndex) const is defined and returns a const-pointer to the element at uIndex in
  the array.

Optionally, the member function void T::Resize(UInt32 zSize) can be defined, and if so, the array will be considered
resizable by reflection methods.

Once defined, you can interact with Reflection::Array like so:

  WeakAny array;
  ... do stuff that assigns a pointer to a type that fulfills the array contract to array...

  Reflection::Array const* pArray = array.GetType().GetTypeInfo().TryGetArray();
  if (nullptr != pArray)
  {
    WeakAny pointerToElementZero;
	if (pArray->TryGetElementPtr(array, 0u, pointerToElementZero)
	{
		...manipulate the value at the pointer...
	}

	if (pArray->TryResize(5u))
	{
	    ...manipulate elements 0-4 of the array that is now 5 elements in size...
	}
  }

NOTE: In the above case, you must call array.GetType().GetTypeInfo() to get the Array object for the pointed at array.
array.GetTypeInfo().TryGetArray() will return nullptr, because array is a *pointer* to the array object, not the array
object itself.

-----------------------------------------------------------------------------------------------------------------------
How To: Work With Tables
-----------------------------------------------------------------------------------------------------------------------
A type is considered a table in the context of reflection if FulfillsTableContract<T>::Value is true. This is true if:
- T::KeyType is defined to the type of keys contained in the table
- T::ValueType is defined to the type of values contained in the table
- T::ValueType* T::Find(const T::KeyType&) is defined and returns a pointer to the value at key in the table.
- T::ValueType const* T::Find(const T::KeyType&) const is defined and returns a const-pointer to the value at
  key in the table.

Optionally, the member function Bool T::Erase(const T::KeyType&) can be defined, and if so, the array table will
be considered erasable by reflection methods.

Once defined, you can interact with Reflection::Table like so:

  WeakAny table;
  ... do stuff that assigns a pointer to a type that fulfills the table contract to table...

  Reflection::Table const* pTable = tabley.GetType().GetTypeInfo().TryGetTable();
  if (nullptr != pTable)
  {
    WeakAny pointerToValue;
	if (pTable->TryGetValuePtr(table, HString("MyKey"), pointerToValue)
	{
		...manipulate the value at the pointer...
	}

	// The last argument to this method indicates that a new value should be inserted
	// if it is not already in the table, and a pointer will be returned to that element.
	if (pTable->TryGetValuePtr(table, HString("MyNewKey"), pointerToValue, true))
	{
		...manipulate the newly inserted element at "MyNewKey".
	}
  }

NOTE: In the above case, you must call table.GetType().GetTypeInfo() to get the Table object for the pointed at table.
table.GetTypeInfo().TryGetTable() will return nullptr, because table is a *pointer* to the table object, not the table
object itself.

NOTE: The above examples only work if the table in WeakAny table has a key type of HString, or a key type that
is constructable from an HString.

-----------------------------------------------------------------------------------------------------------------------
Todo/Known Issues:
-----------------------------------------------------------------------------------------------------------------------
- Missing features/rough edges:
  - no operator overloading - methods cannot have the same name with different signatures.
  - upcasting with Type::CastTo<> is not supported.
  - enum values as Int does not respect the fact that C++ enum can have different sizes.
  - Seoul::IsEnum<> requires manaul specialization using the SEOU_IS_ENUM() macro.
  - ReflectionCast<>, Type::CastTo<>, PointerCast<>, and TypeConstruct<> provide nearly the same or identical
    functionality, and have names that are not entirely accurate to describe what they actually do (PointerCast<> is
	actually more of a pointer cast).
  - Reflection::Registry() does not seem to be a useful class - probably cleaner to merge the GetType(name) form into
    either a static function of Type<>, or a global function like TypeOf<>
  - Methods that pass by reference (i.e. (const Int&) or (Int&)) are supported, but the arguments will be copied,
    so Pass by reference of non copyable types is not supported.

- Performance optimizations:
  - casting, acquiring methods, and acquiring properties by name not only need to walk an unsorted list, but also
    potentially traverse a graph of types.

- Readability/Usability
  - Need more documentation and some cleanup of some of the internal headers
  - Need an overview document to explain how the different bits tie together
  - Need to move stuff in TypeDetail into an Internal header (from Type.h)
  - May need to break up ReflectionDefine.h

- Edge cases
  - TypeId<> of types with various modifiers is likely not handled properly in some contexts
    (i.e. see TypeId<>::GetType() for some of the special handling that can be necessary).
  - Pointers - should not be storable/restorable.
  - WeakAny <-> Any conversion will be needed in some usage scenarious (setting a value to a property from data stored
    in an Any).
  - Very easy to forget a SEOUL_REFLECTION_POLYMORPHIC(blah) macro in a deep class hierarchy, and this will not be caught
    by any error checking. It will only make itself known by the fact that calling GetReflectionThis() will not return
	the expected Reflection::Type() is some instances.

-----------------------------------------------------------------------------------------------------------------------
Questions:
-----------------------------------------------------------------------------------------------------------------------
Q: What is the difference between Reflection::Type and Reflection::TypeInfo? Why do they both exist?

A: TypeInfo is a "lite" class that is primarily descriptive of a C++ type, while Type is a
"heavy" class that is primarily manipulative of a C++ object. TypeInfo reflects modifiers while
Type does not. For example, "char", "const char", "char*", "char**", and "char&" are all associated
with different TypeInfo objects but the same Type object.

This can result in some unclear/unexpected results:
- calling WeakAny::GetType() may return a Type object that can't actually manipulate the WeakAny
  instance that returned the Type object (for example, if the WeakAny wraps a "Foo**" - most of
  the methods of Type will expect a "Foo*" or a "Foo const*".
- the Type object of 2 WeakAny instances may be ==, while the TypeInfo objects are not.
