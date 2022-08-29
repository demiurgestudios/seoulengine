using static SlimCS;

public sealed class Test
{
	public void Print()
	{
		print("Hello World");
	}

	public sealed class A
	{
		public void CallIt(System.Action a, System.Action<string> b, System.Action<string> c)
		{
			if (null != a) { a(); }
			if (null != b) { b("TestIt"); }
			if (null != c) { c("TestIt2"); }
		}
	}

	public void DoIt()
	{
		var a = new A();
		a.CallIt((System.Action)Print, (System.Action<string>)Print2, null);
	}

	void Print2(string s)
	{
		print(s);
	}

	public static void Main ()
	{
		var t = new Test();
		t.DoIt();
	}
}
