using static SlimCS;
// A Fail test (any test with a filename starting with Fail)
// tests that functionality which has been specifically disabled
// in SlimCS triggers a compiler failure.
public static class Test
{
	public static void Main()
	{
		int i = 0;
		fixed (int* p = &i)
		{
			*p = 1;
		}
	}
}
