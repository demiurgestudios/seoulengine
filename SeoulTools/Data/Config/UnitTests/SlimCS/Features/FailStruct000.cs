using static SlimCS;
struct A {} // Should trigger a failure, we don't support structs.

// A Fail test (any test with a filename starting with Fail)
// tests that functionality which has been specifically disabled
// in SlimCS triggers a compiler failure.
public static class Test
{
	public static void Main()
	{
	}
}
