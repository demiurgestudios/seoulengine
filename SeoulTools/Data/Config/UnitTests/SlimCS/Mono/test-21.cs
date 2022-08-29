using System;

public class Blah {

	public class Foo {

		public Foo ()
		{
			SlimCS.print ("Inside the Foo constructor now");
		}
		
		public int Bar (int i, int j)
		{
			SlimCS.print ("The Bar method");
			return i+j;
		}
		
		
	}

	public static int Main ()
	{
		Foo f = new Foo ();

		int j = f.Bar (2, 3);
		SlimCS.print ("Blah.Foo.Bar returned " + j);
		
		if (j == 5)
			return 0;
		else
			return 1;

	}

}
