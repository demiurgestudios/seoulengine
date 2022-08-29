// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using System.Collections.Generic;

namespace SlimCSLib.Generator
{
	public static class Constants
	{
		// Tuned via experimentation - not sure what's up. If we
		// go above this value, it slows the entire process down.
		// May be some internal limitation in Roslyn (maybe
		// there's a lock in there and we just hit it harder with
		// more threads?). Or a bug in how we're using Parallel.*
		public const int kMaxParallelism = 16;

		// Used as placeholders in lookup for switch case resolution.
		public static readonly object kDefaultSentinel = new object();
		public static readonly object kNullSentinel = new object();

		// Used to indicate that a node is expected to mismatch
		// the line it's being emitted at. Used for nodes
		// that the Visitor creates as placeholders or
		// remaps (e.g. to handle named parameters).
		public static readonly SyntaxAnnotation kLineMismatchAllowed = new SyntaxAnnotation();

		// For wrangling differences between methods and accessors.
		public static readonly SeparatedSyntaxList<ParameterSyntax> kEmptyParameters = new SeparatedSyntaxList<ParameterSyntax>();

		// True and false fixed value.
		public static readonly LiteralExpressionSyntax kFalseExpression = SyntaxFactory.LiteralExpression(SyntaxKind.FalseLiteralExpression).WithAdditionalAnnotations(kLineMismatchAllowed);
		public static readonly LiteralExpressionSyntax kTrueExpression = SyntaxFactory.LiteralExpression(SyntaxKind.TrueLiteralExpression).WithAdditionalAnnotations(kLineMismatchAllowed);

		// In a few contexts, we need to add a 'self', which is a 'this' expression.
		public static readonly ThisExpressionSyntax kThisExpression = SyntaxFactory.ThisExpression().WithAdditionalAnnotations(kLineMismatchAllowed);

		public const string kCsharpChar = "char";
		public const string kCsharpCtor = ".ctor";
		public const string kCsharpDiscard = "_";
		public const string kCsharpDouble = "double";
		public const string kCsharpIndexer = "this[]";
		public const string kCsharpInt = "int";
		public const string kCsharpItem = "Item";
		public const string kCsharpNullReferenceException = "NullReferenceException";
		public const string kCsharpPropertyAddPrefix = "add_";
		public const string kCsharpPropertyGetPrefix = "get_";
		public const string kCsharpPropertyRemovePrefix = "remove_";
		public const string kCsharpPropertySetPrefix = "set_";
		public const string kCsharpValueArg = "value";

		public const string kBuiltinStringImplementations = "SlimCSStringLib";
		public const string kStringIsNullOrEmpty = "IsNullOrEmpty";

		public const string kLuaAnd = "and";
		public const string kLuaArrayCreate = "arraycreate";
		public const string kLuaAs = "as";
		public const string kLuaBaseConstructorName = "Constructor";
		public const string kLuaBaseStaticConstructorName = "cctor";
		public const string kLuaBband = "__bband__";
		public const string kLuaBbor = "__bbor__";
		public const string kLuaBbxor = "__bbxor__";
		public const string kLuaBindDelegate = "__bind_delegate__";
		public const string kLuaBitAnd = "bit.band";
		public const string kLuaBitArshift = "bit.arshift";
		public const string kLuaBitLshift = "bit.lshift";
		public const string kLuaBitNot = "bit.bnot";
		public const string kLuaBitOr = "bit.bor";
		public const string kLuaBitRshift = "bit.rshift";
		public const string kLuaBitXor = "bit.bxor";
		public const string kLuaBitToBit = "bit.tobit";
		public const string kLuaBreak = "break";
		public const string kLuaCast = "__cast__";
		public const string kLuaCastInt = "__castint__";
		public const string kLuaClass = "class";
		public const string kLuaClassStatic = "class_static";
		public const string kLuaConcat = "..";
		public const string kLuaDelegate = "Delegate";
		public const string kLuaArrReadPlaceholder = "__arr_read_placeholder";
		public const string kLuaDefaultValueSpecial = "__default_value";
		public const string kLuaDo = "do";
		public const string kLuaDyncast = "dyncast";
		public const string kLuaDval = "dval";
		public const string kLuaElse = "else";
		public const string kLuaElseif = "elseif";
		public const string kLuaEnd = "end";
		public const string kLuaEqual = "==";
		public const string kLuaError = "error";
		public const string kLuaFalse = "false";
		public const string kLuaFinalizer = "__gc";
		public const string kLuaFor = "for";
		public const string kLuaFunction = "function";
		public const string kLuaGenericTypeLookup = "genericlookup";
		public const string kLuaGetType = "GetType";
		public const string kLuaGoto = "goto";
		public const string kLuaI32Mod = "__i32mod__";
		public const string kLuaI32Mul = "__i32mul__";
		public const string kLuaI32Narrow = "__i32narrow__";
		public const string kLuaI32Truncate = "__i32truncate__";
		public const string kLuaIf = "if";
		public const string kLuaInitArr = "initarr";
		public const string kLuaInitList = "initlist";
		public const string kLuaIn = "in";
		public const string kLuaInterface = "interface";
		public const string kLuaIpairs = "ipairs";
		public const string kLuaIs = "is";
		public const string kLuaLocal = "local";
		public const string kLuaMathFmod = "math.fmod";
		public const string kLuaNewDefaultMethod = "New";
		public const string kLuaNewOverrideMethod = "ONew";
		public const string kLuaNil = "nil";
		public const string kLuaNot = "not";
		public const string kLuaNotEqual = "~=";
		public const string kLuaOnInitProgress = "__oninitprogress__";
		public const string kLuaOr = "or";
		public const string kLuaPairs = "pairs";
		public const string kLuaRepeat = "repeat";
		public const string kLuaRequire = "require";
		public const string kLuaReturn = "return";
		public const string kLuaSelf = "self";
		public const string kLuaStringAlign = "string.align";
		public const string kLuaStringGlobal = "String";
		public const string kLuaStringLength = "len";
		public const string kLuaTableUnpack = "table.unpack";
		public const string kLuaThen = "then";
		public const string kLuaTostring = "tostring";
		public const string kLuaTrue = "true";
		public const string kLuaTypeOf = "TypeOf";
		public const string kLuaUntil = "until";
		public const string kLuaWhile = "while";
		public const string kPseudoTuple = "Tuple";
		public const string kPseudoLuaTableTypeName = "Table";
		public const string kPseudoLuaTableVTypeName = "TableV";
		public const string kPseudoTry = "try";
		public const string kPseudoTryFinally = "tryfinally";
		public const string kPseudoUsing = "using";
		public const string kTryCatchFinallyUsingResVar = "res";
		public const string kTryCatchFinallyUsingRetVar = "ret";
		public const string kTryCatchFinallyUsingTestVar = "e";
		public const string kTryCatchFinallyUsingBreakCode = "0";
		public const string kTryCatchFinallyUsingContinueCode = "1";
		public const string kTryCatchFinallyUsingReturnCode = "2";

		public static readonly HashSet<string> kLuaReserved = new HashSet<string>()
		{
			"and",
			"break",
			"do",
			"else",
			"elseif",
			"end",
			"false",
			"for",
			"function",
			"if",
			"in",
			"local",
			"nil",
			"not",
			"or",
			"repeat",
			"return",
			"then",
			"true",
			"until",
			"while",
		};
	}
}
