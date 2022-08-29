using static SlimCS;
public class Test
{
	Test a;

	void A()
	{
		print(a);
		{
			string a = "dd";
			print(a);
		}

		{
			a = null;
			print(a);
		}

		a = new Test();
		print(a);
	}

	public static void Main()
	{
		(new Test()).A();
	}
}
