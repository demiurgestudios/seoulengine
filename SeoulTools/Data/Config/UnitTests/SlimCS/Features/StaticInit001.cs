using static SlimCS;

class Test
{
	public static class C
	{
		static readonly string s_s;
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
		public static readonly string s_t0 = B.GetString(), s_t1 = "Hello World", s_t2 = C.GetString();
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
		print(A.s_t0);
		print(A.s_t1);
		print(A.s_t2);
		print(B.GetString());
		print(C.GetString());

		return 0;
	}
}
