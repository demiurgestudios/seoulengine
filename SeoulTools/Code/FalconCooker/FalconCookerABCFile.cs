// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime;
using System.Runtime.InteropServices;
using System.Text;

namespace FalconCooker
{
	public enum ABCOpCode
	{
		OP_add = 160,
		OP_add_i = 197,
		OP_astype = 134,
		OP_astypelate = 135,
		OP_bitand = 168,
		OP_bitnot = 151,
		OP_bitor = 169,
		OP_bitxor = 170,
		OP_call = 65,
		OP_callmethod = 67,
		OP_callproperty = 70,
		OP_callproplex = 76,
		OP_callpropvoid = 79,
		OP_callstatic = 68,
		OP_callsuper = 69,
		OP_callsupervoid = 78,
		OP_checkfilter = 120,
		OP_coerce = 128,
		OP_coerce_a = 130,
		OP_coerce_s = 133,
		OP_construct = 66,
		OP_constructprop = 74,
		OP_constructsuper = 73,
		OP_convert_b = 118,
		OP_convert_i = 115,
		OP_convert_d = 117,
		OP_convert_o = 119,
		OP_convert_u = 116,
		OP_convert_s = 112,
		OP_debug = 239,
		OP_debugfile = 241,
		OP_debugline = 240,
		OP_declocal = 148,
		OP_declocal_i = 195,
		OP_decrement = 147,
		OP_decrement_i = 193,
		OP_deleteproperty = 106,
		OP_divide = 163,
		OP_dup = 42,
		OP_dxns = 6,
		OP_dxnslate = 7,
		OP_equals = 171,
		OP_esc_xattr = 114,
		OP_esc_xelem = 113,
		OP_find_property = 94,
		OP_findpropstrict = 93,
		OP_getdescendants = 89,
		OP_getglobalscope = 100,
		OP_getglobalslot = 110,
		OP_getlex = 96,
		OP_getlocal = 98,
		OP_getlocal_0 = 208,
		OP_getlocal_1 = 209,
		OP_getlocal_2 = 210,
		OP_getlocal_3 = 211,
		OP_getproperty = 102,
		OP_getscopeobject = 101,
		OP_getslot = 108,
		OP_getsuper = 4,
		OP_greaterequals = 176, // Typo in the AVM2 spec, greaterequals has description of greaterthan
		OP_greaterthan = 175,
		OP_hasnext = 31,
		OP_hasnext2 = 50,
		OP_ifeq = 19,
		OP_iffalse = 18,
		OP_ifge = 24,
		OP_ifgt = 23,
		OP_ifle = 22,
		OP_iflt = 21,
		OP_ifnge = 15,
		OP_ifngt = 14,
		OP_ifnle = 13,
		OP_ifnlt = 12,
		OP_ifne = 20,
		OP_ifstricteq = 25,
		OP_ifstrictne = 26,
		OP_iftrue = 17,
		OP_in = 180,
		OP_inclocal = 146,
		OP_inclocal_i = 194,
		OP_increment = 145,
		OP_increment_i = 192,
		OP_initproperty = 104,
		OP_instanceof = 177,
		OP_istype = 178,
		OP_istypelate = 179,
		OP_jump = 16,
		OP_kill = 8,
		OP_label = 9,
		OP_lessequals = 174,
		OP_lessthan = 173,
		OP_lookupswitch = 27,
		OP_lshift = 165,
		OP_modulo = 164,
		OP_multiply = 162,
		OP_multiply_i = 199,
		OP_negate = 144,
		OP_negate_i = 196,
		OP_newactivation = 87,
		OP_newarray = 86,
		OP_newcatch = 90,
		OP_newclass = 88,
		OP_newfunction = 64,
		OP_newobject = 85,
		OP_nextname = 30,
		OP_nextvalue = 35,
		OP_nop = 2,
		OP_not = 150,
		OP_pop = 41,
		OP_popscope = 29,
		OP_pushbyte = 36,
		OP_pushdouble = 47, // Typo in the AVM2 spec, pushdouble is documented as 46, but is actually 47.
		OP_pushfalse = 39,
		OP_pushint = 45,
		OP_pushnamespace = 49,
		OP_pushnan = 40,
		OP_pushnull = 32,
		OP_pushscope = 48,
		OP_pushshort = 37,
		OP_pushstring = 44,
		OP_pushtrue = 38,
		OP_pushuint = 46,
		OP_pushundefined = 33,
		OP_pushwith = 28,
		OP_returnvalue = 72,
		OP_returnvoid = 71,
		OP_rshift = 166,
		OP_setlocal = 99,
		OP_setlocal_0 = 212,
		OP_setlocal_1 = 213,
		OP_setlocal_2 = 214,
		OP_setlocal_3 = 215,
		OP_setglobalslot = 111,
		OP_setproperty = 97,
		OP_setslot = 109,
		OP_setsuper = 5,
		OP_strictequals = 172,
		OP_subtract = 161,
		OP_subtract_i = 198,
		OP_swap = 43,
		OP_throw = 3,
		OP_typeof = 149,
		OP_urshift = 167
	}

	public struct ClassInfo
	{
		public int m_iStaticInitializer;
		public TraitsInfo[] m_aTraits;
	}

	[Flags]
	public enum InstanceInfoFlags
	{
		ClassSealed = 0x01,
		ClassFinal = 0x02,
		ClassInterface = 0x04,
		ClassProtectedNs = 0x08,
	}

	[Flags]
	public enum TraitsInfoAttributes
	{
		Final = 0x1,
		Override = 0x2,
		Metadata = 0x4,
	}

	public enum TraitsInfoKind
	{
		Slot = 0,
		Method = 1,
		Getter = 2,
		Setter = 3,
		Class = 4,
		Function = 5,
		Const = 6,
	}

	public abstract class TraitsInfoData
	{
	}

	public sealed class TraitsInfoData_Slot : TraitsInfoData
	{
		public int m_iSlotId;
		public int m_iTypeName;
		public int m_iVIndex;
		public ConstantKind m_eKind;
	}

	public sealed class TraitsInfoData_Class : TraitsInfoData
	{
		public int m_iSlotId;
		public int m_iClassIndex;
	}

	public sealed class TraitsInfoData_Function : TraitsInfoData
	{
		public int m_iSlotId;
		public int m_iFunctionIndex;
	}

	public sealed class TraitsInfoData_Method : TraitsInfoData
	{
		public int m_iDispatchId;
		public int m_iMethodIndex;
	}

	public struct TraitsInfo
	{
		public int m_iName;
		public TraitsInfoKind m_eKind;
		public TraitsInfoAttributes m_eAttributes;
		public TraitsInfoData m_Data;
		public int[] m_aMetadata;
	}

	public struct InstanceInfo
	{
		public int m_iName;
		public int m_iSuperName;
		public InstanceInfoFlags m_eFlags;
		public int m_iProtectedNamespace;
		public int[] m_aiInterfaces;
		public int m_iInstanceInitializer;
		public TraitsInfo[] m_aTraits;
	}

	public struct MetadataItemInfo
	{
		public int m_iKey;
		public int m_iValue;
	}

	public struct MetadataInfo
	{
		public int m_iNameIndex;
		public MetadataItemInfo[] m_aItems;
	}

	[Flags]
	public enum MethodInfoFlags
	{
		NeedArguments = 0x01,
		NeedActivation = 0x02,
		NeedReset = 0x04,
		HasOptional = 0x08,
		SetDxns = 0x40,
		HasParamNames = 0x80,
	}

	public enum ConstantKind
	{
		Invalid = 0,
		Int = 0x03,
		UInt = 0x04,
		Double = 0x06,
		Utf8 = 0x01,
		True = 0x0B,
		False = 0x0A,
		Null = 0x0C,
		Undefined = 0x00,
		Namespace = 0x08,
		PackageNamespace = 0x16,
		PackageInternalNs = 0x17,
		ProtectedNamespace = 0x18,
		ExplicitNamespace = 0x19,
		StaticProtectedNs = 0x1A,
		PrivateNs = 0x05,
	}

	public struct MethodInfoOptionDetail
	{
		public ConstantKind m_eKind;
		public int m_iValue;
	}

	public struct MethodInfo
	{
		public int m_iReturnTypeIndex;
		public int[] m_aParamTypes;
		public int m_iNameIndex;
		public MethodInfoFlags m_eFlags;
		public MethodInfoOptionDetail[] m_aOptions;
		public int[] m_aiParameterNames;
		public MethodBodyInfo m_Body;
	}

	public enum NamespaceKind
	{
		Namespace = 0x08,
		PackageNamespace = 0x16,
		PackageInternalNs = 0x17,
		ProtectedNamespace = 0x18,
		ExplicitNamespace = 0x19,
		StaticProtectedNs = 0x1A,
		PrivateNs = 0x05,
	}

	public struct NamespaceInfo
	{
		public NamespaceKind m_eKind;
		public int m_iNameIndex;
	}

	public struct NamespaceSetInfo
	{
		public int[] m_aNamespaces;
	}

	public struct ExceptionInfo
	{
		public int m_iFrom;
		public int m_iTo;
		public int m_iTarget;
		public int m_iExceptionType;
		public int m_iVariableName;
	}

	public struct MethodBodyInfo
	{
		public int m_iMethod;
		public int m_iMaxStack;
		public int m_iLocalCount;
		public int m_iInitScopeDepth;
		public int m_iMaxScopeDepth;
		public byte[] m_aCode;
		public ExceptionInfo[] m_aExceptions;
		public TraitsInfo[] m_aTraits;
	}

	public enum MultinameKind
	{
		QName = 0x07,
		QNameA = 0x0D,
		RTQName = 0x0F,
		RTQNameA = 0x10,
		RTQNameL = 0x11,
		RTQNameLA = 0x12,
		Multiname = 0x09,
		MultinameA = 0x0E,
		MultinameL = 0x1B,
		MultinameLA = 0x1C,

		GenericsName = 0x1D, // undocumented multiname type, seems to be for generics: https://forums.adobe.com/message/1034721?tstart=12
	}

	public struct MultinameInfo
	{
		public MultinameKind m_eKind;
		public int m_iDataIndex;
		public int m_iNameIndex;
	}

	public struct ScriptInfo
	{
		public int m_iScriptInitializer;
		public TraitsInfo[] m_aTraits;
	}

	public struct ABCFileConstantPool
	{
		public Int32[] m_aIntegers;
		public UInt32[] m_aUnsignedIntegers;
		public double[] m_aDoubles;
		public string[] m_aStrings;
		public NamespaceInfo[] m_aNamespaces;
		public NamespaceSetInfo[] m_aNamespaceSetInfos;
		public MultinameInfo[] m_aMultinameInfos;

		public string GetMultiname(AVM2 avm2, int i, Stack<AVM2Value> stack)
		{
			if (i >= 0 && i < m_aMultinameInfos.Length)
			{
				MultinameInfo info = m_aMultinameInfos[i];
				switch (info.m_eKind)
				{
					case MultinameKind.Multiname:
					case MultinameKind.QName:
						return m_aStrings[info.m_iNameIndex];

					case MultinameKind.RTQName:
						{
							// TODO:
							stack.Pop(); // Ignore the namespace value.
							return m_aStrings[info.m_iNameIndex];
						}

					case MultinameKind.MultinameA:
					case MultinameKind.QNameA:
						return m_aStrings[info.m_iNameIndex];

					case MultinameKind.RTQNameA:
						{
							// TODO:
							stack.Pop(); // Ignore the namespace value.
							return m_aStrings[info.m_iNameIndex];
						}

					case MultinameKind.MultinameL:
						{
							string sReturn = string.Empty;
							{
								AVM2Value name = stack.Pop();

								// TODO: There are cases that I still haven't resolved where
								// a complex object is on the stack in the multiname slot. This case
								// should actually allow the type error to bubble up.
								try
								{
									sReturn = name.AsString(avm2);
								}
								catch (AVM2TypeError)
								{
									sReturn = "undefined";
								}
							}

							return sReturn;
						}

					case MultinameKind.MultinameLA:
						{
							string sReturn = string.Empty;
							{
								AVM2Value name = stack.Pop();

								// TODO: There are cases that I still haven't resolved where
								// a complex object is on the stack in the multiname slot. This case
								// should actually allow the type error to bubble up.
								try
								{
									sReturn = name.AsString(avm2);
								}
								catch (AVM2TypeError)
								{
									sReturn = "undefined";
								}
							}

							return sReturn;
						}

					case MultinameKind.RTQNameL:
						{
							string sReturn = string.Empty;
							{
								AVM2Value name = stack.Pop();

								// TODO: There are cases that I still haven't resolved where
								// a complex object is on the stack in the multiname slot. This case
								// should actually allow the type error to bubble up.
								try
								{
									sReturn = name.AsString(avm2);
								}
								catch (AVM2TypeError)
								{
									sReturn = "undefined";
								}
							}

							// TODO:
							stack.Pop(); // Ignore the namespace value.
							return sReturn;
						}

					case MultinameKind.RTQNameLA:
						{
							string sReturn = string.Empty;
							{
								AVM2Value name = stack.Pop();

								// TODO: There are cases that I still haven't resolved where
								// a complex object is on the stack in the multiname slot. This case
								// should actually allow the type error to bubble up.
								try
								{
									sReturn = name.AsString(avm2);
								}
								catch (AVM2TypeError)
								{
									sReturn = "undefined";
								}
							}

							// TODO:
							stack.Pop(); // Ignore the namespace value.
							return sReturn;
						}

					default:
						throw new Exception("Unsupported multiname type '" + Enum.GetName(typeof(MultinameKind), info.m_eKind) + "'.");
				}
			}

			return string.Empty;
		}
	}

	/// <summary>
	/// See also: http://www.adobe.com/content/dam/Adobe/en/devnet/actionscript/articles/avm2overview.pd
	/// </summary>
	public sealed class ABCFile
	{
		#region Private members
		byte[] m_aBytecode;
		int m_iMinorVersion;
		int m_iMajorVersion;
		ABCFileConstantPool m_ConstantPool = default(ABCFileConstantPool);
		MethodInfo[] m_aMethods;
		MetadataInfo[] m_aMetadata;
		InstanceInfo[] m_aInstances;
		ClassInfo[] m_aClasses;
		ScriptInfo[] m_aScripts;
		MethodBodyInfo[] m_aMethodBodies;

		int m_iOffsetInBits;

		void Align()
		{
			m_iOffsetInBits = (0 == (m_iOffsetInBits & 0x7)) ? m_iOffsetInBits : ((m_iOffsetInBits & (~(0x7))) + 8);
		}

		static T Min<T>(T a, T b)
			where T : IComparable<T>
		{
			return (a.CompareTo(b) < 0) ? a : b;
		}

		int OffsetInBits
		{
			get
			{
				return m_iOffsetInBits;
			}

			set
			{
				m_iOffsetInBits = value;
			}
		}

		int OffsetInBytes
		{
			get
			{
				return (m_iOffsetInBits >> 3);
			}

			set
			{
				m_iOffsetInBits = ((value << 3) & (~(0x7)));
			}
		}

		ABCFileConstantPool ReadConstantPool()
		{
			ABCFileConstantPool constantPool = default(ABCFileConstantPool);

			// One (unintuitive and bizarre) thing to note here is that,
			// entry 0 is never read from the input.

			int nIntegerCount = (int)ReadUInt32();
			if (nIntegerCount > 0)
			{
				constantPool.m_aIntegers = new int[nIntegerCount];
				constantPool.m_aIntegers[0] = 0;
				for (int i = 1; i < nIntegerCount; ++i)
				{
					constantPool.m_aIntegers[i] = ReadInt32();
				}
			}

			int nUnsignedIntegerCount = (int)ReadUInt32();
			if (nUnsignedIntegerCount > 0)
			{
				constantPool.m_aUnsignedIntegers = new UInt32[nUnsignedIntegerCount];
				constantPool.m_aUnsignedIntegers[0] = 0;
				for (int i = 1; i < nUnsignedIntegerCount; ++i)
				{
					constantPool.m_aUnsignedIntegers[i] = ReadUInt32();
				}
			}

			int nDoubleCount = (int)ReadUInt32();
			if (nDoubleCount > 0)
			{
				constantPool.m_aDoubles = new double[nDoubleCount];
				constantPool.m_aDoubles[0] = 0.0;
				for (int i = 1; i < nDoubleCount; ++i)
				{
					constantPool.m_aDoubles[i] = ReadDouble();
				}
			}

			int nStringCount = (int)ReadUInt32();
			if (nStringCount > 0)
			{
				constantPool.m_aStrings = new string[nStringCount];
				constantPool.m_aStrings[0] = string.Empty;
				for (int i = 1; i < nStringCount; ++i)
				{
					constantPool.m_aStrings[i] = ReadString();
				}
			}

			int nNamespaceCount = (int)ReadUInt32();
			if (nNamespaceCount > 0)
			{
				constantPool.m_aNamespaces = new NamespaceInfo[nNamespaceCount];
				constantPool.m_aNamespaces[0] = default(NamespaceInfo);
				for (int i = 1; i < nNamespaceCount; ++i)
				{
					constantPool.m_aNamespaces[i] = ReadNamespaceInfo();
				}
			}

			int nNamespaceSetCount = (int)ReadUInt32();
			if (nNamespaceSetCount > 0)
			{
				constantPool.m_aNamespaceSetInfos = new NamespaceSetInfo[nNamespaceSetCount];
				constantPool.m_aNamespaceSetInfos[0] = default(NamespaceSetInfo);
				for (int i = 1; i < nNamespaceSetCount; ++i)
				{
					constantPool.m_aNamespaceSetInfos[i] = ReadNamespaceSetInfo();
				}
			}

			int nMultinameCount = (int)ReadUInt32();
			if (nMultinameCount > 0)
			{
				constantPool.m_aMultinameInfos = new MultinameInfo[nMultinameCount];
				constantPool.m_aMultinameInfos[0] = default(MultinameInfo);
				for (int i = 1; i < nMultinameCount; ++i)
				{
					constantPool.m_aMultinameInfos[i] = ReadMultinameInfo();
				}
			}

			return constantPool;
		}

		ClassInfo ReadClassInfo()
		{
			ClassInfo ret = default(ClassInfo);
			ret.m_iStaticInitializer = (int)ReadUInt32();
			ret.m_aTraits = ReadTraits();
			return ret;
		}

		ScriptInfo ReadScriptInfo()
		{
			ScriptInfo ret = default(ScriptInfo);
			ret.m_iScriptInitializer = (int)ReadUInt32();
			ret.m_aTraits = ReadTraits();
			return ret;
		}

		InstanceInfo ReadInstanceInfo()
		{
			InstanceInfo ret = default(InstanceInfo);
			ret.m_iName = (int)ReadUInt32();
			ret.m_iSuperName = (int)ReadUInt32();
			ret.m_eFlags = (InstanceInfoFlags)ReadByte();

			if (InstanceInfoFlags.ClassProtectedNs == (InstanceInfoFlags.ClassProtectedNs & ret.m_eFlags))
			{
				ret.m_iProtectedNamespace = (int)ReadUInt32();
			}

			int nInterfaces = (int)ReadUInt32();
			if (nInterfaces > 0)
			{
				ret.m_aiInterfaces = new int[nInterfaces];
				for (int i = 0; i < nInterfaces; ++i)
				{
					ret.m_aiInterfaces[i] = (int)ReadUInt32();
				}
			}

			ret.m_iInstanceInitializer = (int)ReadUInt32();
			ret.m_aTraits = ReadTraits();
			return ret;
		}

		MetadataInfo ReadMetadataInfo()
		{
			MetadataInfo ret = default(MetadataInfo);
			ret.m_iNameIndex = (int)ReadUInt32();

			int nItems = (int)ReadUInt32();
			if (nItems > 0)
			{
				ret.m_aItems = new MetadataItemInfo[nItems];
				for (int i = 0; i < nItems; ++i)
				{
					ret.m_aItems[i].m_iKey = (int)ReadUInt32();
					ret.m_aItems[i].m_iValue = (int)ReadUInt32();
				}
			}

			return ret;
		}

		MetadataInfo[] ReadMetadata()
		{
			int nMetadata = (int)ReadUInt32();
			if (nMetadata > 0)
			{
				MetadataInfo[] aReturn = new MetadataInfo[nMetadata];
				for (int i = 0; i < nMetadata; ++i)
				{
					aReturn[i] = ReadMetadataInfo();
				}
				return aReturn;
			}

			return null;
		}

		ExceptionInfo ReadExceptionInfo()
		{
			ExceptionInfo ret = default(ExceptionInfo);
			ret.m_iFrom = (int)ReadUInt32();
			ret.m_iTo = (int)ReadUInt32();
			ret.m_iTarget = (int)ReadUInt32();
			ret.m_iExceptionType = (int)ReadUInt32();
			ret.m_iVariableName = (int)ReadUInt32();
			return ret;
		}

		MethodBodyInfo ReadMethodBodyInfo()
		{
			MethodBodyInfo ret = default(MethodBodyInfo);
			ret.m_iMethod = (int)ReadUInt32();
			ret.m_iMaxStack = (int)ReadUInt32();
			ret.m_iLocalCount = (int)ReadUInt32();
			ret.m_iInitScopeDepth = (int)ReadUInt32();
			ret.m_iMaxScopeDepth = (int)ReadUInt32();

			int nCodeBytes = (int)ReadUInt32();
			if (nCodeBytes > 0)
			{
				ret.m_aCode = new byte[nCodeBytes];
				Array.Copy(m_aBytecode, OffsetInBytes, ret.m_aCode, 0, nCodeBytes);
				OffsetInBytes += nCodeBytes;
			}

			int nExceptions = (int)ReadUInt32();
			if (nExceptions > 0)
			{
				ret.m_aExceptions = new ExceptionInfo[nExceptions];
				for (int i = 0; i < nExceptions; ++i)
				{
					ret.m_aExceptions[i] = ReadExceptionInfo();
				}
			}

			ret.m_aTraits = ReadTraits();

			// Associate the body with the method.
			m_aMethods[ret.m_iMethod].m_Body = ret;

			return ret;
		}

		MethodInfo ReadMethodInfo()
		{
			MethodInfo ret = default(MethodInfo);
			int nParameterCount = (int)ReadUInt32();
			ret.m_iReturnTypeIndex = (int)ReadUInt32();
			if (nParameterCount > 0)
			{
				ret.m_aParamTypes = new int[nParameterCount];
				for (int i = 0; i < nParameterCount; ++i)
				{
					ret.m_aParamTypes[i] = (int)ReadUInt32();
				}
			}
			ret.m_iNameIndex = (int)ReadUInt32();
			ret.m_eFlags = (MethodInfoFlags)ReadByte();

			if (MethodInfoFlags.HasOptional == (MethodInfoFlags.HasOptional & ret.m_eFlags))
			{
				int nOptions = (int)ReadUInt32();
				if (nOptions > 0)
				{
					ret.m_aOptions = new MethodInfoOptionDetail[nOptions];
					for (int i = 0; i < nOptions; ++i)
					{
						ret.m_aOptions[i].m_iValue = (int)ReadUInt32();
						ret.m_aOptions[i].m_eKind = (ConstantKind)ReadByte();
					}
				}
			}

			if (MethodInfoFlags.HasParamNames == (MethodInfoFlags.HasParamNames & ret.m_eFlags))
			{
				if (nParameterCount > 0)
				{
					ret.m_aiParameterNames = new int[nParameterCount];
					for (int i = 0; i < nParameterCount; ++i)
					{
						ret.m_aiParameterNames[i] = (int)ReadUInt32();
					}
				}
			}

			return ret;
		}

		MultinameInfo ReadMultinameInfo()
		{
			MultinameInfo ret = default(MultinameInfo);
			ret.m_eKind = (MultinameKind)ReadByte();

			switch (ret.m_eKind)
			{
				case MultinameKind.QName: // fall-through
				case MultinameKind.QNameA:
					ret.m_iDataIndex = (int)ReadUInt32();
					ret.m_iNameIndex = (int)ReadUInt32();
					break;

				case MultinameKind.RTQName: // fall-through
				case MultinameKind.RTQNameA:
					ret.m_iNameIndex = (int)ReadUInt32();
					break;

				case MultinameKind.RTQNameL: // fall-through
				case MultinameKind.RTQNameLA:
					// Nop data.
					break;

				case MultinameKind.Multiname: // fall-through
				case MultinameKind.MultinameA:
					ret.m_iNameIndex = (int)ReadUInt32();
					ret.m_iDataIndex = (int)ReadUInt32();
					break;

				case MultinameKind.MultinameL: // fall-through
				case MultinameKind.MultinameLA:
					ret.m_iDataIndex = (int)ReadUInt32();
					break;

				case MultinameKind.GenericsName:
					{
						ret.m_iNameIndex = (int)ReadUInt32();

						// Parameter count is currently expected to always be 0.
						// See: https://forums.adobe.com/message/1034721?tstart=12
						int iParameterCount = (int)ReadByte();
						for (int i = 0; i < iParameterCount; ++i)
						{
							ret.m_iDataIndex = (int)ReadUInt32();
						}
					}
					break;

				default:
					throw new Exception();
			}

			return ret;
		}

		NamespaceInfo ReadNamespaceInfo()
		{
			NamespaceInfo ret = default(NamespaceInfo);
			ret.m_eKind = (NamespaceKind)ReadByte();
			ret.m_iNameIndex = (int)ReadUInt32();
			return ret;
		}

		NamespaceSetInfo ReadNamespaceSetInfo()
		{
			NamespaceSetInfo ret = default(NamespaceSetInfo);

			int nCount = (int)ReadUInt32();
			if (nCount > 0)
			{
				ret.m_aNamespaces = new int[nCount];
				for (int i = 0; i < nCount; ++i)
				{
					ret.m_aNamespaces[i] = (int)ReadUInt32();
				}
			}

			return ret;
		}

		TraitsInfo[] ReadTraits()
		{
			int nTraits = (int)ReadUInt32();
			if (nTraits > 0)
			{
				TraitsInfo[] aReturn = new TraitsInfo[nTraits];
				for (int i = 0; i < nTraits; ++i)
				{
					aReturn[i] = ReadTraitsInfo();
				}
				return aReturn;
			}

			return null;
		}

		TraitsInfo ReadTraitsInfo()
		{
			TraitsInfo ret = default(TraitsInfo);
			ret.m_iName = (int)ReadUInt32();
			byte uKindAndAttributes = ReadByte();
			ret.m_eKind = (TraitsInfoKind)(uKindAndAttributes & 0xF);
			ret.m_eAttributes = (TraitsInfoAttributes)((uKindAndAttributes >> 4) & 0xF);
			ret.m_Data = ReadTraitsInfoData(ret.m_eKind);

			if (TraitsInfoAttributes.Metadata == (TraitsInfoAttributes.Metadata & ret.m_eAttributes))
			{
				int nMetadata = (int)ReadUInt32();
				if (nMetadata > 0)
				{
					ret.m_aMetadata = new int[nMetadata];
					for (int i = 0; i < nMetadata; ++i)
					{
						ret.m_aMetadata[i] = (int)ReadUInt32();
					}
				}
			}
			return ret;
		}

		TraitsInfoData ReadTraitsInfoData(TraitsInfoKind eKind)
		{
			switch (eKind)
			{
				case TraitsInfoKind.Slot: // fall-through
				case TraitsInfoKind.Const:
					{
						TraitsInfoData_Slot ret = new TraitsInfoData_Slot();
						ret.m_iSlotId = (int)ReadUInt32();
						ret.m_iTypeName = (int)ReadUInt32();
						ret.m_iVIndex = (int)ReadUInt32();

						if (ret.m_iVIndex != 0)
						{
							ret.m_eKind = (ConstantKind)ReadByte();
						}
						else
						{
							ret.m_eKind = ConstantKind.Invalid;
						}
						return ret;
					}

				case TraitsInfoKind.Class:
					{
						TraitsInfoData_Class ret = new TraitsInfoData_Class();
						ret.m_iSlotId = (int)ReadUInt32();
						ret.m_iClassIndex = (int)ReadUInt32();
						return ret;
					}

				case TraitsInfoKind.Function:
					{
						TraitsInfoData_Function ret = new TraitsInfoData_Function();
						ret.m_iSlotId = (int)ReadUInt32();
						ret.m_iFunctionIndex = (int)ReadUInt32();
						return ret;
					}

				case TraitsInfoKind.Method: // fall-through
				case TraitsInfoKind.Getter: // fall-through
				case TraitsInfoKind.Setter:
					{
						TraitsInfoData_Method ret = new TraitsInfoData_Method();
						ret.m_iDispatchId = (int)ReadUInt32();
						ret.m_iMethodIndex = (int)ReadUInt32();
						return ret;
					}

				default:
					throw new Exception();
			}
		}

		byte ReadByte()
		{
			Align();

			byte uReturn = m_aBytecode[OffsetInBytes];
			OffsetInBits += 8;
			return uReturn;
		}

		[StructLayout(LayoutKind.Explicit)]
		struct UInt64ToDouble
		{
			[FieldOffset(0)]
			public UInt64 m_uIn;
			[FieldOffset(0)]
			public Double m_fOut;
		}

		double ReadDouble()
		{
			UInt64ToDouble converter = default(UInt64ToDouble);
			converter.m_uIn = ReadUInt64();
			return converter.m_fOut;
		}

		Int32 ReadInt24()
		{
			Int32 iResult = ReadUInt16();
			iResult += (Int32)(ReadByte() << 16);

			// Sign extend
			const int iShiftBy = (32 - 24);
			int iIntermediate = (iResult << iShiftBy); // shift the result so the sign is the highest bit.
			iResult = (iIntermediate >> iShiftBy); // shift back, arithmetic shift.

			return iResult;
		}

		Int32 ReadInt32()
		{
			Int32 iResult = 0;
			int iShift = 0;
			for (int i = 0; i < 5; ++i)
			{
				byte uValue = ReadByte();
				iResult |= (((int)uValue & (~(1 << 7))) << iShift);
				iShift += 7;

				if ((uValue & (1 << 7)) == 0)
				{
					break;
				}
			}

			// Sign extend
			int iShiftBy = (32 - Min(iShift, 32));
			int iIntermediate = (iResult << iShiftBy); // shift the result so the sign is the highest bit.
			iResult = (iIntermediate >> iShiftBy); // shift back, arithmetic shift.

			return iResult;
		}

		string ReadString()
		{
			// Length DOES NOT include a null terminator, strings in ABC are not null terminated.
			int nLength = (int)ReadUInt32();

			if (nLength < 0)
			{
				throw new Exception();
			}

			if (0 == nLength)
			{
				return null;
			}
			else
			{
				string sReturn = Encoding.UTF8.GetString(m_aBytecode, OffsetInBytes, nLength);
				OffsetInBytes += nLength;
				return sReturn;
			}
		}

		UInt16 ReadUInt16()
		{
			Align();

			int nOffsetInBytes = (OffsetInBytes);
			UInt16 ret = (UInt16)((m_aBytecode[nOffsetInBytes + 1] << 8) | (m_aBytecode[nOffsetInBytes]));
			OffsetInBits += 16;

			return ret;
		}

		UInt32 ReadUInt32()
		{
			UInt32 uResult = 0;
			int iShift = 0;
			for (int i = 0; i < 5; ++i)
			{
				byte uValue = ReadByte();
				uResult |= (UInt32)(((int)uValue & (~(1 << 7))) << iShift);
				iShift += 7;

				if ((uValue & (1 << 7)) == 0)
				{
					break;
				}
			}

			return uResult;
		}

		UInt64 ReadUInt64()
		{
			Align();

			int nOffsetInBytes = (OffsetInBytes);
			UInt64 ret = (UInt64)(
				(m_aBytecode[nOffsetInBytes + 7] << 56) |
				(m_aBytecode[nOffsetInBytes + 6] << 48) |
				(m_aBytecode[nOffsetInBytes + 5] << 40) |
				(m_aBytecode[nOffsetInBytes + 4] << 32) |
				(m_aBytecode[nOffsetInBytes + 3] << 24) |
				(m_aBytecode[nOffsetInBytes + 2] << 16) |
				(m_aBytecode[nOffsetInBytes + 1] << 8) |
				(m_aBytecode[nOffsetInBytes + 0]));
			OffsetInBits += 64;

			return ret;
		}

		void Parse()
		{
			m_iMinorVersion = (int)ReadUInt16();
			m_iMajorVersion = (int)ReadUInt16();
			m_ConstantPool = ReadConstantPool();

			int nMethods = (int)ReadUInt32();
			if (nMethods > 0)
			{
				m_aMethods = new MethodInfo[nMethods];
				for (int i = 0; i < nMethods; ++i)
				{
					m_aMethods[i] = ReadMethodInfo();
				}
			}

			m_aMetadata = ReadMetadata();

			int nClasses = (int)ReadUInt32();
			if (nClasses > 0)
			{
				m_aInstances = new InstanceInfo[nClasses];
				m_aClasses = new ClassInfo[nClasses];
				for (int i = 0; i < nClasses; ++i)
				{
					m_aInstances[i] = ReadInstanceInfo();
				}

				for (int i = 0; i < nClasses; ++i)
				{
					m_aClasses[i] = ReadClassInfo();
				}
			}

			int nScripts = (int)ReadUInt32();
			if (nScripts > 0)
			{
				m_aScripts = new ScriptInfo[nScripts];
				for (int i = 0; i < nScripts; ++i)
				{
					m_aScripts[i] = ReadScriptInfo();
				}
			}

			int nMethodBodies = (int)ReadUInt32();
			if (nMethodBodies > 0)
			{
				m_aMethodBodies = new MethodBodyInfo[nMethodBodies];
				for (int i = 0; i < nMethodBodies; ++i)
				{
					m_aMethodBodies[i] = ReadMethodBodyInfo();
				}
			}

			if (OffsetInBytes != m_aBytecode.Length)
			{
				throw new Exception();
			}
		}
		#endregion

		public ABCFile(byte[] aBytecode)
		{
			m_aBytecode = aBytecode;
			Parse();
		}

		public int MinorVersion { get { return m_iMinorVersion; } }
		public int MajorVersion { get { return m_iMajorVersion; } }
		public ABCFileConstantPool ConstantPool { get { return m_ConstantPool; } }
		public MethodInfo[] Methods { get { return m_aMethods; } }
		public MetadataInfo[] Metadata { get { return m_aMetadata; } }
		public InstanceInfo[] Instances { get { return m_aInstances; } }
		public ClassInfo[] Classes { get { return m_aClasses; } }
		public ScriptInfo[] Scripts { get { return m_aScripts; } }
		public MethodBodyInfo[] MethodBodies { get { return m_aMethodBodies; } }
	}
} // namespace FalconCooker
