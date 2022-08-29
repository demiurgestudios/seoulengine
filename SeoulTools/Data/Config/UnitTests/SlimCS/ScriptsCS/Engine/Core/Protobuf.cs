// Utilities for working with Google Protocol Buffers
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

/// <summary>
/// A protobuf type schema, loaded by the lua pb library
/// </summary>
public sealed class PbMessageType
{
	private object m_PbType;

	public PbMessageType(string name)
	{
		m_PbType = Protobuf.PbType(name);
	}

	public Table Decode(object message)
	{
		return Protobuf.PbDecode(message, m_PbType);
	}

	public object Encode(Table data)
	{
		return Protobuf.PbEncode(data, m_PbType);
	}
}

public static class Protobuf
{
	#region Methods on External/lua_protobuf/pb
	delegate int LuaReadPbDelegate(string sFilePath);

	delegate object LuaTypeDelegate(string name);

	delegate Table LuaDecodeDelegate(object message, object loadedType);

	delegate object LuaEncodeDelegate(Table data, object loadedType);
	#endregion

	delegate object LuaLibLoadDelegate(int dataHandle);

	static Table s_ExtPbLibrary;

	static Protobuf()
	{
		s_ExtPbLibrary = dyncast<Table>(require("External/lua_protobuf/pb"));
	}

	private static object PbLoad(int dataHandle)
	{
		return dyncast<LuaLibLoadDelegate>(s_ExtPbLibrary["load"])(dataHandle);
	}

	public static object PbType(string name)
	{
		return dyncast<LuaTypeDelegate>(s_ExtPbLibrary["type"])(name);
	}

	internal static object PbEncode(Table data, object loadedType)
	{
		return dyncast<LuaEncodeDelegate>(s_ExtPbLibrary["encode"])(data, loadedType);
	}

	internal static Table PbDecode(object message, object loadedType)
	{
		return dyncast<LuaDecodeDelegate>(s_ExtPbLibrary["decode"])(message, loadedType);
	}

	public static object LoadSchema(string path)
	{
		var dataHandle = dyncast<LuaReadPbDelegate>(_G["SeoulLuaReadPb"])(path);
		return PbLoad(dataHandle);
	}
}
