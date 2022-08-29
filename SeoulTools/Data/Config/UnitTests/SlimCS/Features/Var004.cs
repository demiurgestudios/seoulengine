using static SlimCS;
public class MyClass : System.IDisposable
{
	private string s;
	public MyClass (string s)
	{
		print(s);
		this.s = s;
	}
	public void Dispose()
	{
		print("Dispose");
		s = "";
	}
}

public class Test
{
	public static int Main ()
	{
		using (var v = new MyClass("foo"))
			if (v.GetType() != typeof (MyClass))
				return 1;

		return 0;
	}
}
