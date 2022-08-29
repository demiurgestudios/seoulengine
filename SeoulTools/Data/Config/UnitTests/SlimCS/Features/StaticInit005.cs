using static SlimCS;

class Test
{
	public static class C
	{
		static string s_s { get; }
		static C()
		{
			s_s = "ThisIsC";
		}

		public static string GetString()
		{
			return s_s;
		}
	}

	public static class A
	{
		public static string s_t0 { get; } = B.GetString(); public static string s_t1 { get; } = "Hello World"; public static string s_t2 { get; } = C.GetString();
	}

	public static class B
	{
		static string s_s { get; }
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
		print(A.s_t0);
		print(A.s_t1);
		print(A.s_t2);
		print(B.GetString());
		print(C.GetString());

		return 0;
	}
}
