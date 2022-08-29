using static SlimCS;
using static A;

class A
{
	public class N
	{
	}
}

class Test
{
	public static void Main ()
	{
		N n = default (N); // Am I Int32 or A.N
		print(n);
	}
}
