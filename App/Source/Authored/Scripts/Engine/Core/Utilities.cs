// Miscellaneous core engine utilities.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

using System.Diagnostics;

public static class CoreUtilities
{
	delegate void ConstructDelegate(object oInstance, params object[] aArgs);

	delegate Table CreateTableDelegate(double? iArrayCapacity, double? iRecordCapacity);

	delegate EnumDescription DescribeNativeEnumDelegate(string sName);

	delegate object DescribeNativeUserDataDelegate(string sName);

	delegate object NativeNewNativeUserDataDelegate(string sName);

	/// <summary>
	/// Debug only - raise an exception if b is not true.
	/// </summary>
	/// <param name="b">Condition to check.</param>
	/// <param name="msg">Message to include with the raised exception.</param>
	[Conditional("DEBUG")]
	public static void Assert(bool b, string msg)
	{
		SlimCS.assert(b, msg);
	}

	/// <summary>
	/// Returns a new table with the given capacities.
	/// </summary>
	/// <param name="iArrayCapacity">Size of the array used to store values in the table.</param>
	/// <param name="iRecordCapacity">Size of the array used to store keys in the table.</param>
	/// <returns>A new Table with the given capacities.</returns>
	/// <remarks>
	/// A Table used like an array should pass null for the record capacity. The Table implementation
	/// only needs a record array if it is sparse or uses keys that are not numbers.
	/// </remarks>
	public static Table CreateTable(double? iArrayCapacity = null, double? iRecordCapacity = null)
	{
		return dyncast<CreateTableDelegate>(_G["SeoulCreateTable"])(iArrayCapacity, iRecordCapacity);
	}

	// Wrap the native SeoulDescribeNativeEnum.
	public static EnumDescription DescribeNativeEnum(string sName)
	{
		return dyncast<DescribeNativeEnumDelegate>(_G["SeoulDescribeNativeEnum"])(sName);
	}

	// Wrap the native SeoulDescribeNativeUserData.
	public static object DescribeNativeUserData(string sName)
	{
		return dyncast<DescribeNativeUserDataDelegate>(_G["SeoulDescribeNativeUserData"])(sName);
	}

	// Wrap the native SeoulNativeNewNativeUserData with a wrapped version that
	// automagically invokes Construct(), if the native type exposes it.
	public static object NewNativeUserDataByName(string sName, params object[] vararg0)
	{
		// Instantiate the type.
		var ud = dyncast<NativeNewNativeUserDataDelegate>(_G["SeoulNativeNewNativeUserData"])(sName);

		// If constructed and has a Construct() method,
		// invoke it.
		if (null != ud)
		{
			var construct = dyncast<ConstructDelegate>(dyncast<Table>(ud)["Construct"]);
			if (null != construct)
			{
				construct(ud, vararg0);
			}
		}

		return ud;
	}

	public static T NewNativeUserData<T>(params object[] vararg0)
	{
		// Instantiate the type.
		var ud = dyncast<NativeNewNativeUserDataDelegate>(_G["SeoulNativeNewNativeUserData"])(typeof(T).Name);

		// If constructed and has a Construct() method,
		// invoke it.
		if (null != ud)
		{
			var construct = dyncast<ConstructDelegate>(dyncast<Table>(ud)["Construct"]);
			if (null != construct)
			{
				construct(ud, vararg0);
			}
		}

		return dyncast<T>(ud);
	}

	// If value is nil, returns default; otherwise returns value
	public static T WithDefault<T>(object value, T def)
	{
		if (value == null)
		{
			return def;
		}

		return (T)value;
	}

	// Iterate over the elements in the table and return a count of them
	public static double CountTableElements(Table tValue)
	{
		double iCount = 0;
		foreach ((var _, var _) in pairs(tValue))
		{
			iCount = iCount + 1;
		}
		return iCount;
	}

	/// <summary>
	/// Log an object to the given logger channel.
	/// </summary>
	/// <param name="sChannel">Channel to log to.</param>
	/// <param name="value">object to log.</param>
	/// <param name="fOptionalMaxDepth">Optional argument to limit how much of the tree is printed.</param>
	[Conditional("DEBUG")]
	public static void LogTableOrTostring(string sChannel, object value, double? fOptionalMaxDepth = null)
	{
		if (type(value) == "table")
		{
			LogTable(sChannel, (Table)value, fOptionalMaxDepth);
		}
		else
		{
			CoreNative.LogChannel(sChannel, tostring(value));
		}
	}

	/// <summary>
	/// Searches through a table and returns the index of the string found... else -1;
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <param name="table"></param>
	/// <param name="toFind"></param>
	/// <returns></returns>
	internal static double FindStringInTable(Table<double, string> table, string toFind)
	{
		foreach ((double index, var value) in ipairs(table))
		{
			if (value == toFind)
			{
				return index;
			}
		}

		return -1;
	}

	/// <summary>
	/// Log a table to the given logger channel.
	/// </summary>
	/// <param name="sChannel">Channel to log to.</param>
	/// <param name="value">Table to log.</param>
	/// <param name="fOptionalMaxDepth">Optional argument to limit how much of the tree is printed.</param>
	[Conditional("DEBUG")]
	public static void LogTable(string sChannel, Table value, double? fOptionalMaxDepth = null)
	{
		var tCache = new Table { };
		VfuncT3<Table, string, double?> nestedPrint = null;
		nestedPrint = (t, indent, remainingDepth) =>
		{
			if (remainingDepth == 0)
			{
				return;
			}

			if (null != tCache[tostring(t)])
			{
				CoreNative.LogChannel(sChannel, concat(indent, "ref ", tostring(t)));
			}
			else
			{
				tCache[tostring(t)] = true;
				CoreNative.LogChannel(sChannel, concat(indent, "{"));
				indent = concat(indent, "  ");
				foreach ((var k, var v) in pairs(t))
				{
					if ("table" == type(v))
					{
						CoreNative.LogChannel(sChannel, concat(indent, tostring(k), " ="));
						nestedPrint((Table)v, indent, remainingDepth - 1);
					}
					else
					{
						CoreNative.LogChannel(sChannel, concat(indent, tostring(k), " = ", tostring(v)));
					}
				}
				CoreNative.LogChannel(sChannel, concat(indent, "}"));
			}
		};

		if (fOptionalMaxDepth == null)
		{
			fOptionalMaxDepth = -1;
		}

		if ("table" == type(value))
		{
			nestedPrint(value, string.Empty, fOptionalMaxDepth);
		}
		else
		{
			CoreNative.LogChannel(sChannel, tostring(value));
		}
	}

	public static readonly Table kWeakMetatable = new Table()
	{
		["__mode"] = "kv" // weak table
	};

	public static Table MakeWeakTable()
	{
		var ret = new Table();
		setmetatable(ret, kWeakMetatable);
		return ret;
	}

	// Returns a random number drawn from a gaussian (aka normal) distribution with the given mean and standard deviation.
	// There is the optional ability to limit the max number of standard devitions away from the mean. Using this param
	// results in the returned values no longer being from a truely a normal distrubution (for example this will change the
	// standard deviation of the returned values) but the shape of the clipped curve will be normal between the max values.
	// This function is relatively slow due to the use of math.exp, math.sqrt, math.log, math.cos, and two calls to math.random.
	public static double SlowRandomGaussian(double fMean, double fStandardDeviation, double? fOptionalMaxStandardDeviation = null)
	{
		// When we clip the gaussian with the fOptionalMaxStandardDeviation param we need to figure out
		// what to do with the 'extra' area outside of that given range. Below we explore two
		// ways to do this.

		// Method 1  - Fill in extra area evenly
		// In this method, if a value is generated outside the given fOptionalMaxStandardDeviation
		// range, we just pick a new uniformly distributed random number and return that. This
		// essentially means that we're lifting the whole gaussian curve 'up' to account for
		// the 'extra' area we clipped off the edges.
		//
		// Calculate a normal with mean 0 and std dev 1
		var p = math.sqrt(-2 * math.log(1 - math.random()));
		var rand = p * math.cos(2 * math.pi * math.random());

		// If it's outside the desired range (if that is specified and non-zero), pick a random
		// (uniformly distributed value) value in the desired range.
		if (null != fOptionalMaxStandardDeviation)
		{
			var d = asnum(fOptionalMaxStandardDeviation);
			if (rand > d || rand < -d)
			{
				rand = 2 * (math.random() - 0.5) * d;
			}
		}

		// Method 2 - Edges approach probability of 0
		// This method keeps the shape of the curve between +/- fOptionalMaxStandardDeviation by scaling
		// up that curve segment and filling in the 'extra' area mainly in the middle of the graph.
		// The edges apporach a probabilty of 0 even if fOptionalMaxStandardDeviation is something small like
		// 0.5. For current applications this method is not desired, but there might be some future
		// use for it (and the math was tricky to figure out) so the code is left in as an example.
		//
		//-- Optionally limit the range of the returned values
		//local fMaxStdDevFactor = 1
		//if fOptionalMaxStandardDeviation then
		//	fMaxStdDevFactor = 1 - math.exp(fOptionalMaxStandardDeviation * fOptionalMaxStandardDeviation / -2)
		//end
		//-- Calculate a normal with mean 0 and std dev 1
		//local p = math.sqrt(-2 * math.log(1 - fMaxStdDevFactor * math.random()))
		//local rand = p * math.cos(2 * math.pi * math.random())

		// Return the value with the requested mean and std dev.
		return asnum(fMean + rand * fStandardDeviation);
	}

	public static Table ShallowCopyTable(Table tTable)
	{
		return InternalShallowCopyTable(tTable);
	}

	public static Table<K, V> ShallowCopyTable<K, V>(Table<K, V> tTable)
		where V : class
	{
		return dyncast<Table<K, V>>(InternalShallowCopyTable(tTable));
	}

	public static TableV<K, V> ShallowCopyTable<K, V>(TableV<K, V> tTable)
		where V : struct
	{
		return dyncast<TableV<K, V>>(InternalShallowCopyTable(tTable));
	}

	private static Table InternalShallowCopyTable(Table tTable)
	{
		if (tTable == null)
		{
			return null;
		}

		var tType = type(tTable);
		if (tType != "table")
		{
			error(concat("Can't shallow copy non-table (", tType, ")"));
			return null;
		}

		var tCopy = new Table { };
		foreach ((var key, var value) in pairs(tTable))
		{
			tCopy[key] = value;
		}

		return tCopy;
	}

	// Creates a new table with the elements of the original,
	// and adds any elements from the parent not in the original
	public static SlimCS.Table ShallowCombineTable(Table tOriginal, Table tParent)
	{
		var tOriginalType = type(tOriginal);
		if (tOriginalType != "table")
		{
			error(concat("Can't shallow combine non-table original (", tOriginalType, ")"));
			return null;
		}

		var tRet = CoreUtilities.ShallowCopyTable(tOriginal);

		if (null != tParent)
		{
			var tParentType = type(tParent);
			if (tParentType != "table")
			{
				error(concat("Can't shallow combine non-table parent (", tParentType, ")"));
				return null;
			}

			// Add any values we didn't override from the parent
			foreach ((var key, var value) in pairs(tParent))
			{
				if (null == tRet[key])
				{
					tRet[key] = value;
				}
			}
		}

		return tRet;
	}

	/// <summary>
	/// Variation of ShallowCombineTable that merges tParent directly into tOriginal.
	/// </summary>
	/// <param name="tOriginal">The target table to merge into.</param>
	/// <param name="tParent">The source table. Values are merged if not already defined in tOriginal.</param>
	public static void ShallowMergeTable(Table tOriginal, Table tParent)
	{
		foreach (var (key, value) in pairs(tParent))
		{
			if (null == tOriginal[key])
			{
				tOriginal[key] = value;
			}
		}
	}

	/// <summary>
	/// Pass to VisitTables to traverse. Return false to stop traversal.
	/// </summary>
	public delegate bool VisitFunc(Table table, Table<double, string> aPath);

	/// <summary>
	/// Utility function, visits all table objects globally accessible
	/// (via global objects and closures).
	/// </summary>
	public static void VisitTables(VisitFunc func)
	{
		var tVisitedTable = new TableV<object, bool>();
		var aPath = new Table<double, string>();

		InternalVisitHelper(func, _G, tVisitedTable, aPath);
		InternalVisitHelper(func, debug.getregistry(), tVisitedTable, aPath);
	}

	static void InternalVisitHelper(
		VisitFunc func,
		object objToSearch,
		TableV<object, bool> tVisitedTable,
		Table<double, string> aPath)
	{
		if (null == objToSearch)
		{
			return;
		}

		var sType = type(objToSearch);

		// Do the visit here if it is a table
		if ("table" == sType && !func(dyncast<Table>(objToSearch), aPath))
		{
			return;
		}

		if (tVisitedTable[objToSearch] ?? false)
		{
			return;
		}
		tVisitedTable[objToSearch] = true;

		if ("table" == sType)
		{
			var tableToSearch = dyncast<Table>(objToSearch);
			foreach (var (key, val) in pairs(tableToSearch))
			{
				table.insert(aPath, "<key>");
				InternalVisitHelper(func, dyncast<object>(key), tVisitedTable, aPath);
				table.remove(aPath);

				table.insert(aPath, key.ToString());
				InternalVisitHelper(func, dyncast<object>(val), tVisitedTable, aPath);
				table.remove(aPath);
			}

			table.insert(aPath, "<metatable>");
			InternalVisitHelper(func, getmetatable(objToSearch), tVisitedTable, aPath);
			table.remove(aPath);
		}
		else if ("function" == sType)
		{
			table.insert(aPath, "<fenv>");
			InternalVisitHelper(func, debug.getuservalue(objToSearch), tVisitedTable, aPath);
			table.remove(aPath);

			// 60 is maximum up-value limit for luajit.
			for (int i = 1; i <= 60; ++i)
			{
				var (n, v) = debug.getupvalue(objToSearch, i);
				if (null == n)
				{
					break;
				}

				table.insert(aPath, "<upvalue>:" + n);
				InternalVisitHelper(func, v, tVisitedTable, aPath);
				table.remove(aPath);
			}
		}
		else if ("userdata" == sType)
		{
			table.insert(aPath, "<metatable>");
			InternalVisitHelper(func, getmetatable(objToSearch), tVisitedTable, aPath);
			table.remove(aPath);

			table.insert(aPath, "<user-value>");
			InternalVisitHelper(func, debug.getuservalue(objToSearch), tVisitedTable, aPath);
			table.remove(aPath);
		}
	}

	public static (TableV<string, EnumType>, Table<EnumType, string>) GenerateTwoWayLookupTables<EnumType>()
		where EnumType : struct
	{
		var stringToEnumTable = (TableV<string, EnumType>)astable(typeof(EnumType));

		Table<EnumType, string> enumToStringTable = new Table<EnumType, string>();
		foreach ((string s, EnumType? e) in pairs(stringToEnumTable))
		{
			enumToStringTable[(EnumType)e] = s;
		}

		return (stringToEnumTable, enumToStringTable);
	}

	public static void Shuffle(Table array)
	{
		var count = lenop(array);

		foreach (var i in range(1, count - 1))
		{
			var j = math.random(i, count);
			var temp = array[i];
			array[i] = array[j];
			array[j] = temp;
		}
	}


	// takes a numerically indexed table and creates the "inverted" lookup table.
	// Note, this assumes you are okay with duplicate entries being removed.
	public static TableV<T, double> CreateLookUp<T>(Table table)
	{
		TableV<T, double> newTable = new TableV<T, double>();

		foreach ((var key, var entry) in pairs(table))
		{
			newTable[entry] = key;
		}

		return newTable;
	}

	internal static Table AppendToTableAsArray(Table originalTable, Table newElements)
	{
		Table output = new Table();
		if (!originalTable.IsNullOrEmpty())
		{
			foreach ((var _, var entry) in ipairs(originalTable))
			{
				table.insert(output, entry);
			}
		}
		if (!newElements.IsNullOrEmpty())
		{
			foreach ((var _, var entry) in ipairs(newElements))
			{
				table.insert(output, entry);
			}
		}
		return output;
	}

	class RestoreEntry
	{
		public Table m_Target;
		public Table m_ToRestore;
	}

	static Table<double, RestoreEntry> s_aToRestore = new Table<double, RestoreEntry>();

	/// <summary>
	/// Utility for storing methods that are unsafe outside of the main thread context.
	/// </summary>
	/// <param name="t">Table of methods to store.</param>
	/// <param name="asMethods">List of method names to store.</param>
	/// <remarks>
	/// Part of our main thread protection. Methods are removed from t and replaced
	/// with a function that raises an exception on access. Automatically restores
	/// functions once main thread has been entered.
	/// </remarks>
	public static void MainThreadOnly(Table t, params string[] asMethods)
	{
		var tRestore = new Table();
		foreach (var name in asMethods)
		{
			tRestore[name] = t[name];
			t[name] = (Vfunc0)(() =>
			{
				error($"{name} can only be called from the main thread.");
			});
		}

		// Add an eytr.
		table.insert(
			s_aToRestore,
			new RestoreEntry()
			{
				m_Target = t,
				m_ToRestore = tRestore,
			});
	}

	public static void OnGlobalMainThreadInit()
	{
		foreach ((var _, var entry) in ipairs(s_aToRestore))
		{
			var target = entry.m_Target;
			foreach ((var sName, var func) in pairs(entry.m_ToRestore))
			{
				target[sName] = func;
			}
		}

		// Clear.
		s_aToRestore = new Table<double, RestoreEntry>();
	}

	static CoreUtilities()
	{
		Globals.RegisterGlobalMainThreadInit(OnGlobalMainThreadInit);
	}

	public static bool LessThan(this string a, string b)
	{
		//  This is kinda bad. Just a workaround so get c# to stop complaining so we can let LUA handle string compare.
		return dyncast<int>(a) < dyncast<int>(b);
	}

	public static TableV<double, double> FillNumArray(double min, double max)
	{
		TableV<double, double> output = new TableV<double, double>();
		double index = 1;
		for (double i = min; i <= max; i++)
		{
			table.insert(output, index, i);
			index++;
		}
		return output;
	}

	public delegate string TransformStringArrayFunction(string elem);

	public static Table<double, string> ApplyTransformToStringArray(Table<double, string> array, TransformStringArrayFunction transform)
	{
		Table<double, string> output = new Table<double, string>();

		foreach ((var _, var entry) in ipairs(array))
		{
			table.insert(output, transform(entry));
		}

		return output;
	}

	public static Table<double, string> NumArrayToString(TableV<double, double> array, bool locFormatNumber = true)
	{
		Table<double, string> output = new Table<double, string>();
		double count = CoreUtilities.CountTableElements(array);
		for (double i = 1; i <= count; i++)
		{
			if (locFormatNumber)
			{
				table.insert(output, LocManager.FormatNumber(asnum(array[i])));
			}
			else
			{
				table.insert(output, array[i]);
			}
		}
		return output;
	}

	/// <summary>
	/// Returns a PseudoRandom instance seeded from a secure random source
	/// </summary>
	public static Native.Script_PseudoRandom NewRandom()
	{
		return CoreUtilities.NewNativeUserData<Native.Script_PseudoRandom>();
	}

	/// <summary>
	/// Returns a PseudoRandom instance seeded deterministically, from a hash of the given string
	/// </summary>
	public static Native.Script_PseudoRandom NewRandomFromSeedString(string stringToHash)
	{
		return CoreUtilities.NewNativeUserData<Native.Script_PseudoRandom>(stringToHash);
	}


#if DEBUG
	public static void OpenAssociatedJsonFile(Native.FilePath filePath)
	{
		string sFilePath = @string.gsub(filePath.GetAbsoluteFilename(), "\\", "/");
		Engine.OpenURL("file:///" + sFilePath);
	}
#endif //DEBUG
}
