using static SlimCS;

public sealed class Test
{
	public delegate void D(A a);

	public sealed class A
	{
		D m_D = null;

		public void Add(D d)
		{
			m_D = d;
		}
		public void Remove(D d)
		{
			if (m_D == d)
			{
				m_D = null;
			}
		}

		public void Call()
		{
			if (null != m_D)
			{
				m_D(this);
			}
			else
			{
				print("No Del");
			}
		}
	}

	public sealed class B
	{
		public void Func(A a)
		{
			print(this);
			print(a);
		}

		public void Func2(A a)
		{
			print(a);
			print(this);
		}

		public static void Func3(A a)
		{
			print("STATIC");
			print(a);
		}

		public void Test(A a)
		{
			a.Add(Func);
			a.Call();
			a.Remove(Func);
			a.Call();
			a.Add(Func2);
			a.Call();
			a.Remove(Func2);
			a.Call();
			a.Add(Func3);
			a.Call();
			a.Remove(Func2);
			a.Call();
			a.Remove(Func3);
			a.Call();
		}
	}

	public static void Main ()
	{
		var a = new A();
		var b = new B();
		b.Test(a);
	}
}
