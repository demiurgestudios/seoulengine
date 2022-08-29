using static SlimCS;
// A Fail test (any test with a filename starting with Fail)
// tests that functionality which has been specifically disabled
// in SlimCS triggers a compiler failure.
public static class Test
{
	public struct A {} // Should trigger a failure, we don't support structs.

	public static void Main()
	{
	}
}
