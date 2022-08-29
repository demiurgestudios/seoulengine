using System.Diagnostics.Contracts;

using static SlimCS;

public static class Test
{
	class Inner
	{
		double m_f = 2.0;
		public void Set()
		{
			m_f = 1.0;
			print(m_f);
		}
	}

	static Inner s_Inner = new Inner();

	// Pure method cannot call impure methods on members.
	[Pure]
	public static double Pure
	{
		[Pure]
		get { s_Inner.Set(); return 0.0; }
	}

	public static void Main()
	{
		print("PURE TEST");
	}
}
