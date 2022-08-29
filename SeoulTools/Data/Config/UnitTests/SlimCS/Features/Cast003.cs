using static SlimCS;
using System;

class Test
{
	class A {}
	class B : A {}
	class C : B {}

	public static void Main ()
	{
		object o;
		o = new A();
		print(o as A);
		print(o is A);
		print(o as B);
		print(o is B);
		print(o as C);
		print(o is C);

		o = new B();
		print(o as A);
		print(o is A);
		print(o as B);
		print(o is B);
		print(o as C);
		print(o is C);

		o = new C();
		print(o as A);
		print(o is A);
		print(o as B);
		print(o is B);
		print(o as C);
		print(o is C);

		o = true;
		print(o as A);
		print(o is A);
		print(o as B);
		print(o is B);
		print(o as C);
		print(o is C);

		o = 1.0;
		print(o as A);
		print(o is A);
		print(o as B);
		print(o is B);
		print(o as C);
		print(o is C);

		o = "Hello World";
		print(o as A);
		print(o is A);
		print(o as B);
		print(o is B);
		print(o as C);
		print(o is C);
	}
}
