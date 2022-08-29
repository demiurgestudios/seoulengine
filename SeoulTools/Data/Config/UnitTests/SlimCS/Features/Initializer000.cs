using static SlimCS;

public static class Test
{
	public class A
	{
		public int m_0;
		public double? m_1;
		public int m_2;
		public int? m_3;
		public bool m_4;
	}

	// This test is a regression for initializer lists that contain null.
	public static void Main()
	{
		var a = new A()
		{
			m_0 = 1,
			m_1 = null,
			m_2 = 3,
			m_3 = null,
			m_4 = true,
		};
		print(a.m_0, a.m_1, a.m_2, a.m_3, a.m_4);
	}
}
