using static SlimCS;

// Regression for typeof(T) inside generic constructor.
class Test
{
	class A<T>
	{
		public A()
		{
			print(typeof(T).Name);
		}
	}

	class B {}

	public static void Main ()
	{
		var a = new A<bool?>();
		var b = new A<bool>();
		var c = new A<double?>();
		var d = new A<double>();
		var e = new A<int?>();
		var f = new A<int>();
		var g = new A<string>();
		var h = new A<B>();
		var i = new A<C>();
	}
}

class C {}