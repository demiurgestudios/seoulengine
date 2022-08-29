using static SlimCS;

class Test
{
	static void TestIt<T>(string value)
	{
		print(typeof(T).Name);
		print(value);
	}

	class A {}
	interface B {}

	// Regression for a bug with calling generics
	// of a single string arguments, when called
	// with a string literal.
	public static void Main ()
	{
		TestIt<A>("abcd");
		TestIt<B>("efgh");
		TestIt<D.A>("ijk");
		TestIt<D.B>("lmnopq");
	}
}

namespace D
{
	class A {}
	interface B {}
}
