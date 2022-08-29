//
// This test probes the various explicit unboxing casts
//
using System;

class X {
	static int cast_int (object o) { return (int) o; }
	static double cast_double (object o) { return (double) o; }

	public static int Main ()
	{
		if (cast_int ((object) -1) != -1)
			return 1;
		if (cast_int ((object) 1) != 1)
			return 2;
		if (cast_int ((object) Int32.MaxValue) != Int32.MaxValue)
			return 1;
		if (cast_int ((object) Int32.MinValue) != Int32.MinValue)
			return 2;
		if (cast_int ((object) 0) != 0)
			return 3;

		if (cast_double ((object) (double) -1) != -1)
			return 35;
		if (cast_double ((object) (double) 1) != 1)
			return 36;
		if (cast_double ((object) (double) Double.MaxValue) != Double.MaxValue)
			return 37;
		if (cast_double ((object) (double) Double.MinValue) != Double.MinValue)
			return 38;
		if (cast_double ((object) (double) 0) != 0)
			return 39;

		return 0;
	}
}
