// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Runtime;
using System.Runtime.InteropServices;
using System.Text;

namespace FalconCooker
{
	public enum AVM2ValueType
	{
		Undefined,
		Boolean,
		Null,
		Number,
		Object,
		String,
	}

	public enum EventType
	{
		Dispatch = 0,
		DispatchBubble = 1,
	}

	public sealed class AVM2EvalError : Exception
	{
		public AVM2EvalError()
			: base()
		{
		}

		public AVM2EvalError(string s)
			: base(s)
		{
		}
	}

	public sealed class AVM2ReferenceError : Exception
	{
		public AVM2ReferenceError()
			: base()
		{
		}

		public AVM2ReferenceError(string s)
			: base(s)
		{
		}
	}

	public sealed class AVM2TypeError : Exception
	{
		public AVM2TypeError()
			: base()
		{
		}

		public AVM2TypeError(string s)
			: base(s)
		{
		}
	}

	public interface IAVM2Object
	{
		AVM2Value AsPrimitive(AVM2 avm2, bool bHintString);
		AVM2Value Call(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments);
		bool CanGetProperty(AVM2 avm2, string sName);
		IAVM2ClassObject Class { get; }
		bool DeleteProperty(AVM2 avm2, string sName);
		AVM2Value GetProperty(AVM2 avm2, string sName);
		bool IsCallable { get; }
		AVM2Object SetProperty(AVM2 avm2, string sName, AVM2Value value);
	}

	public interface IAVM2ClassObject
	{
		bool CanGetInheritedProperty(AVM2 avm2, AVM2Object oThis, string sName);
		ClassInfo ClassInfo { get; }
		AVM2Value GetInheritedProperty(AVM2 avm2, AVM2Object oThis, string sName);
		IAVM2MethodObject GetInheritedSetter(string sName);
		InstanceInfo InstanceInfo { get; }
		IAVM2ClassObject Parent { get; }
	}

	public interface IAVM2MethodObject
	{
		AVM2Value Call(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments);
	}

	public sealed class AVM2ArrayObject : AVM2Object
	{
		#region Private members
		AVM2Value[] m_aValue = null;

		static internal AVM2Value getLength(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments)
		{
			AVM2ArrayObject oArray = (AVM2ArrayObject)oThis;

			return new AVM2Value((UInt32)oArray.m_aValue.Length);
		}

		static internal AVM2Value setLength(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments)
		{
			AVM2ArrayObject oArray = (AVM2ArrayObject)oThis;

			int iNewLength = aArguments[0].AsInt32(avm2);

			if (iNewLength != oArray.m_aValue.Length)
			{
				System.Array.Resize(ref oArray.m_aValue, iNewLength);
			}

			return default(AVM2Value);
		}
		#endregion

		public AVM2ArrayObject(IAVM2ClassObject classObject, int iSize)
			: base(classObject)
		{
			m_aValue = new AVM2Value[iSize];
		}

		public AVM2Value[] Array
		{
			get
			{
				return m_aValue;
			}
		}
	}

	public sealed class AVM2BooleanObject : AVM2Object
	{
		#region Private members
		bool m_bValue = false;
		#endregion

		public AVM2BooleanObject(IAVM2ClassObject classObject, bool b)
			: base(classObject)
		{
			m_bValue = b;
		}

		public override AVM2Value AsPrimitive(AVM2 avm2, bool bHintString)
		{
			return new AVM2Value(m_bValue);
		}
	}

	public sealed class AVM2ClassObject : AVM2Object, IAVM2ClassObject
	{
		#region Private members
		AVM2 m_AVM2 = null;
		ClassInfo m_ClassInfo = default(ClassInfo);
		InstanceInfo m_InstanceInfo = default(InstanceInfo);
		AVM2MethodObject m_InstanceInitializer = null;
		IAVM2ClassObject m_Parent = null;
		#endregion

		public AVM2ClassObject(
			IAVM2ClassObject classObject,
			IAVM2ClassObject parent,
			AVM2 avm2,
			ClassInfo classInfo,
			InstanceInfo instanceInfo)
			: base(classObject)
		{
			m_AVM2 = avm2;
			m_ClassInfo = classInfo;
			m_InstanceInfo = instanceInfo;
			m_InstanceInitializer = new AVM2MethodObject(
				avm2.FunctionClassObject,
				avm2,
				new List<AVM2Value>(),
				avm2.File.Methods[instanceInfo.m_iInstanceInitializer].m_Body);
			m_Parent = parent;

			avm2.ApplyTraits(this, classInfo.m_aTraits);

			AVM2MethodObject staticInitializer = new AVM2MethodObject(
				avm2.FunctionClassObject,
				avm2,
				new List<AVM2Value>(),
				avm2.File.Methods[classInfo.m_iStaticInitializer].m_Body);
			staticInitializer.Call(m_AVM2, this, null);
		}

		public override AVM2Value Call(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments)
		{
			AVM2Object oNewObject = null;

			IAVM2ClassObject classObject = Parent;
			while (null != classObject)
			{
				AVM2NativeClassObject nativeClassObject = (classObject as AVM2NativeClassObject);
				if (null != nativeClassObject)
				{
					oNewObject = nativeClassObject.DerivedCall(avm2, this, oThis, aArguments);
					break;
				}
				classObject = classObject.Parent;
			}

			if (null == oNewObject)
			{
				oNewObject = new AVM2Object(this);
			}

			m_AVM2.ApplyTraits(oNewObject, m_InstanceInfo.m_aTraits);
			m_InstanceInitializer.Call(avm2, oNewObject, aArguments);
			return new AVM2Value(oNewObject);
		}

		public bool CanGetInheritedProperty(AVM2 avm2, AVM2Object oThis, string sName)
		{
			if (m_tGetters.ContainsKey(sName))
			{
				return true;
			}

			if (m_tMembers.ContainsKey(sName))
			{
				return true;
			}

			if (null != Parent)
			{
				return Parent.CanGetInheritedProperty(avm2, oThis, sName);
			}

			return false;
		}

		public ClassInfo ClassInfo
		{
			get
			{
				return m_ClassInfo;
			}
		}

		public AVM2Value GetInheritedProperty(AVM2 avm2, AVM2Object oThis, string sName)
		{
			IAVM2MethodObject oMethod = null;
			if (m_tGetters.TryGetValue(sName, out oMethod))
			{
				return oMethod.Call(avm2, oThis, null);
			}

			AVM2Value ret = default(AVM2Value);
			if (m_tMembers.TryGetValue(sName, out ret))
			{
				return ret;
			}

			if (null != Parent)
			{
				return Parent.GetInheritedProperty(avm2, oThis, sName);
			}

			return default(AVM2Value);
		}

		public IAVM2MethodObject GetInheritedSetter(string sName)
		{
			IAVM2MethodObject oMethod = null;
			if (m_tSetters.TryGetValue(sName, out oMethod))
			{
				return oMethod;
			}

			if (null != Parent)
			{
				return Parent.GetInheritedSetter(sName);
			}

			return null;
		}

		public InstanceInfo InstanceInfo
		{
			get
			{
				return m_InstanceInfo;
			}
		}

		public override bool IsCallable
		{
			get
			{
				return true;
			}
		}

		public IAVM2ClassObject Parent
		{
			get
			{
				return m_Parent;
			}
		}
	}


	public sealed class AVM2EventObject : AVM2Object
	{
		public AVM2EventObject(IAVM2ClassObject classObject, string sName, bool bBubbles)
			: base(classObject)
		{
			Bubbles = bBubbles;
			Name = sName;
		}

		public bool Bubbles { get; private set; }
		public string Name { get; private set; }
	}

	public struct AVM2Instruction
	{
		public ABCOpCode m_eOpCode;
		public int m_iOperand1;
		public int m_iOperand2;
		public int m_iOperand3;
		public int m_iOperand4;
		public int m_iOperand5;
	}

	public sealed class AVM2MethodObject : AVM2Object, IAVM2MethodObject
	{
		#region Private members
		AVM2 m_AVM2 = null;
		ABCFile m_File = null;
		MethodBodyInfo m_Method = default(MethodBodyInfo);
		byte[] m_aBytecode = null;
		AVM2Value[] m_aRegisters = null;
		List<AVM2Value> m_ScopeStack = new List<AVM2Value>();

		int m_iOffsetInBits;

		void Align()
		{
			m_iOffsetInBits = (0 == (m_iOffsetInBits & 0x7)) ? m_iOffsetInBits : ((m_iOffsetInBits & (~(0x7))) + 8);
		}

		Stack<AVM2Value> EnterMethod(AVM2Object oThis, AVM2Value[] aArguments)
		{
			m_iOffsetInBits = 0;

			Array.Clear(m_aRegisters, 0, m_aRegisters.Length);
			m_aRegisters[0] = new AVM2Value(oThis);

			int iArguments = (null != aArguments ? aArguments.Length : 0);
			if (null != aArguments)
			{
				for (int i = 0; i < Min(m_Method.m_iLocalCount - 1, aArguments.Length); ++i)
				{
					m_aRegisters[i + 1] = aArguments[i];
				}
			}

			// TODO: Optional arguments.

			return new Stack<AVM2Value>(); // TODO:
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

		AVM2Instruction ReadInstruction()
		{
			AVM2Instruction instruction = default(AVM2Instruction);
			instruction.m_eOpCode = (ABCOpCode)ReadByte();

			switch (instruction.m_eOpCode)
			{
				// The following ops have 1 U30 operand
				case ABCOpCode.OP_call:
				case ABCOpCode.OP_coerce:
				case ABCOpCode.OP_construct:
				case ABCOpCode.OP_constructsuper:
				case ABCOpCode.OP_debugfile:
				case ABCOpCode.OP_debugline:
				case ABCOpCode.OP_declocal:
				case ABCOpCode.OP_declocal_i:
				case ABCOpCode.OP_deleteproperty:
				case ABCOpCode.OP_dxns:
				case ABCOpCode.OP_find_property:
				case ABCOpCode.OP_findpropstrict:
				case ABCOpCode.OP_getdescendants:
				case ABCOpCode.OP_getglobalslot:
				case ABCOpCode.OP_getlex:
				case ABCOpCode.OP_getlocal:
				case ABCOpCode.OP_getproperty:
				case ABCOpCode.OP_getscopeobject:
				case ABCOpCode.OP_getslot:
				case ABCOpCode.OP_getsuper:
				case ABCOpCode.OP_inclocal:
				case ABCOpCode.OP_inclocal_i:
				case ABCOpCode.OP_initproperty:
				case ABCOpCode.OP_istype:
				case ABCOpCode.OP_kill:
				case ABCOpCode.OP_newarray:
				case ABCOpCode.OP_newcatch:
				case ABCOpCode.OP_newclass:
				case ABCOpCode.OP_newfunction:
				case ABCOpCode.OP_newobject:
				case ABCOpCode.OP_pushdouble:
				case ABCOpCode.OP_pushint:
				case ABCOpCode.OP_pushnamespace:
				case ABCOpCode.OP_pushshort:
				case ABCOpCode.OP_pushstring:
				case ABCOpCode.OP_pushuint:
				case ABCOpCode.OP_setlocal:
				case ABCOpCode.OP_setglobalslot:
				case ABCOpCode.OP_setproperty:
				case ABCOpCode.OP_setslot:
				case ABCOpCode.OP_setsuper:
					instruction.m_iOperand1 = (int)ReadUInt32();
					break;

				// The following ops have 1 S24 operand
				case ABCOpCode.OP_ifeq:
				case ABCOpCode.OP_iffalse:
				case ABCOpCode.OP_ifge:
				case ABCOpCode.OP_ifgt:
				case ABCOpCode.OP_ifle:
				case ABCOpCode.OP_iflt:
				case ABCOpCode.OP_ifnge:
				case ABCOpCode.OP_ifngt:
				case ABCOpCode.OP_ifnle:
				case ABCOpCode.OP_ifnlt:
				case ABCOpCode.OP_ifne:
				case ABCOpCode.OP_ifstricteq:
				case ABCOpCode.OP_ifstrictne:
				case ABCOpCode.OP_iftrue:
				case ABCOpCode.OP_jump:
					instruction.m_iOperand1 = (int)ReadInt24();
					break;

				// The following ops have 2 U30 operands
				case ABCOpCode.OP_callmethod:
				case ABCOpCode.OP_callproperty:
				case ABCOpCode.OP_callproplex:
				case ABCOpCode.OP_callpropvoid:
				case ABCOpCode.OP_callstatic:
				case ABCOpCode.OP_callsuper:
				case ABCOpCode.OP_callsupervoid:
				case ABCOpCode.OP_constructprop:
					instruction.m_iOperand1 = (int)ReadUInt32();
					instruction.m_iOperand2 = (int)ReadUInt32();
					break;

				// Debug has byte, U30, byte, U30 operands.
				case ABCOpCode.OP_debug:
					instruction.m_iOperand1 = (int)ReadByte();
					instruction.m_iOperand2 = (int)ReadUInt32();
					instruction.m_iOperand3 = (int)ReadByte();
					instruction.m_iOperand4 = (int)ReadUInt32();
					break;

				// hasnext2 is followed by 2 U8 operands.
				case ABCOpCode.OP_hasnext2:
					instruction.m_iOperand1 = (int)ReadByte();
					instruction.m_iOperand2 = (int)ReadByte();
					break;

				// lookup switch is an S24 index (for the default case)
				// followed by a U30 switch case count, and then finally
				// count+1 S24 offsets. We consume these all now, since our mini
				// interpreter is only meant to extract simple
				// actions that are not conditional.
				case ABCOpCode.OP_lookupswitch:
					instruction.m_iOperand1 = (int)ReadInt24();
					instruction.m_iOperand2 = (int)ReadUInt32();
					for (int i = 0; i < instruction.m_iOperand2 + 1; ++i)
					{
						ReadInt24();
					}
					break;

				case ABCOpCode.OP_pushbyte:
					instruction.m_iOperand1 = (int)ReadByte();
					break;

				// All remaining instructions have no operands.
				default:
					break;
			};

			return instruction;
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
		#endregion

		public AVM2MethodObject(
			IAVM2ClassObject classObject,
			AVM2 avm2,
			List<AVM2Value> scopeStack,
			MethodBodyInfo method)
			: base(classObject)
		{
			m_AVM2 = avm2;
			m_File = m_AVM2.File;
			m_Method = method;
			m_aBytecode = method.m_aCode;
			m_aRegisters = new AVM2Value[method.m_iLocalCount];

			foreach (AVM2Value e in scopeStack)
			{
				m_ScopeStack.Add(e);
			}
		}

		public override AVM2Value Call(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments)
		{
			Stack<AVM2Value> stack = EnterMethod(oThis, aArguments);
			while (OffsetInBytes < m_aBytecode.Length)
			{
				// We are only evaluating code up to the first call to addFrameScript(),
				// so we can extract per-frame actions. Once it has been reached, we immediately
				// unwind to the root call.
				if (oThis is AVM2MovieClipObject mc && mc.AddFrameScriptCalled)
				{
					break;
				}

				AVM2Instruction instruction = ReadInstruction();
				switch (instruction.m_eOpCode)
				{
					case ABCOpCode.OP_add:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();

							AVM2Value r;
							double fA = 0.0;
							double fB = 0.0f;
							if (!a.TryAsNumber(m_AVM2, out fA) ||
								!b.TryAsNumber(m_AVM2, out fB))
							{ 
								r = new AVM2Value(a.AsString(m_AVM2) + b.AsString(m_AVM2));
							}
							else
							{
								r = new AVM2Value(fA + fB);
							}
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_add_i:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsInt32(m_AVM2) + b.AsInt32(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_astype:
						{
							AVM2Value a = stack.Pop();

							string sTypeName = m_File.ConstantPool.GetMultiname(null, instruction.m_iOperand1, null);

							if (sTypeName == a.TypeName)
							{
								stack.Push(a);
							}
							else
							{
								stack.Push(new AVM2Value(null));
							}
						}
						break;
					case ABCOpCode.OP_astypelate:
						{
							AVM2Value oClass = stack.Pop();
							AVM2Value value = stack.Pop();

							// TODO:
							if (oClass.AsString(m_AVM2) == value.TypeName)
							{
								stack.Push(value);
							}
							else
							{
								stack.Push(new AVM2Value(null));
							}
						}
						break;
					case ABCOpCode.OP_bitand:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsInt32(m_AVM2) & b.AsInt32(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_bitnot:
						{
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(~a.AsInt32(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_bitor:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsInt32(m_AVM2) | b.AsInt32(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_bitxor:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsInt32(m_AVM2) ^ b.AsInt32(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_call:
						{
							AVM2Value[] aCallArguments = new AVM2Value[instruction.m_iOperand1];
							for (int i = 0; i < aCallArguments.Length; ++i)
							{
								aCallArguments[aCallArguments.Length - i - 1] = stack.Pop();
							}

							AVM2Value oReceiver = stack.Pop();
							AVM2Value oClosure = stack.Pop();

							AVM2Value r = oClosure.AsObject(m_AVM2).Call(m_AVM2, oReceiver.AsObject(m_AVM2), aCallArguments);

							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_callmethod:
						{
							throw new Exception();
						}
					case ABCOpCode.OP_callproperty: // fall-through
					case ABCOpCode.OP_callproplex:
						{
							AVM2Value[] aCallArguments = new AVM2Value[instruction.m_iOperand2];
							for (int i = 0; i < aCallArguments.Length; ++i)
							{
								aCallArguments[aCallArguments.Length - i - 1] = stack.Pop();
							}

							string sPropertyName = m_File.ConstantPool.GetMultiname(avm2, instruction.m_iOperand1, stack);

							AVM2Value receiver = stack.Pop();

							// Deliberate - we're only trying to evaluate enough to consume simple
							// single line statements in the root (while maintaining old code support),
							// not be a propery evaluator.
							if (receiver.Type != AVM2ValueType.Undefined)
							{
								AVM2Object oReceiver = receiver.AsObject(m_AVM2);
								AVM2Value r = oReceiver.GetProperty(avm2, sPropertyName);
								if (r.Type != AVM2ValueType.Undefined)
								{
									stack.Push(r.AsObject(m_AVM2).Call(m_AVM2, oReceiver, aCallArguments));
								}
								else
								{
									stack.Push(default(AVM2Value));
								}
							}
							else
							{
								stack.Push(default(AVM2Value));
							}

							
						}
						break;
					case ABCOpCode.OP_callpropvoid:
						{
							AVM2Value[] aCallArguments = new AVM2Value[instruction.m_iOperand2];
							for (int i = 0; i < aCallArguments.Length; ++i)
							{
								aCallArguments[aCallArguments.Length - i - 1] = stack.Pop();
							}

							string sPropertyName = m_File.ConstantPool.GetMultiname(avm2, instruction.m_iOperand1, stack);
							AVM2Value receiver = stack.Pop();

							// Deliberate - we're only trying to evaluate enough to consume simple
							// single line statements in the root (while maintaining old code support),
							// not be a propery evaluator.
							if (receiver.Type != AVM2ValueType.Undefined)
							{
								AVM2Object oReceiver = receiver.AsObject(m_AVM2);
								AVM2Value r = oReceiver.GetProperty(avm2, sPropertyName);
								if (r.Type != AVM2ValueType.Undefined)
								{
									// discard return value.
									r.AsObject(m_AVM2).Call(m_AVM2, oReceiver, aCallArguments);
								}
							}
						}
						break;
					case ABCOpCode.OP_callstatic:
						{
							AVM2Value[] aCallArguments = new AVM2Value[instruction.m_iOperand1];
							for (int i = 0; i < aCallArguments.Length; ++i)
							{
								aCallArguments[aCallArguments.Length - i - 1] = stack.Pop();
							}

							AVM2Value receiver = stack.Pop();
							AVM2Object oReceiver = receiver.AsObject(m_AVM2);

							AVM2MethodObject oMethod = new AVM2MethodObject(
								m_AVM2.FunctionClassObject,
								m_AVM2,
								m_ScopeStack,
								m_File.Methods[instruction.m_iOperand1].m_Body);

							AVM2Value r = oMethod.Call(m_AVM2, oReceiver, aCallArguments);

							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_callsuper:
						{
							throw new Exception();
						}
					case ABCOpCode.OP_callsupervoid:
						{
							throw new Exception();
						}
					case ABCOpCode.OP_checkfilter:
						{
							// Nop
						}
						break;
					case ABCOpCode.OP_coerce:
						{
							AVM2Value a = stack.Pop();
							string sTargetType = m_File.ConstantPool.GetMultiname(null, instruction.m_iOperand1, null);
							stack.Push(a);
						}
						break;
					case ABCOpCode.OP_coerce_a:
						{
							// Used by the verifier, a nop for our purposes.
						}
						break;
					case ABCOpCode.OP_coerce_s:
						{
							AVM2Value a = stack.Pop();
							switch (a.Type)
							{
								case AVM2ValueType.Null: // fall-through
								case AVM2ValueType.Undefined:
									stack.Push(new AVM2Value(null));
									break;
								default:
									stack.Push(new AVM2Value(a.AsString(m_AVM2)));
									break;
							};
						}
						break;
					case ABCOpCode.OP_construct:
						{
							throw new Exception();
						}
					case ABCOpCode.OP_constructprop:
						{
							AVM2Value[] aCallArguments = new AVM2Value[instruction.m_iOperand2];
							for (int i = 0; i < aCallArguments.Length; ++i)
							{
								aCallArguments[aCallArguments.Length - i - 1] = stack.Pop();
							}

							string sPropertyName = m_File.ConstantPool.GetMultiname(avm2, instruction.m_iOperand1, stack);

							AVM2Value targetObject = stack.Pop();

							// Deliberate - we're only trying to evaluate enough to consume simple
							// single line statements in the root (while maintaining old code support),
							// not be a propery evaluator.
							if (targetObject.Type != AVM2ValueType.Undefined)
							{
								AVM2Value r = targetObject.AsObject(m_AVM2).GetProperty(avm2, sPropertyName);
								if (r.Type != AVM2ValueType.Undefined)
								{
									stack.Push(r.AsObject(m_AVM2).Call(m_AVM2, targetObject.AsObject(m_AVM2), aCallArguments));
								}
								else
								{
									stack.Push(default(AVM2Value));
								}
							}
							else
							{
								stack.Push(default(AVM2Value));
							}
						}
						break;
					case ABCOpCode.OP_constructsuper:
						{
							AVM2Value[] aCallArguments = new AVM2Value[instruction.m_iOperand1];
							for (int i = 0; i < aCallArguments.Length; ++i)
							{
								aCallArguments[aCallArguments.Length - i - 1] = stack.Pop();
							}

							AVM2Value receiver = stack.Pop();

							// TODO: Invoke base constructor on receiver.
						}
						break;
					case ABCOpCode.OP_convert_b:
						{
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsBoolean(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_convert_d:
						{
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsNumber(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_convert_i:
						{
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsInt32(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_convert_o:
						{
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsObject(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_convert_s:
						{
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsString(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_convert_u:
						{
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsUInt32(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_debug:
						{
							// Nop
						}
						break;
					case ABCOpCode.OP_debugfile:
						{
							// Nop
						}
						break;
					case ABCOpCode.OP_debugline:
						{
							// Nop
						}
						break;
					case ABCOpCode.OP_declocal:
						{
							m_aRegisters[instruction.m_iOperand1] = new AVM2Value(m_aRegisters[instruction.m_iOperand1].AsNumber(m_AVM2) - 1.0);
						}
						break;
					case ABCOpCode.OP_declocal_i:
						{
							m_aRegisters[instruction.m_iOperand1] = new AVM2Value(m_aRegisters[instruction.m_iOperand1].AsInt32(m_AVM2) - 1);
						}
						break;
					case ABCOpCode.OP_decrement:
						{
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsNumber(m_AVM2) - 1.0);
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_decrement_i:
						{
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsInt32(m_AVM2) - 1);
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_deleteproperty:
						{
							string sPropertyName = m_File.ConstantPool.GetMultiname(avm2, instruction.m_iOperand1, stack);
							AVM2Value targetObject = stack.Pop();
							AVM2Value r = new AVM2Value(targetObject.AsObject(m_AVM2).DeleteProperty(avm2, sPropertyName));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_divide:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsNumber(m_AVM2) / b.AsNumber(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_dup:
						{
							AVM2Value a = stack.Peek(); // Peek here is deliberate.
							stack.Push(a);
						}
						break;
					case ABCOpCode.OP_dxns:
						{
							// XML routine, nop for us.
						}
						break;
					case ABCOpCode.OP_dxnslate:
						{
							// XML routine, nop for us.
						}
						break;
					case ABCOpCode.OP_equals:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(AVM2Value.AbstractEqualityCompare(m_AVM2, a, b));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_esc_xattr:
						{
							// XML routine, nop for us. Must throw an exception since it modifies the stack.
							throw new Exception();
						}
					case ABCOpCode.OP_esc_xelem:
						{
							// XML routine, nop for us. Must throw an exception since it modifies the stack.
							throw new Exception();
						}
					case ABCOpCode.OP_find_property:
						{
							string sPropertyName = m_File.ConstantPool.GetMultiname(avm2, instruction.m_iOperand1, stack);

							bool bDone = false;
							for (int i = m_ScopeStack.Count - 1; i >= 0; --i)
							{
								AVM2Value e = m_ScopeStack[i];
								if (e.Type != AVM2ValueType.Undefined)
								{
									if (e.AsObject(m_AVM2).CanGetProperty(avm2, sPropertyName))
									{
										stack.Push(e);
										bDone = true;
										break;
									}
								}
							}

							if (!bDone)
							{
								AVM2Value globalObject = m_AVM2.GetGlobalObjectLazy(sPropertyName);
								if (globalObject.Type != AVM2ValueType.Undefined)
								{
									stack.Push(m_AVM2.Global);
									bDone = true;
								}
							}

							if (!bDone)
							{
								stack.Push(m_AVM2.Global);
								bDone = true;
							}
						}
						break;
					case ABCOpCode.OP_findpropstrict:
						{
							string sPropertyName = m_File.ConstantPool.GetMultiname(avm2, instruction.m_iOperand1, stack);

							bool bDone = false;
							for (int i = m_ScopeStack.Count - 1; i >= 0; --i)
							{
								AVM2Value e = m_ScopeStack[i];
								if (e.Type != AVM2ValueType.Undefined)
								{
									AVM2Object oEntry = e.AsObject(m_AVM2);
									if (oEntry.CanGetProperty(avm2, sPropertyName))
									{
										stack.Push(e);
										bDone = true;
										break;
									}
								}
							}

							if (!bDone)
							{
								AVM2Value globalObject = m_AVM2.GetGlobalObjectLazy(sPropertyName);
								if (globalObject.Type != AVM2ValueType.Undefined)
								{
									stack.Push(m_AVM2.Global);
									bDone = true;
								}
							}

							if (!bDone)
							{
								// Undefined as the final result if all other lookup failed.
								stack.Push(default(AVM2Value));
								bDone = true;
							}
						}
						break;
					case ABCOpCode.OP_getdescendants:
						{
							// Part of XML, not implemented.
							throw new Exception();
						}
					case ABCOpCode.OP_getglobalscope:
						{
							throw new Exception();
						}
					case ABCOpCode.OP_getglobalslot:
						{
							throw new Exception();
						}
					case ABCOpCode.OP_getlex:
						{
							string sPropertyName = m_File.ConstantPool.GetMultiname(avm2, instruction.m_iOperand1, stack);

							bool bDone = false;
							for (int i = m_ScopeStack.Count - 1; i >= 0; --i)
							{
								AVM2Value e = m_ScopeStack[i];
								if (e.Type != AVM2ValueType.Undefined)
								{
									AVM2Object o = e.AsObject(m_AVM2);
									if (o.CanGetProperty(avm2, sPropertyName))
									{
										stack.Push(o.GetProperty(avm2, sPropertyName));
										bDone = true;
										break;
									}
								}
							}

							if (!bDone)
							{
								AVM2Value globalObject = m_AVM2.GetGlobalObjectLazy(sPropertyName);
								if (globalObject.Type != AVM2ValueType.Undefined)
								{
									stack.Push(globalObject);
									bDone = true;
								}
							}

							if (!bDone)
							{
								// Undefined as the final result if all other lookup failed.
								stack.Push(default(AVM2Value));
								bDone = true;
							}
						}
						break;
					case ABCOpCode.OP_getlocal:
						{
							stack.Push(m_aRegisters[instruction.m_iOperand1]);
						}
						break;
					case ABCOpCode.OP_getlocal_0:
						{
							stack.Push(m_aRegisters[0]);
						}
						break;
					case ABCOpCode.OP_getlocal_1:
						{
							stack.Push(m_aRegisters[1]);
						}
						break;
					case ABCOpCode.OP_getlocal_2:
						{
							stack.Push(m_aRegisters[2]);
						}
						break;
					case ABCOpCode.OP_getlocal_3:
						{
							stack.Push(m_aRegisters[3]);
						}
						break;
					case ABCOpCode.OP_getproperty:
						{
							string sPropertyName = m_File.ConstantPool.GetMultiname(avm2, instruction.m_iOperand1, stack);

							AVM2Value targetObject = stack.Pop();
							if (targetObject.Type == AVM2ValueType.Undefined)
							{
								stack.Push(default(AVM2Value));
							}
							else
							{
								AVM2Value r = targetObject.AsObject(m_AVM2).GetProperty(avm2, sPropertyName);
								stack.Push(r);
							}
						}
						break;
					case ABCOpCode.OP_getscopeobject:
						{
							AVM2Value r = m_ScopeStack[instruction.m_iOperand1];
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_getslot:
						{
							AVM2Value a = stack.Pop();
							var iSlotId = instruction.m_iOperand1;
							if (a.Type != AVM2ValueType.Undefined)
							{
								AVM2Object o = a.AsObject(m_AVM2);
								stack.Push(o.GetSlot(iSlotId));
							}
							else
							{
								stack.Push(default(AVM2Value));
							}
						}
						break;
					case ABCOpCode.OP_getsuper:
						{
							throw new Exception();
						}
					case ABCOpCode.OP_greaterequals:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							AVM2AbstractRelationalCompareResult eResult = AVM2Value.AbstractRelationalCompare(m_AVM2, a, b, true); // TODO: AVM2 spec does not defined left or not.
							AVM2Value r = new AVM2Value((AVM2AbstractRelationalCompareResult.False == eResult ? true : false));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_greaterthan:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							AVM2AbstractRelationalCompareResult eResult = AVM2Value.AbstractRelationalCompare(m_AVM2, b, a, true); // TODO: AVM2 spec does not defined left or not.
							AVM2Value r = new AVM2Value((AVM2AbstractRelationalCompareResult.True == eResult ? true : false));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_hasnext:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						stack.Pop();
						stack.Push(new AVM2Value(false));
						break;
					case ABCOpCode.OP_hasnext2:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Push(new AVM2Value(false));
						break;
					case ABCOpCode.OP_ifeq:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						stack.Pop();
						break;
					case ABCOpCode.OP_iffalse:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						break;
					case ABCOpCode.OP_ifge:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						stack.Pop();
						break;
					case ABCOpCode.OP_ifgt:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						stack.Pop();
						break;
					case ABCOpCode.OP_ifle:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						stack.Pop();
						break;
					case ABCOpCode.OP_iflt:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						stack.Pop();
						break;
					case ABCOpCode.OP_ifne:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						stack.Pop();
						break;
					case ABCOpCode.OP_ifnge:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						stack.Pop();
						break;
					case ABCOpCode.OP_ifngt:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						stack.Pop();
						break;
					case ABCOpCode.OP_ifnle:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						stack.Pop();
						break;
					case ABCOpCode.OP_ifnlt:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						stack.Pop();
						break;
					case ABCOpCode.OP_ifstricteq:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						stack.Pop();
						break;
					case ABCOpCode.OP_ifstrictne:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						stack.Pop();
						break;
					case ABCOpCode.OP_iftrue:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						break;
					case ABCOpCode.OP_in:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						stack.Pop();
						stack.Push(new AVM2Value(false));
						break;
					case ABCOpCode.OP_inclocal:
						m_aRegisters[instruction.m_iOperand1] = new AVM2Value(m_aRegisters[instruction.m_iOperand1].AsNumber(m_AVM2) + 1.0);
						break;
					case ABCOpCode.OP_inclocal_i:
						m_aRegisters[instruction.m_iOperand1] = new AVM2Value(m_aRegisters[instruction.m_iOperand1].AsInt32(m_AVM2) + 1);
						break;
					case ABCOpCode.OP_increment:
						{
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsNumber(m_AVM2) + 1.0);
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_increment_i:
						{
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsInt32(m_AVM2) + 1);
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_initproperty:
						{
							string sPropertyName = m_File.ConstantPool.GetMultiname(avm2, instruction.m_iOperand1, stack);

							AVM2Value value = stack.Pop();
							AVM2Value oObject = stack.Pop();

							oObject.AsObject(m_AVM2).SetProperty(m_AVM2, sPropertyName, value);
						}
						break;
					case ABCOpCode.OP_instanceof:
						{
							throw new Exception();
						}
					case ABCOpCode.OP_istype:
						{
							throw new Exception();
						}
					case ABCOpCode.OP_istypelate:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop();
						stack.Pop();
						stack.Push(new AVM2Value(false));
						break;
					case ABCOpCode.OP_jump:
						OffsetInBytes += instruction.m_iOperand1;
						break;
					case ABCOpCode.OP_kill:
						m_aRegisters[instruction.m_iOperand1] = default(AVM2Value);
						break;
					case ABCOpCode.OP_label:
						// Used by the verifier - nop at execution time.
						break;
					case ABCOpCode.OP_lessequals:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							AVM2AbstractRelationalCompareResult eResult = AVM2Value.AbstractRelationalCompare(m_AVM2, b, a, true);
							stack.Push(new AVM2Value(AVM2AbstractRelationalCompareResult.False == eResult ? true : false));
						}
						break;
					case ABCOpCode.OP_lessthan:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							AVM2AbstractRelationalCompareResult eResult = AVM2Value.AbstractRelationalCompare(m_AVM2, b, a, true);
							stack.Push(new AVM2Value(AVM2AbstractRelationalCompareResult.True == eResult ? true : false));
						}
						break;
					case ABCOpCode.OP_lookupswitch:
						// Nop, we don't evaluate conditionals. We've consume
						// the instruction in the ReadInstruction() call.
						stack.Pop(); // Pop the index from the stack.
						break;
					// ECMAScript Language Specific 11.7.1: http://www.ecma-international.org/ecma-262/5.1/#sec-11.7.1
					case ABCOpCode.OP_lshift:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							Int32 iA = a.AsInt32(m_AVM2);
							UInt32 uB = b.AsUInt32(m_AVM2);
							Int32 iResult = (iA << (Int32)(uB & 0x1F));
							AVM2Value r = new AVM2Value(iResult);
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_modulo:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsNumber(m_AVM2) % b.AsNumber(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_multiply:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsNumber(m_AVM2) * b.AsNumber(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_multiply_i:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsInt32(m_AVM2) * b.AsInt32(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_negate:
						{
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(-a.AsNumber(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_negate_i:
						{
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(-a.AsInt32(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_newactivation:
						stack.Push(default(AVM2Value));
						break;
					case ABCOpCode.OP_newarray:
						{
							AVM2ArrayObject arrayObject = new AVM2ArrayObject(m_AVM2.ArrayClassObject, instruction.m_iOperand1);
							for (int i = 0; i < instruction.m_iOperand1; ++i)
							{
								arrayObject.Array[instruction.m_iOperand1 - i - 1] = stack.Pop();
							}
							stack.Push(new AVM2Value(arrayObject));
						}
						break;
					case ABCOpCode.OP_newcatch:
						{
							throw new Exception();
						}
					case ABCOpCode.OP_newclass:
						{
							AVM2Value baseClassObject = stack.Pop();
							ClassInfo classInfo = m_File.Classes[instruction.m_iOperand1];
							InstanceInfo instanceInfo = m_File.Instances[instruction.m_iOperand1];

							string sParentName = m_File.ConstantPool.GetMultiname(null, instanceInfo.m_iSuperName, null);
							IAVM2ClassObject parentClassObject = (IAVM2ClassObject)m_AVM2.GetGlobalObjectLazy(sParentName).AsObject(m_AVM2);
							AVM2ClassObject classObject = new AVM2ClassObject(
								m_AVM2.ClassClassObject,
								parentClassObject,
								m_AVM2,
								classInfo,
								instanceInfo);

							stack.Push(new AVM2Value(classObject));
						}
						break;
					case ABCOpCode.OP_newfunction:
						{
							AVM2MethodObject methodObject = new AVM2MethodObject(
								m_AVM2.FunctionClassObject,
								m_AVM2,
								m_ScopeStack,
								m_File.Methods[instruction.m_iOperand1].m_Body);
							stack.Push(new AVM2Value(methodObject));
						}
						break;
					case ABCOpCode.OP_newobject:
						{
							AVM2Object newObject = new AVM2Object(m_AVM2.ObjectClassObject);
							for (int i = 0; i < instruction.m_iOperand1; ++i)
							{
								AVM2Value propertyValue = stack.Pop();
								AVM2Value propertyName = stack.Pop();
								newObject.SetProperty(avm2, propertyName.AsString(m_AVM2), propertyValue);
							}
							stack.Push(new AVM2Value(newObject));
						}
						break;
					case ABCOpCode.OP_nextname:
						{
							throw new Exception();
						}
					case ABCOpCode.OP_nextvalue:
						{
							throw new Exception();
						}
					case ABCOpCode.OP_nop:
						{
							// Nop
						}
						break;
					case ABCOpCode.OP_not:
						{
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(!a.AsBoolean(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_pop:
						stack.Pop();
						break;
					case ABCOpCode.OP_popscope:
						m_ScopeStack.RemoveAt(m_ScopeStack.Count - 1);
						break;
					case ABCOpCode.OP_pushbyte:
						stack.Push(new AVM2Value(instruction.m_iOperand1));
						break;
					case ABCOpCode.OP_pushdouble:
						stack.Push(new AVM2Value(m_File.ConstantPool.m_aDoubles[instruction.m_iOperand1]));
						break;
					case ABCOpCode.OP_pushfalse:
						stack.Push(new AVM2Value(false));
						break;
					case ABCOpCode.OP_pushint:
						stack.Push(new AVM2Value(m_File.ConstantPool.m_aIntegers[instruction.m_iOperand1]));
						break;
					case ABCOpCode.OP_pushnamespace:
						{
							throw new Exception();
						}
					case ABCOpCode.OP_pushnan:
						stack.Push(new AVM2Value(double.NaN));
						break;
					case ABCOpCode.OP_pushnull:
						stack.Push(new AVM2Value(null));
						break;
					case ABCOpCode.OP_pushscope:
						{
							AVM2Value a = stack.Pop();
							m_ScopeStack.Add(a);
						}
						break;
					case ABCOpCode.OP_pushshort:
						stack.Push(new AVM2Value(instruction.m_iOperand1));
						break;
					case ABCOpCode.OP_pushstring:
						stack.Push(new AVM2Value(m_File.ConstantPool.m_aStrings[instruction.m_iOperand1]));
						break;
					case ABCOpCode.OP_pushtrue:
						stack.Push(new AVM2Value(true));
						break;
					case ABCOpCode.OP_pushuint:
						stack.Push(new AVM2Value(m_File.ConstantPool.m_aUnsignedIntegers[instruction.m_iOperand1]));
						break;
					case ABCOpCode.OP_pushundefined:
						stack.Push(default(AVM2Value));
						break;
					case ABCOpCode.OP_pushwith:
						{
							throw new Exception();
						}
					case ABCOpCode.OP_returnvalue:
						{
							AVM2Value retValue = stack.Pop();

							if (stack.Count != 0)
							{
								throw new AVM2EvalError();
							}

							return retValue;
						}
					case ABCOpCode.OP_returnvoid:
						{
							if (stack.Count != 0)
							{
								throw new AVM2EvalError();
							}

							return default(AVM2Value);
						}
						// ECMAScript Language Specific 11.7.2: http://www.ecma-international.org/ecma-262/5.1/#sec-11.7.2
					case ABCOpCode.OP_rshift:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							Int32 iA = a.AsInt32(m_AVM2);
							UInt32 uB = b.AsUInt32(m_AVM2);
							Int32 iResult = (iA >> (Int32)(uB & 0x1F));
							AVM2Value r = new AVM2Value(iResult);
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_setglobalslot:
						{
							throw new Exception();
						}
					case ABCOpCode.OP_setlocal:
						{
							AVM2Value a = stack.Pop();
							m_aRegisters[instruction.m_iOperand1] = a;
						}
						break;
					case ABCOpCode.OP_setlocal_0:
						{
							AVM2Value a = stack.Pop();
							m_aRegisters[0] = a;
						}
						break;
					case ABCOpCode.OP_setlocal_1:
						{
							AVM2Value a = stack.Pop();
							m_aRegisters[1] = a;
						}
						break;
					case ABCOpCode.OP_setlocal_2:
						{
							AVM2Value a = stack.Pop();
							m_aRegisters[2] = a;
						}
						break;
					case ABCOpCode.OP_setlocal_3:
						{
							AVM2Value a = stack.Pop();
							m_aRegisters[3] = a;
						}
						break;
					case ABCOpCode.OP_setproperty:
						{
							AVM2Value propertyValue = stack.Pop();
							string sPropertyName = m_File.ConstantPool.GetMultiname(avm2, instruction.m_iOperand1, stack);
							AVM2Value oPropertyTarget = stack.Pop();

							// Deliberate - we're only trying to evaluate enough to consume simple
							// single line statements in the root (while maintaining old code support),
							// not be a propery evaluator.
							if (oPropertyTarget.Type != AVM2ValueType.Undefined)
							{
								oPropertyTarget.AsObject(m_AVM2).SetProperty(avm2, sPropertyName, propertyValue);
							}
						}
						break;
					case ABCOpCode.OP_setslot:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							var iSlotId = instruction.m_iOperand1;
							if (a.Type != AVM2ValueType.Undefined)
							{
								AVM2Object o = a.AsObject(m_AVM2);
								o.SetSlot(iSlotId, b);
							}
						}
						break;
					case ABCOpCode.OP_setsuper:
						{
							AVM2Value propertyValue = stack.Pop();
							string sPropertyName = m_File.ConstantPool.GetMultiname(avm2, instruction.m_iOperand1, stack);
							AVM2Value oPropertyTarget = stack.Pop();

							oPropertyTarget.AsObject(m_AVM2).SetProperty(avm2, sPropertyName, propertyValue);
						}
						break;
					case ABCOpCode.OP_strictequals:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(AVM2Value.StrictEqualityCompare(m_AVM2, a, b));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_subtract:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsNumber(m_AVM2) - b.AsNumber(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_subtract_i:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							AVM2Value r = new AVM2Value(a.AsInt32(m_AVM2) - b.AsInt32(m_AVM2));
							stack.Push(r);
						}
						break;
					case ABCOpCode.OP_swap:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							stack.Push(b);
							stack.Push(a);
						}
						break;
					case ABCOpCode.OP_throw:
						{
							throw new Exception();
						}
					case ABCOpCode.OP_typeof:
						{
							AVM2Value a = stack.Pop();
							stack.Push(new AVM2Value(a.TypeName));
						}
						break;
						// ECMAScript Language Specific 11.7.3: http://www.ecma-international.org/ecma-262/5.1/#sec-11.7.3
					case ABCOpCode.OP_urshift:
						{
							AVM2Value b = stack.Pop();
							AVM2Value a = stack.Pop();
							UInt32 uA = a.AsUInt32(m_AVM2);
							UInt32 uB = b.AsUInt32(m_AVM2);
							UInt32 uResult = (uA >> (Int32)(uB & 0x1F));
							AVM2Value r = new AVM2Value(uResult);
							stack.Push(r);
						}
						break;
					default:
						throw new AVM2EvalError("Unknown instruction op code '" + instruction.m_eOpCode.ToString() + "'.");
				};
			}

			return default(AVM2Value);
		}

		public override bool IsCallable
		{
			get
			{
				return true;
			}
		}
	}

	public sealed class AVM2MovieClipObject : AVM2Object
	{
		#region Private members
		bool m_bAddFrameScript = false;
		int m_iActiveFrame = -1;
		Dictionary<int, Dictionary<string, EventType>> m_tEvents = new Dictionary<int, Dictionary<string, EventType>>();
		List<int> m_FrameStops = new List<int>();
		Dictionary<int, bool> m_tVisibleChanges = new Dictionary<int, bool>();
	
		static internal AVM2Value addEventListener(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments)
		{
			return default(AVM2Value);
		}

		static internal AVM2Value addFrameScript(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments)
		{
			AVM2MovieClipObject oMovieClip = (AVM2MovieClipObject)oThis;
			try
			{
				for (int i = 0; i < aArguments.Length; i += 2)
				{
					AVM2Value frameIndex = aArguments[i + 0];
					AVM2Value frameScript = aArguments[i + 1];

					oMovieClip.m_iActiveFrame = frameIndex.AsInt32(avm2);
					try
					{
						bool bBeforeVisible = true;
						if (oThis.CanGetProperty(avm2, "visible"))
						{
							bBeforeVisible = oThis.GetProperty(avm2, "visible").AsBoolean(avm2);
						}

						frameScript.AsObject(avm2).Call(avm2, oThis, null);

						bool bAfterVisible = true;
						if (oThis.CanGetProperty(avm2, "visible"))
						{
							bAfterVisible = oThis.GetProperty(avm2, "visible").AsBoolean(avm2);
						}

						if (bAfterVisible != bBeforeVisible)
						{
							oMovieClip.m_tVisibleChanges.Add(oMovieClip.m_iActiveFrame, bAfterVisible);
						}
					}
					catch (Exception e)
					{
						Console.Error.WriteLine("Error running framescript '" + frameIndex.AsInt32(avm2).ToString() +
							"': " + e.Message.ToString());
					}
					finally
					{
						oMovieClip.m_iActiveFrame = -1;
					}
				}

				oMovieClip.m_FrameStops.Sort();
			}
			finally
			{
				oMovieClip.m_iActiveFrame = -1;
				oMovieClip.m_bAddFrameScript = true;
			}

			return default(AVM2Value);
		}

		static internal AVM2Value dispatchEvent(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments)
		{
			AVM2MovieClipObject oMovieClip = (AVM2MovieClipObject)oThis;
			int iFrame = oMovieClip.m_iActiveFrame;
			Dictionary<string, EventType> events = null;
			if (!oMovieClip.m_tEvents.TryGetValue(iFrame, out events))
			{
				events = new Dictionary<string, EventType>();
				oMovieClip.m_tEvents.Add(iFrame, events);
			}

			AVM2EventObject eventObject = aArguments[0].AsObject(avm2) as AVM2EventObject;
			EventType eExistingValue = EventType.Dispatch;
			events.TryGetValue(eventObject.Name, out eExistingValue);
			if (eventObject.Bubbles) { eExistingValue = EventType.DispatchBubble; }

			events[eventObject.Name] = eExistingValue;

			return default(AVM2Value);
		}

		static internal AVM2Value getChildByName(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments)
		{
			string sPropertyName = aArguments[0].AsString(avm2);
			return oThis.GetProperty(avm2, sPropertyName);
		}

		static internal AVM2Value gotoAndStop(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments)
		{
			AVM2MovieClipObject oMovieClip = (AVM2MovieClipObject)oThis;
			int iFrame = oMovieClip.m_iActiveFrame;
			Dictionary<string, EventType> events = null;
			if (!oMovieClip.m_tEvents.TryGetValue(iFrame, out events))
			{
				events = new Dictionary<string, EventType>();
				oMovieClip.m_tEvents.Add(iFrame, events);
			}

			if (aArguments[0].Type == AVM2ValueType.String)
			{
				events["gotoAndStopByLabel:" + aArguments[0].AsString(avm2)] = EventType.Dispatch;
			}
			else if (aArguments[0].Type == AVM2ValueType.Number)
			{
				events["gotoAndStop:" + ((int)aArguments[0].AsNumber(avm2)).ToString()] = EventType.Dispatch;
			}

			return default(AVM2Value);
		}

		static internal AVM2Value gotoAndPlay(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments)
		{
			AVM2MovieClipObject oMovieClip = (AVM2MovieClipObject)oThis;
			int iFrame = oMovieClip.m_iActiveFrame;
			Dictionary<string, EventType> events = null;
			if (!oMovieClip.m_tEvents.TryGetValue(iFrame, out events))
			{
				events = new Dictionary<string, EventType>();
				oMovieClip.m_tEvents.Add(iFrame, events);
			}

			if (aArguments[0].Type == AVM2ValueType.String)
			{
				events["gotoAndPlayByLabel:" + aArguments[0].AsString(avm2)] = EventType.Dispatch;
			}
			else if (aArguments[0].Type == AVM2ValueType.Number)
			{
				events["gotoAndPlay:" + ((int)aArguments[0].AsNumber(avm2)).ToString()] = EventType.Dispatch;
			}

			return default(AVM2Value);
		}

		static internal AVM2Value stop(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments)
		{
			AVM2MovieClipObject oMovieClip = (AVM2MovieClipObject)oThis;
			oMovieClip.m_FrameStops.Add(oMovieClip.m_iActiveFrame);
			return default(AVM2Value);
		}
		#endregion

		public AVM2MovieClipObject(IAVM2ClassObject classObject)
			: base(classObject)
		{
		}

		public bool AddFrameScriptCalled { get { return m_bAddFrameScript; } }

		public Dictionary<int, Dictionary<string, EventType>> Events
		{
			get
			{
				return m_tEvents;
			}
		}

		public List<int> FrameStops
		{
			get
			{
				return m_FrameStops;
			}
		}

		public Dictionary<int, bool> VisibleChanges
		{
			get
			{
				return m_tVisibleChanges;
			}
		}
	}

	public delegate AVM2Value AVM2NativeClassCallDelegate(AVM2 avm2, IAVM2ClassObject classObject, AVM2Object oThis, AVM2Value[] aArguments);
	public sealed class AVM2NativeClassObject : AVM2Object, IAVM2ClassObject
	{
		#region Private members
		IAVM2ClassObject m_Parent = null;
		AVM2NativeClassCallDelegate m_AVM2NativeClassCallDelegate = null;
		#endregion

		public AVM2NativeClassObject(
			IAVM2ClassObject classObject,
			IAVM2ClassObject parent,
			AVM2NativeClassCallDelegate call)
			: base(classObject)
		{
			m_Parent = parent;
			m_AVM2NativeClassCallDelegate = call;
		}

		public override AVM2Value Call(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments)
		{
			return m_AVM2NativeClassCallDelegate(avm2, this, oThis, aArguments);
		}

		public bool CanGetInheritedProperty(AVM2 avm2, AVM2Object oThis, string sName)
		{
			if (m_tGetters.ContainsKey(sName))
			{
				return true;
			}

			if (m_tMembers.ContainsKey(sName))
			{
				return true;
			}

			if (null != Parent)
			{
				return Parent.CanGetInheritedProperty(avm2, oThis, sName);
			}

			return false;
		}

		public ClassInfo ClassInfo
		{
			get
			{
				return new ClassInfo();
			}
		}

		public AVM2Object DerivedCall(AVM2 avm2, IAVM2ClassObject classObject, AVM2Object oThis, AVM2Value[] aArguments)
		{
			return m_AVM2NativeClassCallDelegate(avm2, classObject, oThis, aArguments).AsObject(avm2);
		}

		public AVM2Value GetInheritedProperty(AVM2 avm2, AVM2Object oThis, string sName)
		{
			IAVM2MethodObject oMethod = null;
			if (m_tGetters.TryGetValue(sName, out oMethod))
			{
				return oMethod.Call(avm2, oThis, null);
			}

			AVM2Value ret = default(AVM2Value);
			if (m_tMembers.TryGetValue(sName, out ret))
			{
				return ret;
			}

			if (null != Parent)
			{
				return Parent.GetInheritedProperty(avm2, oThis, sName);
			}

			return default(AVM2Value);
		}

		public IAVM2MethodObject GetInheritedSetter(string sName)
		{
			IAVM2MethodObject oMethod = null;
			if (m_tSetters.TryGetValue(sName, out oMethod))
			{
				return oMethod;
			}

			if (null != Parent)
			{
				return Parent.GetInheritedSetter(sName);
			}

			return null;
		}

		public InstanceInfo InstanceInfo
		{
			get
			{
				return new InstanceInfo();
			}
		}

		public override bool IsCallable
		{
			get
			{
				return true;
			}
		}

		public IAVM2ClassObject Parent
		{
			get
			{
				return m_Parent;
			}
		}
	}

	public delegate AVM2Value AVM2NativeMethodDelegate(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments);
	public sealed class AVM2NativeMethodObject : AVM2Object, IAVM2MethodObject
	{
		#region Private members
		AVM2NativeMethodDelegate m_AVM2NativeMethodDelegate = null;
		#endregion

		public AVM2NativeMethodObject(
			IAVM2ClassObject classObject,
			AVM2NativeMethodDelegate method)
			: base(classObject)
		{
			m_AVM2NativeMethodDelegate = method;
		}

		public override AVM2Value Call(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments)
		{
			return m_AVM2NativeMethodDelegate(avm2, oThis, aArguments);
		}

		public override bool IsCallable
		{
			get
			{
				return true;
			}
		}
	}

	public sealed class AVM2NumberObject : AVM2Object
	{
		#region Private member
		IAVM2ClassObject m_ClassObject = null;
		double m_fValue = 0.0;
		#endregion

		public AVM2NumberObject(
			IAVM2ClassObject classObject,
			double f)
			: base(classObject)
		{
			m_ClassObject = classObject;
			m_fValue = f;
		}

		public override AVM2Value AsPrimitive(AVM2 avm2, bool bHintString)
		{
			return new AVM2Value(m_fValue);
		}
	}

	public class AVM2Object : IAVM2Object
	{
		public const string kToString = "toString";
		public const string kValueOf = "valueOf";

		#region Private and internal members
		IAVM2ClassObject m_ClassObject = null;
		internal Dictionary<string, IAVM2MethodObject> m_tGetters = new Dictionary<string, IAVM2MethodObject>();
		internal Dictionary<string, AVM2Value> m_tMembers = new Dictionary<string, AVM2Value>();
		internal Dictionary<string, IAVM2MethodObject> m_tSetters = new Dictionary<string, IAVM2MethodObject>();
		readonly Dictionary<int, AVM2Value> m_tSlots = new Dictionary<int, AVM2Value>();
		#endregion

		public AVM2Object(IAVM2ClassObject classObject)
		{
			m_ClassObject = classObject;
		}

		// ECMAScript Language Specification 8.12.8: http://www.ecma-international.org/ecma-262/5.1/#sec-8.12.8
		public virtual AVM2Value AsPrimitive(AVM2 avm2, bool bHintString)
		{
			if (bHintString)
			{
				if (m_tMembers.ContainsKey(kToString))
				{
					AVM2Value toString = m_tMembers[kToString];
					AVM2Object o = toString.AsObject(avm2);
					if (o.IsCallable)
					{
						AVM2Value primitiveValue = o.Call(avm2, o, null);
						if (primitiveValue.IsPrimitive)
						{
							return primitiveValue;
						}
					}
				}

				if (m_tMembers.ContainsKey(kValueOf))
				{
					AVM2Value valueOf = m_tMembers[kValueOf];
					AVM2Object o = valueOf.AsObject(avm2);
					if (o.IsCallable)
					{
						AVM2Value primitiveValue = o.Call(avm2, o, null);
						if (primitiveValue.IsPrimitive)
						{
							return primitiveValue;
						}
					}
				}

				return new AVM2Value(null);
			}
			else
			{
				if (m_tMembers.ContainsKey(kValueOf))
				{
					AVM2Value valueOf = m_tMembers[kValueOf];
					AVM2Object o = valueOf.AsObject(avm2);
					if (o.IsCallable)
					{
						AVM2Value primitiveValue = o.Call(avm2, o, null);
						if (primitiveValue.IsPrimitive)
						{
							return primitiveValue;
						}
					}
				}

				if (m_tMembers.ContainsKey(kToString))
				{
					AVM2Value toString = m_tMembers[kToString];
					AVM2Object o = toString.AsObject(avm2);
					if (o.IsCallable)
					{
						AVM2Value primitiveValue = o.Call(avm2, o, null);
						if (primitiveValue.IsPrimitive)
						{
							return primitiveValue;
						}
					}
				}

				return new AVM2Value(null);
			}
		}

		public virtual AVM2Value Call(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments)
		{
			throw new AVM2EvalError();
		}

		public virtual IAVM2ClassObject Class
		{
			get
			{
				return m_ClassObject;
			}
		}

		public virtual bool DeleteProperty(AVM2 avm2, string sName)
		{
			// TODO: Restrict to dynamic properties/objects.
			if (m_tMembers.ContainsKey(sName))
			{
				m_tMembers.Remove(sName);
				return true;
			}

			return false;
		}

		public virtual bool CanGetProperty(AVM2 avm2, string sName)
		{
			if (m_tGetters.ContainsKey(sName))
			{
				return true;
			}

			if (m_tMembers.ContainsKey(sName))
			{
				return true;
			}

			if (null != Class)
			{
				return Class.CanGetInheritedProperty(avm2, this, sName);
			}

			return false;
		}

		public AVM2Value GetProperty(AVM2 avm2, string sName)
		{
			IAVM2MethodObject oMethod = null;
			if (m_tGetters.TryGetValue(sName, out oMethod))
			{
				return oMethod.Call(avm2, this, null);
			}

			AVM2Value ret = default(AVM2Value);
			if (m_tMembers.TryGetValue(sName, out ret))
			{
				return ret;
			}

			if (null != Class)
			{
				return Class.GetInheritedProperty(avm2, this, sName);
			}

			return default(AVM2Value);
		}

		public AVM2Value GetSlot(int iSlot)
		{
			AVM2Value ret = default(AVM2Value);
			m_tSlots.TryGetValue(iSlot, out ret);
			return ret;
		}

		public virtual bool IsCallable
		{
			get
			{
				return false;
			}
		}

		public AVM2Object SetGetter(AVM2 avm2, string sName, IAVM2MethodObject oGetter)
		{
			m_tGetters[sName] = oGetter;
			return this;
		}

		public virtual AVM2Object SetProperty(AVM2 avm2, string sName, AVM2Value value)
		{
			if (null != Class)
			{
				IAVM2MethodObject oMethod = Class.GetInheritedSetter(sName);
				if (null != oMethod)
				{
					oMethod.Call(avm2, this, new AVM2Value[] { value });
					return this;
				}
			}

			{
				IAVM2MethodObject oMethod = null;
				if (m_tSetters.TryGetValue(sName, out oMethod))
				{
					oMethod.Call(avm2, this, new AVM2Value[] { value });
					return this;
				}
			}

			m_tMembers[sName] = value;
			return this;
		}

		public AVM2Value SetSlot(int iSlot, AVM2Value v)
		{
			m_tSlots[iSlot] = v;
			return v;
		}

		public AVM2Object SetSetter(AVM2 avm2, string sName, IAVM2MethodObject oSetter)
		{
			m_tSetters[sName] = oSetter;
			return this;
		}
	}

	public sealed class AVM2StringObject : AVM2Object
	{
		#region Private members
		string m_sValue = string.Empty;
		#endregion

		public AVM2StringObject(IAVM2ClassObject classObject, string s)
			: base(classObject)
		{
			m_sValue = s;
		}

		public override AVM2Value AsPrimitive(AVM2 avm2, bool bHintString)
		{
			return new AVM2Value(m_sValue);
		}
	}

	public enum AVM2AbstractRelationalCompareResult
	{
		Undefined = -1,
		False,
		True,
	}

	public struct AVM2Value
	{
		#region Private members
		AVM2ValueType m_eType;
		object m_Value;
		#endregion

		public AVM2Value(bool b)
		{
			m_eType = AVM2ValueType.Boolean;
			m_Value = b;
		}

		public AVM2Value(object o)
		{
			m_eType = (null == o ? AVM2ValueType.Null : AVM2ValueType.Object);
			m_Value = o;
		}

		public AVM2Value(double f)
		{
			m_eType = AVM2ValueType.Number;
			m_Value = f;
		}

		public AVM2Value(Int32 i)
		{
			m_eType = AVM2ValueType.Number;
			m_Value = (double)i;
		}

		public AVM2Value(UInt32 u)
		{
			m_eType = AVM2ValueType.Number;
			m_Value = (double)u;
		}

		public AVM2Value(string s)
		{
			m_eType = AVM2ValueType.String;
			m_Value = string.IsNullOrEmpty(s) ? string.Empty : s;
		}

		// ECMAScript Language Specification 11.9.3: http://www.ecma-international.org/ecma-262/5.1/#sec-11.9.3
		public static bool AbstractEqualityCompare(AVM2 avm2, AVM2Value a, AVM2Value b)
		{
			if (a.m_eType == b.m_eType)
			{
				switch (a.m_eType)
				{
					case AVM2ValueType.Boolean: return (a.AsBoolean(avm2) == b.AsBoolean(avm2));
					case AVM2ValueType.Null: return true;
					case AVM2ValueType.Number: return (a.AsNumber(avm2) == b.AsNumber(avm2));
					case AVM2ValueType.Object: return (a.m_Value == b.m_Value);
					case AVM2ValueType.String: return (a.AsString(avm2) == b.AsString(avm2));
					case AVM2ValueType.Undefined: return true;
					default:
						throw new ArgumentException("Unknown type " + a.m_eType.ToString());
				}
			}
			else
			{
				if ((a.m_eType == AVM2ValueType.Null && b.m_eType == AVM2ValueType.Undefined) ||
					(a.m_eType == AVM2ValueType.Undefined && b.m_eType == AVM2ValueType.Null))
				{
					return true;
				}

				if ((a.m_eType == AVM2ValueType.Number && b.m_eType == AVM2ValueType.String) ||
					(a.m_eType == AVM2ValueType.String && b.m_eType == AVM2ValueType.Number))
				{
					return (a.AsNumber(avm2) == b.AsNumber(avm2));
				}

				if (a.m_eType == AVM2ValueType.Boolean)
				{
					return AbstractEqualityCompare(avm2, new AVM2Value(a.AsNumber(avm2)), b);
				}

				if (b.m_eType == AVM2ValueType.Boolean)
				{
					return AbstractEqualityCompare(avm2, a, new AVM2Value(b.AsNumber(avm2)));
				}

				if (a.m_eType == AVM2ValueType.Number && b.m_eType == AVM2ValueType.Object)
				{
					try
					{
						var prim = b.AsPrimitive(avm2, false);
						return AbstractEqualityCompare(avm2, a, prim);
					}
					catch (AVM2TypeError)
					{
						return false;
					}
				}

				if (a.m_eType == AVM2ValueType.String && b.m_eType == AVM2ValueType.Object)
				{
					try
					{
						var prim = b.AsPrimitive(avm2, true);
						return AbstractEqualityCompare(avm2, a, prim);
					}
					catch (AVM2TypeError)
					{
						return false;
					}
				}

				if (b.m_eType == AVM2ValueType.Number && a.m_eType == AVM2ValueType.Object)
				{
					try
					{
						var prim = a.AsPrimitive(avm2, false);
						return AbstractEqualityCompare(avm2, prim, b);
					}
					catch (AVM2TypeError)
					{
						return false;
					}
				}

				if (b.m_eType == AVM2ValueType.String && a.m_eType == AVM2ValueType.Object)
				{
					try
					{
						var prim = a.AsPrimitive(avm2, true);
						return AbstractEqualityCompare(avm2, prim, b);
					}
					catch (AVM2TypeError)
					{
						return false;
					}
				}

				return false;
			}
		}

		// ECMAScript Language Specification 11.8.5: http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
		public static AVM2AbstractRelationalCompareResult AbstractRelationalCompare(AVM2 avm2, AVM2Value a, AVM2Value b, bool bLeftFirst)
		{
			AVM2Value primitiveA;
			AVM2Value primitiveB;

			if (bLeftFirst)
			{
				primitiveA = a.AsPrimitive(avm2, false);
				primitiveB = b.AsPrimitive(avm2, false);
			}
			else
			{
				primitiveB = b.AsPrimitive(avm2, false);
				primitiveA = a.AsPrimitive(avm2, false);
			}

			if (primitiveA.m_eType != AVM2ValueType.String || primitiveB.m_eType != AVM2ValueType.String)
			{
				double fA = primitiveA.AsNumber(avm2);
				double fB = primitiveB.AsNumber(avm2);

				if (double.IsNaN(fA) || double.IsNaN(fB))
				{
					return AVM2AbstractRelationalCompareResult.Undefined;
				}

				// All other cases appear to be handled by standard < comparison of doubles.
				return (fA < fB) ? AVM2AbstractRelationalCompareResult.True : AVM2AbstractRelationalCompareResult.False;
			}
			else
			{
				// Standard string CompareTo appears to handle rules for string ordering.
				string sA = primitiveA.AsString(avm2);
				string sB = primitiveB.AsString(avm2);

				return (string.Compare(sA, sB) < 0) ? AVM2AbstractRelationalCompareResult.True : AVM2AbstractRelationalCompareResult.False;
			}
		}

		// ECMAScript Language Specification 11.9.6: http://www.ecma-international.org/ecma-262/5.1/#sec-11.9.6
		public static bool StrictEqualityCompare(AVM2 avm2, AVM2Value a, AVM2Value b)
		{
			if (a.m_eType != b.m_eType)
			{
				return false;
			}

			switch (a.m_eType)
			{
				case AVM2ValueType.Boolean: return (a.AsBoolean(avm2) == b.AsBoolean(avm2));
				case AVM2ValueType.Null: return true;
				case AVM2ValueType.Number: return (a.AsNumber(avm2) == b.AsNumber(avm2));
				case AVM2ValueType.Object: return (a.m_Value == b.m_Value);
				case AVM2ValueType.String: return (a.AsString(avm2) == b.AsString(avm2));
				case AVM2ValueType.Undefined: return true;
				default:
					throw new ArgumentException("Unknown type " + a.m_eType.ToString());
			};
		}

		// ECMAScript Language Specific 9.2: http://www.ecma-international.org/ecma-262/5.1/#sec-9.2
		public bool AsBoolean(AVM2 avm2)
		{
			switch (m_eType)
			{
				case AVM2ValueType.Boolean: return (bool)m_Value;
				case AVM2ValueType.Null: return false;
				case AVM2ValueType.Number:
					{
						double fValue = (double)m_Value;
						if (0.0 == fValue || double.IsNaN(fValue))
						{
							return false;
						}
						else
						{
							return true;
						}
					}
				case AVM2ValueType.Object: return true;
				case AVM2ValueType.String: return !string.IsNullOrEmpty((string)m_Value);
				case AVM2ValueType.Undefined: return false;
				default:
					throw new InvalidCastException(m_eType.ToString() + " cannot be cast to a boolean.");
			}
		}

		// ECMAScript Language Specification 9.5: http://www.ecma-international.org/ecma-262/5.1/#sec-9.5
		public Int32 AsInt32(AVM2 avm2)
		{
			// Conversion rules appear to be the same as .NET.
			return (Int32)AsNumber(avm2);
		}

		// ECMAScript Language Specification 9.4: http://www.ecma-international.org/ecma-262/5.1/#sec-9.4
		public Int64 AsInt64(AVM2 avm2)
		{
			// Conversion rules appear to be the same as .NET.
			return (Int64)AsNumber(avm2);
		}

		// ECMAScript Language Specification 9.3: http://www.ecma-international.org/ecma-262/5.1/#sec-9.3
		public double AsNumber(AVM2 avm2)
		{
			switch (m_eType)
			{
				case AVM2ValueType.Boolean: return (((bool)m_Value) ? 1 : 0);
				case AVM2ValueType.Null: return 0;
				case AVM2ValueType.Number: return (double)m_Value;
				case AVM2ValueType.Object: return ((AVM2Object)m_Value).AsPrimitive(avm2, false).AsNumber(avm2);
				case AVM2ValueType.String: return double.Parse(AsString(avm2));
				case AVM2ValueType.Undefined: return double.NaN;
				default:
					throw new InvalidCastException(m_eType.ToString() + " cannot be cast to a number.");
			};
		}

		public bool TryAsNumber(AVM2 avm2, out double f)
		{
			switch (m_eType)
			{
				case AVM2ValueType.Boolean: f = (((bool)m_Value) ? 1 : 0); return true;
				case AVM2ValueType.Null: f = 0; return true;
				case AVM2ValueType.Number: f = (double)m_Value; return true;
				case AVM2ValueType.Object: return ((AVM2Object)m_Value).AsPrimitive(avm2, false).TryAsNumber(avm2, out f);
				case AVM2ValueType.String: return double.TryParse(AsString(avm2), NumberStyles.Any, CultureInfo.InvariantCulture, out f);
				case AVM2ValueType.Undefined: f = double.NaN; return true;
				default:
					throw new InvalidCastException(m_eType.ToString() + " cannot be cast to a number.");
			};
		}

		// ECMAScript Language Specification 9.9: http://www.ecma-international.org/ecma-262/5.1/#sec-9.9
		public AVM2Object AsObject(AVM2 avm2)
		{
			switch (m_eType)
			{
				case AVM2ValueType.Boolean: return new AVM2BooleanObject(avm2.BooleanClassObject, (bool)m_Value);
				case AVM2ValueType.Null: throw new AVM2TypeError();
				case AVM2ValueType.Number: return new AVM2NumberObject(avm2.NumberClassObject, (double)m_Value);
				case AVM2ValueType.Object: return ((AVM2Object)m_Value);
				case AVM2ValueType.String: return new AVM2StringObject(avm2.StringClassObject, (string)m_Value);
				case AVM2ValueType.Undefined: throw new AVM2TypeError();
				default:
					throw new InvalidCastException(m_eType.ToString() + " cannot be cast to an object.");
			};
		}

		// ECMAScript Language Specification 9.1: http://www.ecma-international.org/ecma-262/5.1/#sec-9.1
		public AVM2Value AsPrimitive(AVM2 avm2, bool bHintString)
		{
			switch (m_eType)
			{
				case AVM2ValueType.Boolean: return this;
				case AVM2ValueType.Null: return this;
				case AVM2ValueType.Number: return this;
				case AVM2ValueType.Object: return ((AVM2Object)m_Value).AsPrimitive(avm2, bHintString);
				case AVM2ValueType.String: return this;
				case AVM2ValueType.Undefined: return this;
				default:
					throw new InvalidCastException(m_eType.ToString() + " cannot be cast to a primitive.");
			};
		}

		// ECMAScript Language Specification 9.8: http://www.ecma-international.org/ecma-262/5.1/#sec-9.8
		public string AsString(AVM2 avm2)
		{
			switch (m_eType)
			{
				case AVM2ValueType.Boolean: return ((bool)m_Value) ? "true" : "false";
				case AVM2ValueType.Null: return "null";
				case AVM2ValueType.Number: return ((double)m_Value).ToString();
				case AVM2ValueType.Object: return ((AVM2Object)m_Value).AsPrimitive(avm2, true).AsString(avm2);
				case AVM2ValueType.String: return ((string)m_Value);
				case AVM2ValueType.Undefined: return "undefined";
				default:
					throw new InvalidCastException(m_eType.ToString() + " cannot be cast to a string.");
			};
		}

		// ECMAScript Language Specification 9.7: http://www.ecma-international.org/ecma-262/5.1/#sec-9.7
		public UInt16 AsUInt16(AVM2 avm2)
		{
			// Conversion rules appear to be the same as .NET.
			return (UInt16)AsNumber(avm2);
		}

		// ECMAScript Language Specification 9.6: http://www.ecma-international.org/ecma-262/5.1/#sec-9.6
		public UInt32 AsUInt32(AVM2 avm2)
		{
			// Conversion rules appear to be the same as .NET.
			return (UInt32)AsNumber(avm2);
		}

		public bool IsPrimitive
		{
			get
			{
				return (m_eType != AVM2ValueType.Object);
			}
		}

		public override string ToString()
		{
			switch (m_eType)
			{
				case AVM2ValueType.Boolean:
				case AVM2ValueType.Null:
				case AVM2ValueType.Number:
				case AVM2ValueType.String:
					return m_Value.ToString();
				case AVM2ValueType.Object:
					return "object";
				case AVM2ValueType.Undefined:
					return "undefined";
				default:
					throw new AVM2TypeError("Unknown value type '" + m_eType.ToString() + "'.");
			};
		}

		public AVM2ValueType Type
		{
			get
			{
				return m_eType;
			}
		}

		public string TypeName
		{
			get
			{
				switch (m_eType)
				{
					case AVM2ValueType.Boolean: return "Boolean";
					case AVM2ValueType.Null: return "object";
					case AVM2ValueType.Number: return "number";
					case AVM2ValueType.Object: return "object";
					case AVM2ValueType.String: return "string";
					case AVM2ValueType.Undefined: return "undefined";
					default:
						throw new InvalidCastException(m_eType.ToString() + " cannot be cast to a string.");
				};
			}
		}
	}

	public sealed class AVM2
	{
		#region Private members
		IAVM2ClassObject m_ObjectClassObject = null;
		IAVM2ClassObject m_ClassClassObject = null;
		IAVM2ClassObject m_FunctionClassObject = null;
		IAVM2ClassObject m_ArrayClassObject = null;
		IAVM2ClassObject m_BooleanClassObject = null;
		IAVM2ClassObject m_NumberClassObject = null;
		IAVM2ClassObject m_StringClassObject = null;
		IAVM2ClassObject m_MovieClipClassObject = null;
		ABCFile m_File = null;
		AVM2Value m_Global = default(AVM2Value);

		AVM2Object AddGlobal(string sName, AVM2Object oObject)
		{
			m_Global.AsObject(this).SetProperty(this, sName, new AVM2Value(oObject));
			return oObject;
		}

		AVM2Value AVM2ArrayObjectNativeClassCallDelegate(AVM2 avm2, IAVM2ClassObject classObject, AVM2Object oThis, AVM2Value[] aArguments)
		{
			AVM2ArrayObject ret = new AVM2ArrayObject(classObject, null == aArguments ? 0 : aArguments.Length);
			for (int i = 0; i < aArguments.Length; ++i)
			{
				ret.Array[i] = aArguments[i];
			}
			return new AVM2Value(ret);
		}

		AVM2Value AVM2BooleanObjectNativeClassCallDelegate(AVM2 avm2, IAVM2ClassObject classObject, AVM2Object oThis, AVM2Value[] aArguments)
		{
			bool bValue = (aArguments != null && aArguments.Length > 0 ? aArguments[0].AsBoolean(this) : false);
			AVM2BooleanObject ret = new AVM2BooleanObject(classObject, bValue);
			return new AVM2Value(ret);
		}

		AVM2Value AVM2EventNativeClassCallDelegate(AVM2 avm2, IAVM2ClassObject classObject, AVM2Object oThis, AVM2Value[] aArguments)
		{
			string sName = aArguments[0].AsString(avm2);
			bool bBubbles = (aArguments.Length < 2 ? false : aArguments[1].AsBoolean(avm2));
			AVM2EventObject ret = new AVM2EventObject(classObject, sName, bBubbles);
			return new AVM2Value(ret);
		}

		AVM2Value AVM2MethodObjectNativeClassCallDelegate(AVM2 avm2, IAVM2ClassObject classObject, AVM2Object oThis, AVM2Value[] aArguments)
		{
			AVM2MethodObject ret = new AVM2MethodObject(
				classObject,
				this,
				new List<AVM2Value>(),
				File.Methods[aArguments[0].AsInt32(this)].m_Body);
			return new AVM2Value(ret);
		}

		AVM2Value AVM2MovieClipObjectNativeClassCallDelegate(AVM2 avm2, IAVM2ClassObject classObject, AVM2Object oThis, AVM2Value[] aArguments)
		{
			AVM2Object ret = new AVM2MovieClipObject(classObject);
			return new AVM2Value(ret);
		}

		AVM2Value AVM2NumberObjectNativeClassCallDelegate(AVM2 avm2, IAVM2ClassObject classObject, AVM2Object oThis, AVM2Value[] aArguments)
		{
			double fValue = (aArguments != null && aArguments.Length > 0 ? aArguments[0].AsNumber(this) : 0.0f);
			AVM2NumberObject ret = new AVM2NumberObject(classObject, fValue);
			return new AVM2Value(ret);
		}

		AVM2Value AVM2ObjectNativeClassCallDelegate(AVM2 avm2, IAVM2ClassObject classObject, AVM2Object oThis, AVM2Value[] aArguments)
		{
			AVM2Object ret = new AVM2Object(classObject);
			return new AVM2Value(ret);
		}

		AVM2Value AVM2StringObjectNativeClassCallDelegate(AVM2 avm2, IAVM2ClassObject classObject, AVM2Object oThis, AVM2Value[] aArguments)
		{
			string sValue = (aArguments != null && aArguments.Length > 0 ? aArguments[0].AsString(this) : string.Empty);
			AVM2StringObject ret = new AVM2StringObject(classObject, sValue);
			return new AVM2Value(ret);
		}

		#region ActionScript 3 globals
		static internal AVM2Value getTimer(AVM2 avm2, AVM2Object oThis, AVM2Value[] aArguments)
		{
			return new AVM2Value(((double)System.Diagnostics.Stopwatch.GetTimestamp() / (double)System.Diagnostics.Stopwatch.Frequency) * 1000.0);
		}
		#endregion
		#endregion

		public AVM2(ABCFile file)
		{
			m_ObjectClassObject = new AVM2NativeClassObject(null, null, AVM2ObjectNativeClassCallDelegate);
			m_File = file;
			m_Global = new AVM2Value(new AVM2Object(m_ObjectClassObject));

			List<AVM2Value> scopeStack = new List<AVM2Value>();
			AddGlobal("Object", (AVM2Object)m_ObjectClassObject);
			m_ClassClassObject = (IAVM2ClassObject)AddGlobal("Class", new AVM2NativeClassObject(null, m_ObjectClassObject, AVM2ObjectNativeClassCallDelegate));
			m_FunctionClassObject = (IAVM2ClassObject)AddGlobal("Function", new AVM2NativeClassObject(m_ClassClassObject, m_ObjectClassObject, AVM2MethodObjectNativeClassCallDelegate));
			m_ArrayClassObject = (IAVM2ClassObject)(
				AddGlobal("Array", new AVM2NativeClassObject(m_ClassClassObject, m_ObjectClassObject, AVM2ArrayObjectNativeClassCallDelegate))
					.SetGetter(this, "length", new AVM2NativeMethodObject(m_FunctionClassObject, AVM2ArrayObject.getLength))
					.SetSetter(this, "length", new AVM2NativeMethodObject(m_FunctionClassObject, AVM2ArrayObject.setLength)));
			IAVM2ClassObject dictionaryClass = (IAVM2ClassObject)AddGlobal("Dictionary", new AVM2NativeClassObject(m_ClassClassObject, m_ObjectClassObject, AVM2ObjectNativeClassCallDelegate));

			AddGlobal("getTimer", new AVM2NativeMethodObject(m_FunctionClassObject, getTimer));

			m_BooleanClassObject = (IAVM2ClassObject)AddGlobal("Boolean", new AVM2NativeClassObject(m_ClassClassObject, m_ObjectClassObject, AVM2BooleanObjectNativeClassCallDelegate));
			m_NumberClassObject = (IAVM2ClassObject)AddGlobal("Number", new AVM2NativeClassObject(m_ClassClassObject, m_ObjectClassObject, AVM2NumberObjectNativeClassCallDelegate));
			IAVM2ClassObject intClassObject = (IAVM2ClassObject)AddGlobal("int", new AVM2NativeClassObject(m_ClassClassObject, m_ObjectClassObject, AVM2NumberObjectNativeClassCallDelegate));
			m_StringClassObject = (IAVM2ClassObject)AddGlobal("String", new AVM2NativeClassObject(m_ClassClassObject, m_ObjectClassObject, AVM2StringObjectNativeClassCallDelegate));

			IAVM2ClassObject eventClass = (IAVM2ClassObject)AddGlobal("Event", new AVM2NativeClassObject(m_ClassClassObject, m_ObjectClassObject, AVM2EventNativeClassCallDelegate));
			AddGlobal("MouseEvent", new AVM2NativeClassObject(m_ClassClassObject, eventClass, AVM2ObjectNativeClassCallDelegate))
				.SetProperty(this, "MOUSE_DOWN", new AVM2Value("mouseDown"))
				.SetProperty(this, "MOUSE_OUT", new AVM2Value("mouseOut"))
				.SetProperty(this, "MOUSE_OVER", new AVM2Value("mouseOver"));
			AddGlobal("TimerEvent", new AVM2NativeClassObject(m_ClassClassObject, eventClass, AVM2ObjectNativeClassCallDelegate));

			IAVM2ClassObject bitmapDataClass = (IAVM2ClassObject)AddGlobal("BitmapData", new AVM2NativeClassObject(m_ClassClassObject, m_ObjectClassObject, AVM2ObjectNativeClassCallDelegate));
			IAVM2ClassObject fontClass = (IAVM2ClassObject)AddGlobal("Font", new AVM2NativeClassObject(m_ClassClassObject, m_ObjectClassObject, AVM2ObjectNativeClassCallDelegate));

			IAVM2ClassObject pointClass = (IAVM2ClassObject)AddGlobal("Point", new AVM2NativeClassObject(m_ClassClassObject, m_ObjectClassObject, AVM2ObjectNativeClassCallDelegate));
			IAVM2ClassObject rectangleClass = (IAVM2ClassObject)AddGlobal("Rectangle", new AVM2NativeClassObject(m_ClassClassObject, m_ObjectClassObject, AVM2ObjectNativeClassCallDelegate));

			IAVM2ClassObject eventDispatcherClass = (IAVM2ClassObject)AddGlobal("EventDispatcher", new AVM2NativeClassObject(m_ClassClassObject, m_ObjectClassObject, AVM2ObjectNativeClassCallDelegate))
				.SetProperty(this, "addEventListener", new AVM2Value(new AVM2NativeMethodObject(m_FunctionClassObject, AVM2MovieClipObject.addEventListener)))
				.SetProperty(this, "dispatchEvent", new AVM2Value(new AVM2NativeMethodObject(m_FunctionClassObject, AVM2MovieClipObject.dispatchEvent)));
			IAVM2ClassObject displayObjectClass = (IAVM2ClassObject)AddGlobal("DisplayObject", new AVM2NativeClassObject(m_ClassClassObject, eventDispatcherClass, AVM2ObjectNativeClassCallDelegate))
				.SetProperty(this, "name", new AVM2Value(new AVM2StringObject(m_StringClassObject, string.Empty)));
			IAVM2ClassObject timerClass = (IAVM2ClassObject)AddGlobal("Timer", new AVM2NativeClassObject(m_ClassClassObject, eventDispatcherClass, AVM2ObjectNativeClassCallDelegate));
			IAVM2ClassObject interactiveObjectClass = (IAVM2ClassObject)AddGlobal("InteractiveObject", new AVM2NativeClassObject(m_ClassClassObject, displayObjectClass, AVM2ObjectNativeClassCallDelegate));
			IAVM2ClassObject displayObjectContainerClass = (IAVM2ClassObject)AddGlobal("DisplayObjectContainer", new AVM2NativeClassObject(m_ClassClassObject, interactiveObjectClass, AVM2ObjectNativeClassCallDelegate));
			IAVM2ClassObject spriteClass = (IAVM2ClassObject)AddGlobal("Sprite", new AVM2NativeClassObject(m_ClassClassObject, displayObjectContainerClass, AVM2ObjectNativeClassCallDelegate));
			m_MovieClipClassObject = (IAVM2ClassObject)(
				AddGlobal("MovieClip", new AVM2NativeClassObject(m_ClassClassObject, spriteClass, AVM2MovieClipObjectNativeClassCallDelegate))
					.SetProperty(this, "addFrameScript", new AVM2Value(new AVM2NativeMethodObject(m_FunctionClassObject, AVM2MovieClipObject.addFrameScript)))
					.SetProperty(this, "currentFrame", new AVM2Value(new AVM2NumberObject(m_NumberClassObject, 0.0)))
					.SetProperty(this, "getChildByName", new AVM2Value(new AVM2NativeMethodObject(m_FunctionClassObject, AVM2MovieClipObject.getChildByName)))
					.SetProperty(this, "gotoAndStop", new AVM2Value(new AVM2NativeMethodObject(m_FunctionClassObject, AVM2MovieClipObject.gotoAndStop)))
					.SetProperty(this, "gotoAndPlay", new AVM2Value(new AVM2NativeMethodObject(m_FunctionClassObject, AVM2MovieClipObject.gotoAndPlay)))
					.SetProperty(this, "stop", new AVM2Value(new AVM2NativeMethodObject(m_FunctionClassObject, AVM2MovieClipObject.stop)))
					.SetProperty(this, "visible", new AVM2Value(new AVM2BooleanObject(m_BooleanClassObject, true))));
			IAVM2ClassObject textFieldClass = (IAVM2ClassObject)AddGlobal("TextField", new AVM2NativeClassObject(m_ClassClassObject, interactiveObjectClass, AVM2ObjectNativeClassCallDelegate));

			AVM2MethodObject globalInitializer = new AVM2MethodObject(
				m_FunctionClassObject,
				this,
				scopeStack,
				m_File.Methods[m_File.Scripts[m_File.Scripts.Length - 1].m_iScriptInitializer].m_Body);
			globalInitializer.Call(this, Global.AsObject(this), null);
		}

		public IAVM2ClassObject ArrayClassObject
		{
			get
			{
				return m_ArrayClassObject;
			}
		}

		public IAVM2ClassObject BooleanClassObject
		{
			get
			{
				return m_BooleanClassObject;
			}
		}

		public IAVM2ClassObject ClassClassObject
		{
			get
			{
				return m_ClassClassObject;
			}
		}

		public AVM2Value CreateInstance(string sClassName)
		{
			AVM2Value classObject = GetGlobalObjectLazy(sClassName);

			AVM2Value instanceObject = classObject.AsObject(this).Call(this, Global.AsObject(this), null);

			return instanceObject;
		}

		public ABCFile File
		{
			get
			{
				return m_File;
			}
		}

		public void ApplyTraits(AVM2Object oObject, TraitsInfo[] aTraits)
		{
			if (null == aTraits)
			{
				return;
			}

			foreach (TraitsInfo traitsInfo in aTraits)
			{
				string sPropertyName = File.ConstantPool.GetMultiname(null, traitsInfo.m_iName, null);
				switch (traitsInfo.m_eKind)
				{
					case TraitsInfoKind.Getter:
						{
							TraitsInfoData_Method methodInfo = (TraitsInfoData_Method)traitsInfo.m_Data;
							oObject.SetGetter(this, sPropertyName, new AVM2MethodObject(
								m_FunctionClassObject,
								this,
								new List<AVM2Value>(),
								File.Methods[methodInfo.m_iMethodIndex].m_Body));
						}
						break;
					case TraitsInfoKind.Method:
						{
							TraitsInfoData_Method methodInfo = (TraitsInfoData_Method)traitsInfo.m_Data;
							oObject.SetProperty(this, sPropertyName, new AVM2Value(new AVM2MethodObject(
								m_FunctionClassObject,
								this,
								new List<AVM2Value>(),
								File.Methods[methodInfo.m_iMethodIndex].m_Body)));
						}
						break;
					case TraitsInfoKind.Setter:
						{
							TraitsInfoData_Method methodInfo = (TraitsInfoData_Method)traitsInfo.m_Data;
							oObject.SetSetter(this, sPropertyName, new AVM2MethodObject(
								m_FunctionClassObject,
								this,
								new List<AVM2Value>(),
								File.Methods[methodInfo.m_iMethodIndex].m_Body));
						}
						break;
					case TraitsInfoKind.Slot: // fall-through
					case TraitsInfoKind.Const:
						{
							TraitsInfoData_Slot slotInfo = (TraitsInfoData_Slot)traitsInfo.m_Data;
							AVM2Value propertyValue = default(AVM2Value);
							switch (slotInfo.m_eKind)
							{
								case ConstantKind.Double:
									propertyValue = new AVM2Value(File.ConstantPool.m_aDoubles[slotInfo.m_iVIndex]);
									break;
								case ConstantKind.False:
									propertyValue = new AVM2Value(false);
									break;
								case ConstantKind.Int:
									propertyValue = new AVM2Value((double)File.ConstantPool.m_aIntegers[slotInfo.m_iVIndex]);
									break;
								case ConstantKind.Undefined:
									propertyValue = default(AVM2Value);
									break;
								case ConstantKind.Null:
									propertyValue = new AVM2Value(null);
									break;
								case ConstantKind.True:
									propertyValue = new AVM2Value(true);
									break;
								case ConstantKind.UInt:
									propertyValue = new AVM2Value((double)File.ConstantPool.m_aUnsignedIntegers[slotInfo.m_iVIndex]);
									break;
								case ConstantKind.Utf8:
									propertyValue = new AVM2Value(File.ConstantPool.m_aStrings[slotInfo.m_iVIndex]);
									break;
								default:
									throw new AVM2TypeError("Unsupported constant kind '" + slotInfo.m_eKind.ToString() + "'.");
							};
							oObject.SetProperty(this, sPropertyName, propertyValue);
						}
						break;
					default:
						throw new AVM2TypeError("Unsupported traits kind '" + traitsInfo.m_eKind.ToString() + "'.");
				};
			}
		}

		public IAVM2ClassObject FunctionClassObject
		{
			get
			{
				return m_FunctionClassObject;
			}
		}

		public AVM2Value GetGlobalObjectLazy(string sName)
		{
			AVM2Object oGlobal = Global.AsObject(this);
			if (oGlobal.CanGetProperty(this, sName))
			{
				return oGlobal.GetProperty(this, sName);
			}

			foreach (ScriptInfo scriptInfo in m_File.Scripts)
			{
				foreach (TraitsInfo traitsInfo in scriptInfo.m_aTraits)
				{
					string sTraitName = m_File.ConstantPool.GetMultiname(null, traitsInfo.m_iName, null);
					if (sTraitName == sName)
					{
						AVM2Value value = default(AVM2Value);
						switch (traitsInfo.m_eKind)
						{
							case TraitsInfoKind.Class:
								{
									TraitsInfoData_Class data = (TraitsInfoData_Class)traitsInfo.m_Data;
									ClassInfo classInfo = m_File.Classes[data.m_iClassIndex];
									InstanceInfo instanceInfo = m_File.Instances[data.m_iClassIndex];

									string sParentName = m_File.ConstantPool.GetMultiname(null, instanceInfo.m_iSuperName, null);
									IAVM2ClassObject parentClassObject = (IAVM2ClassObject)GetGlobalObjectLazy(sParentName).AsObject(this);
									AVM2ClassObject classObject = new AVM2ClassObject(
										m_ClassClassObject,
										parentClassObject,
										this,
										classInfo,
										instanceInfo);
									value = new AVM2Value(classObject);
								}
								break;
							default:
								value = default(AVM2Value);
								break;
						}

						oGlobal.SetProperty(this, sTraitName, value);
						return value;
					}
				}
			}

			return default(AVM2Value);
		}

		public AVM2Value Global
		{
			get
			{
				return m_Global;
			}
		}

		public IAVM2ClassObject NumberClassObject
		{
			get
			{
				return m_NumberClassObject;
			}
		}

		public IAVM2ClassObject ObjectClassObject
		{
			get
			{
				return m_ObjectClassObject;
			}
		}

		public IAVM2ClassObject StringClassObject
		{
			get
			{
				return m_StringClassObject;
			}
		}
	}
} // namespace FalconCooker
