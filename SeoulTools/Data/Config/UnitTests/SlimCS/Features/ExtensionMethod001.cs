using static SlimCS;
public class AdapterType
{
	protected virtual void DoSomething ()
	{
		print("1");
	}
}

public static class Extensions
{
	public static void DoSomething (this AdapterType obj)
	{
		print(obj);
	}
}

public abstract class Dummy : AdapterType
{
	public virtual bool Refresh ()
	{
		AdapterType someObj = null;
		someObj.DoSomething ();
		return true;
	}
}

public static class Test
{
	sealed class T : Dummy
	{
	}

	public static void Main()
	{
		var t = new T();
		print(t.Refresh());
	}
}
