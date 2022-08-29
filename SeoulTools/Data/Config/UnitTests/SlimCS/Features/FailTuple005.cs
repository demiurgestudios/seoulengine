using static SlimCS;
using System;

public static class Test
{
	(double, double) m_a = (1.0, 2.0);

	public static void Main()
	{
		print(m_a.Item1, m_a.Item2);
	}
}
