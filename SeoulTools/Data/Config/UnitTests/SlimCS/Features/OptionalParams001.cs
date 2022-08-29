using static SlimCS;
public static class Test
{
	public class A
	{
		public int m_i = 0;
		public void TestIt(int a = 1, bool b = true, double f = 5.5)
		{
			print(m_i, a, b, f);
		}
	}

	public delegate void TestItDel(int a = 2, bool b = false, double f = -3.5);

	public static void Main()
	{
		var a = new A();
		TestItDel del = new TestItDel(a.TestIt);
		del();
		del(3);
		del(3, true);
		del(3, true, 8.8);

		a.m_i = 7;
		del();
		del(3);
		del(3, true);
		del(3, true, 8.8);
	}
}
