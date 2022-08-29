// Glue required by SlimCS. Be careful with modifications - this
// file defines much of the backend support functionality needed
// by Lua. As a result, using certain concepts in this file
// (e.g. a cast) will generate infinite recursion or circular
// dependencies.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

[TopLevelChunk]
static class SlimCSMainTopLevelChunk
{
	interface INewable
	{
		dval<object> New(object o);
	}

	static void Chunk()
	{
		var InvokeConstructor = (InvokeConstructorDelegate)((tClass, sCtorName, oInstance, vararg0) =>
		{
			if (null == tClass)
			{
				return;
			}

			var ctor = dyncast<ConstructorDelegate>(dyncast<Table>(tClass)[sCtorName]);
			if (null != ctor)
			{
				ctor(oInstance, vararg0);
			}
		});

		// The default __tostring just returns the fully qualified name.
		var DefaultToString = (ToStringDelegate)((tClass) =>
		{
			return dyncast<Table>(tClass)["m_sFullClassName"];
		});

		// Clone all members (except 'cctor'
		// (static constructor), and '__cinit' (our internal
		// 'class initializer').
		var tDoNotCopy = new Table
		{
			["cctor"] = true,
			["__cinit"] = true
		};

		_G["Activator"] = new Table();
		_G["Activator"]["CreateInstance"] = (System.Func<Table, object>)((type) =>
		{
			return dyncast<INewable>(type["m_Class"]).New(null);
		});

		_G["FinishClassTableInit"] = (VfuncT1<object>)((t) =>
		{
			// Check if the class has already been finished.
			if (null != rawget(t, "^table-setup"))
			{
				return;
			}

			// Done with table setup, one way or another.
			rawset(t, "^table-setup", true);

			// Run the (hacky, would like to remove) class 'initalizer'
			// if one is defined.
			var cinit = rawget(t, "__cinit");
			if (null != cinit)
			{
				dyncast<VfuncT1<object>>(cinit)(t);
			}

			var tSuper = dyncast<Table>(t)["m_tSuper"];
			if (null != tSuper)
			{
				dyncast<VfuncT1<object>>(_G["FinishClassTableInit"])(tSuper);

				// Merge super members.
				foreach ((var k, var v) in pairs(tSuper))
				{
					if (true != tDoNotCopy[k] && null == rawget(t, k))
					{
						rawset(t, k, v);
					}
				}

				var tIs = dyncast<Table>(t)["m_tIs"];
				foreach ((var k, var _) in pairs(dyncast<Table>(tSuper)["m_tIs"]))
				{
					rawset(tIs, k, true);
				}
			}

			// After propagation has completed,
			// check for __init and __tostring.
			// If not explicitly defined, give
			// defaults.

			// Default __tostring just returns the name of the class.
			if (null == rawget(t, "__tostring"))
			{
				rawset(t, "__tostring", DefaultToString);
			}

			// Default __index handler is just the class table itself.
			var index = rawget(t, "__index");
			if (null == index || index == tSuper)
			{
				rawset(t, "__index", t);
			}
		});

		var OverrideNew = (OverrideNewDelegate)((tClass, sCtorName, vararg0) =>
		{
			var oInstance = new Table { };
			setmetatable(oInstance, tClass);
			InvokeConstructor(tClass, sCtorName, oInstance, vararg0);
			return oInstance;
		});
		var DefaultNew = (DefaultNewDelegate)((tClass, vararg0) =>
		{
			return OverrideNew(tClass, "Constructor", vararg0);
		});

		_G["class"] = (ClassDelegate)((sName, sSuper, sFullyQualifiedName, sShortName, vararg0) =>
		{
			var tClass = _G[sName];
			if (null == tClass)
			{
				tClass = new Table { };
				_G[sName] = tClass;
			}

			// This is partial class support. If
			// tClass already has a valid m_sClassName
			// member, return. We've already
			// setup the class table.
			if (null != rawget(tClass, "m_sClassName"))
			{
				return tClass;
			}

			object tSuper = null;
			if (null == sSuper)
			{
				if (tClass != _G["object"])
				{
					tSuper = _G["object"];
				}
			}
			else
			{
				// Handling for generics - the class table will have been passed in.
				if (type(sSuper) == "table")
				{
					tSuper = sSuper;
				}
				else
				{
					tSuper = _G[sSuper];
					if (null == tSuper)
					{
						tSuper = new Table { };
						_G[sSuper] = tSuper;
					}
				}
			}

			// Sanitize.
			sFullyQualifiedName = sFullyQualifiedName ?? sName;
			sShortName = sShortName ?? sName;

			// Set the class, full name, and super fields.
			tClass["m_sFullClassName"] = sFullyQualifiedName;
			tClass["m_sShortClassName"] = sShortName;
			tClass["m_sClassName"] = sName;
			tClass["m_tSuper"] = tSuper;

			// Build a lookup table for 'as' and 'is'
			// queries. Interfaces have string keys,
			// parent classes use the parent table as the key.
			var tIs = new Table { };
			var args = vararg0;
			foreach ((var _, var v) in pairs(args))
			{
				tIs[v] = true;
			}
			tIs[tClass] = true;

			tClass["m_tIs"] = tIs;

			// Setup handlers.
			tClass["New"] = DefaultNew;
			tClass["ONew"] = OverrideNew;

			return tClass;
		});

		// TODO: A static class may need to be more than just a table?
		_G["class_static"] = (ClassStaticDelegate)((sName) =>
		{
			var tClass = _G[sName];
			if (null == tClass)
			{
				tClass = new Table { };
				_G[sName] = tClass;
			}

			return tClass;
		});

		_G["interface"] = (InterfaceDelegate)((sName, sFullyQualifiedName, sShortName, vararg0) =>
		{
			var tInterface = _G[sName];
			if (null == tInterface)
			{
				tInterface = new Table { };
				_G[sName] = tInterface;
			}

			// Sanitize.
			sFullyQualifiedName = sFullyQualifiedName ?? sName;
			sShortName = sShortName ?? sName;

			// Set the class, full name, and super fields.
			tInterface["m_sFullClassName"] = sFullyQualifiedName;
			tInterface["m_sShortClassName"] = sShortName;
			tInterface["m_sClassName"] = sName;

			// Build a lookup table for 'as' and 'is'
			// queries. Interfaces have string keys,
			// parent classes use the parent table as the key.
			var tIs = new Table { };
			var tmp = vararg0;
			var args = dyncast<Table>(tmp);
			foreach ((var _, var v) in pairs(args))
			{
				tIs[v] = true;
			}
			tIs[tInterface] = true;
			tIs[sName] = true;

			tInterface["m_tIs"] = tIs;

			return tInterface;
		});

		// builtin string library extensions
		astable(typeof(@string))["align"] = (AlignDelegate)((s, i) =>
		{
			s = tostring(s);
			var l = @string.len(s);
			var pad = math.abs(i) - l;
			if (pad <= 0)
			{
				return s;
			}

			if (i < 0)
			{
				return concat(s, @string.rep(" ", pad));
			}
			else if (i > 0)
			{
				return concat(@string.rep(" ", pad), s);
			}
			else
			{
				return s;
			}
		});

		astable(typeof(@string))["find_last"] = (FindLastDelegate)((s, pattern, index, plain) =>
		{
			var find = new FindLastDelegate(@string.find);
			(double? i, double? j) = (null, null);
			(double? lastI, double? lastJ) = (null, null);
			do
			{
				(lastI, lastJ) = (i, j);
				(i, j) = find(s, pattern, index, plain);
				if (null != j)
				{
					index = j + 1;
				}
			} while (null != i);

			if (null == lastI)
			{
				return (null, null);
			}
			else
			{
				return (lastI, lastJ);
			}
		});

		// TODO: Possibly temp.
		_G["Table"] = new Table { }; // TODO: Filter this out at compile time?
		var final_class = (FinalClassDelegate)((sName, sSuper, sFullyQualifiedName, sShortName, aArgs) =>
		{
			var t = dyncast<ClassDelegate>(_G["class"])(sName, sSuper, sFullyQualifiedName, sShortName, aArgs);
			dyncast<VfuncT1<object>>(_G["FinishClassTableInit"])(t);
			return t;
		});

		final_class("object");
		_G["Object"] = _G["object"]; // TODO: Alias is the way to go?
		_G["Object"]["m_sFullClassName"] = "System.Object";
		final_class("char");
		final_class("bool", null, null, "Boolean");
		_G["bool"]["__default_value"] = false;
		_G["bool"]["__arr_read_placeholder"] = false;
		_G["Boolean"] = _G["bool"]; // TODO: Alias is the way to go?
		final_class("float", null, null, "Single");
		_G["float"]["__default_value"] = 0;
		_G["Single"] = _G["float"]; // TODO: Alias is the way to go?
		final_class("double", null, null, "Double");
		_G["double"]["__default_value"] = 0;
		_G["Double"] = _G["double"]; // TODO: Alias is the way to go?
		final_class("sbyte", null, null, "SByte");
		_G["sbyte"]["__default_value"] = 0;
		final_class("short", null, null, "Int16");
		_G["short"]["__default_value"] = 0;
		final_class("int", null, null, "Int32");
		_G["int"]["__default_value"] = 0;
		final_class("long", null, null, "Int64");
		_G["long"]["__default_value"] = 0;
		final_class("byte", null, null, "Byte");
		_G["byte"]["__default_value"] = 0;
		final_class("ushort", null, null, "UShort");
		_G["ushort"]["__default_value"] = 0;
		final_class("uint", null, null, "UInt32");
		_G["uint"]["__default_value"] = 0;
		final_class("ulong", null, null, "UInt64");
		_G["ulong"]["__default_value"] = 0;
		final_class("String", null, "string");
		_G["String"]["m_sClassName"] = "string"; // TODO:
		final_class("Action"); // TODO: Don't handle these specially?
		final_class("NullableBoolean", null, null, "Nullable`1"); // TODO: Don't handle these specially?
		final_class("NullableDouble", null, null, "Nullable`1"); // TODO: Don't handle these specially?
		final_class("NullableInt32", null, null, "Nullable`1"); // TODO: Don't handle these specially?
		final_class("Type"); // TODO: Don't handle these specially?
		_G["Type"]["__tostring"] = (GetNameDelegate)((t) =>
		{
			return t["m_Class"]["m_sFullClassName"];
		});

		_G["Char"] = _G["char"]; // TODO: Alias is the way to go?
		_G["Int32"] = _G["int"]; // TODO: Alias is the way to go?

		// TODO: This approache loses specific type checking
		// on delegate casts.
		final_class("Delegate");

		final_class("Attribute");
		_G["Attribute"]["m_sClassName"] = "System.Attribute"; // TODO:

		final_class("Exception");
		_G["Exception"]["m_sClassName"] = "System.Exception"; // TODO:
		_G["Exception"]["Constructor"] = (ExceptionConstructorDelegate)((self, msg) =>
		{
			msg = msg ?? concat("Exception of type '", dyncast<Table>(self)["m_sClassName"], "' was thrown.");
			dyncast<Table>(self)["Message"] = msg;
		});
		_G["Exception"]["get_Message"] = (ExceptionMessageDelegate)((self) =>
		{
			return dyncast<Table>(self)["Message"];
		});
		_G["Exception"]["__tostring"] = (ToStringDelegate)((self) =>
		{
			return
				dyncast<string>(dyncast<Table>(self)["m_sClassName"]) +
				": " +
				dyncast<string>(dyncast<Table>(self)["Message"]);
		});
		final_class("ArgumentException", "Exception");
		_G["ArgumentException"]["Constructor"] = (ExceptionConstructorDelegate)((self, msg) =>
		{
			dyncast<Table>(self)["Message"] = msg;
		});

		final_class("NullReferenceException", "Exception");
		_G["NullReferenceException"]["Constructor"] = (ExceptionConstructorDelegate)((self, msg) =>
		{
			msg = msg ?? "Object reference not set to an instance of an object.";
			dyncast<Table>(self)["Message"] = msg;
		});

		var tWrappers = new Table<string, ErrorWrapperDelegate>
		{
			["nil"] = (ErrorWrapperDelegate)((err) =>
			{
				return err;
			}),
			["string"] = (ErrorWrapperDelegate)((err) =>
			{
				return dyncast<INewable>(_G["Exception"]).New(err);
			}),
			["table"] = (ErrorWrapperDelegate)((err) =>
			{
				var cls = getmetatable(err);
				if (null == cls || null == cls["get_Message"])
				{
					return dyncast<INewable>(_G["Exception"]).New(tostring(err));
				}
				else
				{
					return err;
				}
			})
		};

		var wrapError = (ErrorWrapperDelegate)((err) =>
		{
			var sType = type(err);
			var f = tWrappers[sType];
			return f(err);
		});

		_G["try"] = (TryDelegate)((tryblock, vararg0) =>
		{
			// TODO: This breaks for userland functions that
			// return multiple return values. But I'd rather not
			// add the overhead for an uncommon case, so we'll
			// need another pair (tryfinallymultiple and trymultiple)
			(bool bSuccess, object res, object ret) = (Tuple<bool, object, object>)pcall(tryblock);
			if (!bSuccess)
			{
				res = wrapError(res);

				var tmp = vararg0;
				var args = dyncast<Table>(tmp);
				foreach (double i in range(1, lenop(args), 2))
				{
					var test = dyncast<TestDelegate>(args[i]);

					// Invoke the test to see if this catch should be invoked.
					if (test(res))
					{
						var @catch = args[i + 1];
						(var bNewSuccess, var newRes, var newRet) = (Tuple<bool, object, object>)pcall(@catch, res);

						bSuccess = bNewSuccess;
						if (!bSuccess)
						{
							newRes = wrapError(newRes);
							if (null != newRes)
							{
								res = newRes;
							}
						}
						else
						{
							res = newRes;
						}
						ret = newRet;

						// Done.
						break;
					}
				}
			}

			if (!bSuccess)
			{
				error(res);
			}

			return (res, ret);
		});

		_G["tryfinally"] = (TryDelegate)((tryblock, vararg0) =>
		{
			(var res, var ret) = dyncast<TryDelegate>(_G["try"])(tryblock, vararg0);

			var tmp = vararg0;
			var args = dyncast<Table>(tmp);
			var finallyFunc = dyncast<Vfunc0>(args[lenop(args)]);
			finallyFunc();

			return (res, ret);
		});

		_G["using"] = (UsingDelegate)((vararg0) =>
		{
			var tmp = vararg0;
			var args = dyncast<Table>(tmp);
			var iArgs = lenop(args);
			var block = args[iArgs];
			(var bSuccess, var res, var ret) = (Tuple<bool, object, object>)pcall(block, vararg0);

			foreach (double i in range(iArgs - 1, 1, -1))
			{
				if (args[i] != null)
				{
					dyncast<IDisposable>(args[i]).Dispose();
				}
			}

			if (!bSuccess)
			{
				error(wrapError(res));
			}

			return (res, ret);
		});

		var tLookups = new Table<string, LookupDelegate>
		{
			["boolean"] = (LookupDelegate)((_0, b) =>
			{
				return b == _G["bool"] || b == _G["object"] || b == _G["NullableBoolean"];
			}),
			["function"] = (LookupDelegate)((_0, b) =>
			{
				return b == _G["Delegate"] || b == _G["object"];
			}),
			["nil"] = (LookupDelegate)((_0, b) =>
			{
				return false;
			}),
			["number"] = (LookupDelegate)((_0, b) =>
			{
				return b == _G["double"] || b == _G["int"] || b == _G["object"] || b == _G["NullableDouble"] || b == _G["NullableInt32"];
			}), // TODO: Temp, allow int or double.
			["string"] = (LookupDelegate)((_0, b) =>
			{
				return b == _G["String"] || b == _G["object"];
			}),
			["table"] = (LookupDelegate)((a, b) =>
			{
				return b == _G["Table"] || true == dyncast<Table>(a)?["m_tIs"]?[b] || false;
			}),
			["thread"] = (LookupDelegate)((_0, b) =>
			{
				return false;
			}),
			["userdata"] = (LookupDelegate)((_0, b) =>
			{
				return false;
			}), // TODO: should actually evaluate, not just return false.
		};
		var tTypeLookup = new Table<string, TypeLookupDelegate>
		{
			["boolean"] = (TypeLookupDelegate)((_0) =>
			{
				return _G["bool"];
			}),
			["function"] = (TypeLookupDelegate)((_0) =>
			{
				return _G["Delegate"];
			}),
			["number"] = (TypeLookupDelegate)((_0) =>
			{
				return _G["double"];
			}),
			["string"] = (TypeLookupDelegate)((_0) =>
			{
				return _G["String"];
			}),
			["table"] = (TypeLookupDelegate)((a) =>
			{
				return getmetatable(a);
			})
		};

		var s_tTypes = new Table { };
		_G["TypeOf"] = (TypeOfDelegate)((o) =>
		{
			var ret = s_tTypes[o];
			if (null == ret)
			{
				ret = new Table { };

				// Fill out our minimal System.Reflection.Type table.
				var dyn = dyncast<Table>(o);
				ret["m_Class"] = dyn;
				ret["get_Name"] = (GetNameDelegate)((t) =>
				{
					return t["m_Class"]["m_sShortClassName"];
				});

				// ToString() support.
				setmetatable(ret, _G["Type"]);

				s_tTypes[o] = ret;
			}
			return ret;
		});

		_G["GetType"] = (TypeOfDelegate)((o) =>
		{
			return dyncast<TypeOfDelegate>(_G["TypeOf"])(tTypeLookup[type(o)](o));
		});

		_G["ToString"] = (ToStringDelegate)((o) =>
		{
			return tostring(o);
		});

		_G["__bind_delegate__"] = (BindDelegate)((self, func) =>
		{
			var ret = dyncast<Table>(self)[func];
			if (null == ret)
			{
				ret = (AnyDelegate)((args) =>
				{
					return dyncast<AnyBoundDelegate>(func)(self, args);
				});
				dyncast<Table>(self)[func] = ret;
			}
			return ret;
		});

		_G["arraycreate"] = (ArrayCreateDelegate)((size, val) =>
		{
			// Apply our filtering behavior - we use
			// false as a placeholder for nil.
			if (val == null)
			{
				val = false;
			}

			// TODO: Do this in native with
			// a lua extension. Note that this
			// is quite a bit faster than table.insert().
			var a = new Table { };
			foreach (double i in range(1, size))
			{
				a[i] = val;
			}

			return a;
		});

		_G["genericlookup"] = (GenericLookupDelegate)((sType, sId, vararg0) =>
		{
			var genericType = _G[sId];
			if (null == genericType)
			{
				genericType = dyncast<ClassDelegate>(_G["class"])(sId, sType, sId, sId);

				var tmp = vararg0;
				var args = dyncast<Table>(tmp);
				foreach (double i in range(1, lenop(args), 2))
				{
					rawset(genericType, args[i], args[i + 1]);
				}

				// Finalize.
				if (null != rawget(_G[sType], "^table-setup"))
				{
					dyncast<VfuncT1<object>>(_G["FinishClassTableInit"])(genericType);
					if (null != genericType["cctor"])
					{
						((Vfunc0)genericType["cctor"])();
					}
				}

				_G[sId] = genericType;
			}
			return genericType;
		});

		_G["initarr"] = (InitListDelegate)((inst, vararg0) =>
		{
			var tmp = vararg0;
			var args = dyncast<Table>(tmp);
			double i = 1;
			var add_ = dyncast<System.Action<object, object>>(dyncast<Table>(inst)["Add"]);
			while (args[i] != null)
			{
				var v = args[i];
				add_(inst, v);
				i++;
			}

			return inst;
		});

		_G["initlist"] = (InitListDelegate)((inst, vararg0) =>
		{
			var tmp = vararg0;
			var args = dyncast<Table>(tmp);
			double i = 1;
			while (args[i] != null)
			{
				bool bProp = dyncast<bool>(args[i]);
				var k = args[i + 1];
				var v = args[i + 2];

				if (bProp)
				{
					var f = dyncast<VfuncT2<object, object>>(dyncast<Table>(inst)[k]);
					f(inst, v);
				}
				else
				{
					rawset(inst, k, v);
				}

				i += 3;
			}

			return inst;
		});

		_G["is"] = (IsDelegate)((a, b) =>
		{
			var f = dyncast<IsDelegate>(tLookups[type(a)]);
			return f(a, b);
		});

		_G["as"] = (AsDelegate)((a, b) =>
		{
			var f = dyncast<IsDelegate>(tLookups[type(a)]);
			if (f(a, b))
			{
				return a;
			}
			else
			{
				return null;
			}
		});

		_G["asnum"] = (ToNumDelegate)((a) =>
		{
			return a ?? 0;
		});

		_G["__cast__"] = (AsDelegate)((a, b) =>
		{
			var ret = dyncast<AsDelegate>(_G["as"])(a, b);
			if (ret == null && a != null)
			{
				var classA = tTypeLookup[type(a)](a);
				var classB = b;
				error(
					dyncast<INewable>(_G["Exception"]).New(
						concat("Cannot convert type '", dyncast<Table>(classA)["m_sFullClassName"], "' to '", dyncast<Table>(classB)["m_sFullClassName"], "'")),
					1);
			}
			return ret;
		});

		var __i32truncate__ = (DoubleOp)_G["math"]["i32truncate"];
		_G["__castint__"] = (DoubleConvertOp)((a) =>
		{
			var typeA = type(a);
			if (typeA == "number")
			{
				return __i32truncate__(dyncast<double>(a));
			}
			else
			{
				if (null == a)
				{
					error(dyncast<INewable>(_G["Exception"]).New("Object reference not set to an instance of an object."), 1);
				}
				else
				{
					var classA = tTypeLookup[typeA](a);
					error(dyncast<INewable>(_G["Exception"]).New(concat("Cannot convert type '", dyncast<Table>(classA)["m_sFullClassName"], "' to 'int'")), 1);
				}
				return 0;
			}
		});

		_G["__bband__"] = (BoolBitOp)((a, b) =>
		{
			return a && b;
		});
		_G["__bbor__"] = (BoolBitOp)((a, b) =>
		{
			return a || b;
		});
		_G["__bbxor__"] = (BoolBitOp)((a, b) =>
		{
			return a != b;
		});
		_G["__i32mod__"] = (DoubleBinOp)_G["math"]["i32mod"];
		_G["__i32mul__"] = (DoubleBinOp)_G["math"]["i32mul"];
		_G["__i32narrow__"] = (DoubleOp)_G["bit"]["tobit"];
		_G["__i32truncate__"] = __i32truncate__;
	}

	delegate string AlignDelegate(string s, double i);

	delegate object AnyDelegate(params object[] args);

	delegate object AnyBoundDelegate(object self, params object[] args);

	delegate Table ArrayCreateDelegate(double size, object val);

	delegate object AsDelegate(object a, object b);

	delegate object BindDelegate(object self, object func);

	delegate bool BoolBitOp(bool a, bool b);

	delegate object ClassDelegate(string sName, string sSuper = null, string sFullyQualifiedName = null, string sOrigName = null, params object[] aArgs);

	delegate object ClassStaticDelegate(string sName);

	public delegate void ConstructorDelegate(object oInstance, params object[] vararg0);

	delegate object DefaultNewDelegate(object tClass, params object[] aArgs);

	delegate double DoubleOp(double f);

	delegate double DoubleBinOp(double a, double b);

	delegate double DoubleConvertOp(object f);

	delegate bool IsDelegate(object a, object b);

	delegate object ErrorWrapperDelegate(object err);

	delegate void ExceptionConstructorDelegate(object oSelf, string sMessage);

	delegate string ExceptionMessageDelegate(object oSelf);

	delegate object FinalClassDelegate(string sName, string sSuper = null, string sFullyQualifiedName = null, string sShortName = null, params object[] aArgs);

	delegate (double?, double?) FindLastDelegate(string s, string pattern, double? init = null, bool plain = false);

	delegate object GenericLookupDelegate(string sType, string sId, params object[] aArgs);

	delegate string GetNameDelegate(Table t);

	delegate object InitListDelegate(object o, params object[] aArgs);

	delegate object InterfaceDelegate(string sName, string sFullyQualifiedName = null, string sOrigName = null, params object[] aArgs);

	delegate void InvokeConstructorDelegate(object tClass, string sCtorName, object oInstance, params object[] aArgs);

	delegate bool LookupDelegate(object a, object b);

	delegate object OverrideNewDelegate(object tClass, string sClassName, params object[] aArgs);

	delegate bool TestDelegate(object o);

	delegate double ToNumDelegate(double? a);

	delegate string ToStringDelegate(object a);

	delegate (object, object) TryDelegate(object tryFunction, params object[] aArgs);

	delegate object TypeLookupDelegate(object a);

	delegate object TypeOfDelegate(object o);

	delegate (object, object) UsingDelegate(params object[] aArgs);
}

public interface IDisposable
{
	void Dispose();
}
