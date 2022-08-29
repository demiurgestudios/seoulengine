using static SlimCS;

class Test
{
	public static class A
	{
		public static readonly string s_t = B.GetString();
	}

	public static class B
	{
		static readonly string s_s;
		static B()
		{
			s_s = "ThisIsB";
		}

		public static string GetString()
		{
			return s_s;
		}
	}

	public static int Main ()
	{
		print(A.s_t);
		print(B.GetString());

		return 0;
	}
}
