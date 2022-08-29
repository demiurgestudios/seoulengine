using static SlimCS;

// Regression for name resolution of overloaded generic types.
class Test
{
	class A
	{
		public object m_Value = new object();

		public A()
		{
		}
	}

	class A<T>
	{
		public T m_Value = default(T);

		public A()
		{
		}
	}

	public static void Main ()
	{
		var a = new A();
		print(a.m_Value);
		print(a);
		var b = new A<bool?>();
		print(b.m_Value);
		print(b);
		var d = new A<double?>();
		print(d.m_Value);
		print(d);
		var i = new A<int?>();
		print(i.m_Value);
		print(i);
	}
}
