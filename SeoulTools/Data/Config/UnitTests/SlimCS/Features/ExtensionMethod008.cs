using static SlimCS;

interface IEnumerator
{
}

interface IEnumerable
{
	IEnumerator GetEnumerator();
}

class A : IEnumerable
{
	protected int Count
	{
		get
		{
			print(0);
			return 0;
		}
	}

	IEnumerator IEnumerable.GetEnumerator ()
	{
		return null;
	}
}

class G<T> where T : A
{
	public void Test ()
	{
		T var = null;
		print(var);
		int i = var.Count ();
		print(i);
	}
}

static class Test
{
	public static int Count (this IEnumerable seq)
	{
		print(1);
		return 1;
	}

	public static void Main()
	{
		(new G<A>()).Test();
	}
}
