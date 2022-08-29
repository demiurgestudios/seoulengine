using static SlimCS;

class Test
{
	static string GetName<T>()
	{
		return typeof(T).Name;
	}

	class A {}
	interface B {}

	public static void Main ()
	{
		print(GetName<A>());
		print(GetName<B>());
		print(GetName<D.A>());
		print(GetName<D.B>());
	}
}

namespace D
{
	class A {}
	interface B {}
}
