using static SlimCS;
using System;
using System.Threading.Tasks;

// A Fail test (any test with a filename starting with Fail)
// tests that functionality which has been specifically disabled
// in SlimCS triggers a compiler failure.
public static class Test
{
	class A { void HiThere() { print("Hi There."); } }
	A m_a = new A();
	public static void Main()
	{
		// Lock blocks are not supported.
		lock (m_a)
		{
			m_a.HiThere();
		}
	}
}
