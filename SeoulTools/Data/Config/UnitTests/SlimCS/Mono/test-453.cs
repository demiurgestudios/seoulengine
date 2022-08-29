using System;

class C {
	internal enum Flags {
		Removed	= 0,
		Public	= 1
	}

	static Flags	_enumFlags;

	public static void Main()
	{
		if (((int)(Flags.Removed | 0)).ToString () != "0")
			throw new Exception ();
	}
}

