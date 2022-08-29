using static SlimCS;

public sealed class Test
{
	public sealed class A
	{
		System.Action<A> m_Func;
		public System.Action<A> Func { get; set; }
		public System.Action<A> Func2 { get { return m_Func; } set { m_Func = value; } }

		public void Call()
		{
			m_Func(this);
			Func(this);
			Func2(this);
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
	}

	public static void Main ()
	{
		var a = new A();
		var b = new B();
		a.Func = b.Func;
		a.Func2 = b.Func2;
		a.Call();
	}
}
