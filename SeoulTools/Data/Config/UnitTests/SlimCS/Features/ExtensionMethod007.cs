using static SlimCS;
public class Prop
{
}

public interface I
{
	void Foo (int[] i, bool b);
}

internal static class HelperExtensions
{
	public static void Foo (this I from, I to)
	{
		print("Here.");
	}
}

class Test
{
	public I Prop {
		get { return null; }
	}

	public int[] Loc {
		get { return null; }
	}

	void TestIt()
	{
		Prop.Foo (null);
	}

	public static void Main ()
	{
		(new Test()).TestIt();
	}
}
