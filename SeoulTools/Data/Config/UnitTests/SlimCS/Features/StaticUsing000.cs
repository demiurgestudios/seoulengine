using static SlimCS;
using static A.B.X;

namespace A.B
{
	static class X
	{
		public static int Test ()
		{
			print(5);
			return 5;
		}
	}
}

namespace C
{
	class Test
	{
		public static int Main ()
		{
			if (Test () != 5)
				return 1;

			return 0;
		}
	}
}
