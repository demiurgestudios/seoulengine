using static SlimCS;
using System;
using System.Collections.Generic;
using SCG = System.Collections.Generic;
using SCGL = System.Collections.Generic.List<string>;

class A<T>
{
	public class B
	{
		public int Foo;
	}
}

class X
{
	bool field;
	long Prop { get; set; }
	event Action ev;

	public static int Main ()
	{
		int res;
		var x = new X ();
		res = x.SimpleName (1);
		if (res != 0)
			return res;

		res = x.MemberAccess ();
		if (res != 0)
			return 20 + res;

		res = x.QualifiedName ();
		if (res != 0)
			return 40 + res;

		return 0;
	}

	static void GenMethod<T, U, V> ()
	{
	}

	int SimpleName<T> (T arg)
	{
		const object c = null;

		print(nameof (T));
		print(nameof (arg));
		print(nameof (c));
		print(nameof (field));
		print(nameof (Prop));
		print(nameof (@Main));
		print(nameof (ev));
		print(nameof (Int32));
		print(nameof (Action));
		print(nameof (List<bool>));
		print(nameof (GenMethod));

		return 0;
	}

	int MemberAccess ()
	{
		print(nameof (X.field));
		print(nameof (X.Prop));
		print(nameof (Console.WriteLine));
		print(nameof (System.Collections.Generic.List<long>));
		print(nameof (System.Collections));
		print(nameof (X.GenMethod));
		print(nameof (A<char>.B));
		print(nameof (A<ushort>.B.Foo));

		return 0;
	}

	int QualifiedName ()
	{
		print(nameof (global::System.Int32));
		print(nameof (SCG.List<short>));
		print(nameof (SCGL.Contains));

		return 0;
	}
}
