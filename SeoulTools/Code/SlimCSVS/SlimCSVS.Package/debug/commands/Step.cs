// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.IO;
using Microsoft.VisualStudio.Debugger.Interop;

namespace SlimCSVS.Package.debug.commands
{
	/// <summary>
	/// Must be kept in-sync with equivalent value in the debugger client.
	/// </summary>
	enum StepKind
	{
		Into = 0,
		Over = 1,
		Out = 2,
		Backwards = 3,
	}

	/// <summary>
	/// Must be kept in-sync with equivalent value in the debugger client.
	/// </summary>
	enum StepUnit
	{
		Statement = 0,
		Line = 1,
		Instruction = 2,
	}

	sealed class Step : BaseCommand
	{
		#region Private members
		readonly StepKind m_eStepKind;
		readonly StepUnit m_eStepUnit;

		static StepKind Convert(enum_STEPKIND eStepKind)
		{
			switch (eStepKind)
			{
				case enum_STEPKIND.STEP_BACKWARDS: return StepKind.Backwards;
				case enum_STEPKIND.STEP_INTO: return StepKind.Into;
				case enum_STEPKIND.STEP_OUT: return StepKind.Out;
				case enum_STEPKIND.STEP_OVER: return StepKind.Over;
				default:
					throw new NotImplementedException($"Invalid enum value '{eStepKind}'.");
			}
		}

		static StepUnit Convert(enum_STEPUNIT eStepUnit)
		{
			switch (eStepUnit)
			{
				case enum_STEPUNIT.STEP_INSTRUCTION: return StepUnit.Instruction;
				case enum_STEPUNIT.STEP_LINE: return StepUnit.Line;
				case enum_STEPUNIT.STEP_STATEMENT: return StepUnit.Statement;
				default:
					throw new NotImplementedException($"Invalid enum value '{eStepUnit}'.");
			}
		}
		#endregion

		public Step(enum_STEPKIND eStepKind, enum_STEPUNIT eStepUnit)
			: base(Connection.ERPC.StepCommand)
		{
			m_eStepKind = Convert(eStepKind);
			m_eStepUnit = Convert(eStepUnit);
		}

		protected override void WriteCommand(BinaryWriter writer)
		{
			writer.NetWriteInt32((int)m_eStepKind);
			writer.NetWriteInt32((int)m_eStepUnit);
		}
	}
}
