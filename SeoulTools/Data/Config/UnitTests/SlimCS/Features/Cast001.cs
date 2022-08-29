using static SlimCS;
using System;

class Test
{
	public static void Main ()
	{
		object o;
		o = false;
		print(o as bool?);
		print(o is bool?);
		print(o as double?);
		print(o is double?);
		// TODO: print(o as int?);
		// TODO: print(o is int?);
		print(o as string);
		print(o is string);

		o = 1.0;
		print(o as bool?);
		print(o is bool?);
		print(o as double?);
		print(o is double?);
		// TODO: print(o as int?);
		// TODO: print(o is int?);
		print(o as string);
		print(o is string);

		o = "Hello World";
		print(o as bool?);
		print(o is bool?);
		print(o as double?);
		print(o is double?);
		// TODO: print(o as int?);
		// TODO: print(o is int?);
		print(o as string);
		print(o is string);
	}
}
