using static SlimCS;

class Test
{
	public static class A
	{
		public static string s_t { get; } = B.GetString();
	}

	public static class B
	{
		static string s_s { get; set; }
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
