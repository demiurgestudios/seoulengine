// Keeper of dynamic game state data, global information store that persists
// (within the lifespan of a single script VM - it is not saved to disk
// nor does it persist across a game restart).
//
// "Dynamic" here indicates that keys can be set to any value without
// type checking (e.g. a key could be set to a string or int at different
// times). Generally, exploiting this is not recommended as it can be generally
// confusing and create bugs.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public static class DynamicGameStateData
{
	static readonly Native.Game_ScriptManagerProxy udScriptManagerProxy = null;
	static readonly Table tRootMovieClipRegistry = CoreUtilities.MakeWeakTable();
	static Table tGameStateData = new Table { };

	static Table s_DataTable = new Table { };

	// For each value in tDataTable, contains a list of {path to data table element, metatable class name}
	// pairs to allow us to store objects with a metatable through the ReceiveState/RestoreState pipeline
	static Table s_DataMetatableTable = new Table { };
	static Table tPreviousValueTable = new Table { };

	delegate void RestoreDynamicGameStateDataTableDelegate();

	delegate void SetValueDelegate(string sKey, object v, string sName = null);

	static bool IsStorableValue(object value, string key, Table<double, string> sKeyPath)
	{
		if (type(value) == "function")
		{
			// We can't store a function
			table.insert(sKeyPath, key);
			string sPath = "";
			foreach ((var _, string sPathKey) in ipairs(sKeyPath))
			{
				sPath = sPath + sPathKey + "/";
			}
			CoreNative.Warn($"IsStorableValue can't store function, path={sPath}");
			CoreNative.Warn($"-----------value---------------------------------------");
			CoreUtilities.LogTableOrTostring(LoggerChannel.Warning, value);
			CoreNative.Warn($"-------------------------------------------------------");
			table.remove(sKeyPath, lenop(sKeyPath));
			return false;
		}
		else if (!(type(value) == "table"))
		{
			// If its not a function and not a table, its fine to store
			return true;
		}

		// At this point, we know its a table

		table.insert(sKeyPath, key);
		try
		{
			// If it has a non-class metatable, we can't store it
			var metatable = getmetatable(value);
			if (null != metatable && string.IsNullOrEmpty(metatable["m_sClassName"]))
			{
				string sPath = "";
				foreach ((var _, string sPathKey) in ipairs(sKeyPath))
				{
					sPath = sPath + sPathKey + "/";
				}
				CoreNative.Warn($"IsStorableValue can't store a table value with a non-class metatable, path={sPath}");
				CoreNative.Warn($"-----------value---------------------------------------");
				CoreUtilities.LogTableOrTostring(LoggerChannel.Warning, value);
				CoreNative.Warn($"-----------metatable-----------------------------------");
				CoreUtilities.LogTable(LoggerChannel.Warning, metatable);
				CoreNative.Warn($"-------------------------------------------------------");
				return false;
			}

			// Check if any values in the table aren't storable
			foreach ((var sKey, var v) in pairs(value))
			{
				if (!IsStorableValue(sKey, sKey, sKeyPath) || !IsStorableValue(v, sKey, sKeyPath))
				{
					return false;
				}
			}
		}
		finally
		{
			table.remove(sKeyPath, lenop(sKeyPath));
		}
		return true;
	}

	static void InternalSetValue(string sKey, object value)
	{
		s_DataTable[sKey] = value;

		if (null == value)
		{
			s_DataMetatableTable[sKey] = null;
		}
		else
		{
			var metadataTable = new Table();
			var prefix = new Table<double, string>();
			AccumulateMetatables(value, prefix, metadataTable);
			s_DataMetatableTable[sKey] = metadataTable;
		}
	}

	private static void AccumulateMetatables(object value, Table<double, string> prefix, Table results)
	{
		if (type(value) != "table")
		{
			return;
		}

		var metatable = getmetatable(value);
		if (metatable != null)
		{
			var sClassName = metatable["m_sClassName"];

			if (!string.IsNullOrEmpty(sClassName))
			{
				var p = CoreUtilities.ShallowCopyTable(prefix);
				table.insert(
					results,
					new Table
					{
						["Prefix"] = p,
						["ClassName"] = sClassName,
					});
			}
		}

		foreach ((var key, var v) in pairs(value))
		{
			table.insert(prefix, key);
			AccumulateMetatables(v, prefix, results);
			table.remove(prefix);
		}
	}

	private static void ReattachMetatables()
	{
		foreach ((string sKey, object value) in pairs(s_DataTable))
		{
			var metatables = (Table)s_DataMetatableTable[sKey];

			foreach ((var _, var metatableMapping) in ipairs(metatables))
			{
				var aPrefix = metatableMapping["Prefix"];
				var sClassName = metatableMapping["ClassName"];

				var v = (Table)value;
				foreach ((var _, var key) in ipairs(aPrefix))
				{
					v = (Table)v[key];
				}

				setmetatable(v, _G[sClassName]);
			}
		}
	}

	static DynamicGameStateData()
	{
		udScriptManagerProxy = CoreUtilities.NewNativeUserData<Native.Game_ScriptManagerProxy>();
		if (null == udScriptManagerProxy)
		{
			error("Failed instantiating Game_ScriptManagerProxy.");
		}

		SetValueDelegate SetValueWithCheck = (sKey, v, sName) =>
		{
			if (IsStorableValue(v, sKey, new Table<double, string>()))
			{
				if (v == null && DynamicGameStateData.GetValue<object>(sKey) != null)
				{
					var oldVal = DynamicGameStateData.GetValue<object>(sKey);

					if (oldVal == null)
					{
						tPreviousValueTable[sKey] = null;
					}
					else
					{
						var val = new Table { };
						val["value"] = oldVal;
						val["screen"] = sName;
						tPreviousValueTable[sKey] = val;
					}
				}

				InternalSetValue(sKey, v);
			}
			else
			{
				CoreNative.Warn(concat("Trying to store Unstorable value in DynamicGameStateData, sKey: ", sKey ?? "<none>", " sName: ", sName ?? "<none>"));
			}
		};

		SetValueDelegate SetValueNoCheck = (sKey, v, _0) =>
		{
			InternalSetValue(sKey, v);
		};

		// The check is expensive and only used to make hotloading safe
		// we don't need it in ship.
#if !DEBUG
		rawset(astable(typeof(DynamicGameStateData)), "SetValue", SetValueNoCheck);
#else
		rawset(astable(typeof(DynamicGameStateData)), "SetValue", SetValueWithCheck);
#endif

		_G["RestoreDynamicGameStateData"] = (RestoreDynamicGameStateDataTableDelegate)(() =>
		{
			Table tRestoreData = null;
			Table tRestoreMetatableData = null;
			try
			{
				(tRestoreData, tRestoreMetatableData) = udScriptManagerProxy.RestoreState();
				if (null != tRestoreData && null != tRestoreMetatableData)
				{
					s_DataTable = tRestoreData;
					s_DataMetatableTable = tRestoreMetatableData;

					ReattachMetatables();
				}
				else
				{
					CoreNative.Warn("Error restoring DynamicGameStateData, no data returned");
				}
			}
			catch
			{
				CoreNative.Warn(concat("Error restoring DynamicGameStateData: ", tRestoreData));
			}
		});

		Globals.RegisterOnHotLoad(OnHotLoad);
	}

	public static T GetValue<T>(string sKey)
	{
		return dyncast<T>(s_DataTable[sKey]);
	}

	public static void SetValueWithCheck(string sKey, object v, string sName)
	{
		(var _, var _, var _) = (sKey, v, sName);
		error("Failed to replace placeholder");
	}

	public static void UnsetValue(string sKey, string sOptionalName = null)
	{
		DynamicGameStateData.SetValue(sKey, null, sOptionalName);
	}

	static void OnHotLoad()
	{
		var movieTypeSet = new Table { };

		foreach ((var _, var v) in pairs(tRootMovieClipRegistry))
		{
			movieTypeSet[v] = true;
		}

		foreach ((var sKey, var v) in pairs(tPreviousValueTable))
		{
			if (movieTypeSet[v["screen"]] != null)
			{
				InternalSetValue(sKey, v["value"]);
			}
		}

		udScriptManagerProxy.ReceiveState(s_DataTable, s_DataMetatableTable);
	}

	public static void RegisterRootMovieClip(RootMovieClip rootMovieClip, string sMovieTypeName)
	{
		tRootMovieClipRegistry[rootMovieClip] = sMovieTypeName;
	}

	public static void UnregisterRootMovieClip(RootMovieClip rootMovieClip)
	{
		tRootMovieClipRegistry[rootMovieClip] = null;
	}

	public static void SetValue(string sKey, object v, string sName = null)
	{
		error("Placeholder was not replaced.");
	}
}
